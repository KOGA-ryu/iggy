#pragma once

#include <vector>

#include "runtime3d/Runtime3DCameraState.hpp"
#include "runtime3d/Runtime3DClock.hpp"
#include "runtime3d/Runtime3DCommand.hpp"
#include "runtime3d/Runtime3DWorldState.hpp"

namespace iggy::runtime3d {

enum class Runtime3DSessionLifecycle {
	NotLoaded,
	Loading,
	PlayingRealtime,
	PlanningTactical,
	Paused,
	Completed,
	Failed,
	IncompatibleSave,
};

struct Runtime3DSessionState {
	Runtime3DSessionLifecycle lifecycle = Runtime3DSessionLifecycle::NotLoaded;
	Runtime3DClockState clock;
	Runtime3DCameraModeState camera;
	Runtime3DWorldState world;
	std::vector<Runtime3DCommandRecord> commandLog;
};

} // namespace iggy::runtime3d
