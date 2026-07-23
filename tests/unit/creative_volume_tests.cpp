#include "app/iggy3d/creative/tools/Volume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/tools/Pattern.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
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

cr::CreativeVolumeSelection selection(cr::CreativeGridCoord3 first,
                                      cr::CreativeGridCoord3 second,
                                      double cellSize = 1.0) {
  cr::CreativeVolumeSelection result;
  result.cellSize = cellSize;
  static_cast<void>(cr::advanceCreativeVolumeSelection(result, first));
  static_cast<void>(cr::advanceCreativeVolumeSelection(result, second));
  return result;
}

cr::CreativeVolumeOperationRequest request(
    cr::CreativeVolumeOperationKind operation,
    cr::CreativeVolumeSelection selected,
    cr::CreativeObjectKind kind = cr::CreativeObjectKind::Wall) {
  cr::CreativeVolumeOperationRequest result;
  result.operation = operation;
  result.selection = selected;
  result.objectKind = kind;
  return result;
}

cr::CreativeDocument document(std::string_view name) {
  cr::CreativeDocument result = cr::CreativeDocument::create(std::string{name});
  static_cast<void>(result.assignId(1U));
  return result;
}

cr::CreativeDocumentCreateReceipt createBoundedObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view name,
    cr::CreativeBounds bounds,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt,
    std::vector<std::string> tags = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string{name};
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  request.parentId = parentId;
  request.tags = std::move(tags);
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createPositionedObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view name,
    cr::CreativeVec3 position,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string{name};
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.parentId = parentId;
  return document.createObject(request);
}

cr::CreativeObjectId clonedObjectId(
    const cr::CreativeVolumeOperationReceipt& receipt,
    cr::CreativeObjectId sourceObjectId) {
  const auto found = std::find_if(
      receipt.clonedObjectIdRemaps.begin(),
      receipt.clonedObjectIdRemaps.end(),
      [sourceObjectId](const cr::CreativeVolumeObjectIdRemap& remap) {
        return remap.sourceObjectId == sourceObjectId;
      });
  return found == receipt.clonedObjectIdRemaps.end()
             ? cr::kInvalidObjectId
             : found->clonedObjectId;
}

bool canonicalSelectionAndPreview() {
  cr::CreativeVolumeSelection selected =
      selection({2, 0, 3}, {-1, 2, 1});
  const cr::CreativeGridBounds3 grid = cr::creativeVolumeGridBounds(selected);
  const cr::CreativeBounds world = cr::creativeVolumeWorldBounds(selected);

  cr::CreativeVolumeSelection inProgress;
  static_cast<void>(cr::advanceCreativeVolumeSelection(inProgress, {3, 0, 4}));
  const cr::CreativeVolumeSelection preview =
      cr::previewCreativeVolumeSelection(inProgress, {5, 0, 7});

  bool ok = expect(cr::creativeVolumeSelectionValid(selected),
                   "canonical selection valid") &&
            expect(grid.min.x == -1 && grid.min.y == 0 && grid.min.z == 1,
                   "canonical grid minimum") &&
            expect(grid.max.x == 3 && grid.max.y == 3 && grid.max.z == 4,
                   "canonical grid maximum exclusive") &&
            expect(cr::creativeVolumeCellCount(selected) == 36U,
                   "canonical cell count") &&
            expect(world.min.x == -1.0 && world.max.x == 3.0 &&
                       world.min.y == 0.0 && world.max.y == 3.0,
                   "canonical world bounds") &&
            expect(cr::creativeVolumeSelectionComplete(preview),
                   "cursor preview is complete") &&
            expect(cr::creativeVolumeCellCount(preview) == 12U,
                   "cursor preview cell count");
  ok = expect(cr::resizeCreativeVolumeSelectionHeight(selected, 1),
              "height grows") &&
       expect(cr::creativeVolumeCellCount(selected) == 48U,
              "height growth updates count") &&
       ok;
  return ok;
}

bool fillIsAtomicAndIdempotent() {
  cr::CreativeDocument document = ::document("fill");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 1, 1});
  const cr::CreativeVolumeOperationRequest fill =
      request(cr::CreativeVolumeOperationKind::Fill, selected);

  const cr::CreativeVolumeOperationReceipt first =
      cr::executeCreativeVolumeOperation(document, fill);
  bool ok = expect(first.accepted && first.changed, "fill accepted") &&
            expect(first.createdVoxelCellCount == 8U,
                   "fill creates eight voxel cells") &&
            expect(document.objectCount() == 0U,
                   "fill creates no document objects") &&
            expect(document.voxelField().occupiedCellCount() == 8U,
                   "fill voxel count") &&
            expect(document.voxelField().materialAt({1, 1, 1}) ==
                       cr::CreativeObjectKind::Wall,
                   "fill cells carry material");

  const std::uint64_t revisionBeforeRepeat = document.revision();
  const cr::CreativeVolumeOperationReceipt repeat =
      cr::executeCreativeVolumeOperation(document, fill);
  ok = expect(repeat.accepted && !repeat.changed,
              "repeat fill accepted as no change") &&
       expect(repeat.skippedOccupiedCellCount == 8U,
              "repeat fill reports occupied cells") &&
       expect(document.voxelField().occupiedCellCount() == 8U,
              "repeat fill creates no duplicates") &&
       expect(document.revision() == revisionBeforeRepeat,
              "repeat fill preserves revision") &&
       ok;
  return ok;
}

bool fillOverlapPolicyIsExplicitAndAtomic() {
  cr::CreativeDocument document = ::document("fill overlap policy");
  const std::array<cr::CreativeVoxelEdit, 2U> seed{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
  };
  const cr::CreativeVoxelMutationReceipt seeded =
      document.applyVoxelEdits(seed);
  if (!seeded.accepted) {
    return expect(false, "fill overlap seed accepted");
  }

  cr::CreativeVolumeOperationRequest preserve = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {2, 0, 0}), cr::CreativeObjectKind::Floor);
  const cr::CreativeVolumeOperationReceipt preserved =
      cr::executeCreativeVolumeOperation(document, preserve);
  bool ok = expect(preserved.accepted && preserved.changed &&
                       preserved.fillOverlapPolicy ==
                           cr::CreativeVolumeFillOverlapPolicy::PreserveExisting,
                   "preserve fill records its overlap contract") &&
            expect(preserved.createdVoxelCellCount == 1U &&
                       preserved.skippedOccupiedCellCount == 2U,
                   "preserve fill adds only empty cells") &&
            expect(document.voxelField().materialAt({0, 0, 0}) ==
                       cr::CreativeObjectKind::Wall &&
                       document.voxelField().materialAt({1, 0, 0}) ==
                           cr::CreativeObjectKind::Floor &&
                       document.voxelField().materialAt({2, 0, 0}) ==
                           cr::CreativeObjectKind::Floor,
                   "preserve fill leaves occupied materials intact");

  cr::CreativeVolumeOperationRequest replace = preserve;
  replace.fillOverlapPolicy =
      cr::CreativeVolumeFillOverlapPolicy::ReplaceExisting;
  const cr::CreativeVolumeOperationReceipt replaced =
      cr::executeCreativeVolumeOperation(document, replace);
  ok = expect(replaced.accepted && replaced.changed &&
                  replaced.replacedVoxelCellCount == 1U &&
                  replaced.unchangedMaterialCellCount == 2U,
              "replace fill rewrites differing material and counts matches") &&
       expect(document.voxelField().materialAt({0, 0, 0}) ==
                  cr::CreativeObjectKind::Floor &&
                  document.voxelField().occupiedCellCount() == 3U,
              "replace fill leaves one uniform editable voxel volume") &&
       ok;

  const std::uint64_t revisionBeforeRepeat = document.revision();
  const cr::CreativeVolumeOperationReceipt repeated =
      cr::executeCreativeVolumeOperation(document, replace);
  ok = expect(repeated.accepted && !repeated.changed &&
                  repeated.unchangedMaterialCellCount == 3U &&
                  repeated.reasonCode ==
                      "creative_volume_fill_material_already_applied",
              "repeat replace fill is an explicit no-op") &&
       expect(document.revision() == revisionBeforeRepeat,
              "repeat replace fill preserves revision") &&
       ok;

  cr::CreativeVolumeOperationRequest invalid = replace;
  invalid.fillOverlapPolicy =
      static_cast<cr::CreativeVolumeFillOverlapPolicy>(255U);
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(document, invalid);
  ok = expect(!rejected.accepted && !rejected.changed &&
                  rejected.status ==
                      cr::CreativeVolumeOperationStatus::InvalidRequest,
              "invalid overlap policy fails closed") &&
       expect(document.revision() == revisionBeforeRepeat,
              "invalid overlap policy cannot mutate document") &&
       ok;

  cr::CreativeDocument legacy = ::document("legacy fill migration");
  cr::CreativeDocumentCreateRequest legacyCell;
  legacyCell.kind = cr::CreativeObjectKind::Wall;
  legacyCell.name = "legacy volume cell";
  legacyCell.bounds = cr::creativeVolumeCellBounds({0, 0, 0}, 1.0, {});
  legacyCell.hasBoundsOverride = true;
  legacyCell.tags.emplace_back(cr::kCreativeVolumeCellTag);
  const cr::CreativeDocumentCreateReceipt legacyCreated =
      legacy.createObject(legacyCell);
  const cr::CreativeVolumeOperationReceipt migrated =
      cr::executeCreativeVolumeOperation(
          legacy,
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {0, 0, 0}),
                  cr::CreativeObjectKind::Floor));
  ok = expect(legacyCreated.accepted && migrated.accepted &&
                  !migrated.changed && legacy.objectCount() == 1U,
              "preserve fill leaves a tagged legacy cell untouched") &&
       ok;

  cr::CreativeVolumeOperationRequest migrate = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {0, 0, 0}), cr::CreativeObjectKind::Floor);
  migrate.fillOverlapPolicy =
      cr::CreativeVolumeFillOverlapPolicy::ReplaceExisting;
  const cr::CreativeVolumeOperationReceipt migratedReplace =
      cr::executeCreativeVolumeOperation(legacy, migrate);
  ok = expect(migratedReplace.accepted && migratedReplace.changed &&
                  migratedReplace.matchedObjectCount == 1U &&
                  migratedReplace.removedObjectCount == 1U &&
                  migratedReplace.createdVoxelCellCount == 1U,
              "replace fill migrates one tagged legacy cell atomically") &&
       expect(legacy.objectCount() == 0U &&
                  legacy.voxelField().materialAt({0, 0, 0}) ==
                      cr::CreativeObjectKind::Floor,
              "legacy migration leaves voxel-native editable output") &&
       ok;

  cr::CreativeDocument lockedLegacy = ::document("locked legacy fill");
  legacyCell.locked = true;
  legacyCell.hasLockedOverride = true;
  const cr::CreativeDocumentCreateReceipt lockedCreated =
      lockedLegacy.createObject(legacyCell);
  const std::uint64_t lockedRevision = lockedLegacy.revision();
  const cr::CreativeVolumeOperationReceipt lockedRejected =
      cr::executeCreativeVolumeOperation(lockedLegacy, migrate);
  return expect(lockedCreated.accepted && !lockedRejected.accepted &&
                    !lockedRejected.changed &&
                    lockedRejected.status ==
                        cr::CreativeVolumeOperationStatus::RemoveRejected,
                "locked legacy migration rejects") &&
         expect(lockedLegacy.objectCount() == 1U &&
                    lockedLegacy.voxelField().occupiedCellCount() == 0U &&
                    lockedLegacy.revision() == lockedRevision,
                "failed legacy migration rolls back without partial voxels") &&
         ok;
}

bool hollowCreatesOnlyShell() {
  cr::CreativeDocument document = ::document("hollow");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {2, 2, 2});
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Hollow, selected));

  const bool centerMissing = !document.voxelField().occupied({1, 1, 1});
  bool ok = expect(receipt.accepted && receipt.changed, "hollow accepted") &&
            expect(receipt.plannedCellCount == 26U, "hollow shell count") &&
            expect(receipt.createdVoxelCellCount == 26U,
                   "hollow creates shell") &&
            expect(centerMissing, "hollow center remains empty");

  cr::CreativeDocument solid = ::document("solid to hollow");
  const cr::CreativeVolumeOperationReceipt filled =
      cr::executeCreativeVolumeOperation(
          solid, request(cr::CreativeVolumeOperationKind::Fill, selected));
  const cr::CreativeVolumeOperationReceipt hollowed =
      cr::executeCreativeVolumeOperation(
          solid, request(cr::CreativeVolumeOperationKind::Hollow, selected));
  ok = expect(filled.accepted && filled.createdVoxelCellCount == 27U,
              "solid setup fill") &&
       expect(hollowed.accepted && hollowed.changed,
              "solid converted to hollow") &&
       expect(hollowed.removedVoxelCellCount == 1U,
              "hollow removes kernel-owned interior") &&
       expect(hollowed.createdVoxelCellCount == 0U,
              "hollow reuses existing boundary") &&
       expect(solid.voxelField().occupiedCellCount() == 26U,
              "solid to hollow final count") &&
       ok;
  return ok;
}

bool hollowThicknessAlignmentAndBoundsAreExact() {
  cr::CreativeDocument inwardDocument = ::document("thick inward shell");
  cr::CreativeVolumeOperationRequest inward = request(
      cr::CreativeVolumeOperationKind::Hollow,
      selection({0, 0, 0}, {4, 4, 4}));
  inward.hollowThickness = cr::CreativeVolumeHollowThickness::TwoCells;
  const cr::CreativeVolumeOperationReceipt inwardReceipt =
      cr::executeCreativeVolumeOperation(inwardDocument, inward);
  bool ok = expect(inwardReceipt.accepted && inwardReceipt.changed &&
                       inwardReceipt.plannedCellCount == 124U &&
                       inwardReceipt.createdVoxelCellCount == 124U,
                   "two-cell inward shell applies exact planned cells") &&
            expect(inwardReceipt.hollowBoundsValid &&
                       inwardReceipt.hollowExteriorBounds.min.x == 0 &&
                       inwardReceipt.hollowExteriorBounds.max.x == 5 &&
                       inwardReceipt.hollowInteriorBounds.min.x == 2 &&
                       inwardReceipt.hollowInteriorBounds.max.x == 3 &&
                       !inwardDocument.voxelField().occupied({2, 2, 2}),
                   "inward receipt preserves exact exterior and cavity bounds") &&
            expect(inwardReceipt.hollowThickness ==
                           cr::CreativeVolumeHollowThickness::TwoCells &&
                       inwardReceipt.hollowAlignment ==
                           cr::CreativeVolumeHollowAlignment::Inward &&
                       inwardReceipt.hollowOpening ==
                           cr::CreativeVolumeHollowOpening::Closed &&
                       inwardReceipt.hollowCornerRule ==
                           cr::CreativeVolumeHollowCornerRule::KeepEdges,
                   "hollow receipt preserves every editable parameter");

  cr::CreativeDocument outwardDocument = ::document("outward shell");
  cr::CreativeVolumeOperationRequest outward = request(
      cr::CreativeVolumeOperationKind::Hollow,
      selection({0, 0, 0}, {0, 0, 0}));
  outward.hollowAlignment = cr::CreativeVolumeHollowAlignment::Outward;
  const cr::CreativeVolumeOperationReceipt outwardReceipt =
      cr::executeCreativeVolumeOperation(outwardDocument, outward);
  ok = expect(outwardReceipt.accepted && outwardReceipt.changed &&
                  outwardReceipt.createdVoxelCellCount == 26U &&
                  outwardReceipt.shapeCandidateCellCount == 27U,
              "outward shell grows around a one-cell cavity") &&
       expect(outwardReceipt.hollowExteriorBounds.min.x == -1 &&
                  outwardReceipt.hollowExteriorBounds.max.x == 2 &&
                  outwardReceipt.hollowInteriorBounds.min.x == 0 &&
                  outwardReceipt.hollowInteriorBounds.max.x == 1 &&
                  !outwardDocument.voxelField().occupied({0, 0, 0}) &&
                  outwardDocument.voxelField().occupied({-1, 0, 0}),
              "outward alignment leaves selected bounds as the cavity") &&
       ok;

  cr::CreativeDocument framedDocument = ::document("framed shell opening");
  cr::CreativeVolumeOperationRequest framed = request(
      cr::CreativeVolumeOperationKind::Hollow,
      selection({0, 0, 0}, {4, 4, 4}));
  framed.hollowOpening = cr::CreativeVolumeHollowOpening::PositiveEnd;
  const cr::CreativeVolumeOperationReceipt framedReceipt =
      cr::executeCreativeVolumeOperation(framedDocument, framed);
  ok = expect(framedReceipt.accepted &&
                  framedReceipt.createdVoxelCellCount == 89U &&
                  framedDocument.voxelField().occupied({0, 4, 0}) &&
                  !framedDocument.voxelField().occupied({2, 4, 2}),
              "framed opening retains edge cells and removes its center") &&
       ok;

  cr::CreativeDocument rejectedDocument = ::document("rejected shell");
  cr::CreativeVolumeOperationRequest tooSmall = request(
      cr::CreativeVolumeOperationKind::Hollow,
      selection({0, 0, 0}, {2, 2, 2}));
  tooSmall.hollowThickness = cr::CreativeVolumeHollowThickness::TwoCells;
  const cr::CreativeVolumeOperationReceipt tooSmallReceipt =
      cr::executeCreativeVolumeOperation(rejectedDocument, tooSmall);
  cr::CreativeVolumeOperationRequest overflow = request(
      cr::CreativeVolumeOperationKind::Hollow,
      selection({std::numeric_limits<std::int32_t>::max() - 1, 0, 0},
                {std::numeric_limits<std::int32_t>::max() - 1, 0, 0}));
  overflow.hollowAlignment = cr::CreativeVolumeHollowAlignment::Outward;
  const cr::CreativeVolumeOperationReceipt overflowReceipt =
      cr::executeCreativeVolumeOperation(rejectedDocument, overflow);
  return expect(!tooSmallReceipt.accepted && !tooSmallReceipt.changed &&
                    tooSmallReceipt.reasonCode ==
                        "creative_volume_hollow_shell_does_not_fit",
                "undersized inward shell rejects with a stable reason") &&
         expect(!overflowReceipt.accepted && !overflowReceipt.changed &&
                    overflowReceipt.reasonCode ==
                        "creative_volume_hollow_bounds_overflow",
                "outward coordinate overflow rejects before enumeration") &&
         expect(rejectedDocument.voxelField().occupiedCellCount() == 0U &&
                    rejectedDocument.revision() == 0U,
                "invalid hollow settings cannot partially mutate") &&
         ok;
}

bool shapedFillAndHollowUseSharedPlanner() {
  const cr::CreativeVolumeSelection selected =
      selection({0, 0, 0}, {2, 2, 2});
  cr::CreativeVolumeOperationRequest ellipsoid =
      request(cr::CreativeVolumeOperationKind::Fill, selected);
  ellipsoid.shapeKind = cr::CreativeShapeBrushKind::Ellipsoid;

  cr::CreativeDocument document = ::document("ellipsoid");
  const cr::CreativeVolumeOperationReceipt filled =
      cr::executeCreativeVolumeOperation(document, ellipsoid);
  const bool cornersMissing = !document.voxelField().occupied({0, 0, 0}) &&
                              !document.voxelField().occupied({2, 2, 2});
  bool ok = expect(filled.accepted && filled.changed,
                   "ellipsoid fill accepted") &&
            expect(filled.shapeKind == cr::CreativeShapeBrushKind::Ellipsoid &&
                       filled.shapeAxis == cr::CreativeShapeBrushAxis::Y,
                   "ellipsoid receipt records planner inputs") &&
            expect(filled.shapeCandidateCellCount == 27U &&
                       filled.plannedCellCount == 19U &&
                       filled.createdVoxelCellCount == 19U,
                   "ellipsoid plan and mutation counts agree") &&
            expect(document.voxelField().occupiedCellCount() == 19U &&
                       cornersMissing,
                   "ellipsoid document matches shared shape plan");

  ellipsoid.operation = cr::CreativeVolumeOperationKind::Hollow;
  const cr::CreativeVolumeOperationReceipt hollowed =
      cr::executeCreativeVolumeOperation(document, ellipsoid);
  ok = expect(hollowed.accepted && hollowed.changed,
              "ellipsoid hollow accepted") &&
       expect(hollowed.plannedCellCount == 18U &&
                  hollowed.removedVoxelCellCount == 1U &&
                  hollowed.createdVoxelCellCount == 0U,
              "ellipsoid hollow removes only planner interior") &&
       expect(document.voxelField().occupiedCellCount() == 18U,
              "ellipsoid hollow final object count") &&
       ok;

  cr::CreativeDocument lineDocument = ::document("line");
  cr::CreativeVolumeOperationRequest line = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({-2, 1, 0}, {3, 3, 1}));
  line.shapeKind = cr::CreativeShapeBrushKind::Line;
  const cr::CreativeVolumeOperationReceipt lineReceipt =
      cr::executeCreativeVolumeOperation(lineDocument, line);
  ok = expect(lineReceipt.accepted &&
                  lineReceipt.createdVoxelCellCount == 6U &&
                  lineReceipt.plannedCellCount == 6U,
              "line fill applies deterministic Bresenham plan") &&
       expect(lineDocument.voxelField().occupiedCellCount() == 6U,
              "line document count matches plan") &&
       ok;

  constexpr std::array cylinderAxes{
      cr::CreativeShapeBrushAxis::X,
      cr::CreativeShapeBrushAxis::Y,
      cr::CreativeShapeBrushAxis::Z,
  };
  for (const cr::CreativeShapeBrushAxis axis : cylinderAxes) {
    cr::CreativeDocument cylinderDocument = ::document("oriented cylinder");
    cr::CreativeVolumeOperationRequest cylinder = request(
        cr::CreativeVolumeOperationKind::Fill,
        selection({0, 0, 0}, {4, 2, 2}));
    cylinder.shapeKind = cr::CreativeShapeBrushKind::Cylinder;
    cylinder.shapeAxis = axis;
    const cr::CreativeVolumeOperationReceipt cylinderReceipt =
        cr::executeCreativeVolumeOperation(cylinderDocument, cylinder);
    ok = expect(cylinderReceipt.accepted && cylinderReceipt.changed &&
                    cylinderReceipt.shapeAxis == axis &&
                    cylinderReceipt.createdVoxelCellCount ==
                        cylinderReceipt.plannedCellCount &&
                    cylinderReceipt.plannedCellCount > 0U,
                "cylinder orientation reaches exact volume mutation") &&
         ok;
  }

  cr::CreativeVolumeOperationRequest invalid = line;
  invalid.shapeKind = static_cast<cr::CreativeShapeBrushKind>(255U);
  const std::uint64_t revisionBefore = lineDocument.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(lineDocument, invalid);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeVolumeOperationStatus::InvalidRequest,
                "invalid shape request fails closed") &&
         expect(lineDocument.revision() == revisionBefore &&
                    lineDocument.voxelField().occupiedCellCount() == 6U,
                "invalid shape leaves document unchanged") &&
         ok;
}

bool operationLimitRejectsWithoutMutation() {
  cr::CreativeDocument document = ::document("limit");
  cr::CreativeVolumeOperationRequest fill = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {2, 2, 2}));
  fill.maxAffectedObjects = 10U;
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(document, fill);
  return expect(!receipt.accepted && !receipt.changed,
                "limit rejection not accepted") &&
         expect(receipt.status ==
                    cr::CreativeVolumeOperationStatus::OperationLimitExceeded,
                "limit status") &&
         expect(document.voxelField().occupiedCellCount() == 0U,
                "limit leaves voxel field empty") &&
         expect(document.revision() == revisionBefore,
                "limit preserves revision");
}

bool maximumInteractiveSolidStaysOneChunkCuboid() {
  cr::CreativeDocument document = ::document("maximum interactive solid");
  cr::CreativeVolumeOperationRequest fill = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {7, 7, 7}));
  fill.maxAffectedObjects = 512U;
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(document, fill);
  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &document;
  const cr::CreativeRoomBakeResult bake =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  return expect(receipt.accepted && receipt.createdVoxelCellCount == 512U,
                "maximum interactive solid fills 512 voxels") &&
         expect(document.objectCount() == 0U &&
                    document.voxelField().chunkCount() == 1U,
                "maximum solid creates no per-cell objects") &&
         expect(bake.receipt.accepted &&
                    bake.receipt.bakedVoxelCuboidCount == 1U &&
                    bake.room.staticMeshes.size() == 1U,
                "maximum solid bakes as one greedy cuboid");
}

bool voxelFillDoesNotConsumeObjectIds() {
  cr::CreativeDocument document = ::document("id exhaustion");
  cr::CreativeDocumentRestoreRequest restore;
  restore.documentId = document.id();
  restore.name = "id exhaustion";
  restore.units = document.units();
  restore.gridSettings = document.gridSettings();
  restore.snapSettings = document.documentSnapSettings();
  restore.worldBounds = document.worldBounds();
  restore.nextObjectId = std::numeric_limits<cr::CreativeObjectId>::max();
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(restore);
  if (!restored.accepted) {
    return expect(false, "id exhaustion restore accepted");
  }

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {1, 0, 0})));
  return expect(receipt.accepted && receipt.changed,
                "voxel fill ignores exhausted object ids") &&
         expect(receipt.createdVoxelCellCount == 2U,
                "voxel fill creates both cells") &&
         expect(document.objectCount() == 0U &&
                    document.voxelField().occupiedCellCount() == 2U,
                "voxel fill does not create objects") &&
         expect(document.nextObjectId() ==
                    std::numeric_limits<cr::CreativeObjectId>::max(),
                "voxel fill preserves id cursor") &&
         expect(document.revision() == revisionBefore + 1U,
                "voxel batch advances revision once");
}

bool replaceTargetsOnlyVolumeCells() {
  cr::CreativeDocument document = ::document("replace");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 0, 1});
  const cr::CreativeVolumeOperationReceipt fill =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill, selected));
  if (!fill.accepted) {
    return expect(false, "replace setup fill accepted");
  }

  cr::CreativeVolumeOperationRequest replace = request(
      cr::CreativeVolumeOperationKind::Replace, selected,
      cr::CreativeObjectKind::Floor);
  replace.hasReplaceKindFilter = true;
  replace.replaceKindFilter = cr::CreativeObjectKind::Wall;
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(document, replace);

  return expect(receipt.accepted && receipt.changed, "replace accepted") &&
         expect(receipt.matchedVoxelCellCount == 4U,
                "replace matched voxel count") &&
         expect(receipt.replacedVoxelCellCount == 4U,
                "replace voxel count") &&
         expect(document.voxelField().occupiedCellCount() == 4U,
                "replace preserves cell count") &&
         expect(document.voxelField().materialAt({1, 0, 1}) ==
                    cr::CreativeObjectKind::Floor,
                "replace writes target material");
}

bool replaceClassifiesMembersAndPreservesLegacyIdentity() {
  cr::CreativeDocument document = ::document("replace member domains");
  const std::array<cr::CreativeVoxelEdit, 2U> seed{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
  };
  if (!document.applyVoxelEdits(seed).accepted) {
    return expect(false, "replace member seed voxels accepted");
  }

  const auto createLegacyCell = [&document](
                                    cr::CreativeGridCoord3 cell,
                                    cr::CreativeObjectKind kind,
                                    std::string name) {
    cr::CreativeDocumentCreateRequest create;
    create.kind = kind;
    create.name = std::move(name);
    create.bounds = cr::creativeVolumeCellBounds(cell, 1.0, {});
    create.hasBoundsOverride = true;
    create.layerId = 7U;
    create.hasLayerOverride = true;
    create.visible = false;
    create.hasVisibleOverride = true;
    create.tags.emplace_back(cr::kCreativeVolumeCellTag);
    create.tags.emplace_back("metadata.must-survive");
    return document.createObject(create);
  };

  const cr::CreativeDocumentCreateReceipt wall = createLegacyCell(
      {2, 0, 0}, cr::CreativeObjectKind::Wall, "legacy wall");
  const cr::CreativeDocumentCreateReceipt floor = createLegacyCell(
      {3, 0, 0}, cr::CreativeObjectKind::Floor, "legacy floor");
  const cr::CreativeDocumentCreateReceipt crate = createLegacyCell(
      {4, 0, 0}, cr::CreativeObjectKind::Crate, "tagged prop");
  if (!wall.accepted || !floor.accepted || !crate.accepted) {
    return expect(false, "replace member seed objects accepted");
  }

  cr::CreativeVolumeOperationRequest replace = request(
      cr::CreativeVolumeOperationKind::Replace,
      selection({0, 0, 0}, {4, 0, 0}), cr::CreativeObjectKind::Floor);
  replace.replaceMemberMask = cr::CreativeVolumeMemberMask::Both;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt preview =
      cr::previewCreativeVolumeOperation(document, replace);

  const auto containsCell = [](std::span<const cr::CreativeGridCoord3> cells,
                               cr::CreativeGridCoord3 cell) {
    return std::find(cells.begin(), cells.end(), cell) != cells.end();
  };
  const auto containsObject = [](std::span<const cr::CreativeObjectId> ids,
                                 cr::CreativeObjectId id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
  };

  bool ok = expect(preview.accepted && preview.changed,
                   "mixed replace preview accepted") &&
            expect(preview.replaceMemberMask ==
                           cr::CreativeVolumeMemberMask::Both &&
                       preview.matchedVoxelCellCount == 2U &&
                       preview.matchedObjectCount == 2U &&
                       preview.excludedVoxelCellCount == 0U &&
                       preview.excludedObjectCount == 1U,
                   "mixed replace classifies matched and excluded domains") &&
            expect(preview.replacedVoxelCellCount == 1U &&
                       preview.unchangedMaterialCellCount == 1U &&
                       preview.replacedObjectCount == 1U &&
                       preview.unchangedObjectCount == 1U &&
                       cr::creativeVolumeChangedMemberCount(preview) == 2U,
                   "mixed replace classifies changed and unchanged members") &&
            expect(containsCell(preview.changedVoxelCells, {0, 0, 0}) &&
                       containsCell(preview.unchangedVoxelCells, {1, 0, 0}) &&
                       containsObject(preview.changedObjectIds, wall.objectId) &&
                       containsObject(preview.unchangedObjectIds,
                                      floor.objectId),
                   "mixed replace reports exact preview members") &&
            expect(document.revision() == revisionBefore &&
                       document.voxelField().materialAt({0, 0, 0}) ==
                           cr::CreativeObjectKind::Wall &&
                       document.findObject(wall.objectId)->kind ==
                           cr::CreativeObjectKind::Wall,
                   "mixed replace preview leaves source document unchanged");

  cr::CreativeVolumeOperationRequest voxelsOnly = replace;
  voxelsOnly.hasReplaceKindFilter = true;
  voxelsOnly.replaceKindFilter = cr::CreativeObjectKind::Wall;
  voxelsOnly.replaceMemberMask = cr::CreativeVolumeMemberMask::VoxelCells;
  const cr::CreativeVolumeOperationReceipt voxelPreview =
      cr::previewCreativeVolumeOperation(document, voxelsOnly);
  ok = expect(voxelPreview.accepted && voxelPreview.changed &&
                  voxelPreview.matchedVoxelCellCount == 1U &&
                  voxelPreview.matchedObjectCount == 0U &&
                  voxelPreview.excludedVoxelCellCount == 1U &&
                  voxelPreview.excludedObjectCount == 3U &&
                  voxelPreview.changedVoxelCells.size() == 1U,
              "voxel-only source filter excludes every other member") &&
       ok;

  cr::CreativeVolumeOperationRequest objectsOnly = replace;
  objectsOnly.hasReplaceKindFilter = true;
  objectsOnly.replaceKindFilter = cr::CreativeObjectKind::Wall;
  objectsOnly.replaceMemberMask =
      cr::CreativeVolumeMemberMask::DocumentObjects;
  const cr::CreativeVolumeOperationReceipt objectPreview =
      cr::previewCreativeVolumeOperation(document, objectsOnly);
  ok = expect(objectPreview.accepted && objectPreview.changed &&
                  objectPreview.matchedVoxelCellCount == 0U &&
                  objectPreview.matchedObjectCount == 1U &&
                  objectPreview.excludedVoxelCellCount == 2U &&
                  objectPreview.excludedObjectCount == 2U &&
                  objectPreview.changedObjectIds.size() == 1U &&
                  objectPreview.changedObjectIds.front() == wall.objectId,
              "object-only source filter addresses only compatible objects") &&
       ok;

  cr::CreativeVolumeOperationRequest bounded = replace;
  bounded.maxAffectedObjects = 3U;
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(document, bounded);
  ok = expect(!rejected.accepted && !rejected.changed &&
                  rejected.status ==
                      cr::CreativeVolumeOperationStatus::OperationLimitExceeded &&
                  rejected.matchedVoxelCellCount == 2U &&
                  rejected.matchedObjectCount == 2U &&
                  rejected.changedVoxelCells.empty() &&
                  rejected.unchangedVoxelCells.empty() &&
                  rejected.changedObjectIds.empty() &&
                  rejected.unchangedObjectIds.empty(),
              "replace limit bounds all matched members and clears previews") &&
       expect(document.revision() == revisionBefore,
              "bounded replace rejection is atomic") &&
       ok;

  const cr::CreativeVolumeOperationReceipt applied =
      cr::executeCreativeVolumeOperation(document, replace);
  const cr::CreativeObject* replacedWall = document.findObject(wall.objectId);
  const cr::CreativeObject* unchangedFloor = document.findObject(floor.objectId);
  const cr::CreativeObject* excludedCrate = document.findObject(crate.objectId);
  return expect(applied.accepted && applied.changed &&
                    applied.replacedVoxelCellCount == 1U &&
                    applied.replacedObjectCount == 1U &&
                    document.revision() == revisionBefore + 1U,
                "mixed replace commits both domains in one revision") &&
         expect(document.objectCount() == 3U && replacedWall != nullptr &&
                    replacedWall->id == wall.objectId &&
                    replacedWall->kind == cr::CreativeObjectKind::Floor &&
                    replacedWall->name == "legacy wall" &&
                    replacedWall->layerId == 7U && !replacedWall->visible &&
                    replacedWall->tags.size() == 2U &&
                    replacedWall->tags[1] == "metadata.must-survive",
                "legacy replacement preserves identity and metadata") &&
         expect(unchangedFloor != nullptr &&
                    unchangedFloor->kind == cr::CreativeObjectKind::Floor &&
                    excludedCrate != nullptr &&
                    excludedCrate->kind == cr::CreativeObjectKind::Crate,
                "unchanged member and arbitrary tagged prop remain intact") &&
         expect(document.voxelField().materialAt({0, 0, 0}) ==
                        cr::CreativeObjectKind::Floor &&
                    document.voxelField().materialAt({1, 0, 0}) ==
                        cr::CreativeObjectKind::Floor,
                "mixed replace writes the target voxel material") &&
         ok;
}

bool eraseFiltersAndProtectsSourceOwnedContent() {
  cr::CreativeDocument document = ::document("erase ownership");
  const std::array<cr::CreativeVoxelEdit, 2U> seed{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
  };
  if (!document.applyVoxelEdits(seed).accepted) {
    return expect(false, "erase ownership seed voxels accepted");
  }

  const auto createCellObject = [&document](
                                    cr::CreativeGridCoord3 cell,
                                    cr::CreativeObjectKind kind,
                                    std::string name,
                                    std::vector<std::string> tags = {}) {
    cr::CreativeDocumentCreateRequest create;
    create.kind = kind;
    create.name = std::move(name);
    create.bounds = cr::creativeVolumeCellBounds(cell, 1.0, {});
    create.hasBoundsOverride = true;
    create.tags = std::move(tags);
    return document.createObject(create);
  };
  const cr::CreativeDocumentCreateReceipt ordinary = createCellObject(
      {2, 0, 0}, cr::CreativeObjectKind::Wall, "ordinary wall");
  const cr::CreativeDocumentCreateReceipt worldOwned = createCellObject(
      {3, 0, 0}, cr::CreativeObjectKind::Wall, "generated wall",
      {"creative_world_layout:test_layout"});
  const cr::CreativeDocumentCreateReceipt generated = createCellObject(
      {4, 0, 0}, cr::CreativeObjectKind::Wall, "pattern output");
  const cr::CreativeDocumentCreateReceipt source = createCellObject(
      {10, 0, 0}, cr::CreativeObjectKind::Crate, "pattern source");
  if (!ordinary.accepted || !worldOwned.accepted || !generated.accepted ||
      !source.accepted) {
    return expect(false, "erase ownership seed objects accepted");
  }

  cr::CreativePatternRecipeMutationRequest addRecipe;
  addRecipe.kind = cr::CreativePatternRecipeMutationKind::Add;
  addRecipe.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  addRecipe.recipe.sourceObjectIds = {source.objectId};
  addRecipe.recipe.generatedObjectIds = {generated.objectId};
  if (!document.applyPatternRecipeMutation(addRecipe).accepted) {
    return expect(false, "erase ownership pattern recipe accepted");
  }

  cr::CreativeVolumeOperationRequest erase = request(
      cr::CreativeVolumeOperationKind::Erase,
      selection({0, 0, 0}, {4, 0, 0}));
  erase.hasEraseKindFilter = true;
  erase.eraseKindFilter = cr::CreativeObjectKind::Wall;
  erase.eraseMemberMask = cr::CreativeVolumeMemberMask::Both;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt preview =
      cr::previewCreativeVolumeOperation(document, erase);

  const auto containsObject = [](std::span<const cr::CreativeObjectId> ids,
                                 cr::CreativeObjectId id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
  };
  bool ok = expect(preview.accepted && preview.changed,
                   "owned-content erase preview accepted") &&
            expect(preview.matchedVoxelCellCount == 1U &&
                       preview.excludedVoxelCellCount == 1U &&
                       preview.matchedObjectCount == 1U &&
                       preview.excludedObjectCount == 2U &&
                       preview.protectedObjectCount == 2U &&
                       preview.dependentSourceObjectCount == 1U,
                   "erase preview classifies filters and source ownership") &&
            expect(preview.removedVoxelCells.size() == 1U &&
                       preview.removedVoxelCells.front() ==
                           cr::CreativeGridCoord3{0, 0, 0} &&
                       preview.removedObjectIds.size() == 1U &&
                       preview.removedObjectIds.front() == ordinary.objectId,
                   "erase preview reports exact deletion members") &&
            expect(containsObject(preview.protectedObjectIds,
                                  worldOwned.objectId) &&
                       containsObject(preview.protectedObjectIds,
                                      generated.objectId) &&
                       containsObject(preview.dependentSourceObjectIds,
                                      source.objectId),
                   "erase preview exposes protected outputs and source") &&
            expect(document.revision() == revisionBefore &&
                       document.containsObject(ordinary.objectId) &&
                       document.voxelField().occupied({0, 0, 0}),
                   "erase preview leaves source document unchanged");

  cr::CreativeVolumeOperationRequest voxelsOnly = erase;
  voxelsOnly.eraseMemberMask = cr::CreativeVolumeMemberMask::VoxelCells;
  const cr::CreativeVolumeOperationReceipt voxelPreview =
      cr::previewCreativeVolumeOperation(document, voxelsOnly);
  ok = expect(voxelPreview.accepted && voxelPreview.changed &&
                  voxelPreview.matchedVoxelCellCount == 1U &&
                  voxelPreview.matchedObjectCount == 0U &&
                  voxelPreview.excludedObjectCount == 3U &&
                  voxelPreview.protectedObjectCount == 0U,
              "voxel-only erase ignores object ownership") &&
       ok;

  cr::CreativeVolumeOperationRequest objectsOnly = erase;
  objectsOnly.eraseMemberMask =
      cr::CreativeVolumeMemberMask::DocumentObjects;
  const cr::CreativeVolumeOperationReceipt objectPreview =
      cr::previewCreativeVolumeOperation(document, objectsOnly);
  ok = expect(objectPreview.accepted && objectPreview.changed &&
                  objectPreview.matchedVoxelCellCount == 0U &&
                  objectPreview.excludedVoxelCellCount == 2U &&
                  objectPreview.matchedObjectCount == 1U &&
                  objectPreview.protectedObjectCount == 2U,
              "object-only erase retains protected source ownership") &&
       ok;

  cr::CreativeVolumeOperationRequest bounded = erase;
  bounded.maxAffectedObjects = 4U;
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(document, bounded);
  ok = expect(!rejected.accepted && !rejected.changed &&
                  rejected.status ==
                      cr::CreativeVolumeOperationStatus::OperationLimitExceeded &&
                  rejected.removedObjectIds.empty() &&
                  rejected.removedVoxelCells.empty() &&
                  document.revision() == revisionBefore,
              "erase bounds the complete contained-member scan") &&
       ok;

  const cr::CreativeVolumeOperationReceipt applied =
      cr::executeCreativeVolumeOperation(document, erase);
  return expect(applied.accepted && applied.changed &&
                    applied.removedObjectCount == 1U &&
                    applied.removedVoxelCellCount == 1U,
                "filtered erase applies exact eligible members") &&
         expect(!document.containsObject(ordinary.objectId) &&
                    document.containsObject(worldOwned.objectId) &&
                    document.containsObject(generated.objectId) &&
                    document.containsObject(source.objectId) &&
                    document.patternRecipeStore().recipes.size() == 1U,
                "erase preserves World Layout and pattern ownership") &&
         expect(!document.voxelField().occupied({0, 0, 0}) &&
                    document.voxelField().materialAt({1, 0, 0}) ==
                        cr::CreativeObjectKind::Floor,
                "erase source filter preserves nonmatching voxel material") &&
         ok;
}

bool eraseIsContainedAndRollbackSafe() {
  cr::CreativeDocument document = ::document("erase");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 0, 1});
  const cr::CreativeVolumeOperationReceipt fill =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill, selected));
  if (!fill.accepted) {
    return expect(false, "erase setup fill accepted");
  }

  cr::CreativeDocumentCreateRequest largeFloor;
  largeFloor.kind = cr::CreativeObjectKind::Floor;
  largeFloor.name = "large floor";
  largeFloor.bounds = {{-10.0, 0.0, -10.0}, {10.0, 0.25, 10.0}};
  largeFloor.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt floorReceipt =
      document.createObject(largeFloor);
  if (!floorReceipt.accepted) {
    return expect(false, "erase large floor setup accepted");
  }

  cr::CreativeDocumentCreateRequest contained;
  contained.kind = cr::CreativeObjectKind::Crate;
  contained.name = "locked contained crate";
  contained.bounds = {{0.1, 0.1, 0.1}, {0.9, 0.9, 0.9}};
  contained.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt containedReceipt =
      document.createObject(contained);
  if (!containedReceipt.accepted) {
    return expect(false, "erase contained object setup accepted");
  }
  cr::CreativeObject* locked =
      document.findObject(containedReceipt.objectId);
  locked->locked = true;
  const std::uint64_t countBeforeReject = document.objectCount();
  const std::uint64_t voxelsBeforeReject =
      document.voxelField().occupiedCellCount();
  const std::uint64_t revisionBeforeReject = document.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Erase, selected));
  bool ok = expect(!rejected.accepted && !rejected.changed,
                   "locked erase rejected") &&
            expect(document.objectCount() == countBeforeReject,
                   "locked erase rolls back removals") &&
            expect(document.voxelField().occupiedCellCount() ==
                       voxelsBeforeReject,
                   "locked erase rolls back voxel removals") &&
            expect(document.revision() == revisionBeforeReject,
                   "locked erase preserves revision");

  locked = document.findObject(containedReceipt.objectId);
  locked->locked = false;
  const cr::CreativeVolumeOperationReceipt erased =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Erase, selected));
  ok = expect(erased.accepted && erased.changed, "erase accepted") &&
       expect(erased.removedObjectCount == 1U,
              "erase removes contained object") &&
       expect(erased.removedVoxelCellCount == 4U,
              "erase removes contained voxel cells") &&
       expect(document.objectCount() == 1U, "erase leaves spanning floor") &&
       expect(document.voxelField().occupiedCellCount() == 0U,
              "erase clears selected voxels") &&
       expect(document.containsObject(floorReceipt.objectId),
              "erase preserves non-contained floor") &&
       ok;
  return ok;
}

bool eraseRejectsParentWithExternalChild() {
  cr::CreativeDocument document = ::document("erase parent");
  cr::CreativeDocumentCreateRequest parentRequest;
  parentRequest.kind = cr::CreativeObjectKind::Group;
  parentRequest.name = "parent";
  parentRequest.transform.position = {0.5, 0.5, 0.5};
  parentRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt parent =
      document.createObject(parentRequest);

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Wall;
  childRequest.name = "child outside";
  childRequest.bounds = {{3.0, 0.0, 0.0}, {4.0, 1.0, 1.0}};
  childRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  if (!parent.accepted || !child.accepted) {
    return expect(false, "erase parent setup accepted");
  }
  document.findObject(child.objectId)->parentId = parent.objectId;

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Erase,
                  selection({0, 0, 0}, {0, 0, 0})));
  return expect(!receipt.accepted && !receipt.changed,
                "external child erase rejected") &&
         expect(receipt.status ==
                        cr::CreativeVolumeOperationStatus::RelationshipRejected &&
                    receipt.reasonCode ==
                        "creative_volume_erase_dependency_rejected" &&
                    receipt.blockedObjectIds.size() == 1U &&
                    receipt.blockedObjectIds.front() == parent.objectId &&
                    receipt.dependentSourceObjectIds.size() == 1U &&
                    receipt.dependentSourceObjectIds.front() == child.objectId,
                "external child erase status") &&
         expect(document.containsObject(parent.objectId) &&
                    document.containsObject(child.objectId),
                "external child erase preserves both objects") &&
         expect(document.revision() == revisionBefore,
                "external child erase preserves revision");
}

bool cloneUsesVolumeWidthOffset() {
  cr::CreativeDocument document = ::document("clone");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 0, 0});
  const cr::CreativeVolumeOperationReceipt fill =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill, selected));
  if (!fill.accepted) {
    return expect(false, "clone setup fill accepted");
  }

  const cr::CreativeVolumeOperationReceipt cloned =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Clone, selected));
  const bool shiftedCellsPresent =
      document.voxelField().occupied({2, 0, 0}) &&
      document.voxelField().occupied({3, 0, 0});

  return expect(cloned.accepted && cloned.changed, "clone accepted") &&
         expect(cloned.matchedVoxelCellCount == 2U,
                "clone matched voxel count") &&
         expect(cloned.createdVoxelCellCount == 2U,
                "clone created voxel count") &&
         expect(document.voxelField().occupiedCellCount() == 4U,
                "clone voxel count") &&
         expect(shiftedCellsPresent, "clone shifts by volume width");
}

bool cloneTransformsVoxelsAndHonorsOverlapPolicies() {
  cr::CreativeDocument transformed = ::document("clone transform");
  const std::array seed{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{0, 0, 2}, cr::CreativeObjectKind::Floor},
  };
  if (!transformed.applyVoxelEdits(seed).accepted) {
    return expect(false, "clone transform seed accepted");
  }
  cr::CreativeVolumeOperationRequest transform = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {1, 0, 2}));
  transform.cloneMemberMask = cr::CreativeVolumeMemberMask::VoxelCells;
  transform.hasCloneOffset = true;
  transform.cloneOffset = {10.0, 0.0, 0.0};
  transform.cloneQuarterTurns = 1U;
  transform.cloneMirrorX = true;
  const std::uint64_t transformRevision = transformed.revision();
  const cr::CreativeVolumeOperationReceipt transformedReceipt =
      cr::executeCreativeVolumeOperation(transformed, transform);
  const auto createdTarget = [&transformedReceipt](cr::CreativeGridCoord3 cell) {
    return std::find(transformedReceipt.createdVoxelCells.begin(),
                     transformedReceipt.createdVoxelCells.end(), cell) !=
           transformedReceipt.createdVoxelCells.end();
  };
  bool ok = expect(transformedReceipt.accepted && transformedReceipt.changed &&
                       transformedReceipt.createdVoxelCellCount == 3U &&
                       transformed.revision() == transformRevision + 1U,
                   "clone applies exact quarter-turn mirror transform once") &&
            expect(transformedReceipt.createdVoxelCells.size() == 3U &&
                       createdTarget({10, 0, 0}) &&
                       createdTarget({12, 0, 0}) &&
                       createdTarget({10, 0, 1}),
                   "clone receipt owns exact transformed target cells") &&
            expect(transformed.voxelField().materialAt({10, 0, 0}) ==
                           cr::CreativeObjectKind::Wall &&
                       transformed.voxelField().materialAt({10, 0, 1}) ==
                           cr::CreativeObjectKind::Floor &&
                       transformed.voxelField().materialAt({12, 0, 0}) ==
                           cr::CreativeObjectKind::Floor,
                   "clone preserves material identity at exact targets");

  cr::CreativeDocument overlap = ::document("clone overlap");
  const std::array overlapSeed{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{2, 0, 0}, cr::CreativeObjectKind::Floor},
  };
  if (!overlap.applyVoxelEdits(overlapSeed).accepted) {
    return expect(false, "clone overlap seed accepted");
  }
  cr::CreativeVolumeOperationRequest overlapRequest = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {0, 0, 0}));
  overlapRequest.cloneMemberMask = cr::CreativeVolumeMemberMask::VoxelCells;
  overlapRequest.hasCloneOffset = true;
  overlapRequest.cloneOffset = {2.0, 0.0, 0.0};
  const std::uint64_t overlapRevision = overlap.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(overlap, overlapRequest);
  overlapRequest.cloneVoxelOverlapPolicy =
      cr::CreativeVolumeCloneVoxelOverlapPolicy::PreserveExisting;
  const cr::CreativeVolumeOperationReceipt preserved =
      cr::executeCreativeVolumeOperation(overlap, overlapRequest);
  overlapRequest.cloneVoxelOverlapPolicy =
      cr::CreativeVolumeCloneVoxelOverlapPolicy::ReplaceExisting;
  const cr::CreativeVolumeOperationReceipt replaced =
      cr::executeCreativeVolumeOperation(overlap, overlapRequest);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.reasonCode ==
                        "creative_volume_clone_voxel_occupied" &&
                    rejected.blockedVoxelCells ==
                        std::vector<cr::CreativeGridCoord3>{{2, 0, 0}} &&
                    overlapRevision == rejected.revisionAfter,
                "reject overlap reports exact occupied targets atomically") &&
         expect(preserved.accepted && !preserved.changed &&
                    preserved.unchangedVoxelCells ==
                        std::vector<cr::CreativeGridCoord3>{{2, 0, 0}} &&
                    overlap.revision() == overlapRevision + 1U,
                "preserve overlap skips occupied targets") &&
         expect(replaced.accepted && replaced.changed &&
                    replaced.replacedVoxelCellCount == 1U &&
                    overlap.voxelField().materialAt({2, 0, 0}) ==
                        cr::CreativeObjectKind::Wall,
                "replace overlap changes occupied target material") &&
         ok;
}

bool cloneRemapsHierarchyAndLogicAtomically() {
  cr::CreativeDocument document = ::document("clone relationships");
  const cr::CreativeDocumentCreateReceipt parent = createPositionedObject(
      document, cr::CreativeObjectKind::Group, "Group", {0.5, 0.5, 0.5});
  const cr::CreativeDocumentCreateReceipt source = createPositionedObject(
      document, cr::CreativeObjectKind::Switch, "Switch",
      {0.5, 0.5, 0.5});
  const cr::CreativeDocumentCreateReceipt target = createBoundedObject(
      document, cr::CreativeObjectKind::Door, "Door",
      {{1.1, 0.1, 0.1}, {1.9, 0.9, 0.9}}, parent.objectId);
  const cr::CreativeLogicLinkMutationReceipt linked = document.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Open});
  if (!parent.accepted || !source.accepted || !target.accepted ||
      !linked.accepted) {
    return expect(false, "clone relationship seed accepted");
  }

  cr::CreativeVolumeOperationRequest clone = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {1, 0, 0}));
  clone.cloneMemberMask = cr::CreativeVolumeMemberMask::DocumentObjects;
  clone.hasCloneOffset = true;
  clone.cloneOffset = {4.0, 0.0, 0.0};
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(document, clone);
  const cr::CreativeObjectId clonedSource =
      clonedObjectId(receipt, source.objectId);
  const cr::CreativeObjectId clonedTarget =
      clonedObjectId(receipt, target.objectId);
  const cr::CreativeObjectId clonedParent =
      clonedObjectId(receipt, parent.objectId);
  const cr::CreativeObject* clonedDoor = document.findObject(clonedTarget);
  const cr::CreativeLogicLink* clonedLink =
      document.findLogicLink(clonedSource, clonedTarget);
  return expect(receipt.accepted && receipt.changed &&
                    receipt.createdObjectCount == 3U &&
                    receipt.clonedObjectIdRemaps.size() == 3U &&
                    receipt.clonedLogicLinkCount == 1U &&
                    document.revision() == revisionBefore + 1U,
                "clone publishes hierarchy and logic in one revision") &&
         expect(clonedSource != cr::kInvalidObjectId &&
                    clonedTarget != cr::kInvalidObjectId &&
                    clonedDoor != nullptr && clonedDoor->parentId.has_value() &&
                    *clonedDoor->parentId == clonedParent,
                "clone remaps internal hierarchy") &&
         expect(clonedLink != nullptr &&
                    clonedLink->action == cr::CreativeLogicLinkAction::Open,
                "clone remaps internal logic link");
}

bool clonePreservesPatternRecipesAndRejectsUnsupportedTransforms() {
  cr::CreativeDocument document = ::document("clone pattern");
  const cr::CreativeDocumentCreateReceipt source = createBoundedObject(
      document, cr::CreativeObjectKind::Wall, "Pattern source",
      {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}});
  cr::CreativeLinearArrayRequest arrayRequest;
  arrayRequest.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  arrayRequest.spacing = cr::CreativeLinearArraySpacing::OneCell;
  const cr::CreativeLinearArrayReceipt array =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&source.objectId, 1U}, arrayRequest);
  if (!source.accepted || !array.accepted ||
      array.generatedObjectIds().size() != 2U) {
    return expect(false, "clone pattern seed accepted");
  }

  cr::CreativeVolumeOperationRequest clone = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {2, 0, 0}));
  clone.cloneMemberMask = cr::CreativeVolumeMemberMask::DocumentObjects;
  clone.hasCloneOffset = true;
  clone.cloneOffset = {5.0, 0.0, 0.0};
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt cloned =
      cr::executeCreativeVolumeOperation(document, clone);
  const cr::CreativePatternRecipe* clonedRecipe =
      cloned.clonedPatternRecipeIdRemaps.empty()
          ? nullptr
          : cr::findCreativePatternRecipe(
                document.patternRecipeStore(),
                cloned.clonedPatternRecipeIdRemaps.front().clonedRecipeId);
  bool ok = expect(cloned.accepted && cloned.changed &&
                       cloned.matchedObjectCount == 3U &&
                       cloned.createdObjectCount == 3U &&
                       cloned.clonedPatternRecipeCount == 1U &&
                       cloned.clonedPatternRecipeIdRemaps.size() == 1U &&
                       clonedRecipe != nullptr &&
                       clonedRecipe->sourceObjectIds.size() == 1U &&
                       clonedRecipe->generatedObjectIds.size() == 2U &&
                       document.revision() == revisionBefore + 1U,
                   "clone preserves complete editable pattern recipe") &&
            expect(clonedObjectId(cloned, source.objectId) ==
                           clonedRecipe->sourceObjectIds.front() &&
                       clonedObjectId(cloned,
                                      array.generatedObjectIds().front()) ==
                           clonedRecipe->generatedObjectIds.front(),
                   "clone recipe references remapped object ids");

  cr::CreativeVolumeOperationRequest rotated = clone;
  rotated.cloneQuarterTurns = 1U;
  const std::uint64_t rotatedRevision = document.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(document, rotated);
  cr::CreativeVolumeOperationRequest limited = clone;
  limited.selection = selection({1, 0, 0}, {1, 0, 0});
  limited.maxAffectedObjects = 2U;
  const cr::CreativeVolumeOperationReceipt limitRejected =
      cr::executeCreativeVolumeOperation(document, limited);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.reasonCode ==
                        "creative_volume_clone_pattern_transform_unsupported" &&
                    document.revision() == rotatedRevision,
                "pattern rotation rejects explicitly without mutation") &&
         expect(!limitRejected.accepted && !limitRejected.changed &&
                    limitRejected.reasonCode ==
                        "creative_volume_clone_limit_exceeded" &&
                    limitRejected.matchedObjectCount == 3U &&
                    document.revision() == rotatedRevision,
                "clone limit counts expanded semantic closure") &&
         ok;
}

bool cloneRejectsExternalReferencesAndProtectsGeneratedSources() {
  cr::CreativeDocument external = ::document("clone external");
  cr::CreativeDocumentCreateRequest parentRequest;
  parentRequest.kind = cr::CreativeObjectKind::Group;
  parentRequest.name = "Inside parent";
  parentRequest.transform.position = {0.5, 0.5, 0.5};
  parentRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt parent =
      external.createObject(parentRequest);
  const cr::CreativeDocumentCreateReceipt child = createBoundedObject(
      external, cr::CreativeObjectKind::Wall, "Outside child",
      {{3.0, 0.0, 0.0}, {4.0, 1.0, 1.0}}, parent.objectId);
  const cr::CreativeDocumentCreateReceipt source = createPositionedObject(
      external, cr::CreativeObjectKind::Switch, "Inside switch",
      {0.5, 0.5, 0.5});
  const cr::CreativeDocumentCreateReceipt target = createBoundedObject(
      external, cr::CreativeObjectKind::Door, "Outside door",
      {{4.0, 0.0, 0.0}, {5.0, 2.0, 1.0}});
  const cr::CreativeLogicLinkMutationReceipt linked = external.setLogicLink(
      {source.objectId, target.objectId, cr::CreativeLogicLinkAction::Open});
  if (!parent.accepted || !child.accepted || !source.accepted ||
      !target.accepted || !linked.accepted) {
    return expect(false, "clone external seed accepted");
  }
  cr::CreativeVolumeOperationRequest clone = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {0, 0, 0}));
  clone.cloneMemberMask = cr::CreativeVolumeMemberMask::DocumentObjects;
  clone.hasCloneOffset = true;
  clone.cloneOffset = {8.0, 0.0, 0.0};
  const std::uint64_t externalRevision = external.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(external, clone);

  cr::CreativeDocument generated = ::document("clone generated");
  const cr::CreativeDocumentCreateReceipt owned = createBoundedObject(
      generated, cr::CreativeObjectKind::Wall, "Generated wall",
      {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}}, std::nullopt,
      {"creative_world_layout:building_1"});
  const std::uint64_t generatedRevision = generated.revision();
  const cr::CreativeVolumeOperationReceipt protectedReceipt =
      cr::executeCreativeVolumeOperation(generated, clone);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.reasonCode ==
                        "creative_volume_clone_external_reference_rejected" &&
                    rejected.blockedObjectIds.size() == 2U &&
                    rejected.dependentSourceObjectIds.size() == 2U &&
                    external.revision() == externalRevision &&
                    external.objectCount() == 4U,
                "clone rejects crossing hierarchy and logic references") &&
         expect(owned.accepted && protectedReceipt.accepted &&
                    !protectedReceipt.changed &&
                    protectedReceipt.reasonCode ==
                        "creative_volume_clone_source_owned" &&
                    protectedReceipt.protectedObjectIds ==
                        std::vector<cr::CreativeObjectId>{owned.objectId} &&
                    generated.revision() == generatedRevision &&
                    generated.objectCount() == 1U,
                "clone protects world-layout generated output");
}

bool unsupportedBrushRejects() {
  cr::CreativeDocument document = ::document("unsupported");
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {0, 0, 0}),
                  cr::CreativeObjectKind::SpawnPoint));
  return expect(!receipt.accepted, "point brush rejected") &&
         expect(receipt.status ==
                    cr::CreativeVolumeOperationStatus::UnsupportedObjectKind,
                "point brush status") &&
         expect(document.objectCount() == 0U,
                "point brush leaves document unchanged");
}

bool volumeBrushCycleSkipsUnsupportedKinds() {
  constexpr std::array palette{
      cr::CreativeObjectKind::SpawnPoint,
      cr::CreativeObjectKind::Wall,
      cr::CreativeObjectKind::PatrolRoute,
      cr::CreativeObjectKind::Floor,
  };
  return expect(cr::firstCreativeVolumeBrush(palette) ==
                    cr::CreativeObjectKind::Wall,
                "volume brush first skips point") &&
         expect(cr::nextCreativeVolumeBrush(
                    palette, cr::CreativeObjectKind::Wall) ==
                    cr::CreativeObjectKind::Floor,
                "volume brush next skips path") &&
         expect(cr::nextCreativeVolumeBrush(
                    palette, cr::CreativeObjectKind::Floor) ==
                    cr::CreativeObjectKind::Wall,
                "volume brush wraps") &&
         expect(!cr::creativeVolumeBrushSupported(
                    cr::CreativeObjectKind::SpawnPoint),
                "point is not a volume brush") &&
         expect(!cr::creativeVolumeBrushSupported(
                    cr::CreativeObjectKind::Crate),
                "authored props cannot enter voxel storage through volume tools");
}

bool toolSettingsMapAtomicallyToVolumeRequests() {
  cr::CreativeToolSettings settings =
      cr::makeDefaultCreativeToolSettings();
  cr::CreativeVolumeOperationRequest replace = request(
      cr::CreativeVolumeOperationKind::Replace,
      selection({0, 0, 0}, {1, 0, 0}));
  replace.hasReplaceKindFilter = true;
  replace.replaceKindFilter = cr::CreativeObjectKind::Wall;

  bool ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(replace,
                                                                 settings),
                   "replace any settings map") &&
            expect(!replace.hasReplaceKindFilter &&
                       replace.replaceKindFilter ==
                           cr::CreativeObjectKind::Unknown,
                   "replace any clears source filter");

  cr::CreativeVolumeOperationRequest fill = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {4, 2, 4}));
  settings.shapeBrushKind = cr::CreativeShapeBrushKind::Cylinder;
  settings.shapeBrushAxis = cr::CreativeShapeBrushAxis::Z;
  settings.volumeFillOverlapPolicy =
      cr::CreativeVolumeFillOverlapPolicy::ReplaceExisting;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(fill, settings),
              "shape settings map to fill") &&
       expect(fill.shapeKind == cr::CreativeShapeBrushKind::Cylinder &&
                  fill.shapeAxis == cr::CreativeShapeBrushAxis::Z &&
                  fill.fillOverlapPolicy ==
                      cr::CreativeVolumeFillOverlapPolicy::ReplaceExisting,
              "fill request owns shape axis and overlap policy") &&
       ok;

  cr::CreativeVolumeOperationRequest hollow = request(
      cr::CreativeVolumeOperationKind::Hollow,
      selection({0, 0, 0}, {4, 4, 4}));
  settings.volumeHollowThickness =
      cr::CreativeVolumeHollowThickness::FourCells;
  settings.volumeHollowAlignment =
      cr::CreativeVolumeHollowAlignment::Outward;
  settings.volumeHollowOpening =
      cr::CreativeVolumeHollowOpening::BothEnds;
  settings.volumeHollowCornerRule =
      cr::CreativeVolumeHollowCornerRule::CutThrough;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(hollow, settings),
              "hollow settings map atomically") &&
       expect(hollow.shapeKind == cr::CreativeShapeBrushKind::Cylinder &&
                  hollow.shapeAxis == cr::CreativeShapeBrushAxis::Z &&
                  hollow.hollowThickness ==
                      cr::CreativeVolumeHollowThickness::FourCells &&
                  hollow.hollowAlignment ==
                      cr::CreativeVolumeHollowAlignment::Outward &&
                  hollow.hollowOpening ==
                      cr::CreativeVolumeHollowOpening::BothEnds &&
                  hollow.hollowCornerRule ==
                      cr::CreativeVolumeHollowCornerRule::CutThrough,
              "hollow request preserves shape shell and opening parameters") &&
       ok;

  settings.replaceSourceKind = cr::CreativeObjectKind::Wall;
  settings.volumeReplaceMemberMask =
      cr::CreativeVolumeMemberMask::DocumentObjects;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(replace, settings),
              "replace source settings map") &&
       expect(replace.hasReplaceKindFilter &&
                  replace.replaceKindFilter == cr::CreativeObjectKind::Wall &&
                  replace.replaceMemberMask ==
                      cr::CreativeVolumeMemberMask::DocumentObjects,
              "replace source and member mask become request filters") &&
       ok;

  cr::CreativeVolumeOperationRequest erase = request(
      cr::CreativeVolumeOperationKind::Erase,
      selection({0, 0, 0}, {1, 0, 0}));
  settings.eraseSourceKind = cr::CreativeObjectKind::Floor;
  settings.volumeEraseMemberMask =
      cr::CreativeVolumeMemberMask::VoxelCells;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(erase, settings),
              "erase settings map") &&
       expect(erase.hasEraseKindFilter &&
                  erase.eraseKindFilter == cr::CreativeObjectKind::Floor &&
                  erase.eraseMemberMask ==
                      cr::CreativeVolumeMemberMask::VoxelCells,
              "erase request owns source and member filters") &&
       ok;

  cr::CreativeVolumeOperationRequest clone = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {0, 0, 0}, 0.5));
  settings.cloneOffsetAxis = cr::CreativeCloneOffsetAxis::Z;
  settings.cloneOffsetDistance =
      cr::CreativeCloneOffsetDistance::FourCells;
  settings.cloneRotation = cr::CreativeCloneRotation::Degrees270;
  settings.cloneMirror = cr::CreativeCloneMirror::XAndZ;
  settings.volumeCloneMemberMask =
      cr::CreativeVolumeMemberMask::DocumentObjects;
  settings.cloneVoxelOverlapPolicy =
      cr::CreativeVolumeCloneVoxelOverlapPolicy::ReplaceExisting;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(clone, settings),
              "clone settings map") &&
       expect(clone.hasCloneOffset && clone.cloneOffset.x == 0.0 &&
                  clone.cloneOffset.y == 0.0 && clone.cloneOffset.z == 2.0 &&
                  clone.cloneQuarterTurns == 3U && clone.cloneMirrorX &&
                  clone.cloneMirrorZ &&
                  clone.cloneMemberMask ==
                      cr::CreativeVolumeMemberMask::DocumentObjects &&
                  clone.cloneVoxelOverlapPolicy ==
                      cr::CreativeVolumeCloneVoxelOverlapPolicy::ReplaceExisting,
              "clone request owns offset transform members and overlap") &&
       ok;

  cr::CreativeVolumeOperationRequest invalidScale = clone;
  invalidScale.selection.cellSize = 0.0;
  invalidScale.hasCloneOffset = false;
  invalidScale.cloneOffset = {3.0, 4.0, 5.0};
  const cr::CreativeVolumeOperationRequest beforeInvalidScale = invalidScale;
  ok = expect(!cr::applyCreativeToolSettingsToVolumeRequest(invalidScale,
                                                              settings),
              "invalid clone scale rejected") &&
       expect(invalidScale.hasCloneOffset ==
                  beforeInvalidScale.hasCloneOffset &&
                  invalidScale.cloneOffset.x ==
                      beforeInvalidScale.cloneOffset.x &&
                  invalidScale.cloneOffset.y ==
                      beforeInvalidScale.cloneOffset.y &&
                  invalidScale.cloneOffset.z ==
                      beforeInvalidScale.cloneOffset.z,
              "invalid clone scale leaves request unchanged") &&
       ok;

  cr::CreativeVolumeOperationRequest unchanged = clone;
  unchanged.hasCloneOffset = false;
  unchanged.cloneOffset = {7.0, 8.0, 9.0};
  const cr::CreativeVolumeOperationRequest before = unchanged;
  settings.cloneOffsetAxis = cr::CreativeCloneOffsetAxis::Count;
  ok = expect(!cr::applyCreativeToolSettingsToVolumeRequest(unchanged,
                                                              settings),
              "invalid settings rejected") &&
       expect(unchanged.hasCloneOffset == before.hasCloneOffset &&
                  unchanged.cloneOffset.x == before.cloneOffset.x &&
                  unchanged.cloneOffset.y == before.cloneOffset.y &&
                  unchanged.cloneOffset.z == before.cloneOffset.z &&
                  unchanged.maxAffectedObjects == before.maxAffectedObjects,
              "invalid settings leave request unchanged") &&
       ok;

  settings = cr::makeDefaultCreativeToolSettings();
  unchanged.operation = cr::CreativeVolumeOperationKind::Count;
  const cr::CreativeVolumeOperationRequest invalidOperation = unchanged;
  ok = expect(!cr::applyCreativeToolSettingsToVolumeRequest(unchanged,
                                                              settings),
              "invalid operation rejected") &&
       expect(unchanged.operation == invalidOperation.operation &&
                  unchanged.cloneOffset.x == invalidOperation.cloneOffset.x,
              "invalid operation leaves request unchanged") &&
       ok;
  return ok;
}

bool facadeSelectionAndHistoryRoundTrip() {
  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt install =
      appState.facade.installDocument(document("facade history"));
  if (!install.accepted) {
    return expect(false, "facade history document installed");
  }

  cr::CreativeDocumentHistoryTransaction transaction =
      cr::beginCreativeHistoryTransaction(appState.facade, "volume_fill");
  const cr::CreativeVolumeOperationReceipt fill =
      appState.facade.applyVolumeOperation(
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {1, 0, 0})));
  const cr::CreativeHistoryRecordReceipt recorded =
      cr::commitCreativeHistoryTransaction(
          appState.history, std::move(transaction), appState.facade);
  bool ok = expect(fill.accepted && fill.createdVoxelCellCount == 2U,
                   "facade fill accepted") &&
            expect(cr::selectedTargetCount(appState.facade.selectionState()) ==
                       0U,
                   "voxel fill does not fabricate object selection") &&
            expect(recorded.recorded, "volume fill records one undo step") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U,
                   "volume fill undo depth one");

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  ok = expect(undo.accepted &&
                  appState.facade.document().voxelField().occupiedCellCount() ==
                      0U,
              "volume fill undo restores empty voxel field") &&
       ok;
  const cr::CreativeHistoryApplyReceipt redo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  ok = expect(redo.accepted &&
                  appState.facade.document().voxelField().occupiedCellCount() ==
                      2U,
              "volume fill redo restores voxel cells") &&
       ok;
  return ok;
}

bool requestFingerprintIsExactAndFailClosed() {
  const cr::CreativeVolumeOperationRequest baseline =
      request(cr::CreativeVolumeOperationKind::Clone,
              selection({-2, 1, 3}, {4, 5, 6}));
  const std::uint64_t baselineFingerprint =
      cr::fingerprintCreativeVolumeOperationRequest(baseline);
  bool ok = expect(baselineFingerprint != 0U,
                   "valid volume request has stable fingerprint");

  const auto expectDifferent = [&](auto mutate, std::string_view field) {
    cr::CreativeVolumeOperationRequest changed = baseline;
    mutate(changed);
    return expect(cr::fingerprintCreativeVolumeOperationRequest(changed) !=
                      baselineFingerprint,
                  field);
  };
  ok = expectDifferent(
           [](auto& value) {
             value.operation = cr::CreativeVolumeOperationKind::Fill;
           },
           "fingerprint includes operation") &&
       expectDifferent([](auto& value) { ++value.selection.firstCell.x; },
                       "fingerprint includes first cell") &&
       expectDifferent([](auto& value) { ++value.selection.secondCell.z; },
                       "fingerprint includes second cell") &&
       expectDifferent([](auto& value) { value.selection.origin.x = 0.5; },
                       "fingerprint includes grid origin") &&
       expectDifferent([](auto& value) { value.selection.cellSize = 0.5; },
                       "fingerprint includes cell size") &&
       expectDifferent(
           [](auto& value) { value.objectKind = cr::CreativeObjectKind::Floor; },
           "fingerprint includes material") &&
       expectDifferent(
           [](auto& value) {
             value.shapeKind = cr::CreativeShapeBrushKind::Ellipsoid;
           },
           "fingerprint includes shape") &&
       expectDifferent(
           [](auto& value) { value.shapeAxis = cr::CreativeShapeBrushAxis::Z; },
           "fingerprint includes shape axis") &&
       expectDifferent(
           [](auto& value) {
             value.fillOverlapPolicy =
                 cr::CreativeVolumeFillOverlapPolicy::ReplaceExisting;
           },
           "fingerprint includes fill overlap") &&
       expectDifferent(
           [](auto& value) {
             value.hollowThickness =
                 cr::CreativeVolumeHollowThickness::TwoCells;
           },
           "fingerprint includes shell thickness") &&
       expectDifferent(
           [](auto& value) {
             value.hollowAlignment =
                 cr::CreativeVolumeHollowAlignment::Outward;
           },
           "fingerprint includes shell alignment") &&
       expectDifferent(
           [](auto& value) {
             value.hollowOpening =
                 cr::CreativeVolumeHollowOpening::PositiveEnd;
           },
           "fingerprint includes shell opening") &&
       expectDifferent(
           [](auto& value) {
             value.hollowCornerRule =
                 cr::CreativeVolumeHollowCornerRule::CutThrough;
           },
           "fingerprint includes shell corner rule") &&
       expectDifferent([](auto& value) { value.hasReplaceKindFilter = true; },
                       "fingerprint includes replace filter presence") &&
       expectDifferent(
           [](auto& value) {
             value.replaceKindFilter = cr::CreativeObjectKind::Floor;
           },
           "fingerprint includes replace filter") &&
       expectDifferent(
           [](auto& value) {
             value.replaceMemberMask =
                 cr::CreativeVolumeMemberMask::VoxelCells;
           },
           "fingerprint includes replace member mask") &&
       expectDifferent([](auto& value) { value.hasEraseKindFilter = true; },
                       "fingerprint includes erase filter presence") &&
       expectDifferent(
           [](auto& value) {
             value.eraseKindFilter = cr::CreativeObjectKind::Floor;
           },
           "fingerprint includes erase filter") &&
       expectDifferent(
           [](auto& value) {
             value.eraseMemberMask =
                 cr::CreativeVolumeMemberMask::DocumentObjects;
           },
           "fingerprint includes erase member mask") &&
       expectDifferent([](auto& value) { value.hasCloneOffset = true; },
                       "fingerprint includes clone offset presence") &&
       expectDifferent([](auto& value) { value.cloneOffset.x = 2.0; },
                       "fingerprint includes clone offset") &&
       expectDifferent([](auto& value) { value.cloneQuarterTurns = 1U; },
                       "fingerprint includes clone rotation") &&
       expectDifferent([](auto& value) { value.cloneMirrorX = true; },
                       "fingerprint includes clone X mirror") &&
       expectDifferent([](auto& value) { value.cloneMirrorZ = true; },
                       "fingerprint includes clone Z mirror") &&
       expectDifferent(
           [](auto& value) {
             value.cloneMemberMask =
                 cr::CreativeVolumeMemberMask::VoxelCells;
           },
           "fingerprint includes clone member mask") &&
       expectDifferent(
           [](auto& value) {
             value.cloneVoxelOverlapPolicy =
                 cr::CreativeVolumeCloneVoxelOverlapPolicy::ReplaceExisting;
           },
           "fingerprint includes clone overlap") &&
       expectDifferent([](auto& value) { --value.maxAffectedObjects; },
                       "fingerprint includes operation limit") &&
       ok;

  cr::CreativeVolumeOperationRequest signedZero = baseline;
  signedZero.selection.origin.x = -0.0;
  ok = expect(cr::fingerprintCreativeVolumeOperationRequest(signedZero) ==
                  baselineFingerprint,
              "fingerprint canonicalizes signed zero") &&
       ok;
  cr::CreativeVolumeOperationRequest nonFinite = baseline;
  nonFinite.cloneOffset.y = std::numeric_limits<double>::quiet_NaN();
  return expect(cr::fingerprintCreativeVolumeOperationRequest(nonFinite) == 0U,
                "fingerprint rejects non-finite request geometry") &&
         ok;
}

bool directCornerSelectionAndExpansion() {
  cr::CreativeVolumeSelection selected;
  bool ok = expect(
                cr::setCreativeVolumeSelectionCorner(
                    selected, cr::CreativeVolumeCorner::First, {2, 3, 4}) ==
                    cr::CreativeVolumeSelectionPhase::FirstCorner,
                "direct first corner starts selection") &&
            expect(selected.firstCell.x == 2 && selected.firstCell.y == 3 &&
                       selected.firstCell.z == 4,
                   "direct first corner stored") &&
            expect(cr::setCreativeVolumeSelectionCorner(
                       selected, cr::CreativeVolumeCorner::Second,
                       {-1, 5, 6}) ==
                       cr::CreativeVolumeSelectionPhase::Complete,
                   "direct second corner completes selection") &&
            expect(cr::creativeVolumeCellCount(selected) == 36U,
                   "direct corners define canonical volume");

  ok = expect(cr::expandCreativeVolumeSelectionToCell(selected, {4, 1, 3}),
              "pick expansion grows selection") &&
       expect(selected.firstCell.x == -1 && selected.firstCell.y == 1 &&
                  selected.firstCell.z == 3 && selected.secondCell.x == 4 &&
                  selected.secondCell.y == 5 && selected.secondCell.z == 6,
              "pick expansion stores canonical inclusive corners") &&
       expect(!cr::expandCreativeVolumeSelectionToCell(selected, {0, 2, 4}),
              "contained pick does not mutate selection") &&
       ok;
  return ok;
}

bool regionEditingIsCanonicalAndOverflowSafe() {
  cr::CreativeVolumeSelection selected =
      selection({1, 2, 3}, {3, 4, 5});
  const cr::CreativeVolumeRegionFacts initial =
      cr::inspectCreativeVolumeRegion(selected);
  bool ok = expect(initial.valid && initial.exclusiveBounds.min.x == 1 &&
                       initial.exclusiveBounds.max.x == 4 &&
                       initial.inclusiveMaximum.y == 4 &&
                       initial.dimensions.x == 3 && initial.dimensions.y == 3 &&
                       initial.dimensions.z == 3 && initial.cellCount == 27U,
                   "region facts distinguish inclusive and exclusive bounds");

  ok = expect(cr::resizeCreativeVolumeSelectionFace(
                  selected, cr::CreativeVolumeFace::NegativeX, 2),
              "negative face expands outwards") &&
       expect(cr::creativeVolumeGridBounds(selected).min.x == -1 &&
                  cr::creativeVolumeGridBounds(selected).max.x == 4,
              "negative face expansion changes only minimum") &&
       expect(cr::moveCreativeVolumeSelection(selected, {10, -1, 2}),
              "region moves on all three axes") &&
       expect(cr::creativeVolumeGridBounds(selected).min.x == 9 &&
                  cr::creativeVolumeGridBounds(selected).min.y == 1 &&
                  cr::creativeVolumeGridBounds(selected).min.z == 5,
              "region move preserves dimensions") &&
       ok;

  const cr::CreativeVolumeSelection beforeInvalid = selected;
  ok = expect(!cr::setCreativeVolumeSelectionGridBounds(
                  selected, {{4, 2, 3}, {4, 7, 8}}),
              "zero-width numeric bounds reject") &&
       expect(selected.firstCell.x == beforeInvalid.firstCell.x &&
                  selected.secondCell.z == beforeInvalid.secondCell.z,
              "invalid numeric bounds leave selection unchanged") &&
       ok;

  cr::CreativeVolumeSelection single = selection({0, 0, 0}, {0, 0, 0});
  ok = expect(!cr::resizeCreativeVolumeSelectionFace(
                  single, cr::CreativeVolumeFace::PositiveX, -1),
              "face contraction cannot collapse an axis") &&
       expect(cr::creativeVolumeCellCount(single) == 1U,
              "failed contraction preserves one-cell region") &&
       ok;

  cr::CreativeVolumeSelection edge;
  edge.cellSize = 1.0;
  ok = expect(cr::setCreativeVolumeSelectionGridBounds(
                  edge,
                  {{std::numeric_limits<std::int32_t>::max() - 2, 0, 0},
                   {std::numeric_limits<std::int32_t>::max(), 1, 1}}),
              "near-limit region is representable") &&
       expect(!cr::moveCreativeVolumeSelection(edge, {1, 0, 0}),
              "overflowing move rejects") &&
       expect(cr::creativeVolumeGridBounds(edge).max.x ==
                  std::numeric_limits<std::int32_t>::max(),
              "overflowing move leaves edge region unchanged") &&
       ok;
  return ok;
}

bool fitAndPreviewUseExactDocumentTruth() {
  cr::CreativeDocument fitDocument = document("fit region");
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Wall;
  create.name = "offset wall";
  create.bounds = {{1.25, -0.1, 3.0}, {3.0, 2.1, 5.75}};
  create.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      fitDocument.createObject(create);
  cr::CreativeVolumeSelection fitted;
  const std::array ids{created.objectId};
  const bool fit = cr::fitCreativeVolumeSelectionToObjects(
      fitDocument, ids, 1.0, {}, fitted);
  const cr::CreativeGridBounds3 fittedBounds =
      cr::creativeVolumeGridBounds(fitted);
  bool ok = expect(created.accepted && fit,
                   "selection fits existing content") &&
            expect(fittedBounds.min.x == 1 && fittedBounds.min.y == -1 &&
                       fittedBounds.min.z == 3 && fittedBounds.max.x == 3 &&
                       fittedBounds.max.y == 3 && fittedBounds.max.z == 6,
                   "fit rounds outward to canonical grid lines");

  const cr::CreativeVolumeSelection selected =
      selection({0, 0, 0}, {1, 0, 0});
  cr::CreativeDocument operationDocument = document("exact preview");
  const cr::CreativeVolumeOperationRequest fill =
      request(cr::CreativeVolumeOperationKind::Fill, selected);
  const cr::CreativeVolumeOperationReceipt fillPreview =
      cr::previewCreativeVolumeOperation(operationDocument, fill);
  ok = expect(fillPreview.accepted && fillPreview.changed &&
                  cr::creativeVolumeChangedMemberCount(fillPreview) == 2U,
              "fill preview reports exact changed cells") &&
       expect(operationDocument.voxelField().occupiedCellCount() == 0U &&
                  operationDocument.revision() == 0U,
              "staged preview never mutates source document") &&
       ok;
  const cr::CreativeVolumeOperationReceipt fillApplied =
      cr::executeCreativeVolumeOperation(operationDocument, fill);
  ok = expect(fillApplied.accepted &&
                  cr::creativeVolumeChangedMemberCount(fillApplied) ==
                      cr::creativeVolumeChangedMemberCount(fillPreview),
              "fill preview and execution counts match") &&
       ok;

  cr::CreativeVolumeOperationRequest replace =
      request(cr::CreativeVolumeOperationKind::Replace, selected,
              cr::CreativeObjectKind::Floor);
  replace.hasReplaceKindFilter = true;
  replace.replaceKindFilter = cr::CreativeObjectKind::Wall;
  const cr::CreativeVolumeOperationReceipt replacePreview =
      cr::previewCreativeVolumeOperation(operationDocument, replace);
  ok = expect(replacePreview.accepted && replacePreview.changed &&
                  replacePreview.replacedVoxelCellCount == 2U &&
                  cr::creativeVolumeChangedMemberCount(replacePreview) == 2U,
              "replace preview applies source filter exactly") &&
       expect(operationDocument.voxelField().materialAt({0, 0, 0}) ==
                  cr::CreativeObjectKind::Wall,
              "replace preview leaves source material unchanged") &&
       ok;

  cr::CreativeVolumeOperationRequest clone =
      request(cr::CreativeVolumeOperationKind::Clone, selected);
  clone.hasCloneOffset = true;
  clone.cloneOffset = {2.0, 0.0, 0.0};
  const cr::CreativeVolumeOperationReceipt clonePreview =
      cr::previewCreativeVolumeOperation(operationDocument, clone);
  ok = expect(clonePreview.accepted && clonePreview.changed &&
                  cr::creativeVolumeChangedMemberCount(clonePreview) == 2U,
              "clone preview reports exact target changes") &&
       expect(operationDocument.voxelField().occupiedCellCount() == 2U,
              "clone preview leaves target cells unstaged in source") &&
       ok;

  const cr::CreativeVolumeOperationRequest erase =
      request(cr::CreativeVolumeOperationKind::Erase, selected);
  const cr::CreativeVolumeOperationReceipt erasePreview =
      cr::previewCreativeVolumeOperation(operationDocument, erase);
  return expect(erasePreview.accepted && erasePreview.changed &&
                    cr::creativeVolumeChangedMemberCount(erasePreview) == 2U,
                "erase preview reports exact deletion count") &&
         ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = canonicalSelectionAndPreview() && ok;
  ok = fillIsAtomicAndIdempotent() && ok;
  ok = fillOverlapPolicyIsExplicitAndAtomic() && ok;
  ok = hollowCreatesOnlyShell() && ok;
  ok = hollowThicknessAlignmentAndBoundsAreExact() && ok;
  ok = shapedFillAndHollowUseSharedPlanner() && ok;
  ok = operationLimitRejectsWithoutMutation() && ok;
  ok = maximumInteractiveSolidStaysOneChunkCuboid() && ok;
  ok = voxelFillDoesNotConsumeObjectIds() && ok;
  ok = replaceTargetsOnlyVolumeCells() && ok;
  ok = replaceClassifiesMembersAndPreservesLegacyIdentity() && ok;
  ok = eraseFiltersAndProtectsSourceOwnedContent() && ok;
  ok = eraseIsContainedAndRollbackSafe() && ok;
  ok = eraseRejectsParentWithExternalChild() && ok;
  ok = cloneUsesVolumeWidthOffset() && ok;
  ok = cloneTransformsVoxelsAndHonorsOverlapPolicies() && ok;
  ok = cloneRemapsHierarchyAndLogicAtomically() && ok;
  ok = clonePreservesPatternRecipesAndRejectsUnsupportedTransforms() && ok;
  ok = cloneRejectsExternalReferencesAndProtectsGeneratedSources() && ok;
  ok = unsupportedBrushRejects() && ok;
  ok = volumeBrushCycleSkipsUnsupportedKinds() && ok;
  ok = toolSettingsMapAtomicallyToVolumeRequests() && ok;
  ok = facadeSelectionAndHistoryRoundTrip() && ok;
  ok = requestFingerprintIsExactAndFailClosed() && ok;
  ok = directCornerSelectionAndExpansion() && ok;
  ok = regionEditingIsCanonicalAndOverflowSafe() && ok;
  ok = fitAndPreviewUseExactDocumentTruth() && ok;
  return ok ? 0 : 1;
}
