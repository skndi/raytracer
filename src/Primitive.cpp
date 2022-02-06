#include "Primitive.h"

bool SpherePrim::intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) {
	const float a = dot(ray.dir, ray.dir);
	const float b = 2.f * dot(ray.dir, ray.origin - center);
	const float c = dot(ray.origin - center, ray.origin - center) - radius * radius;
	const float D = b * b - 4 * a * c;
	if (D >= 0.f) {
		const float t = (-b - sqrtf(D)) / (2.f * a);
		if (t >= tMin && t <= tMax) {
			intersection.t = t;
			intersection.p = ray.at(t);
			intersection.normal = (intersection.p - center) / radius;
			intersection.material = material;
			return true;
		}
	}
	return false;
}

bool PrimList::intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) {
	float closest = tMax;
	bool hasHit = false;
	for (int c = 0; c < primitives.size(); c++) {
		Intersection data;
		if (primitives[c]->intersect(ray, tMin, tMax, data)) {
			if (data.t < closest) {
				intersection = data;
				closest = data.t;
				hasHit = true;
			}
		}
	}
	return hasHit;
}

