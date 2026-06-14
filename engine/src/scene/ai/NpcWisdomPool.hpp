#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

struct NpcWisdomEnt {
	ResourceId entryId;
	std::uint32_t minimumWisdom = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcWisdomPool {
	std::vector<NpcWisdomEnt> entries;

	[[nodiscard]] const NpcWisdomEnt *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcWisdomPoolIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumWisdomOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcWisdomPoolIssue {
	NpcWisdomPoolIssueCode code = NpcWisdomPoolIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcWisdomEnt entry;
};

struct NpcWisdomPoolBuildResult {
	bool built = false;
	NpcWisdomPool pool;
	std::vector<NpcWisdomPoolIssue> issues;
};

class NpcWisdomPoolBuilder {
public:
	[[nodiscard]] NpcWisdomPoolBuildResult build(
		const std::vector<NpcWisdomEnt> &entries) const;
};

} // namespace iggy
