#pragma once

#include "Utils.hpp"
#include "Material.h"

#include <vector>
#include <memory>

struct Intersection;

struct Intersectable {
	virtual bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) = 0;
	virtual bool boxIntersect(const BBox &box) = 0;
	virtual void expandBox(BBox &box) = 0;
	virtual ~Intersectable() = default;
};

struct Primitive : Intersectable {
	BBox box;

	virtual void onBeforeRender() {}

	bool boxIntersect(const BBox& other) override {
		return !box.boxIntersection(other).isEmpty();
	}

	void expandBox(BBox &other) override {
		other.add(box);
	}

	~Primitive() override = default;
};

typedef std::unique_ptr<Primitive> PrimPtr;
typedef std::shared_ptr<Primitive> SharedPrimPtr;


struct IntersectionAccelerator {
	virtual void addPrimitive(Intersectable *prim) = 0;
	virtual void clear() = 0;
	virtual void build(int maxDepth = -1, int minPrimitives = -1) = 0;
	virtual bool isBuilt() const = 0;
	virtual bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) = 0;
	virtual ~IntersectionAccelerator() = default;
};

typedef std::unique_ptr<IntersectionAccelerator> AcceleratorPtr;
AcceleratorPtr makeOctTree();

AcceleratorPtr makeDefaultAccelerator();

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
	struct Instance : Intersectable {
		SharedPrimPtr primitive;
		vec3 offset;
		float scale;

		bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
		bool boxIntersect(const BBox &other) override;
		void expandBox(BBox &other) override;
	};
	std::vector<Instance> instances;

	AcceleratorPtr accelerator;
public:

	void onBeforeRender() override;

	void addInstance(SharedPrimPtr prim, const vec3 &offset = vec3(0.f), float scale = 1.f);

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
};
