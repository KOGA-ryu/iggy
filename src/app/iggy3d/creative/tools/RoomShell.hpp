#pragma once

#include "app/iggy3d/creative/document/Document.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeRoomShellBuildStatus : std::uint8_t {
  Unknown,
  DocumentMissing,
  NoRoomSelected,
  SelectedNotRoom,
  InvalidBounds,
  AlreadyExists,
  Generated,
  CreateRejected,
  InstallRejected,
};

struct CreativeRoomShellBuildRequest {
  const CreativeDocument* document = nullptr;
  CreativeObjectId roomObjectId = kInvalidObjectId;
  double wallThickness = 0.25;
};

struct CreativeRoomShellBuildReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeObjectId roomObjectId = kInvalidObjectId;
  std::uint64_t generatedRequestCount = 0;
  std::uint64_t floorRequestCount = 0;
  std::uint64_t wallRequestCount = 0;
  CreativeRoomShellBuildStatus status = CreativeRoomShellBuildStatus::Unknown;
  std::string reasonCode = "creative_room_shell_not_requested";
  std::string message = "creative_room_shell_not_requested";
};

struct CreativeRoomShellBuildResult {
  std::vector<CreativeDocumentCreateRequest> createRequests;
  CreativeRoomShellBuildReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    CreativeRoomShellBuildStatus status) noexcept;

[[nodiscard]] CreativeRoomShellBuildResult buildCreativeRoomShellCreateRequests(
    const CreativeRoomShellBuildRequest& request);

}  // namespace iggy3d::creative
