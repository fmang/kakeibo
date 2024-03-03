CXXFLAGS = -Wall -std=c++20 -I/opt/local/include/opencv4/ --system-header-prefix=opencv2/
LDLIBS = -L/opt/local/lib/opencv4/ -lopencv_highgui.4.8.0 -lopencv_imgcodecs.4.8.0 -lopencv_imgproc.4.8.0 -lopencv_core.4.8.0 -lc++

.PHONY: all receipt-scanner clean

all: receipt-scanner
receipt-scanner: src/receipt-scanner

clean:
	rm src/receipt-scanner src/*.o

src/receipt-scanner: src/receipt-scanner.o src/cutter.o src/detector.o
