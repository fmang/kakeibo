cmake_minimum_required(VERSION 3.24)

project(
	kakeibo
	VERSION 0.1.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED core imgproc imgcodecs highgui)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(
	receipt-scanner
	src/receipt-scanner.cc
	src/kakeibo.h
	src/cutter.cc
	src/detector.cc
)
target_link_libraries(receipt-scanner ${OpenCV_LIBS})
