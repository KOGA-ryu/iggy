#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

enum class NpcActorInteractionRefresh2DStatus {
	Refreshed,
	NoRefreshNeeded,
};

struct NpcActorInteractionRefresh2DConfig {
	bool includeAbsentActors = false;
	bool includeDisabledTargets = false;
};

struct NpcActorInteractionRefreshActor2D {
	ResourceId npcId;
	TileCoord tile;
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
};

struct NpcActorInteractionRefreshTarget2D {
	ResourceId targetId;
	TileCoord tile;
	std::size_t targetIndex = 0;
	InteractionTarget2D target;
};

struct NpcActorInteractionRefresh2DResult {
	NpcActorMovementRefreshWork2D work;
	NpcActorState2DRegistry actors;
	InteractionTarget2DRegistry targets;
	std::vector<TileCoord> dirtyTiles;
	std::vector<NpcActorInteractionRefreshActor2D> affectedActors;
	std::vector<NpcActorInteractionRefreshTarget2D> affectedTargets;
	NpcActorInteractionRefresh2DStatus status =
		NpcActorInteractionRefresh2DStatus::NoRefreshNeeded;
	std::size_t dirtyTileCount = 0;
	std::size_t affectedActorCount = 0;
	std::size_t affectedTargetCount = 0;
	bool refreshed = false;

	[[nodiscard]] bool hasWork() const;
	[[nodiscard]] bool hasAffectedFacts() const;
};

class NpcActorInteractionRefresher2D {
public:
	[[nodiscard]] NpcActorInteractionRefresh2DResult refresh(
		const NpcActorMovementRefreshWork2D &work,
		const NpcActorState2DRegistry &actors = {},
		const InteractionTarget2DRegistry &targets = {},
		const NpcActorInteractionRefresh2DConfig &config = {}) const;
};

} // namespace iggy
