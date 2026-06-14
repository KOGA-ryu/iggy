#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"

namespace iggy {

enum class NpcActorOccupancyQuery2DStatus {
	Empty,
	Occupied,
};

enum class NpcActorOccupancyBlock2DStatus {
	Empty,
	OnlySelf,
	Blocked,
};

struct NpcActorOccupancyQuery2DResult {
	TileCoord tile;
	NpcActorOccupancyQuery2DStatus status = NpcActorOccupancyQuery2DStatus::Empty;
	std::vector<ResourceId> npcIds;
	std::vector<std::size_t> actorIndexes;
	std::vector<NpcActorOccupancyEntry2D> entries;

	[[nodiscard]] bool occupied() const;
};

struct NpcActorOccupancyBlock2DResult {
	TileCoord tile;
	ResourceId movingNpcId;
	NpcActorOccupancyBlock2DStatus status = NpcActorOccupancyBlock2DStatus::Empty;
	NpcActorOccupancyQuery2DResult occupancy;

	[[nodiscard]] bool blocked() const;
};

class NpcActorOccupancyQuery2D {
public:
	[[nodiscard]] NpcActorOccupancyQuery2DResult occupantsAt(
		const NpcActorOccupancy2D &occupancy,
		TileCoord tile) const;

	[[nodiscard]] const NpcActorOccupancyEntry2D *firstOccupantAt(
		const NpcActorOccupancy2D &occupancy,
		TileCoord tile) const;

	[[nodiscard]] bool contains(const NpcActorOccupancy2D &occupancy, TileCoord tile) const;
	[[nodiscard]] bool isOccupied(const NpcActorOccupancy2D &occupancy, TileCoord tile) const;

	[[nodiscard]] NpcActorOccupancyBlock2DResult blockedFor(
		const NpcActorOccupancy2D &occupancy,
		const ResourceId &movingNpcId,
		TileCoord tile) const;
};

[[nodiscard]] NpcActorOccupancyQuery2DResult npcActorOccupantsAt(
	const NpcActorOccupancy2D &occupancy,
	TileCoord tile);

[[nodiscard]] const NpcActorOccupancyEntry2D *npcActorFirstOccupantAt(
	const NpcActorOccupancy2D &occupancy,
	TileCoord tile);

[[nodiscard]] bool npcActorOccupancyContains(const NpcActorOccupancy2D &occupancy, TileCoord tile);
[[nodiscard]] bool npcActorTileOccupied(const NpcActorOccupancy2D &occupancy, TileCoord tile);

[[nodiscard]] NpcActorOccupancyBlock2DResult npcActorTileBlockedFor(
	const NpcActorOccupancy2D &occupancy,
	const ResourceId &movingNpcId,
	TileCoord tile);

} // namespace iggy
