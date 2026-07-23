#include "app/iggy3d/creative/recipes/ObjectLibraryRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>

namespace iggy3d::creative {
namespace {

void setStatus(CreativeObjectLibraryRecipeReceipt& receipt,
               CreativeObjectLibraryRecipeStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

bool validObjectKind(CreativeObjectKind kind) noexcept {
  return kind > CreativeObjectKind::Unknown && kind < CreativeObjectKind::Count;
}

bool validPlacement(const CreativeObjectLibraryPlacementSpec& placement) {
  if (!validObjectKind(placement.kind) || placement.stableKey.empty() ||
      placement.name.empty() ||
      placement.mode >= CreativeObjectLibraryPlacementMode::Count) {
    return false;
  }
  if ((placement.kind == CreativeObjectKind::SpawnPoint &&
       !isValidCreativePlayerSpawnSettings(placement.playerSpawn)) ||
      (placement.kind != CreativeObjectKind::SpawnPoint &&
       !(placement.playerSpawn == CreativePlayerSpawnSettings{}))) {
    return false;
  }
  const bool npcActor = placement.kind == CreativeObjectKind::NpcSpawn ||
                        placement.kind == CreativeObjectKind::EnemySpawn;
  if ((npcActor && !isValidCreativeNpcSpawnSettings(placement.npcSpawn)) ||
      (!npcActor &&
       !(placement.npcSpawn == CreativeNpcSpawnSettings{}))) {
    return false;
  }
  if ((placement.kind == CreativeObjectKind::LootPoint &&
       !isValidCreativeLootPointSettings(placement.lootPoint)) ||
      (placement.kind != CreativeObjectKind::LootPoint &&
       !(placement.lootPoint == CreativeLootPointSettings{}))) {
    return false;
  }
  if ((placement.kind == CreativeObjectKind::ExitPoint &&
       !isValidCreativeExitPointSettings(placement.exitPoint)) ||
      (placement.kind != CreativeObjectKind::ExitPoint &&
       !(placement.exitPoint == CreativeExitPointSettings{}))) {
    return false;
  }
  if (placement.mode == CreativeObjectLibraryPlacementMode::Point) {
    if (!objectHasTransform(placement.kind) ||
        !isFiniteCreativeVec3(placement.point) ||
        !std::isfinite(placement.yawRadians) ||
        !isPositiveCreativeVec3(placement.scale)) {
      return false;
    }
    if (!placement.hasAssetSourceBounds) {
      return true;
    }
    const CreativeBoundsMetrics source =
        measureCreativeBounds(placement.assetSourceBounds);
    return objectHasBounds(placement.kind) && source.valid &&
           isPositiveCreativeVec3(source.size);
  }
  if (placement.hasAssetSourceBounds || placement.yawRadians != 0.0 ||
      placement.scale.x != 1.0 || placement.scale.y != 1.0 ||
      placement.scale.z != 1.0) {
    return false;
  }
  const CreativeBoundsMetrics metrics = measureCreativeBounds(placement.bounds);
  return objectHasBounds(placement.kind) && metrics.valid &&
         isPositiveCreativeVec3(metrics.size);
}

CreativeDocumentCreateRequest createRequest(
    const CreativeObjectLibraryPlacementSpec& placement) {
  CreativeDocumentCreateRequest request;
  request.kind = placement.kind;
  request.name = placement.name;
  request.assetId = placement.assetId;
  request.visible = placement.visible;
  request.hasVisibleOverride = true;
  request.tags = placement.tags;
  if (placement.kind == CreativeObjectKind::SpawnPoint) {
    request.hasPlayerSpawnSettingsOverride = true;
    request.playerSpawn = placement.playerSpawn;
  }
  if (placement.kind == CreativeObjectKind::NpcSpawn ||
      placement.kind == CreativeObjectKind::EnemySpawn) {
    request.hasNpcSpawnSettingsOverride = true;
    request.npcSpawn = placement.npcSpawn;
  }
  if (placement.kind == CreativeObjectKind::LootPoint) {
    request.hasLootPointSettingsOverride = true;
    request.lootPoint = placement.lootPoint;
  }
  if (placement.kind == CreativeObjectKind::ExitPoint) {
    request.hasExitPointSettingsOverride = true;
    request.exitPoint = placement.exitPoint;
  }
  if (placement.mode == CreativeObjectLibraryPlacementMode::Bounds) {
    request.bounds = placement.bounds;
    request.hasBoundsOverride = true;
    if (objectHasTransform(placement.kind)) {
      request.transform.position =
          measureCreativeBounds(placement.bounds).center;
      request.hasTransformOverride = true;
    }
  } else {
    request.transform.position = placement.point;
    request.transform.rotationEulerRadians.y = placement.yawRadians;
    request.transform.scale = placement.scale;
    request.hasTransformOverride = true;
    if (placement.hasAssetSourceBounds) {
      request.bounds = {
          {placement.point.x + placement.assetSourceBounds.min.x,
           placement.point.y + placement.assetSourceBounds.min.y,
           placement.point.z + placement.assetSourceBounds.min.z},
          {placement.point.x + placement.assetSourceBounds.max.x,
           placement.point.y + placement.assetSourceBounds.max.y,
           placement.point.z + placement.assetSourceBounds.max.z},
      };
      request.hasBoundsOverride = true;
    }
  }
  return request;
}

}  // namespace

std::string_view toString(
    CreativeObjectLibraryPlacementMode mode) noexcept {
  switch (mode) {
    case CreativeObjectLibraryPlacementMode::Bounds:
      return "Bounds";
    case CreativeObjectLibraryPlacementMode::Point:
      return "Point";
    case CreativeObjectLibraryPlacementMode::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeObjectLibraryRecipeStatus status) noexcept {
  switch (status) {
    case CreativeObjectLibraryRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeObjectLibraryRecipeStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeObjectLibraryRecipeStatus::Empty:
      return "Empty";
    case CreativeObjectLibraryRecipeStatus::InvalidPlacement:
      return "InvalidPlacement";
    case CreativeObjectLibraryRecipeStatus::InvalidPlan:
      return "InvalidPlan";
    case CreativeObjectLibraryRecipeStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeObjectLibraryRecipeResult buildCreativeObjectLibraryRecipe(
    const CreativeObjectLibraryRecipeRequest& request) {
  CreativeObjectLibraryRecipeResult result;
  result.receipt.requested = true;
  if (request.stableKey.empty() || request.name.empty()) {
    setStatus(result.receipt,
              CreativeObjectLibraryRecipeStatus::InvalidRequest,
              "creative_object_library_recipe_request_invalid");
    return result;
  }
  if (request.placements.empty()) {
    setStatus(result.receipt, CreativeObjectLibraryRecipeStatus::Empty,
              "creative_object_library_recipe_empty");
    return result;
  }

  result.plan.kind = CreativeRecipeKind::ObjectLibrary;
  result.plan.instanceKey = request.stableKey;
  result.plan.instanceName = request.name;
  result.plan.objects.reserve(request.placements.size());
  for (std::size_t index = 0U; index < request.placements.size(); ++index) {
    const CreativeObjectLibraryPlacementSpec& placement =
        request.placements[index];
    result.receipt.failedPlacementIndex = index;
    if (!validPlacement(placement)) {
      result.plan = {};
      setStatus(result.receipt,
                CreativeObjectLibraryRecipeStatus::InvalidPlacement,
                "creative_object_library_recipe_placement_invalid");
      return result;
    }
    CreativeRecipeObjectPlan object;
    object.createRequest = createRequest(placement);
    object.role = CreativeRecipeObjectRole::Source;
    object.stableKey = placement.stableKey;
    result.plan.objects.push_back(std::move(object));
    if (placement.mode == CreativeObjectLibraryPlacementMode::Bounds) {
      ++result.receipt.boundedPlacementCount;
    } else {
      ++result.receipt.pointPlacementCount;
    }
  }

  const CreativeRecipeMaterializeResult validated =
      materializeCreativeRecipe(result.plan, 1U);
  if (!validated.receipt.accepted) {
    result.plan = {};
    setStatus(result.receipt, CreativeObjectLibraryRecipeStatus::InvalidPlan,
              validated.receipt.reasonCode);
    return result;
  }
  result.receipt.failedPlacementIndex = 0U;
  setStatus(result.receipt, CreativeObjectLibraryRecipeStatus::Ready,
            "creative_object_library_recipe_ready", true);
  return result;
}

}  // namespace iggy3d::creative
