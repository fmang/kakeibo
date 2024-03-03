#include "kakeibo.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <getopt.h>
#include <set>

/** Si activé via --explain, affiche visuellement les données traitées. */
bool explain = false;

char mode = 0;

static const char* usage = \
	"Usage: receipt-scanner --cut FICHIER…\n"
	"       receipt-scanner --scan FICHIER…\n"
	"       receipt-scanner --extract FICHIER…\n"
	"       receipt-scanner --compile DOSSIER\n"
;

static struct option options[] = {
	{ "cut", no_argument, 0, 'c' },
	{ "scan", no_argument, 0, 's' },
	{ "extract", no_argument, 0, 'x' },
	{ "compile", no_argument, 0, 'C' },
	{ "train", no_argument, 0, 't' },
	{ "explain", no_argument, 0, 'e' },
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
		case 'C':
		case 's':
		case 't':
		case 'x':
			if (mode != 0)
				bad_usage("Le mode ne peut être spécifié qu’une fois.\n");
			mode = c;
			break;
		case 'e':
			explain = true;
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
			for (auto contour : find_receipts(source)) {
				cv::Mat receipt = cut_receipt(source, contour);
				std::puts(save(receipt).c_str());
			}
		}
		break;

	case 's':
		for (int argi = optind; argi < argc; ++argi) {
			const char* image_path = argv[argi];
			cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
			scan_receipt(source);
		}
		break;

	case 'x':
		for (int argi = optind; argi < argc; ++argi) {
			const char* image_path = argv[argi];
			cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
			extract_samples(source);
		}
		break;

	case 'C':
		if (optind == argc)
			bad_usage("Un dossier d’échantillons est requis.\n");
		else if (argc - optind > 1)
			bad_usage("Trop d’arguments.\n");

		compile_features(argv[optind]);
		break;

	default:
		bad_usage("Aucun mode spécifié.\n");
	}

	return 0;
}

/**
 * Enregistre l’image dans un fichier extracted/0123.png. Renvoie le nom du
 * fichier de sortie.
 */
std::string save(cv::Mat image)
{
	static bool extracted_directory_created = false;
	if (!extracted_directory_created) {
		std::filesystem::create_directories("extracted");
		extracted_directory_created = true;
	}

	static int extracted_count = 0;
	char buffer[32];
	std::snprintf(buffer, 32, "extracted/%04d.png", ++extracted_count);
	std::string output_file = buffer;
	cv::imwrite(output_file, image);
	return output_file;
}

/**
 * Ouvre une fenêtre affichant l’image. Au plus une image à la fois est
 * affichée. Attend que l’utilisateur appuie sur une touche pour passer à
 * l’image suivante. Si l’utilisateur appuie sur une autre touche qu’Espace,
 * toutes les demandes d’affichage successives de la même image seront
 * ignorées.
 *
 * Nécessite --explain pour être activée.
 */
void show(const std::string& name, cv::Mat image)
{
	if (!explain)
		return;

	static std::set<std::string> skip;
	if (skip.contains(name))
		return;

	cv::imshow("Kakeibo", image);
	int key = cv::waitKey(0);
	switch (key) {
	case ' ': break;
	case 's': save(image); break;
	case 'q': explain = false; break;
	default: skip.insert(name);
	}
}
