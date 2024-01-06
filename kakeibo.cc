#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

// Charge l’image passée en argument et la binarise en blanc sur noir.
static cv::Mat load_image(const char* path)
{
	static const int threshold_block_size = 15;
	static const double threshold_offset = -20;
	cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
	cv::adaptiveThreshold(~image, image, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,
	                      threshold_block_size, threshold_offset);
	return image;
}

// Teste la détection de ligne horizontale décrite dans :
// https://docs.opencv.org/4.8.0/dd/dd7/tutorial_morph_lines_detection.html
static void detect_horizontal_lines(cv::Mat source)
{
	cv::Mat horizontal = source.clone();
	int horizontal_size = horizontal.cols / 30;
	cv::Mat horizontal_structure = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(horizontal_size, 1));
	cv::erode(horizontal, horizontal, horizontal_structure, cv::Point(-1, -1));
	cv::dilate(horizontal, horizontal, horizontal_structure, cv::Point(-1, -1));
	cv::imshow("horizontal", horizontal);
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	cv::Mat source = load_image(argv[1]);
	detect_horizontal_lines(source);

	cv::waitKey(0);
	return 0;
}
