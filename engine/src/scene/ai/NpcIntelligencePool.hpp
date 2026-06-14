#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

struct NpcIntelligenceEnt {
	ResourceId entryId;
	std::uint32_t minimumIntelligence = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcIntelligencePool {
	std::vector<NpcIntelligenceEnt> entries;

	[[nodiscard]] const NpcIntelligenceEnt *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcIntelligencePoolIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumIntelligenceOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcIntelligencePoolIssue {
	NpcIntelligencePoolIssueCode code = NpcIntelligencePoolIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcIntelligenceEnt entry;
};

struct NpcIntelligencePoolBuildResult {
	bool built = false;
	NpcIntelligencePool pool;
	std::vector<NpcIntelligencePoolIssue> issues;
};

class NpcIntelligencePoolBuilder {
public:
	[[nodiscard]] NpcIntelligencePoolBuildResult build(
		const std::vector<NpcIntelligenceEnt> &entries) const;
};

} // namespace iggy
