#pragma once

#include "Utils.hpp"
#include "Material.h"

#include <vector>
#include <memory>

struct Intersection;

struct Primitive {
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
		: center(center), radius(radius), material(std::move(material))
	{}

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};

struct PrimList : Primitive {
	std::vector<PrimPtr> primitives;
	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};

struct Instancer : Primitive {
	std::vector<SharedPrimPtr> primitives;
	std::vector<vec3> offsets;
	std::vector<float> scales;

	void addInstance(SharedPrimPtr prim, const vec3 &offset, float scale = 1.f) {
		primitives.push_back(std::move(prim));
		offsets.push_back(offset);
		scales.push_back(scale);
	}

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};
