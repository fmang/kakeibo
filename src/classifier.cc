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
#include <opencv2/ml.hpp>

#include <filesystem>
#include <iostream>

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
	int min_size = cross_size * 2 / 3;
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

struct cell {
	cell(cv::Rect box) : box(box) {}
	cv::Rect box;
	double fill_ratio;
};

struct grid {
	std::vector<cell> horizontal_cells;
	std::vector<cell> vertical_cells;
};

static grid build_grid(int width, int height)
{
	grid g;

	std::vector<int> horizontal_rule = split_axis(width, height);
	for (size_t i = 1; i < horizontal_rule.size(); ++i) {
		int cell_x = horizontal_rule[i - 1];
		int cell_width = horizontal_rule[i] - cell_x;
		g.vertical_cells.emplace_back(cv::Rect(cell_x, 0, cell_width, height));
	}

	std::vector<int> vertical_rule = split_axis(height, width);
	for (size_t i = 1; i < vertical_rule.size(); ++i) {
		int cell_y = vertical_rule[i - 1];
		int cell_height = vertical_rule[i] - cell_y;
		g.horizontal_cells.emplace_back(cv::Rect(0, cell_y, width, cell_height));
	}

	return g;
}

static cell fit_cell(cv::Rect box, cv::Mat image)
{
	cv::Rect shape { 0, 0, image.cols, image.rows };
	box &= shape;
	cv::Mat roi = image(box);
	std::vector<cv::Point> whites;
	cv::findNonZero(roi, whites);

	struct cell fitted_cell = cv::boundingRect(whites);
	fitted_cell.box.x += box.x;
	fitted_cell.box.y += box.y;
	double area = fitted_cell.box.area();
	fitted_cell.fill_ratio = area == 0 ? 0 : whites.size() / area;

	return fitted_cell;
}

static void fit_grid(grid& g, cv::Mat image)
{
	for (struct cell& cell : g.horizontal_cells)
		cell = fit_cell(cell.box, image);

	for (struct cell& cell : g.vertical_cells)
		cell = fit_cell(cell.box, image);
}

struct features {
	std::string path;
	std::string label;
	std::vector<int> values;
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

/**
 * Reçoit un nombre flottant entre 0 et 1, et renvoie un nombre entier entre 0
 * et 9. On prend une racine pour faire ressortir les différences sur les
 * petites valeurs.
 */
int normalize_ratio(double ratio)
{
	return std::cbrt(ratio) * 9;
}

static features extract_features(const std::filesystem::path& path)
{
	features f;
	f.path = path;
	f.label = path.parent_path().filename();

	cv::Mat pixels = cv::imread(path, cv::IMREAD_GRAYSCALE);
	struct grid grid = build_grid(pixels.cols, pixels.rows);
	fit_grid(grid, pixels);
	for (const struct cell& cell : grid.horizontal_cells) {
		f.values.push_back(cell.box.width * 9 / pixels.cols);
		f.values.push_back(normalize_ratio(cell.fill_ratio));
	}
	for (const struct cell& cell : grid.vertical_cells) {
		f.values.push_back(cell.box.height * 9 / pixels.rows);
		f.values.push_back(normalize_ratio(cell.fill_ratio));
	}

	if (debug) {
		static const int scale_factor = 4;
		cv::Mat drawing;
		cv::cvtColor(pixels, drawing, cv::COLOR_GRAY2BGR);
		cv::resize(drawing, drawing, cv::Size(), scale_factor, scale_factor, cv::INTER_NEAREST);
		for (const struct cell& cell : grid.horizontal_cells)
			cv::rectangle(drawing, scale_rect(cell.box, scale_factor), cv::Scalar(255, 0, 0), 1);
		for (const struct cell& cell : grid.vertical_cells)
			cv::rectangle(drawing, scale_rect(cell.box, scale_factor), cv::Scalar(0, 0, 255), 1);
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
		std::printf("%s,%s,", f.path.c_str(), f.label.c_str());
		for (int value : f.values)
			std::printf("%d", value);
		std::printf("\n");
	}
}

struct features_database {
	cv::Mat features;
	std::vector<int> responses;
};

static float parse_feature(char c)
{
	if (c < '0' || c > '9')
		throw std::invalid_argument("bad digit");
	return (c - '0') / 9.;
}

static features_database load_features()
{
	features_database db;
	int features_count = -1;
	std::vector<int> responses_vector;

	for (std::string row; std::getline(std::cin, row);) {
		auto first_comma = std::find(std::begin(row), std::end(row), ',');
		if (first_comma == std::end(row))
			throw std::invalid_argument("expected a 3-column CSV");

		auto label_column = std::next(first_comma);
		auto second_comma = std::find(label_column, std::end(row), ',');
		if (second_comma == std::end(row))
			throw std::invalid_argument("expected a 3-column CSV");

		auto features_column = std::next(second_comma);
		int row_features_count = std::end(row) - features_column;
		if (features_count == -1)
			features_count = row_features_count;
		else if (row_features_count != features_count)
			throw std::invalid_argument("inconsistent features count");

		responses_vector.push_back(*label_column);
		cv::Mat row_features(1, features_count, CV_32F);
		int i = 0;
		for (auto it = features_column; it != std::end(row); ++it)
			row_features.at<float>(i++) = parse_feature(*it);
		db.features.push_back(row_features);
	}

	return db;
}

void train_model()
{
	features_database db = load_features();
	cv::Ptr<cv::ml::TrainData> tdata = cv::ml::TrainData::create(db.features, cv::ml::ROW_SAMPLE, cv::Mat(db.responses));

	cv::Ptr<cv::ml::RTrees> model = cv::ml::RTrees::create();
	model->setMaxDepth(10);
	model->setMinSampleCount(10);
	model->setRegressionAccuracy(0);
	model->setUseSurrogates(false);
	model->setMaxCategories(15);
	model->setPriors(cv::Mat());
	model->setCalculateVarImportance(true);
	model->setActiveVarCount(4);
	cv::TermCriteria tc(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 100, 0.01f);
	model->setTermCriteria(tc);
	model->train(tdata); // segfault ?

	for (int i = 0; i < db.features.rows; i++) {
		cv::Mat sample = db.features.row(i);
		int expected = db.responses[i];
		float result = model->predict(sample);
		std::printf("expected %d, got %f\n", expected, result);
	}
}
