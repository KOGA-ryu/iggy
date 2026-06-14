#include "scene/ai/NpcStrengthStore2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcStrengthBehaviorEntry2D> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool StrengthInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSet2DMaxScore);
}

iggy::NpcStrengthStore2DIssue Issue(
	iggy::NpcStrengthStore2DIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcStrengthBehaviorEntry2D &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcStrengthBehaviorEntry2D *NpcStrengthStore2D::find(const ResourceId &entryId) const
{
	for (const NpcStrengthBehaviorEntry2D &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcStrengthStore2D::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcStrengthStore2DBuildResult NpcStrengthStore2DBuilder::build(
	const std::vector<NpcStrengthBehaviorEntry2D> &entries) const
{
	NpcStrengthStore2DBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcStrengthBehaviorEntry2D &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcStrengthStore2DIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcStrengthStore2DIssueCode::DuplicateEntryId, index, entry));
		if (!StrengthInRange(entry.minimumStrength))
			result.issues.push_back(Issue(NpcStrengthStore2DIssueCode::MinimumStrengthOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcStrengthStore2DIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcStrengthStore2DIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.store.entries = entries;
	return result;
}

} // namespace iggy
