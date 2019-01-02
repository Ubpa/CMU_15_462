/*
 * Implementations for Joint Based Skeletons.
 *
 * Started on October 29th, 2015 by Bryce Summers.
 */

#include "joint.h"
#include "skeleton.h"
#include "mesh.h"

#include "GL/glew.h"

#include <iostream>

using namespace CMU462;
using namespace DynamicScene;

BBox Joint::get_bbox() {
	BBox bbox;
	Vector3D p1 = position;
	Vector3D p2 = p1 + axis;
	bbox.expand(p1);
	bbox.expand(p2);
	return bbox;
}

Info Joint::getInfo() {
	Info info;

	if (!scene || !scene->selected.element) {
		info.push_back("JOINT");
	}
	else {
		info = scene->selected.element->getInfo();
	}

	return info;
}

void Joint::drag(double x, double y, double dx, double dy,
	const Matrix4x4& modelViewProj) {
	Vector4D q(position, 1.);

	// Transform into clip space
	q = modelViewProj * q;
	double w = q.w;
	q /= w;

	// Shift by (dx, dy).
	q.x += dx;
	q.y += dy;

	// Transform back into model space
	q *= w;
	q = modelViewProj.inv() * q;

	Matrix4x4 T = Matrix4x4::identity();
	for (Joint* j = this; j != nullptr; j = j->parent) {
		T = j->getRotation() * T;
	}
	T = skeleton->mesh->getRotation() * T;
	q = T.inv() * q;

	if (skeleton->root == this)
		position = q.to3D();
	else
		axis += q.to3D();
}

StaticScene::SceneObject* Joint::get_static_object() { return nullptr; }

// The real calculation.
void Joint::calculateAngleGradient(Joint* goalJoint, Vector3D q) {
	// (Animation) task 2B

	vector<Joint *> allJs;
	for (auto j = goalJoint; j != this->parent && j != nullptr; j = j->parent)
		allJs.push_back(j);

	if (allJs.size() == 0 || allJs.back() != this)
		return;

	Matrix4x4 T = getTransformation();
	Vector3D endP = goalJoint->getEndPosInWorld();
	Vector3D delta = endP - q;

	// form this to goalJoint
	for (auto iter = allJs.rbegin(); iter != allJs.rend(); iter++) {
		Joint * pJ = *iter;
		Vector3D basePos = T[3].projectTo3D();

		Vector4D axisXQ = T * Vector4D(1, 0, 0, 0);
		Vector3D axisX = Vector3D(axisXQ.x, axisXQ.y, axisXQ.z).unit();
		Vector4D axisYQ = T * Vector4D(0, 1, 0, 0);
		Vector3D axisY = Vector3D(axisYQ.x, axisYQ.y, axisYQ.z).unit();
		Vector4D axisZQ = T * Vector4D(0, 0, 1, 0);
		Vector3D axisZ = Vector3D(axisZQ.x, axisZQ.y, axisZQ.z).unit();

		Vector3D r = endP - basePos;

		pJ->ikAngleGradient.x += dot(cross(axisX, r), delta);
		pJ->ikAngleGradient.y += dot(cross(axisY, r), delta);
		pJ->ikAngleGradient.z += dot(cross(axisZ, r), delta);

		T = T * pJ->getRotation() * Matrix4x4::translation(pJ->axis);
	}
}

// The constructor sets the dynamic angle and velocity of
// the joint to zero (at a perfect vertical with no motion)
Joint::Joint(Skeleton* s) : skeleton(s), capsuleRadius(0.05), renderScale(1.0) {
	scale = Vector3D(1., 1., 1.);
	scales.setValue(0, scale);
}

Vector3D Joint::getAngle(double time) { return rotations(time); }

void Joint::setAngle(double time, Vector3D value) {
	rotations.setValue(time, value);
}

bool Joint::removeAngle(double time) { return rotations.removeKnot(time, .1); }

void Joint::keyframe(double t) {
	positions.setValue(t, position);
	rotations.setValue(t, rotation);
	scales.setValue(t, scale);
	for (Joint* j : kids) j->keyframe(t);
}

void Joint::unkeyframe(double t) {
	positions.removeKnot(t, 0.1);
	rotations.removeKnot(t, 0.1);
	scales.removeKnot(t, 0.1);
	for (Joint* j : kids) j->unkeyframe(t);
}

void Joint::removeJoint(Scene* scene) {
	if (this == skeleton->root) return;

	for (auto childJoint : kids) {
		childJoint->removeJoint(scene);
	}

	scene->removeObject(this);

	auto& kids = parent->kids;
	kids.erase(std::remove(kids.begin(), kids.end(), this), kids.end());

	auto& joints = skeleton->joints;
	joints.erase(std::remove(joints.begin(), joints.end(), this), joints.end());

	delete this;
}

void Joint::getAxes(vector<Vector3D>& axes) {
	Matrix4x4 T = Matrix4x4::identity();
	for (Joint* j = parent; j != nullptr; j = j->parent) {
		T = j->getRotation() * T;
	}
	T = skeleton->mesh->getRotation() * T;
	axes.resize(3);
	axes[0] = T * Vector3D(1., 0., 0.);
	axes[1] = T * Vector3D(0., 1., 0.);
	axes[2] = T * Vector3D(0., 0., 1.);
}

Matrix4x4 Joint::getTransformation() {
	/* (Animation) Task 2a
	Initialize a 4x4 identity transformation matrix. Traverse the hierarchy
	starting from the parent of this joint all the way up to the root (root has
	parent of nullptr) and accumulate their transformations on the left side of
	your transformation matrix. Don't forget to apply a translation which extends
	along the axis of those joints. Finally, apply the mesh's transformation at
	the end.
	*/

	Matrix4x4 T = Matrix4x4::identity();
	for (Joint* j = parent; j != nullptr; j = j->parent) {
		// because we go form parent to root, so matrices multiply at the left side of T
		T = Matrix4x4::translation(j->axis) * T;
		T = j->getRotation() * T;
	}

	T = Matrix4x4::translation(skeleton->mesh->position) * T;

	return T;
}

Matrix4x4 Joint::getBindTransformation() {
	Matrix4x4 T = Matrix4x4::identity();
	for (Joint* j = parent; j != nullptr; j = j->parent) {
		T = Matrix4x4::translation(j->axis) * T;
	}

	// Allow skeleton translation by taking root's position into account
	T = Matrix4x4::translation(skeleton->root->position) * T;

	return T;
}

Vector3D Joint::getBasePosInWorld() {
	/* (Animation) Task 2a
	This should be fairly simple once you implement Joint::getTransform(). You can
	utilize the transformation returned by Joint::getTransform() to compute the
	base position in world coordinate frame.
	*/

	Matrix4x4 tfm = getTransformation();

	return tfm[3].projectTo3D();
}

Vector3D Joint::getEndPosInWorld() {
	/* (Animation) Task 2a
	In addition to what you did for getBasePosInWorld(), you need to apply this
	joint's transformation and translate along this joint's axis to get the end
	position in world coordinate frame.
	*/
	
	Matrix4x4 T = getTransformation() * getRotation() * Matrix4x4::translation(axis);

	return T[3].projectTo3D();
}