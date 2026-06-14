#include "scene/ai/NpcAiBehaviorPool.hpp"

namespace {

bool InUnitRange(float value)
{
	return value >= 0.0F && value <= 1.0F;
}

bool HasEarlierMatchingId(const std::vector<iggy::NpcAiBehaviorPreset> &presets, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (presets[index].presetId == presets[currentIndex].presetId)
			return true;
	}
	return false;
}

iggy::NpcAiBehaviorPoolIssue Issue(
	iggy::NpcAiBehaviorPoolIssueCode code,
	std::size_t presetIndex,
	const iggy::NpcAiBehaviorPreset &preset)
{
	return {
		code,
		presetIndex,
		preset,
	};
}

} // namespace

namespace iggy {

const NpcAiBehaviorPreset *NpcAiBehaviorPool::find(const ResourceId &presetId) const
{
	for (const NpcAiBehaviorPreset &preset : presets) {
		if (preset.presetId == presetId)
			return &preset;
	}
	return nullptr;
}

bool NpcAiBehaviorPool::contains(const ResourceId &presetId) const
{
	return find(presetId) != nullptr;
}

NpcAiBehaviorPoolBuildResult NpcAiBehaviorPoolBuilder::build(
	const std::vector<NpcAiBehaviorPreset> &presets) const
{
	NpcAiBehaviorPoolBuildResult result;

	for (std::size_t index = 0; index < presets.size(); ++index) {
		const NpcAiBehaviorPreset &preset = presets[index];
		if (preset.presetId.empty())
			result.issues.push_back(Issue(NpcAiBehaviorPoolIssueCode::EmptyPresetId, index, preset));
		if (HasEarlierMatchingId(presets, index))
			result.issues.push_back(Issue(NpcAiBehaviorPoolIssueCode::DuplicatePresetId, index, preset));
		if (!InUnitRange(preset.aggression))
			result.issues.push_back(Issue(NpcAiBehaviorPoolIssueCode::AggressionOutOfRange, index, preset));
		if (!InUnitRange(preset.bravery))
			result.issues.push_back(Issue(NpcAiBehaviorPoolIssueCode::BraveryOutOfRange, index, preset));
		if (!InUnitRange(preset.alertness))
			result.issues.push_back(Issue(NpcAiBehaviorPoolIssueCode::AlertnessOutOfRange, index, preset));
		if (preset.preferredRange < 0.0F)
			result.issues.push_back(Issue(NpcAiBehaviorPoolIssueCode::NegativePreferredRange, index, preset));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.presets = presets;
	return result;
}

} // namespace iggy
