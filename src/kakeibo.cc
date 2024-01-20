#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <filesystem>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::fprintf(stderr, "Usage: %s FILE…\n", argv[0]);
		return 2;
	}

	int extracted_count = 0;
	std::filesystem::create_directories("extracted");

	for (int argi = 1; argi < argc; ++argi) {
		const char* image_path = argv[argi];
		cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
		
		for (auto receipt : extract_receipts(source)) {
			std::string output_file = "extracted/" + std::to_string(++extracted_count) + ".jpg";
			std::puts(output_file.c_str());
			cv::imwrite(output_file, receipt);
		}
	}

	return 0;
}
