#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/npc/NpcActorFrameState2D.hpp"
#include "scene/npc/NpcMoveMode.hpp"

namespace iggy {

enum class NpcActorMovementIntent2DType {
	None,
	MoveTo,
	MoveAwayFrom,
};

enum class NpcActorMovementIntent2DStatus {
	Ready,
	NoMovement,
	MissingControl,
	ActorNotPresent,
	UnsupportedBehavior,
	InvalidMoveMode,
};

struct NpcActorMovementIntent2DConfig {
	bool allowAttackingApproach = false;
	bool allowInteractingApproach = false;
};

struct NpcActorMovementIntent2D {
	NpcActorFrameState2D frame;
	NpcActorMovementIntent2DStatus status = NpcActorMovementIntent2DStatus::NoMovement;
	NpcActorMovementIntent2DType type = NpcActorMovementIntent2DType::None;
	ResourceId npcId;
	Vec2 startPosition;
	Vec2 targetPosition;
	NpcMoveMode moveMode = NpcMoveMode::None;
	float speedMultiplier = 0.0F;
	bool requestsMovement = false;

	[[nodiscard]] bool ready() const;
};

class NpcActorMovementIntentProjector2D {
public:
	[[nodiscard]] NpcActorMovementIntent2D project(
		const NpcActorFrameState2D &frame,
		const NpcActorMovementIntent2DConfig &config = {}) const;
};

} // namespace iggy
