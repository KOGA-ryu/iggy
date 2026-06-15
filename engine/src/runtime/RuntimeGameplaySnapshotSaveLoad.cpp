#include "runtime/RuntimeGameplaySnapshotSaveLoad.hpp"

namespace iggy::runtime {
namespace {

[[nodiscard]] RuntimeGameplaySnapshotRestoreConfig defaultRestoreConfig()
{
	RuntimeGameplaySnapshotRestoreConfig config;
	config.session.buildConfig.buildRenderCache = false;
	return config;
}

[[nodiscard]] bool hasSnapshotValidationFailure(const RuntimeGameplaySnapshotDecodeResult &decode)
{
	for (const RuntimeGameplaySnapshotChunkIssue &issue : decode.issues) {
		if (issue.code == RuntimeGameplaySnapshotChunkIssueCode::SnapshotValidationFailed)
			return true;
	}
	return false;
}

} // namespace

RuntimeGameplaySnapshotSaveResult RuntimeGameplaySnapshotSaver::save(
	const std::filesystem::path &path,
	const RuntimeGameplaySnapshot &snapshot) const
{
	RuntimeGameplaySnapshotSaveResult result;
	result.snapshot = snapshot;
	result.validation = RuntimeGameplaySnapshotValidator {}.validate(snapshot);
	if (!result.validation.valid) {
		result.status = RuntimeGameplaySnapshotSaveStatus::SnapshotInvalid;
		return result;
	}

	result.snapshotEncode = RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot);
	result.archiveEncode = RuntimeSaveChunkArchiveEncoder {}.encode(result.snapshotEncode.archive);
	if (!result.archiveEncode.encoded) {
		result.status = RuntimeGameplaySnapshotSaveStatus::ArchiveEncodeFailed;
		return result;
	}

	result.envelopeEncode = RuntimeSaveFileEnvelopeEncoder {}.encodePayload(result.archiveEncode.bytes);
	if (!result.envelopeEncode.encoded) {
		result.status = RuntimeGameplaySnapshotSaveStatus::EnvelopeEncodeFailed;
		return result;
	}

	result.fileWrite = RuntimeSaveFileWriter {}.writeBytes(path, result.envelopeEncode.bytes);
	if (result.fileWrite.status != RuntimeSaveFileIOStatus::Ok) {
		result.status = RuntimeGameplaySnapshotSaveStatus::FileWriteFailed;
		return result;
	}

	result.status = RuntimeGameplaySnapshotSaveStatus::Saved;
	return result;
}

RuntimeGameplaySnapshotLoadResult RuntimeGameplaySnapshotLoader::load(const std::filesystem::path &path) const
{
	return load(path, defaultRestoreConfig());
}

RuntimeGameplaySnapshotLoadResult RuntimeGameplaySnapshotLoader::load(
	const std::filesystem::path &path,
	const RuntimeGameplaySnapshotRestoreConfig &restoreConfig) const
{
	RuntimeGameplaySnapshotLoadResult result;
	result.path = path;
	result.fileRead = RuntimeSaveFileReader {}.readBytes(path);
	if (result.fileRead.status != RuntimeSaveFileIOStatus::Ok) {
		result.status = RuntimeGameplaySnapshotLoadStatus::FileReadFailed;
		return result;
	}

	result.envelopeDecode = RuntimeSaveFileEnvelopeDecoder {}.decode(result.fileRead.bytes);
	if (!result.envelopeDecode.decoded) {
		result.status = RuntimeGameplaySnapshotLoadStatus::EnvelopeDecodeFailed;
		return result;
	}

	result.archiveDecode = RuntimeSaveChunkArchiveDecoder {}.decode(result.envelopeDecode.envelope.payload);
	if (!result.archiveDecode.decoded) {
		result.status = RuntimeGameplaySnapshotLoadStatus::ArchiveDecodeFailed;
		return result;
	}

	result.snapshotDecode = RuntimeGameplaySnapshotChunkDecoder {}.decode(result.archiveDecode.archive);
	result.validation = result.snapshotDecode.validation;
	if (!result.snapshotDecode.decoded) {
		result.status = hasSnapshotValidationFailure(result.snapshotDecode)
			? RuntimeGameplaySnapshotLoadStatus::SnapshotInvalid
			: RuntimeGameplaySnapshotLoadStatus::SnapshotDecodeFailed;
		return result;
	}

	result.validation = RuntimeGameplaySnapshotValidator {}.validate(result.snapshotDecode.snapshot);
	if (!result.validation.valid) {
		result.status = RuntimeGameplaySnapshotLoadStatus::SnapshotInvalid;
		return result;
	}

	result.restore = RuntimeGameplaySnapshotRestorer {}.restore(result.snapshotDecode.snapshot, restoreConfig);
	if (!result.restore.restored) {
		result.status = RuntimeGameplaySnapshotLoadStatus::RestoreFailed;
		return result;
	}

	result.state = result.restore.state;
	result.status = RuntimeGameplaySnapshotLoadStatus::Loaded;
	return result;
}

} // namespace iggy::runtime
