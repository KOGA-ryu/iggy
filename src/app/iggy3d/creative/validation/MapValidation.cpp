#include "app/iggy3d/creative/validation/MapValidation.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

class DiagnosticCollector {
 public:
  explicit DiagnosticCollector(CreativeMapValidationResult& result)
      : result_(result) {
    result_.diagnostics.reserve(kCreativeMapDiagnosticCapacity);
  }

  void add(CreativeMapDiagnosticSeverity severity,
           CreativeMapDiagnosticCode code,
           CreativeObjectId objectId = kInvalidObjectId,
           std::string subject = {},
           std::string detail = {},
           std::uint64_t fact = 0) {
    count(severity);
    if (result_.diagnostics.size() + 1U <
        kCreativeMapDiagnosticCapacity) {
      result_.diagnostics.push_back(
          {severity, code, objectId, std::move(subject), std::move(detail),
           fact});
      return;
    }
    ++droppedCount_;
  }

  void finish() {
    if (droppedCount_ == 0U) {
      return;
    }
    result_.summary.diagnosticsTruncated = true;
    result_.summary.droppedDiagnosticCount = droppedCount_;
    ++result_.summary.warningCount;
    result_.diagnostics.push_back(
        {CreativeMapDiagnosticSeverity::Warning,
         CreativeMapDiagnosticCode::DiagnosticCapacityExceeded,
         kInvalidObjectId,
         {},
         "creative_map_diagnostic_capacity_exceeded",
         droppedCount_});
  }

 private:
  void count(CreativeMapDiagnosticSeverity severity) {
    switch (severity) {
      case CreativeMapDiagnosticSeverity::Info:
        ++result_.summary.infoCount;
        return;
      case CreativeMapDiagnosticSeverity::Warning:
        ++result_.summary.warningCount;
        return;
      case CreativeMapDiagnosticSeverity::Error:
        ++result_.summary.errorCount;
        return;
    }
  }

  CreativeMapValidationResult& result_;
  std::uint64_t droppedCount_ = 0;
};

[[nodiscard]] bool includedObject(const CreativeDocument& document,
                                  const CreativeObject& object,
                                  bool includeHidden) noexcept {
  return includeHidden ||
         creativeObjectEffectivelyVisible(document, object.id);
}

[[nodiscard]] bool bakedObject(
    CreativeObjectId objectId,
    const std::unordered_set<CreativeObjectId>& bakedObjectIds) noexcept {
  return bakedObjectIds.contains(objectId);
}

[[nodiscard]] bool patrolRouteHasRuntimeOwner(
    const CreativeDocument& document,
    CreativeObjectId routeObjectId,
    bool includeHidden) noexcept {
  return std::any_of(
      document.objects().begin(), document.objects().end(),
      [&document, routeObjectId, includeHidden](const CreativeObject& object) {
        const bool isActor = object.kind == CreativeObjectKind::NpcSpawn ||
                             object.kind == CreativeObjectKind::EnemySpawn;
        return isActor && object.parentId == routeObjectId &&
               includedObject(document, object, includeHidden);
      });
}

void appendAssetMetadataDiagnostic(
    const CreativeObject& object,
    const StaticMeshAssetCatalogEntry& entry,
    DiagnosticCollector& diagnostics,
    std::uint64_t& unsupportedCount,
    std::uint64_t& invalidCount) {
  const StaticMeshAuthoringMetadata& metadata = entry.authoringMetadata;
  if (metadata.status == StaticMeshAuthoringMetadataStatus::Invalid ||
      metadata.collisionMode == StaticMeshCollisionMode::Invalid) {
    ++invalidCount;
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::InvalidStaticMeshMetadata,
                    object.id,
                    object.assetId,
                    std::string(metadata.reasonCode));
    return;
  }
  if (metadata.status ==
          StaticMeshAuthoringMetadataStatus::UnsupportedCollision ||
      metadata.collisionMode == StaticMeshCollisionMode::Convex ||
      metadata.collisionMode == StaticMeshCollisionMode::Mesh) {
    ++unsupportedCount;
    diagnostics.add(
        CreativeMapDiagnosticSeverity::Error,
        CreativeMapDiagnosticCode::UnsupportedStaticMeshCollision,
        object.id,
        object.assetId,
        std::string(toString(metadata.collisionMode)));
  }
}

void appendReachabilityDiagnostic(
    const CreativeRoomBakeReachabilityReceipt& reachability,
    DiagnosticCollector& diagnostics) {
  switch (reachability.status) {
    case CreativeRoomBakeReachabilityStatus::Unknown:
    case CreativeRoomBakeReachabilityStatus::NotRequested:
    case CreativeRoomBakeReachabilityStatus::NotChecked:
      diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                      CreativeMapDiagnosticCode::ReachabilityNotChecked,
                      kInvalidObjectId,
                      {},
                      reachability.reasonCode);
      return;
    case CreativeRoomBakeReachabilityStatus::InvalidCellSize:
      diagnostics.add(
          CreativeMapDiagnosticSeverity::Error,
          CreativeMapDiagnosticCode::ReachabilityInvalidCellSize,
          kInvalidObjectId,
          {},
          reachability.reasonCode);
      return;
    case CreativeRoomBakeReachabilityStatus::NoWalkableCells:
      return;
    case CreativeRoomBakeReachabilityStatus::GridTooLarge:
      diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                      CreativeMapDiagnosticCode::ReachabilityGridTooLarge,
                      kInvalidObjectId,
                      {},
                      reachability.reasonCode,
                      reachability.walkableCellCount);
      return;
    case CreativeRoomBakeReachabilityStatus::NoUsableSeeds:
      diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                      CreativeMapDiagnosticCode::ReachabilityNoUsableSeeds,
                      kInvalidObjectId,
                      {},
                      reachability.reasonCode,
                      reachability.blockedSeedCount);
      return;
    case CreativeRoomBakeReachabilityStatus::Reachable:
      if (reachability.blockedSeedCount > 0U) {
        diagnostics.add(CreativeMapDiagnosticSeverity::Warning,
                        CreativeMapDiagnosticCode::ReachabilityBlockedSeed,
                        kInvalidObjectId,
                        {},
                        reachability.reasonCode,
                        reachability.blockedSeedCount);
      }
      return;
    case CreativeRoomBakeReachabilityStatus::IslandsFound:
      diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                      CreativeMapDiagnosticCode::ReachabilityIslands,
                      kInvalidObjectId,
                      {},
                      reachability.reasonCode,
                      reachability.strandedCellCount);
      return;
  }
}

[[nodiscard]] CreativeMapDiagnosticCode playerSpawnDiagnosticCode(
    CreativePlayerSpawnStatus status) noexcept {
  switch (status) {
    case CreativePlayerSpawnStatus::InvalidSettings:
      return CreativeMapDiagnosticCode::PlayerSpawnSettingsInvalid;
    case CreativePlayerSpawnStatus::UnsupportedProfile:
      return CreativeMapDiagnosticCode::PlayerSpawnProfileUnsupported;
    case CreativePlayerSpawnStatus::OutsideWorldBounds:
      return CreativeMapDiagnosticCode::PlayerSpawnOutsideWorldBounds;
    case CreativePlayerSpawnStatus::UnsupportedFloor:
      return CreativeMapDiagnosticCode::PlayerSpawnFloorUnsupported;
    case CreativePlayerSpawnStatus::Obstructed:
      return CreativeMapDiagnosticCode::PlayerSpawnObstructed;
    case CreativePlayerSpawnStatus::Unreachable:
      return CreativeMapDiagnosticCode::PlayerSpawnUnreachable;
    case CreativePlayerSpawnStatus::GroupUnavailable:
      return CreativeMapDiagnosticCode::PlayerSpawnGroupUnavailable;
    case CreativePlayerSpawnStatus::NotRequested:
    case CreativePlayerSpawnStatus::MissingDocument:
    case CreativePlayerSpawnStatus::InvalidDocument:
    case CreativePlayerSpawnStatus::MissingRoomBake:
    case CreativePlayerSpawnStatus::InvalidObject:
    case CreativePlayerSpawnStatus::Ready:
      return CreativeMapDiagnosticCode::PlayerSpawnSettingsInvalid;
  }
  return CreativeMapDiagnosticCode::PlayerSpawnSettingsInvalid;
}

void appendPlayerSpawnDiagnostic(const CreativeObject& object,
                                 const CreativePlayerSpawnPlan& plan,
                                 DiagnosticCollector& diagnostics) {
  if (plan.accepted) {
    return;
  }
  diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                  playerSpawnDiagnosticCode(plan.status), object.id, object.name,
                  std::string(plan.reasonCode));
}

[[nodiscard]] CreativeMapDiagnosticCode mapLogicDiagnosticCode(
    CreativeLogicDiagnosticCode code) noexcept {
  switch (code) {
    case CreativeLogicDiagnosticCode::UnlinkedSource:
      return CreativeMapDiagnosticCode::LogicSourceUnlinked;
    case CreativeLogicDiagnosticCode::MissingTarget:
      return CreativeMapDiagnosticCode::LogicTargetMissing;
    case CreativeLogicDiagnosticCode::ConflictingPressurePlates:
      return CreativeMapDiagnosticCode::ConflictingPressurePlates;
    case CreativeLogicDiagnosticCode::Unknown:
    case CreativeLogicDiagnosticCode::InvalidAction:
    case CreativeLogicDiagnosticCode::MissingSource:
    case CreativeLogicDiagnosticCode::UnsupportedSource:
    case CreativeLogicDiagnosticCode::UnsupportedTarget:
    case CreativeLogicDiagnosticCode::DuplicatePair:
      return CreativeMapDiagnosticCode::LogicLinkInvalid;
  }
  return CreativeMapDiagnosticCode::LogicLinkInvalid;
}

void appendLogicDiagnostics(const CreativeDocument& document,
                            DiagnosticCollector& diagnostics) {
  const CreativeLogicDiagnosticReport report = buildCreativeLogicDiagnostics(
      document.logicLinks(), document.objects());
  for (std::size_t index = 0U; index < report.issueCount; ++index) {
    const CreativeLogicDiagnostic& issue = report.issues[index];
    const CreativeObject* source =
        document.findObject(issue.sourceObjectId);
    diagnostics.add(
        issue.severity == CreativeLogicDiagnosticSeverity::Error
            ? CreativeMapDiagnosticSeverity::Error
            : CreativeMapDiagnosticSeverity::Warning,
        mapLogicDiagnosticCode(issue.code),
        issue.sourceObjectId,
        source != nullptr ? source->name : std::string{},
        std::string(toString(issue.code)),
        issue.targetObjectId);
  }
  if (report.capacityExceeded) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Warning,
                    CreativeMapDiagnosticCode::DiagnosticCapacityExceeded,
                    kInvalidObjectId,
                    {},
                    "creative_logic_diagnostic_capacity_exceeded",
                    report.droppedIssueCount);
  }
}

void appendLogicTargetCollisionDiagnostics(
    const CreativeDocument& document,
    const CreativeRoomBakeResult& baked,
    bool includeHidden,
    DiagnosticCollector& diagnostics) {
  std::unordered_set<CreativeObjectId> surfaceObjectIds;
  surfaceObjectIds.reserve(baked.spatialSurfaceSources.size());
  for (const CreativeRoomBakeSpatialSurfaceSource& source :
       baked.spatialSurfaceSources) {
    surfaceObjectIds.insert(source.objectId);
  }

  std::unordered_set<CreativeObjectId> diagnosedTargets;
  diagnosedTargets.reserve(document.logicLinks().size());
  for (const CreativeLogicLink& link : document.logicLinks()) {
    const CreativeObject* target = document.findObject(link.targetObjectId);
    if (target == nullptr ||
        (target->kind != CreativeObjectKind::Platform &&
         target->kind != CreativeObjectKind::MovingPlatform) ||
        !includedObject(document, *target, includeHidden) ||
        surfaceObjectIds.contains(target->id) ||
        !diagnosedTargets.insert(target->id).second) {
      continue;
    }
    diagnostics.add(
        CreativeMapDiagnosticSeverity::Error,
        CreativeMapDiagnosticCode::LogicTargetCollisionMissing,
        target->id,
        target->name,
        "creative_map_logic_target_collision_missing");
  }
}

}  // namespace

std::string_view toString(CreativeMapValidationStatus status) noexcept {
  switch (status) {
    case CreativeMapValidationStatus::NotRequested:
      return "not_requested";
    case CreativeMapValidationStatus::MissingDocument:
      return "missing_document";
    case CreativeMapValidationStatus::InvalidDocument:
      return "invalid_document";
    case CreativeMapValidationStatus::Validated:
      return "validated";
  }
  return "not_requested";
}

std::string_view toString(CreativeMapDiagnosticSeverity severity) noexcept {
  switch (severity) {
    case CreativeMapDiagnosticSeverity::Info:
      return "info";
    case CreativeMapDiagnosticSeverity::Warning:
      return "warning";
    case CreativeMapDiagnosticSeverity::Error:
      return "error";
  }
  return "info";
}

std::string_view toString(CreativeMapDiagnosticCode code) noexcept {
  switch (code) {
    case CreativeMapDiagnosticCode::Unknown:
      return "unknown";
    case CreativeMapDiagnosticCode::DocumentMissing:
      return "document_missing";
    case CreativeMapDiagnosticCode::DocumentInvalid:
      return "document_invalid";
    case CreativeMapDiagnosticCode::AssetCatalogUnavailable:
      return "asset_catalog_unavailable";
    case CreativeMapDiagnosticCode::MissingStaticMeshAsset:
      return "missing_static_mesh_asset";
    case CreativeMapDiagnosticCode::UnsupportedStaticMeshCollision:
      return "unsupported_static_mesh_collision";
    case CreativeMapDiagnosticCode::InvalidStaticMeshMetadata:
      return "invalid_static_mesh_metadata";
    case CreativeMapDiagnosticCode::NoPlayerSpawn:
      return "no_player_spawn";
    case CreativeMapDiagnosticCode::MultiplePlayerSpawns:
      return "multiple_player_spawns";
    case CreativeMapDiagnosticCode::PlayerSpawnSettingsInvalid:
      return "player_spawn_settings_invalid";
    case CreativeMapDiagnosticCode::PlayerSpawnProfileUnsupported:
      return "player_spawn_profile_unsupported";
    case CreativeMapDiagnosticCode::PlayerSpawnOutsideWorldBounds:
      return "player_spawn_outside_world_bounds";
    case CreativeMapDiagnosticCode::PlayerSpawnFloorUnsupported:
      return "player_spawn_floor_unsupported";
    case CreativeMapDiagnosticCode::PlayerSpawnObstructed:
      return "player_spawn_obstructed";
    case CreativeMapDiagnosticCode::PlayerSpawnUnreachable:
      return "player_spawn_unreachable";
    case CreativeMapDiagnosticCode::PlayerSpawnGroupUnavailable:
      return "player_spawn_group_unavailable";
    case CreativeMapDiagnosticCode::NpcSpawnPlanInvalid:
      return "npc_spawn_plan_invalid";
    case CreativeMapDiagnosticCode::RoomBakeRejected:
      return "room_bake_rejected";
    case CreativeMapDiagnosticCode::RuntimeObjectSkipped:
      return "runtime_object_skipped";
    case CreativeMapDiagnosticCode::InvalidRuntimeBounds:
      return "invalid_runtime_bounds";
    case CreativeMapDiagnosticCode::AssetWalkabilitySkipped:
      return "asset_walkability_skipped";
    case CreativeMapDiagnosticCode::NoWalkableSurface:
      return "no_walkable_surface";
    case CreativeMapDiagnosticCode::ReachabilityNotChecked:
      return "reachability_not_checked";
    case CreativeMapDiagnosticCode::ReachabilityInvalidCellSize:
      return "reachability_invalid_cell_size";
    case CreativeMapDiagnosticCode::ReachabilityGridTooLarge:
      return "reachability_grid_too_large";
    case CreativeMapDiagnosticCode::ReachabilityNoUsableSeeds:
      return "reachability_no_usable_seeds";
    case CreativeMapDiagnosticCode::ReachabilityBlockedSeed:
      return "reachability_blocked_seed";
    case CreativeMapDiagnosticCode::ReachabilityIslands:
      return "reachability_islands";
    case CreativeMapDiagnosticCode::LogicSourceUnlinked:
      return "logic_source_unlinked";
    case CreativeMapDiagnosticCode::LogicTargetMissing:
      return "logic_target_missing";
    case CreativeMapDiagnosticCode::LogicTargetCollisionMissing:
      return "logic_target_collision_missing";
    case CreativeMapDiagnosticCode::LogicLinkInvalid:
      return "logic_link_invalid";
    case CreativeMapDiagnosticCode::ConflictingPressurePlates:
      return "conflicting_pressure_plates";
    case CreativeMapDiagnosticCode::DiagnosticCapacityExceeded:
      return "diagnostic_capacity_exceeded";
  }
  return "unknown";
}

CreativeMapEvaluationResult evaluateCreativeMap(
    const CreativeMapValidationRequest& request) {
  CreativeMapEvaluationResult evaluation;
  CreativeMapValidationResult& result = evaluation.validation;
  result.requested = true;
  DiagnosticCollector diagnostics(result);

  if (request.document == nullptr) {
    result.status = CreativeMapValidationStatus::MissingDocument;
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::DocumentMissing,
                    kInvalidObjectId,
                    {},
                    "creative_map_validation_document_missing");
    diagnostics.finish();
    return evaluation;
  }

  const CreativeDocument& document = *request.document;
  result.documentId = document.id();
  result.documentRevision = document.revision();
  if (!document.isValid()) {
    result.status = CreativeMapValidationStatus::InvalidDocument;
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::DocumentInvalid,
                    kInvalidObjectId,
                    {},
                    "creative_map_validation_document_invalid");
    diagnostics.finish();
    return evaluation;
  }

  result.status = CreativeMapValidationStatus::Validated;
  result.accepted = true;

  std::vector<const CreativeObject*> runtimeObjects;
  runtimeObjects.reserve(document.objects().size());
  std::vector<const CreativeObject*> playerSpawns;
  playerSpawns.reserve(document.objects().size());
  std::uint64_t unsupportedAssetCount = 0;
  std::uint64_t invalidAssetCount = 0;
  for (const CreativeObject& object : document.objects()) {
    if (!includedObject(document, object, request.includeHidden)) {
      continue;
    }

    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    if (descriptor.isEditorOnly) {
      continue;
    }
    if (descriptor.isRuntimeMeaningful) {
      runtimeObjects.push_back(&object);
      ++result.summary.runtimeObjectCount;
      if (descriptor.runtimeAnchorSemantic ==
          CreativeRuntimeAnchorSemantic::Spawn) {
        ++result.summary.playerSpawnCount;
        playerSpawns.push_back(&object);
      }
    }

    if (object.assetId.empty()) {
      continue;
    }
    ++result.summary.referencedStaticMeshAssetCount;
    if (request.staticMeshAssetCatalog == nullptr) {
      continue;
    }
    const StaticMeshAssetCatalogEntry* entry =
        request.staticMeshAssetCatalog->find(object.assetId);
    if (entry == nullptr) {
      diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                      CreativeMapDiagnosticCode::MissingStaticMeshAsset,
                      object.id,
                      object.assetId,
                      "creative_map_asset_missing");
      continue;
    }
    appendAssetMetadataDiagnostic(object,
                                  *entry,
                                  diagnostics,
                                  unsupportedAssetCount,
                                  invalidAssetCount);
  }

  if (result.summary.referencedStaticMeshAssetCount > 0U &&
      request.staticMeshAssetCatalog == nullptr) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::AssetCatalogUnavailable,
                    kInvalidObjectId,
                    {},
                    "creative_map_asset_catalog_unavailable",
                    result.summary.referencedStaticMeshAssetCount);
  }

  if (result.summary.playerSpawnCount == 0U) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::NoPlayerSpawn,
                    kInvalidObjectId,
                    {},
                    "creative_map_player_spawn_missing");
  }

  appendLogicDiagnostics(document, diagnostics);

  CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &document;
  bakeRequest.roomId = request.roomId;
  bakeRequest.sourceName = std::string(document.name());
  bakeRequest.includeHidden = request.includeHidden;
  bakeRequest.validateReachability = true;
  bakeRequest.reachabilityCellSizeMeters =
      request.reachabilityCellSizeMeters;
  bakeRequest.staticMeshAssetCatalog = request.staticMeshAssetCatalog;
  evaluation.roomBake = buildRoomAssetFromCreativeDocument(bakeRequest);
  const CreativeRoomBakeResult& baked = evaluation.roomBake;
  result.roomBake = baked.receipt;
  result.reachability = baked.reachability;

  if (!baked.receipt.accepted) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::RoomBakeRejected,
                    kInvalidObjectId,
                    {},
                    baked.receipt.reasonCode);
  } else {
    appendLogicTargetCollisionDiagnostics(
        document, baked, request.includeHidden, diagnostics);
    for (const CreativeObject* spawn : playerSpawns) {
      appendPlayerSpawnDiagnostic(
          *spawn,
          planCreativePlayerSpawn({&document, &baked, spawn,
                                   request.reachabilityCellSizeMeters}),
          diagnostics);
    }
    result.playerSpawn = resolveCreativePlayerSpawn(
        {&document, &baked, request.playerSpawnGroup,
         request.reachabilityCellSizeMeters, request.includeHidden});
    if (!result.playerSpawn.accepted &&
        result.playerSpawn.status ==
            CreativePlayerSpawnStatus::GroupUnavailable &&
        result.summary.playerSpawnCount > 0U) {
      diagnostics.add(
          CreativeMapDiagnosticSeverity::Error,
          CreativeMapDiagnosticCode::PlayerSpawnGroupUnavailable,
          kInvalidObjectId, {}, std::string(result.playerSpawn.reasonCode),
          result.playerSpawn.groupCandidateCount);
    }

    evaluation.npcSpawns =
        planCreativeNpcSpawns({&document, &baked, request.includeHidden});
    if (!evaluation.npcSpawns.accepted) {
      const CreativeObject* failedObject =
          document.findObject(evaluation.npcSpawns.failedObjectId);
      diagnostics.add(
          CreativeMapDiagnosticSeverity::Error,
          CreativeMapDiagnosticCode::NpcSpawnPlanInvalid,
          evaluation.npcSpawns.failedObjectId,
          failedObject == nullptr ? std::string{} : failedObject->name,
          std::string(evaluation.npcSpawns.reasonCode));
    }
  }

  std::unordered_set<CreativeObjectId> bakedObjectIds;
  bakedObjectIds.reserve(baked.anchorSources.size() +
                         baked.staticMeshSources.size());
  for (const CreativeRoomBakeAnchorSource& source : baked.anchorSources) {
    bakedObjectIds.insert(source.objectId);
  }
  for (const CreativeRoomBakeStaticMeshSource& source :
       baked.staticMeshSources) {
    bakedObjectIds.insert(source.objectId);
  }
  for (const CreativeObject* object : runtimeObjects) {
    if (bakedObject(object->id, bakedObjectIds)) {
      continue;
    }
    if (object->kind == CreativeObjectKind::PatrolRoute &&
        patrolRouteHasRuntimeOwner(document, object->id,
                                   request.includeHidden)) {
      continue;
    }
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::RuntimeObjectSkipped,
                    object->id,
                    object->name,
                    std::string(toString(describeObject(object->kind)
                                             .shapeKind)));
  }

  if (baked.receipt.skippedNoBoundsCount > 0U) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::InvalidRuntimeBounds,
                    kInvalidObjectId,
                    {},
                    "creative_map_runtime_bounds_invalid",
                    baked.receipt.skippedNoBoundsCount);
  }
  if (baked.receipt.skippedUnsupportedAssetCollisionCount >
      unsupportedAssetCount) {
    diagnostics.add(
        CreativeMapDiagnosticSeverity::Error,
        CreativeMapDiagnosticCode::UnsupportedStaticMeshCollision,
        kInvalidObjectId,
        {},
        "creative_map_asset_collision_unsupported",
        baked.receipt.skippedUnsupportedAssetCollisionCount -
            unsupportedAssetCount);
  }
  if (baked.receipt.skippedInvalidAssetMetadataCount > invalidAssetCount) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::InvalidStaticMeshMetadata,
                    kInvalidObjectId,
                    {},
                    "creative_map_asset_metadata_invalid",
                    baked.receipt.skippedInvalidAssetMetadataCount -
                        invalidAssetCount);
  }
  if (baked.receipt.skippedAssetWalkableTransformCount > 0U) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Warning,
                    CreativeMapDiagnosticCode::AssetWalkabilitySkipped,
                    kInvalidObjectId,
                    {},
                    "creative_map_asset_walkability_skipped",
                    baked.receipt.skippedAssetWalkableTransformCount);
  }

  const bool hasWalkableSurface = std::any_of(
      baked.room.spatialSurfaces.begin(),
      baked.room.spatialSurfaces.end(),
      [](const RoomSpatialSurface& surface) {
        return surface.role == RoomSpatialSurfaceRole::Walkable;
      });
  if (!hasWalkableSurface) {
    diagnostics.add(CreativeMapDiagnosticSeverity::Error,
                    CreativeMapDiagnosticCode::NoWalkableSurface,
                    kInvalidObjectId,
                    {},
                    "creative_map_walkable_surface_missing");
  }
  appendReachabilityDiagnostic(baked.reachability, diagnostics);

  diagnostics.finish();
  result.passed = result.summary.errorCount == 0U;
  return evaluation;
}

CreativeMapValidationResult validateCreativeMap(
    const CreativeMapValidationRequest& request) {
  CreativeMapEvaluationResult evaluation = evaluateCreativeMap(request);
  return std::move(evaluation.validation);
}

}  // namespace iggy3d::creative
