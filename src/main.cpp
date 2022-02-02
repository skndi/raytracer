#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <random>
#include <vector>

#include "threading.hpp"
#include "Material.h"
#include "Primitive.h"
#include "Image.hpp"

struct Camera {
	vec3 origin =  {0, 0, 0};
	vec3 llc = {-2.f, -1.f, -1.f};
	vec3 left = {4, 0, 0};
	vec3 up = {0, 2, 0};

	Ray getRay(float u, float v) const {
		return Ray(origin, (llc + u * left + v * up).normalized());
	}
};

vec3 color(const Ray &r, PrimList &prims, int depth = 0) {
	Intersection data;
	if (prims.intersect(r, 0.001f, FLT_MAX, data)) {
		Ray scatter;
		Color attenuation;
		if (depth < 50 && data.material->shade(r, data, attenuation, scatter)) {
			const Color incoming = color(scatter, prims, depth + 1);
			return attenuation * incoming;
		} else {
			return Color(0.f);
		}
	}
	const vec3 dir = r.dir;
	const float f = 0.5f * (dir.y + 1.f);
	return (1.f - f) * vec3(1.f) + f * vec3(0.5f, 0.7f, 1.f);
}


int main() {
	const int width = 1920;
	const int height = 1080;
	const int samplesPerPixel = 24;
	ImageData data(width, height);

	PrimList scene;
	scene.primitives.push_back(new SpherePrim{vec3(0, 0, -1), 0.5f, new Lambert{Color(0.8, 0.3, 0.3)}});
	scene.primitives.push_back(new SpherePrim{vec3(0, -100.5, -1), 100, new Lambert{Color{0.8, 0.8, 0.0}}});
	scene.primitives.push_back(new SpherePrim{vec3(1, 0, -1), 0.5, new Metal{Color{0.8, 0.6, 0.2}, 0.3}});
	scene.primitives.push_back(new SpherePrim{vec3(-1, 0, -1), 0.5, new Dielectric{1.5}});
	scene.primitives.push_back(new SpherePrim{vec3(-1, 0, -1), 0.45f, new Dielectric{1.5}});

	struct PixelRenderer : Task {
		PrimList &scene;
		ImageData &data;
		PixelRenderer(PrimList &scene, ImageData &data) : scene(scene), data(data) {}
		void run(int threadIndex, int threadCount) override {
			const vec3 llc(-2.f, -1.f, -1.f);
			const vec3 hor(4.f, 0.f, 0.f);
			const vec3 vert(0.f, 2.f, 0.f);
			const vec3 origin(0.f);

			const int total = data.width * data.height;
			for (int idx = threadIndex; idx < total; idx += threadCount) {
				const int r = idx / data.width;
				const int c = idx % data.width;

				Color avg(0);
				for (int s = 0; s < samplesPerPixel; s++) {
					const float u = float(c + randFloat()) / float(data.width);
					const float v = float(r + randFloat()) / float(data.height);
					const Ray ray(origin, (llc + u * hor + v * vert).normalized());
					const vec3 sample = color(ray, scene);
					avg += sample;
				}

				avg /= samplesPerPixel;
				data[c][height - r - 1] = Color(sqrtf(avg.r), sqrtf(avg.g), sqrtf(avg.b));
			}
		}
	} pr{scene, data};

	ThreadManager tm(std::thread::hardware_concurrency() / 2);
	tm.start();
	tm.runThreads(pr);
	tm.stop();

	const PNGImage &png = data.createPNGData();
	const int error = stbi_write_png("result.png", width, height, PNGImage::componentCount(), png.data.data(), sizeof(PNGImage::Pixel) * width);

	return 0;
}