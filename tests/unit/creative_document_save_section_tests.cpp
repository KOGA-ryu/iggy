#include "app/iggy3d/creative/world/DocumentSection.hpp"
#include "runtime/save/SaveCodec.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

constexpr double kOneThird = 1.0 / 3.0;
constexpr double kPrecise = 0.1234567890123;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string eraseLinesContaining(std::string source,
                                 std::string_view needle) {
  std::size_t searchFrom = 0U;
  while (true) {
    const std::size_t found = source.find(needle, searchFrom);
    if (found == std::string::npos) {
      return source;
    }
    const std::size_t precedingNewline = source.rfind('\n', found);
    const std::size_t lineBegin = precedingNewline == std::string::npos
                                      ? 0U
                                      : precedingNewline + 1U;
    const std::size_t followingNewline = source.find('\n', found);
    const std::size_t lineEnd = followingNewline == std::string::npos
                                    ? source.size()
                                    : followingNewline + 1U;
    source.erase(lineBegin, lineEnd - lineBegin);
    searchFrom = lineBegin;
  }
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameGridSize(cr::CreativeGridSize3 lhs, cr::CreativeGridSize3 rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height &&
         lhs.depth == rhs.depth;
}

bool sameGridSettings(cr::CreativeGridSettings lhs,
                      cr::CreativeGridSettings rhs) {
  return sameVec3(lhs.origin, rhs.origin) &&
         lhs.cellSizeMeters == rhs.cellSizeMeters &&
         sameGridSize(lhs.size, rhs.size);
}

bool sameSnapSettings(cr::CreativeDocumentSnapSettings lhs,
                      cr::CreativeDocumentSnapSettings rhs) {
  return lhs.mode == rhs.mode && lhs.axes == rhs.axes &&
         lhs.stepX == rhs.stepX && lhs.stepY == rhs.stepY &&
         lhs.stepZ == rhs.stepZ && lhs.originX == rhs.originX &&
         lhs.originY == rhs.originY && lhs.originZ == rhs.originZ;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

bool sameTransform(cr::CreativeTransform lhs, cr::CreativeTransform rhs) {
  return sameVec3(lhs.position, rhs.position) &&
         sameVec3(lhs.rotationEulerRadians, rhs.rotationEulerRadians) &&
         sameVec3(lhs.scale, rhs.scale);
}

bool samePathPoints(std::span<const cr::CreativePathPoint> lhs,
                    std::span<const cr::CreativePathPoint> rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (!sameVec3(lhs[index].position, rhs[index].position) ||
        lhs[index].dwellSeconds != rhs[index].dwellSeconds ||
        lhs[index].outgoingSpeedMultiplier !=
            rhs[index].outgoingSpeedMultiplier) {
      return false;
    }
  }
  return true;
}

bool savePathPointsMatch(
    std::span<const iggy3d::SaveCreativeDocumentPathPointRecord> lhs,
    std::span<const cr::CreativePathPoint> rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (lhs[index].x != rhs[index].position.x ||
        lhs[index].y != rhs[index].position.y ||
        lhs[index].z != rhs[index].position.z ||
        lhs[index].dwellSeconds != rhs[index].dwellSeconds ||
        lhs[index].outgoingSpeedMultiplier !=
            rhs[index].outgoingSpeedMultiplier) {
      return false;
    }
  }
  return true;
}

std::vector<cr::CreativePathPoint> authoredPathPoints() {
  return {
      cr::CreativePathPoint{{kOneThird, 0.0, kPrecise}},
      cr::CreativePathPoint{{4.0, kOneThird, 6.0}},
      cr::CreativePathPoint{{7.0, 0.0, 8.0}},
  };
}

std::vector<cr::CreativePathPoint> authoredLineEndpoints() {
  return {
      cr::CreativePathPoint{{kPrecise, 0.0, kOneThird}},
      cr::CreativePathPoint{{4.0, 0.0, 6.0}},
  };
}

std::vector<cr::CreativePathPoint> authoredMovingPlatformPathPoints() {
  std::vector<cr::CreativePathPoint> points = authoredPathPoints();
  points[1].dwellSeconds = 1.25;
  points[1].outgoingSpeedMultiplier = 2.25;
  points[2].dwellSeconds = 0.5;
  points[2].outgoingSpeedMultiplier = 0.75;
  return points;
}

cr::CreativeGridSettings authoredGridSettings() {
  cr::CreativeGridSettings settings;
  settings.origin = {kOneThird, kPrecise, -7.25};
  settings.cellSizeMeters = kOneThird;
  settings.size = {64, 32, 8};
  return settings;
}

cr::CreativeDocumentSnapSettings authoredSnapSettings() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.mode = cr::CreativeDocumentSnapMode::Grid;
  settings.axes = cr::kCreativeDocumentSnapAxisXZ;
  settings.stepX = kPrecise;
  settings.stepY = 2.5;
  settings.stepZ = kOneThird;
  settings.originX = -1.25;
  settings.originY = kOneThird;
  settings.originZ = 4.75;
  return settings;
}

cr::CreativeBounds authoredWorldBounds() {
  return {{-8.0, -1.0, -4.0}, {64.0, 32.0, 8.0}};
}

cr::CreativeObject restoredGroupObject() {
  cr::CreativeObject object;
  object.id = 2;
  object.kind = cr::CreativeObjectKind::Group;
  object.name = "Root Group";
  object.layerId = 4;
  object.visible = true;
  object.locked = false;
  object.tags = {"container", "roundtrip"};
  return object;
}

cr::CreativeObject restoredWallObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Wall;
  object.name = "Precise Wall";
  object.assetId = "architecture/wall_oak";
  object.assetContentHash = 0x1122334455667788ULL;
  object.assetMaterialVariant = "Weathered Oak";
  object.transform.position = {kOneThird, 2.0, kPrecise};
  object.transform.rotationEulerRadians = {0.0, kPrecise, 1.5};
  object.transform.scale = {1.0, 2.0, 3.0};
  object.bounds = {{0.0, -0.25, kOneThird}, {10.5, 4.25, 7.75}};
  object.layerId = 9;
  object.visible = false;
  object.locked = true;
  object.parentId = 2;
  object.attachmentSocket = "door_frame";
  object.tags = {"wall", "imported"};
  return object;
}

cr::CreativeObject restoredPatrolRouteObject() {
  cr::CreativeObject object;
  object.id = 11;
  object.kind = cr::CreativeObjectKind::PatrolRoute;
  object.name = "Precise Route";
  object.layerId = 5;
  object.visible = true;
  object.locked = false;
  object.tags = {"route", "patrol"};
  object.pathPoints = authoredPathPoints();
  return object;
}

cr::CreativeObject restoredNavLinkObject() {
  cr::CreativeObject object;
  object.id = 12;
  object.kind = cr::CreativeObjectKind::NavLink;
  object.name = "Precise Link";
  object.layerId = 6;
  object.visible = true;
  object.locked = false;
  object.tags = {"link", "navigation"};
  object.pathPoints = authoredLineEndpoints();
  return object;
}

cr::CreativeObject restoredMovingPlatformObject() {
  cr::CreativeObject object;
  object.id = 14;
  object.kind = cr::CreativeObjectKind::MovingPlatform;
  object.name = "Freight Lift";
  object.bounds = {{2.0, 0.25, 3.0}, {5.0, 0.5, 6.0}};
  object.layerId = 8;
  object.visible = true;
  object.locked = false;
  object.tags = {"platform", "freight"};
  object.pathPoints = authoredMovingPlatformPathPoints();
  object.movingPlatform.speedMetersPerSecond = 2.75;
  object.movingPlatform.traversalMode =
      cr::CreativeMovingPlatformTraversalMode::Loop;
  object.movingPlatform.startsActive = false;
  return object;
}

cr::CreativeObject restoredDoorObject() {
  cr::CreativeObject object;
  object.id = 15;
  object.kind = cr::CreativeObjectKind::Door;
  object.name = "Locked Double Door";
  object.bounds = {{2.0, 0.25, 3.0}, {3.8, 2.65, 3.16}};
  object.layerId = 9;
  object.visible = true;
  object.locked = false;
  object.tags = {"door", "roundtrip"};
  object.door.leafArrangement = cr::CreativeDoorLeafArrangement::Double;
  object.door.hingeSide = cr::CreativeDoorHingeSide::MaximumEdge;
  object.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  object.door.initialState = cr::CreativeDoorInitialState::Open;
  object.door.gameplayLocked = true;
  object.door.transitionSeconds = 0.8;
  return object;
}

cr::CreativeObject restoredWindowObject() {
  cr::CreativeObject object;
  object.id = 16;
  object.kind = cr::CreativeObjectKind::Window;
  object.name = "Paired Shutter Window";
  object.bounds = {{4.0, 1.0, 3.0}, {5.5, 2.2, 3.12}};
  object.layerId = 9;
  object.visible = true;
  object.locked = false;
  object.tags = {"window", "roundtrip"};
  object.window.insertKind = cr::CreativeWindowInsertKind::PairedShutters;
  return object;
}

cr::CreativeObject restoredPlayerSpawnObject() {
  cr::CreativeObject object;
  object.id = 17;
  object.kind = cr::CreativeObjectKind::SpawnPoint;
  object.name = "North Entry Spawn";
  object.transform.position = {2.0, 0.25, -3.0};
  object.transform.rotationEulerRadians.y = 1.25;
  object.layerId = 10;
  object.visible = true;
  object.locked = false;
  object.tags = {"spawn", "north_entry"};
  object.playerSpawn.playerProfileId = "default";
  object.playerSpawn.spawnGroup = "north_entry";
  object.playerSpawn.validationRadiusMeters = 0.75;
  object.playerSpawn.fallbackPriority = 4U;
  return object;
}

cr::CreativeObject restoredNpcSpawnObject() {
  cr::CreativeObject object;
  object.id = 18;
  object.kind = cr::CreativeObjectKind::NpcSpawn;
  object.name = "Alert Courtyard Guard";
  object.transform.position = {4.0, 0.25, 2.0};
  object.transform.rotationEulerRadians.y = -0.75;
  object.layerId = 10;
  object.visible = true;
  object.locked = false;
  object.tags = {"npc", "courtyard"};
  object.npcSpawn.behaviorProfileId = "default";
  object.npcSpawn.team = cr::CreativeNpcTeam::Hostile;
  object.npcSpawn.hitPoints = 37U;
  object.npcSpawn.initialAlertLevel = 0.6;
  object.npcSpawn.spawnPolicy = cr::CreativeNpcSpawnPolicy::Disabled;
  return object;
}

cr::CreativeDocumentRestoreRequest authoredRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9001;
  request.name = "Round Trip Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 100;
  request.objects = {restoredGroupObject(), restoredWallObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredPathRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9002;
  request.name = "Path Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 25;
  request.objects = {restoredPatrolRouteObject(), restoredNavLinkObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredMovingPlatformRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9003;
  request.name = "Moving Platform Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 30;
  request.objects = {restoredMovingPlatformObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredDoorRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9004;
  request.name = "Door Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 30;
  request.objects = {restoredDoorObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredWindowRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9005;
  request.name = "Window Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 30;
  request.objects = {restoredWindowObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredPlayerSpawnRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9006;
  request.name = "Player Spawn Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 30;
  request.objects = {restoredPlayerSpawnObject()};
  return request;
}

cr::CreativeDocumentRestoreRequest authoredNpcSpawnRestoreRequest() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9007;
  request.name = "NPC Spawn Creative";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = authoredGridSettings();
  request.snapSettings = authoredSnapSettings();
  request.worldBounds = authoredWorldBounds();
  request.nextObjectId = 30;
  request.objects = {restoredNpcSpawnObject()};
  return request;
}

cr::CreativeDocument authoredDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Before");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored document restore setup\n";
  }
  return document;
}

cr::CreativeDocument authoredPathDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Before Path");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredPathRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored path document restore setup\n";
  }
  return document;
}

cr::CreativeDocument authoredMovingPlatformDocument() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Before Moving Platform");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredMovingPlatformRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored moving platform document restore setup\n";
  }
  return document;
}

cr::CreativeDocument authoredDoorDocument() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Before Door");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredDoorRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored door document restore setup\n";
  }
  return document;
}

cr::CreativeDocument authoredWindowDocument() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Before Window");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredWindowRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored window document restore setup\n";
  }
  return document;
}

cr::CreativeDocument authoredPlayerSpawnDocument() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Before Player Spawn");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredPlayerSpawnRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored player spawn document restore setup\n";
  }
  return document;
}

cr::CreativeDocument authoredNpcSpawnDocument() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Before NPC Spawn");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(authoredNpcSpawnRestoreRequest());
  if (!restored.accepted) {
    std::cerr << "FAIL: authored npc spawn document restore setup\n";
  }
  return document;
}

iggy3d::SaveEnvelope minimalEnvelope() {
  iggy3d::SaveEnvelope envelope;
  envelope.metadata.savedStateHash = 0;
  envelope.metadata.savedStateHashHex = "0000000000000000";
  return envelope;
}

bool sectionObjectMatches(const iggy3d::SaveCreativeDocumentObjectRecord& save,
                          const cr::CreativeObject& object) {
  return save.id == object.id &&
         save.kind == std::string{cr::serializedObjectKindId(object.kind)} &&
         save.name == object.name && save.assetId == object.assetId &&
         save.assetContentHash == object.assetContentHash &&
         save.assetMaterialVariant == object.assetMaterialVariant &&
         save.transform.position.x == object.transform.position.x &&
         save.transform.position.y == object.transform.position.y &&
         save.transform.position.z == object.transform.position.z &&
         save.transform.rotation.x == object.transform.rotationEulerRadians.x &&
         save.transform.rotation.y == object.transform.rotationEulerRadians.y &&
         save.transform.rotation.z == object.transform.rotationEulerRadians.z &&
         save.transform.scale.x == object.transform.scale.x &&
         save.transform.scale.y == object.transform.scale.y &&
         save.transform.scale.z == object.transform.scale.z &&
         save.bounds.min.x == object.bounds.min.x &&
         save.bounds.min.y == object.bounds.min.y &&
         save.bounds.min.z == object.bounds.min.z &&
         save.bounds.max.x == object.bounds.max.x &&
         save.bounds.max.y == object.bounds.max.y &&
         save.bounds.max.z == object.bounds.max.z &&
         save.layerId == object.layerId && save.visible == object.visible &&
         save.locked == object.locked &&
         save.hasParent == object.parentId.has_value() &&
         save.parentId == object.parentId.value_or(cr::kInvalidObjectId) &&
         save.attachmentSocket == object.attachmentSocket &&
         save.tags == object.tags &&
         savePathPointsMatch(save.pathPoints, object.pathPoints) &&
         save.movingPlatformSpeedMetersPerSecond ==
             object.movingPlatform.speedMetersPerSecond &&
         save.movingPlatformTraversalMode ==
             std::string{cr::toString(object.movingPlatform.traversalMode)} &&
         save.movingPlatformStartsActive == object.movingPlatform.startsActive &&
         save.doorLeafArrangement ==
             std::string{cr::toString(object.door.leafArrangement)} &&
         save.doorHingeSide ==
             std::string{cr::toString(object.door.hingeSide)} &&
         save.doorSwingSide ==
             std::string{cr::toString(object.door.swingSide)} &&
         save.doorInitialState ==
             std::string{cr::toString(object.door.initialState)} &&
         save.doorGameplayLocked == object.door.gameplayLocked &&
         save.doorTransitionSeconds == object.door.transitionSeconds &&
         save.windowInsertKind ==
             std::string{cr::toString(object.window.insertKind)} &&
         save.playerSpawnProfileId == object.playerSpawn.playerProfileId &&
         save.playerSpawnGroup == object.playerSpawn.spawnGroup &&
         save.playerSpawnValidationRadiusMeters ==
             object.playerSpawn.validationRadiusMeters &&
         save.playerSpawnFallbackPriority ==
             object.playerSpawn.fallbackPriority &&
         save.npcBehaviorProfileId == object.npcSpawn.behaviorProfileId &&
         save.npcTeam == std::string{cr::toString(object.npcSpawn.team)} &&
         save.npcHitPoints == object.npcSpawn.hitPoints &&
         save.npcInitialAlertLevel == object.npcSpawn.initialAlertLevel &&
         save.npcSpawnPolicy ==
             std::string{cr::toString(object.npcSpawn.spawnPolicy)};
}

bool documentObjectMatches(const cr::CreativeObject& lhs,
                           const cr::CreativeObject& rhs) {
  return lhs.id == rhs.id && lhs.kind == rhs.kind && lhs.name == rhs.name &&
         lhs.assetId == rhs.assetId &&
         lhs.assetContentHash == rhs.assetContentHash &&
         lhs.assetMaterialVariant == rhs.assetMaterialVariant &&
         sameTransform(lhs.transform, rhs.transform) &&
         sameBounds(lhs.bounds, rhs.bounds) && lhs.layerId == rhs.layerId &&
         lhs.visible == rhs.visible && lhs.locked == rhs.locked &&
         lhs.parentId == rhs.parentId &&
         lhs.attachmentSocket == rhs.attachmentSocket &&
         lhs.tags == rhs.tags &&
         samePathPoints(lhs.pathPoints, rhs.pathPoints) &&
         lhs.movingPlatform == rhs.movingPlatform && lhs.door == rhs.door &&
         lhs.window == rhs.window && lhs.playerSpawn == rhs.playerSpawn &&
         lhs.npcSpawn == rhs.npcSpawn;
}

bool buildSectionCopiesDocumentExactly() {
  const cr::CreativeDocument document = authoredDocument();
  const cr::CreativeDocumentRestoreRequest original = authoredRestoreRequest();
  const cr::CreativeObject& group = original.objects[0];
  const cr::CreativeObject& wall = original.objects[1];
  const iggy3d::ProductCreativeDocumentSectionBuildResult result =
      iggy3d::buildSaveCreativeDocumentSection(document);
  const iggy3d::SaveCreativeDocumentSection& section = result.section;

  return expect(result.receipt.requested, "build receipt requested") &&
         expect(result.receipt.accepted, "build receipt accepted") &&
         expect(result.receipt.changed, "build receipt changed") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::Converted,
                "build receipt converted") &&
         expect(result.receipt.documentId == 9001U,
                "build receipt document id") &&
         expect(result.receipt.objectCount == 2U,
                "build receipt object count") &&
         expect(result.receipt.nextObjectId == 100U,
                "build receipt next id") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_section_converted",
                "build receipt reason") &&
         expect(section.present, "section present") &&
         expect(section.version == iggy3d::kSaveCreativeDocumentSectionVersion,
                "section version") &&
         expect(section.documentId == document.id(), "section document id") &&
         expect(section.name == document.name(), "section name") &&
         expect(section.units == "Meters", "section units") &&
         expect(section.gridOrigin.x == document.gridSettings().origin.x &&
                    section.gridOrigin.y == document.gridSettings().origin.y &&
                    section.gridOrigin.z == document.gridSettings().origin.z,
                "section grid origin") &&
         expect(section.cellSizeMeters ==
                    document.gridSettings().cellSizeMeters,
                "section grid cell size") &&
         expect(section.gridWidth ==
                    static_cast<std::uint32_t>(
                        document.gridSettings().size.width) &&
                    section.gridHeight ==
                        static_cast<std::uint32_t>(
                            document.gridSettings().size.height) &&
                    section.gridDepth ==
                        static_cast<std::uint32_t>(
                            document.gridSettings().size.depth),
                "section grid dimensions") &&
         expect(section.snapMode == "Grid", "section snap mode") &&
         expect(section.snapAxes == document.documentSnapSettings().axes,
                "section snap axes") &&
         expect(section.snapStepX == document.documentSnapSettings().stepX &&
                    section.snapStepY == document.documentSnapSettings().stepY &&
                    section.snapStepZ == document.documentSnapSettings().stepZ,
                "section snap steps") &&
         expect(section.worldBounds.min.x == document.worldBounds().min.x &&
                    section.worldBounds.max.z == document.worldBounds().max.z,
                "section world bounds") &&
         expect(section.nextObjectId == document.nextObjectId(),
                "section next id") &&
         expect(section.objects.size() == 2U, "section object count") &&
         expect(sectionObjectMatches(section.objects[0], group),
                "section group object") &&
         expect(sectionObjectMatches(section.objects[1], wall),
                "section wall object");
}

bool encodeDecodeAndRestoreRoundTripsDocument() {
  const cr::CreativeDocument document = authoredDocument();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const bool encodedOmitsRevision =
      encoded.encodedText.find("creativeDocument.revision") ==
      std::string::npos;
  const bool encodedOmitsDirtyFlags =
      encoded.encodedText.find("creativeDocument.dirty") == std::string::npos;
  const bool preciseDoubleSurvivedText =
      encoded.encodedText.find("0.333\n") == std::string::npos &&
      encoded.encodedText.find("0.123\n") == std::string::npos;
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeDocument& restoredDocument = restored.document;
  const cr::CreativeObject* restoredGroup = restoredDocument.findObject(2);
  const cr::CreativeObject* restoredWall = restoredDocument.findObject(7);
  const cr::CreativeDocumentRestoreRequest original =
      authoredRestoreRequest();

  return expect(built.receipt.accepted, "round trip build accepted") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "round trip encode ok") &&
         expect(encodedOmitsRevision, "encoded omits creative revision") &&
         expect(encodedOmitsDirtyFlags, "encoded omits dirty flags") &&
         expect(preciseDoubleSurvivedText,
                "creative doubles not three-decimal formatted") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "round trip decode ok") &&
         expect(decoded.envelope.creativeDocument.present,
                "decoded creative section present") &&
         expect(restored.receipt.accepted, "restore receipt accepted") &&
         expect(restored.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::Converted,
                "restore receipt converted") &&
         expect(restored.receipt.documentId == original.documentId,
                "restore receipt id") &&
         expect(restored.receipt.objectCount == original.objects.size(),
                "restore receipt object count") &&
         expect(restored.receipt.nextObjectId == original.nextObjectId,
                "restore receipt next id") &&
         expect(restoredDocument.id() == document.id(), "restored id") &&
         expect(restoredDocument.name() == document.name(), "restored name") &&
         expect(restoredDocument.units() == cr::CreativeUnits::Meters,
                "restored units") &&
         expect(sameGridSettings(restoredDocument.gridSettings(),
                                 document.gridSettings()),
                "restored grid") &&
         expect(sameSnapSettings(restoredDocument.documentSnapSettings(),
                                 document.documentSnapSettings()),
                "restored snap") &&
         expect(sameBounds(restoredDocument.worldBounds(),
                           document.worldBounds()),
                "restored world bounds") &&
         expect(restoredDocument.nextObjectId() == 100U,
                "restored exact next id") &&
         expect(restoredDocument.revision() == 0U,
                "restored revision zero") &&
         expect(restoredDocument.dirtyFlags() == 0U,
                "restored dirty zero") &&
         expect(restoredDocument.objectCount() == 2U,
                "restored object count") &&
         expect(restoredGroup != nullptr &&
                    documentObjectMatches(*restoredGroup, original.objects[0]),
                "restored group") &&
         expect(restoredWall != nullptr &&
                    documentObjectMatches(*restoredWall, original.objects[1]),
                "restored wall") &&
         expect(restoredWall != nullptr &&
                    restoredWall->transform.position.x == kOneThird &&
                    restoredWall->transform.position.z == kPrecise &&
                    restoredDocument.gridSettings().cellSizeMeters ==
                        kOneThird &&
                    restoredDocument.documentSnapSettings().stepX == kPrecise,
                "precise doubles restored exactly");
}

bool buildSectionCopiesPathPoints() {
  const cr::CreativeDocument document = authoredPathDocument();
  const cr::CreativeDocumentRestoreRequest original =
      authoredPathRestoreRequest();
  const cr::CreativeObject& route = original.objects[0];
  const cr::CreativeObject& link = original.objects[1];
  const iggy3d::ProductCreativeDocumentSectionBuildResult result =
      iggy3d::buildSaveCreativeDocumentSection(document);
  const iggy3d::SaveCreativeDocumentSection& section = result.section;

  return expect(result.receipt.accepted, "path build accepted") &&
         expect(section.objects.size() == 2U, "path build object count") &&
         expect(section.objects[0].kind == "PatrolRoute",
                "path build kind") &&
         expect(section.objects[0].pathPoints.size() == 3U,
                "path build path count") &&
         expect(savePathPointsMatch(section.objects[0].pathPoints,
                                    route.pathPoints),
                "path build points exact") &&
         expect(section.objects[1].kind == "NavLink",
                "line endpoint build kind") &&
         expect(section.objects[1].pathPoints.size() == 2U,
                "line endpoint build path count") &&
         expect(savePathPointsMatch(section.objects[1].pathPoints,
                                    link.pathPoints),
                "line endpoint build points exact");
}

bool restoreSectionRestoresPathPoints() {
  const cr::CreativeDocument document = authoredPathDocument();
  const iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(document).section;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);
  const cr::CreativeObject* route = result.document.findObject(11);
  const cr::CreativeObject* link = result.document.findObject(12);

  return expect(result.receipt.accepted, "path restore accepted") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::Converted,
                "path restore converted") &&
         expect(result.document.id() == 9002U, "path restore document id") &&
         expect(result.document.objectCount() == 2U,
                "path restore object count") &&
         expect(result.document.nextObjectId() == 25U,
                "path restore next object id") &&
         expect(result.document.revision() == 0U,
                "path restore revision zero") &&
         expect(result.document.dirtyFlags() == 0U,
                "path restore dirty zero") &&
         expect(route != nullptr, "path restore route findable") &&
         expect(route != nullptr &&
                    samePathPoints(route->pathPoints, authoredPathPoints()),
                "path restore points exact") &&
         expect(link != nullptr, "line endpoint restore link findable") &&
         expect(link != nullptr &&
                    samePathPoints(link->pathPoints,
                                   authoredLineEndpoints()),
                "line endpoint restore points exact");
}

bool encodeDecodeAndRestoreRoundTripsPathAndLineEndpointPayloads() {
  const cr::CreativeDocument document = authoredPathDocument();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* route = restored.document.findObject(11);
  const cr::CreativeObject* link = restored.document.findObject(12);

  return expect(built.receipt.accepted,
                "path line endpoint codec build accepted") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "path line endpoint codec encode ok") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.1.kind=NavLink\n") !=
                    std::string::npos,
                "line endpoint kind encoded") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.1.pathPoint.count=2\n") !=
                    std::string::npos,
                "line endpoint point count encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "path line endpoint codec decode ok") &&
         expect(restored.receipt.accepted,
                "path line endpoint codec restore accepted") &&
         expect(restored.document.objectCount() == 2U,
                "path line endpoint codec object count") &&
         expect(route != nullptr, "path line endpoint codec route findable") &&
         expect(route != nullptr &&
                    samePathPoints(route->pathPoints, authoredPathPoints()),
                "path line endpoint codec route points") &&
         expect(link != nullptr, "path line endpoint codec link findable") &&
         expect(link != nullptr &&
                    samePathPoints(link->pathPoints,
                                   authoredLineEndpoints()),
                "path line endpoint codec link endpoints");
}

bool movingPlatformSettingsEncodeDecodeAndRestore() {
  const cr::CreativeDocument document = authoredMovingPlatformDocument();
  const cr::CreativeObject original = restoredMovingPlatformObject();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* restoredObject =
      restored.document.findObject(original.id);

  return expect(built.receipt.accepted,
                "moving platform codec build accepted") &&
         expect(built.section.objects.size() == 1U,
                "moving platform codec object count") &&
         expect(sectionObjectMatches(built.section.objects.front(), original),
                "moving platform save record exact") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "moving platform codec encode ok") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.0.movingPlatform."
                    "speedMetersPerSecond=2.75\n") != std::string::npos,
                "moving platform speed encoded") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.0.movingPlatform."
                    "traversalMode=Loop\n") != std::string::npos,
                "moving platform traversal encoded") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.0.movingPlatform."
                    "startsActive=false\n") != std::string::npos,
                "moving platform start state encoded") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.0.pathPoint.1."
                    "dwellSeconds=1.25\n") != std::string::npos,
                "moving platform waypoint dwell encoded") &&
         expect(encoded.encodedText.find(
                    "creativeDocument.object.0.pathPoint.1."
                    "outgoingSpeedMultiplier=2.25\n") !=
                    std::string::npos,
                "moving platform segment speed encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "moving platform codec decode ok") &&
         expect(decodedObject != nullptr &&
                    sectionObjectMatches(*decodedObject, original),
                "moving platform decoded record exact") &&
         expect(restored.receipt.accepted,
                "moving platform restore accepted") &&
         expect(restoredObject != nullptr &&
                    documentObjectMatches(*restoredObject, original),
                "moving platform restored object exact");
}

bool doorSettingsEncodeDecodeRestoreAndLegacyDefault() {
  const cr::CreativeDocument document = authoredDoorDocument();
  const cr::CreativeObject original = restoredDoorObject();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* restoredObject =
      restored.document.findObject(original.id);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentDoorVersion - 1U;
  legacy.objects.front().doorLeafArrangement = "Single";
  legacy.objects.front().doorHingeSide = "MinimumEdge";
  legacy.objects.front().doorSwingSide = "PositiveNormal";
  legacy.objects.front().doorInitialState = "Closed";
  legacy.objects.front().doorGameplayLocked = false;
  legacy.objects.front().doorTransitionSeconds = 0.35;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult migrated =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);
  const cr::CreativeObject* migratedDoor =
      migrated.document.findObject(original.id);

  return expect(built.receipt.accepted && built.section.objects.size() == 1U,
                "door codec build accepted") &&
         expect(sectionObjectMatches(built.section.objects.front(), original),
                "door save record owns every semantic setting") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.door.leafArrangement=Double\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.door.hingeSide=MaximumEdge\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.door.swingSide=NegativeNormal\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.door.initialState=Open\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.door.gameplayLocked=true\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.door.transitionSeconds=0.8\n") !=
                        std::string::npos,
                "door settings encode explicitly and losslessly") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decodedObject != nullptr &&
                    sectionObjectMatches(*decodedObject, original),
                "door settings decode exactly") &&
         expect(restored.receipt.accepted && restoredObject != nullptr &&
                    documentObjectMatches(*restoredObject, original),
                "door settings restore exactly") &&
         expect(migrated.receipt.accepted && migratedDoor != nullptr &&
                    migratedDoor->door == cr::CreativeDoorSettings{},
                "pre-door document versions receive safe door defaults");
}

bool windowSettingsEncodeDecodeRestoreAndLegacyDefault() {
  const cr::CreativeDocument document = authoredWindowDocument();
  const cr::CreativeObject original = restoredWindowObject();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* restoredObject =
      restored.document.findObject(original.id);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentWindowVersion - 1U;
  legacy.objects.front().windowInsertKind = "Glazing";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult migrated =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);
  const cr::CreativeObject* migratedWindow =
      migrated.document.findObject(original.id);

  return expect(built.receipt.accepted && built.section.objects.size() == 1U,
                "window codec build accepted") &&
         expect(sectionObjectMatches(built.section.objects.front(), original),
                "window save record owns insert treatment") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.window.insertKind="
                        "PairedShutters\n") != std::string::npos,
                "window treatment encodes explicitly") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decodedObject != nullptr &&
                    sectionObjectMatches(*decodedObject, original),
                "window treatment decodes exactly") &&
         expect(restored.receipt.accepted && restoredObject != nullptr &&
                    documentObjectMatches(*restoredObject, original),
                "window treatment restores exactly") &&
         expect(migrated.receipt.accepted && migratedWindow != nullptr &&
                    migratedWindow->window == cr::CreativeWindowSettings{},
                "pre-window document versions receive glazing default");
}

bool playerSpawnSettingsEncodeDecodeRestoreAndLegacyDefault() {
  const cr::CreativeDocument document = authoredPlayerSpawnDocument();
  const cr::CreativeObject original = restoredPlayerSpawnObject();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* restoredObject =
      restored.document.findObject(original.id);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentPlayerSpawnVersion - 1U;
  legacy.objects.front().playerSpawnProfileId = "default";
  legacy.objects.front().playerSpawnGroup = "default";
  legacy.objects.front().playerSpawnValidationRadiusMeters = 0.45;
  legacy.objects.front().playerSpawnFallbackPriority = 0U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult migrated =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);
  const cr::CreativeObject* migratedSpawn =
      migrated.document.findObject(original.id);

  return expect(built.receipt.accepted && built.section.objects.size() == 1U,
                "player spawn codec build accepted") &&
         expect(sectionObjectMatches(built.section.objects.front(), original),
                "player spawn save record owns every setting") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.playerSpawn.profileId="
                        "default\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.playerSpawn.group="
                        "north_entry\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.playerSpawn."
                        "validationRadiusMeters=0.75\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.playerSpawn."
                        "fallbackPriority=4\n") != std::string::npos,
                "player spawn settings encode explicitly") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decodedObject != nullptr &&
                    sectionObjectMatches(*decodedObject, original),
                "player spawn settings decode exactly") &&
         expect(restored.receipt.accepted && restoredObject != nullptr &&
                    documentObjectMatches(*restoredObject, original),
                "player spawn settings restore exactly") &&
         expect(migrated.receipt.accepted && migratedSpawn != nullptr &&
                    migratedSpawn->playerSpawn ==
                        cr::CreativePlayerSpawnSettings{},
                "pre-player-spawn versions receive safe defaults");
}

bool npcSpawnSettingsEncodeDecodeRestoreAndLegacyDefault() {
  const cr::CreativeDocument document = authoredNpcSpawnDocument();
  const cr::CreativeObject original = restoredNpcSpawnObject();
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;

  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveCreativeDocumentObjectRecord* decodedObject =
      decoded.envelope.creativeDocument.objects.empty()
          ? nullptr
          : &decoded.envelope.creativeDocument.objects.front();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeObject* restoredObject =
      restored.document.findObject(original.id);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentNpcSpawnVersion - 1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult migrated =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);
  const cr::CreativeObject* migratedNpc =
      migrated.document.findObject(original.id);

  iggy3d::SaveCreativeDocumentSection malformed = built.section;
  malformed.objects.front().npcTeam = "Unknown";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult rejected =
      iggy3d::restoreCreativeDocumentFromSaveSection(malformed);

  return expect(built.receipt.accepted && built.section.objects.size() == 1U,
                "npc spawn codec build accepted") &&
         expect(sectionObjectMatches(built.section.objects.front(), original),
                "npc spawn save record owns every setting") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.npcSpawn."
                        "behaviorProfileId=default\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.npcSpawn.team=Hostile\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.npcSpawn.hitPoints=37\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.npcSpawn."
                        "initialAlertLevel=0.6\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.object.0.npcSpawn."
                        "spawnPolicy=Disabled\n") != std::string::npos,
                "npc spawn settings encode explicitly") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decodedObject != nullptr &&
                    sectionObjectMatches(*decodedObject, original),
                "npc spawn settings decode exactly") &&
         expect(restored.receipt.accepted && restoredObject != nullptr &&
                    documentObjectMatches(*restoredObject, original),
                "npc spawn settings restore exactly") &&
         expect(migrated.receipt.accepted && migratedNpc != nullptr &&
                    migratedNpc->npcSpawn == cr::CreativeNpcSpawnSettings{},
                "pre-npc-spawn versions receive safe defaults") &&
         expect(!rejected.receipt.accepted &&
                    rejected.document.objectCount() == 0U,
                "current invalid npc settings fail closed");
}

bool legacyMovingPlatformReceivesDefaultRouteAndSettings() {
  iggy3d::SaveCreativeDocumentSection legacy =
      iggy3d::buildSaveCreativeDocumentSection(
          authoredMovingPlatformDocument()).section;
  legacy.version = 7U;
  legacy.objects.front().pathPoints.clear();
  legacy.objects.front().movingPlatformSpeedMetersPerSecond = 1.5;
  legacy.objects.front().movingPlatformTraversalMode = "PingPong";
  legacy.objects.front().movingPlatformStartsActive = true;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult migrated =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);
  const cr::CreativeObject* platform = migrated.document.findObject(14U);

  iggy3d::SaveCreativeDocumentSection malformedCurrent = legacy;
  malformedCurrent.version = iggy3d::kSaveCreativeDocumentSectionVersion;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult rejected =
      iggy3d::restoreCreativeDocumentFromSaveSection(malformedCurrent);

  return expect(migrated.receipt.accepted,
                "legacy moving platform restore accepted") &&
         expect(platform != nullptr && platform->pathPoints.size() == 2U &&
                    sameVec3(platform->pathPoints[0].position,
                             {3.5, 0.375, 4.5}) &&
                    sameVec3(platform->pathPoints[1].position,
                             {3.5, 3.375, 4.5}),
                "legacy moving platform receives vertical default route") &&
         expect(platform != nullptr &&
                    platform->movingPlatform ==
                        cr::CreativeMovingPlatformSettings{},
                "legacy moving platform receives safe motion defaults") &&
         expect(!rejected.receipt.accepted &&
                    rejected.receipt.reasonCode == "invalid_path_points",
                "current moving platform save still requires authored route");
}

bool restoreRejectsInvalidPathPayloads() {
  const iggy3d::SaveCreativeDocumentSection valid =
      iggy3d::buildSaveCreativeDocumentSection(authoredPathDocument()).section;

  iggy3d::SaveCreativeDocumentSection missing = valid;
  missing.objects[0].pathPoints.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult missingResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(missing);

  iggy3d::SaveCreativeDocumentSection onePoint = valid;
  onePoint.objects[0].pathPoints.resize(1U);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult onePointResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(onePoint);

  iggy3d::SaveCreativeDocumentSection nonFinite = valid;
  nonFinite.objects[0].pathPoints[1].x =
      std::numeric_limits<double>::quiet_NaN();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult nonFiniteResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(nonFinite);

  iggy3d::SaveCreativeDocumentSection invalidSegmentSpeed = valid;
  invalidSegmentSpeed.objects[0].pathPoints[0].outgoingSpeedMultiplier = 0.0;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      invalidSegmentSpeedResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(
              invalidSegmentSpeed);

  return expect(!missingResult.receipt.accepted,
                "missing path points rejected") &&
         expect(missingResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "missing path points status") &&
         expect(missingResult.receipt.reasonCode == "invalid_path_points",
                "missing path points reason") &&
         expect(missingResult.document.objectCount() == 0U,
                "missing path returns empty document") &&
         expect(!onePointResult.receipt.accepted,
                "one path point rejected") &&
         expect(onePointResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "one path point status") &&
         expect(onePointResult.receipt.reasonCode == "invalid_path_points",
                "one path point reason") &&
         expect(!nonFiniteResult.receipt.accepted,
                "nonfinite path point rejected") &&
         expect(nonFiniteResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "nonfinite path point status") &&
         expect(nonFiniteResult.receipt.reasonCode == "invalid_path_points",
                "nonfinite path point reason") &&
         expect(!invalidSegmentSpeedResult.receipt.accepted &&
                    invalidSegmentSpeedResult.receipt.reasonCode ==
                        "invalid_path_points",
                "out-of-range segment speed rejects restored path");
}

bool restoreRejectsInvalidLineEndpointPayloads() {
  const iggy3d::SaveCreativeDocumentSection valid =
      iggy3d::buildSaveCreativeDocumentSection(authoredPathDocument()).section;

  iggy3d::SaveCreativeDocumentSection tooMany = valid;
  tooMany.objects[1].pathPoints.push_back({7.0, 0.0, 8.0});
  const iggy3d::ProductCreativeDocumentSectionRestoreResult tooManyResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(tooMany);

  iggy3d::SaveCreativeDocumentSection nonFinite = valid;
  nonFinite.objects[1].pathPoints[0].x =
      std::numeric_limits<double>::quiet_NaN();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult nonFiniteResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(nonFinite);

  return expect(!tooManyResult.receipt.accepted,
                "too many line endpoints rejected") &&
         expect(tooManyResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "too many line endpoints status") &&
         expect(tooManyResult.receipt.reasonCode == "invalid_line_endpoints",
                "too many line endpoints reason") &&
         expect(!nonFiniteResult.receipt.accepted,
                "nonfinite line endpoint rejected") &&
         expect(nonFiniteResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "nonfinite line endpoint status") &&
         expect(nonFiniteResult.receipt.reasonCode == "invalid_line_endpoints",
                "nonfinite line endpoint reason");
}

bool restoreRejectsNonPathObjectCarryingPathPoints() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "nonpath path setup object count");
  }
  section.objects[1].pathPoints = {
      {1.0, 0.0, 2.0},
      {3.0, 0.0, 4.0},
  };
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "nonpath path points rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "nonpath path points status") &&
         expect(result.receipt.reasonCode == "path_unsupported",
                "nonpath path points reason") &&
         expect(result.document.objectCount() == 0U,
                "nonpath path returns empty document");
}

bool restoreRejectsUnsupportedParentPayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "unsupported parent setup object count");
  }
  section.objects[1].kind = "Room";
  section.objects[1].name = "Unsupported Parented Room";

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "unsupported parent restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "unsupported parent restore status") &&
         expect(result.receipt.reasonCode == "parent_unsupported",
                "unsupported parent restore reason") &&
         expect(result.document.objectCount() == 0U,
                "unsupported parent returns empty document");
}

bool restoreRejectsMissingParentPayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "missing parent setup object count");
  }
  section.objects[1].parentId = 404;

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "missing parent restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "missing parent restore status") &&
         expect(result.receipt.reasonCode == "missing_parent",
                "missing parent restore reason") &&
         expect(result.document.objectCount() == 0U,
                "missing parent returns empty document");
}

bool restoreRejectsUnsupportedParentOwnerPayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "unsupported owner setup object count");
  }

  iggy3d::SaveCreativeDocumentObjectRecord crateParent = section.objects[1];
  crateParent.id = 6;
  crateParent.kind = "Crate";
  crateParent.name = "Unsupported Owner Crate";
  crateParent.hasParent = false;
  crateParent.parentId = cr::kInvalidObjectId;
  crateParent.attachmentSocket.clear();

  iggy3d::SaveCreativeDocumentObjectRecord wallChild = section.objects[1];
  wallChild.parentId = crateParent.id;
  section.objects = {section.objects[0], crateParent, wallChild};

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted,
                "unsupported owner restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "unsupported owner restore status") &&
         expect(result.receipt.reasonCode == "parent_owner_unsupported",
                "unsupported owner restore reason") &&
         expect(result.document.objectCount() == 0U,
                "unsupported owner returns empty document");
}

bool restoreRejectsParentCyclePayload() {
  iggy3d::SaveCreativeDocumentSection section =
      iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
  if (section.objects.size() < 2U) {
    return expect(false, "parent cycle setup object count");
  }

  section.objects[0].kind = "Group";
  section.objects[0].hasParent = true;
  section.objects[0].parentId = section.objects[1].id;
  section.objects[1].kind = "Group";
  section.objects[1].name = "Cycled Group";
  section.objects[1].hasParent = true;
  section.objects[1].parentId = section.objects[0].id;

  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "parent cycle restore rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidObject,
                "parent cycle restore status") &&
         expect(result.receipt.reasonCode == "parent_cycle",
                "parent cycle restore reason") &&
         expect(result.document.objectCount() == 0U,
                "parent cycle returns empty document");
}

bool buildRejectsDocumentWithInvalidId() {
  const cr::CreativeDocument document = cr::CreativeDocument::create("No Id");
  const iggy3d::ProductCreativeDocumentSectionBuildResult result =
      iggy3d::buildSaveCreativeDocumentSection(document);

  return expect(!result.section.present, "invalid id section absent") &&
         expect(result.receipt.requested, "invalid id requested") &&
         expect(!result.receipt.accepted, "invalid id not accepted") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidDocumentId,
                "invalid id status") &&
         expect(result.receipt.reasonCode == "invalid_document_id",
                "invalid id reason");
}

iggy3d::SaveCreativeDocumentSection validSection() {
  return iggy3d::buildSaveCreativeDocumentSection(authoredDocument()).section;
}

bool restoreRejectsMissingSection() {
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection({});

  return expect(!result.receipt.accepted, "missing section rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        MissingSection,
                "missing section status") &&
         expect(result.receipt.reasonCode ==
                    "missing_creative_document_section",
                "missing section reason") &&
         expect(result.document.objectCount() == 0U,
                "missing section returns empty document");
}

bool restoreRejectsInvalidObjectKind() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  if (section.objects.size() < 2U) {
    return expect(false, "invalid kind setup object count");
  }
  section.objects[1].kind = "DefinitelyNotAKind";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "invalid kind rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidObjectKind,
                "invalid kind status") &&
         expect(result.receipt.reasonCode == "invalid_object_kind",
                "invalid kind reason") &&
         expect(result.document.objectCount() == 0U,
                "invalid kind returns empty document");
}

bool restoreRejectsDisplayNameAsObjectKind() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  if (section.objects.size() < 2U) {
    return expect(false, "display name kind setup object count");
  }
  section.objects[1].kind = "Moving Platform";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "display name kind rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidObjectKind,
                "display name kind status") &&
         expect(result.receipt.reasonCode == "invalid_object_kind",
                "display name kind reason") &&
         expect(result.document.objectCount() == 0U,
                "display name kind returns empty document");
}

bool restoreRejectsDuplicateObjectIds() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  if (section.objects.size() < 2U) {
    return expect(false, "duplicate id setup object count");
  }
  section.objects[1].id = section.objects[0].id;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "duplicate id rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        DuplicateObjectId,
                "duplicate id status") &&
         expect(result.receipt.reasonCode == "duplicate_object_id",
                "duplicate id reason");
}

bool restoreRejectsBadNextObjectId() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  if (section.objects.size() < 2U) {
    return expect(false, "bad next setup object count");
  }
  section.nextObjectId = section.objects[1].id;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "bad next id rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidNextObjectId,
                "bad next id status") &&
         expect(result.receipt.reasonCode == "invalid_next_object_id",
                "bad next id reason");
}

bool restoreRejectsInvalidSnapModeAndAxes() {
  iggy3d::SaveCreativeDocumentSection badMode = validSection();
  badMode.snapMode = "Magnetic";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult modeResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badMode);

  iggy3d::SaveCreativeDocumentSection badAxes = validSection();
  badAxes.snapAxes =
      static_cast<std::uint32_t>(
          std::numeric_limits<cr::CreativeDocumentSnapAxisMask>::max()) +
      1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult axesResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badAxes);

  iggy3d::SaveCreativeDocumentSection badSettings = validSection();
  badSettings.snapStepX = 0.0;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult settingsResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badSettings);

  return expect(!modeResult.receipt.accepted,
                "invalid snap mode rejected") &&
         expect(modeResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidSnap,
                "invalid snap mode status") &&
         expect(modeResult.receipt.reasonCode ==
                    "invalid_document_snap_settings",
                "invalid snap mode reason") &&
         expect(!axesResult.receipt.accepted,
                "invalid snap axes rejected") &&
         expect(axesResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidSnap,
                "invalid snap axes status") &&
         expect(settingsResult.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidSnap,
                "invalid snap settings status");
}

bool restoreRejectsTooLargeGridDimension() {
  iggy3d::SaveCreativeDocumentSection section = validSection();
  section.gridWidth =
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) +
      1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult result =
      iggy3d::restoreCreativeDocumentFromSaveSection(section);

  return expect(!result.receipt.accepted, "too large grid rejected") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::InvalidGrid,
                "too large grid status") &&
         expect(result.receipt.reasonCode == "invalid_grid_settings",
                "too large grid reason");
}

bool voxelChunksEncodeDecodeAndRestore() {
  cr::CreativeDocument document = authoredDocument();
  const std::array edits{
      cr::CreativeVoxelEdit{{-1, 2, 3}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{16, 2, 3}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{17, 2, 3}, cr::CreativeObjectKind::Crate},
  };
  const cr::CreativeVoxelMutationReceipt mutation =
      document.applyVoxelEdits(edits);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  return expect(mutation.accepted && mutation.changed,
                "voxel save setup applies") &&
         expect(built.receipt.accepted &&
                    built.section.version ==
                        iggy3d::kSaveCreativeDocumentSectionVersion &&
                    built.section.voxelChunks.size() == 2U,
                "save section stores sparse chunks") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.voxelChunk.count=2\n") !=
                        std::string::npos,
                "codec writes voxel chunk key") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.voxelChunks.size() == 2U,
                "codec decodes voxel chunks") &&
         expect(restored.receipt.accepted &&
                    restored.document.voxelField().occupiedCellCount() == 3U,
                "restore owns three voxel cells") &&
         expect(restored.document.voxelField().materialAt({-1, 2, 3}) ==
                    cr::CreativeObjectKind::Wall &&
                    restored.document.voxelField().materialAt({16, 2, 3}) ==
                        cr::CreativeObjectKind::Floor &&
                    restored.document.voxelField().materialAt({17, 2, 3}) ==
                        cr::CreativeObjectKind::Crate,
                "restored voxel coordinates and materials match");
}

bool terrainControlsEncodeDecodeAndRestore() {
  cr::CreativeDocument document = authoredDocument();
  const std::array edits{
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{-4, 7}, 5, 3}},
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{8, -2}, 12, 6}},
  };
  const cr::CreativeTerrainMutationReceipt mutation =
      document.applyTerrainControlEdits(edits);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeTerrainControlPoint* first =
      restored.document.terrainField().controlAt({-4, 7});
  const cr::CreativeTerrainControlPoint* second =
      restored.document.terrainField().controlAt({8, -2});

  return expect(mutation.accepted && mutation.changed,
                "terrain save setup applies") &&
         expect(built.receipt.accepted &&
                    built.receipt.terrainControlCount == 2U &&
                    built.section.terrainControls.size() == 2U,
                "save section stores terrain controls") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainControl.count=2\n") !=
                        std::string::npos,
                "codec writes terrain control block") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.terrainControls.size() ==
                        2U,
                "codec decodes terrain controls") &&
         expect(restored.receipt.accepted && first != nullptr &&
                    first->heightCells == 5U && first->radiusCells == 3U &&
                    second != nullptr && second->heightCells == 12U &&
                    second->radiusCells == 6U,
                "restore preserves terrain coordinates and settings");
}

bool terrainMaterialsEncodeDecodeAndRestore() {
  cr::CreativeDocument document = authoredDocument();
  const cr::CreativeTerrainMaterialWeights weightedSurface{128U, 0U, 127U,
                                                            0U};
  const std::array<cr::CreativeTerrainMaterialEdit, 3U> edits{
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {-4, 7},
          cr::CreativeTerrainMaterial::Stone},
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {8, -2},
          cr::CreativeTerrainMaterial::Sand},
      cr::makeCreativeTerrainMaterialWeightEdit({2, 3}, weightedSurface),
  };
  const cr::CreativeTerrainMaterialMutationReceipt mutation =
      document.applyTerrainMaterialEdits(edits);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  iggy3d::SaveEnvelope legacyEnvelope = minimalEnvelope();
  legacyEnvelope.creativeDocument = built.section;
  legacyEnvelope.creativeDocument.version =
      iggy3d::kSaveCreativeDocumentAssetIdentityVersion;
  auto& legacyMaterials = legacyEnvelope.creativeDocument.terrainMaterials;
  legacyMaterials.erase(
      std::remove_if(legacyMaterials.begin(), legacyMaterials.end(),
                     [](const auto& record) {
                       return record.material == "Grass";
                     }),
      legacyMaterials.end());
  for (auto& record : legacyMaterials) {
    record.hasWeights = false;
    record.grassWeight = 0U;
    record.dirtWeight = 0U;
    record.stoneWeight = 0U;
    record.sandWeight = 0U;
  }
  const iggy3d::SaveEncodeResult legacyEncoded =
      iggy3d::encodeSaveEnvelope(legacyEnvelope);
  const std::string legacyText =
      eraseLinesContaining(legacyEncoded.encodedText, ".weightsPresent=");
  const iggy3d::SaveDecodeResult legacyDecoded =
      iggy3d::decodeSaveEnvelope(legacyText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult legacyRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          legacyDecoded.envelope.creativeDocument);

  iggy3d::SaveCreativeDocumentSection malformed = built.section;
  const auto weightedRecord = std::find_if(
      malformed.terrainMaterials.begin(), malformed.terrainMaterials.end(),
      [](const auto& record) { return record.material == "Grass"; });
  if (weightedRecord != malformed.terrainMaterials.end()) {
    ++weightedRecord->grassWeight;
  }
  const iggy3d::ProductCreativeDocumentSectionRestoreResult malformedRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(malformed);

  return expect(mutation.accepted && mutation.changed,
                "terrain material save setup applies") &&
         expect(built.receipt.accepted &&
                    built.receipt.terrainMaterialOverrideCount == 3U &&
                    built.section.version ==
                        iggy3d::kSaveCreativeDocumentSectionVersion &&
                    built.section.terrainMaterials.size() == 3U,
                "save section stores terrain material overrides") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainMaterial.count=3\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(".weightsPresent=true\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(".grassWeight=128\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(".stoneWeight=127\n") !=
                        std::string::npos,
                "codec writes exact weighted terrain material block") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.terrainMaterials.size() ==
                        3U,
                "codec decodes terrain material overrides") &&
         expect(restored.receipt.accepted &&
                    restored.document.terrainMaterialField().materialAt(
                        {-4, 7}) == cr::CreativeTerrainMaterial::Stone &&
                    restored.document.terrainMaterialField().materialAt(
                        {8, -2}) == cr::CreativeTerrainMaterial::Sand &&
                    restored.document.terrainMaterialField().weightsAt({2, 3}) ==
                        weightedSurface &&
                    restored.document.terrainMaterialField().materialAt({2, 3}) ==
                        cr::CreativeTerrainMaterial::Grass &&
                    restored.document.terrainMaterialField().materialAt(
                        {0, 0}) == cr::CreativeTerrainMaterial::Grass,
                "restore preserves weighted overrides and sparse grass default") &&
         expect(legacyEncoded.status == iggy3d::SaveCodecStatus::Ok &&
                    legacyText.find(".weightsPresent=") == std::string::npos &&
                    legacyDecoded.status == iggy3d::SaveCodecStatus::Ok &&
                    legacyRestored.receipt.accepted &&
                    legacyRestored.document.terrainMaterialField().weightsAt(
                        {-4, 7}) ==
                        cr::creativeTerrainMaterialSolidWeights(
                            cr::CreativeTerrainMaterial::Stone) &&
                    legacyRestored.document.terrainMaterialField().weightsAt(
                        {8, -2}) ==
                        cr::creativeTerrainMaterialSolidWeights(
                            cr::CreativeTerrainMaterial::Sand),
                "version 17 material records remain readable as one-hot layers") &&
         expect(!malformedRestored.receipt.accepted,
                "malformed weighted material payload rejects atomically");
}

bool terrainHeightFieldEncodeDecodeRestoreAndLegacyFallback() {
  cr::CreativeDocument document = authoredDocument();
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{-3, 5}, 3U, 2U};
  constexpr std::array<std::uint16_t, 6U> heights{4U, 0U, 6U,
                                                 7U, 8U, 9U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt mutation =
      document.replaceTerrainHeightField(bounds, heights);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeTerrainHeightField& restoredField =
      restored.document.terrainHeightField();

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentSegmentSpeedVersion;
  legacy.terrainHeightField = {};
  const iggy3d::ProductCreativeDocumentSectionRestoreResult legacyRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);

  iggy3d::SaveCreativeDocumentSection malformed = built.section;
  malformed.terrainHeightField.heights.pop_back();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult rejected =
      iggy3d::restoreCreativeDocumentFromSaveSection(malformed);
  iggy3d::SaveCreativeDocumentSection hiddenPayload = built.section;
  hiddenPayload.terrainHeightField.present = false;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult hiddenRejected =
      iggy3d::restoreCreativeDocumentFromSaveSection(hiddenPayload);

  return expect(mutation.accepted && mutation.changed,
                "terrain height save setup applies") &&
         expect(built.receipt.accepted &&
                    built.receipt.terrainHeightCellCount == heights.size() &&
                    built.section.terrainHeightField.present &&
                    built.section.terrainHeightField.minimumX == -3 &&
                    built.section.terrainHeightField.minimumZ == 5 &&
                    built.section.terrainHeightField.widthCells == 3U &&
                    built.section.terrainHeightField.depthCells == 2U &&
                    built.section.terrainHeightField.heights.size() ==
                        heights.size(),
                "save section preserves authored heightfield shape") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainHeightField.present=true\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainHeightField.height.1=0\n") !=
                        std::string::npos,
                "codec writes bounded heights including holes") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.terrainHeightField
                        .heights.size() == heights.size(),
                "codec decodes authored terrain heights") &&
         expect(restored.receipt.accepted &&
                    restored.receipt.terrainHeightCellCount == heights.size() &&
                    restoredField.bounds() == bounds &&
                    std::equal(restoredField.heights().begin(),
                               restoredField.heights().end(), heights.begin(),
                               heights.end()),
                "document restore preserves exact authored heightfield") &&
         expect(legacyRestored.receipt.accepted &&
                    legacyRestored.document.terrainHeightField().cellCount() ==
                        0U,
                "version 10 section without heightfield remains readable") &&
         expect(!rejected.receipt.accepted &&
                    rejected.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainData,
                "malformed heightfield rejects before document mutation") &&
         expect(!hiddenRejected.receipt.accepted &&
                    hiddenRejected.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainData,
                "absent heightfield cannot hide a stale payload");
}

bool terrainOperationsEncodeDecodeRestoreAndRejectDrift() {
  cr::CreativeDocument document = authoredDocument();
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 2U, 2U};
  constexpr std::array<std::uint16_t, 4U> baseHeights{2U, 2U, 2U, 2U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt base =
      document.replaceTerrainHeightField(bounds, baseHeights);

  cr::CreativeTerrainOperationMutationRequest firstRequest;
  firstRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  firstRequest.generation.bounds = bounds;
  firstRequest.generation.baseHeightCells = 5U;
  firstRequest.generation.reliefCells = 0U;
  firstRequest.generation.horizontalScaleCells = 4.0;
  firstRequest.generation.octaveCount = 1U;
  firstRequest.generation.slopeDamping = 0.0;
  cr::applyCreativeTerrainBiomeIntent(
      firstRequest.generation, cr::CreativeTerrainBiomeIntent::Arid);
  firstRequest.generation.materialTransitionHeightCells = 6U;
  firstRequest.composition.featherCells = 0U;
  const cr::CreativeTerrainProtectedRegionMutationReceipt protectedRegion =
      cr::addCreativeTerrainCompositionProtectedRegion(
          firstRequest.composition,
          {{{0, 0}, 1U, 1U},
           cr::CreativeTerrainCompositionMask::Rectangle});
  const cr::CreativeTerrainOperationMutationReceipt first =
      document.applyTerrainOperationMutation(firstRequest);

  cr::CreativeTerrainOperationMutationRequest secondRequest = firstRequest;
  secondRequest.generation.seed = 44U;
  secondRequest.generation.baseHeightCells = 9U;
  secondRequest.composition.mask =
      cr::CreativeTerrainCompositionMask::Ellipse;
  const cr::CreativeTerrainOperationMutationReceipt second =
      document.applyTerrainOperationMutation(secondRequest);
  cr::CreativeTerrainOperationMutationRequest disableFirst;
  disableFirst.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disableFirst.operationId = first.operationId;
  disableFirst.enabled = false;
  const cr::CreativeTerrainOperationMutationReceipt disabled =
      document.applyTerrainOperationMutation(disableFirst);

  const iggy3d::ProductCreativeDocumentSectionBuildResult version18Built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveCreativeDocumentSection version18 = version18Built.section;
  version18.version =
      iggy3d::kSaveCreativeDocumentTerrainMaterialWeightsVersion;
  version18.terrainOperationStackVersion = 1U;
  // Version 18 predates protected regions and generator-authored materials.
  // Keep the synthetic legacy payload internally consistent with those laws.
  version18.terrainHeightField.heights = {9U, 9U, 9U, 9U};
  version18.terrainMaterials.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult version18Restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(version18);

  cr::CreativeTerrainOperationMutationRequest gradeRequest;
  gradeRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  gradeRequest.operationKind = cr::CreativeTerrainOperationKind::Grade;
  gradeRequest.grade.start = {0, 0};
  gradeRequest.grade.end = {2, 0};
  gradeRequest.grade.startHeightCells = 7U;
  gradeRequest.grade.endHeightCells = 9U;
  gradeRequest.grade.halfWidthCells = 1U;
  gradeRequest.grade.crossSlopePermille = 250;
  gradeRequest.grade.falloffCells = 2U;
  const cr::CreativeTerrainOperationMutationReceipt grade =
      document.applyTerrainOperationMutation(gradeRequest);

  const cr::CreativeTerrainMaterialEdit baseMaterialEdit{
      cr::CreativeTerrainMaterialEditKind::Set, {0, 1},
      cr::CreativeTerrainMaterial::Stone};
  const cr::CreativeTerrainMaterialMutationReceipt baseMaterial =
      document.applyTerrainMaterialEdits(
          std::span{&baseMaterialEdit, 1U});
  cr::CreativeTerrainOperationMutationRequest pathRequest;
  pathRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  pathRequest.operationKind = cr::CreativeTerrainOperationKind::Path;
  pathRequest.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  pathRequest.sourceKey = "estate/terrain_path/road.entry";
  pathRequest.path.kind = cr::CreativeTerrainPathKind::Road;
  pathRequest.path.elevation = cr::CreativeTerrainPathElevation::Grade;
  pathRequest.path.curve = cr::CreativeTerrainPathCurvePolicy::CatmullRom;
  pathRequest.path.crossSection =
      cr::CreativeTerrainPathCrossSection::Flat;
  pathRequest.path.startJoin = cr::CreativeTerrainPathEndpointJoin::Blend;
  pathRequest.path.endJoin = cr::CreativeTerrainPathEndpointJoin::Bridge;
  pathRequest.path.falloffCells = 0U;
  pathRequest.path.material = cr::CreativeTerrainMaterial::Dirt;
  pathRequest.path.road.shoulderWidthCells = 0U;
  pathRequest.path.road.maximumGradePermille = 1000U;
  pathRequest.path.road.edgeTreatment =
      cr::CreativeTerrainRoadEdgeTreatment::Curb;
  pathRequest.path.road.edgeWidthMeters = 0.2;
  pathRequest.path.road.edgeHeightMeters = 0.25;
  pathRequest.path.road.edgeMaterial = cr::CreativeStructuralMaterial::Brick;
  pathRequest.path.nextPointId = 10U;
  pathRequest.path.points = {
      {7U, {0, 0}, 8U, 0U, 0U, 25},
      {9U, {1, 0}, 9U, 0U, 0U, -25},
  };
  const cr::CreativeTerrainOperationMutationReceipt path =
      document.applyTerrainOperationMutation(pathRequest);

  cr::CreativeTerrainOperationMutationRequest regionRequest;
  regionRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  regionRequest.operationKind = cr::CreativeTerrainOperationKind::Region;
  regionRequest.region.bounds = bounds;
  regionRequest.region.mask = cr::CreativeTerrainCompositionMask::Ellipse;
  regionRequest.region.mode = cr::CreativeTerrainRegionMode::Noise;
  regionRequest.region.amountCells = 3U;
  regionRequest.region.targetHeightCells = 7U;
  regionRequest.region.noiseReliefCells = 2U;
  regionRequest.region.noiseScaleCells = 5.5;
  regionRequest.region.featherCells = 1U;
  regionRequest.region.seed = 991U;
  const cr::CreativeTerrainOperationMutationReceipt region =
      document.applyTerrainOperationMutation(regionRequest);

  constexpr cr::CreativeTerrainHeightFieldBounds stampBounds{{0, 0}, 2U, 1U};
  constexpr std::array<std::uint16_t, 2U> stampHeights{5U, 0U};
  cr::CreativeTerrainHeightField stampHeight;
  static_cast<void>(stampHeight.replace(stampBounds, stampHeights));
  cr::CreativeTerrainMaterialField stampMaterial;
  const cr::CreativeTerrainMaterialEdit stampStone =
      cr::makeCreativeTerrainMaterialWeightEdit(
          {0, 0}, cr::creativeTerrainMaterialSolidWeights(
                      cr::CreativeTerrainMaterial::Stone));
  static_cast<void>(stampMaterial.apply(std::span{&stampStone, 1U}));
  cr::CreativeTerrainStamp stampAsset;
  const cr::CreativeTerrainStampCopyReceipt stampCopied =
      cr::copyCreativeTerrainRegionToStamp(
          991U, 17U,
          cr::buildCreativeTerrainHeightSurfacePlan(stampHeight),
          stampMaterial, {0, 0}, {1, 0}, "terrain-stamp-save-proof",
          "Save Proof", 8U, stampAsset);
  cr::CreativeTerrainOperationMutationRequest stampRequest;
  stampRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  stampRequest.operationKind = cr::CreativeTerrainOperationKind::Stamp;
  stampRequest.stamp.stamp = stampAsset;
  stampRequest.stamp.targetMinimum = {0, 0};
  stampRequest.stamp.mode = cr::CreativeTerrainStampMode::Replace;
  stampRequest.stamp.elevationMode =
      cr::CreativeTerrainStampElevationMode::Absolute;
  stampRequest.stamp.mirrorX = true;
  stampRequest.stamp.manualHeightOffsetCells = 1;
  const cr::CreativeTerrainOperationMutationReceipt stamp =
      document.applyTerrainOperationMutation(stampRequest);

  cr::CreativeTerrainOperationMutationRequest profileRequest;
  profileRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  profileRequest.operationKind = cr::CreativeTerrainOperationKind::Profile;
  profileRequest.profile.center = {1, 1};
  profileRequest.profile.baseHeightCells = 7U;
  profileRequest.profile.profile = cr::CreativeTerrainProfileKind::Ripple;
  profileRequest.profile.blend = cr::CreativeTerrainProfileBlend::Add;
  profileRequest.profile.rodPolicy =
      cr::CreativeTerrainProfileRodPolicy::Fill;
  profileRequest.profile.direction =
      cr::CreativeTerrainProfileDirection::NegativeZ;
  profileRequest.profile.radiusCells = 4U;
  profileRequest.profile.amplitudeCells = 3U;
  profileRequest.profile.spacingCells = 1U;
  profileRequest.profile.frequency = 1U;
  profileRequest.profile.seed = 77U;
  const cr::CreativeTerrainOperationMutationReceipt profile =
      document.applyTerrainOperationMutation(profileRequest);

  cr::CreativeTerrainOperationMutationRequest landformRequest;
  landformRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  landformRequest.operationKind = cr::CreativeTerrainOperationKind::Landform;
  landformRequest.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  landformRequest.sourceKey = "estate/terrain_landform/terrace.entry";
  landformRequest.landform.kind =
      cr::CreativeTerrainLandformKind::Terrace;
  landformRequest.landform.bounds = {{0, 0}, 2U, 2U};
  landformRequest.landform.baseHeightCells = 3U;
  landformRequest.landform.targetHeightCells = 9U;
  landformRequest.landform.terraceCount = 2U;
  landformRequest.landform.direction =
      cr::CreativeTerrainLandformDirection::NegativeZ;
  landformRequest.landform.edge =
      cr::CreativeTerrainLandformEdge::Retaining;
  landformRequest.landform.edgeWidthCells = 0U;
  landformRequest.landform.featherCells = 1U;
  landformRequest.landform.material = cr::CreativeTerrainMaterial::Stone;
  landformRequest.landform.erosion =
      cr::CreativeTerrainLandformErosion::Weathered;
  landformRequest.landform.erosionReliefCells = 1U;
  landformRequest.landform.seed = 1234U;
  const cr::CreativeTerrainOperationMutationReceipt landform =
      document.applyTerrainOperationMutation(landformRequest);

  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentTerrainHeightVersion;
  legacy.nextTerrainOperationId = 1U;
  legacy.terrainOperationBaseHeightField = {};
  legacy.terrainOperations.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult legacyRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);

  iggy3d::SaveCreativeDocumentSection version26 = built.section;
  version26.version = iggy3d::kSaveCreativeDocumentTerrainLandformVersion;
  version26.terrainOperationStackVersion = 9U;
  version26.terrainHardEdges.clear();
  version26.terrainOperationBaseHardEdges.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult version26Restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(version26);

  iggy3d::SaveCreativeDocumentSection version27 = built.section;
  version27.version = iggy3d::kSaveCreativeDocumentTerrainHardEdgeVersion;
  version27.terrainOperations[3U].pathVersion = 1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult version27Restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(version27);

  iggy3d::SaveCreativeDocumentSection badKind = built.section;
  badKind.terrainOperations.front().generatorKind = "UnknownGenerator";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult badKindResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badKind);
  iggy3d::SaveCreativeDocumentSection badBiome = built.section;
  badBiome.terrainOperations.front().biomeIntent = "UnknownBiome";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult badBiomeResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badBiome);
  iggy3d::SaveCreativeDocumentSection duplicateProtected = built.section;
  duplicateProtected.terrainOperations.front().protectedRegions.push_back(
      duplicateProtected.terrainOperations.front().protectedRegions.front());
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      duplicateProtectedResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(duplicateProtected);
  iggy3d::SaveCreativeDocumentSection badStackVersion = built.section;
  badStackVersion.terrainOperationStackVersion = 99U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      badStackVersionResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(badStackVersion);
  iggy3d::SaveCreativeDocumentSection duplicateId = built.section;
  duplicateId.terrainOperations.back().id =
      duplicateId.terrainOperations.front().id;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult duplicateResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(duplicateId);
  iggy3d::SaveCreativeDocumentSection duplicatePathPoint = built.section;
  duplicatePathPoint.terrainOperations[3U].pathPoints[1U].id =
      duplicatePathPoint.terrainOperations[3U].pathPoints[0U].id;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      duplicatePathPointResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(duplicatePathPoint);
  iggy3d::SaveCreativeDocumentSection missingSourceKey = built.section;
  missingSourceKey.terrainOperations[3U].sourceKey.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      missingSourceKeyResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(missingSourceKey);
  iggy3d::SaveCreativeDocumentSection badRegion = built.section;
  badRegion.terrainOperations[4U].regionMode = "Bogus";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult badRegionResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badRegion);
  iggy3d::SaveCreativeDocumentSection badStamp = built.section;
  badStamp.terrainOperations[5U].stampContentSignature ^= 1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult badStampResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badStamp);
  iggy3d::SaveCreativeDocumentSection badProfile = built.section;
  badProfile.terrainOperations[6U].profileKind = "Bogus";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult badProfileResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badProfile);
  iggy3d::SaveCreativeDocumentSection badLandform = built.section;
  badLandform.terrainOperations[7U].landformKind = "Bogus";
  const iggy3d::ProductCreativeDocumentSectionRestoreResult badLandformResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(badLandform);
  iggy3d::SaveCreativeDocumentSection duplicateHardEdge = built.section;
  if (!duplicateHardEdge.terrainHardEdges.empty()) {
    duplicateHardEdge.terrainHardEdges.push_back(
        duplicateHardEdge.terrainHardEdges.front());
  }
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      duplicateHardEdgeResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(duplicateHardEdge);
  iggy3d::SaveCreativeDocumentSection nonAdjacentHardEdge = built.section;
  if (!nonAdjacentHardEdge.terrainHardEdges.empty()) {
    auto& edge = nonAdjacentHardEdge.terrainHardEdges.front();
    edge.secondX = edge.firstX + 2;
    edge.secondZ = edge.firstZ;
  }
  const iggy3d::ProductCreativeDocumentSectionRestoreResult
      nonAdjacentHardEdgeResult =
          iggy3d::restoreCreativeDocumentFromSaveSection(nonAdjacentHardEdge);
  iggy3d::SaveCreativeDocumentSection drifted = built.section;
  ++drifted.terrainHeightField.heights.front();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult driftedResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(drifted);

  const cr::CreativeTerrainOperationStack& restoredStack =
      restored.document.terrainOperationStack();
  return expect(base.accepted && protectedRegion.accepted &&
                    protectedRegion.changed && first.accepted && second.accepted &&
                    disabled.accepted && grade.accepted &&
                    baseMaterial.accepted && path.accepted && region.accepted &&
                    stampCopied.accepted && stamp.accepted &&
                    profile.accepted && landform.accepted &&
                    !document.terrainHardEdges().empty(),
                "terrain operation save setup applies") &&
         expect(built.receipt.accepted &&
                    built.receipt.terrainOperationCount == 8U &&
                    built.section.terrainOperations.size() == 8U &&
                    built.section.version ==
                        iggy3d::kSaveCreativeDocumentSectionVersion &&
                    built.section.terrainOperationStackVersion ==
                        cr::kCreativeTerrainOperationStackVersion &&
                    built.receipt.terrainHardEdgeCount ==
                        document.terrainHardEdges().size() &&
                    built.section.terrainHardEdges.size() ==
                        document.terrainHardEdges().size() &&
                    built.section.terrainOperationBaseHeightField.present &&
                    built.section.terrainOperationBaseMaterials.size() == 1U &&
                    built.section.terrainOperations.front().paintMaterials &&
                    built.section.terrainOperations.front().biomeIntent ==
                        "Arid" &&
                    built.section.terrainOperations.front().lowlandMaterial ==
                        "Sand" &&
                    built.section.terrainOperations.front().protectedRegions.size() ==
                        1U &&
                    built.section.terrainOperations[3U]
                            .pathRoadShoulderWidthCells == 0U &&
                    built.section.terrainOperations[3U]
                            .pathRoadMaximumGradePermille == 1000U &&
                    built.section.terrainOperations[3U]
                            .pathRoadEdgeTreatment ==
                        static_cast<std::uint8_t>(
                            cr::CreativeTerrainRoadEdgeTreatment::Curb) &&
                    built.section.terrainOperations[3U]
                            .pathRoadEdgeWidthMeters == 0.2 &&
                    built.section.terrainOperations[3U]
                            .pathRoadEdgeHeightMeters == 0.25 &&
                    built.section.terrainOperations[3U]
                            .pathRoadEdgeMaterial ==
                        static_cast<std::uint8_t>(
                            cr::CreativeStructuralMaterial::Brick) &&
                    built.section.terrainOperations[5U].stampAssetId ==
                        "terrain-stamp-save-proof" &&
                    built.section.terrainOperations[5U].stampAssetVersion == 8U &&
                    built.section.terrainOperations[5U]
                            .stampHeightField.heights ==
                        std::vector<std::uint16_t>{5U, 0U} &&
                    built.section.terrainOperations[5U].stampMaterials.size() ==
                        1U &&
                    built.section.terrainOperations[6U].profileKind ==
                        "RIPPLE" &&
                    built.section.terrainOperations[6U].profileDirection ==
                        "-Z" &&
                    built.section.terrainOperations[6U].profileSeed == 77U &&
                    built.section.terrainOperations[7U].landformKind ==
                        "TERRACE" &&
                    built.section.terrainOperations[7U].landformDirection ==
                        "NEGATIVE_Z" &&
                    built.section.terrainOperations[7U].landformMaterial ==
                        "Stone" &&
                    built.section.terrainOperations[7U].landformSeed == 1234U,
                "save section owns ordered operation provenance") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.count=8\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.enabled=false\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.2.kind=Grade\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.2.grade.crossSlopePermille=250\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.kind=Path\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.owner=WorldLayout\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.sourceKey=estate/terrain_path/road.entry\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.path.point.1.id=9\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.path.road.shoulderWidthCells=0\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.path.road.maximumGradePermille=1000\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.path.road.edgeTreatment=1\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.3.path.road.edgeMaterial=4\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.4.kind=Region\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.4.region.mode=Noise\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.4.region.seed=991\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.generation.paintMaterials=true\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.generation.biomeIntent=Arid\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.composition.protectedRegion.count=1\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.baseMaterial.count=1\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.5.kind=Stamp\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.5.stamp.assetId=terrain-stamp-save-proof\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.5.stamp.height.height.1=0\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.6.kind=Profile\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.6.profile.kind=RIPPLE\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.6.profile.seed=77\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.7.kind=Landform\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.7.landform.kind=TERRACE\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.7.landform.seed=1234\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainHardEdge.count=") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.baseHardEdge.count=0\n") !=
                        std::string::npos,
                "codec writes operation order and every durable recipe") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.terrainOperations.size() ==
                        8U,
                "codec decodes bounded operation records") &&
         expect(restored.receipt.accepted &&
                    restoredStack.operations.size() == 8U &&
                    !restoredStack.operations.front().enabled &&
                    restoredStack.operations[1U].generation.seed == 44U &&
                    restoredStack.operations[1U].generation.paintMaterials &&
                    restoredStack.operations[1U].composition.mask ==
                        cr::CreativeTerrainCompositionMask::Ellipse &&
                    restoredStack.operations[1U].generation.biomeIntent ==
                        cr::CreativeTerrainBiomeIntent::Arid &&
                    restoredStack.operations[1U].generation.lowlandMaterial ==
                        cr::CreativeTerrainMaterial::Sand &&
                    restoredStack.operations[1U].composition.protectedRegionCount ==
                        1U &&
                    restoredStack.operations[2U].kind ==
                        cr::CreativeTerrainOperationKind::Grade &&
                    restoredStack.operations[2U].grade == gradeRequest.grade &&
                    restoredStack.operations[3U].kind ==
                        cr::CreativeTerrainOperationKind::Path &&
                    restoredStack.operations[3U].owner ==
                        cr::CreativeTerrainOperationOwner::WorldLayout &&
                    restoredStack.operations[3U].sourceKey ==
                        pathRequest.sourceKey &&
                    restoredStack.operations[3U].path == pathRequest.path &&
                    restoredStack.operations[4U].kind ==
                        cr::CreativeTerrainOperationKind::Region &&
                    restoredStack.operations[4U].region ==
                        regionRequest.region &&
                    restoredStack.operations[5U].kind ==
                        cr::CreativeTerrainOperationKind::Stamp &&
                    restoredStack.operations[5U].stamp ==
                        stampRequest.stamp &&
                    restoredStack.operations[6U].kind ==
                        cr::CreativeTerrainOperationKind::Profile &&
                    restoredStack.operations[6U].profile ==
                        profileRequest.profile &&
                    restoredStack.operations[7U].kind ==
                        cr::CreativeTerrainOperationKind::Landform &&
                    restoredStack.operations[7U].owner ==
                        cr::CreativeTerrainOperationOwner::WorldLayout &&
                    restoredStack.operations[7U].sourceKey ==
                        landformRequest.sourceKey &&
                    restoredStack.operations[7U].landform ==
                        landformRequest.landform &&
                    restoredStack.baseMaterialField.materialAt({0, 1}) ==
                        cr::CreativeTerrainMaterial::Stone &&
                    cr::creativeTerrainHeightFieldsEqual(
                        restored.document.terrainHeightField(),
                        document.terrainHeightField()) &&
                    cr::creativeTerrainMaterialFieldsEqual(
                        restored.document.terrainMaterialField(),
                        document.terrainMaterialField()) &&
                    cr::creativeTerrainHardEdgesEqual(
                        restored.document.terrainHardEdges(),
                        document.terrainHardEdges()),
                "restore preserves recipes and replayed height material and topology") &&
         expect(version18Restored.receipt.accepted,
                "version 18 terrain operation section restores") &&
         expect(version18Restored.document.terrainOperationStack().version ==
                    cr::kCreativeTerrainOperationStackVersion,
                "version 18 operation stack upgrades to the current version") &&
         expect(version18Restored.document.terrainOperationStack()
                        .operations.size() == 2U,
                "version 18 operation stack preserves operation count") &&
         expect(version18Restored.document.terrainOperationStack()
                        .operations.back().kind ==
                    cr::CreativeTerrainOperationKind::GeneratedTerrain,
                "version 18 operations migrate to typed generated terrain") &&
         expect(!version18Restored.document.terrainOperationStack()
                     .operations.back().generation.paintMaterials,
                "version 18 generated terrain remains height-only") &&
         expect(legacyRestored.receipt.accepted &&
                    legacyRestored.document.terrainOperationStack()
                        .operations.empty() &&
                    cr::creativeTerrainHeightFieldsEqual(
                        legacyRestored.document.terrainHeightField(),
                        document.terrainHeightField()),
                "version 11 terrain remains readable as a baked field") &&
         expect(version26Restored.receipt.accepted &&
                    cr::creativeTerrainHardEdgesEqual(
                        version26Restored.document.terrainHardEdges(),
                        document.terrainHardEdges()),
                "version 26 landforms derive missing hard topology on load") &&
         expect(version27Restored.receipt.accepted &&
                    version27Restored.document.terrainOperationStack()
                            .operations[3U]
                            .path.version ==
                        cr::kCreativeTerrainPathSourceVersion &&
                    version27Restored.document.terrainOperationStack()
                            .operations[3U]
                            .path.road == cr::CreativeTerrainRoadSettings{} &&
                    version27Restored.document.terrainOperationStack()
                            .operations[3U]
                            .path.road.maximumGradePermille == 0U,
                "version 27 roads migrate to behavior-preserving construction defaults") &&
         expect(!badKindResult.receipt.accepted &&
                    badKindResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "unknown generator rejects before restore") &&
         expect(!badBiomeResult.receipt.accepted &&
                    !duplicateProtectedResult.receipt.accepted &&
                    badBiomeResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData &&
                    duplicateProtectedResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "invalid biome and duplicate protected regions reject restore") &&
         expect(!badStackVersionResult.receipt.accepted &&
                    badStackVersionResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "unknown operation stack version rejects before restore") &&
         expect(!duplicateResult.receipt.accepted &&
                    duplicateResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "duplicate operation ids reject before restore") &&
         expect(!duplicatePathPointResult.receipt.accepted &&
                    duplicatePathPointResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "duplicate stable path point ids reject before restore") &&
         expect(!missingSourceKeyResult.receipt.accepted &&
                    missingSourceKeyResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "generated operation without source key rejects before restore") &&
         expect(!badRegionResult.receipt.accepted &&
                    badRegionResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "invalid region recipe rejects before restore") &&
         expect(!badStampResult.receipt.accepted &&
                    badStampResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "corrupt baked stamp content rejects before restore") &&
         expect(!badProfileResult.receipt.accepted &&
                    badProfileResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "invalid durable profile recipe rejects before restore") &&
         expect(!badLandformResult.receipt.accepted &&
                    badLandformResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "invalid durable landform recipe rejects before restore") &&
         expect(!duplicateHardEdgeResult.receipt.accepted &&
                    !nonAdjacentHardEdgeResult.receipt.accepted &&
                    duplicateHardEdgeResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainData &&
                    nonAdjacentHardEdgeResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainData,
                "duplicate and non-cardinal hard topology reject before restore") &&
         expect(!driftedResult.receipt.accepted &&
                    driftedResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "derived field drift rejects against deterministic replay");
}

bool roadConstructionSettingsEncodeDecodeAndRestore() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Road save settings");
  static_cast<void>(document.assignId(703U));
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::Path;
  request.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  request.sourceKey = "road_save/terrain_path/main";
  request.path.kind = cr::CreativeTerrainPathKind::Road;
  request.path.elevation = cr::CreativeTerrainPathElevation::Grade;
  request.path.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  request.path.falloffCells = 0U;
  request.path.road.shoulderWidthCells = 2U;
  request.path.road.maximumGradePermille = 250U;
  request.path.road.edgeTreatment =
      cr::CreativeTerrainRoadEdgeTreatment::Curb;
  request.path.road.edgeWidthMeters = 0.22;
  request.path.road.edgeHeightMeters = 0.18;
  request.path.road.edgeMaterial = cr::CreativeStructuralMaterial::Brick;
  request.path.nextPointId = 3U;
  request.path.points = {
      {1U, {0, 0}, 4U, 1U, 0U, 0},
      {2U, {8, 0}, 4U, 1U, 0U, 0},
  };
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(request);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeTerrainOperation* restoredRoad =
      restored.receipt.accepted &&
              restored.document.terrainOperationStack().operations.size() == 1U
          ? &restored.document.terrainOperationStack().operations.front()
          : nullptr;

  return expect(added.accepted && built.receipt.accepted &&
                    built.section.terrainOperations.size() == 1U,
                "road construction save fixture builds") &&
         expect(built.section.terrainOperations[0]
                        .pathRoadShoulderWidthCells == 2U &&
                    built.section.terrainOperations[0]
                            .pathRoadMaximumGradePermille == 250U &&
                    built.section.terrainOperations[0]
                            .pathRoadEdgeTreatment ==
                        static_cast<std::uint8_t>(
                            cr::CreativeTerrainRoadEdgeTreatment::Curb) &&
                    built.section.terrainOperations[0]
                            .pathRoadEdgeWidthMeters == 0.22 &&
                    built.section.terrainOperations[0]
                            .pathRoadEdgeHeightMeters == 0.18 &&
                    built.section.terrainOperations[0].pathRoadEdgeMaterial ==
                        static_cast<std::uint8_t>(
                            cr::CreativeStructuralMaterial::Brick),
                "save section records every road construction setting") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.path.road.shoulderWidthCells=2\n") !=
                        std::string::npos &&
                    decoded.status == iggy3d::SaveCodecStatus::Ok,
                "save codec carries nondefault road construction fields") &&
         expect(restoredRoad != nullptr &&
                    restoredRoad->path == request.path &&
                    cr::creativeTerrainHeightFieldsEqual(
                        restored.document.terrainHeightField(),
                        document.terrainHeightField()),
                "restore preserves road settings and exact generated terrain");
}

bool watercourseSettingsEncodeDecodeRestoreAndMigrate() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Watercourse save settings");
  static_cast<void>(document.assignId(704U));
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::Path;
  request.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  request.sourceKey = "watercourse_save/terrain_path/river";
  request.path.kind = cr::CreativeTerrainPathKind::River;
  request.path.elevation = cr::CreativeTerrainPathElevation::Grade;
  request.path.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  request.path.material = cr::CreativeTerrainMaterial::Sand;
  request.path.watercourse.bankSlopeCells = 2U;
  request.path.watercourse.drainageDirection =
      cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd;
  request.path.watercourse.surfacePolicy =
      cr::CreativeTerrainWaterSurfacePolicy::Reserved;
  request.path.watercourse.surfaceInsetCells = 1U;
  request.path.watercourse.nextCrossingId = 8U;
  request.path.watercourse.crossings = {{7U, 2U, 1U, 2U, 3U}};
  request.path.nextPointId = 4U;
  request.path.points = {
      {1U, {-4, 0}, 9U, 2U, 3U, 0},
      {2U, {0, 0}, 8U, 2U, 3U, 0},
      {3U, {4, 0}, 7U, 2U, 3U, 0},
  };
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(request);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativeTerrainOperation* restoredWatercourse =
      restored.receipt.accepted &&
              restored.document.terrainOperationStack().operations.size() == 1U
          ? &restored.document.terrainOperationStack().operations.front()
          : nullptr;

  cr::CreativeDocument legacyDocument =
      cr::CreativeDocument::create("Legacy watercourse defaults");
  static_cast<void>(legacyDocument.assignId(705U));
  cr::CreativeTerrainOperationMutationRequest legacyRequest = request;
  legacyRequest.sourceKey = "watercourse_save/terrain_path/legacy";
  legacyRequest.path.watercourse = {};
  const cr::CreativeTerrainOperationMutationReceipt legacyAdded =
      legacyDocument.applyTerrainOperationMutation(legacyRequest);
  iggy3d::SaveCreativeDocumentSection legacySection =
      iggy3d::buildSaveCreativeDocumentSection(legacyDocument).section;
  legacySection.version = iggy3d::kSaveCreativeDocumentTerrainRoadVersion;
  legacySection.terrainOperations[0].pathVersion = 2U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult legacyRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacySection);
  const cr::CreativeTerrainOperation* migratedWatercourse =
      legacyRestored.receipt.accepted &&
              legacyRestored.document.terrainOperationStack().operations.size() ==
                  1U
          ? &legacyRestored.document.terrainOperationStack().operations.front()
          : nullptr;

  iggy3d::SaveCreativeDocumentSection invalid = built.section;
  invalid.terrainOperations[0].pathWatercourseCrossings[0].pointId = 99U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult invalidRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(invalid);

  const iggy3d::SaveCreativeDocumentTerrainOperationRecord* record =
      built.section.terrainOperations.size() == 1U
          ? &built.section.terrainOperations.front()
          : nullptr;
  return expect(added.accepted && built.receipt.accepted && record != nullptr,
                "watercourse save fixture builds") &&
         expect(record->pathWatercourseBankSlopeCells == 2U &&
                    record->pathWatercourseDrainageDirection ==
                        static_cast<std::uint8_t>(
                            cr::CreativeTerrainWatercourseDrainageDirection::
                                StartToEnd) &&
                    record->pathWatercourseSurfacePolicy ==
                        static_cast<std::uint8_t>(
                            cr::CreativeTerrainWaterSurfacePolicy::Reserved) &&
                    record->pathWatercourseSurfaceInsetCells == 1U &&
                    record->pathWatercourseNextCrossingId == 8U &&
                    record->pathWatercourseCrossings.size() == 1U &&
                    record->pathWatercourseCrossings[0].id == 7U &&
                    record->pathWatercourseCrossings[0].pointId == 2U &&
                    record->pathWatercourseCrossings[0].approachLengthCells ==
                        3U,
                "save section records every watercourse and crossing field") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.path.watercourse."
                        "bankSlopeCells=2\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.terrainOperation.0.path.watercourse."
                        "crossing.0.pointId=2\n") != std::string::npos &&
                    decoded.status == iggy3d::SaveCodecStatus::Ok,
                "save codec carries nondefault watercourse fields") &&
         expect(restoredWatercourse != nullptr &&
                    restoredWatercourse->path == request.path &&
                    cr::creativeTerrainHeightFieldsEqual(
                        restored.document.terrainHeightField(),
                        document.terrainHeightField()),
                "restore preserves watercourse source and exact terrain") &&
         expect(legacyAdded.accepted && migratedWatercourse != nullptr &&
                    migratedWatercourse->path.version ==
                        cr::kCreativeTerrainPathSourceVersion &&
                    migratedWatercourse->path.watercourse ==
                        cr::CreativeTerrainWatercourseSettings{},
                "version 28 paths migrate to inert watercourse defaults") &&
         expect(!invalidRestored.receipt.accepted &&
                    invalidRestored.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainOperationData,
                "dangling watercourse crossing rejects before restore");
}

bool bakedTerrainHardEdgesRoundTripWithoutProceduralState() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Baked Topology");
  static_cast<void>(document.assignId(702U));
  const std::vector<std::uint16_t> baseHeights(16U, 2U);
  const cr::CreativeTerrainHeightFieldReplaceReceipt base =
      document.replaceTerrainHeightField({{0, 0}, 4U, 4U}, baseHeights);

  cr::CreativeTerrainOperationMutationRequest landform;
  landform.kind = cr::CreativeTerrainOperationMutationKind::Add;
  landform.operationKind = cr::CreativeTerrainOperationKind::Landform;
  landform.landform.bounds = {{0, 0}, 4U, 4U};
  landform.landform.baseHeightCells = 2U;
  landform.landform.targetHeightCells = 8U;
  landform.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  landform.landform.edgeWidthCells = 0U;
  landform.landform.paintSurface = false;
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(landform);
  cr::CreativeTerrainOperationMutationRequest bake;
  bake.kind = cr::CreativeTerrainOperationMutationKind::BakeAll;
  const cr::CreativeTerrainOperationMutationReceipt baked =
      document.applyTerrainOperationMutation(bake);

  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  iggy3d::SaveCreativeDocumentSection staleTopology = built.section;
  staleTopology.terrainHardEdges = {{{0, 0, 1, 0}}};
  const iggy3d::ProductCreativeDocumentSectionRestoreResult staleRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(staleTopology);

  return expect(base.accepted && added.accepted && baked.accepted &&
                    !document.terrainHardEdges().empty() &&
                    document.terrainOperationStack().operations.empty(),
                "BakeAll fixture retains topology without operations") &&
         expect(built.receipt.accepted &&
                    built.section.terrainOperations.empty() &&
                    built.section.terrainOperationBaseHardEdges.empty() &&
                    built.section.terrainHardEdges.size() ==
                        document.terrainHardEdges().size(),
                "save section owns baked topology without a hidden base") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    restored.receipt.accepted &&
                    restored.document.terrainOperationStack().operations.empty() &&
                    cr::creativeTerrainHeightFieldsEqual(
                        restored.document.terrainHeightField(),
                        document.terrainHeightField()) &&
                    cr::creativeTerrainHardEdgesEqual(
                        restored.document.terrainHardEdges(),
                        document.terrainHardEdges()),
                "codec round-trip preserves detached terrain topology exactly") &&
         expect(!staleRestored.receipt.accepted &&
                    staleRestored.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidTerrainData,
                "baked save rejects a hard edge between equal-height cells");
}

bool patternRecipesEncodeDecodeRestoreAndRejectDanglingReferences() {
  cr::CreativeDocumentRestoreRequest request = authoredRestoreRequest();
  cr::CreativePatternRecipe recipe;
  recipe.id = 1U;
  recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  recipe.sourceObjectIds = {2U};
  recipe.generatedObjectIds = {7U};
  recipe.linear.direction = cr::CreativeLinearArrayDirection::NegativeZ;
  recipe.linear.copyCount = cr::CreativeLinearArrayCopyCount::Eight;
  recipe.linear.spacing = cr::CreativeLinearArraySpacing::FourCells;
  recipe.linear.cellSize = 0.75;
  recipe.linear.maxGeneratedObjects = 16U;
  request.patternRecipeStore.nextRecipeId = 2U;
  request.patternRecipeStore.recipes = {recipe};

  cr::CreativeDocument document = cr::CreativeDocument::create("Before Pattern");
  const cr::CreativeDocumentRestoreReceipt setup =
      document.restoreForLoad(request);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  iggy3d::SaveCreativeDocumentSection versionOne = built.section;
  versionOne.patternRecipeStoreVersion = 1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult migrated =
      iggy3d::restoreCreativeDocumentFromSaveSection(versionOne);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentWindowVersion;
  legacy.patternRecipeStoreVersion = cr::kCreativePatternRecipeStoreVersion;
  legacy.nextPatternRecipeId = 1U;
  legacy.patternRecipes.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult legacyRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);

  iggy3d::SaveCreativeDocumentSection dangling = built.section;
  dangling.patternRecipes.front().generatedObjectIds = {999U};
  const iggy3d::ProductCreativeDocumentSectionRestoreResult danglingResult =
      iggy3d::restoreCreativeDocumentFromSaveSection(dangling);

  const cr::CreativePatternRecipe* restoredRecipe =
      cr::findCreativePatternRecipe(
          restored.document.patternRecipeStore(), 1U);
  const cr::CreativePatternRecipe* ownerByGeneratedObject =
      cr::findCreativePatternRecipeByGeneratedObject(
          restored.document.patternRecipeStore(), 7U);
  return expect(setup.accepted, "pattern recipe save setup restores") &&
         expect(built.receipt.accepted &&
                    built.receipt.patternRecipeCount == 1U &&
                    built.section.version ==
                        iggy3d::kSaveCreativeDocumentSectionVersion &&
                    built.section.patternRecipeStoreVersion ==
                        cr::kCreativePatternRecipeStoreVersion &&
                    built.section.nextPatternRecipeId == 2U &&
                    built.section.patternRecipes.size() == 1U,
                "save section owns bounded pattern provenance") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.count=1\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.linear.direction="
                        "-Z\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.linear.cellSize="
                        "0.75\n") != std::string::npos,
                "codec writes exact pattern parameters") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.patternRecipes.size() ==
                        1U,
                "codec decodes bounded pattern records") &&
         expect(restored.receipt.accepted && restoredRecipe != nullptr &&
                    *restoredRecipe == recipe &&
                    ownerByGeneratedObject == restoredRecipe,
                "restore preserves editable pattern identity and ownership") &&
         expect(migrated.receipt.accepted &&
                    migrated.document.patternRecipeStore().version ==
                        cr::kCreativePatternRecipeStoreVersion &&
                    migrated.document.patternRecipeStore().recipes.size() ==
                        1U &&
                    migrated.document.patternRecipeStore().recipes.front() ==
                        recipe,
                "version one array recipes migrate without semantic drift") &&
         expect(legacyRestored.receipt.accepted &&
                    legacyRestored.document.patternRecipeStore()
                        .recipes.empty() &&
                    legacyRestored.document.patternRecipeStore().nextRecipeId ==
                        1U,
                "version 14 documents default to no editable patterns") &&
         expect(!danglingResult.receipt.accepted &&
                    danglingResult.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidPatternRecipeData,
                "dangling pattern outputs reject before publication");
}

bool assetScatterRecipesRoundTripEveryEditableField() {
  cr::CreativeDocumentRestoreRequest request = authoredRestoreRequest();
  cr::CreativePatternRecipe recipe;
  recipe.id = 1U;
  recipe.kind = cr::CreativePatternRecipeKind::AssetScatter;
  recipe.generatedObjectIds = {7U};
  recipe.scatter.objectKind = cr::CreativeObjectKind::Rock;
  recipe.scatter.assetId = "environment/rocks/boulder_a";
  recipe.scatter.assetContentHash = 0x123456789abcdef0ULL;
  recipe.scatter.assetMaterialVariant = "Lichen";
  recipe.scatter.assetSourceBounds = {{-1.25, 0.0, -0.75},
                                      {1.25, 2.5, 0.75}};
  recipe.scatter.paintCenters = {{2.0, 0.5, 3.0}, {9.0, 1.5, -4.0}};
  recipe.scatter.exclusions = {{{4.0, 0.5, 3.0}, 1.75},
                               {{8.0, 1.5, -3.0}, 0.625}};
  recipe.scatter.mask = cr::CreativeAssetScatterRecipeMask::Box;
  recipe.scatter.yaw = cr::CreativeAssetScatterRecipeYaw::QuarterTurns;
  recipe.scatter.baseYawRadians = 1.25;
  recipe.scatter.radiusMeters = 7.5;
  recipe.scatter.spacingMeters = 1.25;
  recipe.scatter.densityFraction = 0.375;
  recipe.scatter.scaleVariation = 0.225;
  recipe.scatter.maximumSlopeRadians = 0.42;
  recipe.scatter.projectToTerrainSurface = true;
  recipe.scatter.avoidCollisions = false;
  recipe.scatter.seed = 0xfedcba9876543210ULL;
  recipe.scatter.maxGeneratedObjects = 96U;
  request.patternRecipeStore.nextRecipeId = 2U;
  request.patternRecipeStore.recipes = {recipe};

  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter Save");
  const cr::CreativeDocumentRestoreReceipt setup =
      document.restoreForLoad(request);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);
  const cr::CreativePatternRecipe* restoredRecipe =
      cr::findCreativePatternRecipe(restored.document.patternRecipeStore(),
                                    recipe.id);

  iggy3d::SaveCreativeDocumentSection invalidVersion = built.section;
  invalidVersion.patternRecipeStoreVersion = 1U;
  const iggy3d::ProductCreativeDocumentSectionRestoreResult rejectedLegacy =
      iggy3d::restoreCreativeDocumentFromSaveSection(invalidVersion);
  return expect(setup.accepted && built.receipt.accepted,
                "scatter persistence setup is valid") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.scatter.assetId="
                        "environment/rocks/boulder_a\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.scatter."
                        "paintCenter.count=2\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.scatter."
                        "exclusion.1.radiusMeters=0.625\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.scatter."
                        "projectToTerrainSurface=true\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.patternRecipe.0.scatter."
                        "avoidCollisions=false\n") != std::string::npos,
                "scatter codec writes identity paint and exclusions") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    restored.receipt.accepted && restoredRecipe != nullptr &&
                    *restoredRecipe == recipe,
                "scatter recipe round trips every editable field exactly") &&
         expect(!rejectedLegacy.receipt.accepted &&
                    rejectedLegacy.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidPatternRecipeData,
                "version one store cannot claim a scatter recipe");
}

bool measurementAnnotationsEncodeDecodeRestoreAndLegacyDefault() {
  cr::CreativeDocumentRestoreRequest request = authoredRestoreRequest();
  cr::CreativeMeasurementAnnotation annotation;
  annotation.id = 3U;
  annotation.name = "Courtyard area";
  annotation.mode = cr::CreativeMeasurementMode::Area;
  annotation.axis = cr::CreativeMeasurementAxis::X;
  annotation.closePath = true;
  annotation.pointCount = 3U;
  annotation.points[0U] =
      {1.0, 0.0, 2.0, cr::CreativeMeasurementSnapKind::Grid};
  annotation.points[1U] =
      {5.0, 0.0, 2.0, cr::CreativeMeasurementSnapKind::Vertex};
  annotation.points[2U] =
      {5.0, 0.0, 6.0, cr::CreativeMeasurementSnapKind::Opening};
  request.measurementAnnotationStore.nextAnnotationId = 4U;
  request.measurementAnnotationStore.annotations = {annotation};

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Before Annotations");
  const cr::CreativeDocumentRestoreReceipt setup =
      document.restoreForLoad(request);
  const iggy3d::ProductCreativeDocumentSectionBuildResult built =
      iggy3d::buildSaveCreativeDocumentSection(document);
  iggy3d::SaveEnvelope envelope = minimalEnvelope();
  envelope.creativeDocument = built.section;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductCreativeDocumentSectionRestoreResult restored =
      iggy3d::restoreCreativeDocumentFromSaveSection(
          decoded.envelope.creativeDocument);

  iggy3d::SaveCreativeDocumentSection legacy = built.section;
  legacy.version = iggy3d::kSaveCreativeDocumentPatternRecipeVersion;
  legacy.measurementAnnotationStoreVersion =
      cr::kCreativeMeasurementAnnotationStoreVersion;
  legacy.nextMeasurementAnnotationId = 1U;
  legacy.measurementAnnotations.clear();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult legacyRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(legacy);

  iggy3d::SaveCreativeDocumentSection invalid = built.section;
  invalid.measurementAnnotations.front().points.front().x =
      std::numeric_limits<double>::quiet_NaN();
  const iggy3d::ProductCreativeDocumentSectionRestoreResult invalidRestored =
      iggy3d::restoreCreativeDocumentFromSaveSection(invalid);

  const cr::CreativeMeasurementAnnotation* restoredAnnotation =
      cr::findCreativeMeasurementAnnotation(
          restored.document.measurementAnnotationStore(), 3U);
  return expect(setup.accepted, "measurement annotation save setup restores") &&
         expect(built.receipt.accepted &&
                    built.receipt.measurementAnnotationCount == 1U &&
                    built.section.version ==
                        iggy3d::kSaveCreativeDocumentSectionVersion &&
                    built.section.measurementAnnotationStoreVersion ==
                        cr::kCreativeMeasurementAnnotationStoreVersion &&
                    built.section.nextMeasurementAnnotationId == 4U &&
                    built.section.measurementAnnotations.size() == 1U,
                "save section owns bounded measurement annotations") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok &&
                    encoded.encodedText.find(
                        "creativeDocument.measurementAnnotation.count=1\n") !=
                        std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.measurementAnnotation.0.name="
                        "Courtyard area\n") != std::string::npos &&
                    encoded.encodedText.find(
                        "creativeDocument.measurementAnnotation.0.point.2."
                        "snapKind=Opening\n") != std::string::npos,
                "codec writes exact measurement annotation data") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok &&
                    decoded.envelope.creativeDocument.measurementAnnotations
                            .size() == 1U,
                "codec decodes bounded measurement annotations") &&
         expect(restored.receipt.accepted && restoredAnnotation != nullptr &&
                    *restoredAnnotation == annotation,
                "restore preserves measurement annotation identity") &&
         expect(legacyRestored.receipt.accepted &&
                    legacyRestored.document.measurementAnnotationStore()
                        .annotations.empty() &&
                    legacyRestored.document.measurementAnnotationStore()
                            .nextAnnotationId == 1U,
                "version 15 documents default to no measurement annotations") &&
         expect(!invalidRestored.receipt.accepted &&
                    invalidRestored.receipt.status ==
                        iggy3d::ProductCreativeDocumentSectionStatus::
                            InvalidMeasurementAnnotationData,
                "invalid measurement annotation rejects before publication");
}

}  // namespace

int main() {
  bool ok = true;
  ok = buildSectionCopiesDocumentExactly() && ok;
  ok = encodeDecodeAndRestoreRoundTripsDocument() && ok;
  ok = buildSectionCopiesPathPoints() && ok;
  ok = restoreSectionRestoresPathPoints() && ok;
  ok = encodeDecodeAndRestoreRoundTripsPathAndLineEndpointPayloads() && ok;
  ok = movingPlatformSettingsEncodeDecodeAndRestore() && ok;
  ok = doorSettingsEncodeDecodeRestoreAndLegacyDefault() && ok;
  ok = windowSettingsEncodeDecodeRestoreAndLegacyDefault() && ok;
  ok = playerSpawnSettingsEncodeDecodeRestoreAndLegacyDefault() && ok;
  ok = npcSpawnSettingsEncodeDecodeRestoreAndLegacyDefault() && ok;
  ok = legacyMovingPlatformReceivesDefaultRouteAndSettings() && ok;
  ok = restoreRejectsInvalidPathPayloads() && ok;
  ok = restoreRejectsInvalidLineEndpointPayloads() && ok;
  ok = restoreRejectsNonPathObjectCarryingPathPoints() && ok;
  ok = restoreRejectsUnsupportedParentPayload() && ok;
  ok = restoreRejectsMissingParentPayload() && ok;
  ok = restoreRejectsUnsupportedParentOwnerPayload() && ok;
  ok = restoreRejectsParentCyclePayload() && ok;
  ok = buildRejectsDocumentWithInvalidId() && ok;
  ok = restoreRejectsMissingSection() && ok;
  ok = restoreRejectsInvalidObjectKind() && ok;
  ok = restoreRejectsDisplayNameAsObjectKind() && ok;
  ok = restoreRejectsDuplicateObjectIds() && ok;
  ok = restoreRejectsBadNextObjectId() && ok;
  ok = restoreRejectsInvalidSnapModeAndAxes() && ok;
  ok = restoreRejectsTooLargeGridDimension() && ok;
  ok = voxelChunksEncodeDecodeAndRestore() && ok;
  ok = terrainControlsEncodeDecodeAndRestore() && ok;
  ok = terrainHeightFieldEncodeDecodeRestoreAndLegacyFallback() && ok;
  ok = terrainOperationsEncodeDecodeRestoreAndRejectDrift() && ok;
  ok = roadConstructionSettingsEncodeDecodeAndRestore() && ok;
  ok = watercourseSettingsEncodeDecodeRestoreAndMigrate() && ok;
  ok = bakedTerrainHardEdgesRoundTripWithoutProceduralState() && ok;
  ok = patternRecipesEncodeDecodeRestoreAndRejectDanglingReferences() && ok;
  ok = assetScatterRecipesRoundTripEveryEditableField() && ok;
  ok = measurementAnnotationsEncodeDecodeRestoreAndLegacyDefault() && ok;
  ok = terrainMaterialsEncodeDecodeAndRestore() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
