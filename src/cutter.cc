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

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cassert>
#include <optional>

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

void shrink_segment(cv::Point& a, cv::Point& b, int border)
{
	cv::Point m = (a + b) / 2;
	double distance = cv::norm(a - b);
	double ratio = (distance - 2 * border) / distance;
	a = m + (a - m) * ratio;
	b = m + (b - m) * ratio;
}

/**
 * Réduit le rectangle en retranchant un nombre de pixel de chaque côté. Un
 * rectangle de 500 pixels de large avec border = 50 px fera 400 px de large à
 * la fin. De même pour la hauteur.
 */
void quad::shrink(int border)
{
	shrink_segment(corners[0], corners[1], border);
	shrink_segment(corners[2], corners[3], border);
	shrink_segment(corners[0], corners[3], border);
	shrink_segment(corners[1], corners[2], border);
}

/**
 * Renvoie l’index i du plus long bord (contour[i], contoun[(i + 1) % n]).
 */
static size_t longest_edge_index(const std::vector<cv::Point>& contour)
{
	int best_index = 0;
	int best_length = 0;
	for (size_t i = 0; i < contour.size(); ++i) {
		cv::Point a = contour[i];
		cv::Point b = contour[(i + 1) % contour.size()];
		cv::Point edge = b - a;
		int length = edge.dot(edge); // Norme au carré.
		if (length > best_length) {
			best_index = i;
			best_length = length;
		}
	}
	return best_index;
}

static std::optional<cv::Point> intersect_lines(cv::Point a1, cv::Point b1, cv::Point a2, cv::Point b2)
{
	double dy1 = b1.y - a1.y;
	double dx1 = a1.x - b1.x;
	double c1 = (a1.x * dy1) + (a1.y * dx1);

	double dy2 = b2.y - a2.y;
	double dx2 = a2.x - b2.x;
	double c2 = (a2.x * dy2) + (a2.y * dx2);

	double det = (dy1 * dx2) - (dy2 * dx1);
	if (det == 0)
		return {};

	double ix = (c1 * dx2 - c2 * dx1) / det;
	double iy = (c2 * dy1 - c1 * dy2) / det;
	return cv::Point(ix, iy);
}

/**
 * Essaie de déduire un rectangle d’un contour. Renvoie la liste des angles les
 * plus droits. S’il y en a 4, il y a de fortes chances qu’on ait un rectangle.
 */
static std::vector<cv::Point> approximate_rectangle(const std::vector<cv::Point>& contour)
{
	// Prend l’enveloppe convexe au cas où les bords seraient masqués, par
	// exemple par un doigt.
	std::vector<cv::Point> hull;
	cv::convexHull(contour, hull);

	// Trouve le bord le plus long pour ne pas utilise un faux bord comme
	// référence pour les angles.
	size_t start_index = longest_edge_index(hull);
	cv::Point anchor_a = hull[start_index];
	cv::Point anchor_b = hull[(start_index + 1) % hull.size()];

	std::vector<cv::Point> corners;
	cv::Point candidate_a;
	cv::Point candidate_b;
	double best_score = 0;
	// On itère 1 index plus loin pour que le bord initial apparaisse comme
	// candidat au moins une fois. Autrement, le dernier coin est sauté.
	for (size_t j = 0; j <= hull.size(); ++j) {
		size_t i = (start_index + 1 + j) % hull.size();
		cv::Point a = hull[i];
		cv::Point b = hull[(i + 1) % hull.size()];
		double score = std::abs((b - a).cross(anchor_b - anchor_a)) / cv::norm(b - a);
		if (explain) {
			std::printf(
				"anchor=(%d,%d),(%d,%d) edge=(%d,%d),(%d,%d) score=%f candidate=(%d,%d),(%d,%d) best_score=%f\n",
				anchor_a.x, anchor_a.y, anchor_b.x, anchor_b.y,
				a.x, a.y, b.x, b.y, score,
				candidate_a.x, candidate_a.y, candidate_b.x, candidate_b.y, best_score
			);
		}
		if (score <= best_score) {
			cv::Point corner = intersect_lines(candidate_a, candidate_b, anchor_a, anchor_b).value();
			if (explain)
				std::printf("push intersection=(%d, %d)\n", corner.x, corner.y);
			corners.push_back(corner);
			anchor_a = candidate_a;
			anchor_b = candidate_b;
			best_score = 0;
			--j; // L’ancre a changé donc il faut revoir le bord courant.
		} else {
			candidate_a = a;
			candidate_b = b;
			best_score = score;
		}
	}

	return corners;
}

std::vector<cv::Mat> cut_receipts(cv::Mat source)
{
	// Résultat.
	std::vector<cv::Mat> receipts;

	// Sélectionne uniquement les pixels clairs avec une saturation quasi-nulle.
	cv::Mat image;
	cv::cvtColor(source, image, cv::COLOR_BGR2HSV);
	cv::inRange(image, cv::Scalar(0, 0, 128), cv::Scalar(255, 32, 255), image);

	// Dilatation pour que les bords fragiles se solidifient.
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::dilate(image, image, element);
	show("shapes", image);

	cv::Mat drawing;
	if (explain)
		drawing = source.clone();

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        for (size_t i = 0; i < contours.size(); i++) {
		std::vector<cv::Point> poly = approximate_rectangle(contours[i]);
		if (explain) {
			std::vector<cv::Point> hull;
			cv::convexHull(contours[i], hull);
			cv::polylines(drawing, hull, true, cv::Scalar(255, 0, 0), 6);
			cv::polylines(drawing, poly, true, poly.size() == 4 ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 3);
		}

		// Traiter uniquement les rectangles;
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

		// Élimine un peu de bordure car il s’agit souvent d’ombre.
		q.shrink(h * 0.005);

		// Conversion du contour en 4 Point2f pour getPerspectiveTransform.
		std::vector<cv::Point2f> old_rect;
		std::transform(q.corners.begin(), q.corners.end(), std::back_inserter(old_rect), [] (cv::Point p) { return p; });

		// Calcul de la taille de l’image extraite. On force largeur et préserve le ratio.
		float new_width = 600;
		float new_height = h * new_width / w;
		std::vector<cv::Point2f> new_rect = { {0, 0}, {new_width, 0}, {new_width, new_height}, {0, new_height} };

		// Extraction du reçu par transformation de perspective.
		cv::Mat extracted_receipt;
		cv::Mat transform = cv::getPerspectiveTransform(old_rect, new_rect);
		cv::warpPerspective(source, extracted_receipt, transform, cv::Size(new_width, new_height));
		receipts.push_back(extracted_receipt);
	}

	if (explain)
		show("contours", drawing);

	return receipts;
}
