#include "runtime/save/SaveFileStore.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string_view>

#if defined(_WIN32)
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace iggy3d {
namespace {

constexpr std::string_view kSaveFileExtension = ".iggy3d.save";

// Flush a file's contents to the filesystem so a committed save survives a
// process or OS crash. The durable write already guards against truncation via
// temp-write + readback; this closes the remaining gap where the bytes live
// only in the OS page cache. Best-effort by platform; returns false only when
// the path cannot be opened or the sync call fails so callers can treat a
// failed sync as a failed durable write rather than silently overstating
// durability. (Power-loss-proof flushing on macOS would additionally need
// F_FULLFSYNC; fsync here protects against process/OS crash.)
bool syncFileToDisk(const std::filesystem::path& path) {
#if defined(_WIN32)
  FILE* file = nullptr;
  if (_wfopen_s(&file, path.wstring().c_str(), L"rb") != 0 || file == nullptr) {
    return false;
  }
  const int fd = _fileno(file);
  const bool ok = fd >= 0 && _commit(fd) == 0;
  std::fclose(file);
  return ok;
#else
  const int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    return false;
  }
  const bool ok = ::fsync(fd) == 0;
  ::close(fd);
  return ok;
#endif
}

// Flush a directory entry so a freshly renamed save name is itself durable.
// POSIX-only; on Windows the rename is made durable by the file flush above.
bool syncDirectoryToDisk(const std::filesystem::path& directory) {
#if defined(_WIN32)
  (void)directory;
  return true;
#else
  const int fd = ::open(directory.c_str(), O_RDONLY);
  if (fd < 0) {
    return false;
  }
  const bool ok = ::fsync(fd) == 0;
  ::close(fd);
  return ok;
#endif
}

bool hasSaveFileExtension(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  return filename.size() > kSaveFileExtension.size() &&
         filename.ends_with(kSaveFileExtension);
}

std::string makeSaveId(std::size_t index) {
  std::ostringstream output;
  output << "save_";
  output.width(3);
  output.fill('0');
  output << index;
  return output.str();
}

SaveFileRecord recordFromEnvelope(const std::filesystem::path& path,
                                  const SaveEnvelope& envelope) {
  SaveFileRecord record;
  record.id = saveFileIdFromPath(path);
  record.path = path;
  record.packageId = envelope.metadata.packageId;
  record.scenarioId = envelope.metadata.scenarioId;
  record.currentTick = envelope.session.currentTick;
  record.nextCommandId = envelope.session.nextCommandId;
  record.savedStateHash = envelope.metadata.savedStateHash;
  record.savedStateHashHex = envelope.metadata.savedStateHashHex;
  return record;
}

std::string readWholeFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

}  // namespace

bool isValidSaveFileId(std::string_view id) {
  if (id.empty()) {
    return false;
  }
  for (const char c : id) {
    const bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!valid) {
      return false;
    }
  }
  return true;
}

std::filesystem::path saveFilePathForId(const std::filesystem::path& root,
                                        std::string_view id) {
  return root / (std::string(id) + std::string(kSaveFileExtension));
}

std::filesystem::path saveFileTempPathForId(const std::filesystem::path& root,
                                            std::string_view id,
                                            std::string_view attemptToken) {
  return root / (std::string(id) + std::string(kSaveFileExtension) + ".tmp_" +
                 std::string(attemptToken));
}

std::filesystem::path saveSnapshotPathForId(const std::filesystem::path& root,
                                            std::string_view id) {
  return root / (std::string(id) + ".snapshot.png");
}

std::filesystem::path saveSnapshotPathForFilePath(
    const std::filesystem::path& path) {
  return saveSnapshotPathForId(path.parent_path(), saveFileIdFromPath(path));
}

std::string saveFileIdFromPath(const std::filesystem::path& path) {
  std::string filename = path.filename().string();
  // branch-gate: BG-1219
  if (filename.size() > kSaveFileExtension.size() &&
      filename.ends_with(kSaveFileExtension)) {
    filename.erase(filename.size() - kSaveFileExtension.size());
  }
  // branch-gate: BG-1219
  return filename.empty() ? "save" : filename;
}

std::filesystem::path deletedSaveDirectory(const std::filesystem::path& root) {
  return root / "deleted";
}

std::filesystem::path deletedSaveFilePathForId(const std::filesystem::path& root,
                                               std::string_view id) {
  return deletedSaveDirectory(root) /
         (std::string(id) + std::string(kSaveFileExtension));
}

std::filesystem::path deletedSaveSnapshotPathForId(
    const std::filesystem::path& root,
    std::string_view id) {
  return deletedSaveDirectory(root) / (std::string(id) + ".snapshot.png");
}

SaveFileDurableWritePlan planDurableSaveFileWrite(
    const std::filesystem::path& root,
    std::string_view id,
    std::string_view attemptToken) {
  SaveFileDurableWritePlan plan;
  if (!isValidSaveFileId(id)) {
    plan.reason = "durable_save_invalid_id";
    return plan;
  }
  if (!isValidSaveFileId(attemptToken)) {
    plan.reason = "durable_save_invalid_attempt_token";
    return plan;
  }

  plan.ok = true;
  plan.reason = "durable_save_plan_ready";
  plan.paths.root = root;
  plan.paths.id = std::string(id);
  plan.paths.attemptToken = std::string(attemptToken);
  plan.paths.finalPath = saveFilePathForId(root, id);
  plan.paths.tempPath = saveFileTempPathForId(root, id, attemptToken);
  plan.paths.snapshotPath = saveSnapshotPathForId(root, id);
  return plan;
}

SaveFileTempWriteResult writeDurableSaveTempFile(
    const SaveFileTempWriteRequest& request) {
  SaveFileTempWriteResult result;
  result.paths = request.plan.paths;
  if (!request.plan.ok) {
    result.reason = request.plan.reason;
    return result;
  }
  if (request.encodedText.empty()) {
    result.reason = "durable_save_empty_payload";
    return result;
  }

  std::error_code error;
  std::filesystem::create_directories(result.paths.root, error);
  if (error) {
    result.reason = "durable_save_root_create_failed";
    return result;
  }
  result.rootCreated = true;

  std::ofstream output(result.paths.tempPath);
  if (!output) {
    result.reason = "durable_save_temp_write_failed";
    return result;
  }
  output << request.encodedText;
  if (!output) {
    result.reason = "durable_save_temp_write_failed";
    return result;
  }
  result.encodedBytes =
      static_cast<std::uint64_t>(request.encodedText.size());
  result.tempWritten = true;

  output.close();
  if (!output) {
    result.reason = "durable_save_temp_write_failed";
    return result;
  }
  result.tempClosed = true;

  result.readBackText = readWholeFile(result.paths.tempPath);
  if (result.readBackText.empty()) {
    result.reason = "durable_save_temp_read_failed";
    return result;
  }
  result.readBackBytes =
      static_cast<std::uint64_t>(result.readBackText.size());
  result.tempReadBack = true;
  if (result.readBackText != request.encodedText) {
    result.reason = "durable_save_temp_mismatch";
    return result;
  }

  // Flush the validated temp file to disk before it is committed so the bytes
  // are durable, not merely in the page cache, when the atomic rename happens.
  if (!syncFileToDisk(result.paths.tempPath)) {
    result.reason = "durable_save_temp_sync_failed";
    return result;
  }

  result.ok = true;
  result.reason = "durable_save_temp_written";
  return result;
}

SaveFileTempValidationResult validateDurableSaveTempFile(
    const SaveFileTempWriteResult& tempWrite) {
  SaveFileTempValidationResult result;
  result.paths = tempWrite.paths;
  if (!tempWrite.ok) {
    result.reason = tempWrite.reason;
    return result;
  }

  const std::string encodedText = readWholeFile(tempWrite.paths.tempPath);
  if (encodedText.empty()) {
    result.reason = "durable_save_temp_read_failed";
    return result;
  }
  result.tempRead = true;
  result.encodedBytes = static_cast<std::uint64_t>(encodedText.size());

  const SaveDecodeResult decoded = decodeSaveEnvelope(encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status != SaveCodecStatus::Ok) {
    result.reason = "durable_save_temp_decode_failed";
    return result;
  }
  result.tempDecoded = true;
  result.tempValidated = true;
  result.ok = true;
  result.reason = "durable_save_temp_validated";
  result.savedStateHash = decoded.envelope.metadata.savedStateHash;
  result.savedStateHashHex = decoded.envelope.metadata.savedStateHashHex;
  result.packageId = decoded.envelope.metadata.packageId;
  result.scenarioId = decoded.envelope.metadata.scenarioId;
  return result;
}

SaveFileFinalCommitResult commitDurableSaveTempFile(
    const SaveFileTempValidationResult& validation) {
  SaveFileFinalCommitResult result;
  result.paths = validation.paths;
  if (!validation.ok) {
    result.reason = validation.reason;
    return result;
  }
  result.tempValidated = true;

  std::error_code error;
  result.previousExisted = std::filesystem::exists(result.paths.finalPath, error);
  result.previousPreserved = !result.previousExisted;
  std::filesystem::rename(result.paths.tempPath, result.paths.finalPath, error);
  if (error) {
    std::error_code existsError;
    result.previousPreserved =
        !result.previousExisted ||
        std::filesystem::exists(result.paths.finalPath, existsError);
    result.reason = "durable_save_atomic_rename_failed";
    return result;
  }
  result.committed = true;

  // Flush the directory entry so the committed name survives a crash too. This
  // is best-effort: the file contents are already durable from the temp sync,
  // so a directory-sync failure does not undo a successful commit.
  (void)syncDirectoryToDisk(result.paths.root);

  const std::string encodedText = readWholeFile(result.paths.finalPath);
  if (encodedText.empty()) {
    result.reason = "durable_save_final_read_failed";
    return result;
  }
  result.finalRead = true;
  result.encodedBytes = static_cast<std::uint64_t>(encodedText.size());

  const SaveDecodeResult decoded = decodeSaveEnvelope(encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status != SaveCodecStatus::Ok) {
    result.reason = "durable_save_final_decode_failed";
    return result;
  }

  result.finalDecoded = true;
  result.finalValidated = true;
  result.ok = true;
  result.reason = "durable_save_final_validated";
  result.savedStateHash = decoded.envelope.metadata.savedStateHash;
  result.savedStateHashHex = decoded.envelope.metadata.savedStateHashHex;
  result.packageId = decoded.envelope.metadata.packageId;
  result.scenarioId = decoded.envelope.metadata.scenarioId;
  return result;
}

SaveFileSoftDeletePlan planSoftDeleteSaveFile(const std::filesystem::path& root,
                                              std::string_view id) {
  SaveFileSoftDeletePlan plan;
  if (!isValidSaveFileId(id)) {
    plan.reason = "soft_delete_invalid_id";
    return plan;
  }

  plan.ok = true;
  plan.reason = "soft_delete_plan_ready";
  plan.paths.root = root;
  plan.paths.id = std::string(id);
  plan.paths.activeSavePath = saveFilePathForId(root, id);
  plan.paths.activeSnapshotPath = saveSnapshotPathForId(root, id);
  plan.paths.deletedSavePath = deletedSaveFilePathForId(root, id);
  plan.paths.deletedSnapshotPath = deletedSaveSnapshotPathForId(root, id);
  return plan;
}

SaveFileRecoverPlan planRecoverDeletedSaveFile(const std::filesystem::path& root,
                                               std::string_view id) {
  SaveFileRecoverPlan plan;
  if (!isValidSaveFileId(id)) {
    plan.reason = "recover_save_invalid_id";
    return plan;
  }

  plan.ok = true;
  plan.reason = "recover_save_plan_ready";
  plan.paths.root = root;
  plan.paths.id = std::string(id);
  plan.paths.activeSavePath = saveFilePathForId(root, id);
  plan.paths.activeSnapshotPath = saveSnapshotPathForId(root, id);
  plan.paths.deletedSavePath = deletedSaveFilePathForId(root, id);
  plan.paths.deletedSnapshotPath = deletedSaveSnapshotPathForId(root, id);
  return plan;
}

SaveFileSoftDeleteResult softDeleteSaveFile(const SaveFileSoftDeletePlan& plan) {
  SaveFileSoftDeleteResult result;
  result.paths = plan.paths;
  if (!plan.ok) {
    result.reason = plan.reason;
    return result;
  }

  std::error_code error;
  if (!std::filesystem::exists(plan.paths.activeSavePath, error)) {
    result.reason = "soft_delete_source_missing";
    return result;
  }
  if (std::filesystem::exists(plan.paths.deletedSavePath, error)) {
    result.targetExisted = true;
    result.reason = "soft_delete_target_exists";
    return result;
  }

  const bool snapshotPresent =
      std::filesystem::exists(plan.paths.activeSnapshotPath, error);
  if (snapshotPresent &&
      std::filesystem::exists(plan.paths.deletedSnapshotPath, error)) {
    result.snapshotTargetExisted = true;
    result.reason = "soft_delete_snapshot_target_exists";
    return result;
  }

  std::filesystem::create_directories(plan.paths.deletedSavePath.parent_path(), error);
  if (error) {
    result.reason = "soft_delete_deleted_dir_create_failed";
    return result;
  }

  std::filesystem::rename(plan.paths.activeSavePath, plan.paths.deletedSavePath, error);
  if (error) {
    result.reason = "soft_delete_move_failed";
    return result;
  }
  result.saveMoved = true;

  if (snapshotPresent) {
    std::filesystem::rename(plan.paths.activeSnapshotPath,
                            plan.paths.deletedSnapshotPath,
                            error);
    if (error) {
      result.reason = "soft_delete_snapshot_move_failed";
      return result;
    }
    result.snapshotMoved = true;
  } else {
    result.snapshotMissing = true;
  }

  result.ok = true;
  result.reason = "soft_delete_moved";
  return result;
}

SaveFileRecoverResult recoverDeletedSaveFile(const SaveFileRecoverPlan& plan) {
  SaveFileRecoverResult result;
  result.paths = plan.paths;
  if (!plan.ok) {
    result.reason = plan.reason;
    return result;
  }

  std::error_code error;
  if (!std::filesystem::exists(plan.paths.deletedSavePath, error)) {
    result.reason = "recover_save_source_missing";
    return result;
  }
  if (std::filesystem::exists(plan.paths.activeSavePath, error)) {
    result.targetExisted = true;
    result.reason = "recover_save_target_exists";
    return result;
  }

  const bool snapshotPresent =
      std::filesystem::exists(plan.paths.deletedSnapshotPath, error);
  if (snapshotPresent &&
      std::filesystem::exists(plan.paths.activeSnapshotPath, error)) {
    result.snapshotTargetExisted = true;
    result.reason = "recover_save_snapshot_target_exists";
    return result;
  }

  std::filesystem::create_directories(plan.paths.activeSavePath.parent_path(), error);
  if (error) {
    result.reason = "recover_save_root_create_failed";
    return result;
  }

  std::filesystem::rename(plan.paths.deletedSavePath, plan.paths.activeSavePath, error);
  if (error) {
    result.reason = "recover_save_move_failed";
    return result;
  }
  result.saveRecovered = true;

  if (snapshotPresent) {
    std::filesystem::rename(plan.paths.deletedSnapshotPath,
                            plan.paths.activeSnapshotPath,
                            error);
    if (error) {
      result.reason = "recover_save_snapshot_move_failed";
      return result;
    }
    result.snapshotRecovered = true;
  } else {
    result.snapshotMissing = true;
  }

  result.ok = true;
  result.reason = "recover_save_moved";
  return result;
}

std::vector<std::filesystem::path> listSaveFilePaths(
    const std::filesystem::path& root) {
  std::vector<std::filesystem::path> paths;
  std::error_code error;
  if (!std::filesystem::exists(root, error) || !std::filesystem::is_directory(root, error)) {
    return paths;
  }
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(root, error)) {
    if (error || !entry.is_regular_file(error) || !hasSaveFileExtension(entry.path())) {
      continue;
    }
    paths.push_back(entry.path());
  }
  std::sort(paths.begin(), paths.end());
  return paths;
}

std::vector<SaveFileRecord> listSaveFiles(const std::filesystem::path& root) {
  std::vector<SaveFileRecord> records;
  for (const std::filesystem::path& path : listSaveFilePaths(root)) {
    const SaveFileReadResult read = readSaveFile(path);
    if (read.ok) {
      records.push_back(read.record);
    }
  }
  std::sort(records.begin(), records.end(), [](const SaveFileRecord& lhs,
                                               const SaveFileRecord& rhs) {
    return lhs.id < rhs.id;
  });
  return records;
}

SaveFileWriteResult writeSessionSaveFile(const SaveFileWriteRequest& request) {
  SaveFileWriteResult result;
  if (request.state == nullptr) {
    result.reason = "save_state_missing";
    return result;
  }

  SaveStateResult saved = saveSessionState(*request.state);
  result.saveStatus = saved.status;
  if (saved.status != SaveLoadStatus::Ok) {
    result.reason = "save_encode_failed";
    return result;
  }
  if (request.authoredRoom != nullptr) {
    saved.envelope.authoredRoom = *request.authoredRoom;
  }
  const SaveEncodeResult encoded = encodeSaveEnvelope(saved.envelope);
  saved.codecStatus = encoded.status;
  result.codecStatus = encoded.status;
  if (encoded.status != SaveCodecStatus::Ok) {
    result.saveStatus = SaveLoadStatus::EncodeFailed;
    result.reason = "save_encode_failed";
    return result;
  }
  saved.encodedSaveText = encoded.encodedText;
  saved.savedStateHash = encoded.savedStateHash;

  std::error_code error;
  std::filesystem::create_directories(request.root, error);
  if (error) {
    result.reason = "save_root_create_failed";
    return result;
  }

  std::string id = isValidSaveFileId(request.idHint)
                       ? request.idHint
                       : makeSaveId(listSaveFiles(request.root).size() + 1U);
  std::filesystem::path path = saveFilePathForId(request.root, id);
  if (!isValidSaveFileId(request.idHint)) {
    std::size_t index = listSaveFiles(request.root).size() + 1U;
    while (std::filesystem::exists(path, error)) {
      ++index;
      id = makeSaveId(index);
      path = saveFilePathForId(request.root, id);
    }
  }

  std::ofstream output(path);
  if (!output) {
    result.reason = "save_file_write_failed";
    return result;
  }
  output << saved.encodedSaveText;
  if (!output) {
    result.reason = "save_file_write_failed";
    return result;
  }

  result.ok = true;
  result.reason = "save_file_written";
  result.record = recordFromEnvelope(path, saved.envelope);
  result.encodedBytes = saved.encodedSaveText.size();
  return result;
}

SaveFileDurableWriteResult writeSessionSaveFileDurably(
    const SaveFileDurableWriteRequest& request) {
  SaveFileDurableWriteResult result;
  if (request.state == nullptr) {
    result.reason = "save_state_missing";
    return result;
  }

  SaveStateResult saved = saveSessionState(*request.state);
  result.saveStatus = saved.status;
  if (saved.status != SaveLoadStatus::Ok) {
    result.reason = "save_encode_failed";
    return result;
  }
  result.envelopeBuilt = true;
  if (request.authoredRoom != nullptr) {
    saved.envelope.authoredRoom = *request.authoredRoom;
  }

  std::error_code error;
  std::string id = isValidSaveFileId(request.idHint)
                       ? request.idHint
                       : makeSaveId(listSaveFiles(request.root).size() + 1U);
  std::filesystem::path path = saveFilePathForId(request.root, id);
  if (!isValidSaveFileId(request.idHint)) {
    std::size_t index = listSaveFiles(request.root).size() + 1U;
    while (std::filesystem::exists(path, error)) {
      ++index;
      id = makeSaveId(index);
      path = saveFilePathForId(request.root, id);
    }
  }

  saved.envelope.metadata.saveId = id;
  saved.envelope.metadata.worldId = request.productMetadata.worldId;
  saved.envelope.metadata.worldTitle = request.productMetadata.worldTitle;
  saved.envelope.metadata.saveTitle = request.productMetadata.saveTitle;
  saved.envelope.metadata.saveType = request.productMetadata.saveType;
  saved.envelope.metadata.createdAtUtc = request.productMetadata.createdAtUtc;
  saved.envelope.metadata.savedAtUtc = request.productMetadata.savedAtUtc;

  const SaveEncodeResult encoded = encodeSaveEnvelope(saved.envelope);
  saved.codecStatus = encoded.status;
  result.codecStatus = encoded.status;
  if (encoded.status != SaveCodecStatus::Ok) {
    result.saveStatus = SaveLoadStatus::EncodeFailed;
    result.reason = "save_encode_failed";
    return result;
  }
  saved.encodedSaveText = encoded.encodedText;
  saved.savedStateHash = encoded.savedStateHash;
  result.encoded = true;

  const SaveFileDurableWritePlan plan =
      planDurableSaveFileWrite(request.root, id, request.attemptToken);
  result.paths = plan.paths;
  if (!plan.ok) {
    result.reason = plan.reason;
    return result;
  }

  const SaveFileTempWriteResult tempWrite =
      writeDurableSaveTempFile({plan, saved.encodedSaveText});
  result.paths = tempWrite.paths;
  result.tempWritten = tempWrite.tempWritten;
  result.encodedBytes = tempWrite.encodedBytes;
  if (!tempWrite.ok) {
    result.reason = tempWrite.reason;
    return result;
  }

  const SaveFileTempValidationResult tempValidation =
      validateDurableSaveTempFile(tempWrite);
  result.tempValidated = tempValidation.tempValidated;
  result.codecStatus = tempValidation.codecStatus;
  result.encodedBytes = tempValidation.encodedBytes;
  if (!tempValidation.ok) {
    result.reason = tempValidation.reason;
    return result;
  }

  const SaveFileFinalCommitResult finalCommit =
      commitDurableSaveTempFile(tempValidation);
  result.paths = finalCommit.paths;
  result.codecStatus = finalCommit.codecStatus;
  result.encodedBytes = finalCommit.encodedBytes;
  result.previousExisted = finalCommit.previousExisted;
  result.previousPreserved = finalCommit.previousPreserved;
  result.committed = finalCommit.committed;
  result.finalValidated = finalCommit.finalValidated;
  if (!finalCommit.ok) {
    result.reason = finalCommit.reason;
    return result;
  }

  result.ok = true;
  result.reason = "durable_save_file_written";
  result.record = recordFromEnvelope(finalCommit.paths.finalPath, saved.envelope);
  return result;
}

SaveFileReadResult readSaveFile(const std::filesystem::path& path) {
  SaveFileReadResult result;
  result.encodedText = readWholeFile(path);
  if (result.encodedText.empty()) {
    result.reason = "save_file_read_failed";
    return result;
  }
  const SaveDecodeResult decoded = decodeSaveEnvelope(result.encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status != SaveCodecStatus::Ok) {
    result.reason = "save_file_decode_failed";
    return result;
  }
  result.ok = true;
  result.reason = "save_file_read";
  result.record = recordFromEnvelope(path, decoded.envelope);
  return result;
}

bool deleteSaveFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::remove(path, error) && !error;
}

std::filesystem::path defaultSaveFileRoot() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".iggy3d" / "saves";
  }
  return std::filesystem::path(".iggy3d") / "saves";
}

}  // namespace iggy3d
