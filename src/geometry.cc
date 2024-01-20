#include "kakeibo.h"

#include <cassert>

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
