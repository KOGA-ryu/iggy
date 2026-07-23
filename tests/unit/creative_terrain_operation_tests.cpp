#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"
#include "app/iggy3d/creative/recipes/TerrainGradeAdapters.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementSystem.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainHeightField makeField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  cr::CreativeTerrainHeightField field;
  static_cast<void>(field.replace(bounds, heights));
  return field;
}

cr::CreativeTerrainGeneratorRecipe flatRecipe(std::uint16_t height) {
  cr::CreativeTerrainGeneratorRecipe recipe;
  recipe.bounds = {{0, 0}, 2U, 2U};
  recipe.baseHeightCells = height;
  recipe.reliefCells = 0U;
  recipe.horizontalScaleCells = 4.0;
  recipe.octaveCount = 1U;
  recipe.slopeDamping = 0.0;
  return recipe;
}

cr::CreativeTerrainCompositionRecipe hardReplace() {
  cr::CreativeTerrainCompositionRecipe recipe;
  recipe.featherCells = 0U;
  return recipe;
}

cr::CreativeTerrainOperationMutationRequest addRequest(
    std::uint16_t height) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.generation = flatRecipe(height);
  request.composition = hardReplace();
  return request;
}

cr::CreativeTerrainHeightField flatField(
    cr::CreativeTerrainHeightFieldBounds bounds, std::uint16_t height) {
  std::vector<std::uint16_t> heights(
      static_cast<std::size_t>(bounds.widthCells) * bounds.depthCells, height);
  return makeField(bounds, heights);
}

cr::CreativeTerrainOperationMutationRequest gradeRequest(
    cr::CreativeTerrainGradeRecipe recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::Grade;
  request.grade = recipe;
  return request;
}

cr::CreativeTerrainOperationMutationRequest regionRequest(
    cr::CreativeTerrainRegionRecipe recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::Region;
  request.region = recipe;
  return request;
}

cr::CreativeTerrainOperationMutationRequest profileRequest(
    cr::CreativeTerrainProfileRecipe recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::Profile;
  request.profile = recipe;
  return request;
}

cr::CreativeTerrainOperationMutationRequest landformRequest(
    cr::CreativeTerrainLandformRecipe recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::Landform;
  request.landform = recipe;
  return request;
}

bool gradeKernelOwnsGeometryAndWalkability() {
  cr::CreativeTerrainField legacy;
  const cr::CreativeTerrainHeightField base =
      flatField({{-2, -2}, 9U, 5U}, 4U);
  const cr::CreativeTerrainSurfacePlan source =
      cr::buildCreativeComposedTerrainSurfacePlan(legacy, base);
  cr::CreativeTerrainGradeRecipe recipe;
  recipe.start = {0, 0};
  recipe.end = {4, 0};
  recipe.startHeightCells = 4U;
  recipe.endHeightCells = 8U;
  recipe.halfWidthCells = 1U;
  recipe.crossSlopePermille = 1000;
  recipe.falloffCells = 1U;
  const cr::CreativeTerrainGradeRecipeResult steep =
      cr::buildCreativeTerrainGradeRecipe(base, source, recipe);
  const cr::CreativeTerrainGradeRecipeResult repeated =
      cr::buildCreativeTerrainGradeRecipe(base, source, recipe);

  cr::CreativeTerrainGradeRecipe flatRecipe = recipe;
  flatRecipe.startHeightCells = 4U;
  flatRecipe.endHeightCells = 4U;
  flatRecipe.halfWidthCells = 0U;
  flatRecipe.crossSlopePermille = 0;
  flatRecipe.falloffCells = 0U;
  const cr::CreativeTerrainGradeRecipeResult flat =
      cr::buildCreativeTerrainGradeRecipe(base, source, flatRecipe);

  return expect(steep.receipt.accepted &&
                    steep.receipt.status ==
                        cr::CreativeTerrainGradeRecipeStatus::Ready &&
                    steep.heightField.heightAt({0, 0}) == 4U &&
                    steep.heightField.heightAt({2, 0}) == 6U &&
                    steep.heightField.heightAt({4, 0}) == 8U,
                "grade interpolates exact endpoint and center heights") &&
         expect(steep.heightField.heightAt({2, 1}) == 7U &&
                    steep.heightField.heightAt({2, -1}) == 5U &&
                    steep.heightField.heightAt({2, 2}) == 4U &&
                    steep.receipt.corridorCellCount > 0U &&
                    steep.receipt.falloffCellCount > 0U,
                "grade applies signed cross slope and bounded falloff") &&
         expect(steep.receipt.readout.lengthCells == 4.0 &&
                    steep.receipt.readout.longitudinalSlopePercent == 100.0 &&
                    steep.receipt.readout.crossSlopePercent == 100.0 &&
                    steep.receipt.readout.maximumCollisionSlopeDegrees > 40.0 &&
                    !steep.receipt.readout.walkable,
                "grade readout uses rendered collision slope policy") &&
         expect(repeated.receipt.accepted &&
                    repeated.receipt.heightHash == steep.receipt.heightHash &&
                    cr::creativeTerrainHeightFieldsEqual(repeated.heightField,
                                                         steep.heightField),
                "grade output and receipt hash are deterministic") &&
         expect(flat.receipt.accepted && flat.receipt.readout.walkable &&
                    flat.receipt.readout.maximumCollisionSlopeDegrees == 0.0 &&
                    flat.receipt.readout.movementBand == "flat",
                "flat grade is traversable under the runtime movement policy");
}

bool gradeAdaptersPreservePathPadAndBridgeSemantics() {
  cr::CreativeTerrainGradePathSegmentRequest pathRequest;
  pathRequest.start = {-4, 3};
  pathRequest.end = {8, 7};
  pathRequest.startHeightCells = 3U;
  pathRequest.endHeightCells = 6U;
  pathRequest.halfWidthCells = 2U;
  pathRequest.crossSlopePermille = 25;
  pathRequest.falloffCells = 4U;
  const cr::CreativeTerrainGradeAdapterPlan path =
      cr::planCreativeTerrainGradePathSegment(pathRequest);

  cr::CreativeTerrainGradeBuildingPadApproachRequest padRequest;
  padRequest.padFootprint = {{10, 10}, {20, 18}};
  padRequest.terrainEndpoint = {4, 14};
  padRequest.padEndpoint = {10, 14};
  padRequest.terrainHeightCells = 2U;
  padRequest.padHeightCells = 5U;
  padRequest.halfWidthCells = 2U;
  padRequest.falloffCells = 3U;
  const cr::CreativeTerrainGradeAdapterPlan pad =
      cr::planCreativeTerrainGradeBuildingPadApproach(padRequest);
  cr::CreativeTerrainGradeBuildingPadApproachRequest interiorPad = padRequest;
  interiorPad.padEndpoint = {15, 14};
  const cr::CreativeTerrainGradeAdapterPlan invalidPad =
      cr::planCreativeTerrainGradeBuildingPadApproach(interiorPad);

  cr::CreativeTerrainGradeBridgeApproachRequest bridgeRequest;
  bridgeRequest.bridgeFootprint = {{0, 0}, {8, 4}};
  bridgeRequest.firstTerrainHeightCells = 2U;
  bridgeRequest.deckHeightCells = 4U;
  bridgeRequest.secondTerrainHeightCells = 3U;
  bridgeRequest.approachLengthCells = 3U;
  bridgeRequest.crossSlopePermille = -20;
  bridgeRequest.falloffCells = 2U;
  const cr::CreativeTerrainGradeAdapterPlan bridge =
      cr::planCreativeTerrainGradeBridgeApproaches(bridgeRequest);

  cr::CreativeTerrainGradeBridgeApproachRequest overflow = bridgeRequest;
  overflow.bridgeFootprint = {
      {std::numeric_limits<std::int32_t>::min(), 0},
      {std::numeric_limits<std::int32_t>::min() + 8, 4}};
  const cr::CreativeTerrainGradeAdapterPlan overflowRejected =
      cr::planCreativeTerrainGradeBridgeApproaches(overflow);

  return expect(path.accepted && path.recipeCount == 1U &&
                    path.recipes[0].start == pathRequest.start &&
                    path.recipes[0].end == pathRequest.end &&
                    path.recipes[0].startHeightCells == 3U &&
                    path.recipes[0].endHeightCells == 6U &&
                    path.recipes[0].halfWidthCells == 2U &&
                    path.recipes[0].crossSlopePermille == 25 &&
                    path.recipes[0].falloffCells == 4U,
                "path segments translate exactly into the shared grade recipe") &&
         expect(pad.accepted && pad.recipeCount == 1U &&
                    pad.recipes[0].start == cr::CreativeTerrainCoord2{4, 14} &&
                    pad.recipes[0].end == cr::CreativeTerrainCoord2{10, 14} &&
                    pad.recipes[0].endHeightCells == 5U &&
                    !invalidPad.accepted &&
                    invalidPad.status ==
                        cr::CreativeTerrainGradeAdapterStatus::InvalidRequest,
                "building pad approaches require an exterior-to-perimeter join") &&
         expect(bridge.accepted && bridge.recipeCount == 2U &&
                    bridge.recipes[0].start ==
                        cr::CreativeTerrainCoord2{-3, 1} &&
                    bridge.recipes[0].end ==
                        cr::CreativeTerrainCoord2{0, 1} &&
                    bridge.recipes[1].start ==
                        cr::CreativeTerrainCoord2{7, 1} &&
                    bridge.recipes[1].end ==
                        cr::CreativeTerrainCoord2{10, 1} &&
                    bridge.recipes[0].halfWidthCells == 2U &&
                    bridge.recipes[1].halfWidthCells == 2U,
                "bridge footprint derives two aligned deck approaches") &&
         expect(!overflowRejected.accepted &&
                    overflowRejected.status ==
                        cr::CreativeTerrainGradeAdapterStatus::CoordinateOverflow,
                "bridge approach derivation rejects coordinate overflow");
}

bool generatedGradeSupportsFullPlayerTraversal() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Terrain Grade Motor Traversal");
  static_cast<void>(document.assignId(921U));
  static_cast<void>(document.setGridSettings({{}, 1.0, {40, 16, 20}}));
  const cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 30U, 11U};
  const std::vector<std::uint16_t> baseHeights(330U, 2U);
  const cr::CreativeTerrainHeightFieldReplaceReceipt base =
      document.replaceTerrainHeightField(bounds, baseHeights);

  cr::CreativeTerrainGradePathSegmentRequest pathRequest;
  pathRequest.start = {4, 5};
  pathRequest.end = {24, 5};
  pathRequest.startHeightCells = 2U;
  pathRequest.endHeightCells = 4U;
  pathRequest.halfWidthCells = 2U;
  pathRequest.falloffCells = 3U;
  const cr::CreativeTerrainGradeAdapterPlan adapter =
      cr::planCreativeTerrainGradePathSegment(pathRequest);
  cr::CreativeTerrainOperationMutationRequest operationRequest;
  operationRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  operationRequest.operationKind = cr::CreativeTerrainOperationKind::Grade;
  operationRequest.grade = adapter.recipes[0];
  const cr::CreativeTerrainOperationMutationReceipt operation =
      adapter.accepted
          ? document.applyTerrainOperationMutation(operationRequest)
          : cr::CreativeTerrainOperationMutationReceipt{};

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = operation.accepted ? &document : nullptr;
  bakeRequest.roomId = "terrain_grade_motor_traversal";
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);

  constexpr float kStartX = 4.5F;
  constexpr float kEndX = 24.5F;
  constexpr float kCenterZ = 5.5F;
  const iggy3d::CollisionQueryResult startGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          surfaces, {kStartX, 16.0F, kCenterZ}, 16.0F, 0.2F);
  const iggy3d::CollisionQueryResult endGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          surfaces, {kEndX, 16.0F, kCenterZ}, 16.0F, 0.2F);

  iggy3d::WorldState world;
  iggy3d::EntityState player;
  player.id = {1U};
  player.stableName = "terrain_grade_player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = iggy3d::identityTransform3();
  player.transform.position = {kStartX, startGround.heightMeters, kCenterZ};
  player.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                         {0.25F, 1.8F, 0.25F});
  const iggy3d::WorldEntityResult seeded = world.seedEntity(player);
  iggy3d::RuntimeConfig runtimeConfig = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::MovementSystemContext context{&world, &runtimeConfig, &surfaces, true};
  constexpr float kMoveMeters = 0.10F;
  constexpr std::size_t kMoveCount = 200U;
  bool everyMoveAccepted = seeded.status == iggy3d::WorldStatus::Ok;
  std::size_t firstBlockedMove = kMoveCount;
  iggy3d::MovementBlockedReason firstBlockedReason =
      iggy3d::MovementBlockedReason::None;
  float firstBlockedSlopeDegrees = 0.0F;
  for (std::size_t moveIndex = 0U; moveIndex < kMoveCount; ++moveIndex) {
    const iggy3d::EntityState* current = world.findById({1U});
    if (current == nullptr) {
      everyMoveAccepted = false;
      break;
    }
    iggy3d::MovementRequest request;
    request.actor = {1U};
    request.destination = {current->transform.position.x + kMoveMeters,
                           current->transform.position.y,
                           current->transform.position.z};
    request.mode = iggy3d::MovementMode::Walk;
    request.maxDistanceMeters = 1.0F;
    const iggy3d::MovementResult moved =
        iggy3d::executeMovement(context, request);
    if (moved.blocked != iggy3d::MovementBlockedReason::None &&
        firstBlockedMove == kMoveCount) {
      firstBlockedMove = moveIndex;
      firstBlockedReason = moved.blocked;
      firstBlockedSlopeDegrees = moved.slopeAngleDegrees;
    }
    everyMoveAccepted =
        everyMoveAccepted &&
        moved.blocked == iggy3d::MovementBlockedReason::None;
  }
  const iggy3d::EntityState* finalPlayer = world.findById({1U});
  const bool reachedEnd =
      finalPlayer != nullptr &&
      std::fabs(finalPlayer->transform.position.x - kEndX) <= 0.03F &&
      std::fabs(finalPlayer->transform.position.z - kCenterZ) <= 0.01F &&
      std::fabs(finalPlayer->transform.position.y - endGround.heightMeters) <=
          0.04F;
  if (!everyMoveAccepted || !reachedEnd) {
    if (firstBlockedMove != kMoveCount) {
      std::cerr << "terrain grade first blocked move: " << firstBlockedMove
                << " reason "
                << iggy3d::movementBlockedReasonName(firstBlockedReason)
                << " slope " << firstBlockedSlopeDegrees << '\n';
    }
    if (finalPlayer != nullptr) {
      std::cerr << "terrain grade final foot: "
                << finalPlayer->transform.position.x << ' '
                << finalPlayer->transform.position.y << ' '
                << finalPlayer->transform.position.z << " expected " << kEndX
                << ' ' << endGround.heightMeters << ' ' << kCenterZ << '\n';
    }
  }

  return expect(base.accepted && adapter.accepted && operation.accepted &&
                    baked.receipt.accepted &&
                    baked.receipt.usedSmoothTerrainCollision,
                "grade reaches room bake as smooth generated collision") &&
         expect(startGround.status == iggy3d::CollisionQueryStatus::Hit &&
                    endGround.status == iggy3d::CollisionQueryStatus::Hit &&
                    endGround.heightMeters > startGround.heightMeters,
                "grade endpoints expose the expected rising ground") &&
         expect(everyMoveAccepted && reachedEnd,
                "runtime player movement traverses the complete generated grade");
}

bool generatedPathSupportsReachabilityAndFullPlayerTraversal() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Terrain Path Runtime Traversal");
  static_cast<void>(document.assignId(922U));
  static_cast<void>(document.setGridSettings({{}, 1.0, {40, 16, 20}}));

  cr::CreativeDocumentCreateRequest spawnRequest;
  spawnRequest.kind = cr::CreativeObjectKind::SpawnPoint;
  spawnRequest.name = "Path Start";
  spawnRequest.transform.position = {4.5, 2.0, 5.5};
  spawnRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt spawn =
      document.createObject(spawnRequest);

  cr::CreativeTerrainPathSourceRecipe path;
  path.kind = cr::CreativeTerrainPathKind::Road;
  path.elevation = cr::CreativeTerrainPathElevation::Grade;
  path.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  path.startJoin = cr::CreativeTerrainPathEndpointJoin::Open;
  path.endJoin = cr::CreativeTerrainPathEndpointJoin::Open;
  path.falloffCells = 0U;
  path.paintSurface = true;
  path.material = cr::CreativeTerrainMaterial::Dirt;
  path.nextPointId = 3U;
  path.points = {
      {1U, {4, 5}, 2U, 2U, 0U, 0},
      {2U, {24, 5}, 4U, 2U, 0U, 0},
  };
  cr::CreativeTerrainOperationMutationRequest operationRequest;
  operationRequest.kind = cr::CreativeTerrainOperationMutationKind::Add;
  operationRequest.operationKind = cr::CreativeTerrainOperationKind::Path;
  operationRequest.path = path;
  const cr::CreativeTerrainOperationMutationReceipt operation =
      document.applyTerrainOperationMutation(operationRequest);

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = operation.accepted ? &document : nullptr;
  bakeRequest.roomId = "terrain_path_runtime_traversal";
  bakeRequest.validateReachability = true;
  bakeRequest.reachabilityCellSizeMeters = 1.0F;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);

  constexpr float kStartX = 4.5F;
  constexpr float kEndX = 24.5F;
  constexpr float kCenterZ = 5.5F;
  const iggy3d::CollisionQueryResult startGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          surfaces, {kStartX, 16.0F, kCenterZ}, 16.0F, 0.2F);
  const iggy3d::CollisionQueryResult endGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          surfaces, {kEndX, 16.0F, kCenterZ}, 16.0F, 0.2F);

  iggy3d::WorldState world;
  iggy3d::EntityState player;
  player.id = {2U};
  player.stableName = "terrain_path_player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = iggy3d::identityTransform3();
  player.transform.position = {kStartX, startGround.heightMeters, kCenterZ};
  player.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                         {0.25F, 1.8F, 0.25F});
  const iggy3d::WorldEntityResult seeded = world.seedEntity(player);
  iggy3d::RuntimeConfig runtimeConfig = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::MovementSystemContext context{&world, &runtimeConfig, &surfaces,
                                        true};
  constexpr float kMoveMeters = 0.10F;
  constexpr std::size_t kMoveCount = 200U;
  bool everyMoveAccepted = seeded.status == iggy3d::WorldStatus::Ok;
  for (std::size_t moveIndex = 0U; moveIndex < kMoveCount; ++moveIndex) {
    const iggy3d::EntityState* current = world.findById({2U});
    if (current == nullptr) {
      everyMoveAccepted = false;
      break;
    }
    iggy3d::MovementRequest request;
    request.actor = {2U};
    request.destination = {current->transform.position.x + kMoveMeters,
                           current->transform.position.y,
                           current->transform.position.z};
    request.mode = iggy3d::MovementMode::Walk;
    request.maxDistanceMeters = 1.0F;
    const iggy3d::MovementResult moved =
        iggy3d::executeMovement(context, request);
    everyMoveAccepted =
        everyMoveAccepted &&
        moved.blocked == iggy3d::MovementBlockedReason::None;
  }
  const iggy3d::EntityState* finalPlayer = world.findById({2U});
  const bool reachedEnd =
      finalPlayer != nullptr &&
      std::fabs(finalPlayer->transform.position.x - kEndX) <= 0.03F &&
      std::fabs(finalPlayer->transform.position.z - kCenterZ) <= 0.01F &&
      std::fabs(finalPlayer->transform.position.y - endGround.heightMeters) <=
          0.04F;

  return expect(spawn.accepted && operation.accepted &&
                    baked.receipt.accepted &&
                    baked.receipt.usedSmoothTerrainCollision,
                "path reaches room bake as smooth generated collision") &&
         expect(baked.reachability.checked && !baked.reachability.hasIslands &&
                    baked.reachability.status ==
                        cr::CreativeRoomBakeReachabilityStatus::Reachable &&
                    baked.reachability.usableSeedCount == 1U &&
                    baked.reachability.reachedCellCount ==
                        baked.reachability.walkableCellCount,
                "path corridor is connected in the runtime reachability bake") &&
         expect(startGround.status == iggy3d::CollisionQueryStatus::Hit &&
                    endGround.status == iggy3d::CollisionQueryStatus::Hit &&
                    endGround.heightMeters > startGround.heightMeters,
                "path endpoints expose the authored rising terrain") &&
         expect(everyMoveAccepted && reachedEnd,
                "runtime player movement traverses the complete generated path");
}

bool gradeOperationsRemainEditableAndFailAtomically() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainMaterialField material;
  const cr::CreativeTerrainHeightField base =
      flatField({{0, 0}, 6U, 3U}, 4U);
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainGradeRecipe recipe;
  recipe.start = {0, 1};
  recipe.end = {5, 1};
  recipe.startHeightCells = 4U;
  recipe.endHeightCells = 6U;
  recipe.halfWidthCells = 0U;
  recipe.falloffCells = 0U;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(legacy, base, material, stack,
                                               gradeRequest(recipe));

  cr::CreativeTerrainOperationMutationRequest update = gradeRequest(recipe);
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.receipt.operationId;
  update.grade.endHeightCells = 7U;
  update.grade.halfWidthCells = 1U;
  update.grade.falloffCells = 1U;
  const cr::CreativeTerrainOperationMutationPlan updated =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.materialField, added.stack, update);

  cr::CreativeTerrainOperationMutationRequest disable;
  disable.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disable.operationId = added.receipt.operationId;
  disable.enabled = false;
  const cr::CreativeTerrainOperationMutationPlan disabled =
      cr::planCreativeTerrainOperationMutation(
          legacy, updated.heightField, updated.materialField, updated.stack,
          disable);

  cr::CreativeTerrainOperationMutationRequest remove;
  remove.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  remove.operationId = added.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan removed =
      cr::planCreativeTerrainOperationMutation(
          legacy, disabled.heightField, disabled.materialField, disabled.stack,
          remove);

  cr::CreativeTerrainGradeRecipe invalid = recipe;
  invalid.end = invalid.start;
  const cr::CreativeTerrainOperationMutationPlan invalidRejected =
      cr::planCreativeTerrainOperationMutation(legacy, base, material, stack,
                                               gradeRequest(invalid));
  cr::CreativeTerrainGradeRecipe outOfRange = recipe;
  outOfRange.startHeightCells = 1U;
  outOfRange.endHeightCells = 1U;
  outOfRange.halfWidthCells = 2U;
  outOfRange.crossSlopePermille = 1000;
  const cr::CreativeTerrainOperationMutationPlan rangeRejected =
      cr::planCreativeTerrainOperationMutation(legacy, base, material, stack,
                                               gradeRequest(outOfRange));
  cr::CreativeTerrainGradeRecipe tooLarge = recipe;
  tooLarge.end = {8192, 1};
  const cr::CreativeTerrainOperationMutationPlan capacityRejected =
      cr::planCreativeTerrainOperationMutation(legacy, base, material, stack,
                                               gradeRequest(tooLarge));

  return expect(added.receipt.accepted && added.receipt.changed &&
                    added.stack.operations.size() == 1U &&
                    added.stack.operations.front().kind ==
                        cr::CreativeTerrainOperationKind::Grade &&
                    added.receipt.replay.gradeOperationCount == 1U,
                "grade is a typed durable terrain operation") &&
         expect(updated.receipt.accepted && updated.receipt.changed &&
                    updated.stack.operations.front().grade == update.grade &&
                    updated.heightField.heightAt({5, 1}) == 7U,
                "grade update replays its editable recipe") &&
         expect(disabled.receipt.accepted &&
                    cr::creativeTerrainHeightFieldsEqual(disabled.heightField,
                                                         base) &&
                    disabled.receipt.replay.disabledOperationCount == 1U,
                "disabled grade preserves provenance and exposes its base") &&
         expect(removed.receipt.accepted && removed.stack.operations.empty() &&
                    removed.stack.baseHeightField.cellCount() == 0U &&
                    cr::creativeTerrainHeightFieldsEqual(removed.heightField,
                                                         base),
                "removing the last grade releases captured provenance") &&
         expect(!invalidRejected.receipt.accepted &&
                    invalidRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::InvalidRequest &&
                    invalidRejected.stack.operations.empty(),
                "degenerate grade request rejects atomically") &&
         expect(!rangeRejected.receipt.accepted &&
                    rangeRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::ReplayRejected &&
                    rangeRejected.stack.operations.empty(),
                "grade height overflow rejects without partial state") &&
         expect(!capacityRejected.receipt.accepted &&
                    capacityRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::ReplayRejected &&
                    capacityRejected.stack.operations.empty(),
                "oversized grade rejects without allocating partial state");
}

bool regionOperationsRemainEditableAndOrdered() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainMaterialField material;
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 7U, 7U};
  const cr::CreativeTerrainHeightField base = flatField(bounds, 4U);
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.bounds = {{1, 1}, 5U, 5U};
  recipe.mode = cr::CreativeTerrainRegionMode::Raise;
  recipe.amountCells = 2U;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(legacy, base, material, stack,
                                               regionRequest(recipe));

  cr::CreativeTerrainOperationMutationRequest update = regionRequest(recipe);
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.receipt.operationId;
  update.region.mode = cr::CreativeTerrainRegionMode::Flatten;
  update.region.targetHeightCells = 9U;
  update.region.mask = cr::CreativeTerrainCompositionMask::Ellipse;
  const cr::CreativeTerrainOperationMutationPlan updated =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.materialField, added.stack, update);

  cr::CreativeTerrainOperationMutationRequest disable;
  disable.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disable.operationId = added.receipt.operationId;
  disable.enabled = false;
  const cr::CreativeTerrainOperationMutationPlan disabled =
      cr::planCreativeTerrainOperationMutation(
          legacy, updated.heightField, updated.materialField, updated.stack,
          disable);
  cr::CreativeTerrainOperationKind parsed =
      cr::CreativeTerrainOperationKind::Count;

  return expect(added.receipt.accepted && added.receipt.changed &&
                    added.stack.operations.size() == 1U &&
                    added.stack.operations.front().region == recipe &&
                    added.heightField.heightAt({3, 3}) == 6U &&
                    added.receipt.replay.regionOperationCount == 1U,
                "region is one typed durable operation") &&
         expect(updated.receipt.accepted && updated.receipt.changed &&
                    updated.stack.operations.front().region == update.region &&
                    updated.heightField.heightAt({3, 3}) == 9U &&
                    updated.heightField.heightAt({1, 1}) == 4U,
                "region update replays ellipse geometry from its recipe") &&
         expect(disabled.receipt.accepted &&
                    cr::creativeTerrainHeightFieldsEqual(disabled.heightField,
                                                         base) &&
                    disabled.receipt.replay.disabledOperationCount == 1U,
                "disabled region exposes the captured base") &&
         expect(cr::toString(cr::CreativeTerrainOperationKind::Region) ==
                        "Region" &&
                    cr::parseCreativeTerrainOperationKind("Region", parsed) &&
                    parsed == cr::CreativeTerrainOperationKind::Region,
                "region operation kind has a strict persistence label");
}

bool profileOperationsRemainEditableSeededAndOrdered() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainHeightField empty;
  cr::CreativeTerrainMaterialField material;
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainProfileRecipe recipe;
  recipe.profile = cr::CreativeTerrainProfileKind::Wave;
  recipe.center = {4, 4};
  recipe.baseHeightCells = 12U;
  recipe.radiusCells = 8U;
  recipe.amplitudeCells = 4U;
  recipe.spacingCells = 1U;
  recipe.frequency = 1U;
  recipe.seed = 7U;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(
          legacy, empty, material, stack, profileRequest(recipe));

  cr::CreativeTerrainOperationMutationRequest update = profileRequest(recipe);
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.receipt.operationId;
  update.profile.center = {6, 4};
  update.profile.seed = 19U;
  update.profile.profile = cr::CreativeTerrainProfileKind::Ripple;
  const cr::CreativeTerrainOperationMutationPlan updated =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.materialField, added.stack, update);

  cr::CreativeTerrainOperationMutationRequest disable;
  disable.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disable.operationId = added.receipt.operationId;
  disable.enabled = false;
  const cr::CreativeTerrainOperationMutationPlan disabled =
      cr::planCreativeTerrainOperationMutation(
          legacy, updated.heightField, updated.materialField, updated.stack,
          disable);

  cr::CreativeTerrainOperationMutationRequest remove;
  remove.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  remove.operationId = added.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan removed =
      cr::planCreativeTerrainOperationMutation(
          legacy, disabled.heightField, disabled.materialField, disabled.stack,
          remove);

  cr::CreativeTerrainOperationKind parsed =
      cr::CreativeTerrainOperationKind::Count;
  return expect(added.receipt.accepted && added.receipt.changed &&
                    added.stack.operations.size() == 1U &&
                    added.stack.operations.front().kind ==
                        cr::CreativeTerrainOperationKind::Profile &&
                    added.stack.operations.front().profile == recipe &&
                    added.receipt.replay.profileOperationCount == 1U,
                "profile is one typed seeded durable operation") &&
         expect(updated.receipt.accepted && updated.receipt.changed &&
                    updated.receipt.operationId == added.receipt.operationId &&
                    updated.stack.operations.front().profile == update.profile &&
                    updated.receipt.replay.profileOperationCount == 1U &&
                    updated.receipt.replay.heightHash !=
                        added.receipt.replay.heightHash,
                "profile update replays moved source parameters in place") &&
         expect(disabled.receipt.accepted &&
                    disabled.heightField.cellCount() == 0U &&
                    disabled.receipt.replay.disabledOperationCount == 1U,
                "disabled profile preserves source intent and exposes base") &&
         expect(removed.receipt.accepted && removed.stack.operations.empty() &&
                    removed.heightField.cellCount() == 0U,
                "removing the final profile releases captured provenance") &&
         expect(cr::toString(cr::CreativeTerrainOperationKind::Profile) ==
                        "Profile" &&
                    cr::parseCreativeTerrainOperationKind("Profile", parsed) &&
                    parsed == cr::CreativeTerrainOperationKind::Profile,
                "profile operation kind has a strict persistence label");
}

bool landformOperationsOwnHeightMaterialAndEditableProvenance() {
  cr::CreativeTerrainField legacy;
  const cr::CreativeTerrainHeightField base =
      flatField({{0, 0}, 8U, 8U}, 2U);
  cr::CreativeTerrainMaterialField material;
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainLandformRecipe recipe;
  recipe.kind = cr::CreativeTerrainLandformKind::Terrace;
  recipe.bounds = {{1, 1}, 6U, 6U};
  recipe.baseHeightCells = 2U;
  recipe.targetHeightCells = 8U;
  recipe.terraceCount = 4U;
  recipe.edge = cr::CreativeTerrainLandformEdge::Retaining;
  recipe.edgeWidthCells = 0U;
  recipe.material = cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(
          legacy, base, material, stack, landformRequest(recipe));

  cr::CreativeTerrainOperationMutationRequest update =
      landformRequest(recipe);
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.receipt.operationId;
  update.landform.kind = cr::CreativeTerrainLandformKind::Cliff;
  update.landform.direction = cr::CreativeTerrainLandformDirection::NegativeZ;
  update.landform.material = cr::CreativeTerrainMaterial::Dirt;
  const cr::CreativeTerrainOperationMutationPlan updated =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.materialField, added.stack, update);

  cr::CreativeTerrainOperationMutationRequest disable;
  disable.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disable.operationId = added.receipt.operationId;
  disable.enabled = false;
  const cr::CreativeTerrainOperationMutationPlan disabled =
      cr::planCreativeTerrainOperationMutation(
          legacy, updated.heightField, updated.materialField, updated.stack,
          disable);
  const cr::CreativeTerrainOperationReplayResult repeated =
      cr::replayCreativeTerrainOperations(legacy, added.stack);
  cr::CreativeTerrainOperationKind parsed =
      cr::CreativeTerrainOperationKind::Count;
  return expect(added.receipt.accepted && added.receipt.changed &&
                    added.stack.operations.size() == 1U &&
                    added.stack.operations.front().kind ==
                        cr::CreativeTerrainOperationKind::Landform &&
                    added.stack.operations.front().landform == recipe &&
                    added.receipt.replay.landformOperationCount == 1U &&
                    added.receipt.replay.outputHardEdgeCount ==
                        added.hardEdges.size() &&
                    added.receipt.replay.hardEdgeHash ==
                        cr::hashCreativeTerrainHardEdges(added.hardEdges) &&
                    !added.hardEdges.empty() &&
                    cr::validateCreativeTerrainHardEdges(added.hardEdges) &&
                    added.materialField.materialAt({3, 3}) ==
                        cr::CreativeTerrainMaterial::Stone,
                "landform is one durable height and material operation") &&
         expect(updated.receipt.accepted && updated.receipt.changed &&
                    updated.receipt.operationId == added.receipt.operationId &&
                    updated.stack.operations.front().landform ==
                        update.landform &&
                    !updated.hardEdges.empty() &&
                    cr::validateCreativeTerrainHardEdges(updated.hardEdges) &&
                    updated.materialField.materialAt({3, 3}) ==
                        cr::CreativeTerrainMaterial::Dirt,
                "landform parameters update in place with stable identity") &&
         expect(disabled.receipt.accepted &&
                    cr::creativeTerrainHeightFieldsEqual(disabled.heightField,
                                                         base) &&
                    disabled.materialField.overrideCount() == 0U &&
                    disabled.hardEdges.empty(),
                "disabling landform restores the exact captured base") &&
         expect(repeated.receipt.accepted &&
                    cr::creativeTerrainHardEdgesEqual(repeated.hardEdges,
                                                      added.hardEdges) &&
                    repeated.receipt.hardEdgeHash ==
                        added.receipt.replay.hardEdgeHash,
                "landform replay reconstructs deterministic hard topology") &&
         expect(cr::toString(cr::CreativeTerrainOperationKind::Landform) ==
                        "Landform" &&
                    cr::parseCreativeTerrainOperationKind("Landform", parsed) &&
                    parsed == cr::CreativeTerrainOperationKind::Landform,
                "landform operation kind has a strict persistence label");
}

bool laterHeightOperationsInvalidateAndRemovalRestoresHardEdges() {
  cr::CreativeTerrainField legacy;
  const cr::CreativeTerrainHeightField base =
      flatField({{0, 0}, 8U, 8U}, 2U);
  cr::CreativeTerrainMaterialField material;
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainLandformRecipe terrace;
  terrace.kind = cr::CreativeTerrainLandformKind::Terrace;
  terrace.bounds = {{0, 0}, 8U, 8U};
  terrace.baseHeightCells = 2U;
  terrace.targetHeightCells = 8U;
  terrace.terraceCount = 4U;
  terrace.edge = cr::CreativeTerrainLandformEdge::Retaining;
  terrace.edgeWidthCells = 0U;
  terrace.paintSurface = false;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(
          legacy, base, material, stack, landformRequest(terrace));

  cr::CreativeTerrainOperationMutationRequest overlay = addRequest(3U);
  overlay.generation.bounds = {{0, 0}, 8U, 8U};
  const cr::CreativeTerrainOperationMutationPlan covered =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.materialField, added.stack, overlay,
          nullptr, &added.hardEdges);
  cr::CreativeTerrainOperationMutationRequest remove;
  remove.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  remove.operationId = covered.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan restored =
      cr::planCreativeTerrainOperationMutation(
          legacy, covered.heightField, covered.materialField, covered.stack,
          remove, nullptr, &covered.hardEdges);

  return expect(added.receipt.accepted && !added.hardEdges.empty(),
                "retaining operation creates a topology fixture") &&
         expect(covered.receipt.accepted && covered.hardEdges.empty(),
                "later height replacement removes every changed hard edge") &&
         expect(restored.receipt.accepted &&
                    cr::creativeTerrainHardEdgesEqual(restored.hardEdges,
                                                      added.hardEdges),
                "removing later replacement reconstructs prior topology");
}

bool orderedReplayAndMutationLifecycle() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainMaterialField material;
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 2U, 2U};
  constexpr std::array<std::uint16_t, 4U> baseHeights{3U, 3U, 3U, 3U};
  const cr::CreativeTerrainHeightField base = makeField(bounds, baseHeights);
  cr::CreativeTerrainOperationStack stack;

  const cr::CreativeTerrainOperationMutationPlan first =
      cr::planCreativeTerrainOperationMutation(legacy, base, material, stack,
                                               addRequest(5U));
  const cr::CreativeTerrainOperationMutationPlan second =
      cr::planCreativeTerrainOperationMutation(legacy, first.heightField,
                                               first.materialField, first.stack,
                                               addRequest(10U));
  cr::CreativeTerrainOperationMutationRequest move;
  move.kind = cr::CreativeTerrainOperationMutationKind::Move;
  move.operationId = second.receipt.operationId;
  move.targetIndex = 0U;
  const cr::CreativeTerrainOperationMutationPlan moved =
      cr::planCreativeTerrainOperationMutation(
          legacy, second.heightField, second.materialField, second.stack,
          move);
  cr::CreativeTerrainOperationMutationRequest disable;
  disable.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disable.operationId = first.receipt.operationId;
  disable.enabled = false;
  const cr::CreativeTerrainOperationMutationPlan disabled =
      cr::planCreativeTerrainOperationMutation(
          legacy, moved.heightField, moved.materialField, moved.stack,
          disable);

  cr::CreativeTerrainOperationMutationRequest removeSecond;
  removeSecond.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  removeSecond.operationId = second.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan removedSecond =
      cr::planCreativeTerrainOperationMutation(
          legacy, disabled.heightField, disabled.materialField,
          disabled.stack, removeSecond);
  cr::CreativeTerrainOperationMutationRequest removeFirst;
  removeFirst.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  removeFirst.operationId = first.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan removedAll =
      cr::planCreativeTerrainOperationMutation(
          legacy, removedSecond.heightField, removedSecond.materialField,
          removedSecond.stack, removeFirst);

  return expect(first.receipt.accepted && first.receipt.changed &&
                    first.receipt.operationId == 1U &&
                    first.stack.baseHeightField.heightAt({0, 0}) == 3U &&
                    first.heightField.heightAt({0, 0}) == 5U,
                "first operation captures the baked base") &&
         expect(second.receipt.accepted &&
                    second.receipt.operationId == 2U &&
                    second.heightField.heightAt({0, 0}) == 10U,
                "later replace operation wins in ordered replay") &&
         expect(moved.receipt.accepted && moved.receipt.changed &&
                    moved.stack.operations.front().id == 2U &&
                    moved.heightField.heightAt({0, 0}) == 5U,
                "reordering deterministically changes replay output") &&
         expect(disabled.receipt.accepted &&
                    disabled.heightField.heightAt({0, 0}) == 10U &&
                    disabled.receipt.replay.disabledOperationCount == 1U,
                "disabled operations remain durable but do not evaluate") &&
         expect(removedSecond.receipt.accepted &&
                    removedSecond.heightField.heightAt({0, 0}) == 3U,
                "removing the only enabled operation exposes baked base") &&
         expect(removedAll.receipt.accepted &&
                    removedAll.stack.operations.empty() &&
                    removedAll.stack.baseHeightField.cellCount() == 0U &&
                    cr::creativeTerrainHeightFieldsEqual(removedAll.heightField,
                                                         base),
                "removing the final operation restores and releases base");
}

bool updateAndReplayAreDeterministic() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainHeightField empty;
  cr::CreativeTerrainMaterialField material;
  cr::CreativeTerrainOperationStack stack;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(legacy, empty, material, stack,
                                               addRequest(6U));
  cr::CreativeTerrainOperationMutationRequest update;
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.receipt.operationId;
  update.generation = flatRecipe(12U);
  update.composition = hardReplace();
  const cr::CreativeTerrainOperationMutationPlan updated =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.materialField, added.stack, update);
  const cr::CreativeTerrainOperationReplayResult replayA =
      cr::replayCreativeTerrainOperations(legacy, updated.stack);
  const cr::CreativeTerrainOperationReplayResult replayB =
      cr::replayCreativeTerrainOperations(legacy, updated.stack);

  return expect(updated.receipt.accepted && updated.receipt.changed &&
                    updated.heightField.heightAt({1, 1}) == 12U,
                "updating a recipe regenerates the derived field") &&
         expect(replayA.receipt.accepted && replayB.receipt.accepted &&
                    replayA.receipt.heightHash == replayB.receipt.heightHash &&
                    cr::creativeTerrainHeightFieldsEqual(replayA.heightField,
                                                         replayB.heightField),
                "ordered replay and output hash are deterministic");
}

bool driftInvalidRequestsAndCapacityFailClosed() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainHeightField empty;
  cr::CreativeTerrainMaterialField material;
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainOperationMutationPlan current =
      cr::planCreativeTerrainOperationMutation(legacy, empty, material, stack,
                                               addRequest(4U));
  constexpr std::array<std::uint16_t, 4U> driftHeights{7U, 7U, 7U, 7U};
  const cr::CreativeTerrainHeightField drifted =
      makeField({{0, 0}, 2U, 2U}, driftHeights);
  const cr::CreativeTerrainOperationMutationPlan driftRejected =
      cr::planCreativeTerrainOperationMutation(
          legacy, drifted, current.materialField, current.stack,
          addRequest(8U));

  cr::CreativeTerrainOperationMutationRequest invalidMove;
  invalidMove.kind = cr::CreativeTerrainOperationMutationKind::Move;
  invalidMove.operationId = current.receipt.operationId;
  invalidMove.targetIndex = 3U;
  const cr::CreativeTerrainOperationMutationPlan moveRejected =
      cr::planCreativeTerrainOperationMutation(
          legacy, current.heightField, current.materialField, current.stack,
          invalidMove);

  while (current.stack.operations.size() <
         cr::kCreativeTerrainOperationCapacity) {
    current = cr::planCreativeTerrainOperationMutation(
        legacy, current.heightField, current.materialField, current.stack,
        addRequest(4U));
  }
  const cr::CreativeTerrainOperationMutationPlan capacityRejected =
      cr::planCreativeTerrainOperationMutation(
          legacy, current.heightField, current.materialField, current.stack,
          addRequest(4U));

  return expect(!driftRejected.receipt.accepted &&
                    driftRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::InvalidStack &&
                    driftRejected.stack.operations.empty(),
                "derived-field drift rejects without a partial plan") &&
         expect(!moveRejected.receipt.accepted &&
                    moveRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::InvalidRequest,
                "out-of-range reorder fails closed") &&
         expect(!capacityRejected.receipt.accepted &&
                    capacityRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::CapacityExceeded &&
                    capacityRejected.stack.operations.empty(),
                "operation capacity rejects atomically");
}

bool pathOperationsOwnHeightAndMaterialProvenance() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Path Ops");
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 8U, 5U};
  const std::vector<std::uint16_t> baseHeights(40U, 4U);
  const cr::CreativeTerrainHeightFieldReplaceReceipt base =
      document.replaceTerrainHeightField(bounds, baseHeights);
  const std::array basePaint{
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {0, 0},
          cr::CreativeTerrainMaterial::Sand},
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {2, 2},
          cr::CreativeTerrainMaterial::Stone},
  };
  const cr::CreativeTerrainMaterialMutationReceipt paintedBase =
      document.applyTerrainMaterialEdits(basePaint);

  cr::CreativeTerrainOperationMutationRequest add;
  add.kind = cr::CreativeTerrainOperationMutationKind::Add;
  add.operationKind = cr::CreativeTerrainOperationKind::Path;
  add.path.kind = cr::CreativeTerrainPathKind::Road;
  add.path.elevation = cr::CreativeTerrainPathElevation::Grade;
  add.path.falloffCells = 0U;
  add.path.material = cr::CreativeTerrainMaterial::Dirt;
  add.path.nextPointId = 3U;
  add.path.points = {
      {1U, {1, 2}, 6U, 0U, 0U, 0},
      {2U, {6, 2}, 6U, 0U, 0U, 0},
  };
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(add);
  const bool addedOwnsBothDomains =
      document.terrainHeightField().heightAt({2, 2}) == 6U &&
      document.terrainMaterialField().materialAt({2, 2}) ==
          cr::CreativeTerrainMaterial::Dirt &&
      document.terrainOperationStack().baseMaterialField.materialAt({2, 2}) ==
          cr::CreativeTerrainMaterial::Stone;

  const std::array underlayEdits{
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {0, 1},
          cr::CreativeTerrainMaterial::Stone},
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set, {3, 2},
          cr::CreativeTerrainMaterial::Sand},
  };
  const cr::CreativeTerrainMaterialMutationReceipt editedUnderlay =
      document.applyTerrainMaterialEdits(underlayEdits);
  const bool pathStillOverridesUnderlay =
      document.terrainMaterialField().materialAt({0, 1}) ==
          cr::CreativeTerrainMaterial::Stone &&
      document.terrainMaterialField().materialAt({3, 2}) ==
          cr::CreativeTerrainMaterial::Dirt &&
      document.terrainOperationStack().baseMaterialField.materialAt({3, 2}) ==
          cr::CreativeTerrainMaterial::Sand;

  cr::CreativeTerrainOperationMutationRequest update = add;
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.operationId;
  update.path.material = cr::CreativeTerrainMaterial::Stone;
  update.path.points[0].coord.z = 3;
  update.path.points[1].coord.z = 3;
  update.path.points[0].heightCells = 7U;
  update.path.points[1].heightCells = 7U;
  const cr::CreativeTerrainOperationMutationReceipt updated =
      document.applyTerrainOperationMutation(update);
  const bool updateRestoresOldFootprint =
      document.terrainHeightField().heightAt({3, 2}) == 4U &&
      document.terrainMaterialField().materialAt({3, 2}) ==
          cr::CreativeTerrainMaterial::Sand &&
      document.terrainHeightField().heightAt({3, 3}) == 7U &&
      document.terrainMaterialField().materialAt({3, 3}) ==
          cr::CreativeTerrainMaterial::Stone;

  cr::CreativeTerrainOperationMutationRequest remove;
  remove.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  remove.operationId = added.operationId;
  const cr::CreativeTerrainOperationMutationReceipt removed =
      document.applyTerrainOperationMutation(remove);

  return expect(base.accepted && paintedBase.accepted && added.accepted &&
                    addedOwnsBothDomains,
                "path captures and derives height plus material") &&
         expect(editedUnderlay.accepted && editedUnderlay.changed &&
                    pathStillOverridesUnderlay,
                "base paint replays below an active path") &&
         expect(updated.accepted && updated.changed &&
                    updateRestoresOldFootprint,
                "path update restores old cells and derives new cells") &&
         expect(removed.accepted && removed.changed &&
                    document.terrainOperationStack().operations.empty() &&
                    document.terrainOperationStack()
                            .baseMaterialField.overrideCount() == 0U &&
                    document.terrainHeightField().heightAt({3, 3}) == 4U &&
                    document.terrainMaterialField().materialAt({3, 2}) ==
                        cr::CreativeTerrainMaterial::Sand &&
                    document.terrainMaterialField().materialAt({0, 1}) ==
                        cr::CreativeTerrainMaterial::Stone,
                "removing the final path restores and releases provenance");
}

bool documentOwnsReplayAndDirectReplacementCollapsesProvenance() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Operations");
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 2U, 2U};
  cr::CreativeTerrainOperationMutationRequest add = addRequest(4U);
  add.composition.mode = cr::CreativeTerrainCompositionMode::Raise;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeTerrainOperationMutationReceipt operation =
      document.applyTerrainOperationMutation(add);
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 10U, 1U}};
  const cr::CreativeTerrainMutationReceipt terrain =
      document.applyTerrainControlEdits(std::span{&control, 1U});
  const bool controlReplayedThroughStack =
      document.terrainOperationStack().operations.size() == 1U &&
      document.terrainHeightField().heightAt({0, 0}) == 10U;
  constexpr std::array<std::uint16_t, 4U> replacementHeights{7U, 7U, 7U,
                                                             7U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt replacement =
      document.replaceTerrainHeightField(bounds, replacementHeights);

  return expect(operation.accepted && operation.changed &&
                    document.revision() > revisionBefore,
                "document commits stack and derived terrain atomically") &&
         expect(terrain.accepted && terrain.changed &&
                    terrain.controlCountAfter == 1U &&
                    controlReplayedThroughStack,
                "legacy control edit replays the active operation stack") &&
         expect(replacement.accepted && replacement.changed &&
                    document.terrainOperationStack().operations.empty() &&
                    document.terrainOperationStack()
                            .baseHeightField.cellCount() == 0U &&
                    document.terrainHeightField().heightAt({0, 0}) == 7U,
                "direct replacement becomes a baked field and clears provenance");
}

bool operationProvenanceIsExplicitAndValidated() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Provenance");
  cr::CreativeTerrainOperationMutationRequest generated = addRequest(4U);
  generated.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  generated.sourceKey = "estate/terrain_path/road.entry";
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(generated);
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       added.operationId);

  cr::CreativeTerrainOperationMutationRequest invalidManual = addRequest(5U);
  invalidManual.sourceKey = "orphaned_source";
  const cr::CreativeTerrainOperationMutationReceipt rejectedManual =
      document.applyTerrainOperationMutation(invalidManual);
  cr::CreativeTerrainOperationMutationRequest invalidGenerated =
      addRequest(5U);
  invalidGenerated.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  const cr::CreativeTerrainOperationMutationReceipt rejectedGenerated =
      document.applyTerrainOperationMutation(invalidGenerated);

  cr::CreativeTerrainOperationOwner parsed =
      cr::CreativeTerrainOperationOwner::Count;
  return expect(added.accepted && added.changed && operation != nullptr &&
                    operation->owner ==
                        cr::CreativeTerrainOperationOwner::WorldLayout &&
                    operation->sourceKey == generated.sourceKey,
                "generated operation retains exact source provenance") &&
         expect(!rejectedManual.accepted && !rejectedGenerated.accepted,
                "owner and source key must form a valid provenance pair") &&
         expect(cr::toString(cr::CreativeTerrainOperationOwner::Manual) ==
                        "Manual" &&
                    cr::parseCreativeTerrainOperationOwner("WorldLayout",
                                                           parsed) &&
                    parsed ==
                        cr::CreativeTerrainOperationOwner::WorldLayout &&
                    !cr::parseCreativeTerrainOperationOwner("Generated",
                                                            parsed),
                "operation owner text contract is total and strict");
}

bool bakeAllPreservesDerivedTerrainAndClearsProceduralProvenance() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Bake Stack");
  cr::CreativeTerrainOperationMutationRequest generated = addRequest(9U);
  generated.generation.materialTransitionHeightCells = 1U;
  generated.generation.highlandMaterial = cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(generated);
  cr::CreativeTerrainLandformRecipe plateau;
  plateau.bounds = {{0, 0}, 2U, 2U};
  plateau.baseHeightCells = 9U;
  plateau.targetHeightCells = 12U;
  plateau.edge = cr::CreativeTerrainLandformEdge::Retaining;
  plateau.edgeWidthCells = 0U;
  plateau.material = cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeTerrainOperationMutationReceipt landformAdded =
      document.applyTerrainOperationMutation(landformRequest(plateau));
  const cr::CreativeTerrainHeightField beforeHeight =
      document.terrainHeightField();
  const cr::CreativeTerrainMaterialField beforeMaterial =
      document.terrainMaterialField();
  const std::span<const cr::CreativeTerrainHardEdge> beforeHardEdgeSpan =
      document.terrainHardEdges();
  const std::vector<cr::CreativeTerrainHardEdge> beforeHardEdges(
      beforeHardEdgeSpan.begin(), beforeHardEdgeSpan.end());

  cr::CreativeTerrainOperationMutationRequest bake;
  bake.kind = cr::CreativeTerrainOperationMutationKind::BakeAll;
  const cr::CreativeTerrainOperationMutationReceipt baked =
      document.applyTerrainOperationMutation(bake);
  const bool exact = cr::creativeTerrainHeightFieldsEqual(
                         beforeHeight, document.terrainHeightField()) &&
                     cr::creativeTerrainMaterialFieldsEqual(
                         beforeMaterial, document.terrainMaterialField()) &&
                     cr::creativeTerrainHardEdgesEqual(
                         beforeHardEdges, document.terrainHardEdges());
  const cr::CreativeTerrainOperationMutationReceipt repeated =
      document.applyTerrainOperationMutation(bake);

  cr::CreativeTerrainOperationMutationRequest overlay = addRequest(12U);
  const cr::CreativeTerrainOperationMutationReceipt overlayAdded =
      document.applyTerrainOperationMutation(overlay);
  cr::CreativeTerrainOperationMutationRequest remove;
  remove.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  remove.operationId = overlayAdded.operationId;
  const cr::CreativeTerrainOperationMutationReceipt overlayRemoved =
      document.applyTerrainOperationMutation(remove);

  return expect(added.accepted && added.changed && landformAdded.accepted &&
                    beforeMaterial.overrideCount() == 4U &&
                    !beforeHardEdges.empty(),
                "bake fixture has derived height and material provenance") &&
         expect(baked.accepted && baked.changed &&
                    baked.operationCountBefore == 2U &&
                    baked.operationCountAfter == 0U && exact,
                "BakeAll preserves exact derived output and clears the stack") &&
         expect(document.terrainOperationStack().operations.empty() &&
                    document.terrainOperationStack()
                            .baseHeightField.cellCount() == 0U &&
                    document.terrainOperationStack()
                            .baseMaterialField.overrideCount() == 0U,
                "baked document retains no hidden procedural base") &&
         expect(repeated.accepted && !repeated.changed &&
                    repeated.status ==
                        cr::CreativeTerrainOperationMutationStatus::NoChange,
                "baking an already detached field is a semantic no-op") &&
         expect(overlayAdded.accepted && overlayRemoved.accepted &&
                    cr::creativeTerrainHeightFieldsEqual(
                        beforeHeight, document.terrainHeightField()) &&
                    cr::creativeTerrainMaterialFieldsEqual(
                        beforeMaterial, document.terrainMaterialField()),
                "new operations capture the baked output as their removable base");
}

bool terrainStampOperationOwnsBakedHeightMaterialAndProvenance() {
  const cr::CreativeTerrainHeightField source =
      flatField({{0, 0}, 2U, 2U}, 4U);
  cr::CreativeTerrainMaterialField sourceMaterial;
  const cr::CreativeTerrainMaterialEdit stone =
      cr::makeCreativeTerrainMaterialWeightEdit(
          {1, 1}, cr::creativeTerrainMaterialSolidWeights(
                      cr::CreativeTerrainMaterial::Stone));
  static_cast<void>(sourceMaterial.apply(std::span{&stone, 1U}));
  cr::CreativeTerrainStamp stamp;
  const cr::CreativeTerrainStampCopyReceipt copied =
      cr::copyCreativeTerrainRegionToStamp(
          81U, 9U, cr::buildCreativeTerrainHeightSurfacePlan(source),
          sourceMaterial, {0, 0}, {1, 1}, "terrain-stamp-shelf",
          "Stone Shelf", 3U, stamp);

  cr::CreativeDocument document = cr::CreativeDocument::create("Stamp Stack");
  constexpr cr::CreativeTerrainHeightFieldBounds baseBounds{{10, 10}, 3U, 3U};
  constexpr std::array<std::uint16_t, 9U> baseHeights{
      2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U};
  static_cast<void>(document.replaceTerrainHeightField(baseBounds, baseHeights));
  const cr::CreativeTerrainHeightField beforeHeight =
      document.terrainHeightField();
  const cr::CreativeTerrainMaterialField beforeMaterial =
      document.terrainMaterialField();

  cr::CreativeTerrainOperationMutationRequest add;
  add.kind = cr::CreativeTerrainOperationMutationKind::Add;
  add.operationKind = cr::CreativeTerrainOperationKind::Stamp;
  add.stamp.stamp = stamp;
  add.stamp.targetMinimum = {10, 10};
  add.stamp.elevationMode = cr::CreativeTerrainStampElevationMode::Surface;
  const cr::CreativeTerrainOperationMutationReceipt added =
      document.applyTerrainOperationMutation(add);
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       added.operationId);
  const bool operationFound = operation != nullptr;
  const cr::CreativeTerrainOperation savedOperation =
      operation == nullptr ? cr::CreativeTerrainOperation{} : *operation;
  const cr::CreativeTerrainOperationReplayResult replay =
      cr::replayCreativeTerrainOperations(document.terrainField(),
                                          document.terrainOperationStack());

  cr::CreativeTerrainOperationMutationRequest update = add;
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.operationId;
  update.stamp.targetMinimum = {11, 11};
  update.stamp.quarterTurns = 1U;
  const cr::CreativeTerrainOperationMutationReceipt updated =
      document.applyTerrainOperationMutation(update);

  cr::CreativeTerrainOperationMutationRequest remove;
  remove.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  remove.operationId = added.operationId;
  const cr::CreativeTerrainOperationMutationReceipt removed =
      document.applyTerrainOperationMutation(remove);

  return expect(copied.accepted && added.accepted && added.changed &&
                    operationFound &&
                    savedOperation.kind ==
                        cr::CreativeTerrainOperationKind::Stamp &&
                    savedOperation.stamp.stamp.assetId ==
                        "terrain-stamp-shelf" &&
                    savedOperation.stamp.stamp.assetVersion == 3U &&
                    savedOperation.stamp.stamp.contentSignature ==
                        stamp.contentSignature,
                "stamp operation embeds exact baked source identity and content") &&
         expect(document.terrainOperationStack().operations.empty() &&
                    replay.receipt.accepted &&
                    replay.receipt.stampOperationCount == 1U &&
                    replay.heightField.heightAt({10, 10}) == 2U &&
                    replay.materialField.materialAt({11, 11}) ==
                        cr::CreativeTerrainMaterial::Stone,
                "stamp replay owns exact transformed height and material output") &&
         expect(updated.accepted && updated.changed &&
                    updated.operationId == added.operationId,
                "stamp operation remains parametrically repositionable") &&
         expect(removed.accepted && removed.changed &&
                    cr::creativeTerrainHeightFieldsEqual(
                        beforeHeight, document.terrainHeightField()) &&
                    cr::creativeTerrainMaterialFieldsEqual(
                        beforeMaterial, document.terrainMaterialField()),
                "removing a stamp restores its exact baked base");
}

}  // namespace

int main() {
  return gradeKernelOwnsGeometryAndWalkability() &&
                 gradeAdaptersPreservePathPadAndBridgeSemantics() &&
                 generatedGradeSupportsFullPlayerTraversal() &&
                 generatedPathSupportsReachabilityAndFullPlayerTraversal() &&
                 gradeOperationsRemainEditableAndFailAtomically() &&
                 regionOperationsRemainEditableAndOrdered() &&
                 profileOperationsRemainEditableSeededAndOrdered() &&
                 landformOperationsOwnHeightMaterialAndEditableProvenance() &&
                 laterHeightOperationsInvalidateAndRemovalRestoresHardEdges() &&
                 orderedReplayAndMutationLifecycle() &&
                 updateAndReplayAreDeterministic() &&
                 driftInvalidRequestsAndCapacityFailClosed() &&
                 pathOperationsOwnHeightAndMaterialProvenance() &&
                 documentOwnsReplayAndDirectReplacementCollapsesProvenance() &&
                 operationProvenanceIsExplicitAndValidated() &&
                 bakeAllPreservesDerivedTerrainAndClearsProceduralProvenance() &&
                 terrainStampOperationOwnsBakedHeightMaterialAndProvenance()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
