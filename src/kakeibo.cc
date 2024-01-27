#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::fprintf(stderr, "Usage: %s FILE…\n", argv[0]);
		return 2;
	}

	for (int argi = 1; argi < argc; ++argi) {
		const char* image_path = argv[argi];
		cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
		for (auto receipt : extract_receipts(source))
			std::puts(save_extract(receipt).c_str());
	}

	return 0;
}
