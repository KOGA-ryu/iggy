#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class NpcObjectiveType {
	None,
	Wait,
	Patrol,
	Guard,
	Investigate,
	Flee,
	Follow,
	Attack,
	MoveTo,
	Interact,
};

struct NpcObjective {
	NpcObjectiveType type = NpcObjectiveType::None;
	ResourceId targetId;
	Vec2 targetPosition;
};

enum class NpcObjectiveStatus {
	Valid,
	MissingTarget,
};

struct NpcObjectiveValidationResult {
	NpcObjectiveStatus status = NpcObjectiveStatus::Valid;
	NpcObjective objective;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] NpcObjective noneNpcObjective();
[[nodiscard]] NpcObjective waitNpcObjective();
[[nodiscard]] NpcObjective patrolNpcObjective(ResourceId targetId = {});
[[nodiscard]] NpcObjective guardNpcObjective(ResourceId targetId = {});
[[nodiscard]] NpcObjective investigateNpcObjective(Vec2 targetPosition);
[[nodiscard]] NpcObjective fleeNpcObjective(Vec2 targetPosition);
[[nodiscard]] NpcObjective followNpcObjective(ResourceId targetId);
[[nodiscard]] NpcObjective attackNpcObjective(ResourceId targetId);
[[nodiscard]] NpcObjective moveToNpcObjective(Vec2 targetPosition);
[[nodiscard]] NpcObjective interactNpcObjective(ResourceId targetId);

[[nodiscard]] NpcObjectiveValidationResult validate(const NpcObjective &objective);
[[nodiscard]] bool valid(const NpcObjective &objective);

} // namespace iggy
