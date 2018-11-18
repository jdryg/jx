#ifndef JX_COLOR_H
#error "Must be included from jx/color.h"
#endif

#include <bx/math.h>

namespace jx
{
inline uint32_t hueToABGR(float hue)
{
	const float d = 1.0f / 6.0f;
	const float t = hue / d;
	if (hue <= d) {
		float fgreen = t;
		return 0xff0000ff | (((uint32_t)(fgreen * 255.0f) & 0x000000FF) << 8);
	} else if (hue > d && hue <= 2.0f * d) {
		float fred = 2.0f - t;
		return 0xff00ff00 | (((uint32_t)(fred * 255.0f) & 0x000000FF) << 0);
	} else if (hue > 2.0f * d && hue <= 3.0f * d) {
		float fblue = t - 2.0f;
		return 0xff00ff00 | (((uint32_t)(fblue * 255.0f) & 0x000000FF) << 16);
	} else if (hue > 3.0f * d && hue <= 4.0f * d) {
		float fgreen = 4.0f - t;
		return 0xffff0000 | (((uint32_t)(fgreen * 255.0f) & 0x000000FF) << 8);
	} else if (hue > 4.0f * d && hue <= 5.0f * d) {
		float fred = t - 4.0f;
		return 0xffff0000 | (((uint32_t)(fred * 255.0f) & 0x000000FF) << 0);
	}

	float fblue = 6.0f - t;
	return 0xff0000ff | (((uint32_t)(fblue * 255.0f) & 0x000000FF) << 16);
}

inline float hueFromABGR(uint32_t abgr)
{
	const uint32_t r = (abgr & 0x000000FF) >> 0;
	const uint32_t g = (abgr & 0x0000FF00) >> 8;
	const uint32_t b = (abgr & 0x00FF0000) >> 16;

	const float fr = (float)r / 255.0f;
	const float fg = (float)g / 255.0f;
	const float fb = (float)b / 255.0f;

	const float cmax = bx::max(fr, fg, fb);
	const float cmin = bx::min(fr, fg, fb);
	const float d = cmax - cmin;

	if (d < 1e-4f) {
		return 0.0f;
	} else if (cmax == fr) {
		return bx::mod((fg - fb) / d, 6.0f) / 6.0f;
	} else if (cmax == fg) {
		return (((fb - fr) / d) + 2.0f) / 6.0f;
	}

	// cmax == fb
	return (((fr - fg) / d) + 4.0f) / 6.0f;
}

inline void HSBFromABGR(uint32_t abgr, float* hue, float* sat, float* brightness)
{
	const uint32_t r = (abgr & 0x000000FF) >> 0;
	const uint32_t g = (abgr & 0x0000FF00) >> 8;
	const uint32_t b = (abgr & 0x00FF0000) >> 16;

	const float fr = (float)r / 255.0f;
	const float fg = (float)g / 255.0f;
	const float fb = (float)b / 255.0f;

	const float cmax = bx::max(fr, fg, fb);
	const float cmin = bx::min(fr, fg, fb);
	const float d = cmax - cmin;

	if (d < 1e-4f) {
		*hue = 0.0f;
	} else if (cmax == fr) {
		*hue = bx::mod((fg - fb) / d, 6.0f) / 6.0f;
	} else if (cmax == fg) {
		*hue = (((fb - fr) / d) + 2.0f) / 6.0f;
	} else {
		// cmax == fb
		*hue = (((fr - fg) / d) + 4.0f) / 6.0f;
	}

	if (cmax > 1e-4f) {
		*sat = d / cmax;
	} else {
		*sat = 0.0f;
	}

	*brightness = cmax;
}

inline uint32_t HSBToABGR(float hue, float sat, float brightness)
{
	const float d = 1.0f / 6.0f;

	const float C = brightness * sat;
	const float X = C * (1.0f - bx::abs(bx::mod((hue * 6.0f), 2.0f) - 1.0f));
	const float m = brightness - C;

	float fred = 0.0f;
	float fgreen = 0.0f;
	float fblue = 0.0f;
	if (hue <= d) {
		fred = C;
		fgreen = X;
	} else if (hue > d && hue <= 2.0f * d) {
		fred = X;
		fgreen = C;
	} else if (hue > 2.0f * d && hue <= 3.0f * d) {
		fgreen = C;
		fblue = X;
	} else if (hue > 3.0f * d && hue <= 4.0f * d) {
		fgreen = X;
		fblue = C;
	} else if (hue > 4.0f * d && hue <= 5.0f * d) {
		fred = X;
		fblue = C;
	} else {
		fred = C;
		fblue = X;
	}

	const uint32_t r = (uint32_t)bx::floor((fred + m) * 255.0f);
	const uint32_t g = (uint32_t)bx::floor((fgreen + m) * 255.0f);
	const uint32_t b = (uint32_t)bx::floor((fblue + m) * 255.0f);

	return 0xFF000000 | (b << 16) | (g << 8) | (r);
}
}
