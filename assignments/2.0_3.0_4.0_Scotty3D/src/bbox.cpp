#include "bbox.h"

#include "GL/glew.h"

#include <algorithm>
#include <iostream>

using namespace CMU462;

bool BBox::intersect(const Ray &r, double &t0, double &t1) const {
	// Implement ray - bounding box intersection test
	// If the ray intersected the bounding box within the range given by
	// t0, t1, update t0 and t1 with the new intersection times.

	const Vector3D origin = r.o;
	const Vector3D dir = r.d;
	float tMin = r.min_t;
	float tMax = r.max_t;
	for (size_t i = 0; i < 3; i++) {
		float invD = r.inv_d[i];
		float t0 = (min[i] - origin[i]) * invD;
		float t1 = (max[i] - origin[i]) * invD;
		if (invD < 0.0f)
			std::swap(t0, t1);

		tMin = std::max(t0, tMin);
		tMax = std::min(t1, tMax);
		if (tMax <= tMin)
			return false;
	}

	t0 = tMin;
	t1 = tMax;

	return true;
}

void BBox::draw(Color c) const {
	glColor4f(c.r, c.g, c.b, c.a);

	// top
	glBegin(GL_LINE_STRIP);
	glVertex3d(max.x, max.y, max.z);
	glVertex3d(max.x, max.y, min.z);
	glVertex3d(min.x, max.y, min.z);
	glVertex3d(min.x, max.y, max.z);
	glVertex3d(max.x, max.y, max.z);
	glEnd();

	// bottom
	glBegin(GL_LINE_STRIP);
	glVertex3d(min.x, min.y, min.z);
	glVertex3d(min.x, min.y, max.z);
	glVertex3d(max.x, min.y, max.z);
	glVertex3d(max.x, min.y, min.z);
	glVertex3d(min.x, min.y, min.z);
	glEnd();

	// side
	glBegin(GL_LINES);
	glVertex3d(max.x, max.y, max.z);
	glVertex3d(max.x, min.y, max.z);
	glVertex3d(max.x, max.y, min.z);
	glVertex3d(max.x, min.y, min.z);
	glVertex3d(min.x, max.y, min.z);
	glVertex3d(min.x, min.y, min.z);
	glVertex3d(min.x, max.y, max.z);
	glVertex3d(min.x, min.y, max.z);
	glEnd();
}

std::ostream &operator<<(std::ostream &os, const BBox &b) {
	return os << "BBOX(" << b.min << ", " << b.max << ")";
}
