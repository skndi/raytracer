#pragma once

#include "Primitive.h"
#include "Utils.hpp"

struct Face {
	int indices[3];
};

struct TriMesh : Primitive {
	std::vector<vec3> vertices;
	std::vector<Face> faces;
	Material *material = nullptr;

	TriMesh(const std::string &objFile, Material *material)
		: material(material) {
		loadFromObj(objFile);
	}

	bool loadFromObj(const std::string &objPath);

	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override;
	bool intersectTriangle(const Ray& ray, const Face &t, Intersection &info);
};

