#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

#include <random>
#include <vector>
#include <cmath>

#include "threading.hpp"
#include "Material.h"
#include "Primitive.h"
#include "Image.hpp"
#include "Mesh.h"

static const int MAX_RAY_DEPTH = 35;
const float PI = 3.14159265358979323846;

float degToRad(float deg) {
	return (deg * PI) / 180.f;
}

struct Camera {
	const vec3 worldUp = {0, 1, 0};
	const float aspect;
    vec3 origin;
    vec3 llc;
    vec3 left;
    vec3 up;

	Camera(float aspect) : aspect(aspect) {}

	void lookAt(float verticalFov, const vec3 &lookFrom, const vec3 &lookAt) {
		origin = lookFrom;
		const float theta = degToRad(verticalFov);
		float half_height = tan(theta / 2);
		const float half_width = aspect * half_height;

		const vec3 w = (origin - lookAt).normalized();
		const vec3 u = cross(worldUp, w).normalized();
		const vec3 v = cross(w, u);
		llc = origin - half_width * u - half_height * v - w;
		left = 2 * half_width * u;
		up = 2 * half_height * v;
	}

    Ray getRay(float u, float v) const {
        return Ray(origin, (llc + u * left + v * up - origin).normalized());
    }
};

vec3 color(const Ray &r, PrimList &prims, int depth = 0) {
	Intersection data;
	if (prims.intersect(r, 0.001f, FLT_MAX, data)) {
		Ray scatter;
		Color attenuation;
		if (depth < MAX_RAY_DEPTH && data.material->shade(r, data, attenuation, scatter)) {
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

void exampleScene(PrimList &scene, Camera &camera) {
	camera.lookAt(90.f, {1, 5, 1}, {0, 0, 0});

	SharedPrimPtr mesh(new TriMesh(MESH_FOLDER "/cube.obj", MaterialPtr(new Lambert{Color(1, 0, 0)})));
	Instancer *instancer = new Instancer;
	instancer->addInstance(mesh, vec3(2, 0, 0));
	instancer->addInstance(mesh, vec3(0, 0, 2));
	instancer->addInstance(mesh, vec3(2, 0, 2));
	scene.primitives.push_back(PrimPtr(instancer));

	const float r = 0.6f;
	scene.primitives.emplace_back(new SpherePrim{vec3(2, 0, 0), r, MaterialPtr(new Lambert{Color(0.8, 0.3, 0.3)})});
	scene.primitives.emplace_back(new SpherePrim{vec3(0, 0, 2), r, MaterialPtr(new Lambert{Color(0.8, 0.3, 0.3)})});
	scene.primitives.emplace_back(new SpherePrim{vec3(0, 0, 0), r, MaterialPtr(new Lambert{Color(0.8, 0.3, 0.3)})});
}


void initCubes(PrimList &scene, Camera &camera) {
	const int count = 20;

	camera.lookAt(90.f, {count, 20, count}, {0, 0, 0});

	SharedPrimPtr mesh(new TriMesh(MESH_FOLDER "/cube.obj", MaterialPtr(new Lambert{Color(1, 0, 0)})));
	Instancer *instancer = new Instancer;

	for (int c = -count; c <= count; c++) {
		for (int r = -count; r <= count; r++) {
			instancer->addInstance(mesh, vec3(c, 0, r), 0.75f);
		}
	}
	scene.primitives.push_back(PrimPtr(instancer));
}

int main() {
	const int width = 800;
	const int height = 600;
	const int samplesPerPixel = 4;
	ImageData data(width, height);
	Camera cam(float(width) / height);

	PrimList scene;
	initCubes(scene, cam);

	struct PixelRenderer : Task {
		PrimList &scene;
		ImageData &data;
		Camera &cam;
		PixelRenderer(PrimList &scene, ImageData &data, Camera &cam) : scene(scene), data(data), cam(cam) {}
		void run(int threadIndex, int threadCount) override {
			const int total = data.width * data.height;
			for (int idx = threadIndex; idx < total; idx += threadCount) {
				const int r = idx / data.width;
				const int c = idx % data.width;

				Color avg(0);
				for (int s = 0; s < samplesPerPixel; s++) {
					const float u = float(c + randFloat()) / float(data.width);
					const float v = float(r + randFloat()) / float(data.height);
					const Ray &ray = cam.getRay(u, v);
					const vec3 sample = color(ray, scene);
					avg += sample;
				}

				avg /= samplesPerPixel;
				data[c][height - r - 1] = Color(sqrtf(avg.r), sqrtf(avg.g), sqrtf(avg.b));
			}
		}
	} pr{scene, data, cam};

	ThreadManager tm(std::thread::hardware_concurrency());
	tm.start();
	tm.runThreads(pr);
	tm.stop();

	const PNGImage &png = data.createPNGData();
	const int error = stbi_write_png("result.png", width, height, PNGImage::componentCount(), png.data.data(), sizeof(PNGImage::Pixel) * width);

	return 0;
}