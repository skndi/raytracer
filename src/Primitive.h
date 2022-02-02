#pragma once

#include <vector>

#include "Utils.hpp"

struct Intersection;

struct Primitive {
	virtual bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) = 0;
};

struct SpherePrim : Primitive {
	vec3 center;
	float radius;
	Material *material = nullptr;

	SpherePrim(vec3 center, float radius, Material *material)
		: center(center), radius(radius), material(material)
	{}

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};


struct PrimList : Primitive {
	std::vector<Primitive *> primitives;
	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};
