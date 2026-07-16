#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"

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

[[nodiscard]] bool isValidRoomShellBounds(const CreativeBounds& bounds,
                                          double wallThickness) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  if (!metrics.valid || !isPositiveCreativeVec3(metrics.size) ||
      !std::isfinite(wallThickness) || wallThickness <= 0.0) {
    return false;
  }

  return metrics.size.x > wallThickness * 2.0 &&
         metrics.size.z > wallThickness * 2.0;
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
  return measureCreativeBounds(descriptor.defaults.bounds).size.y;
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

  CreativeBuildingRecipeRequest building;
  building.stableKey = sourceRoomShellTag(request.roomObjectId);
  building.name = room->name;
  building.rootMode = CreativeBuildingRootMode::ExistingRoom;
  building.existingRoomObjectId = request.roomObjectId;
  building.visible = room->visible;
  building.tags = tags;
  building.boxes.push_back(
      {CreativeObjectKind::Floor,
       "room_shell.floor",
       "Room Shell Floor",
       {{bounds.min.x, bounds.min.y, bounds.min.z},
        {bounds.max.x, bounds.min.y + floorHeight, bounds.max.z}}});
  const double wallHeight = bounds.max.y - bounds.min.y;
  building.walls = {
      {"room_shell.wall.north",
       "Room Shell Wall North",
       {bounds.min.x, bounds.min.y, bounds.min.z + wallThickness * 0.5},
       {bounds.max.x, bounds.min.y, bounds.min.z + wallThickness * 0.5},
       wallHeight,
       wallThickness,
       {},
       {}},
      {"room_shell.wall.south",
       "Room Shell Wall South",
       {bounds.min.x, bounds.min.y, bounds.max.z - wallThickness * 0.5},
       {bounds.max.x, bounds.min.y, bounds.max.z - wallThickness * 0.5},
       wallHeight,
       wallThickness,
       {},
       {}},
      {"room_shell.wall.west",
       "Room Shell Wall West",
       {bounds.min.x + wallThickness * 0.5, bounds.min.y, bounds.min.z},
       {bounds.min.x + wallThickness * 0.5, bounds.min.y, bounds.max.z},
       wallHeight,
       wallThickness,
       {},
       {}},
      {"room_shell.wall.east",
       "Room Shell Wall East",
       {bounds.max.x - wallThickness * 0.5, bounds.min.y, bounds.min.z},
       {bounds.max.x - wallThickness * 0.5, bounds.min.y, bounds.max.z},
       wallHeight,
       wallThickness,
       {},
       {}},
  };
  const CreativeBuildingRecipeResult recipe =
      buildCreativeBuildingRecipe(building);
  if (!recipe.receipt.accepted) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::InvalidBounds,
              recipe.receipt.reasonCode);
    return result;
  }
  CreativeRecipeMaterializeResult materialized = materializeCreativeRecipe(
      recipe.plan, request.document->nextObjectId());
  if (!materialized.receipt.accepted) {
    setStatus(result.receipt,
              CreativeRoomShellStatus::CreateRejected,
              materialized.receipt.reasonCode);
    return result;
  }
  result.createRequests = std::move(materialized.createRequests);

  result.receipt.accepted = true;
  result.receipt.generatedRequestCount = result.createRequests.size();
  result.receipt.floorRequestCount = recipe.receipt.boxObjectCount;
  result.receipt.wallRequestCount = recipe.receipt.wallObjectCount;
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
