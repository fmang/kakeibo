/*
 * Reçoit la photo d’un ticket de caisse en entrée et extrait la date et le montant payé.
 * On suppose une résolution de 10 px / mm.
 */

#include "kakeibo.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

/**
 * Supprime tous les pixels noirs autour d’une image.
 */
static cv::Mat trim(cv::Mat image)
{
	cv::Mat binary;
	cv::cvtColor(image, binary, cv::COLOR_BGR2GRAY);
	cv::threshold(binary, binary, 128, 255, cv::THRESH_BINARY);
	cv::bitwise_not(binary, binary);

	// Efface les pixels de bruit.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
	cv::morphologyEx(binary, binary, cv::MORPH_OPEN, element);

	std::vector<cv::Point> whites;
	cv::findNonZero(binary, whites);
	cv::Rect box = cv::boundingRect(whites);
	return image(box);
}

static void extract_letter(cv::Mat fragment)
{
	cv::Mat letter = trim(fragment);
	if (letter.empty())
		return;

	static bool show = debug;
	if (show) {
		cv::imshow("Letter", letter);
		int key = cv::waitKey(0);
		show = (key == 32); // Espace.
	}

	save_extract(letter);
}

/**
 * Reçoit un fragment d’image contenant par exemple « 合計 », et extrait chaque
 * lettre individuelle. On suppose que le fragment contient exactement 2
 * lettres.
 */
static void extract_letters(cv::Mat fragment)
{
	int half = fragment.cols / 2;
	cv::Mat left = fragment(cv::Range::all(), cv::Range(0, half));
	cv::Mat right = fragment(cv::Range::all(), cv::Range(half, fragment.cols));
	extract_letter(left);
	extract_letter(right);
}

void scan_receipt(cv::Mat source)
{
	// Extrait le rouge pour rendre les tampons moins visibles. Les tickets
	// sont monochromes, donc tous les canaux sont plus ou moins égaux.
	cv::Mat red;
	cv::extractChannel(source, red, 2);
	cv::bitwise_not(red, red);
	cv::adaptiveThreshold(red, red, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_MEAN_C, 75, -50);

	// La dilatation horizontale permet de faire en sorte que les mots se
	// collent. La détection de contour nous donnera alors un contour par
	// mot, plutôt qu’un par lettre. Ça permet aussi d’unifier les kanjis.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(20, 1));
	cv::dilate(red, red, element);

	cv::Mat drawing = debug ? source.clone() : cv::Mat();

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(red, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (auto& contour : contours) {
		cv::Rect box = cv::boundingRect(contour);
		double aspect = static_cast<double>(box.width) / box.height;
		bool promising =
			box.width > 40 && box.height > 20 &&
			box.width < 150 && box.height < 100 &&
			aspect > 2 && aspect < 6 &&
			box.x + box.width < red.cols / 2;

		if (promising)
			extract_letters(source(box));

		if (debug)
			cv::rectangle(drawing, box.tl(), box.br(), promising ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2);
	}

	if (debug) {
		cv::imshow("Red", red);
		cv::imshow("Countours", drawing);
		cv::waitKey(0);
	}
}
