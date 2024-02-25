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
