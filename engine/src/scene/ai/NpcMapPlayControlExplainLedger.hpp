#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcAiProfileTraitResolver.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"

namespace iggy {

enum class NpcMapPlayControlExplainLedgerStatus {
	Empty,
	Reported,
};

enum class NpcMapPlayControlExplainEvent {
	ProfileResolved,
	ProfileMissing,
	ProfileEmptyProfileId,
	ProfileAbsentSkipped,
	MissingControl,
	MissingTraitSet,
	ActorNotPresent,
	HandDrawn,
	HandIssueObserved,
	MapQueried,
	MapSelectionChanged,
	PlayKept,
	PlayFolded,
	ControlProposed,
	ControlFailed,
	ControlApplied,
	ControlApplyFailed,
};

struct NpcMapPlayControlExplainRow {
	std::size_t index = 0;
	NpcMapPlayControlExplainEvent event = NpcMapPlayControlExplainEvent::HandDrawn;
	bool hasActorIndex = false;
	std::size_t actorIndex = 0;
	bool hasFrameIndex = false;
	std::size_t frameIndex = 0;
	bool hasRequestIndex = false;
	std::size_t requestIndex = 0;
	ResourceId npcId;
	ResourceId profileId;
	ResourceId actionTag;
};

struct NpcMapPlayControlExplainLedger {
	bool hasProfile = false;
	NpcAiProfileTraitResolveResult profile;
	NpcMapPlayControlFramePlanResult plan;
	NpcMapPlayControlFrameStep2DResult step;
	NpcMapPlayControlExplainLedgerStatus status =
		NpcMapPlayControlExplainLedgerStatus::Empty;
	std::vector<NpcMapPlayControlExplainRow> rows;
	std::size_t profileResolvedCount = 0;
	std::size_t profileMissingCount = 0;
	std::size_t profileEmptyIdCount = 0;
	std::size_t profileAbsentSkippedCount = 0;
	std::size_t missingControlCount = 0;
	std::size_t missingTraitCount = 0;
	std::size_t actorNotPresentCount = 0;
	std::size_t handDrawnCount = 0;
	std::size_t handIssueCount = 0;
	std::size_t mapQueryCount = 0;
	std::size_t mapChangedSelectionCount = 0;
	std::size_t playKeptCount = 0;
	std::size_t playFoldedCount = 0;
	std::size_t controlProposedCount = 0;
	std::size_t controlFailedCount = 0;
	std::size_t controlAppliedCount = 0;
	std::size_t controlApplyFailedCount = 0;

	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool hasRows() const;
	[[nodiscard]] bool hasControlChanges() const;
};

class NpcMapPlayControlExplainLedgerReporter {
public:
	[[nodiscard]] NpcMapPlayControlExplainLedger report(
		const NpcMapPlayControlFramePlanResult &plan,
		const NpcMapPlayControlFrameStep2DResult &step) const;

	[[nodiscard]] NpcMapPlayControlExplainLedger report(
		const NpcAiProfileTraitResolveResult &profile,
		const NpcMapPlayControlFramePlanResult &plan,
		const NpcMapPlayControlFrameStep2DResult &step) const;
};

} // namespace iggy
