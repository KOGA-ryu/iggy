#include "scene/ai/NpcIntelligencePool.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcIntelligenceEnt> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool IntelligenceInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

iggy::NpcIntelligencePoolIssue Issue(
	iggy::NpcIntelligencePoolIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcIntelligenceEnt &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcIntelligenceEnt *NpcIntelligencePool::find(const ResourceId &entryId) const
{
	for (const NpcIntelligenceEnt &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcIntelligencePool::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcIntelligencePoolBuildResult NpcIntelligencePoolBuilder::build(
	const std::vector<NpcIntelligenceEnt> &entries) const
{
	NpcIntelligencePoolBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcIntelligenceEnt &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcIntelligencePoolIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcIntelligencePoolIssueCode::DuplicateEntryId, index, entry));
		if (!IntelligenceInRange(entry.minimumIntelligence))
			result.issues.push_back(Issue(NpcIntelligencePoolIssueCode::MinimumIntelligenceOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcIntelligencePoolIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcIntelligencePoolIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.entries = entries;
	return result;
}

} // namespace iggy
