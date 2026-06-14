#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

struct NpcAiBehaviorPreset2D {
	ResourceId presetId;
	float aggression = 0.0F;
	float bravery = 0.0F;
	float alertness = 0.0F;
	float preferredRange = 0.0F;
	std::vector<ResourceId> behaviorTags;
};

struct NpcAiBehaviorStore2D {
	std::vector<NpcAiBehaviorPreset2D> presets;

	[[nodiscard]] const NpcAiBehaviorPreset2D *find(const ResourceId &presetId) const;
	[[nodiscard]] bool contains(const ResourceId &presetId) const;
};

enum class NpcAiBehaviorStore2DIssueCode {
	EmptyPresetId,
	DuplicatePresetId,
	AggressionOutOfRange,
	BraveryOutOfRange,
	AlertnessOutOfRange,
	NegativePreferredRange,
};

struct NpcAiBehaviorStore2DIssue {
	NpcAiBehaviorStore2DIssueCode code = NpcAiBehaviorStore2DIssueCode::EmptyPresetId;
	std::size_t presetIndex = 0;
	NpcAiBehaviorPreset2D preset;
};

struct NpcAiBehaviorStore2DBuildResult {
	bool built = false;
	NpcAiBehaviorStore2D store;
	std::vector<NpcAiBehaviorStore2DIssue> issues;
};

class NpcAiBehaviorStore2DBuilder {
public:
	[[nodiscard]] NpcAiBehaviorStore2DBuildResult build(const std::vector<NpcAiBehaviorPreset2D> &presets) const;
};

} // namespace iggy
