#include "Material.h"

bool Lambert::shade(const Ray &ray, const Intersection &data, Color &attenuation, Ray &scatter) {
	const vec3 target = data.p + data.normal + randomUnitSphere();
	scatter = Ray{data.p, (target - data.p).normalized()};
	attenuation = albedo;
	return true;
}

bool Metal::shade(const Ray &ray, const Intersection &data, Color &attenuation, Ray &scatter) {
	const vec3 reflected = (reflect(ray.dir, data.normal).normalized() + fuzz * randomUnitSphere()).normalized();
	scatter = Ray{data.p, reflected};
	attenuation = albedo;
	return dot(scatter.dir, data.normal) > 0.f;
}

static float schlick(float cos, float ior) {
	float r0 = (1.f - ior) / (1.f + ior);
	r0 = r0 * r0;
	return r0 + (1.f - r0) * powf(1.f - cos, 5);
}


bool Dielectric::shade(const Ray &ray, const Intersection &data, Color &attenuation, Ray &scatter) {
	attenuation = Color(1.0f, 1.0f, 1.0f);
	const float hitIor = data.frontFace ? (1.0f / ior) : ior;

	const float cosTheta = fmin(dot(-ray.dir, data.normal), 1.0f);
	const float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

	const bool canRefract = !(hitIor * sinTheta > 1.0f);
	vec3 direction;

	if (!canRefract || schlick(cosTheta, hitIor) > randFloat()) {
		direction = reflect(ray.dir, data.normal);
	} else {
		direction = refract(ray.dir, data.normal, hitIor);
	}

	scatter = Ray(data.p, direction);
	return true;
}
