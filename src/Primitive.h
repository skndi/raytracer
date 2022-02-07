#pragma once

#include "Utils.hpp"
#include "Material.h"

#include <vector>
#include <memory>

struct Intersection;

struct Primitive {
	BBox box;
	virtual bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) = 0;
	virtual ~Primitive() = default;
};

typedef std::unique_ptr<Primitive> PrimPtr;
typedef std::shared_ptr<Primitive> SharedPrimPtr;

struct SpherePrim : Primitive {
	vec3 center;
	float radius;
	std::unique_ptr<Material> material;

	SpherePrim(vec3 center, float radius, MaterialPtr material)
		: center(center), radius(radius), material(std::move(material)) {
		box.add(center);
		box.add(center + vec3(radius, radius, radius));
		box.add(center - vec3(radius, radius, radius));
	}

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};

struct Instancer : Primitive {
	private:
	std::vector<SharedPrimPtr> primitives;
	std::vector<vec3> offsets;
	std::vector<float> scales;
public:
	void addInstance(SharedPrimPtr prim, const vec3 &offset = vec3(0.f), float scale = 1.f);

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};
