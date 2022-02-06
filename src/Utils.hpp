#pragma once

#include <cmath>
#include <ostream>
#include <random>

#define assert(test) do { !!(test) ? (void)0 : __debugbreak(); } while(0)

struct Material;

inline bool similar(float a, float b) {
	return fabs(a - b) < 0.001;
}

#pragma pack(1)
struct vec3 {
	union {
		float _v[3];
		struct {
			float r, g, b;
		};

		struct {
			float x, y, z;
		};
	};

	vec3() {}

	vec3(float v)
		: r(v) , g(v) , b(v)
	{}

	vec3(float r, float g, float b)
		: r(r), g(g), b(b)
	{}

	float &operator[](int index) {
		assert(index < 3 && index > 0);
		return _v[index];
	}

	const float &operator[](int index) const {
		assert(index < 3 && index >= 0);
		return _v[index];
	}

	vec3 operator-() const {
		return vec3(-r, -g, -b);
	}

	float length() const {
		return std::sqrtf(lengthSquare());
	}

	float lengthSquare() const {
		return r * r + g * g + b * b;
	}

	vec3 &operator+=(const vec3 &other) {
		for (int c = 0; c < 3; c++) {
			_v[c] += other._v[c];
		}
		return *this;
	}

	vec3 &operator*=(float v) {
		for (int c = 0; c < 3; c++) {
			_v[c] *= v;
		}
		return *this;
	}

	vec3 &operator/=(float v) {
		for (int c = 0; c < 3; c++) {
			_v[c] /= v;
		}
		return *this;
	}

	vec3 normalized() const {
		vec3 copy(*this);
		return copy /= length();
	}

	void normalize() {
		*this /= length();
	}

	bool similar(const vec3 &other) const {
		return ::similar(r, other.r) &&
			::similar(g, other.g) &&
			::similar(b, other.b);
	}

	bool isNormal() const {
		return similar(normalized());
	}
};

inline std::ostream &operator<<(std::ostream &out, const vec3 &v) {
	return out << v._v[0] << ' ' << v._v[1] << ' ' << v._v[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v) {
	return vec3(u._v[0] + v._v[0], u._v[1] + v._v[1], u._v[2] + v._v[2]);
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
	return vec3(u._v[0] - v._v[0], u._v[1] - v._v[1], u._v[2] - v._v[2]);
}

inline vec3 operator*(const vec3 &u, const vec3 &v) {
	return vec3(u._v[0] * v._v[0], u._v[1] * v._v[1], u._v[2] * v._v[2]);
}

inline vec3 operator*(float t, const vec3 &v) {
	return vec3(t * v._v[0], t * v._v[1], t * v._v[2]);
}

inline vec3 operator*(const vec3 &v, float t) {
	return t * v;
}

inline vec3 operator/(vec3 v, float t) {
	return (1 / t) * v;
}

inline float dot(const vec3 &u, const vec3 &v) {
	return u._v[0] * v._v[0]
		+ u._v[1] * v._v[1]
		+ u._v[2] * v._v[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
	return vec3(u._v[1] * v._v[2] - u._v[2] * v._v[1],
		u._v[2] * v._v[0] - u._v[0] * v._v[2],
		u._v[0] * v._v[1] - u._v[1] * v._v[0]);
}

typedef vec3 Color;

struct Ray {
	vec3 origin;
	vec3 dir;

	Ray() {}

	Ray(const vec3 &origin, const vec3 &dir)
		: origin(origin), dir(dir) {
		assert(dir.isNormal());
	}

	vec3 at(float t) const {
		return origin + dir * t;
	}
};

struct Intersection {
	float t = -1.f;
	vec3 p;
	vec3 normal;
	Material *material = nullptr;
};

inline float randFloat() {
	thread_local std::mt19937 rng(42);
	std::uniform_real_distribution<float> dist(0.f, 0.9999f);
	return dist(rng);
}

inline vec3 randomUnitSphere() {
	vec3 p;
	do {
		p = 2.0f * vec3(randFloat(), randFloat(), randFloat()) - vec3(1);
	} while (p.lengthSquare() >= 1.f);
	return p;
}
