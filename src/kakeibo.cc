#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <cstdlib>
#include <getopt.h>

static const char* usage = \
	"Usage: kakeibo --cut FICHIER…\n"
;

static struct option options[] = {
	{ "cut", no_argument, 0, 'c' },
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
	char mode = 0;

	for (;;) {
		int c = getopt_long(argc, argv, "", options, nullptr);
		if (c == -1)
			break;
		switch (c) {
		case 'c':
			if (mode != 0)
				bad_usage("Le mode ne peut être spécifié qu’une fois.\n");
			mode = c;
			break;
		default:
			bad_usage();
		}
	}

	if (mode == 0)
		bad_usage("Aucun mode spécifié.\n");

	if (optind == argc)
		bad_usage("Aucun fichier d’entrée spécifié.\n");

	for (int argi = optind; argi < argc; ++argi) {
		const char* image_path = argv[argi];
		cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
		for (auto receipt : extract_receipts(source))
			std::puts(save_extract(receipt).c_str());
	}

	return 0;
}
