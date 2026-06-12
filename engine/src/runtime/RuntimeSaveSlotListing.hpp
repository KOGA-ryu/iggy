#pragma once

#include <filesystem>
#include <vector>

#include "runtime/RuntimeSaveMetadata.hpp"
#include "runtime/RuntimeSaveSlotPathPolicy.hpp"

namespace iggy::runtime {

enum class RuntimeSaveSlotListingIssueCode {
	BaseDirectoryMissing,
	SlotDirectoryMissing,
	InvalidSlotFileName,
	FileReadFailed,
	EnvelopeDecodeFailed,
	ArchiveDecodeFailed,
	MetadataDecodeFailed,
	DuplicateMetadata,
};

struct RuntimeSaveSlotListingIssue {
	RuntimeSaveSlotListingIssueCode code = RuntimeSaveSlotListingIssueCode::BaseDirectoryMissing;
	RuntimeSaveSlotId slot;
	std::filesystem::path path;
};

struct RuntimeSaveSlotListingEntry {
	RuntimeSaveSlotId slot;
	std::filesystem::path path;
	bool hasMetadata = false;
	RuntimeSaveMetadata metadata;
};

struct RuntimeSaveSlotListingResult {
	bool listed = false;
	std::vector<RuntimeSaveSlotListingEntry> entries;
	std::vector<RuntimeSaveSlotListingIssue> issues;
};

class RuntimeSaveSlotListing {
public:
	[[nodiscard]] RuntimeSaveSlotListingResult list(const RuntimeSaveSlotPathPolicyConfig &config) const;
};

} // namespace iggy::runtime
