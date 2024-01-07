PKG_CONFIG_PATH = /opt/local/lib/opencv4/pkgconfig/
CXXFLAGS = -std=c++20 $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags opencv4)
LDLIBS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs opencv4) -lc++

.PHONY: all run

all: kakeibo

run: kakeibo
	./kakeibo input.jpg

kakeibo: kakeibo.o
