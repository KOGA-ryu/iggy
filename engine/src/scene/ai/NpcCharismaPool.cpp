#include "scene/ai/NpcCharismaPool.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcCharismaEnt> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool CharismaInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

iggy::NpcCharismaPoolIssue Issue(
	iggy::NpcCharismaPoolIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcCharismaEnt &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcCharismaEnt *NpcCharismaPool::find(const ResourceId &entryId) const
{
	for (const NpcCharismaEnt &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcCharismaPool::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcCharismaPoolBuildResult NpcCharismaPoolBuilder::build(
	const std::vector<NpcCharismaEnt> &entries) const
{
	NpcCharismaPoolBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcCharismaEnt &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcCharismaPoolIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcCharismaPoolIssueCode::DuplicateEntryId, index, entry));
		if (!CharismaInRange(entry.minimumCharisma))
			result.issues.push_back(Issue(NpcCharismaPoolIssueCode::MinimumCharismaOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcCharismaPoolIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcCharismaPoolIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.entries = entries;
	return result;
}

} // namespace iggy
