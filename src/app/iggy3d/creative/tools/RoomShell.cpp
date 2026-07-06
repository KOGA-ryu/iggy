#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cmath>
#include <string>
#include <string_view>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kGeneratedRoomShellTag = "generated_room_shell";

[[nodiscard]] std::string sourceRoomTag(CreativeObjectId roomObjectId) {
  return "source_room_" + std::to_string(roomObjectId);
}

[[nodiscard]] bool hasTag(const CreativeObject& object,
                          std::string_view tag) noexcept {
  for (const std::string& objectTag : object.tags) {
    if (objectTag == tag) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool isFiniteVec3(const CreativeVec3& value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool hasPositiveExtents(const CreativeBounds& bounds) noexcept {
  return bounds.max.x > bounds.min.x && bounds.max.y > bounds.min.y &&
         bounds.max.z > bounds.min.z;
}

[[nodiscard]] CreativeVec3 centerOfBounds(const CreativeBounds& bounds) noexcept {
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * 0.5,
      bounds.min.y + (bounds.max.y - bounds.min.y) * 0.5,
      bounds.min.z + (bounds.max.z - bounds.min.z) * 0.5,
  };
}

[[nodiscard]] bool isValidRoomShellBounds(const CreativeBounds& bounds,
                                          double wallThickness) noexcept {
  if (!isFiniteVec3(bounds.min) || !isFiniteVec3(bounds.max) ||
      !std::isfinite(wallThickness) || wallThickness <= 0.0 ||
      !hasPositiveExtents(bounds)) {
    return false;
  }

  const double width = bounds.max.x - bounds.min.x;
  const double depth = bounds.max.z - bounds.min.z;
  return width > wallThickness * 2.0 && depth > wallThickness * 2.0;
}

void setStatus(CreativeRoomShellBuildReceipt& receipt,
               CreativeRoomShellBuildStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.message = std::string(reasonCode);
}

[[nodiscard]] double defaultFloorHeight() noexcept {
  const CreativeObjectDescriptor& descriptor =
      describeObject(CreativeObjectKind::Floor);
  return descriptor.defaults.bounds.max.y - descriptor.defaults.bounds.min.y;
}

[[nodiscard]] CreativeDocumentCreateRequest makeShellCreateRequest(
    CreativeObjectKind kind,
    std::string name,
    CreativeBounds bounds,
    CreativeObjectId roomObjectId,
    const std::vector<std::string>& tags,
    bool visible) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.hasBoundsOverride = true;
  request.bounds = bounds;
  request.hasTransformOverride = true;
  request.transform.position = centerOfBounds(bounds);
  request.hasVisibleOverride = true;
  request.visible = visible;
  request.parentId = roomObjectId;
  request.tags = tags;
  return request;
}

[[nodiscard]] bool shellAlreadyExists(const CreativeDocument& document,
                                      CreativeObjectId roomObjectId) {
  const std::string sourceTag = sourceRoomTag(roomObjectId);
  for (const CreativeObject& object : document.objects()) {
    if (hasTag(object, kGeneratedRoomShellTag) && hasTag(object, sourceTag)) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::string_view toString(CreativeRoomShellBuildStatus status) noexcept {
  switch (status) {
    case CreativeRoomShellBuildStatus::Unknown:
      return "Unknown";
    case CreativeRoomShellBuildStatus::DocumentMissing:
      return "DocumentMissing";
    case CreativeRoomShellBuildStatus::NoRoomSelected:
      return "NoRoomSelected";
    case CreativeRoomShellBuildStatus::SelectedNotRoom:
      return "SelectedNotRoom";
    case CreativeRoomShellBuildStatus::InvalidBounds:
      return "InvalidBounds";
    case CreativeRoomShellBuildStatus::AlreadyExists:
      return "AlreadyExists";
    case CreativeRoomShellBuildStatus::Generated:
      return "Generated";
    case CreativeRoomShellBuildStatus::CreateRejected:
      return "CreateRejected";
    case CreativeRoomShellBuildStatus::InstallRejected:
      return "InstallRejected";
  }
  return "Unknown";
}

CreativeRoomShellBuildResult buildCreativeRoomShellCreateRequests(
    const CreativeRoomShellBuildRequest& request) {
  CreativeRoomShellBuildResult result;
  result.receipt.requested = true;
  result.receipt.roomObjectId = request.roomObjectId;

  if (request.document == nullptr) {
    setStatus(result.receipt,
              CreativeRoomShellBuildStatus::DocumentMissing,
              "creative_room_shell_document_missing");
    return result;
  }

  if (request.roomObjectId == kInvalidObjectId) {
    setStatus(result.receipt,
              CreativeRoomShellBuildStatus::NoRoomSelected,
              "creative_room_shell_no_room_selected");
    return result;
  }

  const CreativeObject* room = request.document->findObject(
      request.roomObjectId);
  if (room == nullptr || room->kind != CreativeObjectKind::Room) {
    setStatus(result.receipt,
              CreativeRoomShellBuildStatus::SelectedNotRoom,
              "creative_room_shell_selected_not_room");
    return result;
  }

  if (!isValidRoomShellBounds(room->bounds, request.wallThickness)) {
    setStatus(result.receipt,
              CreativeRoomShellBuildStatus::InvalidBounds,
              "creative_room_shell_invalid_bounds");
    return result;
  }

  if (shellAlreadyExists(*request.document, request.roomObjectId)) {
    setStatus(result.receipt,
              CreativeRoomShellBuildStatus::AlreadyExists,
              "creative_room_shell_already_exists");
    return result;
  }

  const CreativeBounds& bounds = room->bounds;
  const double wallThickness = request.wallThickness;
  const double floorHeight = defaultFloorHeight();
  const std::vector<std::string> tags = {
      std::string(kGeneratedRoomShellTag),
      sourceRoomTag(request.roomObjectId),
  };

  result.createRequests.reserve(5);
  result.createRequests.push_back(makeShellCreateRequest(
      CreativeObjectKind::Floor,
      "Room Shell Floor",
      {{bounds.min.x, bounds.min.y, bounds.min.z},
       {bounds.max.x, bounds.min.y + floorHeight, bounds.max.z}},
      request.roomObjectId,
      tags,
      room->visible));
  result.createRequests.push_back(makeShellCreateRequest(
      CreativeObjectKind::Wall,
      "Room Shell Wall North",
      {{bounds.min.x, bounds.min.y, bounds.min.z},
       {bounds.max.x, bounds.max.y, bounds.min.z + wallThickness}},
      request.roomObjectId,
      tags,
      room->visible));
  result.createRequests.push_back(makeShellCreateRequest(
      CreativeObjectKind::Wall,
      "Room Shell Wall South",
      {{bounds.min.x, bounds.min.y, bounds.max.z - wallThickness},
       {bounds.max.x, bounds.max.y, bounds.max.z}},
      request.roomObjectId,
      tags,
      room->visible));
  result.createRequests.push_back(makeShellCreateRequest(
      CreativeObjectKind::Wall,
      "Room Shell Wall West",
      {{bounds.min.x, bounds.min.y, bounds.min.z},
       {bounds.min.x + wallThickness, bounds.max.y, bounds.max.z}},
      request.roomObjectId,
      tags,
      room->visible));
  result.createRequests.push_back(makeShellCreateRequest(
      CreativeObjectKind::Wall,
      "Room Shell Wall East",
      {{bounds.max.x - wallThickness, bounds.min.y, bounds.min.z},
       {bounds.max.x, bounds.max.y, bounds.max.z}},
      request.roomObjectId,
      tags,
      room->visible));

  result.receipt.accepted = true;
  result.receipt.generatedRequestCount = result.createRequests.size();
  result.receipt.floorRequestCount = 1;
  result.receipt.wallRequestCount = 4;
  setStatus(result.receipt,
            CreativeRoomShellBuildStatus::Generated,
            "creative_room_shell_generated");
  return result;
}

}  // namespace iggy3d::creative
