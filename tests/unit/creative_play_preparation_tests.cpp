#include "app/iggy3d/creative/play/PlayPreparation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool addFloor(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Floor;
  request.name = "Play Floor";
  request.transform.position = {0.0, 0.0, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{-3.0, 0.0, -3.0}, {3.0, 0.25, 3.0}};
  request.hasBoundsOverride = true;
  return document.createObject(request).accepted;
}

cr::CreativeDocumentCreateReceipt addSpawn(
    cr::CreativeDocument& document,
    cr::CreativeVec3 position = {1.0, 0.25, -1.0},
    cr::CreativePlayerSpawnSettings settings = {},
    double yawRadians = 0.0,
    std::string name = "Play Spawn") {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::SpawnPoint;
  request.name = std::move(name);
  request.transform.position = position;
  request.transform.rotationEulerRadians.y = yawRadians;
  request.hasTransformOverride = true;
  request.playerSpawn = std::move(settings);
  request.hasPlayerSpawnSettingsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addPatrolRoute(
    cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.name = "Play Route";
  request.hasPathOverride = true;
  request.pathPoints = {{{-2.0, 0.25, 0.0}}, {{2.0, 0.25, 0.0}}};
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addNpc(
    cr::CreativeDocument& document,
    cr::CreativeObjectId routeObjectId) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::NpcSpawn;
  request.name = "Play Guard";
  request.transform.position = {-2.0, 0.25, 0.0};
  request.hasTransformOverride = true;
  request.parentId = routeObjectId;
  return document.createObject(request);
}

bool addRock(cr::CreativeDocument& document,
             std::string assetId = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Rock;
  request.name = "Revision Rock";
  request.assetId = std::move(assetId);
  request.transform.position = {2.0, 0.5, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{1.5, 0.0, -0.5}, {2.5, 1.0, 0.5}};
  request.hasBoundsOverride = true;
  return document.createObject(request).accepted;
}

cr::CreativeDocument playableDocument(cr::CreativeDocumentId id = 17U) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Play Map");
  if (id != cr::kInvalidDocumentId) {
    static_cast<void>(document.assignId(id));
  }
  static_cast<void>(addFloor(document));
  static_cast<void>(addSpawn(document));
  return document;
}

void addNoCollisionAsset(iggy3d::StaticMeshAssetCatalog& catalog,
                         std::string assetId) {
  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = std::move(assetId);
  entry.label = entry.assetId;
  entry.boundsMin = {-0.5F, -0.5F, -0.5F};
  entry.boundsMax = {0.5F, 0.5F, 0.5F};
  entry.authoringMetadata.collisionMode =
      iggy3d::StaticMeshCollisionMode::None;
  entry.authoringMetadata.status =
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored;
  entry.authoringMetadata.reasonCode = "asset_metadata_authored";
  entry.authoringMetadata.collisionSpecified = true;
  catalog.entries.push_back(std::move(entry));
}

const cr::CreativeMapDiagnostic* findDiagnostic(
    const cr::CreativeMapValidationResult& validation,
    cr::CreativeMapDiagnosticCode code) {
  const auto found = std::find_if(
      validation.diagnostics.begin(), validation.diagnostics.end(),
      [code](const cr::CreativeMapDiagnostic& diagnostic) {
        return diagnostic.code == code;
      });
  return found == validation.diagnostics.end() ? nullptr : &*found;
}

bool validMapProducesActivationSnapshot() {
  iggy3d::StaticMeshAssetCatalog catalog;
  const cr::CreativeDocument document = playableDocument();
  cr::CreativePlayPreparationRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  request.roomId = "play_test_room";
  const cr::CreativePlayPreparationResult result =
      cr::prepareCreativePlay(request);
  if (!result.payload.has_value()) {
    return expect(false, "prepared map has payload");
  }
  const cr::CreativePlayActivationPayload& payload = *result.payload;

  return expect(result.requested && result.accepted &&
                    result.status == cr::CreativePlayPreparationStatus::Prepared &&
                    result.reasonCode == "creative_play_prepared",
                "valid map prepares") &&
         expect(result.validation.accepted && result.validation.passed &&
                    result.validation.documentId == document.id() &&
                    result.validation.documentRevision == document.revision(),
                "preparation preserves validation receipt") &&
         expect(payload.documentId == document.id() &&
                    payload.documentRevision == document.revision() &&
                    payload.roomId == "play_test_room" &&
                    payload.room.id == payload.roomId,
                "payload is identity and revision stamped") &&
         expect(payload.playerSpawn.kind == "spawn" &&
                    payload.playerSpawnObjectId != cr::kInvalidObjectId &&
                    payload.playerSpawn.positionMeters.x == 1.0F &&
                    payload.playerSpawn.positionMeters.y == 0.25F &&
                    payload.playerSpawn.positionMeters.z == -1.0F &&
                    payload.playerSpawnSettings ==
                        cr::CreativePlayerSpawnSettings{} &&
                    payload.playerSpawnYawRadians == 0.0F &&
                    payload.playerSpawnCameraPositionMeters.x == 1.0F &&
                    std::fabs(payload.playerSpawnCameraPositionMeters.y -
                              (0.25F +
                               cr::kCreativeDefaultPlayerEyeHeightMeters)) <
                        0.0001F &&
                    payload.playerSpawnCameraPositionMeters.z == -1.0F,
                "payload preserves baked player spawn") &&
         expect(result.validation.roomBake.bakedSpatialSurfaceCount ==
                    payload.room.spatialSurfaces.size() &&
                    result.validation.roomBake.bakedAnchorCount ==
                        payload.room.anchors.size(),
                "validation and payload share one bake result") &&
         expect(result.npcSpawns.accepted &&
                    result.npcSpawns.actors.empty() &&
                    result.npcSpawns.patrolRoutes.empty() &&
                    payload.npcSpawns.empty() &&
                    payload.npcPatrolRoutes.empty(),
                "maps without authored npcs carry an explicit empty plan") &&
         expect(cr::creativePlayActivationIsCurrent(payload, document),
                "fresh payload matches source document") &&
         expect(cr::toString(result.status) == "prepared",
                "prepared status string is stable");
}

bool explicitNpcRouteEntersActivationSnapshot() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(31U);
  const cr::CreativeDocumentCreateReceipt route = addPatrolRoute(document);
  const cr::CreativeDocumentCreateReceipt actor =
      addNpc(document, route.objectId);
  const cr::CreativePlayPreparationResult result =
      cr::prepareCreativePlay({&document, &catalog});

  if (!result.payload.has_value()) {
    return expect(false, "explicit npc route prepares a payload");
  }
  const cr::CreativePlayActivationPayload& payload = *result.payload;
  return expect(route.accepted && actor.accepted && result.accepted &&
                    result.npcSpawns.accepted,
                "explicit npc route passes preparation") &&
         expect(payload.npcSpawns.size() == 1U &&
                    payload.npcPatrolRoutes.size() == 1U &&
                    payload.npcSpawns.front().objectId == actor.objectId &&
                    payload.npcSpawns.front().patrolRouteObjectId ==
                        route.objectId &&
                    payload.npcPatrolRoutes.front().objectId ==
                        route.objectId &&
                    payload.npcPatrolRoutes.front().path.size() == 2U,
                "activation snapshot carries normalized ownership once");
}

bool prioritySelectsOneActivationSpawn() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Fallback Spawn Map");
  static_cast<void>(document.assignId(23U));
  const bool floorCreated = addFloor(document);

  cr::CreativePlayerSpawnSettings fallbackSettings;
  fallbackSettings.spawnGroup = "north_entry";
  fallbackSettings.validationRadiusMeters = 0.5;
  fallbackSettings.fallbackPriority = 7U;
  const cr::CreativeDocumentCreateReceipt fallback = addSpawn(
      document, {1.0, 0.25, -1.0}, fallbackSettings, -0.5,
      "North Entry Fallback");

  cr::CreativePlayerSpawnSettings preferredSettings;
  preferredSettings.spawnGroup = "north_entry";
  preferredSettings.validationRadiusMeters = 0.65;
  preferredSettings.fallbackPriority = 2U;
  const cr::CreativeDocumentCreateReceipt preferred = addSpawn(
      document, {-1.0, 0.25, 1.0}, preferredSettings, 1.25,
      "North Entry Preferred");

  cr::CreativePlayPreparationRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  request.roomId = "fallback_spawn_room";
  request.playerSpawnGroup = "north_entry";
  const cr::CreativePlayPreparationResult result =
      cr::prepareCreativePlay(request);
  if (!result.payload.has_value()) {
    return expect(false, "priority spawn map prepares payload");
  }
  const cr::CreativePlayActivationPayload& payload = *result.payload;
  const std::size_t spawnAnchorCount = static_cast<std::size_t>(std::count_if(
      payload.room.anchors.begin(), payload.room.anchors.end(),
      [](const iggy3d::RoomAnchorAsset& anchor) {
        return anchor.kind == "spawn";
      }));

  return expect(floorCreated && fallback.accepted && preferred.accepted &&
                    result.accepted && result.validation.passed,
                "multiple valid fallback spawns prepare") &&
         expect(result.validation.playerSpawn.accepted &&
                    result.validation.playerSpawn.groupCandidateCount == 2U &&
                    result.validation.playerSpawn.rejectedCandidateCount == 0U &&
                    result.validation.playerSpawn.selected.objectId ==
                        preferred.objectId,
                "lowest authored priority selects deterministically") &&
         expect(payload.playerSpawnObjectId == preferred.objectId &&
                    payload.playerSpawnSettings == preferredSettings &&
                    payload.playerSpawnYawRadians == 1.25F &&
                    payload.playerSpawn.positionMeters.x == -1.0F &&
                    payload.playerSpawn.positionMeters.y == 0.25F &&
                    payload.playerSpawn.positionMeters.z == 1.0F &&
                    payload.playerSpawnCameraPositionMeters.x == -1.0F &&
                    std::fabs(payload.playerSpawnCameraPositionMeters.y -
                              (0.25F +
                               cr::kCreativeDefaultPlayerEyeHeightMeters)) <
                        0.0001F &&
                    payload.playerSpawnCameraPositionMeters.z == 1.0F,
                "selected identity settings pose and camera enter payload") &&
         expect(spawnAnchorCount == 1U &&
                    payload.room.anchors.back().kind == "spawn" &&
                    payload.room.anchors.back().id == payload.playerSpawn.id,
                "activation room contains only selected spawn anchor");
}

bool validationFailureReturnsDiagnosticsWithoutPayload() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = cr::CreativeDocument::create("No Spawn");
  static_cast<void>(document.assignId(18U));
  const bool floorCreated = addFloor(document);
  const cr::CreativePlayPreparationResult result =
      cr::prepareCreativePlay({&document, &catalog});

  return expect(floorCreated && result.requested && !result.accepted &&
                    result.status ==
                        cr::CreativePlayPreparationStatus::ValidationFailed,
                "invalid map is not prepared") &&
         expect(result.validation.accepted && !result.validation.passed &&
                    findDiagnostic(
                        result.validation,
                        cr::CreativeMapDiagnosticCode::NoPlayerSpawn) != nullptr,
                "validation diagnostics survive preparation rejection") &&
         expect(!result.payload.has_value(),
                "validation failure cannot leak activation payload");
}

bool linkedPlatformWithoutCollisionCannotPrepare() {
  iggy3d::StaticMeshAssetCatalog catalog;
  addNoCollisionAsset(catalog, "unphysical_platform");
  cr::CreativeDocument document = playableDocument(22U);

  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::Button;
  sourceRequest.name = "Platform Button";
  sourceRequest.transform.position = {-1.0, 0.5, 0.0};
  sourceRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt source =
      document.createObject(sourceRequest);

  cr::CreativeDocumentCreateRequest platformRequest;
  platformRequest.kind = cr::CreativeObjectKind::Platform;
  platformRequest.name = "Unphysical Platform";
  platformRequest.assetId = "unphysical_platform";
  platformRequest.transform.position = {0.0, 1.5, 1.0};
  platformRequest.hasTransformOverride = true;
  platformRequest.bounds = {{-1.5, 0.0, 0.825}, {1.5, 3.0, 1.175}};
  platformRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt platform =
      document.createObject(platformRequest);
  const bool linked =
      source.accepted && platform.accepted &&
      document
          .setLogicLink({source.objectId, platform.objectId,
                         cr::CreativeLogicLinkAction::Enable})
          .accepted;

  const cr::CreativePlayPreparationResult result =
      cr::prepareCreativePlay({&document, &catalog});
  const cr::CreativeMapDiagnostic* diagnostic = findDiagnostic(
      result.validation,
      cr::CreativeMapDiagnosticCode::LogicTargetCollisionMissing);

  return expect(linked && !result.accepted &&
                    result.status ==
                        cr::CreativePlayPreparationStatus::ValidationFailed,
                "linked platform without collision fails preparation") &&
         expect(diagnostic != nullptr &&
                    diagnostic->objectId == platform.objectId &&
                    diagnostic->subject == "Unphysical Platform",
                "preparation returns object-specific collision diagnostic") &&
         expect(!result.payload.has_value(),
                "invalid linked platform cannot leak activation payload");
}

bool missingDocumentAndIdentityFailClosed() {
  const cr::CreativePlayPreparationResult missing =
      cr::prepareCreativePlay({});

  iggy3d::StaticMeshAssetCatalog catalog;
  const cr::CreativeDocument unsaved =
      playableDocument(cr::kInvalidDocumentId);
  const cr::CreativePlayPreparationResult invalidIdentity =
      cr::prepareCreativePlay({&unsaved, &catalog});

  return expect(!missing.accepted &&
                    missing.status ==
                        cr::CreativePlayPreparationStatus::MissingDocument &&
                    missing.validation.status ==
                        cr::CreativeMapValidationStatus::MissingDocument &&
                    !missing.payload.has_value(),
                "missing document fails before activation") &&
         expect(!invalidIdentity.accepted &&
                    invalidIdentity.status ==
                        cr::CreativePlayPreparationStatus::
                            InvalidDocumentIdentity &&
                    invalidIdentity.validation.passed &&
                    !invalidIdentity.payload.has_value(),
                "unsaved document cannot produce durable activation identity");
}

bool unresolvedAssetPreventsPreparation() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(19U);
  const bool rockCreated = addRock(document, "missing_play_asset");
  const cr::CreativePlayPreparationResult result =
      cr::prepareCreativePlay({&document, &catalog});

  return expect(rockCreated && !result.accepted &&
                    result.status ==
                        cr::CreativePlayPreparationStatus::ValidationFailed,
                "unresolved asset blocks play preparation") &&
         expect(findDiagnostic(
                    result.validation,
                    cr::CreativeMapDiagnosticCode::MissingStaticMeshAsset) !=
                    nullptr &&
                    !result.payload.has_value(),
                "asset failure remains actionable without payload");
}

bool payloadFreshnessRejectsRevisionAndIdentityDrift() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(20U);
  const cr::CreativePlayPreparationResult prepared =
      cr::prepareCreativePlay({&document, &catalog});
  if (!prepared.payload.has_value()) {
    return expect(false, "freshness setup prepares payload");
  }
  const cr::CreativePlayActivationPayload& payload = *prepared.payload;
  const std::uint64_t preparedRevision = document.revision();
  const bool mutated = addRock(document);

  const cr::CreativeDocument differentIdentity = playableDocument(21U);
  return expect(mutated && document.revision() > preparedRevision,
                "freshness setup changes revision") &&
         expect(!cr::creativePlayActivationIsCurrent(payload, document),
                "document mutation stales activation payload") &&
         expect(differentIdentity.revision() == payload.documentRevision &&
                    !cr::creativePlayActivationIsCurrent(payload,
                                                         differentIdentity),
                "matching revision from another document is still stale");
}

}  // namespace

int main() {
  const bool ok = validMapProducesActivationSnapshot() &&
                  explicitNpcRouteEntersActivationSnapshot() &&
                  prioritySelectsOneActivationSpawn() &&
                  validationFailureReturnsDiagnosticsWithoutPayload() &&
                  linkedPlatformWithoutCollisionCannotPrepare() &&
                  missingDocumentAndIdentityFailClosed() &&
                  unresolvedAssetPreventsPreparation() &&
                  payloadFreshnessRejectsRevisionAndIdentityDrift();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
