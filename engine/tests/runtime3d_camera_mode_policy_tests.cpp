#include <cstdlib>

#include "runtime3d/Runtime3DCameraModePolicy.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestNormalRestoresPreviousRealtimeCamera()
{
	iggy::runtime3d::Runtime3DCameraModeState state;
	state.activeMode = iggy::runtime3d::Runtime3DCameraMode::RealtimeFirstPerson;
	state.previousRealtimeMode = iggy::runtime3d::Runtime3DCameraMode::RealtimeFirstPerson;

	const iggy::runtime3d::Runtime3DCameraModeState tactical = iggy::runtime3d::ApplyRuntime3DCameraModePolicy(
		iggy::runtime3d::Runtime3DSessionLifecycle::PlayingRealtime,
		iggy::runtime3d::Runtime3DClockMode::Slow,
		state);

	Expect(tactical.activeMode == iggy::runtime3d::Runtime3DCameraMode::TacticalOrbit, "slow clock should switch to tactical camera");
	Expect(tactical.previousRealtimeMode == iggy::runtime3d::Runtime3DCameraMode::RealtimeFirstPerson, "slow clock should remember realtime camera");

	const iggy::runtime3d::Runtime3DCameraModeState realtime = iggy::runtime3d::ApplyRuntime3DCameraModePolicy(
		iggy::runtime3d::Runtime3DSessionLifecycle::PlayingRealtime,
		iggy::runtime3d::Runtime3DClockMode::Normal,
		tactical);

	Expect(realtime.activeMode == iggy::runtime3d::Runtime3DCameraMode::RealtimeFirstPerson, "normal clock should restore previous realtime camera");
}

void TestPlanningAndPausedUseTacticalCamera()
{
	iggy::runtime3d::Runtime3DCameraModeState state;
	state.activeMode = iggy::runtime3d::Runtime3DCameraMode::RealtimeThirdPersonClose;

	const iggy::runtime3d::Runtime3DCameraModeState planning = iggy::runtime3d::ApplyRuntime3DCameraModePolicy(
		iggy::runtime3d::Runtime3DSessionLifecycle::PlanningTactical,
		iggy::runtime3d::Runtime3DClockMode::Normal,
		state);
	Expect(planning.activeMode == iggy::runtime3d::Runtime3DCameraMode::TacticalOrbit, "planning tactical lifecycle should switch to tactical camera");

	const iggy::runtime3d::Runtime3DCameraModeState paused = iggy::runtime3d::ApplyRuntime3DCameraModePolicy(
		iggy::runtime3d::Runtime3DSessionLifecycle::PlayingRealtime,
		iggy::runtime3d::Runtime3DClockMode::Paused,
		state);
	Expect(paused.activeMode == iggy::runtime3d::Runtime3DCameraMode::TacticalOrbit, "paused clock should switch to tactical camera");
}

} // namespace

int main()
{
	TestNormalRestoresPreviousRealtimeCamera();
	TestPlanningAndPausedUseTacticalCamera();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
