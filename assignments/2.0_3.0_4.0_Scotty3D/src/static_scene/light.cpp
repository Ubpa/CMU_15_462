#include "light.h"

#include <iostream>

#include "../sampler.h"

using namespace CMU462;
using namespace StaticScene;

// Directional Light //

DirectionalLight::DirectionalLight(const Spectrum& rad,
	const Vector3D& lightDir)
	: radiance(rad) {
	dirToLight = -lightDir.unit();
}

Spectrum DirectionalLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	*wi = dirToLight;
	*distToLight = INF_D;
	*pdf = 1.0f;
	return radiance;
}

float DirectionalLight::pdf(const Vector3D& p, const Vector3D& wi) { return 1.0f; }

// Infinite Hemisphere Light //

InfiniteHemisphereLight::InfiniteHemisphereLight(const Spectrum& rad)
	: radiance(rad) {
	sampleToWorld[0] = Vector3D(1, 0, 0);
	sampleToWorld[1] = Vector3D(0, 0, -1);
	sampleToWorld[2] = Vector3D(0, 1, 0);
}

Spectrum InfiniteHemisphereLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight,
	float* pdf) const {
	Vector3D dir = sampler.get_sample();
	*wi = sampleToWorld * dir;
	*distToLight = INF_D;
	*pdf = 1.0 / (2.0 * M_PI);
	return radiance;
}

float InfiniteHemisphereLight::pdf(const Vector3D& p, const Vector3D& wi) {
	if (wi.z < 0)
		return 1.0 / (2.0 * M_PI);
	else
		return 0;
}

// Point Light //

PointLight::PointLight(const Spectrum& rad, const Vector3D& pos)
	: radiance(rad), position(pos) {}

Spectrum PointLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	Vector3D d = position - p;
	*wi = d.unit();
	*distToLight = d.norm();
	*pdf = 1.0f;
	return radiance;
}

float PointLight::pdf(const Vector3D& p, const Vector3D& wi) {
	return 1.0f;
}

// Spot Light //

SpotLight::SpotLight(const Spectrum& rad, const Vector3D& pos,
	const Vector3D& dir, float angle) {}

Spectrum SpotLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	return Spectrum();
}

// Area Light //

AreaLight::AreaLight(const Spectrum& rad, const Vector3D& pos,
	const Vector3D& dir, const Vector3D& dim_x,
	const Vector3D& dim_y)
	: radiance(rad),
	position(pos),
	direction(dir),
	dim_x(dim_x),
	dim_y(dim_y),
	area(dim_x.norm() * dim_y.norm()) {
	direction.normalize();
	printf("[AreaLight]\n"
		"radiance:(%.3f,%.3f,%.3f)\n", rad.r, rad.g, rad.b);
}

Spectrum AreaLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	Vector2D sample = sampler.get_sample() - Vector2D(0.5, 0.5);
	Vector3D d = position + sample.x * dim_x + sample.y * dim_y - p;
	double sqDist = d.norm2();
	double dist = sqrt(sqDist);
	*wi = d / dist;
	double cosTheta = dot(*wi, direction);
	*distToLight = dist;
	*pdf = sqDist / (area * fabs(cosTheta));
	return cosTheta < 0 ? radiance : Spectrum();
};

float AreaLight::pdf(const Vector3D& p, const Vector3D& wi) {
	Vector3D e1 = dim_x;
	Vector3D e2 = dim_y;

	Vector3D e1_x_d = cross(e1, wi);
	double denominator = dot(e1_x_d, e2);

	if (denominator == 0)
		return 0;

	double inv_denominator = 1.0 / denominator;

	Vector3D s = p - position;

	Vector3D e2_x_s = cross(e2, s);
	double r1 = dot(e2_x_s, wi);
	double u = r1 * inv_denominator;
	const double bound = 0.5 + EPS_D;
	if (u < -bound || u > bound)
		return 0;

	double r2 = dot(e1_x_d, s);
	double v = r2 * inv_denominator;
	if (v < -bound || v > bound)
		return 0;

	Vector3D d = position + u * dim_x + v * dim_y - p;
	double sqDist = d.norm2();
	double cosTheta = dot(d, direction) / sqrt(sqDist);

	return sqDist / (area * abs(cosTheta));
}

// Sphere Light //

SphereLight::SphereLight(const Spectrum& rad, const SphereObject* sphere) {}

Spectrum SphereLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	return Spectrum();
}

// Mesh Light

MeshLight::MeshLight(const Spectrum& rad, const Mesh* mesh) {}

Spectrum MeshLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	return Spectrum();
}