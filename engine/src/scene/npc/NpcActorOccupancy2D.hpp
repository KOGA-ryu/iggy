#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

struct NpcActorOccupancy2DConfig {
	bool includeAbsent = false;
};

enum class NpcActorOccupancy2DStatus {
	Built,
};

struct NpcActorOccupancyEntry2D {
	ResourceId npcId;
	TileCoord tile;
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
};

struct NpcActorOccupiedTile2D {
	TileCoord tile;
	std::vector<ResourceId> npcIds;
	std::vector<std::size_t> actorIndexes;
};

enum class NpcActorOccupancyIssue2DCode {
	DuplicateOccupiedTile,
};

struct NpcActorOccupancyIssue2D {
	NpcActorOccupancyIssue2DCode code = NpcActorOccupancyIssue2DCode::DuplicateOccupiedTile;
	TileCoord tile;
	std::size_t firstActorIndex = 0;
	std::size_t laterActorIndex = 0;
	NpcActorOccupiedTile2D occupiedTile;
};

struct NpcActorOccupancy2D {
	NpcActorOccupancy2DStatus status = NpcActorOccupancy2DStatus::Built;
	std::vector<NpcActorOccupancyEntry2D> entries;
	std::vector<NpcActorOccupiedTile2D> occupiedTiles;
	std::vector<NpcActorOccupancyIssue2D> issues;

	[[nodiscard]] bool hasIssues() const;
};

class NpcActorOccupancyProjector2D {
public:
	[[nodiscard]] NpcActorOccupancy2D project(
		const NpcActorState2DRegistry &registry,
		const NpcActorOccupancy2DConfig &config = {}) const;
};

} // namespace iggy
