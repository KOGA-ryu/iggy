#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
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

std::vector<SaveFileRecord> listSaveFiles(const std::filesystem::path& root);
SaveFileWriteResult writeSessionSaveFile(const SaveFileWriteRequest& request);
SaveFileReadResult readSaveFile(const std::filesystem::path& path);
bool deleteSaveFile(const std::filesystem::path& path);
std::filesystem::path defaultSaveFileRoot();

}  // namespace iggy3d
