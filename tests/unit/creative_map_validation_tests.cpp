#include "app/iggy3d/creative/validation/MapValidation.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool addFloor(cr::CreativeDocument& document,
              double centerX,
              double centerZ,
              double halfExtent = 2.0) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Floor;
  request.name = "Validation Floor";
  request.transform.position = {centerX, 0.0, centerZ};
  request.hasTransformOverride = true;
  request.bounds = {{centerX - halfExtent, 0.0, centerZ - halfExtent},
                    {centerX + halfExtent, 0.25,
                     centerZ + halfExtent}};
  request.hasBoundsOverride = true;
  return document.createObject(request).accepted;
}

bool addSpawn(cr::CreativeDocument& document,
              double x,
              double z,
              bool visible = true) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::SpawnPoint;
  request.name = "Player Spawn";
  request.transform.position = {x, 0.25, z};
  request.hasTransformOverride = true;
  request.visible = visible;
  request.hasVisibleOverride = true;
  return document.createObject(request).accepted;
}

bool addAsset(cr::CreativeDocument& document,
              std::string assetId,
              double x,
              double pitchRadians = 0.0) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Rock;
  request.name = assetId;
  request.assetId = std::move(assetId);
  request.transform.position = {x, 0.5, 0.0};
  request.transform.rotationEulerRadians.x = pitchRadians;
  request.hasTransformOverride = true;
  request.bounds = {{x - 0.5, 0.0, -0.5}, {x + 0.5, 1.0, 0.5}};
  request.hasBoundsOverride = true;
  return document.createObject(request).accepted;
}

bool addPatrolRoute(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.name = "Unbridged Patrol Route";
  request.hasPathOverride = true;
  request.pathPoints = {{{-1.0, 0.25, 0.0}}, {{1.0, 0.25, 0.0}}};
  return document.createObject(request).accepted;
}

bool addEditorOnlyReference(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::ReferenceImage;
  request.name = "Editor Reference";
  request.assetId = "editor_reference";
  request.transform.position = {0.0, 2.0, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{-2.0, 1.99, -2.0}, {2.0, 2.01, 2.0}};
  request.hasBoundsOverride = true;
  return document.createObject(request).accepted;
}

cr::CreativeDocument playableDocument(std::string name = "Playable") {
  cr::CreativeDocument document = cr::CreativeDocument::create(std::move(name));
  static_cast<void>(document.assignId(42U));
  static_cast<void>(addFloor(document, 0.0, 0.0));
  static_cast<void>(addSpawn(document, 0.0, 0.0));
  return document;
}

const cr::CreativeMapDiagnostic* findDiagnostic(
    const cr::CreativeMapValidationResult& result,
    cr::CreativeMapDiagnosticCode code) {
  const auto found = std::find_if(
      result.diagnostics.begin(), result.diagnostics.end(),
      [code](const cr::CreativeMapDiagnostic& diagnostic) {
        return diagnostic.code == code;
      });
  return found == result.diagnostics.end() ? nullptr : &*found;
}

std::size_t diagnosticCount(const cr::CreativeMapValidationResult& result,
                            cr::CreativeMapDiagnosticCode code) {
  return static_cast<std::size_t>(std::count_if(
      result.diagnostics.begin(), result.diagnostics.end(),
      [code](const cr::CreativeMapDiagnostic& diagnostic) {
        return diagnostic.code == code;
      }));
}

iggy3d::StaticMeshAssetCatalogEntry catalogEntry(
    std::string assetId,
    iggy3d::StaticMeshCollisionMode collisionMode,
    iggy3d::StaticMeshAuthoringMetadataStatus status,
    std::string_view reasonCode,
    bool walkable = false) {
  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = std::move(assetId);
  entry.label = entry.assetId;
  entry.boundsMin = {-0.5F, -0.5F, -0.5F};
  entry.boundsMax = {0.5F, 0.5F, 0.5F};
  entry.authoringMetadata.collisionMode = collisionMode;
  entry.authoringMetadata.status = status;
  entry.authoringMetadata.reasonCode = reasonCode;
  entry.authoringMetadata.walkable = walkable;
  entry.authoringMetadata.collisionSpecified = true;
  entry.authoringMetadata.walkableSpecified = walkable;
  return entry;
}

bool missingDocumentFailsClosed() {
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap({});
  const cr::CreativeMapDiagnostic* diagnostic =
      findDiagnostic(result, cr::CreativeMapDiagnosticCode::DocumentMissing);

  return expect(result.requested && !result.accepted && !result.passed,
                "missing document fails closed") &&
         expect(result.status ==
                    cr::CreativeMapValidationStatus::MissingDocument,
                "missing document status") &&
         expect(result.summary.errorCount == 1U &&
                    result.diagnostics.size() == 1U && diagnostic != nullptr,
                "missing document emits one structured error") &&
         expect(cr::toString(result.status) == "missing_document" &&
                    cr::toString(diagnostic->severity) == "error" &&
                    cr::toString(diagnostic->code) == "document_missing",
                "diagnostic strings are stable");
}

bool connectedMapPassesWithRevisionStamp() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument();
  const bool editorReferenceCreated = addEditorOnlyReference(document);
  cr::CreativeMapValidationRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap(request);

  return expect(editorReferenceCreated && result.accepted && result.passed &&
                    result.status == cr::CreativeMapValidationStatus::Validated,
                "connected map passes") &&
         expect(result.documentId == document.id() &&
                    result.documentRevision == document.revision(),
                "result is stamped with validated document identity") &&
         expect(result.roomBake.accepted &&
                    result.reachability.status ==
                        cr::CreativeRoomBakeReachabilityStatus::Reachable,
                "validator reuses successful bake reachability") &&
         expect(result.summary.playerSpawnCount == 1U &&
                    result.summary.runtimeObjectCount == 2U &&
                    result.summary.referencedStaticMeshAssetCount == 0U &&
                    result.summary.errorCount == 0U &&
                    result.diagnostics.empty(),
                "connected map summary ignores editor-only references");
}

bool spawnCardinalityIsExplicit() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument missing = cr::CreativeDocument::create("No Spawn");
  static_cast<void>(addFloor(missing, 0.0, 0.0));
  const cr::CreativeMapValidationResult missingResult =
      cr::validateCreativeMap({&missing, &catalog});

  cr::CreativeDocument multiple = playableDocument("Multiple Spawns");
  const bool secondCreated = addSpawn(multiple, 1.0, 1.0);
  const cr::CreativeMapValidationResult multipleResult =
      cr::validateCreativeMap({&multiple, &catalog});

  cr::CreativeDocument hidden = cr::CreativeDocument::create("Hidden Spawn");
  const bool hiddenSetup = addFloor(hidden, 0.0, 0.0) &&
                           addSpawn(hidden, 0.0, 0.0, false);
  const cr::CreativeMapValidationResult hiddenDefault =
      cr::validateCreativeMap({&hidden, &catalog});
  cr::CreativeMapValidationRequest includeHiddenRequest;
  includeHiddenRequest.document = &hidden;
  includeHiddenRequest.staticMeshAssetCatalog = &catalog;
  includeHiddenRequest.includeHidden = true;
  const cr::CreativeMapValidationResult hiddenIncluded =
      cr::validateCreativeMap(includeHiddenRequest);

  return expect(!missingResult.passed &&
                    findDiagnostic(missingResult,
                                   cr::CreativeMapDiagnosticCode::NoPlayerSpawn) !=
                        nullptr &&
                    findDiagnostic(
                        missingResult,
                        cr::CreativeMapDiagnosticCode::
                            ReachabilityNoUsableSeeds) != nullptr,
                "map without a player spawn cannot pass") &&
         expect(secondCreated && !multipleResult.passed &&
                    multipleResult.summary.playerSpawnCount == 2U,
                "multiple player spawns cannot pass") &&
         expect(findDiagnostic(
                    multipleResult,
                    cr::CreativeMapDiagnosticCode::MultiplePlayerSpawns) !=
                    nullptr,
                "multiple spawn diagnostic is explicit") &&
         expect(hiddenSetup && !hiddenDefault.passed &&
                    hiddenDefault.summary.playerSpawnCount == 0U,
                "hidden spawn does not satisfy normal validation") &&
         expect(hiddenIncluded.passed &&
                    hiddenIncluded.summary.playerSpawnCount == 1U,
                "includeHidden explicitly admits hidden spawn");
}

bool importedAssetFailuresAreActionable() {
  iggy3d::StaticMeshAssetCatalog emptyCatalog;
  cr::CreativeDocument missing = playableDocument("Missing Asset");
  const bool missingCreated = addAsset(missing, "missing_rock", 1.0);
  const cr::CreativeMapValidationResult missingResult =
      cr::validateCreativeMap({&missing, &emptyCatalog});
  const cr::CreativeMapDiagnostic* missingDiagnostic = findDiagnostic(
      missingResult, cr::CreativeMapDiagnosticCode::MissingStaticMeshAsset);

  cr::CreativeDocument unavailable = playableDocument("No Catalog");
  const bool unavailableCreated = addAsset(unavailable, "catalogless_rock", 1.0);
  const cr::CreativeMapValidationResult unavailableResult =
      cr::validateCreativeMap({&unavailable, nullptr});
  const cr::CreativeMapDiagnostic* unavailableDiagnostic = findDiagnostic(
      unavailableResult, cr::CreativeMapDiagnosticCode::AssetCatalogUnavailable);

  iggy3d::StaticMeshAssetCatalog badCatalog;
  badCatalog.entries.push_back(catalogEntry(
      "bad_collision", iggy3d::StaticMeshCollisionMode::Mesh,
      iggy3d::StaticMeshAuthoringMetadataStatus::UnsupportedCollision,
      "mesh_collision_not_supported"));
  badCatalog.entries.push_back(catalogEntry(
      "bad_metadata", iggy3d::StaticMeshCollisionMode::Invalid,
      iggy3d::StaticMeshAuthoringMetadataStatus::Invalid,
      "asset_metadata_invalid"));
  cr::CreativeDocument unsupported = playableDocument("Unsupported Asset");
  const bool unsupportedCreated = addAsset(unsupported, "bad_collision", 1.0) &&
                                  addAsset(unsupported, "bad_metadata", 2.0);
  const cr::CreativeMapValidationResult unsupportedResult =
      cr::validateCreativeMap({&unsupported, &badCatalog});
  const cr::CreativeMapDiagnostic* unsupportedDiagnostic = findDiagnostic(
      unsupportedResult,
      cr::CreativeMapDiagnosticCode::UnsupportedStaticMeshCollision);
  const cr::CreativeMapDiagnostic* invalidDiagnostic = findDiagnostic(
      unsupportedResult,
      cr::CreativeMapDiagnosticCode::InvalidStaticMeshMetadata);

  return expect(missingCreated && !missingResult.passed &&
                    missingDiagnostic != nullptr &&
                    missingDiagnostic->subject == "missing_rock" &&
                    missingDiagnostic->objectId != cr::kInvalidObjectId,
                "missing asset identifies object and asset id") &&
         expect(unavailableCreated && !unavailableResult.passed &&
                    unavailableDiagnostic != nullptr &&
                    unavailableDiagnostic->fact == 1U,
                "absent catalog reports unresolved reference count") &&
         expect(unsupportedCreated && !unsupportedResult.passed &&
                    unsupportedDiagnostic != nullptr &&
                    unsupportedDiagnostic->subject == "bad_collision" &&
                    diagnosticCount(
                        unsupportedResult,
                        cr::CreativeMapDiagnosticCode::
                            UnsupportedStaticMeshCollision) == 1U,
                "unsupported collision emits one non-duplicated asset error") &&
         expect(invalidDiagnostic != nullptr &&
                    invalidDiagnostic->subject == "bad_metadata" &&
                    diagnosticCount(
                        unsupportedResult,
                        cr::CreativeMapDiagnosticCode::
                            InvalidStaticMeshMetadata) == 1U,
                "invalid metadata emits one non-duplicated asset error");
}

bool lostAssetWalkabilityIsWarningOnly() {
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "tilted_walkable", iggy3d::StaticMeshCollisionMode::Bounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored,
      "asset_metadata_authored", true));
  cr::CreativeDocument document = playableDocument("Tilted Walkable");
  const bool created = addAsset(document, "tilted_walkable", 1.0, 0.25);
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap({&document, &catalog});
  const cr::CreativeMapDiagnostic* diagnostic = findDiagnostic(
      result, cr::CreativeMapDiagnosticCode::AssetWalkabilitySkipped);

  return expect(created && result.accepted && result.passed,
                "lost optional asset walkability does not reject map") &&
         expect(result.summary.warningCount == 1U &&
                    result.summary.errorCount == 0U &&
                    diagnostic != nullptr && diagnostic->fact == 1U,
                "tilted walkable asset emits one warning");
}

bool skippedRuntimeObjectFailsValidation() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument("Runtime Gap");
  const bool routeCreated = addPatrolRoute(document);
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap({&document, &catalog});
  const cr::CreativeMapDiagnostic* diagnostic = findDiagnostic(
      result, cr::CreativeMapDiagnosticCode::RuntimeObjectSkipped);

  return expect(routeCreated && result.roomBake.accepted,
                "runtime gap map still bakes its supported content") &&
         expect(!result.passed && diagnostic != nullptr &&
                    diagnostic->subject == "Unbridged Patrol Route" &&
                    diagnostic->detail == "Path",
                "unbridged runtime object is named and rejected");
}

bool linkedPlatformWithoutCollisionFailsValidation() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument("Invalid Logic Platform");

  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::Switch;
  sourceRequest.name = "Platform Switch";
  sourceRequest.transform.position = {-1.0, 0.5, 0.0};
  sourceRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt source =
      document.createObject(sourceRequest);

  cr::CreativeDocumentCreateRequest platformRequest;
  platformRequest.kind = cr::CreativeObjectKind::Platform;
  platformRequest.name = "Vertical Platform";
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

  const cr::CreativeMapEvaluationResult evaluation = cr::evaluateCreativeMap(
      {&document, &catalog});
  const cr::CreativeMapValidationResult& result = evaluation.validation;
  const cr::CreativeMapDiagnostic* diagnostic = findDiagnostic(
      result,
      cr::CreativeMapDiagnosticCode::LogicTargetCollisionMissing);
  const bool meshBaked = std::any_of(
      evaluation.roomBake.staticMeshSources.begin(),
      evaluation.roomBake.staticMeshSources.end(),
      [&platform](const cr::CreativeRoomBakeStaticMeshSource& sourceFact) {
        return sourceFact.objectId == platform.objectId;
      });
  const bool collisionMissing = std::none_of(
      evaluation.roomBake.spatialSurfaceSources.begin(),
      evaluation.roomBake.spatialSurfaceSources.end(),
      [&platform](const cr::CreativeRoomBakeSpatialSurfaceSource& sourceFact) {
        return sourceFact.objectId == platform.objectId;
      });

  return expect(linked && result.accepted && !result.passed && meshBaked &&
                    collisionMissing,
                "renderable linked platform without collision cannot pass") &&
         expect(diagnostic != nullptr &&
                    diagnosticCount(
                        result,
                        cr::CreativeMapDiagnosticCode::
                            LogicTargetCollisionMissing) == 1U &&
                    diagnostic->objectId == platform.objectId &&
                    diagnostic->subject == "Vertical Platform" &&
                    diagnostic->detail ==
                        "creative_map_logic_target_collision_missing",
                "missing target collision names the authored platform once") &&
         expect(cr::toString(diagnostic->code) ==
                    "logic_target_collision_missing",
                "missing target collision code is stable");
}

bool disconnectedWalkableIslandFailsValidation() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument("Island");
  const bool islandCreated = addFloor(document, 12.0, 0.0, 1.0);
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap({&document, &catalog});
  const cr::CreativeMapDiagnostic* diagnostic = findDiagnostic(
      result, cr::CreativeMapDiagnosticCode::ReachabilityIslands);

  return expect(islandCreated && !result.passed,
                "disconnected floor cannot pass") &&
         expect(result.reachability.status ==
                    cr::CreativeRoomBakeReachabilityStatus::IslandsFound &&
                    result.reachability.strandedCellCount > 0U,
                "existing reachability kernel finds island") &&
         expect(diagnostic != nullptr &&
                    diagnostic->fact == result.reachability.strandedCellCount,
                "island diagnostic carries stranded cell count");
}

bool invalidReachabilityConfigurationFailsClosed() {
  iggy3d::StaticMeshAssetCatalog catalog;
  const cr::CreativeDocument document = playableDocument("Bad Cell Size");
  cr::CreativeMapValidationRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  request.reachabilityCellSizeMeters = 0.0F;
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap(request);

  return expect(!result.passed &&
                    result.reachability.status ==
                        cr::CreativeRoomBakeReachabilityStatus::InvalidCellSize,
                "invalid reachability cell size cannot pass") &&
         expect(findDiagnostic(
                    result,
                    cr::CreativeMapDiagnosticCode::
                        ReachabilityInvalidCellSize) != nullptr,
                "invalid cell size has dedicated diagnostic");
}

bool competingPressurePlatesFailValidation() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument("Competing Plates");
  cr::CreativeDocumentCreateRequest plateRequest;
  plateRequest.kind = cr::CreativeObjectKind::PressurePlate;
  plateRequest.name = "Plate A";
  const cr::CreativeDocumentCreateReceipt plateA =
      document.createObject(plateRequest);
  plateRequest.name = "Plate B";
  const cr::CreativeDocumentCreateReceipt plateB =
      document.createObject(plateRequest);
  cr::CreativeDocumentCreateRequest doorRequest;
  doorRequest.kind = cr::CreativeObjectKind::Door;
  doorRequest.name = "Shared Door";
  const cr::CreativeDocumentCreateReceipt door =
      document.createObject(doorRequest);
  const bool linked =
      document
          .setLogicLink({plateA.objectId, door.objectId,
                         cr::CreativeLogicLinkAction::Open})
          .accepted &&
      document
          .setLogicLink({plateB.objectId, door.objectId,
                         cr::CreativeLogicLinkAction::Open})
          .accepted;
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap({&document, &catalog});
  const cr::CreativeMapDiagnostic* conflict = findDiagnostic(
      result, cr::CreativeMapDiagnosticCode::ConflictingPressurePlates);

  return expect(plateA.accepted && plateB.accepted && door.accepted && linked,
                "pressure plate conflict fixture is authored") &&
         expect(!result.passed && conflict != nullptr &&
                    conflict->objectId == plateB.objectId &&
                    conflict->fact == door.objectId,
                "map validation rejects competing hold sources") &&
         expect(cr::toString(conflict->code) ==
                    "conflicting_pressure_plates",
                "pressure plate validation code is stable");
}

bool diagnosticsStayBoundedAndDeterministic() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument("Diagnostic Capacity");
  bool allCreated = true;
  for (std::size_t index = 0; index < 300U; ++index) {
    allCreated =
        addAsset(document, "missing_asset", 10.0 + static_cast<double>(index)) &&
        allCreated;
  }
  const cr::CreativeMapValidationResult result =
      cr::validateCreativeMap({&document, &catalog});

  return expect(allCreated && !result.passed,
                "capacity fixture creates and fails") &&
         expect(result.diagnostics.size() ==
                    cr::kCreativeMapDiagnosticCapacity,
                "diagnostic storage is bounded") &&
         expect(result.summary.errorCount == 300U &&
                    result.summary.warningCount == 1U &&
                    result.summary.diagnosticsTruncated &&
                    result.summary.droppedDiagnosticCount == 45U,
                "summary retains complete failure counts") &&
         expect(result.diagnostics.front().code ==
                    cr::CreativeMapDiagnosticCode::MissingStaticMeshAsset &&
                    result.diagnostics.back().code ==
                        cr::CreativeMapDiagnosticCode::
                            DiagnosticCapacityExceeded &&
                    result.diagnostics.back().fact == 45U,
                "overflow sentinel is stable and reports dropped count");
}

}  // namespace

int main() {
  const bool ok = missingDocumentFailsClosed() &&
                  connectedMapPassesWithRevisionStamp() &&
                  spawnCardinalityIsExplicit() &&
                  importedAssetFailuresAreActionable() &&
                  lostAssetWalkabilityIsWarningOnly() &&
                  skippedRuntimeObjectFailsValidation() &&
                  linkedPlatformWithoutCollisionFailsValidation() &&
                  disconnectedWalkableIslandFailsValidation() &&
                  invalidReachabilityConfigurationFailsClosed() &&
                  competingPressurePlatesFailValidation() &&
                  diagnosticsStayBoundedAndDeterministic();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
