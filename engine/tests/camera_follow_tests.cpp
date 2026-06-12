#include <cstdlib>

#include "scene/camera/CameraFollow.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::CameraFollowResult Step(iggy::Vec2 cameraPosition, iggy::Vec2 targetPosition, float maxStep, float deadZoneRadius)
{
	return iggy::CameraFollow {}.step({ cameraPosition }, targetPosition, { maxStep, deadZoneRadius });
}

void TestTargetInsideDeadZoneDoesNotMove()
{
	const iggy::CameraFollowResult result = Step({ 1.0F, 1.0F }, { 2.0F, 1.0F }, 0.5F, 2.0F);

	Expect(!result.moved, "target inside dead zone should not move camera");
	Expect(NearVec(result.state.position, { 1.0F, 1.0F }), "inside dead zone should preserve camera position");
}

void TestTargetOnDeadZoneBoundaryDoesNotMove()
{
	const iggy::CameraFollowResult result = Step({ 1.0F, 1.0F }, { 3.0F, 1.0F }, 0.5F, 2.0F);

	Expect(!result.moved, "target on dead-zone boundary should not move camera");
	Expect(NearVec(result.state.position, { 1.0F, 1.0F }), "dead-zone boundary should preserve camera position");
}

void TestPartialStepTowardTarget()
{
	const iggy::CameraFollowResult result = Step({ 0.0F, 0.0F }, { 4.0F, 0.0F }, 1.25F, 0.5F);

	Expect(result.moved, "target outside dead zone with positive max step should move camera");
	Expect(NearVec(result.state.position, { 1.25F, 0.0F }), "partial step should move by maxStep toward target");
}

void TestLargeStepWithZeroDeadZoneReachesTarget()
{
	const iggy::CameraFollowResult result = Step({ 0.0F, 0.0F }, { 4.0F, 0.0F }, 10.0F, 0.0F);

	Expect(result.moved, "large step with zero dead zone should move camera");
	Expect(NearVec(result.state.position, { 4.0F, 0.0F }), "zero dead zone should allow large step to reach target exactly");
}

void TestLargeStepWithDeadZoneStopsAtBoundary()
{
	const iggy::CameraFollowResult result = Step({ 0.0F, 0.0F }, { 4.0F, 0.0F }, 10.0F, 1.0F);

	Expect(result.moved, "large step with dead zone should move camera");
	Expect(NearVec(result.state.position, { 3.0F, 0.0F }), "large step should stop when target is on dead-zone boundary");
}

void TestZeroMaxStepDoesNotMove()
{
	const iggy::CameraFollowResult result = Step({ 0.0F, 0.0F }, { 4.0F, 0.0F }, 0.0F, 0.0F);

	Expect(!result.moved, "zero maxStep should not move camera");
	Expect(NearVec(result.state.position, { 0.0F, 0.0F }), "zero maxStep should preserve camera position");
}

void TestNegativeMaxStepDoesNotMove()
{
	const iggy::CameraFollowResult result = Step({ 0.0F, 0.0F }, { 4.0F, 0.0F }, -1.0F, 0.0F);

	Expect(!result.moved, "negative maxStep should not move camera");
	Expect(NearVec(result.state.position, { 0.0F, 0.0F }), "negative maxStep should preserve camera position");
}

void TestNegativeDeadZoneBehavesLikeZero()
{
	const iggy::CameraFollowResult result = Step({ 0.0F, 0.0F }, { 4.0F, 0.0F }, 10.0F, -2.0F);

	Expect(result.moved, "negative dead zone should still allow movement");
	Expect(NearVec(result.state.position, { 4.0F, 0.0F }), "negative dead zone should behave like zero dead zone");
}

} // namespace

int main()
{
	TestTargetInsideDeadZoneDoesNotMove();
	TestTargetOnDeadZoneBoundaryDoesNotMove();
	TestPartialStepTowardTarget();
	TestLargeStepWithZeroDeadZoneReachesTarget();
	TestLargeStepWithDeadZoneStopsAtBoundary();
	TestZeroMaxStepDoesNotMove();
	TestNegativeMaxStepDoesNotMove();
	TestNegativeDeadZoneBehavesLikeZero();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
