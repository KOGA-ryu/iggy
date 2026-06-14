#include "scene/ai/NpcConstitutionPool.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcConstitutionEnt> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool ConstitutionInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

iggy::NpcConstitutionPoolIssue Issue(
	iggy::NpcConstitutionPoolIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcConstitutionEnt &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcConstitutionEnt *NpcConstitutionPool::find(const ResourceId &entryId) const
{
	for (const NpcConstitutionEnt &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcConstitutionPool::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcConstitutionPoolBuildResult NpcConstitutionPoolBuilder::build(
	const std::vector<NpcConstitutionEnt> &entries) const
{
	NpcConstitutionPoolBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcConstitutionEnt &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcConstitutionPoolIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcConstitutionPoolIssueCode::DuplicateEntryId, index, entry));
		if (!ConstitutionInRange(entry.minimumConstitution))
			result.issues.push_back(Issue(NpcConstitutionPoolIssueCode::MinimumConstitutionOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcConstitutionPoolIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcConstitutionPoolIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.entries = entries;
	return result;
}

} // namespace iggy
