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
			intersection.material = material.get();
			return true;
		}
	}
	return false;
}

void Instancer::addInstance(SharedPrimPtr prim, const vec3& offset, float scale) {
	BBox primBox;
	primBox.min = (prim->box.min * scale) + offset;
	primBox.max = (prim->box.max * scale) + offset;
	box.add(primBox);
	primitives.push_back(std::move(prim));
	offsets.push_back(offset);
	scales.push_back(scale);

}

bool Instancer::intersect(const Ray& ray, float tMin, float tMax, Intersection& intersection) {
	if (!box.testIntersect(ray)) {
		return false;
	}
	float closest = tMax;
	bool hasHit = false;
	Ray local = ray;
	for (int c = 0; c < primitives.size(); c++) {
		Intersection data;
		local.origin = (ray.origin - offsets[c]) / scales[c];
		if (primitives[c]->intersect(local, tMin, tMax, data)) {
			if (data.t < closest) {
				intersection = data;
				closest = data.t;
				hasHit = true;
			}
		}
	}
	return hasHit;
}

