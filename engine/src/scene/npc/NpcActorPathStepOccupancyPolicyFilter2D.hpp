#pragma once

#include "scene/npc/NpcActorOccupancyPolicy2D.hpp"
#include "scene/npc/NpcActorPathStepOccupancyFilter2D.hpp"

namespace iggy {

enum class NpcActorPathStepOccupancyPolicyFilter2DStatus {
	Allowed,
	NoStepProposal,
	BlockedByNpc,
};

struct NpcActorPathStepOccupancyPolicyFilter2DConfig {
	NpcActorOccupancyPolicy2DConfig policy;
};

struct NpcActorPathStepOccupancyPolicyFilter2D {
	NpcActorPathStepOccupancyFilter2D filter;
	NpcActorOccupancyPolicy2DResult policy;
	NpcActorPathStepOccupancyPolicyFilter2DStatus status = NpcActorPathStepOccupancyPolicyFilter2DStatus::NoStepProposal;
	bool requestsMovement = false;
	ResourceId blockingNpcId;

	[[nodiscard]] bool allowed() const;
};

class NpcActorPathStepOccupancyPolicyFilterProjector2D {
public:
	[[nodiscard]] NpcActorPathStepOccupancyPolicyFilter2D filter(
		const NpcActorPathStep2D &step,
		const NpcActorOccupancy2D &occupancy,
		const NpcActorPathStepOccupancyPolicyFilter2DConfig &config = {}) const;
};

} // namespace iggy
