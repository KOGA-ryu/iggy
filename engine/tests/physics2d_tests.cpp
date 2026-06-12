#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "servers/physics2d/ShapeQuery2D.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

bool NearVec(iggy::Vec2 actual, iggy::Vec2 expected)
{
	return Near(actual.x, expected.x) && Near(actual.y, expected.y);
}

void TestPointAndRectQueries()
{
	const iggy::Rect2 rect { { 1.0F, 2.0F }, { 3.0F, 4.0F } };

	Expect(iggy::physics2d::ContainsPoint(rect, { 2.0F, 3.0F }), "physics2d point query should hit inside rect");
	Expect(!iggy::physics2d::ContainsPoint(rect, { 4.1F, 3.0F }), "physics2d point query should miss outside rect");
	Expect(iggy::physics2d::Overlaps(rect, { { 3.0F, 5.0F }, { 2.0F, 2.0F } }), "physics2d rect query should overlap intersecting rects");
	Expect(!iggy::physics2d::Overlaps(rect, { { 5.0F, 2.0F }, { 1.0F, 1.0F } }), "physics2d rect query should miss separated rects");
}

void TestRaycastHitsAabb()
{
	const iggy::Ray2 ray { { 0.0F, 2.0F }, { 1.0F, 0.0F } };
	const iggy::Aabb2 bounds { { 3.0F, 1.0F }, { 5.0F, 4.0F } };
	const iggy::physics2d::RaycastHit2D hit = iggy::physics2d::RaycastAabb(ray, bounds);

	Expect(hit.hit, "raycast should hit AABB in front of ray");
	Expect(Near(hit.distance, 3.0F), "raycast should report entry distance");
	Expect(NearVec(hit.point, { 3.0F, 2.0F }), "raycast should report entry point");
	Expect(hit.normal == iggy::Vec2 { -1.0F, 0.0F }, "raycast should report entry normal");
}

void TestRaycastMissesAabb()
{
	const iggy::Ray2 ray { { 0.0F, 0.0F }, { 1.0F, 0.0F } };
	const iggy::Aabb2 bounds { { 3.0F, 1.0F }, { 5.0F, 4.0F } };

	Expect(!iggy::physics2d::RaycastAabb(ray, bounds).hit, "raycast should miss AABB outside ray path");
	Expect(!iggy::physics2d::RaycastAabb({ { 0.0F, 2.0F }, { 1.0F, 0.0F } }, bounds, 2.0F).hit, "raycast should respect max distance");
}

void TestRaycastFromInsideAabb()
{
	const iggy::Ray2 ray { { 4.0F, 2.0F }, { 1.0F, 0.0F } };
	const iggy::Aabb2 bounds { { 3.0F, 1.0F }, { 5.0F, 4.0F } };
	const iggy::physics2d::RaycastHit2D hit = iggy::physics2d::RaycastAabb(ray, bounds);

	Expect(hit.hit, "raycast from inside AABB should hit immediately");
	Expect(Near(hit.distance, 0.0F), "raycast from inside should report zero distance");
	Expect(NearVec(hit.point, ray.origin), "raycast from inside should report origin as point");
	Expect(hit.normal == iggy::Vec2 {}, "raycast from inside should have no entry normal");
}

} // namespace

int main()
{
	TestPointAndRectQueries();
	TestRaycastHitsAabb();
	TestRaycastMissesAabb();
	TestRaycastFromInsideAabb();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
