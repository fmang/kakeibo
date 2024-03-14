#pragma once

#include <opencv2/core.hpp>

#include <array>
#include <string>
#include <vector>

// receipt-scanner.cc

extern bool explain;
void show(const std::string& name, cv::Mat image);
std::string save(cv::Mat image);

// cutter.cc

struct quad {
	quad(const std::vector<cv::Point>& corners);
	std::array<cv::Point, 4> corners;
	int height() const;
	int width() const;
	void shrink(int border);
	void rectify();
};

std::vector<quad> find_receipts(cv::Mat photo);
cv::Mat cut_receipt(cv::Mat photo, quad contour);

// detector.cc

void scan_receipt(cv::Mat photo);
void extract_letters(cv::Mat photo);
void extract_logo(cv::Mat photo);
void compile_features(const char *samples_path);
