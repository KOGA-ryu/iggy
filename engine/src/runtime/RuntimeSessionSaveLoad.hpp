#pragma once

#include <filesystem>

#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSaveMetadata.hpp"
#include "runtime/RuntimeSessionSnapshot.hpp"
#include "runtime/RuntimeSessionSnapshotChunkCodec.hpp"
#include "runtime/RuntimeSessionSnapshotValidator.hpp"

namespace iggy::runtime {

enum class RuntimeSessionSaveStatus {
	Saved,
	MetadataInvalid,
	SnapshotInvalid,
	ArchiveEncodeFailed,
	EnvelopeEncodeFailed,
	FileWriteFailed,
};

enum class RuntimeSessionLoadStatus {
	Loaded,
	FileReadFailed,
	EnvelopeDecodeFailed,
	ArchiveDecodeFailed,
	MetadataDecodeFailed,
	SnapshotDecodeFailed,
	SnapshotInvalid,
	RestoreFailed,
};

struct RuntimeSessionSaveResult {
	RuntimeSessionSaveStatus status = RuntimeSessionSaveStatus::SnapshotInvalid;
	RuntimeSessionSnapshot snapshot;
	RuntimeSessionSnapshotValidationResult validation;
	RuntimeSaveChunkArchive archive;
	RuntimeSaveChunkArchiveEncodeResult archiveEncode;
	RuntimeSaveFileEnvelopeEncodeResult envelopeEncode;
	RuntimeSaveFileWriteResult fileWrite;
	bool hasMetadata = false;
	RuntimeSaveMetadata metadata;
	RuntimeSaveMetadataValidationResult metadataValidation;
};

struct RuntimeSessionLoadResult {
	RuntimeSessionLoadStatus status = RuntimeSessionLoadStatus::FileReadFailed;
	RuntimeSessionState session;
	RuntimeSaveFileReadResult fileRead;
	RuntimeSaveFileEnvelopeDecodeResult envelopeDecode;
	RuntimeSaveChunkArchiveDecodeResult archiveDecode;
	RuntimeSessionSnapshotDecodeResult snapshotDecode;
	RuntimeSessionSnapshotValidationResult validation;
	RuntimeSessionSnapshotRestoreResult restore;
	bool hasMetadata = false;
	RuntimeSaveMetadata metadata;
	RuntimeSaveMetadataDecodeResult metadataDecode;
};

class RuntimeSessionSaver {
public:
	[[nodiscard]] RuntimeSessionSaveResult save(
		const RuntimeSessionState &session,
		const std::filesystem::path &path) const;

	[[nodiscard]] RuntimeSessionSaveResult save(
		const RuntimeSessionState &session,
		const std::filesystem::path &path,
		const RuntimeSaveMetadata &metadata) const;
};

class RuntimeSessionLoader {
public:
	[[nodiscard]] RuntimeSessionLoadResult load(
		const std::filesystem::path &path,
		const RuntimeSessionSnapshotRestoreConfig &restoreConfig) const;
};

} // namespace iggy::runtime
