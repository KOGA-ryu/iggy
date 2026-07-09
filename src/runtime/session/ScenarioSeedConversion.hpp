#pragma once

#include <optional>

#include "content/ScenarioSeed.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/objective/ObjectiveState.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

template <typename T>
struct Result;

Result<PlayerSlotKind> playerSlotKindFromScenario(ScenarioPlayerSlotKind kind);
Result<ClockMode> clockModeFromScenario(ScenarioClockMode mode);
Result<CameraMode> cameraModeFromScenario(ScenarioCameraMode mode);
Result<PatrolMode> patrolModeFromScenario(ScenarioPatrolMode mode);
Result<EntityState> entityFromScenario(const ScenarioEntitySeed& seed, EntityId id);
Result<std::optional<CombatantState>> combatantFromScenario(
    const ScenarioCombatantSeed* seed, EntityId id);
Result<ObjectiveRecord> objectiveFromScenario(const ScenarioObjectiveSeed& seed);

}  // namespace iggy3d
