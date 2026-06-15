#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

enum class NpcActorAiMapRefresh2DStatus {
	Refreshed,
	NoRefreshNeeded,
};

struct NpcActorAiMapRefresh2DConfig {
	bool includeAbsentActors = false;
	bool queryAffectedActors = true;
};

struct NpcActorAiMapRefreshActor2D {
	ResourceId npcId;
	TileCoord tile;
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
	AiMapQuery2DResult query;
};

struct NpcActorAiMapRefresh2DResult {
	NpcActorMovementRefreshWork2D work;
	NpcActorState2DRegistry actors;
	AiMap2D map;
	std::vector<TileCoord> dirtyTiles;
	std::vector<NpcActorAiMapRefreshActor2D> affectedActors;
	NpcActorAiMapRefresh2DStatus status = NpcActorAiMapRefresh2DStatus::NoRefreshNeeded;
	std::size_t dirtyTileCount = 0;
	std::size_t affectedActorCount = 0;
	bool refreshed = false;

	[[nodiscard]] bool hasWork() const;
	[[nodiscard]] bool hasAffectedActors() const;
};

class NpcActorAiMapRefresher2D {
public:
	[[nodiscard]] NpcActorAiMapRefresh2DResult refresh(
		const NpcActorMovementRefreshWork2D &work,
		const NpcActorState2DRegistry &actors = {},
		const AiMap2D &map = {},
		const NpcActorAiMapRefresh2DConfig &config = {}) const;
};

} // namespace iggy
