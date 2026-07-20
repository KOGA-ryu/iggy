#include "EditorWorldLayoutPlanView.hpp"

#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>

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
  state.activeLevelIndex = 0U;
  return state;
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
          StyleRole::Window,
          StyleRole::Stair,
          StyleRole::Ramp,
          StyleRole::RoofOutline,
          StyleRole::RoofRidge,
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
                    restoredSource && invalidated && cache.buildCount == 6U &&
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

}  // namespace

int main() {
  bool ok = true;
  ok = adapterIsExhaustive() && ok;
  ok = terrainKindsRemainExact() && ok;
  ok = objectCategoriesResolveDeclaratively() && ok;
  ok = cacheReusesStableFrames() && ok;
  ok = sourceProvenanceDrivesOverlays() && ok;
  return ok ? 0 : 1;
}
