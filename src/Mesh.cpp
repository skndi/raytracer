#define TINYOBJLOADER_IMPLEMENTATION

#include "Mesh.h"
#include "third_party/tiny_obj_loader.h"

bool TriMesh::loadFromObj(const std::string& objPath) {
	tinyobj::attrib_t inattrib;
	std::vector<tinyobj::shape_t> inshapes;
	std::string error;
	std::vector<tinyobj::material_t> materials;
	const bool loadRes = tinyobj::LoadObj(&inattrib, &inshapes, &materials, &error, objPath.c_str(), nullptr);
	if (!loadRes) {
		printf("Error loading file \"%s\", \"%s\"", objPath.c_str(), error.c_str());
	}

	static_assert(sizeof(vec3) == sizeof(tinyobj::real_t) * 3, "next line avoids copy with type alias");
	vertices.swap(reinterpret_cast<std::vector<vec3>&>(inattrib.vertices));

	for (int c = 0; c < inshapes.size(); c++) {
		int index = 0;
		bool skipMesh = false;
		const tinyobj::mesh_t &mesh = inshapes[c].mesh;

		const std::vector<unsigned char> &numFaceVertices = inshapes[c].mesh.num_face_vertices;
		for (int r = 0; r < numFaceVertices.size(); r++) {
			if (numFaceVertices[r] != 3) {
				skipMesh = true;
				break;
			}
		}
		if (skipMesh) {
			continue;
		}

		faces.reserve(faces.size() + numFaceVertices.size());
		for (int r = 0; r < numFaceVertices.size(); r++) {
			const Face &face = {
				mesh.indices[index++].vertex_index,
				mesh.indices[index++].vertex_index,
				mesh.indices[index++].vertex_index,
			};
			faces.push_back(face);
		}
		
	}
	return true;
}

/// source https://github.com/anrieff/quaddamage/blob/master/src/mesh.cpp#L165
bool TriMesh::intersectTriangle(const Ray& ray, const Face &t, Intersection &info)
{

	const vec3 &A = vertices[t.indices[0]];
	const vec3 &B = vertices[t.indices[1]];
	const vec3 &C = vertices[t.indices[2]];

	const vec3 AB = B - A;
	const vec3 AC = C - A;

	const vec3 normal = cross(AB, AC).normalized();;

	if (dot(ray.dir, normal) > 0) {
		return false;
	}

	const vec3 ABcrossAC = cross(AB, AC);
	
	const vec3 H = ray.origin - A;
	const vec3 D = ray.dir;
	
	const float Dcr = - dot(ABcrossAC, D);

	if (fabs(Dcr) < 1e-12) {
		return false;
	}

	const float rDcr = 1.f / Dcr;
	const float gamma = dot(ABcrossAC, H) * rDcr;
	if (gamma < 0 || gamma > info.t) {
		return false;
	}

	const vec3 HcrossD = cross(H, D);
	const float lambda2 = dot(HcrossD, AC) * rDcr;
	if (lambda2 < 0 || lambda2 > 1) {
		return false;
	}

	const float lambda3 = -dot(AB, HcrossD) * rDcr;
	if (lambda3 < 0 || lambda3 > 1) {
		return false;
	}

	if (lambda2 + lambda3 > 1) {
		return false;
	}

	info.t = gamma;
	info.p = ray.origin + ray.dir * gamma;
	info.normal = normal;
	info.material = material.get();

	return true;
}


bool TriMesh::intersect(const Ray& ray, float tMin, float tMax, Intersection& intersection) {
	intersection.t = tMax;
	bool haveRes = false;
	for (int c = 0; c < faces.size(); c++) {
		haveRes = haveRes || intersectTriangle(ray, faces[c], intersection);
	}
	return haveRes;
}


