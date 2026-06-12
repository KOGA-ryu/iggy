#include <cstdlib>

#include "scene/camera/CameraRig.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::Aabb2 Bounds(iggy::Vec2 min, iggy::Vec2 max)
{
	return { min, max };
}

iggy::CameraRigConfig Config(float maxStep, float deadZoneRadius)
{
	iggy::CameraRigConfig config;
	config.follow = { maxStep, deadZoneRadius };
	return config;
}

iggy::CameraRigConfig BoundedConfig(float maxStep, float deadZoneRadius, iggy::Aabb2 bounds)
{
	iggy::CameraRigConfig config = Config(maxStep, deadZoneRadius);
	config.clampToBounds = true;
	config.bounds = bounds;
	return config;
}

iggy::CameraRigResult Update(iggy::Vec2 cameraPosition, iggy::Vec2 targetPosition, const iggy::CameraRigConfig &config)
{
	return iggy::CameraRig {}.update({ cameraPosition }, targetPosition, config);
}

void TestNoBoundsMatchesFollowMovement()
{
	const iggy::CameraRigResult result = Update({ 0.0F, 0.0F }, { 4.0F, 0.0F }, Config(1.5F, 0.5F));

	Expect(result.followed, "unbounded rig should report follow movement");
	Expect(!result.clamped, "unbounded rig should not report clamp");
	Expect(NearVec(result.state.position, { 1.5F, 0.0F }), "unbounded rig should return follow result position");
}

void TestNoBoundsInsideDeadZoneDoesNotFollowOrClamp()
{
	const iggy::CameraRigResult result = Update({ 0.0F, 0.0F }, { 0.5F, 0.0F }, Config(1.5F, 1.0F));

	Expect(!result.followed, "target inside dead zone should not follow");
	Expect(!result.clamped, "unbounded rig should not clamp");
	Expect(NearVec(result.state.position, { 0.0F, 0.0F }), "inside dead zone should preserve camera position");
}

void TestBoundsClampPostFollowState()
{
	const iggy::CameraRigResult result = Update({ 0.0F, 0.0F }, { 10.0F, 0.0F }, BoundedConfig(8.0F, 0.0F, Bounds({ 0.0F, -1.0F }, { 5.0F, 1.0F })));

	Expect(result.followed, "bounded rig should report follow movement before clamp");
	Expect(result.clamped, "bounded rig should report clamp when post-follow state is outside bounds");
	Expect(NearVec(result.state.position, { 5.0F, 0.0F }), "bounded rig should clamp post-follow state");
}

void TestFollowInsideBoundsReportsNoClamp()
{
	const iggy::CameraRigResult result = Update({ 0.0F, 0.0F }, { 10.0F, 0.0F }, BoundedConfig(3.0F, 0.0F, Bounds({ 0.0F, -1.0F }, { 5.0F, 1.0F })));

	Expect(result.followed, "follow inside bounds should report follow movement");
	Expect(!result.clamped, "follow inside bounds should not report clamp");
	Expect(NearVec(result.state.position, { 3.0F, 0.0F }), "follow inside bounds should preserve followed position");
}

void TestStartingOutsideBoundsClampsWhenFollowDoesNotMove()
{
	const iggy::CameraRigResult result = Update({ 8.0F, 0.0F }, { 8.5F, 0.0F }, BoundedConfig(1.0F, 1.0F, Bounds({ 0.0F, -1.0F }, { 5.0F, 1.0F })));

	Expect(!result.followed, "dead zone should prevent follow movement");
	Expect(result.clamped, "bounds should clamp starting state even when follow does not move");
	Expect(NearVec(result.state.position, { 5.0F, 0.0F }), "starting outside bounds should clamp to bounds");
}

void TestStartingOutsideBoundsClampsWhenMaxStepIsZero()
{
	const iggy::CameraRigResult result = Update({ -3.0F, 0.0F }, { 10.0F, 0.0F }, BoundedConfig(0.0F, 0.0F, Bounds({ 0.0F, -1.0F }, { 5.0F, 1.0F })));

	Expect(!result.followed, "zero maxStep should prevent follow movement");
	Expect(result.clamped, "bounds should clamp when maxStep prevents follow");
	Expect(NearVec(result.state.position, { 0.0F, 0.0F }), "zero-step rig should still clamp starting state");
}

void TestFollowedAndClampedFlagsCanBothBeTrue()
{
	const iggy::CameraRigResult result = Update({ 2.0F, 0.0F }, { 10.0F, 0.0F }, BoundedConfig(5.0F, 0.0F, Bounds({ 0.0F, -1.0F }, { 4.0F, 1.0F })));

	Expect(result.followed, "rig should report follow movement");
	Expect(result.clamped, "rig should report clamp after follow movement");
	Expect(NearVec(result.state.position, { 4.0F, 0.0F }), "rig should return clamped post-follow state");
}

} // namespace

int main()
{
	TestNoBoundsMatchesFollowMovement();
	TestNoBoundsInsideDeadZoneDoesNotFollowOrClamp();
	TestBoundsClampPostFollowState();
	TestFollowInsideBoundsReportsNoClamp();
	TestStartingOutsideBoundsClampsWhenFollowDoesNotMove();
	TestStartingOutsideBoundsClampsWhenMaxStepIsZero();
	TestFollowedAndClampedFlagsCanBothBeTrue();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
