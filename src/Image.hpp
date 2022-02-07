#pragma once

#include "Utils.hpp"

#include <fstream>
#include <vector>

struct ImageData;


struct PNGImage {
#pragma pack(1)
	union Pixel {
		uint8_t rgba[3];
		struct {
			uint8_t r, g, b;
		};
	};
#pragma pack(pop)

	static int componentCount() {
		return int(sizeof(Pixel::rgba) / sizeof(Pixel::rgba[0]));
	}

	std::vector<Pixel> data;

	PNGImage(int w, int h): data(w * h) {
		
	}
};

struct ImageData {
	int width;
	int height;
	std::vector<Color> pixels;

	void init(int w, int h) {
		width = w;
		height = h;
		pixels.resize(width * height);
	}

	ImageData(int width, int height) {
		init(width, height);
	}

	ImageData() = default;

	PNGImage createPNGData() const {
		PNGImage img(width, height);
		for (int c = 0; c < pixels.size(); c++) {
			PNGImage::Pixel &out = img.data[c];
			const Color &in = pixels[c];
			float sum = 0.f;
			for (int r = 0; r < 3; r++) {
				out.rgba[r] = in[r] * 255.0;
				sum += in[r];
			}
		}
		return img;
	}

	struct ConstRowProxy {
		const ImageData &img;
		int col;
		
		const Color &operator[](int row) const {
			return img.pixels[row * img.width + col];
		}
	};

	struct RowProxy {
		ImageData &img;
		int col;

		Color &operator[](int row) {
			return img.pixels[row * img.width + col];
		}

		const Color &operator[](int row) const {
			return img.pixels[row * img.width + col];
		}
	};

	RowProxy operator[](int col) {
		return RowProxy{*this, col};
	}

	ConstRowProxy operator[](int col) const {
		return ConstRowProxy{*this, col};
	}
};
