#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplaySnapshot.hpp"
#include "runtime/RuntimeGameplaySnapshotValidator.hpp"
#include "runtime/RuntimeSaveChunkArchive.hpp"
#include "runtime/RuntimeSessionSnapshotChunkCodec.hpp"

namespace iggy::runtime {

enum class RuntimeGameplaySnapshotChunkIssueCode {
	InvalidArchive,
	MissingRequiredChunk,
	DuplicateSingletonChunk,
	UnsupportedChunkVersion,
	MalformedChunkPayload,
	InvalidEnumValue,
	NestedSessionDecodeFailed,
	SnapshotValidationFailed,
};

struct RuntimeGameplaySnapshotChunkIssue {
	RuntimeGameplaySnapshotChunkIssueCode code = RuntimeGameplaySnapshotChunkIssueCode::InvalidArchive;
	RuntimeSaveChunkId chunkId;
	std::size_t chunkIndex = 0;
};

struct RuntimeGameplaySnapshotEncodeResult {
	RuntimeSaveChunkArchive archive;
};

struct RuntimeGameplaySnapshotDecodeResult {
	bool decoded = false;
	RuntimeGameplaySnapshot snapshot;
	std::vector<RuntimeGameplaySnapshotChunkIssue> issues;
	RuntimeSessionSnapshotDecodeResult session;
	RuntimeGameplaySnapshotValidationResult validation;
};

class RuntimeGameplaySnapshotChunkEncoder {
public:
	[[nodiscard]] RuntimeGameplaySnapshotEncodeResult encode(const RuntimeGameplaySnapshot &snapshot) const;
};

class RuntimeGameplaySnapshotChunkDecoder {
public:
	[[nodiscard]] RuntimeGameplaySnapshotDecodeResult decode(const RuntimeSaveChunkArchive &archive) const;
};

} // namespace iggy::runtime
