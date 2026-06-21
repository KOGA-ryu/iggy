#include "runtime3d/Runtime3DCameraModePolicy.hpp"

namespace iggy::runtime3d {
namespace {

bool RequiresTacticalCamera(Runtime3DSessionLifecycle lifecycle, Runtime3DClockMode clockMode)
{
	return lifecycle == Runtime3DSessionLifecycle::PlanningTactical ||
		lifecycle == Runtime3DSessionLifecycle::Paused ||
		clockMode == Runtime3DClockMode::Slow ||
		clockMode == Runtime3DClockMode::Paused ||
		clockMode == Runtime3DClockMode::StepRequested;
}

} // namespace

Runtime3DCameraModeState ApplyRuntime3DCameraModePolicy(
	Runtime3DSessionLifecycle lifecycle,
	Runtime3DClockMode clockMode,
	Runtime3DCameraModeState state)
{
	if (IsRuntime3DRealtimeCameraMode(state.activeMode))
		state.previousRealtimeMode = state.activeMode;

	if (RequiresTacticalCamera(lifecycle, clockMode)) {
		if (!IsRuntime3DTacticalCameraMode(state.activeMode))
			state.activeMode = Runtime3DCameraMode::TacticalOrbit;
		return state;
	}

	if (lifecycle == Runtime3DSessionLifecycle::PlayingRealtime && clockMode == Runtime3DClockMode::Normal) {
		state.activeMode = IsRuntime3DRealtimeCameraMode(state.previousRealtimeMode)
			? state.previousRealtimeMode
			: Runtime3DCameraMode::RealtimeThirdPersonClose;
	}

	return state;
}

} // namespace iggy::runtime3d
