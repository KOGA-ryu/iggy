#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class NpcBehaviorState2DType {
	None,
	Idle,
	Waiting,
	Seeking,
	Fleeing,
	Attacking,
	Interacting,
	Stunned,
	Disabled,
};

struct NpcBehaviorState2D {
	NpcBehaviorState2DType type = NpcBehaviorState2DType::None;
	ResourceId targetId;
	Vec2 targetPosition;
};

enum class NpcBehaviorState2DStatus {
	Valid,
	MissingTarget,
};

struct NpcBehaviorState2DValidationResult {
	NpcBehaviorState2DStatus status = NpcBehaviorState2DStatus::Valid;
	NpcBehaviorState2D state;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] NpcBehaviorState2D noneNpcBehaviorState2D();
[[nodiscard]] NpcBehaviorState2D idleNpcBehaviorState2D();
[[nodiscard]] NpcBehaviorState2D waitingNpcBehaviorState2D();
[[nodiscard]] NpcBehaviorState2D seekingNpcBehaviorState2D(Vec2 targetPosition);
[[nodiscard]] NpcBehaviorState2D fleeingNpcBehaviorState2D(Vec2 targetPosition);
[[nodiscard]] NpcBehaviorState2D attackingNpcBehaviorState2D(ResourceId targetId);
[[nodiscard]] NpcBehaviorState2D interactingNpcBehaviorState2D(ResourceId targetId);
[[nodiscard]] NpcBehaviorState2D stunnedNpcBehaviorState2D();
[[nodiscard]] NpcBehaviorState2D disabledNpcBehaviorState2D();

[[nodiscard]] NpcBehaviorState2DValidationResult validate(const NpcBehaviorState2D &state);
[[nodiscard]] bool valid(const NpcBehaviorState2D &state);

} // namespace iggy
