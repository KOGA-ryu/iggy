#include "scene/ai/NpcWisdomPool.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcWisdomEnt> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool WisdomInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

iggy::NpcWisdomPoolIssue Issue(
	iggy::NpcWisdomPoolIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcWisdomEnt &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcWisdomEnt *NpcWisdomPool::find(const ResourceId &entryId) const
{
	for (const NpcWisdomEnt &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcWisdomPool::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcWisdomPoolBuildResult NpcWisdomPoolBuilder::build(
	const std::vector<NpcWisdomEnt> &entries) const
{
	NpcWisdomPoolBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcWisdomEnt &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcWisdomPoolIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcWisdomPoolIssueCode::DuplicateEntryId, index, entry));
		if (!WisdomInRange(entry.minimumWisdom))
			result.issues.push_back(Issue(NpcWisdomPoolIssueCode::MinimumWisdomOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcWisdomPoolIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcWisdomPoolIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.entries = entries;
	return result;
}

} // namespace iggy
