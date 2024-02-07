/*
 * Reçoit la photo d’un ticket de caisse en entrée et détecte ses éléments :
 * zones de texte, ligne du total, montant, ainsi que la date à travers les
 * chiffres ou encore depuis le code barre.
 *
 * On suppose une résolution de 10 px / mm.
 */

#include "kakeibo.h"

#include <opencv2/imgproc.hpp>

/**
 * Représente une ligne de texte. box est la bounding box sur l’image d’entrée.
 * letters est une liste de bounding boxes pour chaque lettre de la ligne.
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
		if (letter.area() < 25) // Ignore le bruit.
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
		if (box.area() < 25) // Ignore le bruit.
			continue;
		lines.push_back(extract_text_line(binary, box));
	}

	return lines;
}

/**
 * Affiche pour le debug l’image source avec les lignes encadrées en rouge et
 * les lettre en bleu.
 */
static void show_text_lines(cv::Mat source, const std::vector<text_line> lines)
{
	cv::Mat drawing = source.clone();
	for (const text_line& line : lines) {
		cv::rectangle(drawing, line.box.tl(), line.box.br(), cv::Scalar(0, 0, 255), 2);
		for (const cv::Rect& letter : line.letters)
			cv::rectangle(drawing, letter.tl(), letter.br(), cv::Scalar(255, 0, 0), 1);
	}
	show("Text lines", drawing);
}

void scan_receipt(cv::Mat source)
{
	// Extrait le rouge pour rendre les tampons moins visibles. Les tickets
	// sont monochromes, donc tous les canaux sont plus ou moins égaux.
	cv::Mat binary;
	cv::extractChannel(source, binary, 2); // Canal rouge.
	cv::bitwise_not(binary, binary); // Blanc sur noir.
	cv::adaptiveThreshold(binary, binary, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_MEAN_C, 75, -50);

	std::vector<text_line> lines = extract_text_lines(binary);
	if (debug)
		show_text_lines(source, lines);
}
