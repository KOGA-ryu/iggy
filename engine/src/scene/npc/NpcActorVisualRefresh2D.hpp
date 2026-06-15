#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

enum class NpcActorVisualRefresh2DStatus {
	Refreshed,
	NoRefreshNeeded,
};

struct NpcActorVisualRefresh2DConfig {
	bool includeAbsentActors = false;
};

struct NpcActorVisualRefreshActor2D {
	ResourceId npcId;
	TileCoord tile;
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
};

struct NpcActorVisualRefreshPacket2D {
	std::vector<TileCoord> dirtyTiles;
	std::vector<NpcActorVisualRefreshActor2D> affectedActors;
	NpcActorVisualRefresh2DStatus status = NpcActorVisualRefresh2DStatus::NoRefreshNeeded;
	std::size_t dirtyTileCount = 0;
	std::size_t affectedActorCount = 0;
	bool refreshNeeded = false;

	[[nodiscard]] bool hasWork() const;
	[[nodiscard]] bool hasAffectedActors() const;
};

struct NpcActorVisualRefresh2DResult {
	NpcActorMovementRefreshWork2D work;
	NpcActorState2DRegistry actors;
	NpcActorVisualRefreshPacket2D render;
	NpcActorVisualRefreshPacket2D visibility;

	[[nodiscard]] bool hasWork() const;
};

class NpcActorVisualRefresher2D {
public:
	[[nodiscard]] NpcActorVisualRefresh2DResult refresh(
		const NpcActorMovementRefreshWork2D &work,
		const NpcActorState2DRegistry &actors = {},
		const NpcActorVisualRefresh2DConfig &config = {}) const;
};

} // namespace iggy
