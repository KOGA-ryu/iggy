#pragma once

#include <cstddef>
#include <vector>

#include "scene/npc/NpcActorFrameState2D.hpp"
#include "scene/npc/NpcActorMovementIntent2D.hpp"

namespace iggy {

enum class NpcActorMovementFrameIntent2DStatus {
	Projected,
	NoReadyMovement,
	NoFrameEntries,
};

struct NpcActorMovementFrameIntent2DEntry {
	std::size_t frameIndex = 0;
	NpcActorFrameState2D frame;
	NpcActorMovementIntent2D intent;
};

struct NpcActorMovementFrameIntent2DResult {
	NpcActorFrameState2DProjectionResult frameState;
	std::vector<NpcActorMovementFrameIntent2DEntry> entries;
	std::vector<NpcActorFrameState2DIssue> issues;
	NpcActorMovementFrameIntent2DStatus status = NpcActorMovementFrameIntent2DStatus::NoFrameEntries;
	std::size_t entryCount = 0;
	std::size_t readyCount = 0;
	std::size_t noMovementCount = 0;
	std::size_t missingControlCount = 0;
	std::size_t actorNotPresentCount = 0;
	std::size_t unsupportedBehaviorCount = 0;
	std::size_t invalidMoveModeCount = 0;

	[[nodiscard]] bool hasReadyMovement() const;
};

class NpcActorMovementFrameIntentProjector2D {
public:
	[[nodiscard]] NpcActorMovementFrameIntent2DResult project(
		const NpcActorFrameState2DProjectionResult &frameState,
		const NpcActorMovementIntent2DConfig &config = {}) const;
};

} // namespace iggy
