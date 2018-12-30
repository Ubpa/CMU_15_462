#include "bvh.h"

#include "CMU462/CMU462.h"
#include "static_scene/triangle.h"

#include <iostream>
#include <stack>

using namespace std;

using namespace CMU462;
using namespace StaticScene;

BVHNode::~BVHNode() {
	delete l;
	delete r;
}


void BVHNode::Build(vector<Primitive *> & primitives, size_t maxLeafSize) {
	// Build bvh form start to start + range

	constexpr size_t bucketNum = 8;

	for (size_t i = start; i < start + range; i++)
		bb.expand(primitives[i]->get_bbox());

	if (range < maxLeafSize) {
		l = nullptr;
		r = nullptr;
		return;
	}

	// get best partition
	vector<Primitive *> bestPartition[2];
	double minCost = DBL_MAX;
	for (size_t dim = 0; dim < 3; dim++) {
		// 1. compute buckets
		double bucketLen = bb.extent[dim] / bucketNum;
		double left = bb.min[dim];
		vector<vector<Primitive *>> buckets(bucketNum);
		vector<BBox> boxesOfBuckets(bucketNum);
		for (size_t i = 0; i < range; i++) {
			BBox box = primitives[i + start]->get_bbox();
			double center = box.centroid()[dim];
			size_t bucketID = (center - left) / bucketLen;
			buckets[bucketID].push_back(primitives[i + start]);
			boxesOfBuckets[bucketID].expand(box);
		}

		// 2. accumulate buckets
		vector<BBox> leftBox(bucketNum);
		vector<size_t> leftAccNum(bucketNum);
		leftAccNum[0] = 0;
		vector<BBox> rightBox(bucketNum);
		vector<size_t> rightAccNum(bucketNum);
		rightAccNum[0] = 0;
		for (size_t i = 1; i <= bucketNum - 1; i++) {
			leftBox[i] = leftBox[i - 1];
			leftBox[i].expand(boxesOfBuckets[i - 1]);
			leftAccNum[i] = leftAccNum[i - 1] + buckets[i - 1].size();
			rightBox[i] = rightBox[i - 1];
			rightBox[i].expand(boxesOfBuckets[bucketNum - i]);
			rightAccNum[i] = rightAccNum[i - 1] + buckets[bucketNum - i].size();
		}

		// 3. get best partition of dim
		size_t bestLeftNum = 0;
		double minCostDim = DBL_MAX;
		for (size_t leftNum = 1; leftNum <= bucketNum - 1; leftNum++) {
			size_t rightNum = bucketNum - leftNum;
			double leftS = leftBox[leftNum].surface_area();
			double rightS = rightBox[rightNum].surface_area();
			double costDim = leftS * leftAccNum[leftNum] + rightS * rightAccNum[rightNum];
			if (costDim < minCostDim) {
				bestLeftNum = leftNum;
				minCostDim = costDim;
			}
		}

		// 4. set best partition
		if (minCostDim < minCost) {
			bestPartition[0].clear();
			bestPartition[1].clear();

			minCost = minCostDim;
			for (size_t i = 0; i < bestLeftNum; i++) {
				for (auto primitive : buckets[i])
					bestPartition[0].push_back(primitive);
			}
			for (size_t i = bestLeftNum; i < bucketNum; i++) {
				for (auto primitive : buckets[i])
					bestPartition[1].push_back(primitive);
			}
		}
	}

	// recursion
	if (bestPartition[0].size() == range || bestPartition[1].size() == range) {
		size_t leftNum = range / 2;
		l = new BVHNode(primitives, maxLeafSize, start, leftNum);
		r = new BVHNode(primitives, maxLeafSize, start + leftNum, range - leftNum);
	}
	else {
		for (size_t i = 0; i < bestPartition[0].size(); i++)
			primitives[i + start] = bestPartition[0][i];
		for (size_t i = 0; i < bestPartition[1].size(); i++)
			primitives[i + start + bestPartition[0].size()] = bestPartition[1][i];

		l = new BVHNode(primitives, maxLeafSize, start, bestPartition[0].size());
		r = new BVHNode(primitives, maxLeafSize, start + bestPartition[0].size(), bestPartition[1].size());
	}
}

bool BVHNode::intersect(const vector<Primitive*>& primitives, const Ray &ray) const {
	// Ray - BVHNode intersection

	int hit = 0;
	if (isLeaf()) {
		for (size_t i = 0; i < range; i++)
			if (primitives[i + start]->intersect(ray))
				hit = true;

		return hit;
	}
	else {
		double t1, t2, t3, t4;
		bool leftBoxHit = l->bb.intersect(ray, t1, t2);
		bool rightBoxHit = r->bb.intersect(ray, t3, t4);
		if (leftBoxHit) {
			if (rightBoxHit) {
				BVHNode* first = (t1 <= t3) ? l : r;
				BVHNode* second = (t1 <= t3) ? r : l;
				hit += first->intersect(primitives, ray);
				if (t3 < ray.max_t)
					hit += second->intersect(primitives, ray);

				return hit;
			}
			else
				return l->intersect(primitives, ray);
		}
		else if (rightBoxHit)
			return r->intersect(primitives, ray);
		else
			return false;
	}
}

bool BVHNode::intersect(const vector<Primitive*>& primitives, const Ray& ray, Intersection* isect) const {
	// Ray - BVHNode intersection

	int hit = 0;

	if (isLeaf()) {
		for (size_t i = 0; i < range; i++)
			hit += primitives[i + start]->intersect(ray, isect);

		return hit;
	}
	else {
		double t1, t2, t3, t4;
		bool leftBoxHit = l->bb.intersect(ray, t1, t2);
		bool rightBoxHit = r->bb.intersect(ray, t3, t4);
		if (leftBoxHit) {
			if (rightBoxHit) {
				BVHNode* first = (t1 <= t3) ? l : r;
				BVHNode* second = (t1 <= t3) ? r : l;
				hit += first->intersect(primitives, ray, isect);
				if (t3 < isect->t)
					hit += second->intersect(primitives, ray, isect);

				return hit;
			}
			else
				return l->intersect(primitives, ray, isect);
		}
		else if (rightBoxHit)
			return r->intersect(primitives, ray, isect);
		else
			return false;
	}
}

BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
	size_t max_leaf_size) 
	: Aggregate(_primitives)
{
	// Construct a BVH from the given vector of primitives and maximum leaf
	// size configuration. The starter code build a BVH aggregate with a
	// single leaf node (which is also the root) that encloses all the
	// primitives.

	root = new BVHNode(this->primitives, max_leaf_size, 0, primitives.size());
}


BVHAccel::~BVHAccel() {
	// Implement a proper destructor for your BVH accelerator aggregate
	delete root;
}

BBox BVHAccel::get_bbox() const { return root->bb; }

bool BVHAccel::intersect(const Ray &ray) const {
	// Implement ray - bvh aggregate intersection test. A ray intersects
	// with a BVH aggregate if and only if it intersects a primitive in
	// the BVH that is not an aggregate.
	
	return root->intersect(primitives, ray);
}

bool BVHAccel::intersect(const Ray &ray, Intersection *isect) const {
	// Implement ray - bvh aggregate intersection test. A ray intersects
	// with a BVH aggregate if and only if it intersects a primitive in
	// the BVH that is not an aggregate. When an intersection does happen.
	// You should store the non-aggregate primitive in the intersection data
	// and not the BVH aggregate itself.

	return root->intersect(primitives, ray, isect);
}