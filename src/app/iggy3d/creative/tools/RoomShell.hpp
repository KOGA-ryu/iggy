#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeRoomShellStatus : std::uint8_t {
  Unknown,
  DocumentMissing,
  NoRoomSelected,
  SelectedNotRoom,
  InvalidBounds,
  AlreadyExists,
  Generated,
  NoGeneratedShell,
  Removed,
  CreateRejected,
  RemoveRejected,
  InstallRejected,
};

struct CreativeRoomShellBuildRequest {
  const CreativeDocument* document = nullptr;
  CreativeObjectId roomObjectId = kInvalidObjectId;
  double wallThickness = defaultCreativeWallGeometry().thicknessMeters;
};

struct CreativeRoomShellBuildReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeObjectId roomObjectId = kInvalidObjectId;
  std::uint64_t generatedRequestCount = 0;
  std::uint64_t floorRequestCount = 0;
  std::uint64_t wallRequestCount = 0;
  CreativeRoomShellStatus status = CreativeRoomShellStatus::Unknown;
  std::string reasonCode = "creative_room_shell_not_requested";
  std::string message = "creative_room_shell_not_requested";
};

struct CreativeRoomShellBuildResult {
  std::vector<CreativeDocumentCreateRequest> createRequests;
  CreativeRoomShellBuildReceipt receipt;
};

struct CreativeRoomShellRemoveRequest {
  const CreativeDocument* document = nullptr;
  CreativeObjectId roomObjectId = kInvalidObjectId;
};

struct CreativeRoomShellRemoveReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeObjectId roomObjectId = kInvalidObjectId;
  std::uint64_t removedObjectCount = 0;
  std::uint64_t floorObjectCount = 0;
  std::uint64_t wallObjectCount = 0;
  CreativeRoomShellStatus status = CreativeRoomShellStatus::Unknown;
  std::string reasonCode = "creative_room_shell_remove_not_requested";
  std::string message = "creative_room_shell_remove_not_requested";
};

struct CreativeRoomShellRemoveResult {
  std::vector<CreativeObjectId> objectIds;
  CreativeRoomShellRemoveReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    CreativeRoomShellStatus status) noexcept;

[[nodiscard]] std::string_view generatedRoomShellTag() noexcept;
[[nodiscard]] std::string sourceRoomShellTag(CreativeObjectId roomObjectId);
[[nodiscard]] bool creativeRoomShellCreateRequestHasProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeObjectId roomObjectId);
[[nodiscard]] bool creativeRoomShellObjectHasProvenance(
    const CreativeObject& object,
    CreativeObjectId roomObjectId);
[[nodiscard]] std::vector<CreativeObjectId> collectCreativeRoomShellChildIds(
    const CreativeDocument& document,
    CreativeObjectId roomObjectId);

[[nodiscard]] CreativeRoomShellBuildResult buildCreativeRoomShellCreateRequests(
    const CreativeRoomShellBuildRequest& request);

[[nodiscard]] bool creativeRoomHasGeneratedShellChildren(
    const CreativeDocument& document,
    CreativeObjectId roomObjectId);

[[nodiscard]] CreativeRoomShellRemoveResult findCreativeRoomShellChildren(
    const CreativeRoomShellRemoveRequest& request);

}  // namespace iggy3d::creative
