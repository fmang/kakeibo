#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include <vector>

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	const char* image_path = argv[1];
	cv::Mat source = cv::imread(image_path, cv::IMREAD_GRAYSCALE);

	// Floute les détails pour que le bruit du fond ne génère pas de bords.
	// Comme nos tables sont en bois, le motif du bois casse tout.
	cv::Mat image;
	cv::GaussianBlur(source, image, cv::Size(5, 5), 0);

	// Détection des bords avec un fort seuil. On s’attend à des tickets
	// blancs sur fond foncé.
	cv::Canny(image, image, 75, 200);

	// Diltation pour que les bords fragiles se solidifient.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::dilate(image, image, element);
	cv::imshow("Dilated", image);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Mat drawing;
	cvtColor(source, drawing, cv::COLOR_GRAY2BGR);
        for (size_t i = 0; i < contours.size(); i++) {
		// Dessiner uniquement les contours rectangles.
		std::vector<cv::Point> poly;
		cv::approxPolyDP(contours[i], poly, 100, true /* closed */);
		if (poly.size() == 4) {
			// TODO: Valider que la taille du contour est suffisamment grande.
			static const cv::Scalar red(0, 0, 255);
			cv::drawContours(drawing, contours, i, red, 2);
			for (cv::Point corner : poly) {
				static const cv::Scalar yellow(0, 255, 255);
				cv::circle(drawing, corner, 5, yellow, 2);
			}
		}
	}
	cv::imshow("Contours", drawing);

	cv::waitKey(0);
	return 0;
}
