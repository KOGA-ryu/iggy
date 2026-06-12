#include <cstdlib>

#include "scene/camera/CameraClamp.hpp"
#include "scene/camera/CameraFollow.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::Aabb2 Bounds(iggy::Vec2 min, iggy::Vec2 max)
{
	return { min, max };
}

iggy::CameraClampResult Clamp(iggy::Vec2 cameraPosition, iggy::Aabb2 bounds)
{
	return iggy::CameraClamp {}.clamp({ cameraPosition }, bounds);
}

void TestInsideBoundsUnchanged()
{
	const iggy::CameraClampResult result = Clamp({ 2.0F, 3.0F }, Bounds({ 0.0F, 0.0F }, { 5.0F, 5.0F }));

	Expect(!result.clamped, "camera inside bounds should not clamp");
	Expect(NearVec(result.state.position, { 2.0F, 3.0F }), "inside bounds should preserve position");
}

void TestXBelowMinClampsToMin()
{
	const iggy::CameraClampResult result = Clamp({ -1.0F, 3.0F }, Bounds({ 0.0F, 0.0F }, { 5.0F, 5.0F }));

	Expect(result.clamped, "x below min should clamp");
	Expect(NearVec(result.state.position, { 0.0F, 3.0F }), "x below min should clamp to min.x");
}

void TestXAboveMaxClampsToMax()
{
	const iggy::CameraClampResult result = Clamp({ 8.0F, 3.0F }, Bounds({ 0.0F, 0.0F }, { 5.0F, 5.0F }));

	Expect(result.clamped, "x above max should clamp");
	Expect(NearVec(result.state.position, { 5.0F, 3.0F }), "x above max should clamp to max.x");
}

void TestYBelowMinClampsToMin()
{
	const iggy::CameraClampResult result = Clamp({ 2.0F, -3.0F }, Bounds({ 0.0F, 0.0F }, { 5.0F, 5.0F }));

	Expect(result.clamped, "y below min should clamp");
	Expect(NearVec(result.state.position, { 2.0F, 0.0F }), "y below min should clamp to min.y");
}

void TestYAboveMaxClampsToMax()
{
	const iggy::CameraClampResult result = Clamp({ 2.0F, 9.0F }, Bounds({ 0.0F, 0.0F }, { 5.0F, 5.0F }));

	Expect(result.clamped, "y above max should clamp");
	Expect(NearVec(result.state.position, { 2.0F, 5.0F }), "y above max should clamp to max.y");
}

void TestBothAxesClamp()
{
	const iggy::CameraClampResult result = Clamp({ -2.0F, 9.0F }, Bounds({ 0.0F, 0.0F }, { 5.0F, 5.0F }));

	Expect(result.clamped, "both axes outside bounds should clamp");
	Expect(NearVec(result.state.position, { 0.0F, 5.0F }), "both axes should clamp independently");
}

void TestDegenerateBoundsClampToPoint()
{
	const iggy::CameraClampResult result = Clamp({ 4.0F, -2.0F }, Bounds({ 2.0F, 3.0F }, { 2.0F, 3.0F }));

	Expect(result.clamped, "degenerate point bounds should clamp");
	Expect(NearVec(result.state.position, { 2.0F, 3.0F }), "degenerate point bounds should clamp to single point");
}

void TestDegenerateBoundsClampToLine()
{
	const iggy::CameraClampResult result = Clamp({ 4.0F, -2.0F }, Bounds({ 2.0F, 0.0F }, { 2.0F, 5.0F }));

	Expect(result.clamped, "degenerate line bounds should clamp");
	Expect(NearVec(result.state.position, { 2.0F, 0.0F }), "degenerate line bounds should clamp fixed axis and ranged axis");
}

void TestInvertedBoundsNormalizeBeforeClamp()
{
	const iggy::CameraClampResult result = Clamp({ 6.0F, -2.0F }, Bounds({ 5.0F, 5.0F }, { 0.0F, 0.0F }));

	Expect(result.clamped, "inverted bounds should normalize and clamp");
	Expect(NearVec(result.state.position, { 5.0F, 0.0F }), "inverted bounds should behave like ordered bounds");
}

void TestFollowThenClampComposition()
{
	const iggy::CameraFollowResult followed = iggy::CameraFollow {}.step({ { 0.0F, 0.0F } }, { 10.0F, 0.0F }, { 8.0F, 0.0F });
	const iggy::CameraClampResult clamped = iggy::CameraClamp {}.clamp(followed.state, Bounds({ 0.0F, -1.0F }, { 5.0F, 1.0F }));

	Expect(followed.moved, "composition setup should move during follow step");
	Expect(NearVec(followed.state.position, { 8.0F, 0.0F }), "follow step should run independently before clamp");
	Expect(clamped.clamped, "composition clamp should clamp followed position");
	Expect(NearVec(clamped.state.position, { 5.0F, 0.0F }), "manual follow then clamp should produce clamped camera state");
}

} // namespace

int main()
{
	TestInsideBoundsUnchanged();
	TestXBelowMinClampsToMin();
	TestXAboveMaxClampsToMax();
	TestYBelowMinClampsToMin();
	TestYAboveMaxClampsToMax();
	TestBothAxesClamp();
	TestDegenerateBoundsClampToPoint();
	TestDegenerateBoundsClampToLine();
	TestInvertedBoundsNormalizeBeforeClamp();
	TestFollowThenClampComposition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
