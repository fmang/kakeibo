#include "kakeibo.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <getopt.h>
#include <set>

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
			for (auto receipt : cut_receipts(source))
				std::puts(save(receipt).c_str());
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

std::string save(cv::Mat image)
{
	static bool extracted_directory_created = false;
	if (!extracted_directory_created) {
		std::filesystem::create_directories("extracted");
		extracted_directory_created = true;
	}

	static int extracted_count = 0;
	char buffer[32];
	std::snprintf(buffer, 32, "extracted/%03d.jpg", ++extracted_count);
	std::string output_file = buffer;
	cv::imwrite(output_file, image);
	return output_file;
}

void show(const std::string& name, cv::Mat image)
{
	if (!debug)
		return;

	static std::set<std::string> skip;
	if (skip.contains(name))
		return;

	cv::imshow("Kakeibo", image);
	int key = cv::waitKey(0);
	switch (key) {
	case ' ': break;
	case 's': save(image); break;
	default: skip.insert(name);
	}
}
