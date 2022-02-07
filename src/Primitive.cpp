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

bool Instancer::Instance::intersect(const Ray& ray, float tMin, float tMax, Intersection& intersection) {
	const Ray local = {
		(ray.origin - offset) / scale,
		ray.dir
	};
	return primitive->intersect(local, tMin, tMax, intersection);
}

bool Instancer::Instance::boxIntersect(const BBox &other) {
	const BBox transformed {
		primitive->box.min * scale + offset,
		primitive->box.max * scale + offset
	};
	BBox intersection = other.boxIntersection(transformed);
	return !intersection.isEmpty();
}

void Instancer::Instance::expandBox(BBox &other) {
	const BBox transformed {
		primitive->box.min * scale + offset,
		primitive->box.max * scale + offset
	};
	other.add(transformed);
}

void Instancer::onBeforeRender() {
	for (int c = 0; c < instances.size(); c++) {
		instances[c].primitive->onBeforeRender();
	}
	if (instances.size() < 50) {
		return;
	}

	if (!accelerator) {
		accelerator = makeQuadTree();
	}
	if (!accelerator->isBuilt()) {
		accelerator->clear();
		for (int c = 0; c < instances.size(); c++) {
			accelerator->addPrimitive(&instances[c]);
		}
		accelerator->build(2, 20);
	}
}

void Instancer::addInstance(SharedPrimPtr prim, const vec3& offset, float scale) {
	BBox primBox;
	primBox.min = (prim->box.min * scale) + offset;
	primBox.max = (prim->box.max * scale) + offset;
	box.add(primBox);
	Instance instance;
	instance.primitive = std::move(prim);
	instance.offset = offset;
	instance.scale = scale;
	instances.push_back(instance);
}

bool Instancer::intersect(const Ray& ray, float tMin, float tMax, Intersection& intersection) {
	if (!box.testIntersect(ray)) {
		return false;
	}
	if (accelerator) {
		return accelerator->intersect(ray, tMin, tMax, intersection);
	}
	float closest = tMax;
	bool hasHit = false;
	for (int c = 0; c < instances.size(); c++) {
		Intersection data;
		if (instances[c].intersect(ray, tMin, tMax, data)) {
			if (data.t < closest) {
				intersection = data;
				closest = data.t;
				hasHit = true;
			}
		}
	}
	return hasHit;
}

struct QuadTree : IntersectionAccelerator {
	struct Node {
		BBox box;
		Node *children[8] = {nullptr, };
		std::vector<Intersectable*> primitives;
		bool isLeaf() const {
			return children[0] == nullptr;
		}
	};

	std::vector<Intersectable*> allPrimitives;
	Node *root = nullptr;
	int MAX_DEPTH = 35;
	int MIN_PRIMITIVES = 10;

	void clear(Node *n) {
		if (!n) {
			return;
		}

		for (int c = 0; c < 8; c++) {
			clear(n->children[c]);
			delete n->children[c];
		}
	}

	void clear() {
		clear(root);
		allPrimitives.clear();
	}

	void addPrimitive(Intersectable* prim) override {
		allPrimitives.push_back(prim);
	}

	void build(Node *n, int depth = 0) {
		if (depth >= MAX_DEPTH || n->primitives.size() <= MIN_PRIMITIVES) {
			return;
		}
		BBox childBoxes[8];
		n->box.octSplit(childBoxes);

		for (int c = 0; c < 8; c++) {
			Node *& child = n->children[c];
			child = new Node;
			memset(child->children, 0, sizeof(child->children));
			child->box = childBoxes[c];
			for (int r = 0; r < n->primitives.size(); r++) {
				if (n->primitives[r]->boxIntersect(child->box)) {
					child->primitives.push_back(n->primitives[r]);
				}
			}
			if (child->primitives.size() == n->primitives.size()) {
				build(child, MAX_DEPTH + 1);
			} else {
				build(child, depth + 1);
			}
		}
		n->primitives.clear();
	}

	void build(int maxDepth = -1, int minPrimitives = -1) override {
		if (maxDepth != -1) {
			MAX_DEPTH = maxDepth;
		}
		if (minPrimitives != -1) {
			MIN_PRIMITIVES = minPrimitives;
		}
		if (root) {
			clear(root);
			delete root;
		}
		printf("Building oct tree with %d primitives\n", int(allPrimitives.size()));
		root = new Node();
		root->primitives.swap(allPrimitives);
		for (int c = 0; c < root->primitives.size(); c++) {
			root->primitives[c]->expandBox(root->box);
		}
		build(root);
	}

	bool intersect(Node *n, const Ray& ray, float tMin, float tMax, Intersection& intersection) {
		float closest = tMax;
		bool hasHit = false;

		if (n->isLeaf()) {
			for (int c = 0; c < n->primitives.size(); c++) {
				Intersection data;
				if (n->primitives[c]->intersect(ray, tMin, tMax, data)) {
					if (data.t < closest) {
						intersection = data;
						closest = data.t;
						hasHit = true;
					}
				}
			}
		} else {
			for (int c = 0; c < 8; c++) {
				if (n->children[c]->box.testIntersect(ray)) {
					Intersection data;
					if (intersect(n->children[c], ray, tMin, tMax, data)) {
						if (data.t < closest) {
							intersection = data;
							closest = data.t;
							hasHit = true;
						}
					}
				}
			}
		}

		return hasHit;
	}

	bool intersect(const Ray& ray, float tMin, float tMax, Intersection& intersection) override {
		return intersect(root, ray, tMin, tMax, intersection);
	}

	bool isBuilt() const override {
		return root != nullptr;
	}

	~QuadTree() override {
		clear();
	}
};

AcceleratorPtr makeQuadTree() {
	return AcceleratorPtr(new QuadTree());
}

