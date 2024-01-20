#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>

#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " FILE..." << std::endl;
		return 2;
	}

	int extracted_count = 0;
	for (int argi = 1; argi < argc; ++argi) {
		const char* image_path = argv[argi];
		cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
		
		for (auto receipt : extract_receipts(source)) {
			std::string output_file = "extracted-" + std::to_string(++extracted_count) + ".jpg";
			std::cout << output_file << std::endl;
			cv::imwrite(output_file, receipt);
		}
	}

	return 0;
}
