#include "servers/physics2d/ShapeQuery2D.hpp"

#include <algorithm>
#include <limits>

namespace iggy::physics2d {

namespace {

struct AxisRange {
	float entry = -std::numeric_limits<float>::infinity();
	float exit = std::numeric_limits<float>::infinity();
	Vec2 entryNormal;
};

bool ComputeAxisRange(float origin, float direction, float min, float max, Vec2 negativeNormal, Vec2 positiveNormal, AxisRange &range)
{
	if (direction == 0.0F)
		return origin >= min && origin <= max;

	float entry = (min - origin) / direction;
	float exit = (max - origin) / direction;
	Vec2 entryNormal = negativeNormal;

	if (entry > exit) {
		std::swap(entry, exit);
		entryNormal = positiveNormal;
	}

	if (entry > range.entry) {
		range.entry = entry;
		range.entryNormal = entryNormal;
	}
	range.exit = std::min(range.exit, exit);
	return range.entry <= range.exit;
}

} // namespace

bool ContainsPoint(Rect2 rect, Vec2 point)
{
	return rect.contains(point);
}

bool Overlaps(Rect2 left, Rect2 right)
{
	return left.overlaps(right);
}

RaycastHit2D RaycastAabb(Ray2 ray, Aabb2 bounds, float maxDistance)
{
	AxisRange range;

	if (!ComputeAxisRange(ray.origin.x, ray.direction.x, bounds.min.x, bounds.max.x, { -1.0F, 0.0F }, { 1.0F, 0.0F }, range))
		return {};
	if (!ComputeAxisRange(ray.origin.y, ray.direction.y, bounds.min.y, bounds.max.y, { 0.0F, -1.0F }, { 0.0F, 1.0F }, range))
		return {};
	if (range.exit < 0.0F || range.entry > maxDistance)
		return {};

	const float distance = std::max(range.entry, 0.0F);
	const Vec2 normal = range.entry < 0.0F ? Vec2 {} : range.entryNormal;
	return { true, distance, ray.pointAtDistance(distance), normal };
}

} // namespace iggy::physics2d
