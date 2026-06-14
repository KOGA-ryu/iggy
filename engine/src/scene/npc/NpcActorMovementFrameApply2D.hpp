#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "scene/npc/NpcActorMovementExecutor2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

enum class NpcActorMovementFrameApply2DStatus {
	Applied,
	NoMovementsApplied,
};

enum class NpcActorMovementFrameApply2DIssueCode {
	ActorNotFound,
};

struct NpcActorMovementFrameApply2DRequest {
	NpcActorPathStepOccupancyFilter2D filter;
};

struct NpcActorMovementFrameApply2DEntry {
	std::size_t requestIndex = 0;
	NpcActorMovementFrameApply2DRequest request;
	std::optional<std::size_t> actorIndex;
	NpcActorMovementExecutor2DResult executor;
	bool hasIssue = false;
	NpcActorMovementFrameApply2DIssueCode issue = NpcActorMovementFrameApply2DIssueCode::ActorNotFound;
};

struct NpcActorMovementFrameApply2DResult {
	NpcActorState2DRegistry inputRegistry;
	NpcActorState2DRegistry registry;
	std::vector<NpcActorMovementFrameApply2DEntry> entries;
	NpcActorMovementFrameApply2DStatus status = NpcActorMovementFrameApply2DStatus::NoMovementsApplied;
	std::size_t requestCount = 0;
	std::size_t appliedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedCount = 0;
	std::size_t noMovementCount = 0;
	std::size_t rejectedCount = 0;
	std::size_t missingActorCount = 0;
	std::size_t changedCount = 0;
	bool changed = false;

	[[nodiscard]] bool applied() const;
	[[nodiscard]] bool hasChanges() const;
};

class NpcActorMovementFrameApplier2D {
public:
	[[nodiscard]] NpcActorMovementFrameApply2DResult apply(
		const NpcActorState2DRegistry &registry,
		const std::vector<NpcActorMovementFrameApply2DRequest> &requests) const;
};

} // namespace iggy
