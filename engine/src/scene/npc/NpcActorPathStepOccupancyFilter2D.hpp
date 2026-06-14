#pragma once

#include "core/resource/ResourceId.hpp"
#include "scene/npc/NpcActorOccupancyQuery2D.hpp"
#include "scene/npc/NpcActorPathStep2D.hpp"

namespace iggy {

enum class NpcActorPathStepOccupancyFilter2DStatus {
	Allowed,
	NoStepProposal,
	BlockedByNpc,
};

struct NpcActorPathStepOccupancyFilter2DConfig {
};

struct NpcActorPathStepOccupancyFilter2D {
	NpcActorPathStep2D step;
	NpcActorOccupancyBlock2DResult occupancy;
	NpcActorPathStepOccupancyFilter2DStatus status = NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal;
	bool requestsMovement = false;
	ResourceId blockingNpcId;

	[[nodiscard]] bool allowed() const;
};

class NpcActorPathStepOccupancyFilterProjector2D {
public:
	[[nodiscard]] NpcActorPathStepOccupancyFilter2D filter(
		const NpcActorPathStep2D &step,
		const NpcActorOccupancy2D &occupancy,
		const NpcActorPathStepOccupancyFilter2DConfig &config = {}) const;
};

} // namespace iggy
