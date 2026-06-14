#include "scene/ai/NpcStrengthPool.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcStrengthEnt> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool StrengthInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

iggy::NpcStrengthPoolIssue Issue(
	iggy::NpcStrengthPoolIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcStrengthEnt &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcStrengthEnt *NpcStrengthPool::find(const ResourceId &entryId) const
{
	for (const NpcStrengthEnt &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcStrengthPool::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcStrengthPoolBuildResult NpcStrengthPoolBuilder::build(
	const std::vector<NpcStrengthEnt> &entries) const
{
	NpcStrengthPoolBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcStrengthEnt &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcStrengthPoolIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcStrengthPoolIssueCode::DuplicateEntryId, index, entry));
		if (!StrengthInRange(entry.minimumStrength))
			result.issues.push_back(Issue(NpcStrengthPoolIssueCode::MinimumStrengthOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcStrengthPoolIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcStrengthPoolIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.entries = entries;
	return result;
}

} // namespace iggy
