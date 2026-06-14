#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

struct NpcDexterityEnt {
	ResourceId entryId;
	std::uint32_t minimumDexterity = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcDexterityPool {
	std::vector<NpcDexterityEnt> entries;

	[[nodiscard]] const NpcDexterityEnt *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcDexterityPoolIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumDexterityOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcDexterityPoolIssue {
	NpcDexterityPoolIssueCode code = NpcDexterityPoolIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcDexterityEnt entry;
};

struct NpcDexterityPoolBuildResult {
	bool built = false;
	NpcDexterityPool pool;
	std::vector<NpcDexterityPoolIssue> issues;
};

class NpcDexterityPoolBuilder {
public:
	[[nodiscard]] NpcDexterityPoolBuildResult build(
		const std::vector<NpcDexterityEnt> &entries) const;
};

} // namespace iggy
