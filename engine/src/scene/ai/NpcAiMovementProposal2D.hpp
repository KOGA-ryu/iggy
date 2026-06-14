#pragma once

#include <cstddef>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcAiPathReport2D.hpp"

namespace iggy {

enum class NpcAiMovementProposal2DStatus {
	Proposed,
	NoPath,
	HoldPosition,
	AlreadyAtTarget,
};

struct NpcAiMovementProposal2DConfig {
	std::size_t waypointLookahead = 1;
	float arrivalTolerance = 0.001F;
};

struct NpcAiMovementProposal2DResult {
	NpcAiMovementProposal2DStatus status = NpcAiMovementProposal2DStatus::NoPath;
	NpcAiPathReport2DResult path;
	ResourceId npcId;
	Vec2 startPosition;
	Vec2 finalTargetPosition;
	Vec2 proposedPosition;
	std::size_t selectedWaypointIndex = 0;
	NpcAiBehaviorIntent2DType intentType = NpcAiBehaviorIntent2DType::None;
	bool requestsMovement = false;

	[[nodiscard]] bool hasMovementProposal() const;
};

class NpcAiMovementProposalBuilder2D {
public:
	[[nodiscard]] NpcAiMovementProposal2DResult build(
		const NpcAiPathReport2DResult &path,
		const NpcAiMovementProposal2DConfig &config = {}) const;
};

} // namespace iggy
