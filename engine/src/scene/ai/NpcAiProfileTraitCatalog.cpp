#include "scene/ai/NpcAiProfileTraitCatalog.hpp"

namespace {

const iggy::NpcAiProfileTraitEntry *FindEntry(
	const std::vector<iggy::NpcAiProfileTraitEntry> &entries,
	const iggy::ResourceId &profileId,
	std::size_t limit,
	std::size_t &entryIndex)
{
	for (std::size_t index = 0; index < limit; ++index) {
		if (entries[index].profileId == profileId) {
			entryIndex = index;
			return &entries[index];
		}
	}
	return nullptr;
}

void AddIssue(
	std::vector<iggy::NpcAiProfileTraitCatalogIssue> &issues,
	iggy::NpcAiProfileTraitCatalogIssueCode code,
	std::size_t entryIndex,
	const iggy::NpcAiProfileTraitEntry &entry,
	std::size_t firstEntryIndex = 0,
	iggy::NpcTraitSetValidationResult traitValidation = {})
{
	issues.push_back({
		code,
		entryIndex,
		firstEntryIndex,
		entry,
		traitValidation,
	});
}

} // namespace

namespace iggy {

const NpcAiProfileTraitEntry *NpcAiProfileTraitCatalog::find(const ResourceId &profileId) const
{
	for (const NpcAiProfileTraitEntry &entry : entries) {
		if (entry.profileId == profileId)
			return &entry;
	}
	return nullptr;
}

bool NpcAiProfileTraitCatalog::contains(const ResourceId &profileId) const
{
	return find(profileId) != nullptr;
}

bool NpcAiProfileTraitCatalogBuildResult::ok() const
{
	return built;
}

NpcAiProfileTraitCatalogBuildResult NpcAiProfileTraitCatalogBuilder::build(
	const std::vector<NpcAiProfileTraitEntry> &entries) const
{
	NpcAiProfileTraitCatalogBuildResult result;
	result.entryCount = entries.size();

	for (std::size_t index = 0; index < entries.size(); ++index) {
		const NpcAiProfileTraitEntry &entry = entries[index];
		if (entry.profileId.empty()) {
			AddIssue(result.issues, NpcAiProfileTraitCatalogIssueCode::EmptyProfileId, index, entry);
		}

		std::size_t firstEntryIndex = 0;
		if (!entry.profileId.empty() && FindEntry(entries, entry.profileId, index, firstEntryIndex) != nullptr) {
			AddIssue(
				result.issues,
				NpcAiProfileTraitCatalogIssueCode::DuplicateProfileId,
				index,
				entry,
				firstEntryIndex);
		}

		const NpcTraitSetValidationResult traitValidation = validate(entry.traits);
		if (!traitValidation.ok()) {
			AddIssue(
				result.issues,
				NpcAiProfileTraitCatalogIssueCode::InvalidTraitSet,
				index,
				entry,
				0,
				traitValidation);
		}
	}

	result.built = result.issues.empty();
	if (result.built)
		result.catalog.entries = entries;
	return result;
}

} // namespace iggy
