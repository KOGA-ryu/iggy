#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

struct NpcAiBehaviorPreset {
	ResourceId presetId;
	float aggression = 0.0F;
	float bravery = 0.0F;
	float alertness = 0.0F;
	float preferredRange = 0.0F;
	std::vector<ResourceId> behaviorTags;
};

struct NpcAiBehaviorPool {
	std::vector<NpcAiBehaviorPreset> presets;

	[[nodiscard]] const NpcAiBehaviorPreset *find(const ResourceId &presetId) const;
	[[nodiscard]] bool contains(const ResourceId &presetId) const;
};

enum class NpcAiBehaviorPoolIssueCode {
	EmptyPresetId,
	DuplicatePresetId,
	AggressionOutOfRange,
	BraveryOutOfRange,
	AlertnessOutOfRange,
	NegativePreferredRange,
};

struct NpcAiBehaviorPoolIssue {
	NpcAiBehaviorPoolIssueCode code = NpcAiBehaviorPoolIssueCode::EmptyPresetId;
	std::size_t presetIndex = 0;
	NpcAiBehaviorPreset preset;
};

struct NpcAiBehaviorPoolBuildResult {
	bool built = false;
	NpcAiBehaviorPool pool;
	std::vector<NpcAiBehaviorPoolIssue> issues;
};

class NpcAiBehaviorPoolBuilder {
public:
	[[nodiscard]] NpcAiBehaviorPoolBuildResult build(const std::vector<NpcAiBehaviorPreset> &presets) const;
};

} // namespace iggy
