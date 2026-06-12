#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "runtime/RuntimeSaveChunkArchive.hpp"

namespace iggy::runtime {

enum class RuntimeSaveChunkArchiveCodecIssueCode {
	InvalidArchive,
	TruncatedInput,
	InvalidPayloadSize,
	TrailingBytes,
};

struct RuntimeSaveChunkArchiveCodecIssue {
	RuntimeSaveChunkArchiveCodecIssueCode code = RuntimeSaveChunkArchiveCodecIssueCode::InvalidArchive;
	std::size_t offset = 0;
	std::size_t chunkIndex = 0;
};

struct RuntimeSaveChunkArchiveEncodeResult {
	bool encoded = false;
	std::vector<std::uint8_t> bytes;
	RuntimeSaveChunkArchiveValidationResult validation;
};

struct RuntimeSaveChunkArchiveDecodeResult {
	bool decoded = false;
	RuntimeSaveChunkArchive archive;
	std::vector<RuntimeSaveChunkArchiveCodecIssue> issues;
	RuntimeSaveChunkArchiveValidationResult validation;
};

class RuntimeSaveChunkArchiveEncoder {
public:
	[[nodiscard]] RuntimeSaveChunkArchiveEncodeResult encode(const RuntimeSaveChunkArchive &archive) const;
};

class RuntimeSaveChunkArchiveDecoder {
public:
	[[nodiscard]] RuntimeSaveChunkArchiveDecodeResult decode(const std::vector<std::uint8_t> &bytes) const;
};

} // namespace iggy::runtime
