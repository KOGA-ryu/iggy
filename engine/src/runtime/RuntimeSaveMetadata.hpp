#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/RuntimeSaveChunkArchive.hpp"

namespace iggy::runtime {

inline constexpr std::uint32_t RuntimeSaveMetadataChunkVersion = 1;
inline constexpr std::uint32_t RuntimeSaveMetadataMaxDisplayNameBytes = 128;

struct RuntimeSaveMetadata {
	std::string displayName;
	std::uint64_t createdTick = 0;
};

enum class RuntimeSaveMetadataIssueCode {
	EmptyDisplayName,
	DisplayNameTooLong,
};

struct RuntimeSaveMetadataIssue {
	RuntimeSaveMetadataIssueCode code = RuntimeSaveMetadataIssueCode::EmptyDisplayName;
};

struct RuntimeSaveMetadataValidationResult {
	bool valid = false;
	std::vector<RuntimeSaveMetadataIssue> issues;
};

class RuntimeSaveMetadataValidator {
public:
	[[nodiscard]] RuntimeSaveMetadataValidationResult validate(const RuntimeSaveMetadata &metadata) const;
};

enum class RuntimeSaveMetadataChunkIssueCode {
	UnexpectedChunkId,
	UnsupportedChunkVersion,
	MalformedPayload,
	MetadataInvalid,
};

struct RuntimeSaveMetadataChunkIssue {
	RuntimeSaveMetadataChunkIssueCode code = RuntimeSaveMetadataChunkIssueCode::MalformedPayload;
};

struct RuntimeSaveMetadataDecodeResult {
	bool decoded = false;
	RuntimeSaveMetadata metadata;
	RuntimeSaveMetadataValidationResult validation;
	std::vector<RuntimeSaveMetadataChunkIssue> issues;
};

[[nodiscard]] RuntimeSaveChunkId runtimeSaveMetadataChunkId();

class RuntimeSaveMetadataChunkEncoder {
public:
	[[nodiscard]] RuntimeSaveChunk encode(const RuntimeSaveMetadata &metadata) const;
};

class RuntimeSaveMetadataChunkDecoder {
public:
	[[nodiscard]] RuntimeSaveMetadataDecodeResult decode(const RuntimeSaveChunk &chunk) const;
};

} // namespace iggy::runtime
