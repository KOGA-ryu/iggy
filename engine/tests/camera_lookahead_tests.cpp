#include <cstdlib>

#include "scene/camera/CameraLookahead.hpp"
#include "scene/camera/CameraRig.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::CameraLookaheadResult Apply(iggy::Vec2 targetPosition, iggy::Vec2 targetVelocity, float distance, float minSpeed)
{
	return iggy::CameraLookahead {}.apply(targetPosition, targetVelocity, { distance, minSpeed });
}

void TestZeroVelocityDoesNotApply()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 0.0F, 0.0F }, 4.0F, 0.0F);

	Expect(!result.applied, "zero velocity should not apply lookahead");
	Expect(NearVec(result.target, { 2.0F, 3.0F }), "zero velocity should return original target");
}

void TestVelocityBelowMinSpeedDoesNotApply()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 1.0F, 0.0F }, 4.0F, 2.0F);

	Expect(!result.applied, "velocity below minSpeed should not apply lookahead");
	Expect(NearVec(result.target, { 2.0F, 3.0F }), "velocity below minSpeed should return original target");
}

void TestVelocityEqualToMinSpeedApplies()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 2.0F, 0.0F }, 4.0F, 2.0F);

	Expect(result.applied, "velocity equal to minSpeed should apply when speed is positive");
	Expect(NearVec(result.target, { 6.0F, 3.0F }), "velocity equal to minSpeed should offset target");
}

void TestHorizontalVelocityAppliesDistance()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { -5.0F, 0.0F }, 2.5F, 0.0F);

	Expect(result.applied, "horizontal velocity should apply lookahead");
	Expect(NearVec(result.target, { -0.5F, 3.0F }), "horizontal velocity should offset by configured distance");
}

void TestVerticalVelocityAppliesDistance()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 0.0F, 5.0F }, 2.5F, 0.0F);

	Expect(result.applied, "vertical velocity should apply lookahead");
	Expect(NearVec(result.target, { 2.0F, 5.5F }), "vertical velocity should offset by configured distance");
}

void TestDiagonalVelocityNormalizesOffset()
{
	const iggy::CameraLookaheadResult result = Apply({ 1.0F, 1.0F }, { 3.0F, 4.0F }, 10.0F, 0.0F);

	Expect(result.applied, "diagonal velocity should apply lookahead");
	Expect(NearVec(result.target, { 7.0F, 9.0F }), "diagonal velocity should normalize before applying distance");
}

void TestZeroDistanceDisablesLookahead()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 5.0F, 0.0F }, 0.0F, 0.0F);

	Expect(!result.applied, "zero distance should disable lookahead");
	Expect(NearVec(result.target, { 2.0F, 3.0F }), "zero distance should return original target");
}

void TestNegativeDistanceDisablesLookahead()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 5.0F, 0.0F }, -1.0F, 0.0F);

	Expect(!result.applied, "negative distance should disable lookahead");
	Expect(NearVec(result.target, { 2.0F, 3.0F }), "negative distance should return original target");
}

void TestNegativeMinSpeedBehavesLikeZero()
{
	const iggy::CameraLookaheadResult result = Apply({ 2.0F, 3.0F }, { 5.0F, 0.0F }, 4.0F, -3.0F);

	Expect(result.applied, "negative minSpeed should behave like zero minSpeed");
	Expect(NearVec(result.target, { 6.0F, 3.0F }), "negative minSpeed should allow lookahead for moving target");
}

void TestLookaheadTargetCanFeedCameraRig()
{
	const iggy::CameraLookaheadResult lookahead = Apply({ 2.0F, 0.0F }, { 1.0F, 0.0F }, 3.0F, 0.0F);
	iggy::CameraRigConfig rigConfig;
	rigConfig.follow = { 10.0F, 0.0F };
	const iggy::CameraRigResult rig = iggy::CameraRig {}.update({ { 0.0F, 0.0F } }, lookahead.target, rigConfig);

	Expect(lookahead.applied, "composition setup should apply lookahead");
	Expect(NearVec(lookahead.target, { 5.0F, 0.0F }), "composition setup should produce adjusted target");
	Expect(rig.followed, "rig should move toward lookahead target");
	Expect(NearVec(rig.state.position, { 5.0F, 0.0F }), "manual lookahead then rig should follow adjusted target");
}

} // namespace

int main()
{
	TestZeroVelocityDoesNotApply();
	TestVelocityBelowMinSpeedDoesNotApply();
	TestVelocityEqualToMinSpeedApplies();
	TestHorizontalVelocityAppliesDistance();
	TestVerticalVelocityAppliesDistance();
	TestDiagonalVelocityNormalizesOffset();
	TestZeroDistanceDisablesLookahead();
	TestNegativeDistanceDisablesLookahead();
	TestNegativeMinSpeedBehavesLikeZero();
	TestLookaheadTargetCanFeedCameraRig();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
