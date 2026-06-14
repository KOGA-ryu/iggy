#include "scene/ai/NpcAiBehaviorStore2D.hpp"

namespace {

bool InUnitRange(float value)
{
	return value >= 0.0F && value <= 1.0F;
}

bool HasEarlierMatchingId(const std::vector<iggy::NpcAiBehaviorPreset2D> &presets, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (presets[index].presetId == presets[currentIndex].presetId)
			return true;
	}
	return false;
}

iggy::NpcAiBehaviorStore2DIssue Issue(
	iggy::NpcAiBehaviorStore2DIssueCode code,
	std::size_t presetIndex,
	const iggy::NpcAiBehaviorPreset2D &preset)
{
	return {
		code,
		presetIndex,
		preset,
	};
}

} // namespace

namespace iggy {

const NpcAiBehaviorPreset2D *NpcAiBehaviorStore2D::find(const ResourceId &presetId) const
{
	for (const NpcAiBehaviorPreset2D &preset : presets) {
		if (preset.presetId == presetId)
			return &preset;
	}
	return nullptr;
}

bool NpcAiBehaviorStore2D::contains(const ResourceId &presetId) const
{
	return find(presetId) != nullptr;
}

NpcAiBehaviorStore2DBuildResult NpcAiBehaviorStore2DBuilder::build(
	const std::vector<NpcAiBehaviorPreset2D> &presets) const
{
	NpcAiBehaviorStore2DBuildResult result;

	for (std::size_t index = 0; index < presets.size(); ++index) {
		const NpcAiBehaviorPreset2D &preset = presets[index];
		if (preset.presetId.empty())
			result.issues.push_back(Issue(NpcAiBehaviorStore2DIssueCode::EmptyPresetId, index, preset));
		if (HasEarlierMatchingId(presets, index))
			result.issues.push_back(Issue(NpcAiBehaviorStore2DIssueCode::DuplicatePresetId, index, preset));
		if (!InUnitRange(preset.aggression))
			result.issues.push_back(Issue(NpcAiBehaviorStore2DIssueCode::AggressionOutOfRange, index, preset));
		if (!InUnitRange(preset.bravery))
			result.issues.push_back(Issue(NpcAiBehaviorStore2DIssueCode::BraveryOutOfRange, index, preset));
		if (!InUnitRange(preset.alertness))
			result.issues.push_back(Issue(NpcAiBehaviorStore2DIssueCode::AlertnessOutOfRange, index, preset));
		if (preset.preferredRange < 0.0F)
			result.issues.push_back(Issue(NpcAiBehaviorStore2DIssueCode::NegativePreferredRange, index, preset));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.store.presets = presets;
	return result;
}

} // namespace iggy
