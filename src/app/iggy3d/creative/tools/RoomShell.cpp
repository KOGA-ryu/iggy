#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kGeneratedRoomShellTag = "generated_room_shell";

[[nodiscard]] bool hasTag(const std::vector<std::string>& tags,
                          std::string_view tag) noexcept {
  for (const std::string& objectTag : tags) {
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
               CreativeRoomShellStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.message = std::string(reasonCode);
}

void setStatus(CreativeRoomShellRemoveReceipt& receipt,
               CreativeRoomShellStatus status,
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
  return !collectCreativeRoomShellChildIds(document, roomObjectId).empty();
}

}  // namespace

std::string_view toString(CreativeRoomShellStatus status) noexcept {
  switch (status) {
    case CreativeRoomShellStatus::Unknown:
      return "Unknown";
    case CreativeRoomShellStatus::DocumentMissing:
      return "DocumentMissing";
    case CreativeRoomShellStatus::NoRoomSelected:
      return "NoRoomSelected";
    case CreativeRoomShellStatus::SelectedNotRoom:
      return "SelectedNotRoom";
    case CreativeRoomShellStatus::InvalidBounds:
      return "InvalidBounds";
    case CreativeRoomShellStatus::AlreadyExists:
      return "AlreadyExists";
    case CreativeRoomShellStatus::Generated:
      return "Generated";
    case CreativeRoomShellStatus::NoGeneratedShell:
      return "NoGeneratedShell";
    case CreativeRoomShellStatus::Removed:
      return "Removed";
    case CreativeRoomShellStatus::CreateRejected:
      return "CreateRejected";
    case CreativeRoomShellStatus::RemoveRejected:
      return "RemoveRejected";
    case CreativeRoomShellStatus::InstallRejected:
      return "InstallRejected";
  }
  return "Unknown";
}

std::string_view generatedRoomShellTag() noexcept {
  return kGeneratedRoomShellTag;
}

std::string sourceRoomShellTag(CreativeObjectId roomObjectId) {
  return "source_room_" + std::to_string(roomObjectId);
}

bool creativeRoomShellCreateRequestHasProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeObjectId roomObjectId) {
  if (roomObjectId == kInvalidObjectId || !request.parentId.has_value() ||
      request.parentId.value() != roomObjectId) {
    return false;
  }

  const std::string sourceTag = sourceRoomShellTag(roomObjectId);
  return hasTag(request.tags, generatedRoomShellTag()) &&
         hasTag(request.tags, sourceTag);
}

bool creativeRoomShellObjectHasProvenance(const CreativeObject& object,
                                          CreativeObjectId roomObjectId) {
  if (roomObjectId == kInvalidObjectId || !object.parentId.has_value() ||
      object.parentId.value() != roomObjectId) {
    return false;
  }

  const std::string sourceTag = sourceRoomShellTag(roomObjectId);
  return hasTag(object.tags, generatedRoomShellTag()) &&
         hasTag(object.tags, sourceTag);
}

std::vector<CreativeObjectId> collectCreativeRoomShellChildIds(
    const CreativeDocument& document,
    CreativeObjectId roomObjectId) {
  std::vector<CreativeObjectId> objectIds;
  if (roomObjectId == kInvalidObjectId) {
    return objectIds;
  }

  for (const CreativeObject& object : document.objects()) {
    if (creativeRoomShellObjectHasProvenance(object, roomObjectId)) {
      objectIds.push_back(object.id);
    }
  }
  return objectIds;
}

CreativeRoomShellBuildResult buildCreativeRoomShellCreateRequests(
    const CreativeRoomShellBuildRequest& request) {
  CreativeRoomShellBuildResult result;
  result.receipt.requested = true;
  result.receipt.roomObjectId = request.roomObjectId;

  if (request.document == nullptr) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::DocumentMissing,
              "creative_room_shell_document_missing");
    return result;
  }

  if (request.roomObjectId == kInvalidObjectId) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::NoRoomSelected,
              "creative_room_shell_no_room_selected");
    return result;
  }

  const CreativeObject* room = request.document->findObject(
      request.roomObjectId);
  if (room == nullptr || room->kind != CreativeObjectKind::Room) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::SelectedNotRoom,
              "creative_room_shell_selected_not_room");
    return result;
  }

  if (!isValidRoomShellBounds(room->bounds, request.wallThickness)) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::InvalidBounds,
              "creative_room_shell_invalid_bounds");
    return result;
  }

  if (shellAlreadyExists(*request.document, request.roomObjectId)) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::AlreadyExists,
              "creative_room_shell_already_exists");
    return result;
  }

  const CreativeBounds& bounds = room->bounds;
  const double wallThickness = request.wallThickness;
  const double floorHeight = defaultFloorHeight();
  const std::vector<std::string> tags = {
      std::string(generatedRoomShellTag()),
      sourceRoomShellTag(request.roomObjectId),
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
            CreativeRoomShellStatus::Generated,
            "creative_room_shell_generated");
  return result;
}

bool creativeRoomHasGeneratedShellChildren(const CreativeDocument& document,
                                           CreativeObjectId roomObjectId) {
  if (roomObjectId == kInvalidObjectId) {
    return false;
  }

  for (const CreativeObject& object : document.objects()) {
    if (creativeRoomShellObjectHasProvenance(object, roomObjectId)) {
      return true;
    }
  }
  return false;
}

CreativeRoomShellRemoveResult findCreativeRoomShellChildren(
    const CreativeRoomShellRemoveRequest& request) {
  CreativeRoomShellRemoveResult result;
  result.receipt.requested = true;
  result.receipt.roomObjectId = request.roomObjectId;

  if (request.document == nullptr) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::DocumentMissing,
              "creative_room_shell_remove_document_missing");
    return result;
  }

  if (request.roomObjectId == kInvalidObjectId) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::NoRoomSelected,
              "creative_room_shell_remove_no_room_selected");
    return result;
  }

  const CreativeObject* room = request.document->findObject(
      request.roomObjectId);
  if (room == nullptr || room->kind != CreativeObjectKind::Room) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::SelectedNotRoom,
              "creative_room_shell_remove_selected_not_room");
    return result;
  }

  const std::vector<CreativeObjectId> generatedChildIds =
      collectCreativeRoomShellChildIds(*request.document, request.roomObjectId);
  for (const CreativeObjectId childId : generatedChildIds) {
    const CreativeObject* object = request.document->findObject(childId);
    if (object == nullptr) {
      continue;
    }

    result.objectIds.push_back(object->id);
    if (object->kind == CreativeObjectKind::Floor) {
      ++result.receipt.floorObjectCount;
    } else if (object->kind == CreativeObjectKind::Wall) {
      ++result.receipt.wallObjectCount;
    }
  }

  result.receipt.removedObjectCount = result.objectIds.size();
  if (result.objectIds.empty()) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::NoGeneratedShell,
              "creative_room_shell_remove_not_found");
    return result;
  }

  result.receipt.accepted = true;
  setStatus(result.receipt,
            CreativeRoomShellStatus::Removed,
            "creative_room_shell_removed");
  return result;
}

}  // namespace iggy3d::creative
