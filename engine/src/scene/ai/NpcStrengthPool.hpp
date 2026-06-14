#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

struct NpcStrengthEnt {
	ResourceId entryId;
	std::uint32_t minimumStrength = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcStrengthPool {
	std::vector<NpcStrengthEnt> entries;

	[[nodiscard]] const NpcStrengthEnt *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcStrengthPoolIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumStrengthOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcStrengthPoolIssue {
	NpcStrengthPoolIssueCode code = NpcStrengthPoolIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcStrengthEnt entry;
};

struct NpcStrengthPoolBuildResult {
	bool built = false;
	NpcStrengthPool pool;
	std::vector<NpcStrengthPoolIssue> issues;
};

class NpcStrengthPoolBuilder {
public:
	[[nodiscard]] NpcStrengthPoolBuildResult build(
		const std::vector<NpcStrengthEnt> &entries) const;
};

} // namespace iggy
