#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeSaveChunkArchive.hpp"
#include "runtime/RuntimeSessionSnapshot.hpp"
#include "runtime/RuntimeSessionSnapshotValidator.hpp"

namespace iggy::runtime {

enum class RuntimeSessionSnapshotChunkIssueCode {
	InvalidArchive,
	MissingRequiredChunk,
	DuplicateSingletonChunk,
	UnsupportedChunkVersion,
	MalformedChunkPayload,
	InvalidEnumValue,
	SnapshotValidationFailed,
};

struct RuntimeSessionSnapshotChunkIssue {
	RuntimeSessionSnapshotChunkIssueCode code = RuntimeSessionSnapshotChunkIssueCode::InvalidArchive;
	RuntimeSaveChunkId chunkId;
	std::size_t chunkIndex = 0;
};

struct RuntimeSessionSnapshotDecodeResult {
	bool decoded = false;
	RuntimeSessionSnapshot snapshot;
	std::vector<RuntimeSessionSnapshotChunkIssue> issues;
	RuntimeSessionSnapshotValidationResult validation;
};

class RuntimeSessionSnapshotChunkEncoder {
public:
	[[nodiscard]] RuntimeSaveChunkArchive encode(const RuntimeSessionSnapshot &snapshot) const;
};

class RuntimeSessionSnapshotChunkDecoder {
public:
	[[nodiscard]] RuntimeSessionSnapshotDecodeResult decode(const RuntimeSaveChunkArchive &archive) const;
};

} // namespace iggy::runtime
