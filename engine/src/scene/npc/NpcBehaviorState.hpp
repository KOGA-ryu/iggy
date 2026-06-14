#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class NpcBehaviorStateType {
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

struct NpcBehaviorState {
	NpcBehaviorStateType type = NpcBehaviorStateType::None;
	ResourceId targetId;
	Vec2 targetPosition;
};

enum class NpcBehaviorStateStatus {
	Valid,
	MissingTarget,
};

struct NpcBehaviorStateValidationResult {
	NpcBehaviorStateStatus status = NpcBehaviorStateStatus::Valid;
	NpcBehaviorState state;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] NpcBehaviorState noneNpcBehaviorState();
[[nodiscard]] NpcBehaviorState idleNpcBehaviorState();
[[nodiscard]] NpcBehaviorState waitingNpcBehaviorState();
[[nodiscard]] NpcBehaviorState seekingNpcBehaviorState(Vec2 targetPosition);
[[nodiscard]] NpcBehaviorState fleeingNpcBehaviorState(Vec2 targetPosition);
[[nodiscard]] NpcBehaviorState attackingNpcBehaviorState(ResourceId targetId);
[[nodiscard]] NpcBehaviorState interactingNpcBehaviorState(ResourceId targetId);
[[nodiscard]] NpcBehaviorState stunnedNpcBehaviorState();
[[nodiscard]] NpcBehaviorState disabledNpcBehaviorState();

[[nodiscard]] NpcBehaviorStateValidationResult validate(const NpcBehaviorState &state);
[[nodiscard]] bool valid(const NpcBehaviorState &state);

} // namespace iggy
