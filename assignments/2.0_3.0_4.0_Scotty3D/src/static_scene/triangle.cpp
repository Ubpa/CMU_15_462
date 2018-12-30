#include "triangle.h"

#include "CMU462/CMU462.h"
#include "GL/glew.h"

using namespace CMU462;
using namespace StaticScene;

Triangle::Triangle(const Mesh* mesh, vector<size_t>& v) : mesh(mesh), v(v) {}
Triangle::Triangle(const Mesh* mesh, size_t v1, size_t v2, size_t v3)
	: mesh(mesh), v1(v1), v2(v2), v3(v3) {}

BBox Triangle::get_bbox() const {
	// compute the bounding box of the triangle

	Vector3D ps[3] = {
		mesh->positions[v1],
		mesh->positions[v2],
		mesh->positions[v3]
	};

	BBox box;
	for (size_t i = 0; i < 3; i++) {
		Vector3D p = ps[i];
		for (size_t j = 0; j < 3; j++) {
			if (box.min[j] > p[j])
				box.min[j] = p[j];
			
			if (box.max[j] < p[j])
				box.max[j] = p[j];
		}
	}

	for (size_t dim = 0; dim < 3; dim++) {
		if (box.min[dim] == box.max[dim]) {
			box.min[dim] -= 0.000001;
			box.max[dim] += 0.000001;
		}
	}
	
	return box;
}

bool Triangle::intersect(const Ray& r) const {
	// implement ray-triangle intersection. 

	Vector3D p1 = mesh->positions[v1];
	Vector3D p2 = mesh->positions[v2];
	Vector3D p3 = mesh->positions[v3];

	Vector3D e1 = p2 - p1;
	Vector3D e2 = p3 - p1;
	
	Vector3D e1_x_d = cross(e1, r.d);
	double denominator = dot(e1_x_d, e2);

	if (denominator == 0)
		return false;

	double inv_denominator = 1.0 / denominator;

	Vector3D s = r.o - p1;

	Vector3D e2_x_s = cross(e2, s);
	double r1 = dot(e2_x_s, r.d);
	double u = r1 * inv_denominator;
	if (u < 0 || u > 1)
		return false;

	double r2 = dot(e1_x_d, s);
	double v = r2 * inv_denominator;
	if (v < 0 || v > 1 || u + v > 1)
		return false;

	double r3 = dot(e2_x_s, e1);
	double t = 1 * inv_denominator;
	if (t < r.min_t + 0.001 || t > r.max_t)
		return false;

	r.max_t = t;

	return true;
}

bool Triangle::intersect(const Ray& r, Intersection* isect) const {
	// implement ray-triangle intersection. 
	// When an intersection takes place, 
	// the Intersection data should be updated accordingly

	Vector3D p1 = mesh->positions[v1];
	Vector3D p2 = mesh->positions[v2];
	Vector3D p3 = mesh->positions[v3];

	Vector3D e1 = p2 - p1;
	Vector3D e2 = p3 - p1;

	Vector3D e1_x_d = cross(e1, r.d);
	double denominator = dot(e1_x_d, e2);

	if (denominator == 0)
		return false;

	double inv_denominator = 1.0 / denominator;

	Vector3D s = r.o - p1;

	Vector3D e2_x_s = cross(e2, s);
	double r1 = dot(e2_x_s, r.d);
	double u = r1 * inv_denominator;
	if (u < 0 || u > 1)
		return false;

	double r2 = dot(e1_x_d, s);
	double v = r2 * inv_denominator;
	double u_plus_v = u + v;
	if (v < 0 || v > 1 || u_plus_v > 1)
		return false;

	double r3 = dot(e2_x_s, e1);
	double t = 1 * inv_denominator;
	if (t < r.min_t + 0.001 || t > r.max_t)
		return false;

	r.max_t = t;

	double w = 1 - u_plus_v;

	Vector3D n1 = mesh->normals[v1];
	Vector3D n2 = mesh->normals[v2];
	Vector3D n3 = mesh->normals[v3];
	Vector3D outwardN = u * n1 + v * n2 + w * n3;

	isect->n = dot(outwardN, r.d) < 0 ? outwardN : -outwardN;

	isect->primitive = this;
	
	isect->bsdf = mesh->get_bsdf();

	return true;
}

void Triangle::draw(const Color& c) const {
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_TRIANGLES);
	glVertex3d(mesh->positions[v1].x, mesh->positions[v1].y,
		mesh->positions[v1].z);
	glVertex3d(mesh->positions[v2].x, mesh->positions[v2].y,
		mesh->positions[v2].z);
	glVertex3d(mesh->positions[v3].x, mesh->positions[v3].y,
		mesh->positions[v3].z);
	glEnd();
}

void Triangle::drawOutline(const Color& c) const {
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINE_LOOP);
	glVertex3d(mesh->positions[v1].x, mesh->positions[v1].y,
		mesh->positions[v1].z);
	glVertex3d(mesh->positions[v2].x, mesh->positions[v2].y,
		mesh->positions[v2].z);
	glVertex3d(mesh->positions[v3].x, mesh->positions[v3].y,
		mesh->positions[v3].z);
	glEnd();
}
