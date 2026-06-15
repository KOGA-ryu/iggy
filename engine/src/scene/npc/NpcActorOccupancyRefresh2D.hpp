#pragma once

#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"

namespace iggy {

enum class NpcActorOccupancyRefresh2DStatus {
	Refreshed,
	NoRefreshNeeded,
};

struct NpcActorOccupancyRefresh2DConfig {
	NpcActorOccupancy2DConfig occupancy;
};

struct NpcActorOccupancyRefresh2DResult {
	NpcActorState2DRegistry registry;
	NpcActorOccupancy2D previousOccupancy;
	NpcActorMovementRefreshWork2D work;
	NpcActorOccupancy2D occupancy;
	NpcActorOccupancyRefresh2DStatus status = NpcActorOccupancyRefresh2DStatus::NoRefreshNeeded;
	bool refreshed = false;

	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool refreshedOccupancy() const;
};

class NpcActorOccupancyRefresher2D {
public:
	[[nodiscard]] NpcActorOccupancyRefresh2DResult refresh(
		const NpcActorState2DRegistry &registry,
		const NpcActorOccupancy2D &previousOccupancy,
		const NpcActorMovementRefreshWork2D &work,
		const NpcActorOccupancyRefresh2DConfig &config = {}) const;
};

} // namespace iggy
