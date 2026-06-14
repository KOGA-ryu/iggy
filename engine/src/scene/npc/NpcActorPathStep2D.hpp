#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorPathReport2D.hpp"
#include "scene/npc/NpcMoveMode.hpp"
#include "servers/navigation/NavigationPathFollower.hpp"

namespace iggy {

enum class NpcActorPathStep2DStatus {
	Proposed,
	NoPath,
	AlreadyAtTarget,
	InvalidMoveMode,
	ZeroStep,
};

struct NpcActorPathStep2DConfig {
	float baseStepDistance = 1.0F;
	float arrivalTolerance = 0.001F;
};

struct NpcActorPathStep2D {
	NpcActorPathReport2D pathReport;
	navigation::NavigationPathFollowResult follower;
	NpcActorPathStep2DStatus status = NpcActorPathStep2DStatus::NoPath;
	ResourceId npcId;
	Vec2 oldPosition;
	Vec2 proposedPosition;
	TileCoord oldTile;
	TileCoord proposedTile;
	NpcMoveMode moveMode = NpcMoveMode::None;
	float requestedDistance = 0.0F;
	float maxDistance = 0.0F;
	bool requestsMovement = false;
	bool completedPath = false;

	[[nodiscard]] bool proposed() const;
	[[nodiscard]] bool ready() const;
};

class NpcActorPathStepper2D {
public:
	[[nodiscard]] NpcActorPathStep2D step(
		const NpcActorPathReport2D &pathReport,
		const NpcActorPathStep2DConfig &config = {}) const;
};

} // namespace iggy
