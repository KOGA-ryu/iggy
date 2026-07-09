#pragma once

#include "content/ScenarioSeed.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

template <typename T>
struct Result;

Result<PlayerSlotKind> playerSlotKindFromScenario(ScenarioPlayerSlotKind kind);
Result<ClockMode> clockModeFromScenario(ScenarioClockMode mode);
Result<CameraMode> cameraModeFromScenario(ScenarioCameraMode mode);
Result<PatrolMode> patrolModeFromScenario(ScenarioPatrolMode mode);

}  // namespace iggy3d
