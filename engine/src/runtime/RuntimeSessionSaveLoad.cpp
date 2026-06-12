#include "runtime/RuntimeSessionSaveLoad.hpp"

namespace iggy::runtime {
namespace {

[[nodiscard]] bool hasSnapshotValidationFailure(const RuntimeSessionSnapshotDecodeResult &decode)
{
	for (const RuntimeSessionSnapshotChunkIssue &issue : decode.issues) {
		if (issue.code == RuntimeSessionSnapshotChunkIssueCode::SnapshotValidationFailed)
			return true;
	}
	return false;
}

} // namespace

RuntimeSessionSaveResult RuntimeSessionSaver::save(
	const RuntimeSessionState &session,
	const std::filesystem::path &path) const
{
	RuntimeSessionSaveResult result;
	result.snapshot = RuntimeSessionSnapshotBuilder {}.capture(session);
	result.validation = RuntimeSessionSnapshotValidator {}.validate(result.snapshot);
	if (!result.validation.valid) {
		result.status = RuntimeSessionSaveStatus::SnapshotInvalid;
		return result;
	}

	result.archive = RuntimeSessionSnapshotChunkEncoder {}.encode(result.snapshot);
	result.archiveEncode = RuntimeSaveChunkArchiveEncoder {}.encode(result.archive);
	if (!result.archiveEncode.encoded) {
		result.status = RuntimeSessionSaveStatus::ArchiveEncodeFailed;
		return result;
	}

	result.envelopeEncode = RuntimeSaveFileEnvelopeEncoder {}.encodePayload(result.archiveEncode.bytes);
	if (!result.envelopeEncode.encoded) {
		result.status = RuntimeSessionSaveStatus::EnvelopeEncodeFailed;
		return result;
	}

	result.fileWrite = RuntimeSaveFileWriter {}.writeBytes(path, result.envelopeEncode.bytes);
	if (result.fileWrite.status != RuntimeSaveFileIOStatus::Ok) {
		result.status = RuntimeSessionSaveStatus::FileWriteFailed;
		return result;
	}

	result.status = RuntimeSessionSaveStatus::Saved;
	return result;
}

RuntimeSessionLoadResult RuntimeSessionLoader::load(
	const std::filesystem::path &path,
	const RuntimeSessionSnapshotRestoreConfig &restoreConfig) const
{
	RuntimeSessionLoadResult result;
	result.fileRead = RuntimeSaveFileReader {}.readBytes(path);
	if (result.fileRead.status != RuntimeSaveFileIOStatus::Ok) {
		result.status = RuntimeSessionLoadStatus::FileReadFailed;
		return result;
	}

	result.envelopeDecode = RuntimeSaveFileEnvelopeDecoder {}.decode(result.fileRead.bytes);
	if (!result.envelopeDecode.decoded) {
		result.status = RuntimeSessionLoadStatus::EnvelopeDecodeFailed;
		return result;
	}

	result.archiveDecode = RuntimeSaveChunkArchiveDecoder {}.decode(result.envelopeDecode.envelope.payload);
	if (!result.archiveDecode.decoded) {
		result.status = RuntimeSessionLoadStatus::ArchiveDecodeFailed;
		return result;
	}

	result.snapshotDecode = RuntimeSessionSnapshotChunkDecoder {}.decode(result.archiveDecode.archive);
	result.validation = result.snapshotDecode.validation;
	if (!result.snapshotDecode.decoded) {
		result.status = hasSnapshotValidationFailure(result.snapshotDecode) ? RuntimeSessionLoadStatus::SnapshotInvalid : RuntimeSessionLoadStatus::SnapshotDecodeFailed;
		return result;
	}

	result.restore = RuntimeSessionSnapshotRestorer {}.restore(result.snapshotDecode.snapshot, restoreConfig);
	if (!result.restore.restored) {
		result.status = RuntimeSessionLoadStatus::RestoreFailed;
		return result;
	}

	result.session = result.restore.session;
	result.status = RuntimeSessionLoadStatus::Loaded;
	return result;
}

} // namespace iggy::runtime
