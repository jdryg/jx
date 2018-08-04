#include <jx/math.h>

namespace jx
{
bool closestPointOnLineSegment(float x, float y, float sx, float sy, float ex, float ey, float thickness, float* t)
{
	const float esx = ex - sx;
	const float esy = ey - sy;
	const float psx = x - sx;
	const float psy = y - sy;

	const float segmentLengthSqr = esx * esx + esy * esy;
	if (segmentLengthSqr < 1e-5f) {
		// s == e
		if (t) {
			*t = 0.0f;
		}

		return (psx * psx + psy * psy) <= thickness * thickness;
	}

	const float dist = (psx * esx + psy * esy) / segmentLengthSqr;
	if (dist < 0.0f) {
		if (t) {
			*t = 0.0f;
		}

		return (psx * psx + psy * psy) <= thickness * thickness;
	} else if (dist > 1.0f) {
		const float pex = x - ex;
		const float pey = y - ey;
		if (t) {
			*t = 1.0f;
		}

		return (pex * pex + pey * pey) <= thickness * thickness;
	}

	if (t) {
		*t = dist;
	}

	const float pprojx = psx - esx * dist;
	const float pprojy = psy - esy * dist;
	return (pprojx * pprojx + pprojy * pprojy) <= thickness * thickness;
}
}
