#define _CRT_SECURE_NO_WARNINGS

#include <random>
#include <vector>
#include <cmath>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

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
	float aspect;
	vec3 origin;
	vec3 llc;
	vec3 left;
	vec3 up;

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

struct Scene {
	Scene() = default;
	Scene(const Scene &) = delete;
	Scene &operator=(const Scene &) = delete;

	int width = 640;
	int height = 480;
	int samplesPerPixel = 2;
	Instancer primitives;
	Camera camera;
	ImageData image;

	void onBeforeRender() {
		primitives.onBeforeRender();
	}

	void initImage(int w, int h, int spp) {
		image.init(w, h);
		width = w;
		height = h;
		samplesPerPixel = spp;
		camera.aspect = float(width) / height;
	}

	void addPrimitive(PrimPtr primitive) {
		primitives.addInstance(std::move(primitive));
	}
};

vec3 color(const Ray &r, Instancer &prims, int depth = 0) {
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

void initExampleScene(Scene &scene) {
	scene.initImage(800, 600, 4);
	scene.camera.lookAt(90.f, {-0.1f, 5, -0.1f}, {0, 0, 0});

	SharedPrimPtr mesh(new TriangleMesh(MESH_FOLDER "/cube.obj", MaterialPtr(new Lambert{Color(1, 0, 0)})));
	Instancer *instancer = new Instancer;
	instancer->addInstance(mesh, vec3(2, 0, 0));
	instancer->addInstance(mesh, vec3(0, 0, 2));
	instancer->addInstance(mesh, vec3(2, 0, 2));
	scene.addPrimitive(PrimPtr(instancer));

	const float r = 0.6f;
	scene.addPrimitive(PrimPtr(new SpherePrim{vec3(2, 0, 0), r, MaterialPtr(new Lambert{Color(0.8, 0.3, 0.3)})}));
	scene.addPrimitive(PrimPtr(new SpherePrim{vec3(0, 0, 2), r, MaterialPtr(new Lambert{Color(0.8, 0.3, 0.3)})}));
	scene.addPrimitive(PrimPtr(new SpherePrim{vec3(0, 0, 0), r, MaterialPtr(new Lambert{Color(0.8, 0.3, 0.3)})}));
}

void initHeavyDragons(Scene &scene) {
	const int count = 50;

	scene.initImage(800, 600, 4);
	scene.camera.lookAt(90.f, {0, 3, count}, {0, 3, 0});

	SharedPrimPtr mesh(new TriangleMesh(MESH_FOLDER "/dragon.obj", MaterialPtr(new Lambert{Color(0.2, 0.7, 0.1)})));
	Instancer *instancer = new Instancer;

	for (int c = -count; c <= count; c++) {
		for (int r = -count; r <= count; r++) {
			instancer->addInstance(mesh, vec3(c, 0, r), 0.05f);
			instancer->addInstance(mesh, vec3(c, 6, r), 0.05f);
		}
	}

	scene.addPrimitive(PrimPtr(instancer));
}

void initCubes(Scene &scene) {
	const int count = 20;

	scene.initImage(800, 600, 2);
	scene.camera.lookAt(90.f, {0, 2, count}, {0, 0, 0});

	SharedPrimPtr mesh(new TriangleMesh(MESH_FOLDER "/cube.obj", MaterialPtr(new Lambert{Color(1, 0, 0)})));
	Instancer *instancer = new Instancer;

	for (int c = -count; c <= count; c++) {
		for (int r = -count; r <= count; r++) {
			instancer->addInstance(mesh, vec3(c, 0, r), 0.5f);
		}
	}

	scene.addPrimitive(PrimPtr(instancer));
}

void initDragon(Scene &scene) {
	scene.initImage(800, 600, 4);
	scene.camera.lookAt(90.f, {8, 10, 7}, {0, 0, 0});
	scene.addPrimitive(PrimPtr(new TriangleMesh(MESH_FOLDER "/dragon.obj", MaterialPtr(new Lambert{Color(0.2, 0.7, 0.1)}))));
}

int main() {
	Scene scene;
	printf("Loading scene...\n");
	initCubes(scene);
	printf("Preparing scene...\n");
	scene.onBeforeRender();

	struct PixelRenderer : Task {
		Scene &scene;
		std::atomic<int> renderedPixels;
		PixelRenderer(Scene &scene)
			: scene(scene), renderedPixels(0)
		{}

		void run(int threadIndex, int threadCount) override {
			const int total = scene.width * scene.height;
			const int incrementPrint = total / 100;
			for (int idx = threadIndex; idx < total; idx += threadCount) {
				const int r = idx / scene.width;
				const int c = idx % scene.width;

				Color avg(0);
				for (int s = 0; s < scene.samplesPerPixel; s++) {
					const float u = float(c + randFloat()) / float(scene.width);
					const float v = float(r + randFloat()) / float(scene.height);
					const Ray &ray = scene.camera.getRay(u, v);
					const vec3 sample = color(ray, scene.primitives);
					avg += sample;
				}

				avg /= scene.samplesPerPixel;
				scene.image[c][scene.height - r - 1] = Color(sqrtf(avg.x), sqrtf(avg.y), sqrtf(avg.z));
				const int completed = renderedPixels.fetch_add(1, std::memory_order_relaxed);
				if (completed % incrementPrint == 0) {
					printf("\r%d%% ", int(float(completed) / float(total) * 100));
				}
			}
		}
	} pr{scene};

	ThreadManager tm(std::thread::hardware_concurrency());
	tm.start();
	printf("Rendering ...\n");
	{
		Timer timer;
		tm.runThreads(pr);
		printf("Render time: %gms\n", Timer::toMs<float>(timer.elapsedNs()));
	}
	tm.stop();

	const char *resultImage = "result.png";
	printf("Saving image to %s...\n", resultImage);
	const PNGImage &png = scene.image.createPNGData();
	const int error = stbi_write_png(resultImage, scene.width, scene.height, PNGImage::componentCount(), png.data.data(), sizeof(PNGImage::Pixel) * scene.width);

	return 0;
}