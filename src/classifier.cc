/**
 * Plutôt que de comparer naïvement les pixels des échantillons, essayons d’en
 * extraire les propriétés. Le papier ci-dessous propose de faire des
 * projections sur des cellules, ce qui parait être une bonne idée. Comme les
 * kanjis ont facilement des barres au milieu, prenons un nombre impair de
 * cellules. Pour insister sur les bords, on pourrait aussi faire en sorte que
 * les cellules du bord soient plus petites, et celle du centre plus grande.
 *
 * Concernant le scaling, visons un format standard carré. Ceci dit, pour
 * éviter que les glyphes étroits comme 1 soient trop déformés, interdisons le
 * scaling passée une limite, et complétons avec des bords noirs.
 *
 * Comme les images sont de basse résolution, plutôt que d’appliquer le scaling
 * sur l’image source, appliquons-le sur la grille qui servira à la projection.
 * Sur une image carrée, les cellules feront 1, 2, 3, 2, et 1 neuvième de
 * l’image pour chaque dimension. Les 5 cellules cumulées ne devront cependant
 * pas faire moins de 2/3 de l’autre dimension. Si elles sortent de l’image, on
 * considèrera que la projection vaut 0.
 *
 * Acceptons --debug avec show() pour débugger visuellement l’extraction des
 * features.
 *
 * Référence :
 * - Rapid Feature Extraction for Optical Character Recognition
 *   <https://arxiv.org/pdf/1206.0238.pdf>
 */

#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>

/**
 * Indique comment découper une image en 5 cellules selon une dimension de
 * l’image, et la dimension croisée. La 2e dimension sert à éviter de trop
 * étirer l’image.
 *
 * Renvoie 6 coordonnées définissant les bords des 5 cellules.
 */
static std::vector<int> split_axis(int main_size, int cross_size)
{
	int middle = main_size / 2;
	int min_size = cross_size / 2;
	main_size = std::max(main_size, min_size);

	int center_size = main_size / 3;
	int lateral_size = (main_size - center_size) / 3;

	return {
		middle - main_size / 2,
		middle - center_size / 2 - lateral_size,
		middle - center_size / 2,
		middle + center_size / 2,
		middle + center_size / 2 + lateral_size,
		middle + main_size / 2 + main_size % 2,
	};
}

struct grid {
	std::vector<cv::Rect> horizontal_cells;
	std::vector<cv::Rect> vertical_cells;
};

static grid build_grid(int width, int height)
{
	grid g;

	std::vector<int> horizontal_rule = split_axis(width, height);
	for (size_t i = 1; i < horizontal_rule.size(); ++i) {
		int cell_x = horizontal_rule[i - 1];
		int cell_width = horizontal_rule[i] - cell_x;
		g.vertical_cells.emplace_back(cell_x, 0, cell_width, height);
	}

	std::vector<int> vertical_rule = split_axis(height, width);
	for (size_t i = 1; i < vertical_rule.size(); ++i) {
		int cell_y = vertical_rule[i - 1];
		int cell_height = vertical_rule[i] - cell_y;
		g.horizontal_cells.emplace_back(0, cell_y, width, cell_height);
	}

	return g;
}

static cv::Rect fit_cell(cv::Rect cell, cv::Mat image)
{
	cv::Rect shape { 0, 0, image.cols, image.rows };
	cell &= shape;
	cv::Mat roi = image(cell);
	std::vector<cv::Point> whites;
	cv::findNonZero(roi, whites);
	cv::Rect fitted_cell = cv::boundingRect(whites);
	fitted_cell.x += cell.x;
	fitted_cell.y += cell.y;
	return fitted_cell;
}

static void fit_grid(grid& g, cv::Mat image)
{
	for (cv::Rect& cell : g.horizontal_cells)
		cell = fit_cell(cell, image);

	for (cv::Rect& cell : g.vertical_cells)
		cell = fit_cell(cell, image);
}

struct features {
	std::string path;
	std::string label;
};

static cv::Rect scale_rect(const cv::Rect& rect, int factor)
{
	return cv::Rect {
		rect.x * factor,
		rect.y * factor,
		rect.width * factor,
		rect.height * factor,
	};
}

static features extract_features(const std::filesystem::path& path)
{
	features f;
	f.path = path;
	f.label = path.parent_path().filename();

	cv::Mat pixels = cv::imread(path, cv::IMREAD_GRAYSCALE);
	struct grid grid = build_grid(pixels.cols, pixels.rows);
	fit_grid(grid, pixels);

	if (debug) {
		static const int scale_factor = 4;
		cv::Mat drawing;
		cv::cvtColor(pixels, drawing, cv::COLOR_GRAY2BGR);
		cv::resize(drawing, drawing, cv::Size(), scale_factor, scale_factor, cv::INTER_NEAREST);
		for (const cv::Rect& cell : grid.horizontal_cells)
			cv::rectangle(drawing, scale_rect(cell, scale_factor), cv::Scalar(255, 0, 0), 1);
		for (const cv::Rect& cell : grid.vertical_cells)
			cv::rectangle(drawing, scale_rect(cell, scale_factor), cv::Scalar(0, 0, 255), 1);
		show(f.label, drawing);
	}

	return f;
}

void compile_features()
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("samples")) {
		if (!entry.is_regular_file())
			continue;

		std::filesystem::path path = entry.path();
		if (path.extension() != ".png")
			continue;

		features f = extract_features(path);
		std::printf("%s,%s\n", f.path.c_str(), f.label.c_str());
	}
}
