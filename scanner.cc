/*
 * Reçoit la photo d’un ticket de caisse en entrée et extrait la date et le montant payé.
 * On suppose une résolution de 10 px / mm.
 */

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	const char* image_path = argv[1];
	cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);

	// Extrait le rouge pour rendre les tampons moins visibles. Les tickets
	// sont monochromes, donc tous les canaux sont plus ou moins égaux.
	cv::Mat red;
	cv::extractChannel(source, red, 2);
	cv::bitwise_not(red, red);
	cv::adaptiveThreshold(red, red, 255, cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_MEAN_C, 75, -50);

	cv::Mat drawing = source.clone();
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(red, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (auto& contour : contours) {
		cv::Rect box = cv::boundingRect(contour);
		cv::rectangle(drawing, box.tl(), box.br(), cv::Scalar(0, 0, 255), 2);
	}

	cv::imshow("Red", red);
	cv::imshow("Countours", drawing);
	cv::waitKey(0);
	return 0;
}
