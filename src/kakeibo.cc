#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <cstdlib>
#include <getopt.h>

bool debug = false;
char mode = 0;

static const char* usage = \
	"Usage: kakeibo --cut FICHIER…\n"
	"       kakeibo --extract FICHIER…\n"
;

static struct option options[] = {
	{ "cut", no_argument, 0, 'c' },
	{ "extract", no_argument, 0, 'x' },
	{ "debug", no_argument, 0, 'd' },
	{}
};

static void bad_usage(const char* message = nullptr)
{
	if (message)
		std::fputs(message, stderr);
	std::fputs(usage, stderr);
	std::exit(2);
}

int main(int argc, char** argv)
{
	for (;;) {
		int c = getopt_long(argc, argv, "", options, nullptr);
		if (c == -1)
			break;
		switch (c) {
		case 'c':
		case 'x':
			if (mode != 0)
				bad_usage("Le mode ne peut être spécifié qu’une fois.\n");
			mode = c;
			break;
		case 'd':
			debug = true;
			break;
		default:
			bad_usage();
		}
	}

	switch(mode) {
	case 'c':
		for (int argi = optind; argi < argc; ++argi) {
			const char* image_path = argv[argi];
			cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
			for (auto receipt : extract_receipts(source))
				std::puts(save_extract(receipt).c_str());
		}
		break;

	case 'x':
		for (int argi = optind; argi < argc; ++argi) {
			const char* image_path = argv[argi];
			cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
			scan_receipt(source);
		}
		break;

	default:
		bad_usage("Aucun mode spécifié.\n");
	}

	return 0;
}
