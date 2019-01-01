#include "sampler.h"
#include <random>

using namespace CMU462;

using namespace std;

static uniform_real_distribution<double> dMap(0.0, 1.0);
static default_random_engine engine;

// Uniform Sampler2D Implementation //

Vector2D UniformGridSampler2D::get_sample() const {
	// Implement uniform 2D grid sampler
	double x = dMap(engine);
	double y = dMap(engine);

	return Vector2D(x, y);
}

// Uniform Hemisphere Sampler3D Implementation //

Vector3D UniformHemisphereSampler3D::get_sample() const {
	double Xi1 = dMap(engine);
	double Xi2 = dMap(engine);

	double cosTheta = Xi1;

	double sinTheta = sin(acos(Xi1));
	double phi = 2.0 * PI * Xi2;

	double xs = sinTheta * cos(phi);
	double ys = sinTheta * sin(phi);
	double zs = cosTheta;

	return Vector3D(xs, ys, zs);
}

Vector3D CosineWeightedHemisphereSampler3D::get_sample() const {
	float f;
	return get_sample(&f);
}

Vector3D CosineWeightedHemisphereSampler3D::get_sample(float *pdf) const {
	double Xi1 = dMap(engine);
	double Xi2 = dMap(engine);

	double cosTheta = Xi1 * Xi1;

	double sinTheta = sin(acos(cosTheta));
	double phi = 2.0 * PI * Xi2;

	double xs = sinTheta * cos(phi);
	double ys = sinTheta * sin(phi);
	double zs = cosTheta;

	*pdf = sinTheta * cosTheta / PI;

	return Vector3D(xs, ys, zs);
}