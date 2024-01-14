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
	cv::imshow("Source", source);

	cv::waitKey(0);
	return 0;
}
