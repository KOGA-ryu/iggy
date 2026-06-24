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

bool isValidSaveFileId(std::string_view id);
std::filesystem::path saveFilePathForId(const std::filesystem::path& root,
                                        std::string_view id);
std::filesystem::path saveFileTempPathForId(const std::filesystem::path& root,
                                            std::string_view id,
                                            std::string_view attemptToken);
std::filesystem::path saveSnapshotPathForId(const std::filesystem::path& root,
                                            std::string_view id);
SaveFileDurableWritePlan planDurableSaveFileWrite(
    const std::filesystem::path& root,
    std::string_view id,
    std::string_view attemptToken);
SaveFileTempWriteResult writeDurableSaveTempFile(
    const SaveFileTempWriteRequest& request);
SaveFileTempValidationResult validateDurableSaveTempFile(
    const SaveFileTempWriteResult& tempWrite);

std::vector<SaveFileRecord> listSaveFiles(const std::filesystem::path& root);
SaveFileWriteResult writeSessionSaveFile(const SaveFileWriteRequest& request);
SaveFileReadResult readSaveFile(const std::filesystem::path& path);
bool deleteSaveFile(const std::filesystem::path& path);
std::filesystem::path defaultSaveFileRoot();

}  // namespace iggy3d
