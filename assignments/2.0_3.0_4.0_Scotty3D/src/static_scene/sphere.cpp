#include "sphere.h"

#include <cmath>

#include "../bsdf.h"
#include "../misc/sphere_drawing.h"

using namespace CMU462;
using namespace StaticScene;

bool Sphere::test(const Ray& r, double& t1, double& t2) const {
	// Implement ray - sphere intersection test.
	// Return true if there are intersections and writing the
	// smaller of the two intersection times in t1 and the larger in t2.

	double a = r.d.norm2();
	Vector3D co = r.o - o;
	double b = dot(r.d, co);
	double c = co.norm2() - this->r * this->r;

	double discriminant = b * b - a * c;
	if (discriminant <= 0)
		return false;

	double sqrt_discriminant = sqrt(discriminant);
	double inv_a = 1.0 / a;

	t1 = (-b - sqrt_discriminant) * inv_a;
	t2 = (-b + sqrt_discriminant) * inv_a;

	return true;
}

bool Sphere::intersect(const Ray& r) const {
	// Implement ray - sphere intersection.
	// Note that you might want to use the the Sphere::test helper here.

	double t1, t2;
	if (!test(r, t1, t2))
		return false;

	if (t1 < r.min_t) {
		if (t2 < r.min_t)
			return false;
		else if (t2 < r.max_t) {
			r.max_t = t2;
			return true;
		}
		else
			return false;
	}
	else if (t1 < r.max_t) {
		r.max_t = t1;
		return true;
	}
	else
		return false;
}

bool Sphere::intersect(const Ray& r, Intersection* isect) const {
	// Implement ray - sphere intersection.
	// Note again that you might want to use the the Sphere::test helper here.
	// When an intersection takes place, the Intersection data should be updated
	// correspondingly.

	double t1, t2;
	if (!test(r, t1, t2))
		return false;

	double t;

	if (t1 < r.min_t) {
		if (t2 < r.min_t)
			return false;
		else if (t2 < r.max_t)
			t = t2;
		else
			return false;
	}
	else if (t1 < r.max_t)
		t = t1;
	else
		return false;

	r.max_t = t;

	isect->t = t;
	isect->primitive = this;
	isect->bsdf = object->get_bsdf();
	isect->n = (r.o + t * r.d - o) / this->r;

	return true;
}

void Sphere::draw(const Color& c) const { Misc::draw_sphere_opengl(o, r, c); }

void Sphere::drawOutline(const Color& c) const {
	// Misc::draw_sphere_opengl(o, r, c);
}