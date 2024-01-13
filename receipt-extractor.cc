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

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

static const cv::Scalar red(0, 0, 255);
static const cv::Scalar green(0, 255, 0);
static const cv::Scalar yellow(0, 255, 255);

/**
 * Reçoit un contour contenant 4 points et réordonne les points dans le sens
 * horaire, en commençant par le coin haut-gauche.
 */
struct quad {
	quad(const std::vector<cv::Point>& corners);
	std::array<cv::Point, 4> corners;

	int height() const;
	int width() const;
};

quad::quad(const std::vector<cv::Point>& points)
{
	assert(points.size() == 4);

	auto [top_left, bottom_right] = std::minmax_element(
		points.begin(), points.end(),
		[](cv::Point a, cv::Point b) { return a.x + a.y < b.x + b.y; });
	auto [bottom_left, top_right] = std::minmax_element(
		points.begin(), points.end(),
		[](cv::Point a, cv::Point b) { return a.x - a.y < b.x - b.y; });

	corners[0] = *top_left;
	corners[1] = *top_right;
	corners[2] = *bottom_right;
	corners[3] = *bottom_left;
}

int quad::height() const
{
	return (cv::norm(corners[0] - corners[3]) + cv::norm(corners[1] - corners[2])) / 2;
}

int quad::width() const
{
	return (cv::norm(corners[0] - corners[1]) + cv::norm(corners[2] - corners[3])) / 2;
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
		return 1;
	}

	const char* image_path = argv[1];
	cv::Mat source = cv::imread(image_path, cv::IMREAD_GRAYSCALE);

	// Floute les détails pour que le bruit du fond ne génère pas de bords.
	// Comme nos tables sont en bois, le motif du bois casse tout.
	cv::Mat image;
	cv::GaussianBlur(source, image, cv::Size(5, 5), 0);

	// Détection des bords avec un fort seuil. On s’attend à des tickets
	// blancs sur fond foncé.
	cv::Canny(image, image, 75, 200);

	// Diltation pour que les bords fragiles se solidifient.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::dilate(image, image, element);
	cv::imshow("Dilated", image);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Mat drawing;
	cvtColor(source, drawing, cv::COLOR_GRAY2BGR);
        for (size_t i = 0; i < contours.size(); i++) {
		// Dessiner uniquement les contours rectangles.
		std::vector<cv::Point> poly;
		cv::approxPolyDP(contours[i], poly, 100, true /* closed */);
		if (poly.size() != 4)
			continue;

		quad q(poly);
		int w = q.width();
		int h = q.height();
		if (w < 400 || h < 300)
			continue; // Trop petit.
		if (w > h * 0.8)
			// Trop large par rapport à sa hauteur.
			continue;

		cv::polylines(drawing, q.corners, false, red, 10);
		cv::drawMarker(drawing, q.corners[0], yellow, cv::MARKER_CROSS, 40, 10);

		// TODO:
		// Calculer la marge comme 5% de la largeur. La couper sur les 4 bords.
		// Calculer la taille extraite du reçu, puis corriger la perspective.
	}
	cv::imshow("Contours", drawing);

	cv::waitKey(0);
	return 0;
}
