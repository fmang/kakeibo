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
bool cut = false;

static const char* usage =
	"Usage: receipt-scanner [--cut] [--scan|--extract] FICHIER…\n"
	"       receipt-scanner --compile DOSSIER\n"
	"       receipt-scanner --help\n"
;

static const char* help =
	"Options :\n"
	"       --cut           Découpe les reçus contenus dans l’image.\n"
	"       --scan          Sort le contenu du reçu sous forme textuelle.\n"
	"       --extract       Extrait chaque lettre du reçu en image indivuelle.\n"
	"       --compile       Compile une collection d’échantillons en CSV.\n"
	"       --help          Affiche cette aide.\n"
	"\n"
	"Le mode par défaut est --cut --scan, qui a pour effet d’écrire sur la sortie\n"
	"standard tous les reçus d’une photo sous forme textuelle.\n"
	"\n"
	"--cut extrait et enregistre chaque reçu des photos. --scan reçoit des reçus et\n"
	"en extrait le texte. --extract reçoit des reçus et en extrait les morceaux\n"
	"d’images utilisés pour la reconnaissance de lettres. Combiner --cut aux autres\n"
	"modes permet d’opérer sur des photos plutôt que des reçus déjà extraits.\n"
	"\n"
	"--compile reçoit un dossier dont le nom de chaque sous-dossier sert d’étiquette\n"
	"et dans lesquels chaque fichier est une image échantillon. Ces échantillons\n"
	"sous compilés en CSV, écrit sur la sortie standard.\n"
;

static struct option options[] = {
	{ "cut", no_argument, 0, 'c' },
	{ "scan", no_argument, 0, 's' },
	{ "extract", no_argument, 0, 'x' },
	{ "compile", no_argument, 0, 'C' },
	{ "explain", no_argument, 0, 'e' },
	{ "help", no_argument, 0, 'h' },
	{}
};

static void bad_usage(const char* message = nullptr)
{
	if (message)
		std::fputs(message, stderr);
	std::fputs(usage, stderr);
	std::exit(2);
}

/**
 * Reçoit l’image d’un reçu et le traite selon le mode choisi par
 * l’utilisateur. Si --cut est passé, on reçoit chaque reçu pré-découpé.
 * Autrement, on reçoit l’image d’entrée.
 */
static void process_receipt(cv::Mat receipt)
{
	static bool first_receipt = true;

	switch(mode) {
	case 'c':
		std::puts(save(receipt).c_str());
		break;

	case 's':
		if (!first_receipt)
			puts(""); // Sépare les reçus d’une ligne vide.
		scan_receipt(receipt);
		break;

	case 'x':
		extract_letters(receipt);
		break;
	}

	first_receipt = false;
}

int main(int argc, char** argv)
{
	for (;;) {
		int c = getopt_long(argc, argv, "", options, nullptr);
		if (c == -1)
			break;
		switch (c) {
		case 'c':
			cut = true;
			break;
		case 'C':
		case 's':
		case 'x':
			if (mode != 0)
				bad_usage("Le mode ne peut être spécifié qu’une fois.\n");
			mode = c;
			break;
		case 'e':
			explain = true;
			break;
		case 'h':
			puts(usage);
			puts(help);
			return 0;
		default:
			bad_usage();
		}
	}

	// En l’absence de mode, si --cut est spécifié on utilise le mode
	// cut-only. Sinon, on utilise le mode cut-scan.
	if (mode == 0) {
		mode = cut ? 'c' : 's';
		cut = true;
	}

	if (cut && !(mode == 'c' || mode == 's' || mode == 'x' || mode == 'l'))
		bad_usage("--cut n’est pas compatible avec le mode spécifié.\n");

	switch(mode) {
	case 'c':
	case 's':
	case 'x':
	case 'l':
		if (optind == argc)
			bad_usage("Aucun fichier à traiter.\n");

		for (int argi = optind; argi < argc; ++argi) {
			const char* image_path = argv[argi];
			cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);
			if (cut) {
				for (auto contour : find_receipts(source))
					process_receipt(cut_receipt(source, contour));
			} else {
				process_receipt(source);
			}
		}
		break;

	case 'C':
		if (optind == argc)
			bad_usage("Un dossier d’échantillons est requis.\n");
		else if (argc - optind > 1)
			bad_usage("Trop d’arguments.\n");

		compile_features(argv[optind]);
		break;
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
