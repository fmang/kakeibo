#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

// Définit la résolution attendue de l’image. Ça affecte les seuils utilisés
// pour les fonctions d’approximation.
static const double dpi = 100;

// Convertit des millimètres en pixels. En définissant les constantes de
// longueur en millimètres, il sera facile d’ajuster la résolution (dpi) sans
// revoir tout le code.
double operator ""_mm(unsigned long long length)
{
	static const double mm_per_inch = 25.4;
	static const double pixels_per_mm = dpi / mm_per_inch;
	return length * pixels_per_mm;
}

// On suppose que le format du papier est A5 en portrait.
static const double paper_width = 148_mm;
static const double paper_height = 210_mm;

// Renvoie la différence relative entre deux valeurs. 0.10 signifie qu’il y a 10 % d’écart.
static double relative_difference(double actual, double expected)
{
	return std::abs(actual - expected) / expected;
}

// Charge l’image passée en argument et la binarise en blanc sur noir.
static cv::Mat load_image(const char* path)
{
	cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
	
	// S’assure qu’on reçoit une image de résolution similaire à ce qu’on attend.
	if (relative_difference(image.rows, paper_height) > 0.20) {
		std::cout << image.rows << std::endl;
		std::string error = "expected image height of " + std::to_string(paper_height) + " (± 20%)";
		throw std::runtime_error(error);
	}

	static const int threshold_block_size = 5_mm;
	static const double threshold_offset = -15;
	cv::adaptiveThreshold(~image, image, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,
	                      threshold_block_size, threshold_offset);

	// Efface les points de bruit.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(2, 2));
	cv::morphologyEx(image, image, cv::MORPH_OPEN, element);

	return image;
}

// Teste la détection de ligne horizontale décrite dans :
// https://docs.opencv.org/4.8.0/dd/dd7/tutorial_morph_lines_detection.html
static void detect_horizontal_lines(cv::Mat source)
{
	cv::Mat horizontal = source.clone();
	cv::Mat horizontal_structure = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(25, 1));
	cv::erode(horizontal, horizontal, horizontal_structure);
	cv::dilate(horizontal, horizontal, horizontal_structure);
	cv::imshow("horizontal", horizontal);
}

// Utilise la détection de lignes de OpenCV. Les lignes tendent à être coupées,
// donc ce n’est pas idéal.
static void detect_lines(cv::Mat image)
{
	cv::imshow("source", image);
	std::vector<cv::Vec4f> lines;
	auto lsd = cv::createLineSegmentDetector(cv::LSD_REFINE_STD);
	lsd->detect(image, lines);
	cv::Mat output = cv::Mat::zeros(image.rows, image.cols, image.depth());
	lsd->drawSegments(output, lines);
	cv::imshow("LSD", output);
}

static void detect_contours(cv::Mat source)
{
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(source, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Mat drawing = cv::Mat::zeros(source.size(), CV_8UC3);
        for (size_t i = 0; i < contours.size(); i++) {
		cv::Scalar color(std::rand() & 255, std::rand() & 255, std::rand() & 255);
		cv::drawContours(drawing, contours, i, color, 2, cv::LINE_8, hierarchy, 0);
	}
	cv::imshow("Contours", drawing);
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	cv::Mat source = load_image(argv[1]);
	cv::imshow("Threshold", source);

	cv::waitKey(0);
	return 0;
}
