#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "runtime/RuntimeSaveChunkArchive.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"

namespace iggy::runtime {

enum class RuntimeSaveFormatVersionStatus {
	Supported,
	UnsupportedEnvelopeVersion,
	UnsupportedArchiveVersion,
	UnsupportedChunkVersion,
	UnknownChunk,
};

struct RuntimeSaveFormatVersionIssue {
	RuntimeSaveFormatVersionStatus status = RuntimeSaveFormatVersionStatus::Supported;
	RuntimeSaveChunkId chunkId;
	std::uint32_t version = 0;
	std::size_t chunkIndex = 0;
};

struct RuntimeSaveFormatVersionReport {
	bool supported = false;
	std::vector<RuntimeSaveFormatVersionIssue> issues;
};

class RuntimeSaveFormatVersionPolicy {
public:
	[[nodiscard]] bool supportsEnvelopeVersion(std::uint32_t version) const;
	[[nodiscard]] bool supportsArchiveVersion(std::uint32_t version) const;
	[[nodiscard]] bool supportsSnapshotChunkVersion(RuntimeSaveChunkId chunkId, std::uint32_t version) const;
	[[nodiscard]] RuntimeSaveFormatVersionReport inspect(
		const RuntimeSaveFileEnvelope &envelope,
		const RuntimeSaveChunkArchive &archive) const;
};

} // namespace iggy::runtime
