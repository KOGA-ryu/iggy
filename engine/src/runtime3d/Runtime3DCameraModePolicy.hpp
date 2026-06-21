#pragma once

#include "runtime3d/Runtime3DClock.hpp"
#include "runtime3d/Runtime3DSessionState.hpp"

namespace iggy::runtime3d {

[[nodiscard]] Runtime3DCameraModeState ApplyRuntime3DCameraModePolicy(
	Runtime3DSessionLifecycle lifecycle,
	Runtime3DClockMode clockMode,
	Runtime3DCameraModeState state);

} // namespace iggy::runtime3d
