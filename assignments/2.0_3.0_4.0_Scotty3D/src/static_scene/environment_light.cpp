#include "environment_light.h"

#include <unordered_set>

#include <random>

using namespace CMU462;
using namespace StaticScene;
using namespace std;

static uniform_real_distribution<double> dMap(0.0, 1.0);
static default_random_engine engine;

void EnvironmentLight::AliasTable::Init(const vector<double> & pTable) {
	const double partP = 1.0 / pTable.size();
	items.resize(pTable.size());

	unordered_set<size_t> spares;
	unordered_set<size_t> overflows;

	for (size_t i = 0; i < pTable.size(); i++) {
		items[i].idx[0] = i;
		items[i].spiltP = pTable[i];
		if (pTable[i] < partP)
			spares.insert(i);
		else if (pTable[i] > partP)
			overflows.insert(i);
		else
			items[i].idx[1] = -1;
	}

	for (auto i : overflows) {
		while (1) {
			// final one will overflow a little because of precision
			// so it is correct when next condition happen
			if (spares.empty())
				break;
			size_t j = *spares.begin();
			spares.erase(j);
			items[j].idx[1] = i;
			items[i].spiltP -= partP - items[j].spiltP;
			if (items[i].spiltP < partP) {
				spares.insert(i);
				break;
			}
			else if (items[i].spiltP == partP) {
				items[i].idx[1] = -1;
				break;
			}
		}
	}
}

// p : [0, 1)
size_t EnvironmentLight::AliasTable::Sample(double p) const {
	// O(1)
	size_t ID = p * items.size();
	double leftP = p * items.size() - ID;
	if (leftP < items[ID].spiltP)
		return items[ID].idx[0];
	else
		return items[ID].idx[1];
}


EnvironmentLight::EnvironmentLight(const HDRImageBuffer* envMap)
	: envMap(envMap) {
	size_t w = envMap->w;
	size_t h = envMap->h;
	double thetaStep = PI / h;

	double sum = 0;
	p = vector<double>(w*h);
	double theta = 0.5 * thetaStep;
	for (size_t y = 0; y < h; y++, theta += thetaStep) {
		for (size_t x = 0; x < w; x++) {
			size_t idx = x + y * w;
			Spectrum color = envMap->data[idx];
			p[idx] = color.illum() * sin(theta);
			sum += p[idx];
		}
	}

	for (size_t i = 0; i < w*h; i++)
		p[i] /= sum;

	table.Init(p);

	printf("[EnvironmentLight]\n"
		"width: %d\n"
		"height: %d\n", w, h);
}

Spectrum EnvironmentLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	double rd = dMap(engine);
	size_t idx = table.Sample(rd);
	double texcX = ((idx % envMap->w) + dMap(engine)) / envMap->w;
	double texcY = (idx + dMap(engine)) / envMap->w;
	double theta = PI * texcY;
	double phi = 2 * PI * texcX;
	*pdf = this->p[idx] / envMap->w / envMap->h;
	wi->x = sin(theta) * sin(phi);
	wi->z = sin(theta) * cos(phi);
	wi->y = cos(theta);
	*distToLight = FLT_MAX;
	return sample_dir(*wi);
}

float EnvironmentLight::pdf(const Vector3D& p, const Vector3D& wi) {
	size_t w = envMap->w;
	size_t h = envMap->h;
	double theta = acos(wi.y);
	double phi = atan2(wi.x, wi.z) + PI;
	double texcX = phi / (2.0 * PI);
	double texcY = theta / PI;

	int x = texcX * w;
	int y = texcY * h;
	int idx = x + y * w;
	return this->p[idx];
}

Spectrum EnvironmentLight::sample_dir(const Vector3D& dir) const {
	size_t w = envMap->w;
	size_t h = envMap->h;
	double theta = acos(dir.y);
	double phi = atan2(dir.x, dir.z) + PI;
	double texcX = phi / (2.0 * PI);
	double texcY = theta / PI;

	double xd = texcX * w;
	double yd = texcY * h;

	int x0 = clamp<int>(xd, 0, w - 1);
	int x1 = clamp<int>(x0 + ((xd - x0) < 0.5 ? -1 : 1), 0, w - 1);
	int y0 = clamp<int>(yd, 0, h - 1);
	int y1 = clamp<int>(y0 + ((yd - y0) < 0.5 ? -1 : 1), 0, h - 1);
	int idx[4] = {
		y0 * w + x0,
		y0 * w + x1,
		y1 * w + x0,
		y1 * w + x1
	};

	Spectrum colors[4];
	for (int i = 0; i < 4; i++)
		colors[i] = envMap->data[idx[i]];

	double tx = abs(xd - (x0 + 0.5));
	double ty = abs(yd - (y0 + 0.5));
	Spectrum mixColor = (1 - ty)*((1 - tx)*colors[0] + tx * colors[1]) + ty * ((1 - tx)*colors[2] + tx * colors[3]);

	return mixColor;
}