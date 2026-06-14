#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet2D.hpp"
#include "scene/npc/NpcBehaviorState2D.hpp"

namespace iggy {

struct NpcStrengthBehaviorEntry2D {
	ResourceId entryId;
	std::uint32_t minimumStrength = 0;
	NpcBehaviorState2DType behaviorState = NpcBehaviorState2DType::None;
	ResourceId actionTag;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
};

struct NpcStrengthStore2D {
	std::vector<NpcStrengthBehaviorEntry2D> entries;

	[[nodiscard]] const NpcStrengthBehaviorEntry2D *find(const ResourceId &entryId) const;
	[[nodiscard]] bool contains(const ResourceId &entryId) const;
};

enum class NpcStrengthStore2DIssueCode {
	EmptyEntryId,
	DuplicateEntryId,
	MinimumStrengthOutOfRange,
	EmptyActionTag,
	NegativeWeight,
};

struct NpcStrengthStore2DIssue {
	NpcStrengthStore2DIssueCode code = NpcStrengthStore2DIssueCode::EmptyEntryId;
	std::size_t entryIndex = 0;
	NpcStrengthBehaviorEntry2D entry;
};

struct NpcStrengthStore2DBuildResult {
	bool built = false;
	NpcStrengthStore2D store;
	std::vector<NpcStrengthStore2DIssue> issues;
};

class NpcStrengthStore2DBuilder {
public:
	[[nodiscard]] NpcStrengthStore2DBuildResult build(
		const std::vector<NpcStrengthBehaviorEntry2D> &entries) const;
};

} // namespace iggy
