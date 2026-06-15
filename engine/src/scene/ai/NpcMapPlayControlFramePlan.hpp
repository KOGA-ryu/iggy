#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcCharismaDraw.hpp"
#include "scene/ai/NpcCharismaPool.hpp"
#include "scene/ai/NpcConstitutionDraw.hpp"
#include "scene/ai/NpcConstitutionPool.hpp"
#include "scene/ai/NpcDexterityDraw.hpp"
#include "scene/ai/NpcDexterityPool.hpp"
#include "scene/ai/NpcHand.hpp"
#include "scene/ai/NpcIntelligenceDraw.hpp"
#include "scene/ai/NpcIntelligencePool.hpp"
#include "scene/ai/NpcMapPlayControlFrameStep2D.hpp"
#include "scene/ai/NpcStrengthDraw.hpp"
#include "scene/ai/NpcStrengthPool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/ai/NpcWisdomDraw.hpp"
#include "scene/ai/NpcWisdomPool.hpp"
#include "scene/npc/NpcActorFrameState2D.hpp"

namespace iggy {

struct NpcMapPlayControlFramePlanSubject {
	ResourceId npcId;
	NpcTraitSet traits;
	bool hasProposalContextOverride = false;
	NpcPlayControlProposalContext proposalContext;
};

struct NpcMapPlayControlFramePlanPools {
	NpcStrengthPool strength;
	NpcDexterityPool dexterity;
	NpcConstitutionPool constitution;
	NpcIntelligencePool intelligence;
	NpcWisdomPool wisdom;
	NpcCharismaPool charisma;
};

struct NpcMapPlayControlFramePlanConfig {
	bool includeAbsentActors = false;
	NpcMapPlayControlFrameStep2DConfig step;
};

enum class NpcMapPlayControlFramePlanEntryStatus {
	RequestPrepared,
	MissingControl,
	MissingTraitSet,
	ActorNotPresent,
};

enum class NpcMapPlayControlFramePlanIssueCode {
	MissingControlState,
	OrphanControlState,
	MissingTraitSet,
	ActorNotPresent,
};

struct NpcMapPlayControlFramePlanIssue {
	NpcMapPlayControlFramePlanIssueCode code = NpcMapPlayControlFramePlanIssueCode::MissingTraitSet;
	std::size_t frameIndex = 0;
	std::size_t subjectIndex = 0;
	NpcActorFrameState2DIssue frameIssue;
	ResourceId npcId;
};

struct NpcMapPlayControlFramePlanEntry {
	std::size_t frameIndex = 0;
	NpcActorFrameState2D frame;
	std::optional<std::size_t> subjectIndex;
	NpcMapPlayControlFramePlanSubject subject;
	NpcTraitSetValidationResult traitValidation;
	NpcStrengthDrawResult strength;
	NpcDexterityDrawResult dexterity;
	NpcConstitutionDrawResult constitution;
	NpcIntelligenceDrawResult intelligence;
	NpcWisdomDrawResult wisdom;
	NpcCharismaDrawResult charisma;
	NpcHand hand;
	AiMapQuery2DResult map;
	NpcPlayControlProposalContext proposalContext;
	std::optional<std::size_t> requestIndex;
	NpcMapPlayControlFrameStep2DRequest request;
	NpcMapPlayControlFramePlanEntryStatus status = NpcMapPlayControlFramePlanEntryStatus::MissingTraitSet;
};

enum class NpcMapPlayControlFramePlanStatus {
	Planned,
	NoRequests,
};

struct NpcMapPlayControlFramePlanResult {
	NpcActorFrameState2DProjectionResult frameState;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	AiMap2D map;
	NpcMapPlayControlFramePlanPools pools;
	NpcMapPlayControlFramePlanConfig config;
	std::vector<NpcMapPlayControlFramePlanEntry> entries;
	std::vector<NpcMapPlayControlFramePlanIssue> issues;
	std::vector<NpcMapPlayControlFrameStep2DRequest> requests;
	NpcMapPlayControlFramePlanStatus status = NpcMapPlayControlFramePlanStatus::NoRequests;
	std::size_t frameEntryCount = 0;
	std::size_t subjectCount = 0;
	std::size_t requestCount = 0;
	std::size_t preparedCount = 0;
	std::size_t missingControlCount = 0;
	std::size_t orphanControlCount = 0;
	std::size_t missingTraitCount = 0;
	std::size_t actorNotPresentCount = 0;
	std::size_t drawIssueCount = 0;
	std::size_t mapQueryCount = 0;

	[[nodiscard]] bool hasRequests() const;
};

class NpcMapPlayControlFramePlanner {
public:
	[[nodiscard]] NpcMapPlayControlFramePlanResult plan(
		const NpcActorState2DRegistry &actors,
		const NpcActorControlState2DRegistry &controls,
		const std::vector<NpcMapPlayControlFramePlanSubject> &subjects,
		const NpcMapPlayControlFramePlanPools &pools,
		const AiMap2D &map,
		const NpcMapPlayControlFramePlanConfig &config = {}) const;
};

} // namespace iggy
