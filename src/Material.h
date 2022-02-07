#pragma once
#include "Utils.hpp"

struct Intersection;

struct Material {
	virtual bool shade(const Ray &in, const Intersection &data, Color &attenuation, Ray &scatter) = 0;
};

typedef std::unique_ptr<Material> MaterialPtr;

struct Lambert : Material {
	Color albedo;
	Lambert(Color albedo)
		: albedo(albedo)
	{}
	bool shade(const Ray& ray, const Intersection& data, Color& attenuation, Ray& scatter) override;
};

struct Metal : Material {
	Color albedo;
	float fuzz;
	Metal(Color albedo, float fuzz)
		: albedo(albedo)
		, fuzz(fuzz)
	{}
	bool shade(const Ray& ray, const Intersection& data, Color& attenuation, Ray& scatter) override;
};
