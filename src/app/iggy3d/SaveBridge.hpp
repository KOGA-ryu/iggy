#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "app/frontend/SaveSlotModel.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult {
  std::filesystem::path saveRoot;
  SaveSlotList slots;
  std::string_view status = "save_bridge_ready";
};

struct ProductSaveWriteRequest {
  std::filesystem::path saveRoot;
  std::string saveIdHint;
  std::string attemptToken;
  const SessionState* state = nullptr;
  const SaveAuthoredRoomSection* authoredRoom = nullptr;
  std::string worldId;
  std::string saveType;
  std::string autoTitle;
};

struct ProductSaveWriteResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string durableReason = "not_requested";
  SaveFileRecord record;
  SaveFileDurableWritePaths paths;
  std::uint64_t encodedBytes = 0;
  bool durableWriteRequested = false;
  bool tempWritten = false;
  bool tempValidated = false;
  bool committed = false;
  bool finalValidated = false;
  bool previousExisted = false;
  bool previousPreserved = true;
  std::string worldId;
  std::string saveType;
  std::string autoTitle;
};

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId);
ProductSaveWriteResult writeProductSessionSaveDurably(
    const ProductSaveWriteRequest& request);

}  // namespace iggy3d
