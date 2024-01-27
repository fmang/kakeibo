/*
 * Reçoit la photo d’un ticket de caisse en entrée et extrait la date et le montant payé.
 * On suppose une résolution de 10 px / mm.
 */

#include "kakeibo.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

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
		cv::Scalar color(0, 255, 0);
		cv::Rect box = cv::boundingRect(contour);
		double aspect = static_cast<double>(box.width) / box.height;
		if (box.width < 40 || box.height < 20 ||
		    box.width > 150 || box.height > 100 ||
		    aspect < 2 || aspect > 6 ||
		    box.x + box.width > red.cols / 2)
			color = cv::Scalar(0, 0, 255);

		if (debug)
			cv::rectangle(drawing, box.tl(), box.br(), color, 2);
	}

	if (debug) {
		cv::imshow("Red", red);
		cv::imshow("Countours", drawing);
		cv::waitKey(0);
	}
}
