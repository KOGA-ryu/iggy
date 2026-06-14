#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcMapPlayReport.hpp"
#include "scene/ai/NpcPlayControlProposal.hpp"
#include "scene/npc/NpcActorControlState2D.hpp"
#include "scene/npc/NpcPlayControlFrameApply2D.hpp"
#include "scene/npc/NpcPlayControlFrameReport2D.hpp"

namespace iggy {

struct NpcMapPlayControlFrameStep2DConfig {
	NpcReadConfig rawRead;
	NpcMapReadConfig mapRead;
	NpcFoldConfig fold;
	NpcPlayControlProposalConfig proposal;
};

struct NpcMapPlayControlFrameStep2DRequest {
	ResourceId npcId;
	NpcHand hand;
	AiMapQuery2DResult map;
	NpcPlayControlProposalContext proposalContext;
};

struct NpcMapPlayControlFrameStep2DEntry {
	std::size_t requestIndex = 0;
	NpcMapPlayControlFrameStep2DRequest request;
	NpcMapPlayReport mapPlay;
	NpcPlayControlProposal proposal;
	NpcPlayControlFrameProposal2D frameProposal;
};

enum class NpcMapPlayControlFrameStep2DStatus {
	Ran,
	NoRequests,
	NoControlsChanged,
};

struct NpcMapPlayControlFrameStep2DResult {
	NpcActorControlState2DRegistry registry;
	std::vector<NpcMapPlayControlFrameStep2DEntry> entries;
	NpcPlayControlFrameApply2DResult apply;
	NpcPlayControlFrameReport2D report;
	NpcMapPlayControlFrameStep2DStatus status = NpcMapPlayControlFrameStep2DStatus::NoRequests;
	std::size_t requestCount = 0;
	std::size_t proposalCount = 0;
	std::size_t proposedCount = 0;
	std::size_t proposalFailedCount = 0;
	std::size_t appliedCount = 0;
	std::size_t failedApplyCount = 0;
	std::size_t mapChangedSelectionCount = 0;

	[[nodiscard]] bool changed() const;
};

class NpcMapPlayControlFrameStepper2D {
public:
	[[nodiscard]] NpcMapPlayControlFrameStep2DResult step(
		const NpcActorControlState2DRegistry &controls,
		const std::vector<NpcMapPlayControlFrameStep2DRequest> &requests,
		const NpcMapPlayControlFrameStep2DConfig &config = {}) const;
};

} // namespace iggy
