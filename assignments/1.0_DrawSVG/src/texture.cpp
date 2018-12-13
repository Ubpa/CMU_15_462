#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {
    MipLevel& mip = tex.mipmap[i];
	for (size_t y = 0; y < mip.height; y++) {
	  for (size_t x = 0; x < mip.width; x++) {
		size_t pos[4][2] = { {2 * x,2 * y},{2 * x + 1,2 * y},{2 * x,2 * y + 1},{2 * x + 1,2 * y + 1} };
		Color sumColor(0, 0, 0, 0);
		for (size_t k = 0; k < 4; k++) {
		  size_t curIdx = 4 * (pos[k][0] + pos[k][1] * tex.mipmap[i - 1].width);
		  float a = tex.mipmap[i - 1].texels[curIdx + 3];
		  float r = a*tex.mipmap[i - 1].texels[curIdx + 0];
		  float g = a*tex.mipmap[i - 1].texels[curIdx + 1];
		  float b = a*tex.mipmap[i - 1].texels[curIdx + 2];
		  sumColor += Color(r, g, b, a);
		}
		sumColor *= 0.25f;
		if (sumColor.a != 0) {
			sumColor.r *= 1.0f / sumColor.a;
			sumColor.g *= 1.0f / sumColor.a;
			sumColor.b *= 1.0f / sumColor.a;
		}
		mip.texels[4 * (y*mip.width + x) + 0] = sumColor.r;
		mip.texels[4 * (y*mip.width + x) + 1] = sumColor.g;
		mip.texels[4 * (y*mip.width + x) + 2] = sumColor.b;
		mip.texels[4 * (y*mip.width + x) + 3] = sumColor.a;
      }
	}
  }
}

Color Sampler2DImp::sample_nearest(Texture& tex, float u, float v, int level) {

	// Task 6: Implement nearest neighbour interpolation

	// return magenta for invalid level
	if(level >= tex.mipmap.size())
		return Color(1, 0, 1, 1);

	size_t x = clamp<float>(u, 0, 0.999999f)*tex.mipmap[level].width;
	size_t y = clamp<float>(v, 0, 0.999999f)*tex.mipmap[level].height;
	size_t idx = 4 * (y * tex.mipmap[level].width + x);
	float r = tex.mipmap[level].texels[idx + 0] / 255.0f;
	float g = tex.mipmap[level].texels[idx + 1] / 255.0f;
	float b = tex.mipmap[level].texels[idx + 2] / 255.0f;
	float a = tex.mipmap[level].texels[idx + 3] / 255.0f;
	return Color(r, g, b, a);
}

Color Sampler2DImp::sample_bilinear(Texture& tex, float u, float v, int level) {

	// Task 6: Implement bilinear filtering

	// return magenta for invalid level
	if (level >= tex.mipmap.size())
		return Color(1, 0, 1, 1);

	float xf = clamp<float>(u, 0, 0.999999f)*tex.mipmap[level].width;
	float yf = clamp<float>(v, 0, 0.999999f)*tex.mipmap[level].height;

	int x0 = static_cast<int>(xf);
	int x1 = clamp<int>(x0 + ((xf-x0) < 0.5 ? -1 : 1), 0, tex.mipmap[level].width - 1);
	int y0 = static_cast<int>(yf);
	int y1 = clamp<int>(y0 + ((yf-y0) < 0.5 ? -1 : 1), 0, tex.mipmap[level].height - 1);
	int idx[4] = {
		4 * (y0 * tex.mipmap[level].width + x0),
		4 * (y0 * tex.mipmap[level].width + x1),
		4 * (y1 * tex.mipmap[level].width + x0),
		4 * (y1 * tex.mipmap[level].width + x1)
	};

	Color colors[4];
	for (int i = 0; i < 4; i++) {
		colors[i].a = tex.mipmap[level].texels[idx[i] + 3] / 255.0f;
		colors[i].r = tex.mipmap[level].texels[idx[i] + 0] / 255.0f * colors[i].a;
		colors[i].g = tex.mipmap[level].texels[idx[i] + 1] / 255.0f * colors[i].a;
		colors[i].b = tex.mipmap[level].texels[idx[i] + 2] / 255.0f * colors[i].a;
	}
	
	float tx = abs(xf - (x0+0.5f));
	float ty = abs(yf - (y0+0.5f));
	Color mixColor = (1-ty)*((1-tx)*colors[0] + tx*colors[1]) + ty*((1-tx)*colors[2] + tx*colors[3]);
	if (mixColor.a != 0) {
		mixColor.r *= 1.0f / mixColor.a;
		mixColor.g *= 1.0f / mixColor.a;
		mixColor.b *= 1.0f / mixColor.a;
	}
	return mixColor;
}

Color Sampler2DImp::sample_trilinear(Texture& tex, float u, float v, float u_scale, float v_scale) {

	// Task 7: Implement trilinear filtering
	
	float f_level = max(log2f(max(tex.width/u_scale, tex.height/v_scale)), 0.0f);
	int i_level = floor(f_level);
	
	if (i_level + 1 >= tex.mipmap.size())
		return sample_bilinear(tex, u, v, tex.mipmap.size() - 1);

	Color c0 = sample_bilinear(tex, u, v, i_level);
	c0.r *= c0.a;
	c0.g *= c0.a;
	c0.b *= c0.a;
	Color c1 = sample_bilinear(tex, u, v, i_level + 1);
	c1.r *= c1.a;
	c1.g *= c1.a;
	c1.b *= c1.a;

	float t = f_level - i_level;
	Color mixColor = (1 - t)*c0 + t * c1;
	if (mixColor.a != 0) {
		mixColor.r *= 1.0f / mixColor.a;
		mixColor.g *= 1.0f / mixColor.a;
		mixColor.b *= 1.0f / mixColor.a;
	}
	return mixColor;
}

} // namespace CMU462
