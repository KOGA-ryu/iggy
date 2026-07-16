#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutTransform.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
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

cr::CreativeDocument makeDocument(cr::CreativeDocumentId id) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Layout Test");
  static_cast<void>(document.assignId(id));
  static_cast<void>(document.setGridSettings(
      {{10.0, 1.0, -10.0}, 1.0, {64, 16, 64}}));
  return document;
}

cr::CreativeAppState makeAppState(cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(makeDocument(id)));
  return appState;
}

const cr::CreativeObject* findNamed(const cr::CreativeDocument& document,
                                    std::string_view name) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [name](const cr::CreativeObject& object) { return object.name == name; });
  return found == document.objects().end() ? nullptr : &*found;
}

cr::CreativeWorldLayout smallHouseLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "estate_level_0";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "Small House";
  building.rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  building.rootFootprint = {{0, 0}, {6, 4}};
  building.rootHeightCells = 4U;
  layout.buildings.push_back(building);
  layout.boxes = {
      {0U, cr::CreativeObjectKind::Floor, "floor", "House Floor",
       {{0, 0}, {6, 4}}, 0, 1U},
      {0U, cr::CreativeObjectKind::Roof, "roof", "House Roof",
       {{0, 0}, {6, 4}}, 4, 1U},
  };
  layout.walls = {
      {0U, "north", "North Wall", {0, 0}, {6, 0}, 1, 3U, 0.25},
      {0U, "south", "South Wall", {0, 4}, {6, 4}, 1, 3U, 0.25},
      {0U, "west", "West Wall", {0, 0}, {0, 4}, 1, 3U, 0.25},
      {0U, "east", "East Wall", {6, 0}, {6, 4}, 1, 3U, 0.25},
  };
  cr::CreativeWorldLayoutOpening door;
  door.wallIndex = 1U;
  door.stableKey = "front_door";
  door.name = "Front Door";
  door.centerOffsetCells = 3.0;
  door.widthCells = 2.0;
  door.cutoutHeightCells = 2.25;
  layout.openings.push_back(door);

  cr::CreativeWorldLayoutOpening window;
  window.wallIndex = 3U;
  window.kind = cr::CreativeBuildingOpeningKind::Window;
  window.stableKey = "east_window";
  window.name = "East Window";
  window.centerOffsetCells = 2.0;
  window.widthCells = 1.5;
  window.cutoutBottomCells = 1.0;
  window.cutoutHeightCells = 1.0;
  layout.openings.push_back(window);
  return layout;
}

cr::CreativeWorldLayout transformableBuildingLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "transform_layout";
  layout.buildings.push_back({"building",
                              "Transform Building",
                              cr::CreativeBuildingRootMode::CreateRoom,
                              {{10, 20}, {18, 26}},
                              0,
                              4U,
                              true,
                              {}});
  layout.rooms.push_back(
      {0U, "room", "Transform Room", {{11, 21}, {17, 25}}, 0, 3U, 0.25, 1U});
  layout.boxes.push_back({0U,
                          cr::CreativeObjectKind::Floor,
                          "floor",
                          "Transform Floor",
                          {{10, 20}, {18, 26}},
                          0,
                          1U});
  layout.walls = {
      {0U, "wall_h", "Horizontal Wall", {10, 23}, {18, 23}, 0.5, 3U, 0.25},
      {0U, "wall_v", "Vertical Wall", {14, 20}, {14, 26}, 0.5, 3U, 0.25},
  };

  const auto addRoomDoor =
      [&](std::string key, cr::CreativeWorldLayoutRoomEdge edge, double offset,
          cr::CreativeBuildingOpeningPose pose) {
        cr::CreativeWorldLayoutOpening opening;
        opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
        opening.roomIndex = 0U;
        opening.roomEdge = edge;
        opening.kind = cr::CreativeBuildingOpeningKind::Door;
        opening.pose = pose;
        opening.stableKey = std::move(key);
        opening.name = opening.stableKey;
        opening.centerOffsetCells = offset;
        opening.widthCells = 1.0;
        opening.cutoutHeightCells = 2.0;
        layout.openings.push_back(std::move(opening));
      };
  addRoomDoor("door_n", cr::CreativeWorldLayoutRoomEdge::North, 1.0,
              cr::CreativeBuildingOpeningPose::OpenFromStartPositiveNormal);
  addRoomDoor("door_e", cr::CreativeWorldLayoutRoomEdge::East, 1.0,
              cr::CreativeBuildingOpeningPose::OpenFromStartPositiveNormal);
  addRoomDoor("door_s", cr::CreativeWorldLayoutRoomEdge::South, 2.0,
              cr::CreativeBuildingOpeningPose::OpenFromEndNegativeNormal);
  addRoomDoor("door_w", cr::CreativeWorldLayoutRoomEdge::West, 3.0,
              cr::CreativeBuildingOpeningPose::OpenFromEndPositiveNormal);

  const auto addWallDoor = [&](std::string key, std::size_t wallIndex,
                               double offset,
                               cr::CreativeBuildingOpeningPose pose) {
    cr::CreativeWorldLayoutOpening opening;
    opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
    opening.wallIndex = wallIndex;
    opening.kind = cr::CreativeBuildingOpeningKind::Door;
    opening.pose = pose;
    opening.stableKey = std::move(key);
    opening.name = opening.stableKey;
    opening.centerOffsetCells = offset;
    opening.widthCells = 1.0;
    opening.cutoutHeightCells = 2.0;
    layout.openings.push_back(std::move(opening));
  };
  addWallDoor("door_wall_h", 0U, 2.0,
              cr::CreativeBuildingOpeningPose::OpenFromStartPositiveNormal);
  addWallDoor("door_wall_v", 1U, 4.0,
              cr::CreativeBuildingOpeningPose::OpenFromEndNegativeNormal);

  cr::CreativeWorldLayoutTerrainProfile terrain;
  terrain.stableKey = "unowned_terrain";
  terrain.center = {12, 22};
  layout.terrainProfiles.push_back(terrain);
  return layout;
}

bool sameBuildingTransformGeometry(const cr::CreativeWorldLayout &lhs,
                                   const cr::CreativeWorldLayout &rhs) {
  if (lhs.buildings.size() != rhs.buildings.size() ||
      lhs.rooms.size() != rhs.rooms.size() ||
      lhs.boxes.size() != rhs.boxes.size() ||
      lhs.walls.size() != rhs.walls.size() ||
      lhs.openings.size() != rhs.openings.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.buildings.size(); ++index) {
    if (lhs.buildings[index].stableKey != rhs.buildings[index].stableKey ||
        lhs.buildings[index].rootFootprint.minimum !=
            rhs.buildings[index].rootFootprint.minimum ||
        lhs.buildings[index].rootFootprint.maximum !=
            rhs.buildings[index].rootFootprint.maximum) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < lhs.rooms.size(); ++index) {
    if (lhs.rooms[index].stableKey != rhs.rooms[index].stableKey ||
        lhs.rooms[index].footprint.minimum !=
            rhs.rooms[index].footprint.minimum ||
        lhs.rooms[index].footprint.maximum !=
            rhs.rooms[index].footprint.maximum) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < lhs.boxes.size(); ++index) {
    if (lhs.boxes[index].stableKey != rhs.boxes[index].stableKey ||
        lhs.boxes[index].footprint.minimum !=
            rhs.boxes[index].footprint.minimum ||
        lhs.boxes[index].footprint.maximum !=
            rhs.boxes[index].footprint.maximum) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < lhs.walls.size(); ++index) {
    if (lhs.walls[index].stableKey != rhs.walls[index].stableKey ||
        lhs.walls[index].start != rhs.walls[index].start ||
        lhs.walls[index].end != rhs.walls[index].end) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < lhs.openings.size(); ++index) {
    const cr::CreativeWorldLayoutOpening &left = lhs.openings[index];
    const cr::CreativeWorldLayoutOpening &right = rhs.openings[index];
    if (left.stableKey != right.stableKey || left.hostKind != right.hostKind ||
        left.wallIndex != right.wallIndex ||
        left.roomIndex != right.roomIndex || left.roomEdge != right.roomEdge ||
        left.centerOffsetCells != right.centerOffsetCells ||
        left.pose != right.pose) {
      return false;
    }
  }
  return true;
}

bool buildingTransformPreservesHostedOpeningSemantics() {
  const cr::CreativeWorldLayout source = transformableBuildingLayout();
  const cr::CreativeWorldLayoutBuildingTransformResult right =
      cr::transformCreativeWorldLayoutBuilding(
          source,
          {0U,
           cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  if (!right.accepted) {
    return expect(false, "right building transform accepted");
  }
  const cr::CreativeWorldLayout &layout = right.transformed;
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(makeDocument(205U), layout);
  const auto mirrorX = cr::transformCreativeWorldLayoutBuilding(
      source, {0U, cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX});
  const auto mirrorZ = cr::transformCreativeWorldLayoutBuilding(
      source, {0U, cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ});

  return expect(right.sourceBounds.minimum ==
                        cr::CreativeTerrainCoord2{10, 20} &&
                    right.sourceBounds.maximum ==
                        cr::CreativeTerrainCoord2{18, 26} &&
                    right.transformedBounds.minimum ==
                        cr::CreativeTerrainCoord2{10, 20} &&
                    right.transformedBounds.maximum ==
                        cr::CreativeTerrainCoord2{16, 28},
                "quarter turn keeps the minimum grid line and swaps spans") &&
         expect(
             layout.rooms[0].footprint.minimum ==
                     cr::CreativeTerrainCoord2{11, 21} &&
                 layout.rooms[0].footprint.maximum ==
                     cr::CreativeTerrainCoord2{15, 27} &&
                 layout.walls[0].start == cr::CreativeTerrainCoord2{13, 20} &&
                 layout.walls[0].end == cr::CreativeTerrainCoord2{13, 28} &&
                 layout.walls[1].start == cr::CreativeTerrainCoord2{16, 24} &&
                 layout.walls[1].end == cr::CreativeTerrainCoord2{10, 24},
             "rooms, boxes, and ordered wall endpoints share one transform") &&
         expect(
             layout.openings[0].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::East &&
                 layout.openings[0].centerOffsetCells == 1.0 &&
                 layout.openings[0].pose == cr::CreativeBuildingOpeningPose::
                                                OpenFromStartNegativeNormal &&
                 layout.openings[1].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::South &&
                 layout.openings[1].centerOffsetCells == 3.0 &&
                 layout.openings[1].pose == cr::CreativeBuildingOpeningPose::
                                                OpenFromEndPositiveNormal &&
                 layout.openings[2].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::West &&
                 layout.openings[2].centerOffsetCells == 2.0 &&
                 layout.openings[2].pose == cr::CreativeBuildingOpeningPose::
                                                OpenFromEndPositiveNormal &&
                 layout.openings[3].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::North &&
                 layout.openings[3].centerOffsetCells == 1.0 &&
                 layout.openings[3].pose == cr::CreativeBuildingOpeningPose::
                                                OpenFromStartPositiveNormal,
             "room-edge direction, offset, hinge, and normal remap together") &&
         expect(
             layout.openings[4].centerOffsetCells == 2.0 &&
                 layout.openings[4].pose == cr::CreativeBuildingOpeningPose::
                                                OpenFromStartNegativeNormal &&
                 layout.openings[5].centerOffsetCells == 4.0 &&
                 layout.openings[5].pose ==
                     cr::CreativeBuildingOpeningPose::OpenFromEndNegativeNormal,
             "standalone-wall offsets retain start-to-end parameterization") &&
         expect(
             mirrorX.accepted && mirrorZ.accepted &&
                 mirrorX.transformed.openings[0].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::North &&
                 mirrorX.transformed.openings[0].centerOffsetCells == 5.0 &&
                 mirrorX.transformed.openings[0].pose ==
                     cr::CreativeBuildingOpeningPose::
                         OpenFromEndPositiveNormal &&
                 mirrorX.transformed.openings[1].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::West &&
                 mirrorX.transformed.openings[1].pose ==
                     cr::CreativeBuildingOpeningPose::
                         OpenFromStartNegativeNormal &&
                 mirrorX.transformed.openings[5].pose ==
                     cr::CreativeBuildingOpeningPose::OpenFromEndPositiveNormal,
             "mirror X reverses canonical edges and vertical normals") &&
         expect(mirrorZ.transformed.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::South &&
                    mirrorZ.transformed.openings[0].pose ==
                        cr::CreativeBuildingOpeningPose::
                            OpenFromStartNegativeNormal &&
                    mirrorZ.transformed.openings[1].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    mirrorZ.transformed.openings[1].centerOffsetCells == 3.0 &&
                    mirrorZ.transformed.openings[1].pose ==
                        cr::CreativeBuildingOpeningPose::
                            OpenFromEndPositiveNormal &&
                    mirrorZ.transformed.openings[4].pose ==
                        cr::CreativeBuildingOpeningPose::
                            OpenFromStartNegativeNormal,
                "mirror Z reverses canonical edges and horizontal normals") &&
         expect(layout.terrainProfiles[0].center ==
                    source.terrainProfiles[0].center,
                "unowned terrain remains outside a building transform") &&
         expect(compiled.receipt.accepted,
                "transformed hosted openings remain exact 3D compilable");
}

bool buildingTransformsRoundTripAndRejectOverflow() {
  const cr::CreativeWorldLayout source = transformableBuildingLayout();
  cr::CreativeWorldLayout turned = source;
  for (std::size_t turn = 0U; turn < 4U; ++turn) {
    const cr::CreativeWorldLayoutBuildingTransformResult result =
        cr::transformCreativeWorldLayoutBuilding(
            turned,
            {0U,
             cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
    if (!result.accepted) {
      return expect(false, "each round-trip quarter turn accepted");
    }
    turned = result.transformed;
  }
  const auto mirrorTwice =
      [&](cr::CreativeWorldLayoutBuildingTransformOperation operation) {
        const auto first =
            cr::transformCreativeWorldLayoutBuilding(source, {0U, operation});
        const auto second = cr::transformCreativeWorldLayoutBuilding(
            first.transformed, {0U, operation});
        return first.accepted && second.accepted &&
               sameBuildingTransformGeometry(source, second.transformed);
      };
  const auto right = cr::transformCreativeWorldLayoutBuilding(
      source,
      {0U, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const auto rightThenLeft = cr::transformCreativeWorldLayoutBuilding(
      right.transformed,
      {0U, cr::CreativeWorldLayoutBuildingTransformOperation::RotateLeft90});

  cr::CreativeWorldLayout overflow = source;
  overflow.buildings[0].rootFootprint = {
      {std::numeric_limits<std::int32_t>::max() - 1,
       std::numeric_limits<std::int32_t>::min()},
      {std::numeric_limits<std::int32_t>::max(),
       std::numeric_limits<std::int32_t>::max()}};
  overflow.rooms.clear();
  overflow.boxes.clear();
  overflow.walls.clear();
  overflow.openings.clear();
  const auto rejected = cr::transformCreativeWorldLayoutBuilding(
      overflow,
      {0U, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});

  cr::CreativeWorldLayout malformed = source;
  malformed.rooms[0].footprint.maximum.x =
      malformed.rooms[0].footprint.minimum.x;
  const auto invalidGeometry = cr::transformCreativeWorldLayoutBuilding(
      malformed,
      {0U, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});

  return expect(sameBuildingTransformGeometry(source, turned),
                "four quarter turns restore exact authored geometry") &&
         expect(right.accepted && rightThenLeft.accepted &&
                    sameBuildingTransformGeometry(source,
                                                  rightThenLeft.transformed),
                "left and right quarter turns are exact inverses") &&
         expect(
             mirrorTwice(
                 cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX) &&
                 mirrorTwice(cr::CreativeWorldLayoutBuildingTransformOperation::
                                 MirrorZ),
             "each building mirror is an exact involution") &&
         expect(!rejected.accepted &&
                    rejected.status ==
                        cr::CreativeWorldLayoutBuildingTransformStatus::
                            CoordinateOverflow &&
                    rejected.transformed.buildings.empty(),
                "coordinate overflow rejects without a partial candidate") &&
         expect(!invalidGeometry.accepted &&
                    invalidGeometry.status ==
                        cr::CreativeWorldLayoutBuildingTransformStatus::
                            InvalidGeometry &&
                    invalidGeometry.transformed.buildings.empty(),
                "malformed geometry is distinct from coordinate overflow");
}

bool twoDimensionalBuildingCompilesToExactThreeDimensionalOutput() {
  const cr::CreativeDocument document = makeDocument(201U);
  const cr::CreativeWorldLayout layout = smallHouseLayout();
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const cr::CreativeObject* root = findNamed(preview.document, "Small House");
  const cr::CreativeObject* door = findNamed(preview.document, "Front Door");
  const cr::CreativeObject* window = findNamed(preview.document, "East Window");

  return expect(compiled.receipt.accepted &&
                    compiled.receipt.status == cr::CreativeWorldLayoutStatus::Ready,
                "2D building layout compiles") &&
         expect(compiled.receipt.objectRecipeCount == 1U &&
                    compiled.receipt.objectCount == 14U,
                "2D building layout has deterministic generated object count") &&
         expect(preview.accepted && preview.document.objectCount() == 14U,
                "2D building plan previews exact document") &&
         expect(root != nullptr && root->bounds.min.x == 10.0 &&
                    root->bounds.min.y == 1.0 &&
                    root->bounds.min.z == -10.0 &&
                    root->bounds.max.x == 16.0 &&
                    root->bounds.max.y == 5.0 &&
                    root->bounds.max.z == -6.0,
                "grid-line footprint converts to exact world bounds") &&
         expect(door != nullptr && door->bounds.min.x == 12.0 &&
                    door->bounds.max.x == 14.0 &&
                    door->bounds.min.z == -6.125 &&
                    door->bounds.max.z == -5.875,
                "door slots into selected wall cutout") &&
         expect(window != nullptr && window->bounds.min.y == 3.0 &&
                    window->bounds.max.y == 4.0,
                "window inherits sill height in generated insert") &&
         expect(root != nullptr &&
                    cr::creativeRecipeObjectHasInstanceProvenance(
                        *root, cr::CreativeRecipeKind::Building,
                        "estate_level_0.house",
                        cr::CreativeRecipeObjectRole::Source, "root") &&
                    std::find(root->tags.begin(), root->tags.end(),
                              cr::creativeWorldLayoutTag("estate_level_0")) !=
                        root->tags.end(),
                "generated building preserves layout and instance provenance");
}

bool rebuildingAndDeletingLayoutNeverDuplicatesOutput() {
  cr::CreativeAppState appState = makeAppState(202U);
  cr::CreativeWorldLayout layout = smallHouseLayout();
  cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt firstApply =
      cr::applyCreativeWorldLayoutPlan(appState.facade, first.plan);
  const std::uint64_t firstObjectCount =
      appState.facade.document().objectCount();

  layout.buildings[0].name = "Renamed House";
  cr::CreativeWorldLayoutCompileResult replacement =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt replaced =
      cr::applyCreativeWorldLayoutPlan(appState.facade, replacement.plan);
  const std::uint64_t replacementObjectCount =
      appState.facade.document().objectCount();
  const bool replacementNamePresent =
      findNamed(appState.facade.document(), "Renamed House") != nullptr;

  cr::CreativeWorldLayout empty;
  empty.stableKey = layout.stableKey;
  cr::CreativeWorldLayoutCompileResult deletion =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), empty);
  const cr::CreativeWorldLayoutApplyReceipt deleted =
      cr::applyCreativeWorldLayoutPlan(appState.facade, deletion.plan);

  return expect(firstApply.accepted && firstApply.changed &&
                    firstObjectCount == 14U,
                "first layout generation has exact output count") &&
         expect(replacement.receipt.accepted &&
                    replacement.receipt.objectRemoveCount == 14U &&
                    replacement.receipt.objectCount == 14U,
                "layout rebuild replaces complete prior output") &&
         expect(replaced.accepted && replaced.changed &&
                    replacementObjectCount == 14U && replacementNamePresent,
                "layout replacement applies atomically without duplication") &&
         expect(deletion.receipt.accepted &&
                    deletion.receipt.objectRemoveCount == 14U,
                "empty source layout compiles deletion of stale output") &&
         expect(deleted.accepted && deleted.changed &&
                    appState.facade.document().objectCount() == 0U,
                "deleting final symbol removes generated 3D output");
}

bool authoritativeTerrainAndMaterialApplyAsOneHistoryStep() {
  cr::CreativeAppState appState = makeAppState(203U);
  const cr::CreativeTerrainControlEdit oldControl{
      cr::CreativeTerrainEditKind::Upsert, {{50, 50}, 6U, 1U}};
  const cr::CreativeTerrainMaterialEdit oldMaterial{
      cr::CreativeTerrainMaterialEditKind::Set, {50, 50},
      cr::CreativeTerrainMaterial::Stone};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&oldControl, 1U}));
  static_cast<void>(appState.facade.applyTerrainMaterialEdits(
      std::span{&oldMaterial, 1U}));

  cr::CreativeWorldLayout layout;
  layout.stableKey = "terrain_level_0";
  layout.terrainOwnership =
      cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;
  cr::CreativeWorldLayoutTerrainProfile hill;
  hill.stableKey = "hill.main";
  hill.kind = cr::CreativeTerrainRecipeKind::Hill;
  hill.baseHeightCells = 6U;
  hill.radiusCells = 2U;
  hill.amplitudeCells = 2U;
  layout.terrainProfiles.push_back(hill);
  layout.terrainPaths.push_back(
      {"river.main", cr::CreativeTerrainRecipeKind::River, 0U, 2U,
       cr::CreativeTerrainPathElevation::Level, 1U, 2U, true,
       cr::CreativeTerrainMaterial::Count});
  layout.terrainPathPoints = {{{6, 0}, 8U}, {{9, 0}, 8U}};

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(appState.facade.document(),
                                         compiled.plan);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, compiled.plan, "world_layout_terrain_test");
  const bool replacedOldTerrain =
      appState.facade.document().terrainField().controlAt({50, 50}) == nullptr &&
      appState.facade.document().terrainMaterialField().overrideAt({50, 50}) ==
          nullptr;
  const bool riverPainted = std::any_of(
      appState.facade.document().terrainMaterialField().overrides().begin(),
      appState.facade.document().terrainMaterialField().overrides().end(),
      [](const cr::CreativeTerrainMaterialOverride& value) {
        return value.material == cr::CreativeTerrainMaterial::Sand;
      });
  const std::uint64_t undoDepth = cr::creativeUndoDepth(appState.history);
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(compiled.receipt.accepted &&
                    !compiled.plan.terrainEdits.empty() &&
                    !compiled.plan.materialEdits.empty(),
                "authoritative terrain layout compiles net field diff") &&
         expect(preview.accepted && preview.hasTerrainRenderPlan &&
                    preview.terrainRenderPlan.accepted,
                "mixed terrain layout previews exact render plan") &&
         expect(applied.accepted && applied.changed &&
                    applied.historyReceipt.recorded && undoDepth == 1U,
                "mixed terrain layout applies as one history step") &&
         expect(replacedOldTerrain && riverPainted,
                "authoritative layout replaces old terrain and paints river") &&
         expect(undone.accepted && undone.changed &&
                    appState.facade.document().terrainField().controlAt({50, 50}) !=
                        nullptr &&
                    appState.facade.document().terrainMaterialField().materialAt(
                        {50, 50}) == cr::CreativeTerrainMaterial::Stone,
                "one undo restores pre-layout terrain and material");
}

bool invalidAndStaleSourcesFailClosed() {
  cr::CreativeDocument document = makeDocument(204U);
  cr::CreativeWorldLayout duplicate = smallHouseLayout();
  duplicate.boxes[1].stableKey = duplicate.boxes[0].stableKey;
  const cr::CreativeWorldLayoutCompileResult duplicateResult =
      cr::buildCreativeWorldLayoutPlan(document, duplicate);

  cr::CreativeWorldLayout relativeTerrain;
  relativeTerrain.stableKey = "relative_terrain";
  cr::CreativeWorldLayoutTerrainProfile relativeHill;
  relativeHill.stableKey = "hill";
  relativeHill.radiusCells = 2U;
  relativeHill.amplitudeCells = 2U;
  relativeHill.blend = cr::CreativeTerrainProfileBlend::Add;
  relativeTerrain.terrainProfiles.push_back(relativeHill);
  const cr::CreativeWorldLayoutCompileResult relativeResult =
      cr::buildCreativeWorldLayoutPlan(document, relativeTerrain);

  const cr::CreativeWorldLayoutCompileResult valid =
      cr::buildCreativeWorldLayoutPlan(document, smallHouseLayout());
  cr::Facade facade;
  static_cast<void>(facade.installDocument(document));
  const cr::CreativeTerrainControlEdit external{
      cr::CreativeTerrainEditKind::Upsert, {{30, 30}, 4U, 1U}};
  static_cast<void>(facade.applyTerrainControlEdits(
      std::span{&external, 1U}));
  const std::uint64_t revisionBefore = facade.document().revision();
  const cr::CreativeWorldLayoutApplyReceipt stale =
      cr::applyCreativeWorldLayoutPlan(facade, valid.plan);

  return expect(!duplicateResult.receipt.accepted &&
                    duplicateResult.receipt.status ==
                        cr::CreativeWorldLayoutStatus::DuplicateStableKey &&
                    duplicateResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Box,
                "duplicate source identity rejects at exact symbol table") &&
         expect(!relativeResult.receipt.accepted &&
                    relativeResult.receipt.status ==
                        cr::CreativeWorldLayoutStatus::InvalidSymbol,
                "relative terrain semantics reject from durable layout") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.status == cr::CreativeWorldLayoutStatus::StalePlan &&
                    facade.document().revision() == revisionBefore &&
                    facade.document().objectCount() == 0U,
                "stale world layout plan cannot publish partial output");
}

}  // namespace

int main() {
  const bool ok =
      twoDimensionalBuildingCompilesToExactThreeDimensionalOutput() &&
      rebuildingAndDeletingLayoutNeverDuplicatesOutput() &&
      authoritativeTerrainAndMaterialApplyAsOneHistoryStep() &&
      buildingTransformPreservesHostedOpeningSemantics() &&
      buildingTransformsRoundTripAndRejectOverflow() &&
      invalidAndStaleSourcesFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
