#include "scene/ai/NpcDexterityPool.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcDexterityEnt> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].entryId == entries[currentIndex].entryId)
			return true;
	}
	return false;
}

bool DexterityInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

iggy::NpcDexterityPoolIssue Issue(
	iggy::NpcDexterityPoolIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcDexterityEnt &entry)
{
	return {
		code,
		entryIndex,
		entry,
	};
}

} // namespace

namespace iggy {

const NpcDexterityEnt *NpcDexterityPool::find(const ResourceId &entryId) const
{
	for (const NpcDexterityEnt &entry : entries) {
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

bool NpcDexterityPool::contains(const ResourceId &entryId) const
{
	return find(entryId) != nullptr;
}

NpcDexterityPoolBuildResult NpcDexterityPoolBuilder::build(
	const std::vector<NpcDexterityEnt> &entries) const
{
	NpcDexterityPoolBuildResult result;

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcDexterityEnt &entry = entries[index];
		if (entry.entryId.empty())
			result.issues.push_back(Issue(NpcDexterityPoolIssueCode::EmptyEntryId, index, entry));
		if (HasEarlierMatchingId(entries, index))
			result.issues.push_back(Issue(NpcDexterityPoolIssueCode::DuplicateEntryId, index, entry));
		if (!DexterityInRange(entry.minimumDexterity))
			result.issues.push_back(Issue(NpcDexterityPoolIssueCode::MinimumDexterityOutOfRange, index, entry));
		if (entry.actionTag.empty())
			result.issues.push_back(Issue(NpcDexterityPoolIssueCode::EmptyActionTag, index, entry));
		if (entry.weight < 0.0F)
			result.issues.push_back(Issue(NpcDexterityPoolIssueCode::NegativeWeight, index, entry));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.pool.entries = entries;
	return result;
}

} // namespace iggy
