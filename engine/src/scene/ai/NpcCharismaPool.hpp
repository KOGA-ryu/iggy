#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

struct NpcCharismaEnt {
	ResourceId entryId;
	std::uint32_t minimumCharisma = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcCharismaPool {
	std::vector<NpcCharismaEnt> entries;

	[[nodiscard]] const NpcCharismaEnt *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcCharismaPoolIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumCharismaOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcCharismaPoolIssue {
	NpcCharismaPoolIssueCode code = NpcCharismaPoolIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcCharismaEnt entry;
};

struct NpcCharismaPoolBuildResult {
	bool built = false;
	NpcCharismaPool pool;
	std::vector<NpcCharismaPoolIssue> issues;
};

class NpcCharismaPoolBuilder {
public:
	[[nodiscard]] NpcCharismaPoolBuildResult build(
		const std::vector<NpcCharismaEnt> &entries) const;
};

} // namespace iggy
