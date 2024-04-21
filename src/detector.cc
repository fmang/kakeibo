/*
 * Reçoit la photo d’un ticket de caisse et en extrait les lignes de texte,
 * puis chaque lettre dans ces lignes. Chaque lettre est appelée échantillon
 * vis-à-vis de l’algorithme de classification. On extrait ensuite les features
 * de chaque échantillon sous forme textuelle pour servir d’entrée au module
 * kakeibo.classifier.
 *
 * Avec --extract, on enregistre chaque lettre découpée en image individuelle.
 * Ces images groupées en dossier, --compile permet de générer un CSV servant à
 * l’apprentissage. Enfin, --scan sort sous forme textuelle les features de
 * toutes les lettres trouvées. Chaque ligne de texte est une ligne du reçu, et
 * chaque mot (features) est une lettre.
 *
 * Toute la partie apprentissage machine est hors de ce module. On s’occupe ici
 * uniquement du découpage des lettres et le l’extraction des features.
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
	void sort();
};

/**
 * Trie les lettres de gauche à droite.
 */
void text_line::sort()
{
	auto left_of = [](const cv::Rect& a, const cv::Rect& b) {
		return a.x < b.x;
	};
	std::sort(letters.begin(), letters.end(), left_of);
}

/**
 * Reçoit une image binaire et le contour de la ligne à extraire.
 * Extrait les lettres de la ligne et construit l’objet text_line.
 */
static text_line extract_text_line(cv::Mat binary, const std::vector<cv::Point>& contour)
{
	std::vector<cv::Point> hull;
	cv::convexHull(contour, hull);
	cv::Rect line_box = cv::boundingRect(hull);
	if (line_box.area() < 50) // Ignore le bruit.
		return {};

	cv::Size dilatation { /* horizontal */ 1, /* vertical */ 19 };
	cv::Mat extract = cv::Mat(
		line_box.height + 2 * dilatation.height,
		line_box.width + 2 * dilatation.width,
		binary.type(),
		cv::Scalar(0)
	);

	cv::Mat input_roi = binary(line_box);
	cv::Mat input_mask = cv::Mat(line_box.height, line_box.width, CV_8UC1, cv::Scalar(0));
	for (cv::Point& p : hull) p -= line_box.tl();
	cv::fillConvexPoly(input_mask, hull, cv::Scalar(255));

	// Copie la ligne qui nous intéresse au milieu de *extract*, avec les
	// bordures pour la dilatation. Le masque sert à ne pas prendre de
	// morceaux des lignes autour si la ligne est de travers.
	cv::Rect output_box(dilatation.width, dilatation.height, line_box.width, line_box.height);
	cv::Mat output_roi = extract(output_box);
	input_roi.copyTo(output_roi, input_mask);

	// La dilatation verticale permet de rassembler les contours d’une même lettre.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(dilatation.width, dilatation.height));
	cv::morphologyEx(extract, extract, cv::MORPH_CLOSE, element);

	std::vector<cv::Rect> letters;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(extract, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (auto& contour : contours) {
		cv::Rect letter = cv::boundingRect(contour);
		if (letter.width < 10 && letter.height < 10) // Ignore le bruit.
			continue;

		letter.x += line_box.x - dilatation.width;
		letter.y += line_box.y - dilatation.height;
		letters.push_back(letter);
	}

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
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(19, 5));
	cv::morphologyEx(binary, dilated, cv::MORPH_CLOSE, element);

	std::vector<text_line> lines;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(dilated, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (auto& contour : contours) {
		text_line line = extract_text_line(binary, contour);
		if (line.letters.empty())
			continue;

		// Les lignes avec une seule lettre sont vraisemblablement du
		// bruit, ou un morceau de lettre qui n’a pas été attrapé dans
		// le contour de la ligne.
		if (line.letters.size() == 1 && line.letters[0].area() < 200)
			continue;

		lines.push_back(line);
	}

	return lines;
}

/**
 * Dessine sur la photo source les countours des lettres encadrés. Les lettres
 * d’une même ligne seront de la même couleur.
 */
static void draw_text_lines(cv::Mat drawing, const std::vector<text_line> lines)
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
	for (const text_line& line : lines) {
		cv::Scalar color = colors[++color_index % colors.size()];
		for (const cv::Rect& letter : line.letters)
			cv::rectangle(drawing, letter.tl(), letter.br(), color, 1);
	}
}

/**
 * Ajoute une partie droite (extra) à une ligne de base.
 * Le résultat est stocké dans base.
 */
static void merge_text_lines(text_line& base, text_line& extra)
{
	base.box |= extra.box;
	base.letters.insert(base.letters.end(), extra.letters.begin(), extra.letters.end());
}

/**
 * Renvoie le nombre de pixels de superposition sur la projection verticale
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
	cv::adaptiveThreshold(binary, binary, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_MEAN_C, 75, -30);
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
	cv::morphologyEx(binary, binary, cv::MORPH_OPEN, element);
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

/**
 * Extrait les lettres d’un reçu et écrit sur la sortie standard le contenu du
 * reçu en forme textuelle pour servir d’entrée à kakeibo.classifier --decode.
 */
void scan_receipt(cv::Mat source)
{
	cv::Mat binary = binarize(source);
	std::vector<text_line> lines = extract_text_lines(binary);
	compact_lines(lines);

	if (explain) {
		cv::Mat drawing = source.clone();
		draw_text_lines(drawing, lines);
		show("detection", drawing);
	}

	for (text_line& line : lines) {
		bool first = true;
		line.sort();
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

/**
 * Extrait dans pleins de petits fichiers chaque lettre contenu dans le reçu.
 * Ces images sont destinées à servir d’échantillons pour le moteur de
 * reconnaissance de lettres.
 */
void extract_letters(cv::Mat source)
{
	cv::Mat binary = binarize(source);
	std::vector<text_line> lines = extract_text_lines(binary);
	compact_lines(lines);
	for (text_line& line : lines) {
		line.sort();
		for (const cv::Rect& letter : line.letters)
			save(binary(letter));
	}
}

/**
 * Représente un échantillon stocké dans le dossier samples/, prétraité pour
 * servir à l’entrainement du modèle de reconnaissance de lettres.
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
 * Fouille toutes les images du dossier passé en argument et bâtit un CSV pour
 * entrainer le modèle de reconnaissance de lettres. Ce format est accepté par
 * kakeibo.classifier --train.
 */
void compile_features(const char* path)
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path)) {
		if (!entry.is_regular_file())
			continue;

		std::filesystem::path path = entry.path();
		if (path.extension() != ".png")
			continue;

		features f = load_features(path);
		std::printf("%s,%s,%s\n", f.path.c_str(), f.label.c_str(), f.values.c_str());
	}
}
