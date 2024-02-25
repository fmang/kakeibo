/*
 * Reçoit la photo d’un ticket de caisse en entrée et détecte ses éléments :
 * zones de texte, ligne du total, montant, ainsi que la date à travers les
 * chiffres ou encore depuis le code barre.
 *
 * On suppose une résolution de 10 px / mm.
 */

#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>

/**
 * Représente une ligne de texte. box est la bounding box sur l’image d’entrée.
 * letters est une liste de bounding boxes pour chaque lettre de la ligne,
 * relatives à l’image source.
 */
struct text_line {
	cv::Rect box;
	std::vector<cv::Rect> letters;
};

/**
 * Reçoit une image binaire et la position de la ligne à extraire.
 * Extrait les lettres de la ligne et construit l’objet text_line.
 */
static text_line extract_text_line(cv::Mat binary, cv::Rect line_box)
{
	cv::Size dilatation { /* horizontal */ 1, /* vertical */ 19 };
	cv::Mat extract = cv::Mat(
		line_box.height + 2 * dilatation.height,
		line_box.width + 2 * dilatation.width,
		binary.type()
	);

	cv::copyMakeBorder(
		binary(line_box), extract,
		/* top */ dilatation.height, /* bottom */ dilatation.height,
		/* left */ dilatation.width, /* right */ dilatation.width,
		cv::BORDER_CONSTANT | cv::BORDER_ISOLATED
	);

	// La dilatation verticale permet de rassembler les contours d’une même lettre.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(dilatation.width, dilatation.height));
	cv::morphologyEx(extract, extract, cv::MORPH_CLOSE, element);

	std::vector<cv::Rect> letters;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(extract, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (auto& contour : contours) {
		cv::Rect letter = cv::boundingRect(contour);
		if (letter.area() < 100) // Ignore le bruit.
			continue;

		letter.x += line_box.x - dilatation.width;
		letter.y += line_box.y - dilatation.height;
		letters.push_back(letter);
	}

	// Trier les lettres de gauche à droite.
	auto left_of = [](const cv::Rect& a, const cv::Rect& b) {
		return a.x < b.x;
	};
	std::sort(letters.begin(), letters.end(), left_of);

	return text_line { line_box, letters };
}

/**
 * Reçoit une image binaire et détecte les lignes de texte.
 * Renvoie la liste des text_line trouvés.
 */
static std::vector<text_line> extract_text_lines(cv::Mat binary)
{
	// La dilatation horizontale permet de rassembler les lignes dans un même contour.
	cv::Mat dilated;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(19, 1));
	cv::morphologyEx(binary, dilated, cv::MORPH_CLOSE, element);

	std::vector<text_line> lines;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(dilated, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (auto& contour : contours) {
		cv::Rect box = cv::boundingRect(contour);
		if (box.area() < 50) // Ignore le bruit.
			continue;
		text_line line = extract_text_line(binary, box);
		if (!line.letters.empty())
			lines.push_back(line);
	}

	return lines;
}

/**
 * Affiche pour le debug l’image source avec les lettres encadrées. Les lettres
 * d’une même ligne seront de la même couleur.
 */
static void show_text_lines(cv::Mat source, const std::vector<text_line> lines)
{
	static std::vector<cv::Scalar> colors = {
		cv::Scalar(255, 0, 0),
		cv::Scalar(0, 255, 0),
		cv::Scalar(0, 0, 255),
		cv::Scalar(128, 128, 0),
		cv::Scalar(128, 0, 128),
		cv::Scalar(0, 128, 128),
	};
	int color_index = 0;
	cv::Mat drawing = source.clone();
	for (const text_line& line : lines) {
		cv::Scalar color = colors[++color_index % colors.size()];
		for (const cv::Rect& letter : line.letters)
			cv::rectangle(drawing, letter.tl(), letter.br(), color, 1);
	}
	show("Text lines", drawing);
}

/**
 * Ajoute une partie droite (extra) à une ligne de base.
 * Si extra est à droite, les deux lignes sont inversées sur place.
 * Le résultat est stocké dans base.
 */
static void merge_text_lines(text_line& base, text_line& extra)
{
	if (base.box.x > extra.box.x)
		std::swap(base, extra);
	base.box |= extra.box;
	base.letters.insert(base.letters.end(), extra.letters.begin(), extra.letters.end());
}

/**
 * Renvoie le nombre de pixels de superposition sur la projection horizontale
 * des deux lignes. S’il est négatif, il n’y a aucune superposition.
 */
static int vertical_overlap(const text_line& a, const text_line& b)
{
	int top = std::max(a.box.y, b.box.y);
	int bottom = std::min(a.box.y + a.box.height, b.box.y + b.box.height);
	return bottom - top;
}

/**
 * Fusionne les lignes alignées horizontalement.
 */
static void compact_lines(std::vector<text_line>& lines)
{
	// On veut au moins un élément pour démarrer l’accumulation.
	if (lines.empty())
		return;

	// Trie sur y, de haut en bas.
	auto above = [](const text_line& a, const text_line& b) {
		return a.box.y < b.box.y;
	};
	std::sort(lines.begin(), lines.end(), above);

	std::vector<text_line> compacted_lines;
	auto it = lines.begin();
	text_line* accumulator = &compacted_lines.emplace_back(*it++);
	for (; it != lines.end(); ++it) {
		int overlap = vertical_overlap(*accumulator, *it);
		if (overlap > accumulator->box.height / 2 && overlap > it->box.height / 2)
			merge_text_lines(*accumulator, *it);
		else
			accumulator = &compacted_lines.emplace_back(*it);
	}
	lines = std::move(compacted_lines);
}

static cv::Mat binarize(cv::Mat color)
{
	// Extrait le rouge pour rendre les tampons moins visibles. Les tickets
	// sont monochromes, donc tous les canaux sont plus ou moins égaux.
	cv::Mat binary;
	cv::extractChannel(color, binary, 2); // Canal rouge.
	cv::bitwise_not(binary, binary); // Blanc sur noir.
	cv::adaptiveThreshold(binary, binary, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_MEAN_C, 75, -50);
	return binary;
}

/**
 * Reçoit une image noir et blanc et génère une chaine de 64 chiffres avec les
 * données de l’échantillon en niveau de gris, de résolution 8×8.
 */
static std::string extract_features(cv::Mat sample)
{
	std::string word;
	cv::Mat pixels;
	cv::resize(sample, pixels, cv::Size(8, 8));
	for (int y = 0; y < pixels.rows; ++y) {
		for (int x = 0; x < pixels.cols; ++x) {
			int value = pixels.at<uchar>(y, x);
			int scaled = value * 9 / 255.;
			word.push_back('0' + scaled);
		}
	}
	return word;
}

void scan_receipt(cv::Mat source)
{
	cv::Mat binary = binarize(source);
	std::vector<text_line> lines = extract_text_lines(binary);
	compact_lines(lines);
	if (debug)
		show_text_lines(source, lines);

	for (const text_line& line : lines) {
		bool first = true;
		for (const cv::Rect& letter : line.letters) {
			if (first)
				first = false;
			else
				std::putchar(' ');
			std::string word = extract_features(binary(letter));
			std::fputs(word.c_str(), stdout);
		}
		std::putchar('\n');
	}
}

void extract_samples(cv::Mat source)
{
	cv::Mat binary = binarize(source);
	for (auto& line : extract_text_lines(binary)) {
		for (auto& letter : line.letters)
			save(binary(letter));
	}
}

/**
 * Représente un échantillon stocké dans le dossier samples/, prétraité pour
 * servir à l’entrainement du modèle de reconnaissance de lettre.
 */
struct features {
	std::string path;
	std::string label;
	std::string values;
};

/**
 * Construit un échantillon pour l’apprentissage depuis une image noir et
 * blanc.
 */
static features load_features(const std::filesystem::path& path)
{
	cv::Mat sample = cv::imread(path, cv::IMREAD_GRAYSCALE);
	features f;
	f.path = path;
	f.label = path.parent_path().filename();
	f.values = extract_features(sample);
	return f;
}

/**
 * Fouille toutes les images du dossier samples/ et bâtit un CSV pour entrainer
 * le modèle de reconnaissance de lettres.
 */
void compile_features()
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("samples")) {
		if (!entry.is_regular_file())
			continue;

		std::filesystem::path path = entry.path();
		if (path.extension() != ".png")
			continue;

		features f = load_features(path);
		std::printf("%s,%s,%s\n", f.path.c_str(), f.label.c_str(), f.values.c_str());
	}
}
