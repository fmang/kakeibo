#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

static const int threshold_block_size = 15;
static const double threshold_offset = -20;

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}
	std::string image_path = argv[1];

	cv::Mat gray = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
	cv::Mat binary;
	cv::adaptiveThreshold(~gray, binary, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,
	                      threshold_block_size, threshold_offset);

	cv::imshow("", binary);
	cv::waitKey(0);
	return 0;
}
