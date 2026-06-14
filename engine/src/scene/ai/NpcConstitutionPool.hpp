#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

struct NpcConstitutionEnt {
	ResourceId entryId;
	std::uint32_t minimumConstitution = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcConstitutionPool {
	std::vector<NpcConstitutionEnt> entries;

	[[nodiscard]] const NpcConstitutionEnt *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcConstitutionPoolIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumConstitutionOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcConstitutionPoolIssue {
	NpcConstitutionPoolIssueCode code = NpcConstitutionPoolIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcConstitutionEnt entry;
};

struct NpcConstitutionPoolBuildResult {
	bool built = false;
	NpcConstitutionPool pool;
	std::vector<NpcConstitutionPoolIssue> issues;
};

class NpcConstitutionPoolBuilder {
public:
	[[nodiscard]] NpcConstitutionPoolBuildResult build(
		const std::vector<NpcConstitutionEnt> &entries) const;
};

} // namespace iggy
