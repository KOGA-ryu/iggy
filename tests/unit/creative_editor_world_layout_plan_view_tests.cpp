#include "EditorWorldLayoutPlanView.hpp"

#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeGridSettings grid() {
  cr::CreativeGridSettings result;
  result.cellSizeMeters = 1.0;
  result.size = {64, 32, 64};
  return result;
}

app::CreativeEditorWorldLayoutState layoutState() {
  app::CreativeEditorWorldLayoutState state;
  state.source.stableKey = "plan_selection_layout";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  building.rootFootprint = {{0, 0}, {8, 8}};
  state.source.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  state.source.levels.push_back(level);

  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room";
  room.name = "Room";
  room.footprint = {{0, 0}, {8, 8}};
  state.source.rooms.push_back(room);

  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "partition";
  wall.name = "Partition";
  wall.start = {4, 0};
  wall.end = {4, 8};
  state.source.walls.push_back(wall);

  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  opening.wallIndex = 0U;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = "door";
  opening.name = "Door";
  opening.centerOffsetCells = 4.0;
  state.source.openings.push_back(opening);

  cr::CreativeWorldLayoutRoofAperture aperture;
  aperture.levelIndex = 0U;
  aperture.kind = cr::CreativeStructuralRoofApertureKind::Skylight;
  aperture.stableKey = "skylight";
  aperture.name = "Skylight";
  aperture.minimumXCells = 2.0;
  aperture.maximumXCells = 3.0;
  aperture.minimumZCells = 2.0;
  aperture.maximumZCells = 3.0;
  state.source.roofApertures.push_back(aperture);
  state.activeLevelIndex = 0U;
  return state;
}

cr::CreativeObjectId addObject(cr::CreativeDocument& document,
                               std::string name,
                               std::vector<std::string> tags = {},
                               bool visible = true,
                               bool locked = false) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = std::move(name);
  request.tags = std::move(tags);
  request.visible = visible;
  request.hasVisibleOverride = true;
  request.locked = locked;
  request.hasLockedOverride = true;
  return document.createObject(request).objectId;
}

cr::CreativeWorldLayoutPlanPrimitive primitive(
    cr::CreativeWorldLayoutPlanRole role) {
  cr::CreativeWorldLayoutPlanPrimitive value;
  value.role = role;
  value.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Segment;
  value.layer = cr::CreativeWorldLayoutPlanLayer::Active;
  value.points[0] = {0.0, 0.0};
  value.points[1] = {1.0, 0.0};
  value.pointCount = 2U;
  return value;
}

bool adapterIsExhaustive() {
  using PlanRole = cr::CreativeWorldLayoutPlanRole;
  using StyleRole = app::CreativeEditorDraftingRole;
  constexpr std::array<StyleRole,
                       static_cast<std::size_t>(PlanRole::Count)>
      kExpected = {
          StyleRole::RoomFloor,
          StyleRole::ExteriorWall,
          StyleRole::InteriorPartition,
          StyleRole::SharedBoundary,
          StyleRole::Door,
          StyleRole::DoorSwing,
          StyleRole::OpeningFacing,
          StyleRole::Window,
          StyleRole::WindowShutter,
          StyleRole::Stair,
          StyleRole::Ramp,
          StyleRole::RoofOutline,
          StyleRole::RoofRidge,
          StyleRole::RoofSkylight,
          StyleRole::RoofClearance,
          StyleRole::Hill,
          StyleRole::Hill,
          StyleRole::ContourMinor,
          StyleRole::ObjectBounds,
          StyleRole::Bridge,
          StyleRole::PlayerSpawn,
          StyleRole::NpcSpawn,
      };
  bool allMapped = true;
  for (std::size_t index = 0U; index < kExpected.size(); ++index) {
    auto value = primitive(static_cast<PlanRole>(index));
    if (value.role == PlanRole::TerrainProfile ||
        value.role == PlanRole::TerrainPath) {
      value.terrainKind = cr::CreativeTerrainRecipeKind::Hill;
    }
    if (value.role == PlanRole::Contour) {
      value.contourMajor = false;
    }
    if (value.role == PlanRole::Object) {
      value.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
      value.objectKind = cr::CreativeObjectKind::Unknown;
    }
    if (app::creativeEditorWorldLayoutPlanDraftingRole(value) !=
        kExpected[index]) {
      std::cerr << "wrong plan-role adapter at index " << index << '\n';
      allMapped = false;
    }
  }
  return expect(allMapped, "every semantic plan role has one visual owner");
}

bool terrainKindsRemainExact() {
  using StyleRole = app::CreativeEditorDraftingRole;
  constexpr std::array<StyleRole,
                       static_cast<std::size_t>(
                           cr::CreativeTerrainRecipeKind::Count)>
      kExpected = {
          StyleRole::Hill,      StyleRole::Valley, StyleRole::Crater,
          StyleRole::Ridge,     StyleRole::Road, StyleRole::River,
          StyleRole::Ditch,     StyleRole::RidgeLine, StyleRole::Plateau,
          StyleRole::Plateau,   StyleRole::Ridge,
      };
  bool exact = true;
  for (std::size_t index = 0U; index < kExpected.size(); ++index) {
    auto value = primitive(cr::CreativeWorldLayoutPlanRole::TerrainProfile);
    value.terrainKind = static_cast<cr::CreativeTerrainRecipeKind>(index);
    if (app::creativeEditorWorldLayoutPlanDraftingRole(value) !=
        kExpected[index]) {
      exact = false;
    }
  }
  return expect(exact, "terrain semantics never collapse into plateau");
}

bool objectCategoriesResolveDeclaratively() {
  auto value = primitive(cr::CreativeWorldLayoutPlanRole::Object);
  value.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
  value.objectKind = cr::CreativeObjectKind::Wall;
  const bool architecture =
      app::creativeEditorWorldLayoutPlanDraftingRole(value) ==
      app::CreativeEditorDraftingRole::ObjectArchitecture;
  value.objectKind = cr::CreativeObjectKind::Rock;
  const bool nature = app::creativeEditorWorldLayoutPlanDraftingRole(value) ==
                      app::CreativeEditorDraftingRole::ObjectNature;
  value.objectKind = cr::CreativeObjectKind::CoverPoint;
  value.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Point;
  const bool cover = app::creativeEditorWorldLayoutPlanDraftingRole(value) ==
                     app::CreativeEditorDraftingRole::ObjectCover;
  value.objectKind = cr::CreativeObjectKind::Furniture;
  value.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
  const bool prop = app::creativeEditorWorldLayoutPlanDraftingRole(value) ==
                    app::CreativeEditorDraftingRole::ObjectProp;
  value.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Point;
  value.objectKind = cr::CreativeObjectKind::PatrolNode;
  const bool point = app::creativeEditorWorldLayoutPlanDraftingRole(value) ==
                     app::CreativeEditorDraftingRole::ObjectPoint;
  return expect(architecture && nature && cover && prop && point,
                "object descriptor categories drive plan presentation");
}

bool cacheReusesStableFrames() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  app::CreativeEditorWorldLayoutTopographyState topography;
  app::CreativeEditorWorldLayoutPlanViewCache cache;
  const bool first = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  const bool idle = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  ++state.revision;
  const bool revision = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  ++topography.buildCount;
  const bool contours = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.buildingTransform.active = true;
  state.buildingTransform.sourceRevision = state.revision;
  state.buildingTransform.buildingIndex = 0U;
  state.buildingTransform.candidate = state.source;
  const bool candidate = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.buildingTransform.active = false;
  const bool restoredSource = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.preview.document = cr::CreativeDocument::create("Plan Preview");
  const bool previewDocumentReady = state.preview.document.assignId(7101U);
  state.preview.accepted = true;
  state.previewVisible = true;
  state.previewLayoutRevision = state.revision;
  state.previewContentRevision = 1U;
  state.previewSource = state.source;
  const bool preview = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  const bool previewIdle = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  ++state.previewContentRevision;
  const bool previewRefresh = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.previewVisible = false;
  const bool previewClosed = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.planLowerLevelContextVisible = false;
  const bool lowerContext = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.planUpperLevelContextVisible = true;
  const bool upperContext = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  state.planRoofOverheadVisible = false;
  const bool roofOverhead = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  app::invalidateCreativeEditorWorldLayoutPlanView(cache);
  const bool invalidated = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  bool paintOrderStable =
      cache.paintOrder.size() == cache.projection.primitives.size();
  std::uint8_t previousOrder = 0U;
  for (const std::size_t index : cache.paintOrder) {
    if (index >= cache.projection.primitives.size()) {
      paintOrderStable = false;
      continue;
    }
    const auto role = app::creativeEditorWorldLayoutPlanDraftingRole(
        cache.projection.primitives[index]);
    const std::uint8_t currentOrder =
        app::creativeEditorDraftingStyle(role).drawOrder;
    paintOrderStable &= currentOrder >= previousOrder;
    previousOrder = currentOrder;
  }
  return expect(first && !idle && revision && contours && candidate &&
                    restoredSource && previewDocumentReady && preview &&
                    !previewIdle && previewRefresh && previewClosed &&
                    lowerContext && upperContext && roofOverhead &&
                    invalidated && cache.buildCount == 12U &&
                    !cache.key.lowerLevelContextVisible &&
                    cache.key.upperLevelContextVisible &&
                    !cache.key.roofOverheadVisible &&
                    cache.projection.accepted && paintOrderStable,
                "stable frames reuse while candidate transitions invalidate");
}

bool sourceProvenanceDrivesOverlays() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  auto room = primitive(cr::CreativeWorldLayoutPlanRole::RoomFloor);
  room.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
  room.source = {cr::CreativeWorldLayoutTable::Room, 0U,
                 cr::CreativeWorldLayoutTable::None,
                 cr::kInvalidCreativeWorldLayoutIndex};
  state.selection = {app::CreativeEditorWorldLayoutSelectionKind::Room, 0U};
  state.buildingManipulation.active = true;
  state.buildingManipulation.buildingIndex = 0U;
  state.buildingManipulation.previewDeltaXCells = 3;
  state.buildingManipulation.previewDeltaZCells = -2;
  const auto [offsetX, offsetZ] =
      app::creativeEditorWorldLayoutPlanPrimitiveOffset(state, state.source,
                                                        room);
  state.roomManipulation.active = true;
  state.roomManipulation.target.roomIndex = 0U;

  auto opening = primitive(cr::CreativeWorldLayoutPlanRole::Door);
  opening.source = {cr::CreativeWorldLayoutTable::Opening, 0U,
                    cr::CreativeWorldLayoutTable::None,
                    cr::kInvalidCreativeWorldLayoutIndex};
  state.roomManipulation.active = false;
  state.wallManipulation.active = true;
  state.wallManipulation.target.wallIndex = 0U;
  return expect(
      app::creativeEditorWorldLayoutPlanPrimitiveSelected(state, room) &&
          offsetX == 3.0 && offsetZ == -2.0 &&
          app::creativeEditorWorldLayoutPlanPrimitiveBuildingIndex(
              state.source, opening) == 0U &&
          app::creativeEditorWorldLayoutPlanPrimitiveSuppressed(
              state, state.source, opening),
      "selection, building movement, and manipulation use source provenance");
}

bool hitTestUsesPaintOrderLayersOffsetsAndProvenance() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  app::CreativeEditorWorldLayoutPlanViewCache cache;
  cache.valid = true;
  cache.projection.accepted = true;

  auto room = primitive(cr::CreativeWorldLayoutPlanRole::RoomFloor);
  room.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
  room.pointCount = 4U;
  room.points = {{{0.0, 0.0}, {8.0, 0.0}, {8.0, 8.0}, {0.0, 8.0}}};
  room.source = {cr::CreativeWorldLayoutTable::Room, 0U,
                 cr::CreativeWorldLayoutTable::None,
                 cr::kInvalidCreativeWorldLayoutIndex};
  auto wall = primitive(cr::CreativeWorldLayoutPlanRole::InteriorPartition);
  wall.points[0] = {4.0, 0.0};
  wall.points[1] = {4.0, 8.0};
  wall.source = {cr::CreativeWorldLayoutTable::Wall, 0U,
                 cr::CreativeWorldLayoutTable::None,
                 cr::kInvalidCreativeWorldLayoutIndex};
  auto contour = primitive(cr::CreativeWorldLayoutPlanRole::Contour);
  contour.points[0] = {0.0, 4.0};
  contour.points[1] = {8.0, 4.0};
  contour.source = {};
  cache.projection.primitives = {room, wall, contour};
  cache.paintOrder = {0U, 1U, 2U};

  const app::CreativeEditorWorldLayoutPlanHit wallHit =
      app::hitCreativeEditorWorldLayoutPlan(cache, state, state.source,
                                            {4.0, 3.0}, 0.1);
  const app::CreativeEditorWorldLayoutPlanHit roomHit =
      app::hitCreativeEditorWorldLayoutPlan(cache, state, state.source,
                                            {2.0, 3.0}, 0.1);
  cache.projection.primitives[1].layer =
      cr::CreativeWorldLayoutPlanLayer::LowerContext;
  const app::CreativeEditorWorldLayoutPlanHit lowerContextIgnored =
      app::hitCreativeEditorWorldLayoutPlan(cache, state, state.source,
                                            {4.0, 3.0}, 0.1);
  cache.projection.primitives[1].layer =
      cr::CreativeWorldLayoutPlanLayer::UpperContext;
  const app::CreativeEditorWorldLayoutPlanHit upperContextIgnored =
      app::hitCreativeEditorWorldLayoutPlan(cache, state, state.source,
                                            {4.0, 3.0}, 0.1);
  cache.projection.primitives[1].layer =
      cr::CreativeWorldLayoutPlanLayer::Active;
  state.buildingManipulation.active = true;
  state.buildingManipulation.buildingIndex = 0U;
  state.buildingManipulation.previewDeltaXCells = 3;
  const app::CreativeEditorWorldLayoutPlanHit moved =
      app::hitCreativeEditorWorldLayoutPlan(cache, state, state.source,
                                            {7.0, 3.0}, 0.1);

  return expect(
      wallHit.hit && wallHit.primitiveIndex == 1U &&
          wallHit.table == cr::CreativeWorldLayoutTable::Wall &&
          wallHit.sourceIndex == 0U && wallHit.sourceLevelIndex == 0U &&
          roomHit.hit && roomHit.sourceLevelIndex == 0U &&
          roomHit.primitiveIndex == 0U && lowerContextIgnored.hit &&
          lowerContextIgnored.primitiveIndex == 0U &&
          upperContextIgnored.hit &&
          upperContextIgnored.primitiveIndex == 0U && moved.hit &&
          moved.primitiveIndex == 1U,
                "topmost active provenance wins and building offsets remain clickable");
}

bool overlapStackCyclesSemanticSourcesAndStaysBounded() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  app::CreativeEditorWorldLayoutPlanViewCache cache;
  cache.valid = true;
  cache.projection.accepted = true;

  auto room = primitive(cr::CreativeWorldLayoutPlanRole::RoomFloor);
  room.kind = cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
  room.pointCount = 4U;
  room.points = {{{0.0, 0.0}, {8.0, 0.0}, {8.0, 8.0}, {0.0, 8.0}}};
  room.source = {cr::CreativeWorldLayoutTable::Room, 0U,
                 cr::CreativeWorldLayoutTable::None,
                 cr::kInvalidCreativeWorldLayoutIndex};
  auto wall = primitive(cr::CreativeWorldLayoutPlanRole::InteriorPartition);
  wall.points[0] = {4.0, 0.0};
  wall.points[1] = {4.0, 8.0};
  wall.source = {cr::CreativeWorldLayoutTable::Wall, 0U,
                 cr::CreativeWorldLayoutTable::None,
                 cr::kInvalidCreativeWorldLayoutIndex};
  auto wallSecondGlyph = wall;
  wallSecondGlyph.role = cr::CreativeWorldLayoutPlanRole::SharedBoundary;
  cache.projection.primitives = {room, wall, wallSecondGlyph};
  cache.paintOrder = {0U, 1U, 2U};

  const app::CreativeEditorWorldLayoutPlanHitStack stack =
      app::hitCreativeEditorWorldLayoutPlanStack(
          cache, state, state.source, {4.0, 3.0}, 0.1);
  const app::CreativeEditorWorldLayoutPlanHit first =
      app::cycleCreativeEditorWorldLayoutPlanHit(stack, {});
  const app::CreativeEditorWorldLayoutPlanHit afterWall =
      app::cycleCreativeEditorWorldLayoutPlanHit(
          stack, {app::CreativeEditorWorldLayoutSelectionKind::Wall, 0U});
  const app::CreativeEditorWorldLayoutPlanHit afterRoom =
      app::cycleCreativeEditorWorldLayoutPlanHit(
          stack, {app::CreativeEditorWorldLayoutSelectionKind::Room, 0U});

  app::CreativeEditorWorldLayoutState crowded = layoutState();
  crowded.source.walls.clear();
  app::CreativeEditorWorldLayoutPlanViewCache crowdedCache;
  crowdedCache.valid = true;
  crowdedCache.projection.accepted = true;
  for (std::size_t index = 0U;
       index <= app::kCreativeEditorWorldLayoutPlanHitCapacity; ++index) {
    cr::CreativeWorldLayoutWall sourceWall;
    sourceWall.buildingIndex = 0U;
    sourceWall.stableKey = "wall_" + std::to_string(index);
    sourceWall.start = {0, 0};
    sourceWall.end = {8, 0};
    crowded.source.walls.push_back(sourceWall);
    auto candidate = wall;
    candidate.source.primaryIndex = index;
    crowdedCache.projection.primitives.push_back(candidate);
    crowdedCache.paintOrder.push_back(index);
  }
  const app::CreativeEditorWorldLayoutPlanHitStack bounded =
      app::hitCreativeEditorWorldLayoutPlanStack(
          crowdedCache, crowded, crowded.source, {4.0, 0.0}, 0.1);

  return expect(stack.count == 2U && stack.totalHitPrimitiveCount == 3U &&
                    first.table == cr::CreativeWorldLayoutTable::Wall &&
                    afterWall.table == cr::CreativeWorldLayoutTable::Room &&
                    afterRoom.table == cr::CreativeWorldLayoutTable::Wall,
                "overlap cycle deduplicates glyphs and wraps semantic sources") &&
         expect(bounded.count ==
                        app::kCreativeEditorWorldLayoutPlanHitCapacity &&
                    bounded.totalHitPrimitiveCount ==
                        app::kCreativeEditorWorldLayoutPlanHitCapacity + 1U &&
                    bounded.truncated,
                "plan overlap stack reports its fixed capacity");
}

bool overheadRoofApertureRemainsSelectable() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  app::CreativeEditorWorldLayoutTopographyState topography;
  app::CreativeEditorWorldLayoutPlanViewCache cache;
  const bool refreshed = app::refreshCreativeEditorWorldLayoutPlanView(
      cache, state, topography, grid());
  const app::CreativeEditorWorldLayoutPlanHit hit =
      app::hitCreativeEditorWorldLayoutPlan(cache, state, state.source,
                                            {2.5, 2.5}, 0.05);
  cr::CreativeWorldLayoutPlanPrimitive aperturePrimitive;
  aperturePrimitive.role = cr::CreativeWorldLayoutPlanRole::RoofSkylight;
  aperturePrimitive.kind =
      cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon;
  aperturePrimitive.layer = cr::CreativeWorldLayoutPlanLayer::Overhead;
  aperturePrimitive.source = {
      cr::CreativeWorldLayoutTable::RoofAperture, 0U,
      cr::CreativeWorldLayoutTable::None,
      cr::kInvalidCreativeWorldLayoutIndex};
  state.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::RoofAperture, 0U};
  return expect(refreshed && cache.projection.accepted && hit.hit &&
                    hit.table == cr::CreativeWorldLayoutTable::RoofAperture &&
                    hit.sourceIndex == 0U && hit.sourceLevelIndex == 0U &&
                    app::creativeEditorWorldLayoutPlanPrimitiveSelected(
                        state, aperturePrimitive) &&
                    app::creativeEditorWorldLayoutPlanPrimitiveBuildingIndex(
                        state.source, aperturePrimitive) == 0U,
                "overhead roof apertures retain selectable source provenance");
}

bool regionSelectionUsesDirectionDeduplicationAndCapacity() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  app::CreativeEditorWorldLayoutPlanViewCache cache;
  cache.valid = true;
  cache.projection.accepted = true;

  auto wall = primitive(cr::CreativeWorldLayoutPlanRole::InteriorPartition);
  wall.points[0] = {4.0, 0.0};
  wall.points[1] = {4.0, 8.0};
  wall.source = {cr::CreativeWorldLayoutTable::Wall, 0U,
                 cr::CreativeWorldLayoutTable::None,
                 cr::kInvalidCreativeWorldLayoutIndex};
  auto duplicateWall = wall;
  duplicateWall.role = cr::CreativeWorldLayoutPlanRole::SharedBoundary;
  auto opening = primitive(cr::CreativeWorldLayoutPlanRole::Door);
  opening.points[0] = {6.0, 2.0};
  opening.points[1] = {6.0, 4.0};
  opening.source = {cr::CreativeWorldLayoutTable::Opening, 0U,
                    cr::CreativeWorldLayoutTable::None,
                    cr::kInvalidCreativeWorldLayoutIndex};
  auto context = wall;
  context.layer = cr::CreativeWorldLayoutPlanLayer::LowerContext;
  cache.projection.primitives = {wall, duplicateWall, opening, context};
  cache.paintOrder = {0U, 1U, 2U, 3U};

  const app::CreativeEditorWorldLayoutPlanRegionSelection window =
      app::selectCreativeEditorWorldLayoutPlanRegion(
          cache, state, state.source, {3.0, -1.0}, {5.0, 9.0});
  const app::CreativeEditorWorldLayoutPlanRegionSelection crossing =
      app::selectCreativeEditorWorldLayoutPlanRegion(
          cache, state, state.source, {4.1, 2.0}, {3.9, 3.0});
  const app::CreativeEditorWorldLayoutPlanRegionSelection narrowWindow =
      app::selectCreativeEditorWorldLayoutPlanRegion(
          cache, state, state.source, {3.9, 2.0}, {4.1, 3.0});
  const app::CreativeEditorWorldLayoutPlanRegionSelection both =
      app::selectCreativeEditorWorldLayoutPlanRegion(
          cache, state, state.source, {3.0, -1.0}, {7.0, 9.0});
  const app::CreativeEditorWorldLayoutPlanRegionSelection overflow =
      app::selectCreativeEditorWorldLayoutPlanRegion(
          cache, state, state.source, {3.0, -1.0}, {7.0, 9.0}, 0.0, 1U);

  return expect(window.accepted &&
                    window.mode ==
                        cr::CreativeWorldLayoutPlanRegionMode::Window &&
                    window.sources.size() == 1U &&
                    window.sources.front().table ==
                        cr::CreativeWorldLayoutTable::Wall,
                "window region deduplicates one source across primitives") &&
         expect(crossing.accepted &&
                    crossing.mode ==
                        cr::CreativeWorldLayoutPlanRegionMode::Crossing &&
                    crossing.sources == window.sources &&
                    narrowWindow.accepted && narrowWindow.sources.empty(),
                "drag direction selects crossing versus enclosed geometry") &&
         expect(both.accepted && both.sources.size() == 2U &&
                    overflow.overflowed && !overflow.accepted &&
                    overflow.sources.empty(),
                "source capacity fails atomically");
}

bool sourceObjectSelectionComposesAndKeepsInspectableObjects() {
  app::CreativeEditorWorldLayoutState state = layoutState();
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Plan Region Selection");
  const std::string layoutTag =
      cr::creativeWorldLayoutTag(state.source.stableKey);
  const cr::CreativeObjectId wallId = addObject(
      document, "Hidden Locked Wall",
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      state.source, cr::CreativeWorldLayoutTable::Wall, 0U)},
      false, true);
  const cr::CreativeObjectId openingId = addObject(
      document, "Opening",
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      state.source, cr::CreativeWorldLayoutTable::Opening,
                      0U)});
  const cr::CreativeObjectId authoredId = addObject(document, "Authored");

  cr::CreativeSelectionState none = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeWorldLayoutSourceRef wallSource{
      cr::CreativeWorldLayoutTable::Wall, 0U};
  const cr::CreativeWorldLayoutSourceRef openingSource{
      cr::CreativeWorldLayoutTable::Opening, 0U};
  const app::CreativeEditorObjectSelectionPlan replace =
      app::planCreativeEditorWorldLayoutObjectSelection(
          state, document, none,
          std::span<const cr::CreativeWorldLayoutSourceRef>{&wallSource, 1U},
          app::CreativeEditorSelectionComposition::Replace);

  cr::CreativeSelectionState current = cr::makeDefaultCreativeSelectionState();
  const cr::TargetRef currentTargets[] = {
      cr::TargetRef{static_cast<cr::Id>(authoredId)},
      cr::TargetRef{static_cast<cr::Id>(openingId)}};
  static_cast<void>(cr::setSelectedTargets(
      current, currentTargets,
      cr::TargetRef{static_cast<cr::Id>(authoredId)}));
  const app::CreativeEditorObjectSelectionPlan add =
      app::planCreativeEditorWorldLayoutObjectSelection(
          state, document, current,
          std::span<const cr::CreativeWorldLayoutSourceRef>{&wallSource, 1U},
          app::CreativeEditorSelectionComposition::Add);
  const app::CreativeEditorObjectSelectionPlan toggle =
      app::planCreativeEditorWorldLayoutObjectSelection(
          state, document, current,
          std::span<const cr::CreativeWorldLayoutSourceRef>{&openingSource, 1U},
          app::CreativeEditorSelectionComposition::Toggle);
  const app::CreativeEditorObjectSelectionPlan overflow =
      app::planCreativeEditorWorldLayoutObjectSelection(
          state, document, current,
          std::span<const cr::CreativeWorldLayoutSourceRef>{&wallSource, 1U},
          app::CreativeEditorSelectionComposition::Add, 2U);
  ++state.revision;
  const app::CreativeEditorObjectSelectionPlan stale =
      app::planCreativeEditorWorldLayoutObjectSelection(
          state, document, none,
          std::span<const cr::CreativeWorldLayoutSourceRef>{&wallSource, 1U},
          app::CreativeEditorSelectionComposition::Replace);

  return expect(replace.accepted && replace.objectIds.size() == 1U &&
                    replace.objectIds.front() == wallId &&
                    replace.primaryObjectId == wallId,
                "hidden locked generated objects remain plan-selectable") &&
         expect(add.accepted && add.objectIds.size() == 3U &&
                    add.objectIds[0] == authoredId &&
                    add.objectIds[1] == openingId &&
                    add.objectIds[2] == wallId &&
                    add.primaryObjectId == authoredId,
                "add composition preserves order and primary") &&
         expect(toggle.accepted && toggle.objectIds.size() == 1U &&
                    toggle.objectIds.front() == authoredId &&
                    toggle.primaryObjectId == authoredId,
                "toggle composition removes an existing source scope") &&
         expect(!overflow.accepted && overflow.overflowed &&
                    overflow.objectIds.empty() && !stale.accepted,
                "capacity and stale generation reject atomically");
}

}  // namespace

int main() {
  bool ok = true;
  ok = adapterIsExhaustive() && ok;
  ok = terrainKindsRemainExact() && ok;
  ok = objectCategoriesResolveDeclaratively() && ok;
  ok = cacheReusesStableFrames() && ok;
  ok = sourceProvenanceDrivesOverlays() && ok;
  ok = hitTestUsesPaintOrderLayersOffsetsAndProvenance() && ok;
  ok = overlapStackCyclesSemanticSourcesAndStaysBounded() && ok;
  ok = overheadRoofApertureRemainsSelectable() && ok;
  ok = regionSelectionUsesDirectionDeduplicationAndCapacity() && ok;
  ok = sourceObjectSelectionComposesAndKeepsInspectableObjects() && ok;
  return ok ? 0 : 1;
}
