#pragma once

#include <cstddef>

#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorOccupancyQuery2D.hpp"

namespace iggy {

enum class NpcActorOccupancyPolicy2DStatus {
	Allowed,
	Blocked,
};

struct NpcActorOccupancyPolicy2DConfig {
	std::size_t maxOccupantsPerTile = 1;
};

struct NpcActorOccupancyPolicy2DResult {
	NpcActorOccupancyQuery2DResult occupancy;
	NpcActorOccupancyPolicy2DStatus status = NpcActorOccupancyPolicy2DStatus::Blocked;
	ResourceId movingNpcId;
	ResourceId blockingNpcId;
	TileCoord tile;
	std::size_t occupancyCount = 0;
	std::size_t effectiveOccupancyCount = 0;
	std::size_t capacity = 1;

	[[nodiscard]] bool allowed() const;
	[[nodiscard]] bool blocked() const;
};

class NpcActorOccupancyPolicy2D {
public:
	[[nodiscard]] NpcActorOccupancyPolicy2DResult evaluate(
		const NpcActorOccupancy2D &occupancy,
		const ResourceId &movingNpcId,
		TileCoord tile,
		const NpcActorOccupancyPolicy2DConfig &config = {}) const;
};

} // namespace iggy
