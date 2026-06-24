#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct SaveFileRecord {
  std::string id;
  std::filesystem::path path;
  std::string packageId;
  std::string scenarioId;
  std::uint64_t currentTick = 0;
  CommandId nextCommandId = kInvalidCommandId;
  StateHashValue savedStateHash = 0;
  std::string savedStateHashHex;
};

struct SaveFileWriteRequest {
  std::filesystem::path root;
  std::string idHint;
  const SessionState* state = nullptr;
  const SaveAuthoredRoomSection* authoredRoom = nullptr;
};

struct SaveFileWriteResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileRecord record;
  SaveLoadStatus saveStatus = SaveLoadStatus::InvalidSourceState;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  std::uint64_t encodedBytes = 0;
};

struct SaveFileReadResult {
  bool ok = false;
  std::string reason = "not_requested";
  std::string encodedText;
  SaveFileRecord record;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
};

struct SaveFileDurableWritePaths {
  std::filesystem::path root;
  std::string id;
  std::string attemptToken;
  std::filesystem::path finalPath;
  std::filesystem::path tempPath;
  std::filesystem::path snapshotPath;
};

struct SaveFileDurableWritePlan {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileDurableWritePaths paths;
};

struct SaveFileProductMetadata {
  std::string saveId;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

struct SaveFileDurableWriteRequest {
  std::filesystem::path root;
  std::string idHint;
  std::string attemptToken;
  const SessionState* state = nullptr;
  const SaveAuthoredRoomSection* authoredRoom = nullptr;
  SaveFileProductMetadata productMetadata;
};

struct SaveFileDurableWriteResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileRecord record;
  SaveFileDurableWritePaths paths;
  SaveLoadStatus saveStatus = SaveLoadStatus::InvalidSourceState;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  std::uint64_t encodedBytes = 0;
  bool envelopeBuilt = false;
  bool encoded = false;
  bool tempWritten = false;
  bool tempValidated = false;
  bool committed = false;
  bool finalValidated = false;
  bool previousExisted = false;
  bool previousPreserved = true;
};

struct SaveFileTempWriteRequest {
  SaveFileDurableWritePlan plan;
  std::string encodedText;
};

struct SaveFileTempWriteResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileDurableWritePaths paths;
  std::uint64_t encodedBytes = 0;
  std::uint64_t readBackBytes = 0;
  bool rootCreated = false;
  bool tempWritten = false;
  bool tempClosed = false;
  bool tempReadBack = false;
  std::string readBackText;
};

struct SaveFileTempValidationResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileDurableWritePaths paths;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  std::uint64_t encodedBytes = 0;
  std::uint64_t savedStateHash = 0;
  std::string savedStateHashHex;
  std::string packageId;
  std::string scenarioId;
  bool tempRead = false;
  bool tempDecoded = false;
  bool tempValidated = false;
};

struct SaveFileFinalCommitResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileDurableWritePaths paths;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  std::uint64_t encodedBytes = 0;
  std::uint64_t savedStateHash = 0;
  std::string savedStateHashHex;
  std::string packageId;
  std::string scenarioId;
  bool previousExisted = false;
  bool previousPreserved = true;
  bool tempValidated = false;
  bool committed = false;
  bool finalRead = false;
  bool finalDecoded = false;
  bool finalValidated = false;
};

struct SaveFileSoftDeletePaths {
  std::filesystem::path root;
  std::string id;
  std::filesystem::path activeSavePath;
  std::filesystem::path activeSnapshotPath;
  std::filesystem::path deletedSavePath;
  std::filesystem::path deletedSnapshotPath;
};

struct SaveFileSoftDeletePlan {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileSoftDeletePaths paths;
};

struct SaveFileRecoverPlan {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileSoftDeletePaths paths;
};

struct SaveFileSoftDeleteResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileSoftDeletePaths paths;
  bool saveMoved = false;
  bool snapshotMoved = false;
  bool snapshotMissing = false;
  bool targetExisted = false;
  bool snapshotTargetExisted = false;
};

struct SaveFileRecoverResult {
  bool ok = false;
  std::string reason = "not_requested";
  SaveFileSoftDeletePaths paths;
  bool saveRecovered = false;
  bool snapshotRecovered = false;
  bool snapshotMissing = false;
  bool targetExisted = false;
  bool snapshotTargetExisted = false;
};

bool isValidSaveFileId(std::string_view id);
std::filesystem::path saveFilePathForId(const std::filesystem::path& root,
                                        std::string_view id);
std::filesystem::path saveFileTempPathForId(const std::filesystem::path& root,
                                            std::string_view id,
                                            std::string_view attemptToken);
std::filesystem::path saveSnapshotPathForId(const std::filesystem::path& root,
                                            std::string_view id);
std::filesystem::path deletedSaveFilePathForId(const std::filesystem::path& root,
                                               std::string_view id);
std::filesystem::path deletedSaveSnapshotPathForId(
    const std::filesystem::path& root,
    std::string_view id);
SaveFileDurableWritePlan planDurableSaveFileWrite(
    const std::filesystem::path& root,
    std::string_view id,
    std::string_view attemptToken);
SaveFileTempWriteResult writeDurableSaveTempFile(
    const SaveFileTempWriteRequest& request);
SaveFileTempValidationResult validateDurableSaveTempFile(
    const SaveFileTempWriteResult& tempWrite);
SaveFileFinalCommitResult commitDurableSaveTempFile(
    const SaveFileTempValidationResult& validation);
SaveFileSoftDeletePlan planSoftDeleteSaveFile(const std::filesystem::path& root,
                                              std::string_view id);
SaveFileRecoverPlan planRecoverDeletedSaveFile(const std::filesystem::path& root,
                                               std::string_view id);
SaveFileSoftDeleteResult softDeleteSaveFile(const SaveFileSoftDeletePlan& plan);
SaveFileRecoverResult recoverDeletedSaveFile(const SaveFileRecoverPlan& plan);

std::vector<SaveFileRecord> listSaveFiles(const std::filesystem::path& root);
SaveFileWriteResult writeSessionSaveFile(const SaveFileWriteRequest& request);
SaveFileDurableWriteResult writeSessionSaveFileDurably(
    const SaveFileDurableWriteRequest& request);
SaveFileReadResult readSaveFile(const std::filesystem::path& path);
bool deleteSaveFile(const std::filesystem::path& path);
std::filesystem::path defaultSaveFileRoot();

}  // namespace iggy3d
