#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	std::string image_path = argv[1];
	cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
	cv::imshow("", img);
	cv::waitKey(0);
	return 0;
}
