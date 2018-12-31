#include "bsdf.h"

#include <algorithm>
#include <iostream>
#include <utility>
#include <random>

using namespace std;

using namespace CMU462;

static uniform_real_distribution<double> dMap(0.0, 1.0);
static default_random_engine engine;

void CMU462::make_coord_space(Matrix3x3& o2w, const Vector3D& n) {
	Vector3D z = Vector3D(n.x, n.y, n.z);
	Vector3D h = z;
	if (fabs(h.x) <= fabs(h.y) && fabs(h.x) <= fabs(h.z))
		h.x = 1.0;
	else if (fabs(h.y) <= fabs(h.x) && fabs(h.y) <= fabs(h.z))
		h.y = 1.0;
	else
		h.z = 1.0;

	z.normalize();
	Vector3D y = cross(h, z);
	y.normalize();
	Vector3D x = cross(z, y);
	x.normalize();

	o2w[0] = x;
	o2w[1] = y;
	o2w[2] = z;
}

// Diffuse BSDF //

Spectrum DiffuseBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	return albedo * (1.0 / PI);
}

Spectrum DiffuseBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	// Implement DiffuseBSDF
	double Xi1 = dMap(engine);
	double Xi2 = dMap(engine);
	
	double a = sqrt(1 - Xi1 * Xi1);
	wi->x = a * cos(2 * PI * Xi2);
	wi->y = a * sin(2 * PI * Xi2);
	wi->z = Xi1;
	*pdf = this->pdf(wo, *wi);

	return albedo * (1.0 / PI);
}

float DiffuseBSDF::pdf(const Vector3D& wo, const Vector3D& wi) {
	return 1.0f / (2 * PI);
}

// Mirror BSDF //

Spectrum MirrorBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	return Spectrum();
}

Spectrum MirrorBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	// Implement MirrorBSDF

	reflect(wo, wi);

	*pdf = 1.0;

	return 1.0 / abs(wi->z) * reflectance;
}

// Glossy BSDF //

/*
Spectrum GlossyBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum GlossyBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
  *pdf = 1.0f;
  return reflect(wo, wi, reflectance);
}
*/

// Refraction BSDF //

Spectrum RefractionBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	return Spectrum();
}

Spectrum RefractionBSDF::sample_f(const Vector3D& wo, Vector3D* wi,
	float* pdf) {
	// Implement RefractionBSDF
	// *pdf = FLT_MAX;
	*pdf = 1.0f;

	if (!refract(wo, wi, ior))
		reflect(wo, wi);

	return 1.0 / abs(wi->z) * transmittance;
}

// Glass BSDF //

Spectrum GlassBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	return Spectrum();
}

Spectrum GlassBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	// Compute Fresnel coefficient and either reflect or refract based on it.

	if (!refract(wo, wi, ior)) {
		*pdf = 1.0f;
		reflect(wo, wi);
		return 1.0 / abs(wi->z) * reflectance;
	}

	float cosTheta = wo.z >= 0 ? wo.z : wi->z;

	float R0 = pow((ior - 1) / (ior + 1), 2);

	float Fr = R0 + (1 - R0) * pow((1 - cosTheta), 5);

	if (dMap(engine) < Fr) {
		*pdf = Fr;
		reflect(wo, wi);
		return Fr / abs(wi->z) * reflectance;
	}

	*pdf = 1 - Fr;
	float iorRatio = wo.z > 0 ? 1.0f / ior : ior;
	float attenuation = iorRatio * iorRatio * (1 - Fr) / abs(wi->z);
	return attenuation * transmittance;
}

void BSDF::reflect(const Vector3D& wo, Vector3D* wi) {
	// Implement reflection of wo about normal (0,0,1) and store result in wi.

	wi->x = - wo.x;
	wi->y = - wo.y;
	wi->z = wo.z;
}

bool BSDF::refract(const Vector3D& wo, Vector3D* wi, float ior) {
	// Use Snell's Law to refract wo surface and store result ray in wi.
	// Return false if refraction does not occur due to total internal reflection
	// and true otherwise. When dot(wo,n) is positive, then wo corresponds to a
	// ray entering the surface through vacuum.

	float inv = wo.z >= 0 ? 1.0 / ior : ior;

	double discriminant = 1 - (1 - wo.z * wo.z) * inv * inv;
	if (discriminant <= 0)
		return false;

	wi->x = - wo.x * inv;
	wi->y = - wo.y * inv;
	wi->z = (wo.z >= 0 ? -1 : 1) * sqrt(discriminant);
	wi->normalize();

	return true;
}

// Emission BSDF //

Spectrum EmissionBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	return Spectrum();
}

Spectrum EmissionBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	*wi = sampler.get_sample(pdf);
	return Spectrum();
}
