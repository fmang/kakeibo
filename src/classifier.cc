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
#include <iostream>

struct features {
	std::string path;
	std::string label;
	std::vector<int> values;
};

static features extract_features(const std::filesystem::path& path)
{
	features f;
	f.path = path;
	f.label = path.parent_path().filename();

	cv::Mat pixels = cv::imread(path, cv::IMREAD_GRAYSCALE);
	cv::resize(pixels, pixels, cv::Size(8, 8));
	for (int y = 0; y < pixels.rows; ++y) {
		for (int x = 0; x < pixels.cols; ++x) {
			int value = pixels.at<uchar>(y, x);
			f.values.push_back(value * 9 / 255.);
		}
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
