#include "app/iggy3d/creative/world/DocumentSectionInternal.hpp"

#include "content/assets/StaticMeshAsset.hpp"

#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::document_section_internal {
namespace {

[[nodiscard]] SaveCreativeDocumentVec3Record toSaveVec3(
    creative::CreativeVec3 value) noexcept {
  return {value.x, value.y, value.z};
}

[[nodiscard]] creative::CreativeVec3 toCreativeVec3(
    SaveCreativeDocumentVec3Record value) noexcept {
  return {value.x, value.y, value.z};
}

[[nodiscard]] SaveCreativeDocumentTransformRecord toSaveTransform(
    creative::CreativeTransform transform) noexcept {
  SaveCreativeDocumentTransformRecord record;
  record.position = toSaveVec3(transform.position);
  record.rotation = toSaveVec3(transform.rotationEulerRadians);
  record.scale = toSaveVec3(transform.scale);
  return record;
}

[[nodiscard]] creative::CreativeTransform toCreativeTransform(
    SaveCreativeDocumentTransformRecord record) noexcept {
  creative::CreativeTransform transform;
  transform.position = toCreativeVec3(record.position);
  transform.rotationEulerRadians = toCreativeVec3(record.rotation);
  transform.scale = toCreativeVec3(record.scale);
  return transform;
}

}  // namespace

[[nodiscard]] SaveCreativeDocumentBoundsRecord toSaveBounds(
    creative::CreativeBounds bounds) noexcept {
  return {toSaveVec3(bounds.min), toSaveVec3(bounds.max)};
}

[[nodiscard]] creative::CreativeBounds toCreativeBounds(
    SaveCreativeDocumentBoundsRecord record) noexcept {
  return {toCreativeVec3(record.min), toCreativeVec3(record.max)};
}

namespace {

[[nodiscard]] SaveCreativeDocumentPathPointRecord toSavePathPoint(
    creative::CreativePathPoint point) noexcept {
  return {point.position.x, point.position.y, point.position.z,
          point.dwellSeconds, point.outgoingSpeedMultiplier};
}

[[nodiscard]] creative::CreativePathPoint toCreativePathPoint(
    SaveCreativeDocumentPathPointRecord record) noexcept {
  return {{record.x, record.y, record.z}, record.dwellSeconds,
          record.outgoingSpeedMultiplier};
}

}  // namespace

[[nodiscard]] std::string_view toSaveUnits(
    creative::CreativeUnits units) noexcept {
  switch (units) {
    case creative::CreativeUnits::Meters:
      return "Meters";
  }
  return {};
}

[[nodiscard]] bool parseUnits(std::string_view value,
                              creative::CreativeUnits& out) noexcept {
  if (value == "Meters") {
    out = creative::CreativeUnits::Meters;
    return true;
  }
  return false;
}

namespace {

[[nodiscard]] std::string_view toSaveSnapMode(
    creative::CreativeDocumentSnapMode mode) noexcept {
  switch (mode) {
    case creative::CreativeDocumentSnapMode::Disabled:
      return "Disabled";
    case creative::CreativeDocumentSnapMode::Grid:
      return "Grid";
  }
  return {};
}

[[nodiscard]] bool parseSnapMode(
    std::string_view value,
    creative::CreativeDocumentSnapMode& out) noexcept {
  if (value == "Disabled") {
    out = creative::CreativeDocumentSnapMode::Disabled;
    return true;
  }
  if (value == "Grid") {
    out = creative::CreativeDocumentSnapMode::Grid;
    return true;
  }
  return false;
}

[[nodiscard]] bool toCreativeGridSize(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t depth,
    creative::CreativeGridSize3& out) noexcept {
  constexpr auto maxValue =
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
  if (width > maxValue || height > maxValue || depth > maxValue) {
    return false;
  }
  out.width = static_cast<std::int32_t>(width);
  out.height = static_cast<std::int32_t>(height);
  out.depth = static_cast<std::int32_t>(depth);
  return true;
}

[[nodiscard]] bool toSaveGridDimension(std::int32_t value,
                                       std::uint32_t& out) noexcept {
  if (value < 0) {
    return false;
  }
  out = static_cast<std::uint32_t>(value);
  return true;
}

}  // namespace

[[nodiscard]] bool toSaveGridSettings(
    creative::CreativeGridSettings settings,
    SaveCreativeDocumentSection& section) noexcept {
  section.gridOrigin = toSaveVec3(settings.origin);
  section.cellSizeMeters = settings.cellSizeMeters;
  return toSaveGridDimension(settings.size.width, section.gridWidth) &&
         toSaveGridDimension(settings.size.height, section.gridHeight) &&
         toSaveGridDimension(settings.size.depth, section.gridDepth);
}

[[nodiscard]] bool toCreativeGridSettings(
    const SaveCreativeDocumentSection& section,
    creative::CreativeGridSettings& out) noexcept {
  out.origin = toCreativeVec3(section.gridOrigin);
  out.cellSizeMeters = section.cellSizeMeters;
  return toCreativeGridSize(section.gridWidth,
                            section.gridHeight,
                            section.gridDepth,
                            out.size);
}

[[nodiscard]] bool toSaveSnapSettings(
    creative::CreativeDocumentSnapSettings settings,
    SaveCreativeDocumentSection& section) noexcept {
  const std::string_view snapMode = toSaveSnapMode(settings.mode);
  if (snapMode.empty()) {
    return false;
  }
  section.snapMode = std::string{snapMode};
  section.snapAxes = settings.axes;
  section.snapStepX = settings.stepX;
  section.snapStepY = settings.stepY;
  section.snapStepZ = settings.stepZ;
  section.snapOriginX = settings.originX;
  section.snapOriginY = settings.originY;
  section.snapOriginZ = settings.originZ;
  return true;
}

[[nodiscard]] bool toCreativeSnapSettings(
    const SaveCreativeDocumentSection& section,
    creative::CreativeDocumentSnapSettings& out) noexcept {
  if (!parseSnapMode(section.snapMode, out.mode) ||
      section.snapAxes > std::numeric_limits<
                             creative::CreativeDocumentSnapAxisMask>::max()) {
    return false;
  }
  out.axes =
      static_cast<creative::CreativeDocumentSnapAxisMask>(section.snapAxes);
  out.stepX = section.snapStepX;
  out.stepY = section.snapStepY;
  out.stepZ = section.snapStepZ;
  out.originX = section.snapOriginX;
  out.originY = section.snapOriginY;
  out.originZ = section.snapOriginZ;
  return creative::isValidCreativeDocumentSnapSettings(out);
}

[[nodiscard]] SaveCreativeDocumentObjectRecord toSaveObject(
    const creative::CreativeObject& object) {
  SaveCreativeDocumentObjectRecord record;
  record.id = object.id;
  record.kind = std::string{creative::serializedObjectKindId(object.kind)};
  record.name = object.name;
  record.assetId = object.assetId;
  record.assetContentHash = object.assetContentHash;
  record.assetMaterialVariant = object.assetMaterialVariant;
  record.transform = toSaveTransform(object.transform);
  record.bounds = toSaveBounds(object.bounds);
  record.layerId = object.layerId;
  record.visible = object.visible;
  record.locked = object.locked;
  record.hasParent = object.parentId.has_value();
  record.parentId = object.parentId.value_or(creative::kInvalidObjectId);
  record.attachmentSocket = object.attachmentSocket;
  record.tags = object.tags;
  record.pathPoints.reserve(object.pathPoints.size());
  for (const creative::CreativePathPoint& point : object.pathPoints) {
    record.pathPoints.push_back(toSavePathPoint(point));
  }
  if (object.kind == creative::CreativeObjectKind::MovingPlatform) {
    record.movingPlatformSpeedMetersPerSecond =
        object.movingPlatform.speedMetersPerSecond;
    record.movingPlatformTraversalMode =
        std::string{creative::toString(object.movingPlatform.traversalMode)};
    record.movingPlatformStartsActive = object.movingPlatform.startsActive;
  }
  if (object.kind == creative::CreativeObjectKind::Door) {
    record.doorLeafArrangement =
        std::string{creative::toString(object.door.leafArrangement)};
    record.doorHingeSide =
        std::string{creative::toString(object.door.hingeSide)};
    record.doorSwingSide =
        std::string{creative::toString(object.door.swingSide)};
    record.doorInitialState =
        std::string{creative::toString(object.door.initialState)};
    record.doorGameplayLocked = object.door.gameplayLocked;
    record.doorTransitionSeconds = object.door.transitionSeconds;
  }
  if (object.kind == creative::CreativeObjectKind::Window) {
    record.windowInsertKind =
        std::string{creative::toString(object.window.insertKind)};
  }
  if (object.kind == creative::CreativeObjectKind::SpawnPoint) {
    record.playerSpawnProfileId = object.playerSpawn.playerProfileId;
    record.playerSpawnGroup = object.playerSpawn.spawnGroup;
    record.playerSpawnValidationRadiusMeters =
        object.playerSpawn.validationRadiusMeters;
    record.playerSpawnFallbackPriority = object.playerSpawn.fallbackPriority;
  }
  if (object.kind == creative::CreativeObjectKind::NpcSpawn ||
      object.kind == creative::CreativeObjectKind::EnemySpawn) {
    record.npcBehaviorProfileId = object.npcSpawn.behaviorProfileId;
    record.npcTeam = std::string{creative::toString(object.npcSpawn.team)};
    record.npcHitPoints = object.npcSpawn.hitPoints;
    record.npcInitialAlertLevel = object.npcSpawn.initialAlertLevel;
    record.npcSpawnPolicy =
        std::string{creative::toString(object.npcSpawn.spawnPolicy)};
  }
  if (object.kind == creative::CreativeObjectKind::LootPoint) {
    record.lootItemId = object.lootPoint.itemId;
    record.lootItemCount = object.lootPoint.itemCount;
    record.lootDeactivateOnCollect = object.lootPoint.deactivateOnCollect;
  }
  if (object.kind == creative::CreativeObjectKind::ExitPoint) {
    record.exitRequiredItemId = object.exitPoint.requiredItemId;
    record.exitRequiredItemCount = object.exitPoint.requiredItemCount;
  }
  return record;
}

[[nodiscard]] bool toCreativeObject(
    const SaveCreativeDocumentObjectRecord& record,
    std::uint32_t sectionVersion,
    creative::CreativeObject& out) noexcept {
  creative::CreativeObjectKind kind = creative::CreativeObjectKind::Unknown;
  if (!creative::parseSerializedObjectKindId(record.kind, kind) ||
      (!record.assetId.empty() &&
       !validStaticMeshAssetId(record.assetId)) ||
      (record.assetId.empty() &&
       (record.assetContentHash != 0U ||
        !record.assetMaterialVariant.empty())) ||
      !creative::validCreativeAssetMaterialVariantName(
          record.assetMaterialVariant)) {
    return false;
  }
  out.id = record.id;
  out.kind = kind;
  out.name = record.name;
  out.assetId = record.assetId;
  out.assetContentHash = record.assetContentHash;
  out.assetMaterialVariant = record.assetMaterialVariant;
  out.transform = toCreativeTransform(record.transform);
  out.bounds = toCreativeBounds(record.bounds);
  out.layerId = record.layerId;
  out.visible = record.visible;
  out.locked = record.locked;
  out.parentId = record.hasParent
                     ? std::optional<creative::CreativeObjectId>{record.parentId}
                     : std::nullopt;
  out.attachmentSocket = record.attachmentSocket;
  out.tags = record.tags;
  out.pathPoints.reserve(record.pathPoints.size());
  for (const SaveCreativeDocumentPathPointRecord& point : record.pathPoints) {
    out.pathPoints.push_back(toCreativePathPoint(point));
  }
  if (kind == creative::CreativeObjectKind::MovingPlatform) {
    if (!creative::parseCreativeMovingPlatformTraversalMode(
            record.movingPlatformTraversalMode,
            out.movingPlatform.traversalMode)) {
      return false;
    }
    out.movingPlatform.speedMetersPerSecond =
        record.movingPlatformSpeedMetersPerSecond;
    out.movingPlatform.startsActive = record.movingPlatformStartsActive;
    if (!creative::isValidCreativeMovingPlatformSettings(
            out.movingPlatform)) {
      return false;
    }
  }
  if (kind == creative::CreativeObjectKind::Door) {
    if (!creative::parseCreativeDoorLeafArrangement(
            record.doorLeafArrangement, out.door.leafArrangement) ||
        !creative::parseCreativeDoorHingeSide(record.doorHingeSide,
                                              out.door.hingeSide) ||
        !creative::parseCreativeDoorSwingSide(record.doorSwingSide,
                                              out.door.swingSide) ||
        !creative::parseCreativeDoorInitialState(record.doorInitialState,
                                                 out.door.initialState)) {
      return false;
    }
    out.door.gameplayLocked = record.doorGameplayLocked;
    out.door.transitionSeconds = record.doorTransitionSeconds;
    if (!creative::isValidCreativeDoorSettings(out.door)) {
      return false;
    }
  }
  if (kind == creative::CreativeObjectKind::Window) {
    if (!creative::parseCreativeWindowInsertKind(record.windowInsertKind,
                                                  out.window.insertKind) ||
        !creative::isValidCreativeWindowSettings(out.window)) {
      return false;
    }
  }
  if (kind == creative::CreativeObjectKind::SpawnPoint) {
    out.playerSpawn.playerProfileId = record.playerSpawnProfileId;
    out.playerSpawn.spawnGroup = record.playerSpawnGroup;
    out.playerSpawn.validationRadiusMeters =
        record.playerSpawnValidationRadiusMeters;
    out.playerSpawn.fallbackPriority = record.playerSpawnFallbackPriority;
    if (!creative::isValidCreativePlayerSpawnSettings(out.playerSpawn)) {
      return false;
    }
  }
  if (kind == creative::CreativeObjectKind::NpcSpawn ||
      kind == creative::CreativeObjectKind::EnemySpawn) {
    if (sectionVersion >= kSaveCreativeDocumentNpcSpawnVersion) {
      out.npcSpawn.behaviorProfileId = record.npcBehaviorProfileId;
      out.npcSpawn.hitPoints = record.npcHitPoints;
      out.npcSpawn.initialAlertLevel = record.npcInitialAlertLevel;
      if (!creative::parseCreativeNpcTeam(record.npcTeam, out.npcSpawn.team) ||
          !creative::parseCreativeNpcSpawnPolicy(
              record.npcSpawnPolicy, out.npcSpawn.spawnPolicy) ||
          !creative::isValidCreativeNpcSpawnSettings(out.npcSpawn)) {
        return false;
      }
    }
  }
  if (kind == creative::CreativeObjectKind::LootPoint &&
      sectionVersion >= kSaveCreativeDocumentObjectiveVersion) {
    out.lootPoint.itemId = record.lootItemId;
    out.lootPoint.itemCount = record.lootItemCount;
    out.lootPoint.deactivateOnCollect = record.lootDeactivateOnCollect;
    if (!creative::isValidCreativeLootPointSettings(out.lootPoint)) {
      return false;
    }
  }
  if (kind == creative::CreativeObjectKind::ExitPoint &&
      sectionVersion >= kSaveCreativeDocumentObjectiveVersion) {
    out.exitPoint.requiredItemId = record.exitRequiredItemId;
    out.exitPoint.requiredItemCount = record.exitRequiredItemCount;
    if (!creative::isValidCreativeExitPointSettings(out.exitPoint)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] SaveCreativeDocumentLogicLinkRecord toSaveLogicLink(
    const creative::CreativeLogicLink& link) {
  SaveCreativeDocumentLogicLinkRecord record;
  record.sourceObjectId = link.sourceObjectId;
  record.targetObjectId = link.targetObjectId;
  record.action = std::string{creative::toString(link.action)};
  return record;
}

[[nodiscard]] bool toCreativeLogicLink(
    const SaveCreativeDocumentLogicLinkRecord& record,
    creative::CreativeLogicLink& out) noexcept {
  out.sourceObjectId = record.sourceObjectId;
  out.targetObjectId = record.targetObjectId;
  return creative::parseCreativeLogicLinkAction(record.action, out.action);
}

}  // namespace iggy3d::document_section_internal
