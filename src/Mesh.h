#pragma once

#include "Primitive.h"
#include "Utils.hpp"

struct Face {
	int indices[3];
};

struct TriMesh : Primitive {
	std::vector<vec3> vertices;
	std::vector<Face> faces;
	std::unique_ptr<Material> material;

	TriMesh(const std::string &objFile, std::unique_ptr<Material> material)
		: material(std::move(material)) {
		loadFromObj(objFile);
	}

	bool loadFromObj(const std::string &objPath);

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
	bool intersectTriangle(const Ray& ray, const Face &t, Intersection &info);
};

