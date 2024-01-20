/*
 * Reçoit une photo contenant un ou plusieurs tickets de caisse et extrait
 * chaque ticket individuellement.
 *
 * Le ticket de caisse de Kinokuniya fait un peut moins de 6 cm. Sur une photo
 * portrait alignant 3 tickets, j’obtient une largeur de 660 px par ticket. Ça
 * fait une résolution d’environ de 11 px / mm, soit 280 dpi. Oublions les
 * unités impériales et partons sur du 10 px / mm. Par conséquent, un ticket
 * après extraction fera 600 px de large. La hauteur après extraction sera
 * calculée pour faire correspondre l’aspect avec le rectangle détecté.
 *
 * Obtenir le vrai aspect est un sujet de recherche, mais il est résolu :
 * https://stackoverflow.com/questions/38285229/calculating-aspect-ratio-of-perspective-transform-destination-image
 *
 * Autres références :
 * - https://pyimagesearch.com/2014/08/25/4-point-opencv-getperspective-transform-example/
 * - https://pyimagesearch.com/2014/09/01/build-kick-ass-mobile-document-scanner-just-5-minutes/
 */

#include "kakeibo.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>

static const cv::Scalar red(0, 0, 255);
static const cv::Scalar green(0, 255, 0);
static const cv::Scalar yellow(0, 255, 255);

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	const char* image_path = argv[1];
	cv::Mat source = cv::imread(image_path, cv::IMREAD_COLOR);

	// Grise l’image avant de travailler sur les contours.
	cv::Mat image;
	cv::cvtColor(source, image, cv::COLOR_BGR2GRAY);

	// Floute les détails pour que le bruit du fond ne génère pas de bords.
	// Comme nos tables sont en bois, le motif du bois casse tout.
	cv::GaussianBlur(image, image, cv::Size(5, 5), 0);

	// Détection des bords avec un fort seuil. On s’attend à des tickets
	// blancs sur fond foncé.
	cv::Canny(image, image, 75, 200);

	// Dilatation pour que les bords fragiles se solidifient.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::dilate(image, image, element);
	//cv::imshow("Dilated", image);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	// Image sur laquelle on dessine les countours trouvés pour faciliter le debug.
	cv::Mat drawing = source.clone();

	int extracted_count = 0;
        for (size_t i = 0; i < contours.size(); i++) {
		// Dessiner uniquement les contours rectangles.
		std::vector<cv::Point> poly;
		cv::approxPolyDP(contours[i], poly, 100, true /* closed */);
		if (poly.size() != 4)
			continue;

		cv::drawContours(drawing, contours, i, red, 5);

		quad q(poly);
		int w = q.width();
		int h = q.height();
		if (w < 400 || h < 300)
			continue; // Trop petit.
		if (w > h * 0.8)
			// Trop large par rapport à sa hauteur.
			continue;

		q.shrink(h * 0.005);
		cv::polylines(drawing, q.corners, false, green, 10);
		cv::drawMarker(drawing, q.corners[0], yellow, cv::MARKER_CROSS, 40, 10);

		float new_width = 600;
		float new_height = h * new_width / w;
		std::vector<cv::Point2f> old_rect;
		std::transform(q.corners.begin(), q.corners.end(), std::back_inserter(old_rect), [] (cv::Point p) { return p; });
		std::vector<cv::Point2f> new_rect = { {0, 0}, {new_width, 0}, {new_width, new_height}, {0, new_height} };
		cv::Mat transform = cv::getPerspectiveTransform(old_rect, new_rect);
		cv::Mat extracted_receipt;
		cv::warpPerspective(source, extracted_receipt, transform, cv::Size(new_width, new_height));

		std::string output_file = "extracted-" + std::to_string(++extracted_count) + ".jpg";
		std::cout << output_file << std::endl;
		cv::imwrite(output_file, extracted_receipt);
	}

	cv::imshow("Contours", drawing);
	cv::waitKey(0);
	return 0;
}
