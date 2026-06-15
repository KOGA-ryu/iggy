#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcTraitSet.hpp"

namespace iggy {

struct NpcAiProfileTraitEntry {
	ResourceId profileId;
	NpcTraitSet traits;
};

struct NpcAiProfileTraitCatalog {
	std::vector<NpcAiProfileTraitEntry> entries;

	[[nodiscard]] const NpcAiProfileTraitEntry *find(const ResourceId &profileId) const;
	[[nodiscard]] bool contains(const ResourceId &profileId) const;
};

enum class NpcAiProfileTraitCatalogIssueCode {
	EmptyProfileId,
	DuplicateProfileId,
	InvalidTraitSet,
};

struct NpcAiProfileTraitCatalogIssue {
	NpcAiProfileTraitCatalogIssueCode code = NpcAiProfileTraitCatalogIssueCode::EmptyProfileId;
	std::size_t entryIndex = 0;
	std::size_t firstEntryIndex = 0;
	NpcAiProfileTraitEntry entry;
	NpcTraitSetValidationResult traitValidation;
};

struct NpcAiProfileTraitCatalogBuildResult {
	bool built = false;
	NpcAiProfileTraitCatalog catalog;
	std::vector<NpcAiProfileTraitCatalogIssue> issues;
	std::size_t entryCount = 0;

	[[nodiscard]] bool ok() const;
};

class NpcAiProfileTraitCatalogBuilder {
public:
	[[nodiscard]] NpcAiProfileTraitCatalogBuildResult build(
		const std::vector<NpcAiProfileTraitEntry> &entries) const;
};

} // namespace iggy
