#pragma once

#include <filesystem>

#include "runtime/RuntimeGameplaySnapshot.hpp"
#include "runtime/RuntimeGameplaySnapshotChunkCodec.hpp"
#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"

namespace iggy::runtime {

enum class RuntimeGameplaySnapshotSaveStatus {
	Saved,
	SnapshotInvalid,
	ArchiveEncodeFailed,
	EnvelopeEncodeFailed,
	FileWriteFailed,
};

enum class RuntimeGameplaySnapshotLoadStatus {
	Loaded,
	FileReadFailed,
	EnvelopeDecodeFailed,
	ArchiveDecodeFailed,
	SnapshotDecodeFailed,
	SnapshotInvalid,
	RestoreFailed,
};

struct RuntimeGameplaySnapshotSaveResult {
	RuntimeGameplaySnapshotSaveStatus status = RuntimeGameplaySnapshotSaveStatus::SnapshotInvalid;
	RuntimeGameplaySnapshot snapshot;
	RuntimeGameplaySnapshotValidationResult validation;
	RuntimeGameplaySnapshotEncodeResult snapshotEncode;
	RuntimeSaveChunkArchiveEncodeResult archiveEncode;
	RuntimeSaveFileEnvelopeEncodeResult envelopeEncode;
	RuntimeSaveFileWriteResult fileWrite;
};

struct RuntimeGameplaySnapshotLoadResult {
	RuntimeGameplaySnapshotLoadStatus status = RuntimeGameplaySnapshotLoadStatus::FileReadFailed;
	std::filesystem::path path;
	RuntimeSaveFileReadResult fileRead;
	RuntimeSaveFileEnvelopeDecodeResult envelopeDecode;
	RuntimeSaveChunkArchiveDecodeResult archiveDecode;
	RuntimeGameplaySnapshotDecodeResult snapshotDecode;
	RuntimeGameplaySnapshotValidationResult validation;
	RuntimeGameplaySnapshotRestoreResult restore;
	RuntimeGameplayState state;
};

class RuntimeGameplaySnapshotSaver {
public:
	[[nodiscard]] RuntimeGameplaySnapshotSaveResult save(
		const std::filesystem::path &path,
		const RuntimeGameplaySnapshot &snapshot) const;
};

class RuntimeGameplaySnapshotLoader {
public:
	[[nodiscard]] RuntimeGameplaySnapshotLoadResult load(const std::filesystem::path &path) const;

	[[nodiscard]] RuntimeGameplaySnapshotLoadResult load(
		const std::filesystem::path &path,
		const RuntimeGameplaySnapshotRestoreConfig &restoreConfig) const;
};

} // namespace iggy::runtime
