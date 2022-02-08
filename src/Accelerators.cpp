#include "Primitive.h"
#include "threading.hpp"

struct OctTree : IntersectionAccelerator {
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
	int depth = 0;
	int leafSize = 0;
	int nodes = 0;
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

	void build(Node *n, int currentDepth = 0) {
		if (currentDepth >= MAX_DEPTH || n->primitives.size() <= MIN_PRIMITIVES) {
			leafSize = std::max(leafSize, int(n->primitives.size()));
			return;
		}
		depth = std::max(depth, currentDepth);
		BBox childBoxes[8];
		n->box.octSplit(childBoxes);
		nodes = leafSize = currentDepth = 0;
		for (int c = 0; c < 8; c++) {
			Node *& child = n->children[c];
			child = new Node;
			nodes++;
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
				build(child, currentDepth + 1);
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
		printf("Building oct tree with %d primitives... ", int(allPrimitives.size()));
		Timer timer;
		root = new Node();
		root->primitives.swap(allPrimitives);
		for (int c = 0; c < root->primitives.size(); c++) {
			root->primitives[c]->expandBox(root->box);
		}
		build(root);
		printf(" done in %lldms, nodes %d, depth %d, %d leaf size\n", timer.toMs(timer.elapsedNs()), nodes, depth, leafSize);
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

	~OctTree() override {
		clear();
	}
};

/// TODO: Implement one/both or any other acceleration structure and change makeDefaultAccelerator to create it
struct KDTree : IntersectionAccelerator {
	void addPrimitive(Intersectable *prim) override = 0;
	void clear() override = 0;
	void build(int maxDepth = -1, int minPrimitives = -1) override = 0;
	bool isBuilt() const override = 0;
	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override = 0;
};


struct BVHTree : IntersectionAccelerator {
	void addPrimitive(Intersectable *prim) override = 0;
	void clear() override = 0;
	void build(int maxDepth = -1, int minPrimitives = -1) override = 0;
	bool isBuilt() const override = 0;
	bool intersect(const Ray &ray, float tMin, float tMax, Intersection &intersection) override = 0;
};

AcceleratorPtr makeDefaultAccelerator() {
	// TODO: uncomment or add the acceleration structure you have implemented
	//return AcceleratorPtr(new KDTree());
	//return AcceleratorPtr(new BVHTree());
	return AcceleratorPtr(new OctTree());
}

