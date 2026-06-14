#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class NpcObjective2DType {
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

struct NpcObjective2D {
	NpcObjective2DType type = NpcObjective2DType::None;
	ResourceId targetId;
	Vec2 targetPosition;
};

enum class NpcObjective2DStatus {
	Valid,
	MissingTarget,
};

struct NpcObjective2DValidationResult {
	NpcObjective2DStatus status = NpcObjective2DStatus::Valid;
	NpcObjective2D objective;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] NpcObjective2D noneNpcObjective2D();
[[nodiscard]] NpcObjective2D waitNpcObjective2D();
[[nodiscard]] NpcObjective2D patrolNpcObjective2D(ResourceId targetId = {});
[[nodiscard]] NpcObjective2D guardNpcObjective2D(ResourceId targetId = {});
[[nodiscard]] NpcObjective2D investigateNpcObjective2D(Vec2 targetPosition);
[[nodiscard]] NpcObjective2D fleeNpcObjective2D(Vec2 targetPosition);
[[nodiscard]] NpcObjective2D followNpcObjective2D(ResourceId targetId);
[[nodiscard]] NpcObjective2D attackNpcObjective2D(ResourceId targetId);
[[nodiscard]] NpcObjective2D moveToNpcObjective2D(Vec2 targetPosition);
[[nodiscard]] NpcObjective2D interactNpcObjective2D(ResourceId targetId);

[[nodiscard]] NpcObjective2DValidationResult validate(const NpcObjective2D &objective);
[[nodiscard]] bool valid(const NpcObjective2D &objective);

} // namespace iggy
