#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutAdoption.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingTemplatePlacement.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsCollisionQueries.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double epsilon = 1.0e-12) {
  return std::abs(lhs - rhs) <= epsilon;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

cr::CreativeDocument makeDocument(cr::CreativeDocumentId id);
const cr::CreativeObject* findNamed(const cr::CreativeDocument& document,
                                    std::string_view name);

bool terrainGroundedBuildingsShiftAsOneAndFillRelief() {
  cr::CreativeDocument document = makeDocument(76U);
  constexpr std::array<std::uint16_t, 4U> heights{2U, 3U, 2U, 3U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt terrain =
      document.replaceTerrainHeightField({{0, 0}, 2U, 2U}, heights);

  cr::CreativeWorldLayout layout;
  layout.stableKey = "grounded_layout";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "grounded_house";
  building.name = "Grounded House";
  building.rootFootprint = {{0, 0}, {2, 2}};
  building.groundingMode =
      cr::CreativeWorldLayoutGroundingMode::Foundation;
  building.maximumGroundReliefCells = 1U;
  layout.buildings.push_back(building);
  layout.levels.push_back(
      {0U, "ground", "Ground", 0.05, 3U, 1U, 1U, 1U});
  layout.rooms.push_back(
      {0U, 0U, "room", "Grounded Room", {{0, 0}, {2, 2}}, 0.25});

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  cr::Facade facade;
  static_cast<void>(facade.installDocument(std::move(document)));
  const cr::CreativeWorldLayoutApplyReceipt applied =
      compiled.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlan(facade, compiled.plan)
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeObject* floor =
      findNamed(facade.document(), "Grounded Room Floor");
  const cr::CreativeObject* foundation =
      findNamed(facade.document(), "Grounded House Foundation");

  return expect(terrain.accepted && terrain.changed,
                "grounded building fixture has dense terrain") &&
         expect(compiled.receipt.accepted &&
                    compiled.receipt.groundedBuildingCount == 1U &&
                    compiled.receipt.foundationObjectCount == 1U,
                "world layout reports grounded foundation output") &&
         expect(applied.accepted && applied.changed && floor != nullptr &&
                    near(floor->bounds.min.y, 4.0) &&
                    near(floor->bounds.max.y, 4.05),
                "floor underside rests on highest terrain cell") &&
         expect(foundation != nullptr &&
                    near(foundation->bounds.min.y, 3.0) &&
                    near(foundation->bounds.max.y, 4.0),
                "foundation fills bounded terrain relief");
}

bool stagedLandformGroundsBuildingAgainstPreviewTerrain() {
  cr::CreativeDocument document = makeDocument(79U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "staged_landform_grounding";

  cr::CreativeWorldLayoutTerrainProfile plateau;
  plateau.stableKey = "building_pad";
  plateau.kind = cr::CreativeTerrainRecipeKind::Plateau;
  plateau.usesLandformRecipe = true;
  plateau.landform.kind = cr::CreativeTerrainLandformKind::Plateau;
  plateau.landform.bounds = {{0, 0}, 2U, 2U};
  plateau.landform.baseHeightCells = 1U;
  plateau.landform.targetHeightCells = 5U;
  plateau.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  plateau.landform.edgeWidthCells = 0U;
  layout.terrainProfiles.push_back(plateau);

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "plateau_house";
  building.name = "Plateau House";
  building.rootFootprint = {{0, 0}, {2, 2}};
  building.groundingMode =
      cr::CreativeWorldLayoutGroundingMode::Foundation;
  building.maximumGroundReliefCells = 0U;
  layout.buildings.push_back(building);
  layout.levels.push_back(
      {0U, "ground", "Ground", 0.05, 3U, 1U, 1U, 1U});
  layout.rooms.push_back(
      {0U, 0U, "room", "Plateau Room", {{0, 0}, {2, 2}}, 0.25});

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  cr::Facade facade;
  static_cast<void>(facade.installDocument(std::move(document)));
  const cr::CreativeWorldLayoutApplyReceipt applied =
      compiled.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlan(facade, compiled.plan)
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeObject* floor =
      findNamed(facade.document(), "Plateau Room Floor");

  return expect(compiled.receipt.accepted &&
                    compiled.receipt.groundedBuildingCount == 1U &&
                    compiled.plan.terrainOperationMutations.size() == 1U &&
                    compiled.plan.terrainOperationMutations[0].operationKind ==
                        cr::CreativeTerrainOperationKind::Landform,
                "building compile stages the landform before grounding") &&
         expect(applied.accepted && applied.changed && floor != nullptr &&
                    near(floor->bounds.min.y, 6.0) &&
                    near(floor->bounds.max.y, 6.05),
                "building floor rests on the staged plateau surface");
}

bool denseTerrainRevisionInvalidatesGroundedPlans() {
  cr::CreativeDocument document = makeDocument(77U);
  constexpr std::array<std::uint16_t, 1U> firstHeight{2U};
  static_cast<void>(
      document.replaceTerrainHeightField({{0, 0}, 1U, 1U}, firstHeight));

  cr::CreativeWorldLayout layout;
  layout.stableKey = "stale_grounding";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  building.rootFootprint = {{0, 0}, {1, 1}};
  building.groundingMode =
      cr::CreativeWorldLayoutGroundingMode::Foundation;
  layout.buildings.push_back(building);
  layout.levels.push_back(
      {0U, "ground", "Ground", 0.05, 2U, 1U, 1U, 1U});
  layout.rooms.push_back(
      {0U, 0U, "room", "Room", {{0, 0}, {1, 1}}, 0.1});

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  constexpr std::array<std::uint16_t, 1U> changedHeight{3U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt changed =
      document.replaceTerrainHeightField({{0, 0}, 1U, 1U}, changedHeight);
  cr::Facade facade;
  static_cast<void>(facade.installDocument(std::move(document)));
  const cr::CreativeWorldLayoutApplyReceipt stale =
      cr::applyCreativeWorldLayoutPlan(facade, compiled.plan);

  return expect(compiled.receipt.accepted,
                compiled.receipt.reasonCode) &&
         expect(changed.accepted && changed.changed,
                "dense terrain edit changes the source revision") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.status == cr::CreativeWorldLayoutStatus::StalePlan,
                "dense terrain revision invalidates grounded plan");
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

cr::CreativeWorldLayoutTerrainPath terrainPath(std::string key,
                                               std::int32_t z,
                                               std::uint16_t height,
                                               std::uint16_t halfWidth = 1U) {
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = std::move(key);
  path.recipe.kind = cr::CreativeTerrainPathKind::Road;
  path.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  path.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Crowned;
  path.recipe.material = cr::CreativeTerrainMaterial::Dirt;
  path.recipe.nextPointId = 3U;
  path.recipe.points = {
      {1U, {0, z}, height, halfWidth, 1U, 0},
      {2U, {8, z}, height, halfWidth, 1U, 0},
  };
  return path;
}

cr::CreativeWorldLayoutTerrainProfile landform(
    std::string key,
    cr::CreativeTerrainLandformKind kind =
        cr::CreativeTerrainLandformKind::Terrace) {
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = std::move(key);
  profile.kind = cr::creativeTerrainRecipeKind(kind);
  profile.usesLandformRecipe = true;
  profile.landform.kind = kind;
  profile.landform.bounds = {{0, 0}, 8U, 4U};
  profile.landform.baseHeightCells = 2U;
  profile.landform.targetHeightCells = 8U;
  profile.landform.terraceCount = 4U;
  profile.landform.direction =
      cr::CreativeTerrainLandformDirection::PositiveX;
  profile.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  profile.landform.edgeWidthCells = 0U;
  profile.landform.material = cr::CreativeTerrainMaterial::Stone;
  return profile;
}

const cr::CreativeTerrainOperation* findTerrainOperation(
    const cr::CreativeDocument& document,
    std::string_view sourceKey) {
  const auto& operations = document.terrainOperationStack().operations;
  const auto found = std::find_if(
      operations.begin(), operations.end(),
      [sourceKey](const cr::CreativeTerrainOperation& operation) {
        return operation.sourceKey == sourceKey;
      });
  return found == operations.end() ? nullptr : &*found;
}

const cr::CreativeObject* findNamed(const cr::CreativeDocument& document,
                                    std::string_view name) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [name](const cr::CreativeObject& object) { return object.name == name; });
  return found == document.objects().end() ? nullptr : &*found;
}

const cr::CreativeObject* findManaged(
    const cr::CreativeDocument& document,
    std::string_view instanceKey,
    std::string_view stableKey) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [&](const cr::CreativeObject& object) {
        return cr::creativeRecipeObjectInstanceKey(object) == instanceKey &&
               cr::creativeRecipeObjectStableKey(object) == stableKey;
      });
  return found == document.objects().end() ? nullptr : &*found;
}

std::map<std::string, cr::CreativeObjectId> managedObjectIds(
    const cr::CreativeDocument& document,
    std::string_view instanceKey) {
  std::map<std::string, cr::CreativeObjectId> output;
  for (const cr::CreativeObject& object : document.objects()) {
    if (cr::creativeRecipeObjectInstanceKey(object) == instanceKey) {
      output.emplace(std::string(cr::creativeRecipeObjectStableKey(object)),
                     object.id);
    }
  }
  return output;
}

const cr::CreativeWorldLayoutRecipeMemberConflict* findMemberConflict(
    const cr::CreativeWorldLayoutCompileResult& compiled,
    std::string_view stableKey) {
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       compiled.recipeChanges) {
    const auto found = std::find_if(
        change.memberConflicts.begin(), change.memberConflicts.end(),
        [stableKey](
            const cr::CreativeWorldLayoutRecipeMemberConflict& conflict) {
          return conflict.stableKey == stableKey;
        });
    if (found != change.memberConflicts.end()) {
      return &*found;
    }
  }
  return nullptr;
}

cr::CreativeWorldLayoutConflictDecision memberDecision(
    std::string instanceKey,
    const cr::CreativeWorldLayoutRecipeMemberConflict& conflict,
    cr::CreativeWorldLayoutConflictResolution resolution) {
  return cr::makeCreativeWorldLayoutMemberConflictDecision(
      std::move(instanceKey), conflict, resolution);
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
  layout.levels.push_back(
      {0U, "level", "Ground Level", 0.0, 3U, 1U, 1U, 1U});
  layout.rooms.push_back(
      {0U, 0U, "room", "Transform Room", {{11, 21}, {17, 25}}, 0.25});
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

  const auto openDoor = [](cr::CreativeDoorHingeSide hinge,
                           cr::CreativeDoorSwingSide swing) {
    cr::CreativeDoorSettings settings;
    settings.hingeSide = hinge;
    settings.swingSide = swing;
    settings.initialState = cr::CreativeDoorInitialState::Open;
    return settings;
  };
  const auto addRoomDoor =
      [&](std::string key, cr::CreativeWorldLayoutRoomEdge edge, double offset,
          cr::CreativeDoorSettings door) {
        cr::CreativeWorldLayoutOpening opening;
        opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
        opening.roomIndex = 0U;
        opening.roomEdge = edge;
        opening.kind = cr::CreativeBuildingOpeningKind::Door;
        opening.door = door;
        opening.stableKey = std::move(key);
        opening.name = opening.stableKey;
        opening.centerOffsetCells = offset;
        opening.widthCells = 1.0;
        opening.cutoutHeightCells = 2.0;
        layout.openings.push_back(std::move(opening));
      };
  addRoomDoor("door_n", cr::CreativeWorldLayoutRoomEdge::North, 1.0,
              openDoor(cr::CreativeDoorHingeSide::MinimumEdge,
                       cr::CreativeDoorSwingSide::PositiveNormal));
  addRoomDoor("door_e", cr::CreativeWorldLayoutRoomEdge::East, 1.0,
              openDoor(cr::CreativeDoorHingeSide::MinimumEdge,
                       cr::CreativeDoorSwingSide::PositiveNormal));
  addRoomDoor("door_s", cr::CreativeWorldLayoutRoomEdge::South, 2.0,
              openDoor(cr::CreativeDoorHingeSide::MaximumEdge,
                       cr::CreativeDoorSwingSide::NegativeNormal));
  addRoomDoor("door_w", cr::CreativeWorldLayoutRoomEdge::West, 3.0,
              openDoor(cr::CreativeDoorHingeSide::MaximumEdge,
                       cr::CreativeDoorSwingSide::PositiveNormal));

  const auto addWallDoor = [&](std::string key, std::size_t wallIndex,
                               double offset, cr::CreativeDoorSettings door) {
    cr::CreativeWorldLayoutOpening opening;
    opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
    opening.wallIndex = wallIndex;
    opening.kind = cr::CreativeBuildingOpeningKind::Door;
    opening.door = door;
    opening.stableKey = std::move(key);
    opening.name = opening.stableKey;
    opening.centerOffsetCells = offset;
    opening.widthCells = 1.0;
    opening.cutoutHeightCells = 2.0;
    layout.openings.push_back(std::move(opening));
  };
  addWallDoor("door_wall_h", 0U, 2.0,
              openDoor(cr::CreativeDoorHingeSide::MinimumEdge,
                       cr::CreativeDoorSwingSide::PositiveNormal));
  addWallDoor("door_wall_v", 1U, 4.0,
              openDoor(cr::CreativeDoorHingeSide::MaximumEdge,
                       cr::CreativeDoorSwingSide::NegativeNormal));

  cr::CreativeWorldLayoutOpening window;
  window.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  window.wallIndex = 0U;
  window.kind = cr::CreativeBuildingOpeningKind::Window;
  window.window.insertKind = cr::CreativeWindowInsertKind::PairedShutters;
  window.stableKey = "window_wall_h";
  window.name = "Shutter Window";
  window.centerOffsetCells = 6.0;
  window.widthCells = 1.0;
  window.cutoutBottomCells = 1.0;
  window.cutoutHeightCells = 1.0;
  window.insertBottomCells = 1.0;
  window.insertHeightCells = 1.0;
  layout.openings.push_back(window);

  cr::CreativeWorldLayoutTerrainProfile terrain;
  terrain.stableKey = "unowned_terrain";
  terrain.center = {12, 22};
  layout.terrainProfiles.push_back(terrain);
  return layout;
}

cr::CreativeWorldLayout roofApertureBuildingLayout() {
  cr::CreativeWorldLayout layout = transformableBuildingLayout();
  cr::CreativeWorldLayoutRoofAperture aperture;
  aperture.levelIndex = 0U;
  aperture.kind = cr::CreativeStructuralRoofApertureKind::Skylight;
  aperture.stableKey = "roof_skylight";
  aperture.name = "Roof Skylight";
  aperture.minimumXCells = 12.25;
  aperture.maximumXCells = 13.75;
  aperture.minimumZCells = 21.0;
  aperture.maximumZCells = 22.5;
  layout.roofApertures.push_back(std::move(aperture));
  return layout;
}

cr::CreativeWorldLayout verticalConnectorBuildingLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "vertical_connector_layout";
  layout.buildings.push_back({"vertical_building",
                              "Vertical Building",
                              cr::CreativeBuildingRootMode::CreateRoom,
                              {{0, 0}, {8, 6}},
                              0,
                              4U,
                              true,
                              {}});
  layout.levels = {
      {0U, "vertical_level_0", "Ground", 0.0, 4U, 1U, 1U, 1U},
      {0U, "vertical_level_1", "Upper", 4.0, 4U, 1U, 1U, 1U},
  };
  layout.rooms = {
      {0U, 0U, "vertical_room_0", "Ground Room", {{0, 0}, {8, 6}}, 0.25},
      {0U, 1U, "vertical_room_1", "Upper Room", {{0, 0}, {8, 6}}, 0.25},
  };
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "vertical_stair",
       "Main Stair",
       {{1, 2}, {5, 4}}});
  return layout;
}

bool sameBuildingTransformGeometry(const cr::CreativeWorldLayout &lhs,
                                   const cr::CreativeWorldLayout &rhs) {
  if (lhs.buildings.size() != rhs.buildings.size() ||
      lhs.levels.size() != rhs.levels.size() ||
      lhs.rooms.size() != rhs.rooms.size() ||
      lhs.boxes.size() != rhs.boxes.size() ||
      lhs.walls.size() != rhs.walls.size() ||
      lhs.openings.size() != rhs.openings.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& left = lhs.levels[index];
    const cr::CreativeWorldLayoutLevel& right = rhs.levels[index];
    if (left.buildingIndex != right.buildingIndex ||
        left.stableKey != right.stableKey || left.name != right.name ||
        left.floorTopLayer != right.floorTopLayer ||
        left.wallHeightCells != right.wallHeightCells ||
        left.floorThicknessLayers != right.floorThicknessLayers ||
        left.ceilingThicknessLayers != right.ceilingThicknessLayers ||
        left.roofThicknessLayers != right.roofThicknessLayers ||
        left.roofStyle != right.roofStyle ||
        left.roofRidgeAxis != right.roofRidgeAxis ||
        left.roofPitchDegrees != right.roofPitchDegrees ||
        left.roofOverhangCells != right.roofOverhangCells ||
        left.roofSlopeDirection != right.roofSlopeDirection ||
        left.roofMaterial != right.roofMaterial) {
      return false;
    }
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
        !(left.door == right.door) || !(left.window == right.window) ||
        left.facing != right.facing) {
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
                 layout.openings[0].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MaximumEdge &&
                 layout.openings[0].door.swingSide ==
                     cr::CreativeDoorSwingSide::NegativeNormal &&
                 layout.openings[0].facing ==
                     cr::CreativeBuildingOpeningFacing::NegativeNormal &&
                 layout.openings[1].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::South &&
                 layout.openings[1].centerOffsetCells == 3.0 &&
                 layout.openings[1].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MaximumEdge &&
                 layout.openings[1].door.swingSide ==
                     cr::CreativeDoorSwingSide::PositiveNormal &&
                 layout.openings[1].facing ==
                     cr::CreativeBuildingOpeningFacing::PositiveNormal &&
                 layout.openings[2].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::West &&
                 layout.openings[2].centerOffsetCells == 2.0 &&
                 layout.openings[2].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MinimumEdge &&
                 layout.openings[2].door.swingSide ==
                     cr::CreativeDoorSwingSide::PositiveNormal &&
                 layout.openings[3].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::North &&
                 layout.openings[3].centerOffsetCells == 1.0 &&
                 layout.openings[3].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MinimumEdge &&
                 layout.openings[3].door.swingSide ==
                     cr::CreativeDoorSwingSide::PositiveNormal,
             "room-edge direction, offset, hinge, and normal remap together") &&
         expect(
             layout.openings[4].centerOffsetCells == 2.0 &&
                 layout.openings[4].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MaximumEdge &&
                 layout.openings[4].door.swingSide ==
                     cr::CreativeDoorSwingSide::NegativeNormal &&
                 layout.openings[5].centerOffsetCells == 4.0 &&
                 layout.openings[5].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MinimumEdge &&
                 layout.openings[5].door.swingSide ==
                     cr::CreativeDoorSwingSide::NegativeNormal &&
                 layout.openings[6].window.insertKind ==
                     cr::CreativeWindowInsertKind::PairedShutters,
             "standalone-wall offsets and window treatment survive transform") &&
         expect(
             mirrorX.accepted && mirrorZ.accepted &&
                 mirrorX.transformed.openings[0].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::North &&
                 mirrorX.transformed.openings[0].centerOffsetCells == 5.0 &&
                 mirrorX.transformed.openings[0].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MaximumEdge &&
                 mirrorX.transformed.openings[0].door.swingSide ==
                     cr::CreativeDoorSwingSide::PositiveNormal &&
                 mirrorX.transformed.openings[1].roomEdge ==
                     cr::CreativeWorldLayoutRoomEdge::West &&
                 mirrorX.transformed.openings[1].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MaximumEdge &&
                 mirrorX.transformed.openings[1].door.swingSide ==
                     cr::CreativeDoorSwingSide::NegativeNormal &&
                 mirrorX.transformed.openings[0].facing ==
                     cr::CreativeBuildingOpeningFacing::PositiveNormal &&
                 mirrorX.transformed.openings[1].facing ==
                     cr::CreativeBuildingOpeningFacing::NegativeNormal &&
                 mirrorX.transformed.openings[5].door.hingeSide ==
                     cr::CreativeDoorHingeSide::MinimumEdge &&
                 mirrorX.transformed.openings[5].door.swingSide ==
                     cr::CreativeDoorSwingSide::PositiveNormal,
             "mirror X reverses canonical edges and vertical normals") &&
         expect(mirrorZ.transformed.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::South &&
                    mirrorZ.transformed.openings[0].door.hingeSide ==
                        cr::CreativeDoorHingeSide::MaximumEdge &&
                    mirrorZ.transformed.openings[0].door.swingSide ==
                        cr::CreativeDoorSwingSide::NegativeNormal &&
                    mirrorZ.transformed.openings[1].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    mirrorZ.transformed.openings[1].centerOffsetCells == 3.0 &&
                    mirrorZ.transformed.openings[1].door.hingeSide ==
                        cr::CreativeDoorHingeSide::MaximumEdge &&
                    mirrorZ.transformed.openings[1].door.swingSide ==
                        cr::CreativeDoorSwingSide::PositiveNormal &&
                    mirrorZ.transformed.openings[0].facing ==
                        cr::CreativeBuildingOpeningFacing::NegativeNormal &&
                    mirrorZ.transformed.openings[1].facing ==
                        cr::CreativeBuildingOpeningFacing::PositiveNormal &&
                    mirrorZ.transformed.openings[4].door.hingeSide ==
                        cr::CreativeDoorHingeSide::MaximumEdge &&
                    mirrorZ.transformed.openings[4].door.swingSide ==
                        cr::CreativeDoorSwingSide::NegativeNormal,
                "mirror Z reverses canonical edges and horizontal normals") &&
         expect(layout.terrainProfiles[0].center ==
                    source.terrainProfiles[0].center,
                "unowned terrain remains outside a building transform") &&
         expect(compiled.receipt.accepted,
                "transformed hosted openings remain exact 3D compilable");
}

bool buildingTransformsRoundTripAndRejectOverflow() {
  cr::CreativeWorldLayout source = transformableBuildingLayout();
  source.levels[0].roofStyle = cr::CreativeStructuralRoofStyle::Shed;
  source.levels[0].roofRidgeAxis =
      cr::CreativeStructuralRoofRidgeAxis::X;
  source.levels[0].roofSlopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::PositiveX;
  source.levels[0].roofMaterial = cr::CreativeStructuralMaterial::Stone;
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
  const auto mirroredX = cr::transformCreativeWorldLayoutBuilding(
      source,
      {0U, cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX});
  cr::CreativeWorldLayout grounded = source;
  grounded.buildings[0].rootMode = cr::CreativeBuildingRootMode::None;
  grounded.buildings[0].groundingMode =
      cr::CreativeWorldLayoutGroundingMode::Foundation;
  const auto groundedTurn = cr::transformCreativeWorldLayoutBuilding(
      grounded,
      {0U, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});

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
         expect(right.transformed.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::Z &&
                    right.transformed.levels[0].roofSlopeDirection ==
                        cr::CreativeStructuralRoofSlopeDirection::PositiveZ &&
                    right.transformed.levels[0].roofMaterial ==
                        cr::CreativeStructuralMaterial::Stone &&
                    mirroredX.accepted &&
                    mirroredX.transformed.levels[0].roofSlopeDirection ==
                        cr::CreativeStructuralRoofSlopeDirection::NegativeX &&
                    mirroredX.transformed.levels[0].roofMaterial ==
                        cr::CreativeStructuralMaterial::Stone,
                "roof axis and downhill direction transform while material persists") &&
         expect(groundedTurn.accepted &&
                    groundedTurn.transformed.buildings[0].rootFootprint
                            .minimum ==
                        cr::CreativeTerrainCoord2{10, 20} &&
                    groundedTurn.transformed.buildings[0].rootFootprint
                            .maximum ==
                        cr::CreativeTerrainCoord2{16, 28},
                "grounded root footprint follows a rootless building transform") &&
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

bool buildingEditKernelsAreAtomicAndRemapOwnership() {
  const cr::CreativeWorldLayout source = transformableBuildingLayout();

  std::int64_t defaultDeltaX = 0;
  std::int64_t defaultDeltaZ = 0;
  const bool defaultOffset =
      cr::defaultCreativeWorldLayoutBuildingDuplicateOffset(
          source, 0U, defaultDeltaX, defaultDeltaZ);

  const cr::CreativeWorldLayoutBuildingEditResult moved =
      cr::moveCreativeWorldLayoutBuilding(source, {0U, 3, -2});
  const bool moveExact =
      moved.accepted && moved.changed &&
      moved.status == cr::CreativeWorldLayoutBuildingEditStatus::Ready &&
      source.buildings[0].rootFootprint.minimum ==
          cr::CreativeTerrainCoord2{10, 20} &&
      moved.edited.buildings[0].rootFootprint.minimum ==
          cr::CreativeTerrainCoord2{13, 18} &&
      moved.edited.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{14, 19} &&
      moved.edited.boxes[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{21, 24} &&
      moved.edited.walls[0].start == cr::CreativeTerrainCoord2{13, 21} &&
      moved.edited.openings[0].centerOffsetCells ==
          source.openings[0].centerOffsetCells &&
      moved.edited.terrainProfiles[0].center ==
          source.terrainProfiles[0].center;

  const cr::CreativeWorldLayoutBuildingEditResult overflow =
      cr::moveCreativeWorldLayoutBuilding(
          source, {0U, std::numeric_limits<std::int64_t>::max(), 0});
  const bool overflowAtomic =
      !overflow.accepted && !overflow.changed &&
      overflow.status ==
          cr::CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow &&
      overflow.edited.buildings.empty() &&
      source.walls[0].start == cr::CreativeTerrainCoord2{10, 23};

  const cr::CreativeWorldLayoutBuildingEditResult duplicated =
      cr::duplicateCreativeWorldLayoutBuilding(source, {0U, 20, 0, 40U});
  const bool duplicateExact =
      duplicated.accepted && duplicated.changed &&
      duplicated.resultBuildingIndex == 1U &&
      duplicated.nextStableOrdinal == 53U &&
      duplicated.edited.buildings.size() == 2U &&
      duplicated.edited.levels.size() == 2U &&
      duplicated.edited.rooms.size() == 2U &&
      duplicated.edited.boxes.size() == 2U &&
      duplicated.edited.walls.size() == 4U &&
      duplicated.edited.openings.size() == 14U &&
      duplicated.edited.buildings[1].stableKey == "building_40" &&
      duplicated.edited.buildings[1].rootFootprint.minimum ==
          cr::CreativeTerrainCoord2{30, 20} &&
      duplicated.edited.rooms[1].buildingIndex == 1U &&
      duplicated.edited.rooms[1].levelIndex == 1U &&
      duplicated.edited.levels[1].buildingIndex == 1U &&
      duplicated.edited.walls[2].buildingIndex == 1U &&
      duplicated.edited.openings[7].roomIndex == 1U &&
      duplicated.edited.openings[11].wallIndex == 2U &&
      duplicated.edited.terrainProfiles.size() == 1U;

  const cr::CreativeWorldLayoutBuildingEditResult removed =
      cr::deleteCreativeWorldLayoutBuilding(duplicated.edited, {0U});
  const bool deleteExact =
      removed.accepted && removed.changed &&
      removed.edited.buildings.size() == 1U &&
      removed.edited.levels.size() == 1U &&
      removed.edited.rooms.size() == 1U &&
      removed.edited.boxes.size() == 1U &&
      removed.edited.walls.size() == 2U &&
      removed.edited.openings.size() == 7U &&
      removed.edited.rooms[0].buildingIndex == 0U &&
      removed.edited.rooms[0].levelIndex == 0U &&
      removed.edited.walls[0].buildingIndex == 0U &&
      removed.edited.openings[0].roomIndex == 0U &&
      removed.edited.openings[4].wallIndex == 0U &&
      removed.edited.buildings[0].rootFootprint.minimum ==
          cr::CreativeTerrainCoord2{30, 20} &&
      removed.edited.terrainProfiles.size() == 1U;

  cr::CreativeWorldLayout invalid = source;
  invalid.openings[0].roomEdge = cr::CreativeWorldLayoutRoomEdge::Count;
  const cr::CreativeWorldLayoutBuildingEditResult invalidDuplicate =
      cr::duplicateCreativeWorldLayoutBuilding(invalid, {0U, 20, 0, 1U});
  const cr::CreativeWorldLayoutBuildingEditResult invalidDelete =
      cr::deleteCreativeWorldLayoutBuilding(invalid, {0U});

  return expect(defaultOffset && defaultDeltaX == 10 && defaultDeltaZ == 0,
                "building duplicate offset comes from aggregate bounds") &&
         expect(moveExact,
                "building move edits all owned geometry but not terrain") &&
         expect(overflowAtomic,
                "building move overflow publishes no partial candidate") &&
         expect(duplicateExact,
                "building duplicate remaps owners, hosts, and stable keys") &&
         expect(deleteExact,
                "building delete compacts owners and opening hosts") &&
         expect(!invalidDuplicate.accepted && !invalidDelete.accepted &&
                    invalidDuplicate.status ==
                        cr::CreativeWorldLayoutBuildingEditStatus::
                            InvalidOwnership &&
                    invalidDelete.status ==
                        cr::CreativeWorldLayoutBuildingEditStatus::
                            InvalidOwnership,
                "building edits reject malformed ownership before copying");
}

bool roofAperturesFollowBuildingOwnershipAndTemplateSync() {
  const cr::CreativeWorldLayout source = roofApertureBuildingLayout();
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint sourceFingerprint =
      cr::fingerprintCreativeWorldLayoutBuilding(source, 0U);
  cr::CreativeWorldLayout changedSource = source;
  changedSource.roofApertures[0].maximumXCells += 0.25;
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint changedFingerprint =
      cr::fingerprintCreativeWorldLayoutBuilding(changedSource, 0U);

  const cr::CreativeWorldLayoutBuildingEditResult moved =
      cr::moveCreativeWorldLayoutBuilding(source, {0U, 3, -2});
  const cr::CreativeWorldLayoutBuildingTransformResult rotated =
      cr::transformCreativeWorldLayoutBuilding(
          source,
          {0U,
           cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const cr::CreativeWorldLayoutBuildingEditResult duplicated =
      cr::duplicateCreativeWorldLayoutBuilding(source, {0U, 20, 0, 40U});
  const std::string duplicateApertureKey =
      duplicated.accepted && duplicated.edited.roofApertures.size() == 2U
          ? duplicated.edited.roofApertures[1].stableKey
          : std::string{};
  const cr::CreativeWorldLayoutBuildingEditResult removed =
      duplicated.accepted
          ? cr::deleteCreativeWorldLayoutBuilding(duplicated.edited, {0U})
          : cr::CreativeWorldLayoutBuildingEditResult{};

  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          source, {0U, "aperture_house", "Aperture House"});
  cr::CreativeWorldLayout destination;
  destination.stableKey = "aperture_destination";
  const cr::CreativeWorldLayoutBuildingEditResult stamped =
      captured.accepted
          ? cr::stampCreativeWorldLayoutBuildingTemplate(
                destination, captured.value, {{30, 40}, 100U, false})
          : cr::CreativeWorldLayoutBuildingEditResult{};

  cr::CreativeWorldLayoutBuildingTemplateResult updated;
  if (captured.accepted) {
    cr::CreativeWorldLayout updatedLayout = captured.value.normalizedLayout;
    updatedLayout.roofApertures[0].maximumXCells += 0.25;
    updated = cr::loadCreativeWorldLayoutBuildingTemplate(
        std::move(updatedLayout));
  }
  const cr::CreativeWorldLayoutBuildingTemplateSyncReceipt sourceChanged =
      stamped.accepted && updated.accepted
          ? cr::inspectCreativeWorldLayoutBuildingTemplateSync(
                stamped.edited, 0U, &updated.value)
          : cr::CreativeWorldLayoutBuildingTemplateSyncReceipt{};
  const std::string stampedApertureKey =
      stamped.accepted && stamped.edited.roofApertures.size() == 1U
          ? stamped.edited.roofApertures[0].stableKey
          : std::string{};
  const cr::CreativeWorldLayoutBuildingTemplateRefreshResult refreshed =
      stamped.accepted && updated.accepted
          ? cr::refreshCreativeWorldLayoutBuildingTemplateInstances(
                stamped.edited,
                {&updated.value,
                 cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                     SafeInstances,
                 cr::kInvalidCreativeWorldLayoutIndex,
                 stamped.nextStableOrdinal})
          : cr::CreativeWorldLayoutBuildingTemplateRefreshResult{};
  const cr::CreativeWorldLayoutBuildingTemplateSyncReceipt currentAfterRefresh =
      refreshed.accepted
          ? cr::inspectCreativeWorldLayoutBuildingTemplateSync(
                refreshed.edited, 0U, &updated.value)
          : cr::CreativeWorldLayoutBuildingTemplateSyncReceipt{};

  return expect(sourceFingerprint.valid && changedFingerprint.valid &&
                    sourceFingerprint.value != changedFingerprint.value,
                "roof aperture source participates in building fingerprint") &&
         expect(moved.accepted && moved.edited.roofApertures.size() == 1U &&
                    moved.edited.roofApertures[0].minimumXCells == 15.25 &&
                    moved.edited.roofApertures[0].maximumXCells == 16.75 &&
                    moved.edited.roofApertures[0].minimumZCells == 19.0 &&
                    moved.edited.roofApertures[0].maximumZCells == 20.5,
                "building move offsets exact roof aperture bounds") &&
         expect(rotated.accepted &&
                    rotated.transformed.roofApertures.size() == 1U &&
                    rotated.transformed.roofApertures[0].minimumXCells == 13.5 &&
                    rotated.transformed.roofApertures[0].maximumXCells == 15.0 &&
                    rotated.transformed.roofApertures[0].minimumZCells == 22.25 &&
                    rotated.transformed.roofApertures[0].maximumZCells == 23.75,
                "building rotation maps roof aperture corners exactly") &&
         expect(duplicated.accepted &&
                    duplicated.edited.roofApertures.size() == 2U &&
                    duplicated.edited.roofApertures[1].levelIndex == 1U &&
                    duplicateApertureKey != "roof_skylight" &&
                    duplicated.edited.roofApertures[1].minimumXCells == 32.25 &&
                    duplicated.edited.roofApertures[1].maximumZCells == 22.5,
                "building duplicate remaps and rekeys roof aperture source") &&
         expect(removed.accepted &&
                    removed.edited.roofApertures.size() == 1U &&
                    removed.edited.roofApertures[0].levelIndex == 0U &&
                    removed.edited.roofApertures[0].stableKey ==
                        duplicateApertureKey &&
                    removed.edited.roofApertures[0].minimumXCells == 32.25,
                "building delete removes owned aperture and compacts level owner") &&
         expect(captured.accepted &&
                    captured.value.normalizedLayout.roofApertures.size() == 1U &&
                    captured.value.normalizedLayout.roofApertures[0]
                            .minimumXCells == 2.25 &&
                    captured.value.normalizedLayout.roofApertures[0]
                            .minimumZCells == 1.0,
                "template capture normalizes roof aperture plan bounds") &&
         expect(stamped.accepted && stampedApertureKey != "roof_skylight" &&
                    stamped.edited.roofApertures.size() == 1U &&
                    stamped.edited.roofApertures[0].levelIndex == 0U &&
                    stamped.edited.roofApertures[0].minimumXCells == 32.25 &&
                    stamped.edited.roofApertures[0].maximumZCells == 42.5,
                "template stamp restores aperture geometry with fresh identity") &&
         expect(updated.accepted &&
                    sourceChanged.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            SourceChanged,
                "template sync observes changed aperture semantics") &&
         expect(refreshed.accepted &&
                    refreshed.refreshedInstanceCount == 1U &&
                    refreshed.edited.roofApertures.size() == 1U &&
                    refreshed.edited.roofApertures[0].stableKey ==
                        stampedApertureKey &&
                    refreshed.edited.roofApertures[0].maximumXCells == 34.0 &&
                    currentAfterRefresh.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
                "template refresh preserves aperture identity and updates bounds");
}

bool verticalConnectorOwnershipFollowsBuildingKernels() {
  const cr::CreativeWorldLayout source = verticalConnectorBuildingLayout();
  const cr::CreativeWorldLayoutBuildingEditResult moved =
      cr::moveCreativeWorldLayoutBuilding(source, {0U, 3, -2});
  const cr::CreativeWorldLayoutBuildingTransformResult rotated =
      cr::transformCreativeWorldLayoutBuilding(
          source,
          {0U,
           cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const cr::CreativeWorldLayoutBuildingEditResult duplicated =
      cr::duplicateCreativeWorldLayoutBuilding(source, {0U, 20, 0, 10U});
  const cr::CreativeWorldLayoutBuildingEditResult removed =
      duplicated.accepted
          ? cr::deleteCreativeWorldLayoutBuilding(duplicated.edited, {0U})
          : cr::CreativeWorldLayoutBuildingEditResult{};
  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          source, {0U, "vertical_template", "Vertical Template"});
  cr::CreativeWorldLayout destination;
  destination.stableKey = "vertical_destination";
  const cr::CreativeWorldLayoutBuildingEditResult stamped =
      captured.accepted
          ? cr::stampCreativeWorldLayoutBuildingTemplate(
                destination, captured.value, {{10, 10}, 100U, false})
          : cr::CreativeWorldLayoutBuildingEditResult{};

  return expect(moved.accepted &&
                    moved.edited.verticalConnectors.size() == 1U &&
                    moved.edited.verticalConnectors[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{4, 0} &&
                    moved.edited.verticalConnectors[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 2},
                "building move offsets its connector footprint") &&
         expect(
             rotated.accepted &&
                 rotated.transformed.verticalConnectors.size() == 1U &&
                 rotated.transformed.verticalConnectors[0].direction ==
                     cr::CreativeWorldLayoutVerticalDirection::PositiveZ,
             "building transform rotates connector geometry and direction") &&
         expect(
             duplicated.accepted &&
                 duplicated.edited.verticalConnectors.size() == 2U &&
                 duplicated.edited.verticalConnectors[1].buildingIndex == 1U &&
                 duplicated.edited.verticalConnectors[1].lowerRoomIndex == 2U &&
                 duplicated.edited.verticalConnectors[1].upperRoomIndex == 3U &&
                 duplicated.edited.verticalConnectors[1].footprint.minimum ==
                     cr::CreativeTerrainCoord2{21, 2},
             "building duplicate remaps connector owner and room references") &&
         expect(removed.accepted && removed.edited.buildings.size() == 1U &&
                    removed.edited.verticalConnectors.size() == 1U &&
                    removed.edited.verticalConnectors[0].buildingIndex == 0U &&
                    removed.edited.verticalConnectors[0].lowerRoomIndex == 0U &&
                    removed.edited.verticalConnectors[0].upperRoomIndex == 1U,
                "building delete compacts surviving connector ownership") &&
         expect(captured.accepted &&
                    captured.value.normalizedLayout.verticalConnectors.size() ==
                        1U &&
                    stamped.accepted &&
                    stamped.edited.verticalConnectors.size() == 1U &&
                    stamped.edited.verticalConnectors[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{11, 12} &&
                    stamped.edited.verticalConnectors[0].lowerRoomIndex == 0U &&
                    stamped.edited.verticalConnectors[0].upperRoomIndex == 1U,
                "template capture and stamp preserve connector ownership");
}

bool explicitTopologyFollowsBuildingOwnershipKernels() {
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(
          transformableBuildingLayout());
  if (!expect(materialized.accepted && materialized.changed,
              "building topology fixture materializes")) {
    return false;
  }
  const cr::CreativeWorldLayout& source = materialized.edited;
  const cr::CreativeWorldLayoutRoomGraph sourceGraph =
      cr::buildCreativeWorldLayoutRoomGraph(source);
  const std::size_t sourceVertexCount = source.topologyVertices.size();
  const std::size_t sourceEdgeCount = source.topologyEdges.size();
  const std::size_t sourceBoundaryCount = source.roomBoundaries.size();
  const cr::CreativeTerrainCoord2 firstVertex =
      source.topologyVertices.front().position;
  const std::string firstEdgeKey = source.topologyEdges.front().stableKey;

  const cr::CreativeWorldLayoutBuildingEditResult moved =
      cr::moveCreativeWorldLayoutBuilding(source, {0U, 3, -2});
  const cr::CreativeWorldLayoutBuildingTransformResult rotated =
      cr::transformCreativeWorldLayoutBuilding(
          source,
          {0U,
           cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const cr::CreativeWorldLayoutRoomGraph movedGraph =
      moved.accepted ? cr::buildCreativeWorldLayoutRoomGraph(moved.edited)
                     : cr::CreativeWorldLayoutRoomGraph{};
  const cr::CreativeWorldLayoutRoomGraph rotatedGraph =
      rotated.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(rotated.transformed)
          : cr::CreativeWorldLayoutRoomGraph{};

  const cr::CreativeWorldLayoutBuildingEditResult duplicated =
      cr::duplicateCreativeWorldLayoutBuilding(source, {0U, 20, 0, 100U});
  const cr::CreativeWorldLayoutRoomGraph duplicatedGraph =
      duplicated.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(duplicated.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  bool duplicateDirectHostsRemapped = duplicated.accepted;
  for (std::size_t index = source.openings.size();
       duplicateDirectHostsRemapped && index < duplicated.edited.openings.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        duplicated.edited.openings[index];
    if (opening.hostKind ==
            cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex !=
            cr::kInvalidCreativeWorldLayoutIndex) {
      duplicateDirectHostsRemapped =
          opening.roomIndex >= source.rooms.size() &&
          opening.roomTopologyEdgeIndex >= sourceEdgeCount;
    }
  }

  const cr::CreativeWorldLayoutBuildingEditResult removed =
      duplicated.accepted
          ? cr::deleteCreativeWorldLayoutBuilding(duplicated.edited, {0U})
          : cr::CreativeWorldLayoutBuildingEditResult{};
  const cr::CreativeWorldLayoutRoomGraph removedGraph =
      removed.accepted ? cr::buildCreativeWorldLayoutRoomGraph(removed.edited)
                       : cr::CreativeWorldLayoutRoomGraph{};

  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          source, {0U, "topology_house", "Topology House"});
  const cr::CreativeWorldLayoutBuildingTemplateResult rotatedTemplate =
      captured.accepted
          ? cr::transformCreativeWorldLayoutBuildingTemplate(
                captured.value,
                cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX)
          : cr::CreativeWorldLayoutBuildingTemplateResult{};
  cr::CreativeWorldLayout destination;
  destination.stableKey = "topology_destination";
  const cr::CreativeWorldLayoutBuildingEditResult stamped =
      rotatedTemplate.accepted
          ? cr::stampCreativeWorldLayoutBuildingTemplate(
                destination, rotatedTemplate.value, {{30, 40}, 500U, false})
          : cr::CreativeWorldLayoutBuildingEditResult{};
  const cr::CreativeWorldLayoutRoomGraph stampedGraph =
      stamped.accepted ? cr::buildCreativeWorldLayoutRoomGraph(stamped.edited)
                       : cr::CreativeWorldLayoutRoomGraph{};

  cr::CreativeWorldLayoutBuildingTemplateResult updated;
  if (captured.accepted) {
    cr::CreativeWorldLayout revised = captured.value.normalizedLayout;
    revised.topologyEdges.front().material =
        cr::CreativeStructuralMaterial::Stone;
    updated = cr::loadCreativeWorldLayoutBuildingTemplate(std::move(revised));
  }
  const std::string stampedEdgeKey =
      stamped.accepted ? stamped.edited.topologyEdges.front().stableKey : "";
  const cr::CreativeWorldLayoutBuildingTemplateRefreshResult refreshed =
      stamped.accepted && updated.accepted
          ? cr::refreshCreativeWorldLayoutBuildingTemplateInstances(
                stamped.edited,
                {&updated.value,
                 cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                     SafeInstances,
                 cr::kInvalidCreativeWorldLayoutIndex,
                 stamped.nextStableOrdinal})
          : cr::CreativeWorldLayoutBuildingTemplateRefreshResult{};
  const cr::CreativeWorldLayoutRoomGraph refreshedGraph =
      refreshed.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(refreshed.edited)
          : cr::CreativeWorldLayoutRoomGraph{};

  return expect(sourceGraph.accepted && sourceGraph.sourceWasExplicit,
                "canonical building starts with a valid explicit graph") &&
         expect(moved.accepted && movedGraph.accepted &&
                    moved.edited.topologyVertices.front().position ==
                        cr::CreativeTerrainCoord2{firstVertex.x + 3,
                                                  firstVertex.z - 2} &&
                    moved.edited.topologyEdges.front().stableKey == firstEdgeKey,
                "building move offsets topology without identity churn") &&
         expect(rotated.accepted && rotatedGraph.accepted &&
                    rotated.transformed.topologyEdges.front().stableKey ==
                        firstEdgeKey,
                "building transform preserves a valid canonical graph") &&
         expect(duplicated.accepted && duplicatedGraph.accepted &&
                    duplicated.edited.topologyVertices.size() ==
                        sourceVertexCount * 2U &&
                    duplicated.edited.topologyEdges.size() ==
                        sourceEdgeCount * 2U &&
                    duplicated.edited.roomBoundaries.size() ==
                        sourceBoundaryCount * 2U &&
                    duplicateDirectHostsRemapped,
                "building duplicate remaps topology and direct opening hosts") &&
         expect(removed.accepted && removedGraph.accepted &&
                    removed.edited.buildings.size() == 1U &&
                    removed.edited.topologyVertices.size() ==
                        sourceVertexCount &&
                    removed.edited.topologyEdges.size() == sourceEdgeCount &&
                    removed.edited.roomBoundaries.size() ==
                        sourceBoundaryCount,
                "building delete compacts surviving topology atomically") &&
         expect(captured.accepted,
                "template capture preserves topology") &&
         expect(rotatedTemplate.accepted,
                "template transform preserves topology") &&
         expect(stamped.accepted,
                "template stamp preserves topology") &&
         expect(stampedGraph.accepted &&
                    stamped.edited.topologyEdges.size() == sourceEdgeCount,
                "stamped topology remains a valid explicit graph") &&
         expect(updated.accepted && refreshed.accepted &&
                    refreshed.refreshedInstanceCount == 1U &&
                    refreshedGraph.accepted &&
                    refreshed.edited.topologyEdges.front().stableKey ==
                        stampedEdgeKey &&
                    refreshed.edited.topologyEdges.front().material ==
                        cr::CreativeStructuralMaterial::Stone,
                "linked template refresh keeps wall identity and updates semantics");
}

bool buildingTemplatesNormalizeTransformPersistAndStamp() {
  const cr::CreativeWorldLayout source = transformableBuildingLayout();
  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          source, {0U, "guard_house", "Guard House"});
  if (!captured.accepted) {
    return expect(false, "building template capture accepted");
  }
  const cr::CreativeWorldLayoutBuildingTemplate& value = captured.value;
  const bool normalized =
      cr::validCreativeWorldLayoutBuildingTemplate(value) &&
      value.bounds.minimum == cr::CreativeTerrainCoord2{} &&
      value.bounds.maximum == cr::CreativeTerrainCoord2{8, 6} &&
      value.normalizedLayout.buildings[0].rootFootprint.minimum ==
          cr::CreativeTerrainCoord2{} &&
      value.normalizedLayout.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{1, 1} &&
      value.normalizedLayout.walls[0].start ==
          cr::CreativeTerrainCoord2{0, 3} &&
      value.normalizedLayout.openings[0].roomIndex == 0U &&
      value.normalizedLayout.openings[4].wallIndex == 0U &&
      value.normalizedLayout.terrainProfiles.empty();

  const cr::CreativeWorldLayoutBuildingTemplateResult rotated =
      cr::transformCreativeWorldLayoutBuildingTemplate(
          value,
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
  const bool orientationExact =
      rotated.accepted && rotated.value.bounds.minimum ==
                              cr::CreativeTerrainCoord2{} &&
      rotated.value.bounds.maximum == cr::CreativeTerrainCoord2{6, 8} &&
      rotated.value.normalizedLayout.openings[0].roomEdge ==
          cr::CreativeWorldLayoutRoomEdge::East;

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(value.normalizedLayout);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  cr::CreativeWorldLayoutBuildingTemplateResult reloaded;
  if (decoded.accepted) {
    reloaded = cr::loadCreativeWorldLayoutBuildingTemplate(decoded.layout);
  }
  const bool codecRoundTrip =
      reloaded.accepted && reloaded.value.templateId == "guard_house" &&
      reloaded.value.label == "Guard House" &&
      reloaded.value.bounds.maximum == cr::CreativeTerrainCoord2{8, 6};

  cr::CreativeWorldLayout destination;
  destination.stableKey = "destination";
  const cr::CreativeWorldLayoutBuildingEditResult stamped =
      cr::stampCreativeWorldLayoutBuildingTemplate(
          destination, rotated.value, {{-4, 7}, 100U, false});
  const bool stampExact =
      stamped.accepted && stamped.changed &&
      stamped.resultBuildingIndex == 0U &&
      stamped.nextStableOrdinal == 113U &&
      stamped.edited.buildings[0].stableKey == "building_100" &&
      stamped.edited.levels.size() == 1U &&
      stamped.edited.levels[0].buildingIndex == 0U &&
      stamped.edited.buildings[0].name == "Guard House" &&
      stamped.edited.buildings[0].rootFootprint.minimum ==
          cr::CreativeTerrainCoord2{-4, 7} &&
      stamped.edited.rooms[0].buildingIndex == 0U &&
      stamped.edited.rooms[0].levelIndex == 0U &&
      stamped.edited.walls[0].buildingIndex == 0U &&
      stamped.edited.openings[0].roomIndex == 0U &&
      stamped.edited.openings[4].wallIndex == 0U &&
      stamped.edited.terrainProfiles.empty();

  cr::CreativeWorldLayoutBuildingTemplate invalid = value;
  invalid.normalizedLayout.terrainProfiles.push_back(
      source.terrainProfiles[0]);

  cr::CreativeWorldLayoutBuildingTemplate invalidObject = value;
  cr::CreativeWorldLayoutObject object;
  object.stableKey = "unowned_boulder";
  object.name = "Unowned Boulder";
  object.kind = cr::CreativeObjectKind::Rock;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.assetId = "boulder_01";
  invalidObject.normalizedLayout.objects.push_back(std::move(object));

  return expect(normalized,
                "building template capture normalizes semantic geometry") &&
         expect(orientationExact,
                "building template orientation keeps an origin anchor") &&
         expect(codecRoundTrip,
                "building template survives the versioned layout codec") &&
         expect(stampExact,
                "building template stamp allocates fresh owners and hosts") &&
         expect(!cr::validCreativeWorldLayoutBuildingTemplate(invalid),
                "building template cannot absorb unowned terrain") &&
         expect(!cr::validCreativeWorldLayoutBuildingTemplate(invalidObject),
                "building template cannot absorb unowned objects");
}

bool buildingTemplatePlacementAnalysisAndDetachAreExact() {
  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          transformableBuildingLayout(),
          {0U, "placement_house", "Placement House"});
  if (!captured.accepted) {
    return expect(false, "placement analysis template capture accepted");
  }

  cr::CreativeWorldLayout emptyDestination;
  emptyDestination.stableKey = "placement_destination";
  const cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis clear =
      cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
          {&emptyDestination, &captured.value, {30, 40}, {}, nullptr});
  const cr::CreativeWorldLayoutBuildingEditResult existing =
      cr::stampCreativeWorldLayoutBuildingTemplate(
          emptyDestination, captured.value, {{30, 40}, 100U, false});
  const cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis overlap =
      existing.accepted
          ? cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
                {&existing.edited, &captured.value, {30, 40}, {}, nullptr})
          : cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis{};
  const cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis adjacent =
      existing.accepted
          ? cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
                {&existing.edited, &captured.value, {38, 40}, {}, nullptr})
          : cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis{};
  const cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis overflow =
      cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
          {&emptyDestination,
           &captured.value,
           {std::numeric_limits<std::int32_t>::max(), 40},
           {},
           nullptr});

  const cr::CreativeWorldLayoutBuildingTemplateFingerprint beforeDetach =
      existing.accepted
          ? cr::fingerprintCreativeWorldLayoutBuilding(existing.edited, 0U)
          : cr::CreativeWorldLayoutBuildingTemplateFingerprint{};
  const std::string stableKey =
      existing.accepted ? existing.edited.buildings[0].stableKey : std::string{};
  const cr::CreativeWorldLayoutBuildingEditResult detached =
      existing.accepted
          ? cr::detachCreativeWorldLayoutBuildingTemplateInstance(
                existing.edited, 0U)
          : cr::CreativeWorldLayoutBuildingEditResult{};
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint afterDetach =
      detached.accepted
          ? cr::fingerprintCreativeWorldLayoutBuilding(detached.edited, 0U)
          : cr::CreativeWorldLayoutBuildingTemplateFingerprint{};
  const cr::CreativeWorldLayoutBuildingEditResult detachedAgain =
      detached.accepted
          ? cr::detachCreativeWorldLayoutBuildingTemplateInstance(
                detached.edited, 0U)
          : cr::CreativeWorldLayoutBuildingEditResult{};

  const cr::CreativeWorldLayoutBuildingTemplateResult smallHouse =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          smallHouseLayout(), {0U, "foundation_house", "Foundation House"});
  cr::CreativeWorldLayoutBuildingTemplateResult foundationTemplate;
  if (smallHouse.accepted) {
    cr::CreativeWorldLayout foundationLayout = smallHouse.value.normalizedLayout;
    foundationLayout.buildings[0].groundingMode =
        cr::CreativeWorldLayoutGroundingMode::Foundation;
    foundationLayout.buildings[0].maximumGroundReliefCells = 1U;
    foundationTemplate =
        cr::loadCreativeWorldLayoutBuildingTemplate(std::move(foundationLayout));
  }
  cr::CreativeDocument terrainDocument = makeDocument(91U);
  std::vector<std::uint16_t> heights(24U, 2U);
  for (std::size_t index = 1U; index < heights.size(); index += 2U) {
    heights[index] = 3U;
  }
  const cr::CreativeTerrainHeightFieldReplaceReceipt terrainReceipt =
      terrainDocument.replaceTerrainHeightField({{0, 0}, 6U, 4U}, heights);
  const cr::CreativeTerrainSurfacePlan terrainSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          terrainDocument.terrainField(), terrainDocument.terrainHeightField());
  const cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis foundation =
      foundationTemplate.accepted && terrainSurface.accepted
          ? cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
                {&emptyDestination, &foundationTemplate.value, {0, 0},
                 terrainDocument.gridSettings(), &terrainSurface})
          : cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis{};
  const cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis
      missingFoundationTerrain =
          foundationTemplate.accepted
              ? cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
                    {&emptyDestination, &foundationTemplate.value, {0, 0}, {},
                     nullptr})
              : cr::CreativeWorldLayoutBuildingTemplatePlacementAnalysis{};
  const cr::CreativeWorldLayoutBuildingEditResult foundationStamped =
      foundationTemplate.accepted
          ? cr::stampCreativeWorldLayoutBuildingTemplate(
                emptyDestination, foundationTemplate.value, {{0, 0}, 200U, false})
          : cr::CreativeWorldLayoutBuildingEditResult{};
  const cr::CreativeWorldLayoutCompileResult foundationCompiled =
      foundationStamped.accepted
          ? cr::buildCreativeWorldLayoutPlan(terrainDocument,
                                             foundationStamped.edited)
          : cr::CreativeWorldLayoutCompileResult{};

  return expect(clear.accepted &&
                    clear.status == cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::Ready &&
                    clear.templateVersion == captured.value.sourceFingerprint.value &&
                    clear.bounds.minimum == cr::CreativeTerrainCoord2{30, 40} &&
                    clear.bounds.maximum == cr::CreativeTerrainCoord2{38, 46} &&
                    clear.footprint.minimum == cr::CreativeTerrainCoord2{30, 40} &&
                    clear.footprint.maximum == cr::CreativeTerrainCoord2{38, 46} &&
                    clear.levelCount == 1U && clear.entranceCount == 6U &&
                    clear.terrainImpact ==
                        cr::CreativeWorldLayoutBuildingTemplateTerrainImpact::None,
                "placement analysis exposes version footprint entrances levels and terrain") &&
         expect(existing.accepted && !overlap.accepted &&
                    overlap.status ==
                        cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::
                            BuildingOverlap &&
                    overlap.conflictingBuildingIndex == 0U,
                "placement analysis rejects a vertically overlapping building") &&
         expect(adjacent.accepted &&
                    adjacent.conflictingBuildingIndex ==
                        cr::kInvalidCreativeWorldLayoutIndex,
                "placement analysis allows buildings whose footprints only touch") &&
         expect(!overflow.accepted &&
                    overflow.status ==
                        cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::
                            CoordinateOverflow,
                "placement analysis rejects coordinate overflow atomically") &&
         expect(detached.accepted && detached.changed && beforeDetach.valid &&
                    beforeDetach == afterDetach &&
                    detached.edited.buildings[0].stableKey == stableKey &&
                    !cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(
                         detached.edited, 0U)
                         .present &&
                    detachedAgain.accepted && !detachedAgain.changed,
                "template detach preserves content identity and is idempotent") &&
         expect(terrainReceipt.accepted && foundation.accepted &&
                    foundation.terrainImpact ==
                        cr::CreativeWorldLayoutBuildingTemplateTerrainImpact::Foundation &&
                    foundation.grounding.reliefCells == 1U &&
                    foundationCompiled.receipt.accepted &&
                    foundationCompiled.receipt.groundedBuildingCount == 1U &&
                    foundationCompiled.receipt.foundationObjectCount == 1U,
                "placement terrain facts match the compiler foundation result") &&
         expect(!missingFoundationTerrain.accepted &&
                    missingFoundationTerrain.status ==
                        cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::
                            TerrainRejected &&
                    missingFoundationTerrain.bounds.valid &&
                    missingFoundationTerrain.terrainImpact ==
                        cr::CreativeWorldLayoutBuildingTemplateTerrainImpact::
                            Unknown,
                "missing terrain fails closed while retaining placement facts");
}

bool buildingTemplateSyncIsSafeAtomicAndPersistent() {
  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          transformableBuildingLayout(), {0U, "sync_house", "Sync House"});
  if (!captured.accepted) {
    return expect(false, "sync template capture accepted");
  }
  cr::CreativeWorldLayout destination;
  destination.stableKey = "sync_destination";
  const cr::CreativeWorldLayoutBuildingEditResult first =
      cr::stampCreativeWorldLayoutBuildingTemplate(
          destination, captured.value, {{10, 20}, 100U, false});
  const cr::CreativeWorldLayoutBuildingTemplateResult rotated =
      cr::transformCreativeWorldLayoutBuildingTemplate(
          captured.value,
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
  const cr::CreativeWorldLayoutBuildingEditResult second =
      cr::stampCreativeWorldLayoutBuildingTemplate(
          first.edited, rotated.value,
          {{40, 50}, first.nextStableOrdinal, false});
  const cr::CreativeWorldLayoutBuildingEditResult moved =
      cr::moveCreativeWorldLayoutBuilding(second.edited, {1U, 3, -2});
  const cr::CreativeWorldLayoutBuildingTransformResult mirrored =
      cr::transformCreativeWorldLayoutBuilding(
          moved.edited,
          {1U, cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX});
  if (!first.accepted || !rotated.accepted || !second.accepted ||
      !moved.accepted || !mirrored.accepted) {
    return expect(false, "linked instance setup accepted");
  }

  cr::CreativeWorldLayout edited = mirrored.transformed;
  const auto currentAfterPlacement =
      cr::inspectCreativeWorldLayoutBuildingTemplateSync(
          edited, 1U, &captured.value);
  const auto movedProvenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(edited, 1U);
  const bool rigidPlacementRemainsCurrent =
      currentAfterPlacement.accepted &&
      currentAfterPlacement.state ==
          cr::CreativeWorldLayoutBuildingTemplateSyncState::Current &&
      movedProvenance.anchor == cr::CreativeTerrainCoord2{43, 48} &&
      movedProvenance.orientation ==
          cr::CreativeWorldLayoutBuildingTemplateOrientation::MirrorDiagonal;

  cr::CreativeWorldLayout roofEdited = edited;
  const auto roofLevel = std::find_if(
      roofEdited.levels.begin(), roofEdited.levels.end(),
      [](const cr::CreativeWorldLayoutLevel& level) {
        return level.buildingIndex == 1U;
      });
  if (roofLevel == roofEdited.levels.end()) {
    return expect(false, "linked roof level exists");
  }
  roofLevel->roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  const auto roofConflict =
      cr::inspectCreativeWorldLayoutBuildingTemplateSync(
          roofEdited, 1U, &captured.value);

  const auto firstLevel = std::find_if(
      edited.levels.begin(), edited.levels.end(),
      [](const cr::CreativeWorldLayoutLevel& level) {
        return level.buildingIndex == 0U;
      });
  firstLevel->wallHeightCells = 9U;

  cr::CreativeWorldLayout updatedSource = captured.value.normalizedLayout;
  updatedSource.levels[0].wallHeightCells += 2U;
  const cr::CreativeWorldLayoutBuildingTemplateResult updated =
      cr::loadCreativeWorldLayoutBuildingTemplate(std::move(updatedSource));
  if (!updated.accepted) {
    return expect(false, "updated sync template accepted");
  }
  const auto conflict = cr::inspectCreativeWorldLayoutBuildingTemplateSync(
      edited, 0U, &updated.value);
  const auto sourceChanged =
      cr::inspectCreativeWorldLayoutBuildingTemplateSync(edited, 1U,
                                                         &updated.value);
  const auto sourceMissing =
      cr::inspectCreativeWorldLayoutBuildingTemplateSync(edited, 1U, nullptr);

  const cr::CreativeWorldLayoutBuildingTemplateRefreshResult safe =
      cr::refreshCreativeWorldLayoutBuildingTemplateInstances(
          edited,
          {&updated.value,
           cr::CreativeWorldLayoutBuildingTemplateRefreshMode::SafeInstances,
           cr::kInvalidCreativeWorldLayoutIndex, second.nextStableOrdinal});
  const auto firstAfterSafe =
      safe.accepted
          ? cr::inspectCreativeWorldLayoutBuildingTemplateSync(
                safe.edited, 0U, &updated.value)
          : cr::CreativeWorldLayoutBuildingTemplateSyncReceipt{};
  const auto secondAfterSafe =
      safe.accepted
          ? cr::inspectCreativeWorldLayoutBuildingTemplateSync(
                safe.edited, 1U, &updated.value)
          : cr::CreativeWorldLayoutBuildingTemplateSyncReceipt{};
  const auto provenanceAfterSafe =
      safe.accepted
          ? cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(
                safe.edited, 1U)
          : cr::CreativeWorldLayoutBuildingTemplateInstanceProvenance{};

  const cr::CreativeWorldLayoutBuildingTemplateRefreshResult forced =
      safe.accepted
          ? cr::refreshCreativeWorldLayoutBuildingTemplateInstances(
                safe.edited,
                {&updated.value,
                 cr::CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll,
                 cr::kInvalidCreativeWorldLayoutIndex,
                 safe.nextStableOrdinal})
          : cr::CreativeWorldLayoutBuildingTemplateRefreshResult{};
  const auto firstAfterForce =
      forced.accepted
          ? cr::inspectCreativeWorldLayoutBuildingTemplateSync(
                forced.edited, 0U, &updated.value)
          : cr::CreativeWorldLayoutBuildingTemplateSyncReceipt{};

  const cr::CreativeWorldLayoutEncodeResult encoded =
      forced.accepted ? cr::encodeCreativeWorldLayout(forced.edited)
                      : cr::CreativeWorldLayoutEncodeResult{};
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted ? cr::decodeCreativeWorldLayout(encoded.encodedText)
                       : cr::CreativeWorldLayoutDecodeResult{};
  const auto persistedSync =
      decoded.accepted
          ? cr::inspectCreativeWorldLayoutBuildingTemplateSync(
                decoded.layout, 1U, &updated.value)
          : cr::CreativeWorldLayoutBuildingTemplateSyncReceipt{};

  return expect(rigidPlacementRemainsCurrent,
                "instance move and orientation remain placement provenance") &&
         expect(roofConflict.state ==
                    cr::CreativeWorldLayoutBuildingTemplateSyncState::
                        LocallyModified,
                "authored roof settings participate in instance sync") &&
         expect(conflict.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            Conflict &&
                    sourceChanged.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            SourceChanged &&
                    sourceMissing.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            SourceMissing,
                "sync separates conflicts, source changes, and missing sources") &&
         expect(safe.accepted && safe.refreshedInstanceCount == 1U &&
                    firstAfterSafe.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            Conflict &&
                    secondAfterSafe.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            Current &&
                    provenanceAfterSafe.anchor ==
                        cr::CreativeTerrainCoord2{43, 48} &&
                    provenanceAfterSafe.orientation ==
                        cr::CreativeWorldLayoutBuildingTemplateOrientation::
                            MirrorDiagonal,
                "safe refresh updates only untouched instances at their pose") &&
         expect(forced.accepted && forced.refreshedInstanceCount == 1U &&
                    firstAfterForce.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
                "force refresh explicitly replaces the remaining conflict") &&
         expect(encoded.accepted && decoded.accepted &&
                    persistedSync.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
                "instance provenance survives the existing layout codec");
}

bool structuralSurfacesCompileFromExplicitPlanesOnNonUnitGrid() {
  cr::CreativeDocument document = makeDocument(206U);
  static_cast<void>(document.setGridSettings(
      {{10.0, 2.0, -10.0}, 0.5, {64, 32, 64}}));

  cr::CreativeWorldLayout layout;
  layout.stableKey = "structural_planes";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Structural Planes";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  layout.buildings.push_back(std::move(building));
  layout.boxes = {
      {0U, cr::CreativeObjectKind::Floor, "floor", "Plane Floor",
       {{2, 4}, {6, 8}}, 3.0, 2U},
      {0U, cr::CreativeObjectKind::Ceiling, "ceiling", "Plane Ceiling",
       {{2, 4}, {6, 8}}, 5.5, 2U},
      {0U, cr::CreativeObjectKind::Roof, "roof", "Plane Roof",
       {{2, 4}, {6, 8}}, 7.0, 1U},
      {0U, cr::CreativeObjectKind::Room, "volume", "Grid Volume",
       {{2, 4}, {6, 8}}, 1.5, 2U},
  };

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  const cr::CreativeObject* floor = findNamed(preview.document, "Plane Floor");
  const cr::CreativeObject* ceiling =
      findNamed(preview.document, "Plane Ceiling");
  const cr::CreativeObject* roof = findNamed(preview.document, "Plane Roof");
  const cr::CreativeObject* volume = findNamed(preview.document, "Grid Volume");

  return expect(compiled.receipt.accepted && preview.accepted,
                "explicit structural planes compile and preview") &&
         expect(floor != nullptr && floor->bounds.min.x == 11.0 &&
                    floor->bounds.max.x == 13.0 &&
                    floor->bounds.min.z == -8.0 &&
                    floor->bounds.max.z == -6.0 &&
                    near(floor->bounds.min.y, 3.4) &&
                    near(floor->bounds.max.y, 3.5) &&
                    sameVec3(floor->transform.scale, {1.0, 1.0, 1.0}),
                "floor extends down from exact finished top without scale "
                "fixup") &&
         expect(ceiling != nullptr && near(ceiling->bounds.min.y, 4.75) &&
                    near(ceiling->bounds.max.y, 5.25) &&
                    sameVec3(ceiling->transform.scale, {1.0, 1.0, 1.0}),
                "ceiling extends up from exact support plane") &&
         expect(roof != nullptr && near(roof->bounds.min.y, 5.5) &&
                    near(roof->bounds.max.y, 5.75) &&
                    sameVec3(roof->transform.scale, {1.0, 1.0, 1.0}),
                "roof uses its thin descriptor layer above support plane") &&
         expect(volume != nullptr && near(volume->bounds.min.y, 2.75) &&
                    near(volume->bounds.max.y, 3.75),
                "ordinary box retains grid-cell layer semantics");
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
  const cr::CreativeObject* doorFrame =
      findNamed(preview.document, "Front Door Minimum Jamb");
  const cr::CreativeObject* doorHandle =
      findNamed(preview.document, "Front Door Primary Handle");
  const cr::CreativeObject* window = findNamed(preview.document, "East Window");
  const cr::CreativeObject* windowFrame =
      findNamed(preview.document, "East Window Minimum Jamb");

  return expect(compiled.receipt.accepted &&
                    compiled.receipt.status == cr::CreativeWorldLayoutStatus::Ready,
                "2D building layout compiles") &&
         expect(compiled.receipt.objectRecipeCount == 1U &&
                    compiled.receipt.objectCount == 24U,
                "2D building layout has deterministic generated object count") &&
         expect(preview.accepted && preview.document.objectCount() == 24U,
                "2D building plan previews exact document") &&
         expect(root != nullptr && root->bounds.min.x == 10.0 &&
                    root->bounds.min.y == 1.0 &&
                    root->bounds.min.z == -10.0 &&
                    root->bounds.max.x == 16.0 &&
                    root->bounds.max.y == 5.0 &&
                    root->bounds.max.z == -6.0,
                "grid-line footprint converts to exact world bounds") &&
         expect(door != nullptr && doorFrame != nullptr &&
                    doorHandle != nullptr && window != nullptr &&
                    windowFrame != nullptr && root != nullptr &&
                    door->parentId == root->id &&
                    doorFrame->parentId == root->id &&
                    doorHandle->parentId == door->id &&
                    door->door == layout.openings[0].door &&
                    window->parentId == root->id &&
                    windowFrame->parentId == root->id &&
                    window->window == layout.openings[1].window,
                "hosted openings retain grouped frames and semantic inserts") &&
         expect(window != nullptr && near(window->bounds.min.y, 3.06) &&
                    near(window->bounds.max.y, 3.94),
                "window fits inside the sill and lintel frame") &&
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
                    firstObjectCount == 24U,
                "first layout generation has exact output count") &&
         expect(replacement.receipt.accepted &&
                    replacement.receipt.objectRecipePatchCount == 1U &&
                    replacement.receipt.objectRecipeReplaceCount == 0U &&
                    replacement.receipt.objectRemoveCount == 0U &&
                    replacement.plan.objectRecipePatches.size() == 1U &&
                    replacement.plan.objectRecipes.empty() &&
                    replacement.receipt.objectCount == 24U &&
                    replacement.recipeChanges.size() == 1U &&
                    replacement.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch &&
                    replacement.recipeChanges[0].memberCounts.updateCount ==
                        1U &&
                    replacement.recipeChanges[0]
                            .memberCounts.preserveCount == 23U,
                "layout rebuild patches complete prior output in place") &&
         expect(replaced.accepted && replaced.changed &&
                    replacementObjectCount == 24U && replacementNamePresent,
                "layout replacement applies atomically without duplication") &&
         expect(deletion.receipt.accepted &&
                    deletion.receipt.objectRemoveCount == 24U,
                "empty source layout compiles deletion of stale output") &&
         expect(deleted.accepted && deleted.changed &&
                    appState.facade.document().objectCount() == 0U,
                "deleting final symbol removes generated 3D output");
}

bool selectiveRegenerationPreservesIdentityAndManualRefinement() {
  cr::CreativeAppState appState = makeAppState(212U);
  cr::CreativeWorldLayout layout = smallHouseLayout();

  cr::CreativeWorldLayoutObject crateA;
  crateA.kind = cr::CreativeObjectKind::Crate;
  crateA.stableKey = "crate_a";
  crateA.name = "Crate A";
  crateA.boundsCells = {{8.0, 0.0, 1.0}, {9.0, 1.0, 2.0}};
  layout.objects.push_back(crateA);

  cr::CreativeWorldLayoutObject crateB = crateA;
  crateB.stableKey = "crate_b";
  crateB.name = "Crate B";
  crateB.boundsCells = {{10.0, 0.0, 1.0}, {11.0, 1.0, 2.0}};
  layout.objects.push_back(crateB);

  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt firstApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, first.plan);
  const cr::CreativeObject* firstBuilding =
      findNamed(appState.facade.document(), "Small House");
  const cr::CreativeObject* firstCrateA =
      findNamed(appState.facade.document(), "Crate A");
  const cr::CreativeObject* firstCrateB =
      findNamed(appState.facade.document(), "Crate B");
  if (firstBuilding == nullptr || firstCrateA == nullptr ||
      firstCrateB == nullptr) {
    return expect(false, "selective fixture generated all recipe groups");
  }
  const cr::CreativeObjectId buildingId = firstBuilding->id;
  const cr::CreativeObjectId crateAId = firstCrateA->id;
  const cr::CreativeObjectId crateBId = firstCrateB->id;

  const cr::CreativeVec3 refinedPosition{77.0, 3.0, 88.0};
  const cr::CreativeDocumentMutationReceipt refined =
      cr::moveDocumentObject(appState.facade.documentForPersistence(),
                             crateBId, refinedPosition);
  const cr::CreativeWorldLayoutCompileResult unchanged =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt unchangedApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, unchanged.plan);
  const cr::CreativeObject* retainedCrateB =
      appState.facade.document().findObject(crateBId);
  const bool unchangedPreservedRefinement =
      retainedCrateB != nullptr &&
      sameVec3(retainedCrateB->transform.position, refinedPosition);

  layout.objects[0].name = "Crate A Revised";
  const cr::CreativeWorldLayoutCompileResult replacement =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt replaced =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, replacement.plan, "selective_world_layout_test");
  const cr::CreativeObject* replacedCrateA =
      findNamed(appState.facade.document(), "Crate A Revised");
  const cr::CreativeObject* preservedBuilding =
      appState.facade.document().findObject(buildingId);
  const cr::CreativeObject* preservedCrateB =
      appState.facade.document().findObject(crateBId);
  const bool replacementPreserved =
      replacedCrateA != nullptr && replacedCrateA->id == crateAId &&
      preservedBuilding != nullptr && preservedBuilding->id == buildingId &&
      preservedCrateB != nullptr &&
      sameVec3(preservedCrateB->transform.position, refinedPosition);
  const cr::CreativeAuthoringOperationRecord* operation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord> expectedOperation =
      operation != nullptr
          ? std::optional<cr::CreativeAuthoringOperationRecord>{*operation}
          : std::nullopt;
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeObject* restoredCrateA =
      appState.facade.document().findObject(crateAId);
  const cr::CreativeObject* restoredCrateB =
      appState.facade.document().findObject(crateBId);

  return expect(first.receipt.accepted && firstApplied.accepted &&
                    first.receipt.objectRecipeCreateCount == 3U &&
                    first.receipt.objectRecipePatchCount == 0U &&
                    first.receipt.objectRecipeReplaceCount == 0U &&
                    first.receipt.objectRecipeKeepCount == 0U &&
                    first.receipt.objectRecipeCount == 3U &&
                    first.receipt.objectCount == 26U,
                "initial generation reports three created recipe groups") &&
         expect(refined.changed && unchanged.receipt.accepted &&
                    unchanged.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    unchanged.receipt.objectRecipeCreateCount == 0U &&
                    unchanged.receipt.objectRecipePatchCount == 0U &&
                    unchanged.receipt.objectRecipeReplaceCount == 0U &&
                    unchanged.receipt.objectRecipeKeepCount == 2U &&
                    unchanged.receipt.objectRecipeRefinedCount == 1U &&
                    unchanged.receipt.objectRecipeConflictCount == 0U &&
                    unchanged.receipt.objectRecipeCount == 0U &&
                    unchanged.receipt.objectRemoveCount == 0U &&
                    unchanged.receipt.objectCount == 0U,
                "unchanged source distinguishes retained and refined groups") &&
         expect(unchangedApplied.accepted && !unchangedApplied.changed &&
                    unchangedPreservedRefinement,
                "no-change apply preserves manual generated refinement") &&
         expect(replacement.receipt.accepted &&
                    replacement.receipt.objectRecipeCreateCount == 0U &&
                    replacement.receipt.objectRecipePatchCount == 1U &&
                    replacement.receipt.objectRecipeReplaceCount == 0U &&
                    replacement.receipt.objectRecipeKeepCount == 1U &&
                    replacement.receipt.objectRecipeRefinedCount == 1U &&
                    replacement.receipt.objectRecipeCount == 1U &&
                    replacement.receipt.objectRemoveCount == 0U &&
                    replacement.plan.objectRecipePatches.size() == 1U &&
                    replacement.plan.objectRecipes.empty() &&
                    replacement.receipt.objectCount == 1U &&
                    replacement.recipeChanges.size() == 3U &&
                    replacement.recipeChanges[1].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch &&
                    replacement.recipeChanges[1]
                            .memberCounts.updateCount == 1U &&
                    replacement.recipeChanges[1]
                            .memberCounts.createCount == 0U &&
                    replacement.recipeChanges[1]
                            .memberCounts.removeCount == 0U,
                "one source change schedules exactly one stable-id patch") &&
         expect(replaced.accepted && replaced.changed &&
                    replaced.historyReceipt.recorded &&
                    replacementPreserved,
                "selective patch retains matching identity and unrelated edits") &&
         expect(expectedOperation.has_value() &&
                    expectedOperation->family ==
                        cr::CreativeAuthoringFamily::Building &&
                    expectedOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Reconcile &&
                    expectedOperation->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Parametric &&
                    expectedOperation->action == "WorldLayout.Apply" &&
                    expectedOperation->requestFingerprint ==
                        cr::fingerprintCreativeWorldLayoutPlanSource(
                            replacement.plan) &&
                    expectedOperation->affectedMemberCount ==
                        cr::creativeWorldLayoutPlanAffectedMemberCount(
                            replacement.plan),
                "world layout history records its exact source plan") &&
         expect(undone.accepted && undone.changed &&
                    undone.targetOperation == expectedOperation &&
                    restoredCrateA != nullptr &&
                    restoredCrateA->name == "Crate A" &&
                    restoredCrateB != nullptr &&
                    sameVec3(restoredCrateB->transform.position,
                             refinedPosition),
                "selective generation remains one undo transaction");
}

cr::CreativeWorldLayout singleCrateLayout(std::string name = "Managed Crate") {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "reconciliation_layout";
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Crate;
  object.stableKey = "crate";
  object.name = std::move(name);
  object.boundsCells = {{1.0, 0.0, 1.0}, {2.0, 1.0, 2.0}};
  object.tags = {"author:keep"};
  layout.objects.push_back(std::move(object));
  return layout;
}

bool refinedOutputBlocksSourceChangesUntilExplicitlyRegenerated() {
  cr::CreativeAppState appState = makeAppState(215U);
  cr::CreativeWorldLayout layout = singleCrateLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApply =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const cr::CreativeObject* generated =
      findNamed(appState.facade.document(), "Managed Crate");
  if (generated == nullptr) {
    return expect(false, "conflict fixture generated its managed object");
  }
  const cr::CreativeObjectId generatedId = generated->id;
  const cr::CreativeVec3 refinedPosition{40.0, 2.0, 30.0};
  const cr::CreativeDocumentMutationReceipt refined =
      cr::moveDocumentObject(appState.facade.documentForPersistence(),
                             generatedId, refinedPosition);

  layout.objects[0].name = "Revised Managed Crate";
  const std::uint64_t revisionBeforeBlocked =
      appState.facade.document().revision();
  const cr::CreativeWorldLayoutCompileResult blocked =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const std::array regenerateDecision{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Regenerate}};
  const cr::CreativeWorldLayoutCompileResult overwrite =
      cr::buildCreativeWorldLayoutPlan(
          appState.facade.document(), layout, {regenerateDecision});
  const cr::CreativeWorldLayoutApplyReceipt overwritten =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, overwrite.plan, "world_layout_conflict_overwrite");
  const cr::CreativeObject* replacement =
      findNamed(appState.facade.document(), "Revised Managed Crate");
  const cr::CreativeObjectId replacementId =
      replacement != nullptr ? replacement->id : cr::kInvalidObjectId;
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeObject* restored =
      appState.facade.document().findObject(generatedId);

  return expect(initial.receipt.accepted && initialApply.accepted &&
                    refined.changed,
                "managed output can be refined through the normal document path") &&
         expect(!blocked.receipt.accepted &&
                    blocked.receipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    blocked.receipt.objectRecipeConflictCount == 1U &&
                    blocked.plan.objectRecipes.empty() &&
                    blocked.plan.objectRemoveIds.empty() &&
                    blocked.recipeChanges.size() == 1U &&
                    blocked.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Conflict &&
                    appState.facade.document().revision() ==
                        revisionBeforeBlocked,
                "source change over refined output fails closed without mutation") &&
         expect(overwrite.receipt.accepted &&
                    overwrite.receipt.objectRecipePatchCount == 0U &&
                    overwrite.receipt.objectRecipeReplaceCount == 1U &&
                    overwrite.plan.objectRemoveIds.size() == 1U &&
                    overwrite.plan.objectRecipes.size() == 1U &&
                    overwrite.recipeChanges.size() == 1U &&
                    overwrite.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Replace &&
                    overwrite.recipeChanges[0].memberCounts.createCount ==
                        1U &&
                    overwrite.recipeChanges[0].memberCounts.removeCount ==
                        1U,
                "explicit regenerate resolves the reviewed conflict") &&
         expect(overwritten.accepted && overwritten.changed &&
                    overwritten.historyReceipt.recorded &&
                    replacementId != cr::kInvalidObjectId &&
                    replacementId != generatedId,
                "regenerate replaces refined output in one history step") &&
         expect(undone.accepted && undone.changed && restored != nullptr &&
                    sameVec3(restored->transform.position, refinedPosition),
                "undo restores the exact refined pre-resolution object");
}

bool detachResolutionPreservesRefinementAndCreatesFreshManagedOutput() {
  cr::CreativeAppState appState = makeAppState(216U);
  cr::CreativeWorldLayout layout = singleCrateLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApply =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const cr::CreativeObject* generated =
      findNamed(appState.facade.document(), "Managed Crate");
  if (generated == nullptr) {
    return expect(false, "detach fixture generated its managed object");
  }
  const cr::CreativeObjectId refinedId = generated->id;
  const cr::CreativeVec3 refinedPosition{22.0, 4.0, 18.0};
  const cr::CreativeDocumentMutationReceipt refined =
      cr::moveDocumentObject(appState.facade.documentForPersistence(),
                             refinedId, refinedPosition);
  layout.objects[0].name = "Fresh Managed Crate";

  const std::array detachDecision{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Detach}};
  const cr::CreativeWorldLayoutCompileResult detached =
      cr::buildCreativeWorldLayoutPlan(
          appState.facade.document(), layout, {detachDecision});
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, detached.plan, "world_layout_conflict_detach");
  const cr::CreativeObject* preserved =
      appState.facade.document().findObject(refinedId);
  const cr::CreativeObject* fresh =
      findNamed(appState.facade.document(), "Fresh Managed Crate");
  const std::string layoutTag = cr::creativeWorldLayoutTag(layout.stableKey);
  const bool ownershipRemoved =
      preserved != nullptr &&
      std::find(preserved->tags.begin(), preserved->tags.end(), layoutTag) ==
          preserved->tags.end() &&
      cr::creativeRecipeObjectInstanceKey(*preserved).empty() &&
      cr::creativeRecipeObjectOutputFingerprint(*preserved) == 0U &&
      std::find(preserved->tags.begin(), preserved->tags.end(),
                "author:keep") != preserved->tags.end();

  return expect(initial.receipt.accepted && initialApply.accepted &&
                    refined.changed,
                "detach fixture starts from refined managed output") &&
         expect(detached.receipt.accepted &&
                    detached.receipt.objectRecipeConflictCount == 1U &&
                    detached.receipt.objectRecipeDetachCount == 1U &&
                    detached.plan.objectDetachIds.size() == 1U &&
                    detached.plan.objectRemoveIds.empty() &&
                    detached.plan.objectRecipes.size() == 1U &&
                    detached.recipeChanges.size() == 1U &&
                    detached.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::DetachAndReplace &&
                    detached.recipeChanges[0].memberCounts.createCount == 1U &&
                    detached.recipeChanges[0].memberCounts.detachCount == 1U &&
                    detached.recipeChanges[0].memberCounts.removeCount == 0U,
                "detach resolution plans preservation plus fresh generation") &&
         expect(applied.accepted && applied.changed &&
                    applied.historyReceipt.recorded &&
                    appState.facade.document().objectCount() == 2U &&
                    ownershipRemoved && preserved != nullptr &&
                    sameVec3(preserved->transform.position, refinedPosition),
                "detached refinement keeps identity and authored geometry") &&
         expect(fresh != nullptr && fresh->id != refinedId &&
                    !cr::creativeRecipeObjectInstanceKey(*fresh).empty() &&
                    cr::creativeRecipeObjectOutputFingerprint(*fresh) != 0U,
                "layout receives a separate freshly managed object");
}

bool removedSourceCannotSilentlyDeleteRefinedOutput() {
  cr::CreativeAppState appState = makeAppState(217U);
  cr::CreativeWorldLayout layout = singleCrateLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApply =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const cr::CreativeObject* generated =
      findNamed(appState.facade.document(), "Managed Crate");
  if (generated == nullptr) {
    return expect(false, "removed-source fixture generated its managed object");
  }
  const cr::CreativeObjectId refinedId = generated->id;
  const cr::CreativeDocumentMutationReceipt refined =
      cr::moveDocumentObject(appState.facade.documentForPersistence(),
                             refinedId, {16.0, 3.0, 12.0});
  layout.objects.clear();

  const cr::CreativeWorldLayoutCompileResult blocked =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const std::array detachDecision{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Detach}};
  const cr::CreativeWorldLayoutCompileResult detached =
      cr::buildCreativeWorldLayoutPlan(
          appState.facade.document(), layout, {detachDecision});
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, detached.plan, "world_layout_removed_source_detach");
  const cr::CreativeObject* preserved =
      appState.facade.document().findObject(refinedId);

  return expect(initial.receipt.accepted && initialApply.accepted &&
                    refined.changed,
                "removed-source fixture starts from refined managed output") &&
         expect(!blocked.receipt.accepted &&
                    blocked.receipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    blocked.recipeChanges.size() == 1U &&
                    blocked.recipeChanges[0].desiredRecipeIndex ==
                        cr::kInvalidCreativeWorldLayoutRecipeIndex &&
                    blocked.plan.objectRemoveIds.empty(),
                "removing source blocks before deleting refined output") &&
         expect(detached.receipt.accepted &&
                    detached.plan.objectDetachIds.size() == 1U &&
                    detached.plan.objectRemoveIds.empty() &&
                    detached.plan.objectRecipes.empty() && applied.accepted &&
                    applied.changed && applied.historyReceipt.recorded,
                "explicit detach resolves a removed-source conflict once") &&
         expect(preserved != nullptr &&
                    cr::creativeRecipeObjectInstanceKey(*preserved).empty() &&
                    cr::creativeRecipeObjectOutputFingerprint(*preserved) == 0U,
                "detached removed-source output remains as an authored object");
}

bool conflictDecisionsAreExactCompleteAndIndependentlyApplied() {
  cr::CreativeAppState appState = makeAppState(218U);
  cr::CreativeWorldLayout layout = singleCrateLayout("Managed Crate A");
  cr::CreativeWorldLayoutObject second = layout.objects.front();
  second.kind = cr::CreativeObjectKind::Barrel;
  second.stableKey = "barrel";
  second.name = "Managed Barrel B";
  second.boundsCells = {{3.0, 0.0, 1.0}, {4.0, 1.0, 2.0}};
  layout.objects.push_back(second);

  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApply =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const cr::CreativeObject* crate =
      findNamed(appState.facade.document(), "Managed Crate A");
  const cr::CreativeObject* barrel =
      findNamed(appState.facade.document(), "Managed Barrel B");
  if (crate == nullptr || barrel == nullptr) {
    return expect(false, "mixed conflict fixture generated both groups");
  }
  const cr::CreativeObjectId crateId = crate->id;
  const cr::CreativeObjectId barrelId = barrel->id;
  const cr::CreativeVec3 crateRefinement{20.0, 2.0, 10.0};
  const cr::CreativeVec3 barrelRefinement{24.0, 2.0, 10.0};
  const cr::CreativeDocumentMutationReceipt crateMoved =
      cr::moveDocumentObject(appState.facade.documentForPersistence(), crateId,
                             crateRefinement);
  const cr::CreativeDocumentMutationReceipt barrelMoved =
      cr::moveDocumentObject(appState.facade.documentForPersistence(), barrelId,
                             barrelRefinement);
  layout.objects[0].name = "Rebuilt Crate A";
  layout.objects[1].name = "Fresh Barrel B";

  const std::array incompleteDecision{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Regenerate}};
  const cr::CreativeWorldLayoutCompileResult incomplete =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {incompleteDecision});
  const std::array duplicateDecisions{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Regenerate},
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Detach}};
  const cr::CreativeWorldLayoutCompileResult duplicate =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {duplicateDecisions});
  const std::array staleDecision{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.missing",
          cr::CreativeWorldLayoutConflictResolution::Detach}};
  const cr::CreativeWorldLayoutCompileResult stale =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {staleDecision});
  const std::array decisions{
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.barrel",
          cr::CreativeWorldLayoutConflictResolution::Detach},
      cr::CreativeWorldLayoutConflictDecision{
          "reconciliation_layout.objects.crate",
          cr::CreativeWorldLayoutConflictResolution::Regenerate}};
  const cr::CreativeWorldLayoutCompileResult resolved =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {decisions});
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, resolved.plan, "world_layout_mixed_conflict_resolution");
  const cr::CreativeObject* rebuiltCrate =
      findNamed(appState.facade.document(), "Rebuilt Crate A");
  const cr::CreativeObject* preservedBarrel =
      appState.facade.document().findObject(barrelId);
  const cr::CreativeObject* freshBarrel =
      findNamed(appState.facade.document(), "Fresh Barrel B");

  return expect(initial.receipt.accepted && initialApply.accepted &&
                    crateMoved.changed && barrelMoved.changed,
                "mixed conflict fixture refines both managed groups") &&
         expect(!incomplete.receipt.accepted &&
                    incomplete.receipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    incomplete.plan.objectRemoveIds.empty() &&
                    incomplete.plan.objectDetachIds.empty() &&
                    incomplete.plan.objectRecipes.empty(),
                "incomplete review remains blocked with no executable plan") &&
         expect(!duplicate.receipt.accepted &&
                    duplicate.receipt.status ==
                        cr::CreativeWorldLayoutStatus::InvalidDocument &&
                    duplicate.receipt.reasonCode ==
                        "creative_world_layout_conflict_decisions_invalid" &&
                    duplicate.plan.objectRemoveIds.empty(),
                "duplicate group decisions are rejected") &&
         expect(!stale.receipt.accepted &&
                    stale.receipt.status ==
                        cr::CreativeWorldLayoutStatus::InvalidDocument &&
                    stale.receipt.reasonCode ==
                        "creative_world_layout_conflict_decision_stale" &&
                    stale.plan.objectRemoveIds.empty() &&
                    stale.plan.objectDetachIds.empty(),
                "stale group decisions are rejected") &&
         expect(resolved.receipt.accepted &&
                    resolved.receipt.objectRecipeConflictCount == 2U &&
                    resolved.receipt.objectRecipeReplaceCount == 1U &&
                    resolved.receipt.objectRecipeDetachCount == 1U &&
                    resolved.plan.objectRemoveIds.size() == 1U &&
                    resolved.plan.objectDetachIds.size() == 1U &&
                    resolved.plan.objectRecipes.size() == 2U,
                "opposite decisions compile independently by stable key") &&
         expect(applied.accepted && applied.changed &&
                    applied.historyReceipt.recorded && rebuiltCrate != nullptr &&
                    rebuiltCrate->id != crateId && preservedBarrel != nullptr &&
                    sameVec3(preservedBarrel->transform.position,
                             barrelRefinement) &&
                    cr::creativeRecipeObjectInstanceKey(*preservedBarrel)
                        .empty() &&
                    freshBarrel != nullptr && freshBarrel->id != barrelId,
                "one transaction regenerates one group and detaches the other");
}

bool buildingRegenerationIsolatedToChangedOwnershipGroup() {
  cr::CreativeAppState appState = makeAppState(214U);
  const cr::CreativeWorldLayoutBuildingEditResult duplicated =
      cr::duplicateCreativeWorldLayoutBuilding(
          smallHouseLayout(), {0U, 12, 0, 100U});
  if (!duplicated.accepted || duplicated.edited.buildings.size() != 2U) {
    return expect(false, "two-building selective fixture is valid");
  }
  cr::CreativeWorldLayout layout = duplicated.edited;
  const std::string firstName = layout.buildings[0].name;
  const std::string secondName = layout.buildings[1].name;
  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt firstApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, first.plan);
  const cr::CreativeObject* firstRoot =
      findNamed(appState.facade.document(), firstName);
  const cr::CreativeObject* secondRoot =
      findNamed(appState.facade.document(), secondName);
  if (firstRoot == nullptr || secondRoot == nullptr) {
    return expect(false, "two-building recipe roots materialize");
  }
  const cr::CreativeObjectId firstRootId = firstRoot->id;
  const cr::CreativeObjectId secondRootId = secondRoot->id;
  const auto firstBuildingIds = managedObjectIds(
      appState.facade.document(), "estate_level_0.house");
  const std::array selectedIds{firstRootId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, firstRootId);

  layout.walls[0].name = "North Wall Revised";
  const cr::CreativeWorldLayoutCompileResult replacement =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt replaced =
      cr::applyCreativeWorldLayoutPlan(appState.facade, replacement.plan);
  const cr::CreativeObject* replacedFirstRoot =
      findNamed(appState.facade.document(), firstName);
  const cr::CreativeObject* retainedSecondRoot =
      findNamed(appState.facade.document(), secondName);
  const cr::CreativeObject* revisedNorthWall =
      findNamed(appState.facade.document(), "North Wall Revised");
  const auto replacedBuildingIds = managedObjectIds(
      appState.facade.document(), "estate_level_0.house");

  return expect(first.receipt.accepted && firstApplied.accepted &&
                    first.receipt.objectRecipeCreateCount == 2U,
                "two buildings begin as two independent recipe groups") &&
         expect(replacement.receipt.accepted &&
                    replacement.receipt.objectRecipePatchCount == 1U &&
                    replacement.receipt.objectRecipeReplaceCount == 0U &&
                    replacement.receipt.objectRecipeKeepCount == 1U &&
                    replacement.receipt.objectRecipeCount == 1U &&
                    replacement.receipt.objectRemoveCount == 0U &&
                    replacement.plan.objectRecipePatches.size() == 1U &&
                    replacement.plan.objectRecipes.empty() &&
                    replacement.receipt.objectCount == 24U &&
                    replacement.recipeChanges.size() == 2U &&
                    replacement.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch &&
                    replacement.recipeChanges[0]
                            .memberCounts.updateCount == 1U &&
                    replacement.recipeChanges[0]
                            .memberCounts.preserveCount == 23U,
                "wall edit schedules only its owning building patch") &&
         expect(selected.accepted && replaced.accepted && replaced.changed,
                "stable-id patch applies after selecting generated output") &&
         expect(replacedFirstRoot != nullptr &&
                    replacedFirstRoot->id == firstRootId &&
                    retainedSecondRoot != nullptr &&
                    retainedSecondRoot->id == secondRootId,
                "both building roots preserve identity") &&
         expect(revisedNorthWall != nullptr &&
                    revisedNorthWall->name == "North Wall Revised",
                "patched wall receives revised source state") &&
         expect(firstBuildingIds == replacedBuildingIds,
                "all matching building members preserve stable ids") &&
         expect(appState.facade.selectionState().selectedTarget.value ==
                    firstRootId,
                "regeneration restores selection by stable object id");
}

bool openingEditsPatchGeometryWithoutIdentityChurn() {
  cr::CreativeAppState appState = makeAppState(220U);
  cr::CreativeWorldLayout layout = smallHouseLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const std::string_view instanceKey = "estate_level_0.house";
  const auto initialIds = managedObjectIds(appState.facade.document(),
                                           instanceKey);
  const cr::CreativeObject* initialDoor = findManaged(
      appState.facade.document(), instanceKey, "house.front_door.insert");
  const cr::CreativeObject* initialFloor = findManaged(
      appState.facade.document(), instanceKey, "house.floor");
  const cr::CreativeObject* initialWindow = findManaged(
      appState.facade.document(), instanceKey, "house.east_window.insert");
  if (initialDoor == nullptr || initialFloor == nullptr ||
      initialWindow == nullptr) {
    return expect(false, "opening patch fixture materializes");
  }
  const cr::CreativeBounds doorBoundsBefore = initialDoor->bounds;
  const cr::CreativeObjectId doorId = initialDoor->id;
  const cr::CreativeObjectId selectedFloorId = initialFloor->id;
  const std::uint64_t floorFingerprintBefore =
      cr::fingerprintCreativeRecipeObjectState(*initialFloor, "root");
  const cr::CreativeObjectId selectedWindowId = initialWindow->id;
  const cr::CreativeDocumentCreateReceipt triggerCreated =
      appState.facade.createDocumentObject(
          cr::CreativeObjectKind::TriggerZone);
  const cr::CreativeLogicLinkMutationReceipt linkCreated =
      appState.facade.setLogicLink(
          {triggerCreated.objectId, doorId,
           cr::CreativeLogicLinkAction::Toggle});
  const std::array selectedIds{selectedFloorId, selectedWindowId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, selectedWindowId);

  layout.openings[0].centerOffsetCells = 2.5;
  const cr::CreativeWorldLayoutCompileResult changed =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, changed.plan, "opening_identity_patch_test");
  const auto changedIds = managedObjectIds(appState.facade.document(),
                                           instanceKey);
  const cr::CreativeObject* changedDoor = findManaged(
      appState.facade.document(), instanceKey, "house.front_door.insert");
  const cr::CreativeObject* changedFloor = findManaged(
      appState.facade.document(), instanceKey, "house.floor");
  const bool geometryChangedWithoutIdChurn =
      initialIds == changedIds && changedDoor != nullptr &&
      !sameBounds(changedDoor->bounds, doorBoundsBefore);
  const bool floorStateStayedExact =
      changedFloor != nullptr &&
      cr::fingerprintCreativeRecipeObjectState(*changedFloor, "root") ==
          floorFingerprintBefore;
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const bool selectionStayedOnGeneratedMembers =
      cr::selectedTargetCount(selection) == 2U &&
      cr::selectionContainsTarget(
          selection,
          {static_cast<cr::Id>(selectedFloorId)}) &&
      cr::selectionContainsTarget(
          selection,
          {static_cast<cr::Id>(selectedWindowId)}) &&
      selection.selectedTarget.value == selectedWindowId;
  const cr::CreativeLogicLink* preservedLink =
      appState.facade.document().findLogicLink(triggerCreated.objectId,
                                                doorId);
  const bool authoredLinkPreserved =
      preservedLink != nullptr &&
      preservedLink->action == cr::CreativeLogicLinkAction::Toggle;
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeObject* restoredDoor = findManaged(
      appState.facade.document(), instanceKey, "house.front_door.insert");

  return expect(initial.receipt.accepted && initialApplied.accepted &&
                    triggerCreated.accepted && linkCreated.accepted &&
                    selected.accepted,
                "opening patch fixture starts selected and linked") &&
         expect(changed.receipt.accepted &&
                    changed.receipt.objectRecipePatchCount == 1U &&
                    changed.receipt.objectRecipeReplaceCount == 0U &&
                    changed.receipt.objectRemoveCount == 0U &&
                    changed.plan.objectRecipePatches.size() == 1U &&
                    changed.plan.objectRecipes.empty() &&
                    changed.recipeChanges.size() == 1U &&
                    changed.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch &&
                    changed.recipeChanges[0].memberCounts.createCount == 0U &&
                    changed.recipeChanges[0].memberCounts.updateCount > 0U &&
                    changed.recipeChanges[0].memberCounts.removeCount == 0U,
                "opening edit compiles as one stable-id patch") &&
         expect(applied.accepted && applied.changed &&
                    applied.historyReceipt.recorded,
                "opening patch applies as one history transaction") &&
         expect(geometryChangedWithoutIdChurn,
                "opening geometry changes without stable-id churn") &&
         expect(floorStateStayedExact,
                "unaffected floor semantic state stays exact") &&
         expect(selectionStayedOnGeneratedMembers,
                "opening patch retains generated multi-selection and primary") &&
         expect(authoredLinkPreserved,
                "opening patch retains authored logic link by stable id") &&
         expect(undone.accepted && undone.changed &&
                    restoredDoor != nullptr &&
                    sameBounds(restoredDoor->bounds, doorBoundsBefore),
                "opening patch remains one exact undo transaction");
}

bool refinementOnUnchangedMemberSurvivesSiblingSourceEdit() {
  cr::CreativeAppState appState = makeAppState(221U);
  cr::CreativeWorldLayout layout = smallHouseLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const std::string_view instanceKey = "estate_level_0.house";
  const cr::CreativeObject* north = findManaged(
      appState.facade.document(), instanceKey, "house.north.segment.1");
  const cr::CreativeObject* west = findManaged(
      appState.facade.document(), instanceKey, "house.west.segment.1");
  if (north == nullptr || west == nullptr) {
    return expect(false, "sibling refinement fixture materializes");
  }
  const cr::CreativeObjectId northId = north->id;
  const cr::CreativeObjectId westId = west->id;
  const cr::CreativeVec3 refinedPosition{31.0, 4.0, 27.0};
  const cr::CreativeDocumentMutationReceipt refined =
      cr::moveDocumentObject(appState.facade.documentForPersistence(),
                             northId, refinedPosition);

  layout.walls[2].name = "West Wall Revised";
  const cr::CreativeWorldLayoutCompileResult changed =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, changed.plan);
  const cr::CreativeObject* preservedNorth =
      appState.facade.document().findObject(northId);
  const cr::CreativeObject* revisedWest =
      appState.facade.document().findObject(westId);
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  return expect(initial.receipt.accepted && initialApplied.accepted &&
                    refined.changed,
                "sibling refinement fixture starts refined") &&
         expect(changed.receipt.accepted &&
                    changed.receipt.objectRecipeConflictCount == 0U &&
                    changed.receipt.objectRecipePatchCount == 1U &&
                    changed.receipt.objectRecipeReplaceCount == 0U &&
                    changed.plan.objectRecipePatches.size() == 1U,
                "disjoint source edit does not conflict with refinement") &&
         expect(applied.accepted && applied.changed &&
                    preservedNorth != nullptr &&
                    sameVec3(preservedNorth->transform.position,
                             refinedPosition) &&
                    revisedWest != nullptr &&
                    revisedWest->name == "West Wall Revised",
                "patch preserves refined member and updates sibling in place") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.receipt.objectRecipeRefinedCount == 1U &&
                    stable.receipt.objectRecipeConflictCount == 0U,
                "refined group is stable after sibling patch");
}

bool exactMemberConflictChoicesPreserveIdentityAndRejectStaleState() {
  cr::CreativeAppState appState = makeAppState(223U);
  cr::CreativeWorldLayout layout = smallHouseLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const std::string_view instanceKey = "estate_level_0.house";
  const cr::CreativeObject* north = findManaged(
      appState.facade.document(), instanceKey, "house.north.segment.1");
  const cr::CreativeObject* west = findManaged(
      appState.facade.document(), instanceKey, "house.west.segment.1");
  if (north == nullptr || west == nullptr) {
    return expect(false, "exact conflict fixture materializes wall members");
  }
  const cr::CreativeObjectId northId = north->id;
  const cr::CreativeObjectId westId = west->id;
  const cr::CreativeVec3 generatedPosition = north->transform.position;
  const std::string generatedName = north->name;
  const cr::CreativeVec3 refinedPosition{31.0, 4.0, 27.0};
  const cr::CreativeDocumentMutationReceipt refined =
      cr::moveDocumentObject(appState.facade.documentForPersistence(),
                             northId, refinedPosition);
  const std::array selectedIds{northId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, northId);

  layout.walls[0].name = "North Wall From 2D";
  layout.walls[2].name = "West Wall From 2D";
  const cr::CreativeWorldLayoutCompileResult blocked =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutRecipeMemberConflict* conflict =
      findMemberConflict(blocked, "house.north.segment.1");
  if (conflict == nullptr) {
    return expect(false, "exact conflict identifies the changed north wall");
  }

  cr::CreativeWorldLayoutConflictDecision staleDecision = memberDecision(
      std::string(instanceKey), *conflict,
      cr::CreativeWorldLayoutConflictResolution::UseSource);
  ++staleDecision.currentFingerprint;
  const std::array staleDecisions{staleDecision};
  const cr::CreativeWorldLayoutCompileResult stale =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {staleDecisions});

  const std::array useSourceDecisions{memberDecision(
      std::string(instanceKey), *conflict,
      cr::CreativeWorldLayoutConflictResolution::UseSource)};
  const cr::CreativeWorldLayoutCompileResult useSource =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {useSourceDecisions});
  const cr::CreativeWorldLayoutApplyReceipt sourceApplied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, useSource.plan, "world_layout_exact_member_use_source");
  const cr::CreativeObject* sourcedNorth =
      appState.facade.document().findObject(northId);
  const cr::CreativeObject* sourcedWest =
      appState.facade.document().findObject(westId);
  const bool sourceStateApplied =
      sourcedNorth != nullptr && sourcedNorth->id == northId &&
      sourcedNorth->name == "North Wall From 2D" &&
      sameVec3(sourcedNorth->transform.position, generatedPosition) &&
      sourcedWest != nullptr && sourcedWest->name == "West Wall From 2D" &&
      appState.facade.selectionState().selectedTarget.value == northId;
  const cr::CreativeHistoryApplyReceipt sourceUndone =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Undo);
  const cr::CreativeObject* restoredNorth =
      appState.facade.document().findObject(northId);
  const bool sourceUndoRestored =
      sourceUndone.accepted && sourceUndone.changed &&
      restoredNorth != nullptr && restoredNorth->name == generatedName &&
      sameVec3(restoredNorth->transform.position, refinedPosition);

  const cr::CreativeWorldLayoutCompileResult blockedAgain =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutRecipeMemberConflict* keepConflict =
      findMemberConflict(blockedAgain, "house.north.segment.1");
  if (keepConflict == nullptr) {
    return expect(false, "undo restores the exact concurrent wall conflict");
  }
  const std::array keepDecisions{memberDecision(
      std::string(instanceKey), *keepConflict,
      cr::CreativeWorldLayoutConflictResolution::KeepRefinement)};
  const cr::CreativeWorldLayoutCompileResult keep =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {keepDecisions});
  const cr::CreativeWorldLayoutApplyReceipt keepApplied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, keep.plan, "world_layout_exact_member_keep_refinement");
  const cr::CreativeObject* keptNorth =
      appState.facade.document().findObject(northId);
  const cr::CreativeObject* updatedWest =
      appState.facade.document().findObject(westId);
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  return expect(initial.receipt.accepted && initialApplied.accepted &&
                    refined.changed && selected.accepted,
                "exact conflict fixture starts refined and selected") &&
         expect(!blocked.receipt.accepted &&
                    blocked.receipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    blocked.recipeChanges.size() == 1U &&
                    blocked.recipeChanges[0].memberConflicts.size() == 1U &&
                    conflict->kind == cr::CreativeWorldLayoutMemberConflictKind::
                                          ConcurrentEdit &&
                    conflict->objectId == northId &&
                    conflict->baselineFingerprint != 0U &&
                    conflict->currentFingerprint !=
                        conflict->baselineFingerprint &&
                    conflict->desiredFingerprint !=
                        conflict->baselineFingerprint,
                "conflict report binds the exact three-way wall state") &&
         expect(!stale.receipt.accepted &&
                    stale.receipt.status ==
                        cr::CreativeWorldLayoutStatus::InvalidDocument &&
                    stale.receipt.reasonCode ==
                        "creative_world_layout_conflict_decision_stale" &&
                    stale.plan.objectRecipePatches.empty() &&
                    stale.plan.objectRemoveIds.empty() &&
                    stale.plan.objectDetachIds.empty(),
                "state-bound decision rejects a changed fingerprint atomically") &&
         expect(useSource.receipt.accepted &&
                    useSource.receipt.objectRecipeConflictCount == 1U &&
                    useSource.receipt.objectRecipePatchCount == 1U &&
                    useSource.receipt.objectRecipeReplaceCount == 0U &&
                    useSource.plan.objectRecipePatches.size() == 1U &&
                    sourceApplied.accepted && sourceApplied.changed &&
                    sourceApplied.historyReceipt.recorded &&
                    sourceStateApplied,
                "Use 2D updates exact and safe sibling members without id churn") &&
         expect(sourceUndoRestored,
                "member resolution is one exact undo transaction") &&
         expect(keep.receipt.accepted &&
                    keep.receipt.objectRecipeConflictCount == 1U &&
                    keep.receipt.objectRecipePatchCount == 1U &&
                    keepApplied.accepted && keepApplied.changed &&
                    keepApplied.historyReceipt.recorded && keptNorth != nullptr &&
                    keptNorth->id == northId &&
                    keptNorth->name == generatedName &&
                    sameVec3(keptNorth->transform.position,
                             refinedPosition) &&
                    updatedWest != nullptr &&
                    updatedWest->name == "West Wall From 2D",
                "Keep 3D rebases metadata while safe siblings still patch") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.receipt.objectRecipeRefinedCount == 1U &&
                    stable.receipt.objectRecipeConflictCount == 0U,
                "rebased 3D override remains stable on the next compile");
}

bool linkedRemovedMemberCanDetachOrExplicitlyRemove() {
  cr::CreativeAppState appState = makeAppState(224U);
  cr::CreativeWorldLayout layout = smallHouseLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const std::string_view instanceKey = "estate_level_0.house";
  const std::string_view doorStableKey = "house.front_door.insert";
  const cr::CreativeObject* door =
      findManaged(appState.facade.document(), instanceKey, doorStableKey);
  if (door == nullptr) {
    return expect(false, "linked removal fixture materializes door insert");
  }
  const cr::CreativeObjectId doorId = door->id;
  const cr::CreativeDocumentCreateReceipt trigger =
      appState.facade.createDocumentObject(cr::CreativeObjectKind::TriggerZone);
  const cr::CreativeLogicLinkMutationReceipt linked =
      appState.facade.setLogicLink(
          {trigger.objectId, doorId, cr::CreativeLogicLinkAction::Toggle});
  const std::array selectedIds{doorId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, doorId);

  layout.openings.erase(layout.openings.begin());
  const cr::CreativeWorldLayoutCompileResult blocked =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutRecipeMemberConflict* conflict =
      findMemberConflict(blocked, doorStableKey);
  if (conflict == nullptr) {
    return expect(false, "linked removal reports the exact door insert");
  }
  const std::array detachDecision{memberDecision(
      std::string(instanceKey), *conflict,
      cr::CreativeWorldLayoutConflictResolution::DetachMember)};
  const cr::CreativeWorldLayoutCompileResult detach =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {detachDecision});
  const cr::CreativeWorldLayoutApplyReceipt detached =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, detach.plan, "world_layout_linked_member_detach");
  const cr::CreativeObject* authoredDoor =
      appState.facade.document().findObject(doorId);
  const bool detachState =
      authoredDoor != nullptr &&
      cr::creativeRecipeObjectInstanceKey(*authoredDoor).empty() &&
      appState.facade.document().findLogicLink(trigger.objectId, doorId) !=
          nullptr &&
      appState.facade.selectionState().selectedTarget.value == doorId;
  const cr::CreativeHistoryApplyReceipt detachUndone =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Undo);
  const bool detachUndoRestoredDoor =
      appState.facade.document().findObject(doorId) != nullptr;
  const bool detachUndoRestoredLink =
      appState.facade.document().findLogicLink(trigger.objectId, doorId) !=
      nullptr;

  const cr::CreativeWorldLayoutCompileResult blockedAgain =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutRecipeMemberConflict* removeConflict =
      findMemberConflict(blockedAgain, doorStableKey);
  if (removeConflict == nullptr) {
    return expect(false, "undo restores the linked door conflict");
  }
  const std::array removeDecision{memberDecision(
      std::string(instanceKey), *removeConflict,
      cr::CreativeWorldLayoutConflictResolution::RemoveMember)};
  const cr::CreativeWorldLayoutCompileResult remove =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout,
                                       {removeDecision});
  const cr::CreativeWorldLayoutApplyReceipt removed =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, remove.plan, "world_layout_linked_member_remove");

  return expect(initial.receipt.accepted && initialApplied.accepted &&
                    trigger.accepted && linked.accepted && selected.accepted,
                "linked removal fixture starts linked and selected") &&
         expect(!blocked.receipt.accepted &&
                    blocked.receipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    conflict->kind ==
                        cr::CreativeWorldLayoutMemberConflictKind::
                            SourceRemovedLinked &&
                    conflict->objectId == doorId,
                "removed linked output fails closed at the exact member") &&
         expect(detach.receipt.accepted &&
                    detach.receipt.objectRecipeConflictCount == 1U &&
                    detach.receipt.objectRecipePatchCount == 1U &&
                    detach.plan.objectDetachIds.size() == 1U &&
                    detached.accepted && detached.changed &&
                    detached.historyReceipt.recorded && detachState,
                "Detach preserves the door identity, link, and selection") &&
         expect(detachUndone.accepted && detachUndone.changed,
                "detached member and sibling patches undo together") &&
         expect(detachUndoRestoredDoor,
                "detach undo restores managed door identity") &&
         expect(detachUndoRestoredLink,
                "detach undo restores the authored door link") &&
         expect(remove.receipt.accepted &&
                    remove.receipt.objectRecipeConflictCount == 1U &&
                    remove.receipt.objectRecipePatchCount == 1U &&
                    !remove.plan.objectRemoveIds.empty() && removed.accepted &&
                    removed.changed && removed.historyReceipt.recorded &&
                    appState.facade.document().findObject(doorId) == nullptr &&
                    appState.facade.document().findLogicLink(
                        trigger.objectId, doorId) == nullptr &&
                    !cr::selectionContainsTarget(
                        appState.facade.selectionState(),
                        {static_cast<cr::Id>(doorId)}),
                "Remove explicitly deletes the door and its incident link");
}

bool authoredChildBlocksRemovalOfManagedParent() {
  cr::CreativeAppState appState = makeAppState(222U);
  const cr::CreativeWorldLayout layout = smallHouseLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt initialApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, initial.plan);
  const cr::CreativeObject* root = findManaged(
      appState.facade.document(), "estate_level_0.house", "root");
  if (root == nullptr) {
    return expect(false, "managed-parent fixture materializes");
  }
  const cr::CreativeObjectId rootId = root->id;

  cr::CreativeDocumentCreateRequest child;
  child.kind = cr::CreativeObjectKind::Wall;
  child.name = "Authored Child";
  child.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 0.25}};
  child.hasBoundsOverride = true;
  child.transform.position = {0.5, 0.5, 0.125};
  child.hasTransformOverride = true;
  child.parentId = rootId;
  const cr::CreativeDocumentCreateReceipt childCreated =
      appState.facade.createDocumentObject(child);
  const std::array selectedIds{rootId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, rootId);

  cr::CreativeWorldLayout removed;
  removed.stableKey = layout.stableKey;
  const cr::CreativeWorldLayoutCompileResult blocked =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), removed);
  const cr::CreativeWorldLayoutRecipeMemberConflict* conflict =
      findMemberConflict(blocked, "root");
  if (conflict == nullptr) {
    return expect(false, "managed parent reports its exact removal conflict");
  }
  const std::array invalidRemoveDecision{memberDecision(
      "estate_level_0.house", *conflict,
      cr::CreativeWorldLayoutConflictResolution::RemoveMember)};
  const cr::CreativeWorldLayoutCompileResult invalidRemove =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), removed,
                                       {invalidRemoveDecision});
  const std::array detachDecision{memberDecision(
      "estate_level_0.house", *conflict,
      cr::CreativeWorldLayoutConflictResolution::DetachMember)};
  const cr::CreativeWorldLayoutCompileResult detached =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), removed,
                                       {detachDecision});
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlanWithHistory(
          appState, detached.plan, "world_layout_parent_member_detach");
  const cr::CreativeObject* detachedRoot =
      appState.facade.document().findObject(rootId);
  const cr::CreativeObject* retainedChild =
      appState.facade.document().findObject(childCreated.objectId);
  const bool detachedState =
      detachedRoot != nullptr && retainedChild != nullptr &&
      retainedChild->parentId == rootId &&
      cr::creativeRecipeObjectInstanceKey(*detachedRoot).empty() &&
      cr::creativeRecipeObjectOutputFingerprint(*detachedRoot) == 0U &&
      appState.facade.document().objectCount() == 2U &&
      appState.facade.selectionState().selectedTarget.value == rootId;
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeObject* restoredRoot =
      appState.facade.document().findObject(rootId);

  return expect(initial.receipt.accepted && initialApplied.accepted &&
                    childCreated.accepted && selected.accepted,
                "authored child attaches outside managed group") &&
         expect(!blocked.receipt.accepted &&
                    blocked.receipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    blocked.receipt.objectRecipeConflictCount == 1U &&
                    blocked.plan.objectRemoveIds.empty() &&
                    blocked.plan.objectRecipePatches.empty() &&
                    blocked.recipeChanges.size() == 1U &&
                    blocked.recipeChanges[0].memberConflicts.size() == 1U &&
                    conflict->kind ==
                        cr::CreativeWorldLayoutMemberConflictKind::
                            SourceRemovedParent,
                "managed parent removal fails closed around authored child") &&
         expect(!invalidRemove.receipt.accepted &&
                    invalidRemove.receipt.status ==
                        cr::CreativeWorldLayoutStatus::InvalidDocument &&
                    invalidRemove.receipt.reasonCode ==
                        "creative_world_layout_conflict_decisions_invalid" &&
                    invalidRemove.plan.objectRemoveIds.empty(),
                "parent conflict cannot select an impossible remove action") &&
         expect(detached.receipt.accepted &&
                    detached.receipt.objectRecipeConflictCount == 1U &&
                    detached.receipt.objectRecipeDetachCount == 1U &&
                    detached.plan.objectDetachIds.size() == 1U &&
                    detached.plan.objectRemoveIds.size() == 23U &&
                    detached.recipeChanges[0].memberCounts.detachCount == 1U &&
                    detached.recipeChanges[0].memberCounts.removeCount == 23U &&
                    applied.accepted && applied.changed &&
                    applied.historyReceipt.recorded && detachedState,
                "member detach preserves the authored hierarchy and removes safe siblings") &&
         expect(undone.accepted && undone.changed && restoredRoot != nullptr &&
                    !cr::creativeRecipeObjectInstanceKey(*restoredRoot).empty() &&
                    appState.facade.document().objectCount() == 25U,
                "hierarchy-aware member resolution remains one undo step");
}

bool unversionedGeneratedGroupsMigrateOnceThenRemainStable() {
  cr::CreativeAppState appState = makeAppState(213U);
  const cr::CreativeWorldLayout layout = smallHouseLayout();
  cr::CreativeWorldLayoutCompileResult legacy =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const std::uint64_t legacyFingerprint =
      legacy.plan.objectRecipes[0].definitionFingerprint;
  for (cr::CreativeRecipePlan& recipe : legacy.plan.objectRecipes) {
    recipe.definitionFingerprint = 0U;
  }
  const cr::CreativeWorldLayoutApplyReceipt legacyApplied =
      cr::applyCreativeWorldLayoutPlan(appState.facade, legacy.plan);
  const cr::CreativeObject* legacyRoot =
      findNamed(appState.facade.document(), "Small House");
  const cr::CreativeObjectId legacyRootId =
      legacyRoot != nullptr ? legacyRoot->id : cr::kInvalidObjectId;
  const bool legacyHasNoDefinition =
      legacyRoot != nullptr &&
      !cr::creativeRecipeObjectHasDefinitionFingerprint(*legacyRoot,
                                                        legacyFingerprint);

  const cr::CreativeWorldLayoutCompileResult migration =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt migrated =
      cr::applyCreativeWorldLayoutPlan(appState.facade, migration.plan);
  const cr::CreativeObject* migratedRoot =
      findNamed(appState.facade.document(), "Small House");
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  return expect(legacyApplied.accepted && legacyHasNoDefinition,
                "legacy fixture has no definition provenance") &&
         expect(migration.receipt.accepted &&
                    migration.receipt.objectRecipePatchCount == 1U &&
                    migration.receipt.objectRecipeReplaceCount == 0U &&
                    migration.receipt.objectRecipeKeepCount == 0U &&
                    migration.receipt.objectRemoveCount == 0U &&
                    migration.plan.objectRecipePatches.size() == 1U &&
                    migration.plan.objectRecipes.empty() &&
                    migration.receipt.objectCount == 24U &&
                    migration.recipeChanges.size() == 1U &&
                    migration.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch &&
                    migration.recipeChanges[0]
                            .memberCounts.preserveCount == 24U &&
                    migration.recipeChanges[0]
                            .memberCounts.updateCount == 0U,
                "unversioned generated group schedules metadata adoption") &&
         expect(migrated.accepted && migrated.changed &&
                    migratedRoot != nullptr &&
                    migratedRoot->id == legacyRootId,
                "legacy migration adopts output without identity churn") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.receipt.objectRecipeKeepCount == 1U &&
                    stable.receipt.objectRecipeCount == 0U &&
                    stable.receipt.objectRemoveCount == 0U,
                "migrated generated group is stable on the next compile");
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
  cr::CreativeWorldLayoutTerrainPath river;
  river.stableKey = "river.main";
  river.recipe.kind = cr::CreativeTerrainPathKind::River;
  river.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  river.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  river.recipe.material = cr::CreativeTerrainMaterial::Sand;
  river.recipe.nextPointId = 3U;
  river.recipe.points = {
      {1U, {6, 0}, 8U, 1U, 2U, 0},
      {2U, {9, 0}, 8U, 1U, 2U, 0},
  };
  layout.terrainPaths.push_back(std::move(river));

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

bool watercourseCompileRetainsCanonicalAttachmentsWithoutWaterObjects() {
  cr::CreativeAppState appState = makeAppState(204U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "watercourse_layout";
  cr::CreativeWorldLayoutTerrainPath river;
  river.stableKey = "river.main";
  river.recipe.kind = cr::CreativeTerrainPathKind::River;
  river.recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
  river.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  river.recipe.material = cr::CreativeTerrainMaterial::Sand;
  river.recipe.watercourse.bankSlopeCells = 2U;
  river.recipe.watercourse.drainageDirection =
      cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd;
  river.recipe.watercourse.surfacePolicy =
      cr::CreativeTerrainWaterSurfacePolicy::Reserved;
  river.recipe.watercourse.surfaceInsetCells = 1U;
  river.recipe.watercourse.nextCrossingId = 10U;
  river.recipe.watercourse.crossings = {{9U, 2U, 1U, 2U, 3U}};
  river.recipe.nextPointId = 4U;
  river.recipe.points = {
      {1U, {-4, 0}, 9U, 2U, 3U, 0},
      {2U, {0, 0}, 8U, 2U, 3U, 0},
      {3U, {4, 0}, 7U, 2U, 3U, 0},
  };
  layout.terrainPaths.push_back(river);

  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      first.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlan(appState.facade, first.plan)
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWatercoursePlan* plan =
      first.plan.watercoursePlans.size() == 1U
          ? &first.plan.watercoursePlans.front()
          : nullptr;
  const cr::CreativeWatercourseCrossingFrame* crossing =
      plan != nullptr && plan->crossings.size() == 1U
          ? &plan->crossings.front()
          : nullptr;

  cr::CreativeWorldLayout uphill = layout;
  uphill.terrainPaths[0].recipe.points[2].heightCells = 10U;
  const cr::CreativeWorldLayoutCompileResult rejected =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), uphill);

  return expect(first.receipt.accepted &&
                    first.receipt.watercourseCount == 1U &&
                    first.receipt.watercourseCrossingCount == 1U &&
                    first.plan.terrainOperationMutations.size() == 1U &&
                    plan != nullptr && !plan->surfaceSamples.empty() &&
                    crossing != nullptr,
                "world layout retains one canonical watercourse plan") &&
         expect(crossing->id == 9U && crossing->sourcePointId == 2U &&
                    sameVec3(crossing->crossingAxis, {0.0, 0.0, 1.0}) &&
                    sameVec3(crossing->centerMeters, {10.0, 7.0, -10.0}) &&
                    crossing->leftBankMeters.x == 10.0 &&
                    crossing->leftBankMeters.z == -5.0 &&
                    crossing->rightBankMeters.x == 10.0 &&
                    crossing->rightBankMeters.z == -15.0 &&
                    crossing->leftApproachMeters.x == 10.0 &&
                    crossing->leftApproachMeters.z == -2.0 &&
                    crossing->rightApproachMeters.x == 10.0 &&
                    crossing->rightApproachMeters.z == -18.0 &&
                    crossing->leftBankMeters.y ==
                        1.0 + crossing->leftBankGrid.y &&
                    crossing->rightBankMeters.y ==
                        1.0 + crossing->rightBankGrid.y &&
                    crossing->leftApproachMeters.y ==
                        1.0 + crossing->leftApproachGrid.y &&
                    crossing->rightApproachMeters.y ==
                        1.0 + crossing->rightApproachGrid.y &&
                    crossing->leftBankMeters.y < crossing->centerMeters.y &&
                    crossing->rightBankMeters.y < crossing->centerMeters.y &&
                    crossing->channelBedMeters.y <=
                        crossing->clearanceReferenceMeters.y &&
                    crossing->spanMeters == 10.0,
                "crossing frame retains terrain banks approaches and deck") &&
         expect(first.plan.objectRecipes.empty() &&
                    first.plan.objectRecipePatches.empty() &&
                    first.receipt.objectCount == 0U && applied.accepted &&
                    appState.facade.document().objectCount() == 0U,
                "watercourse compile fabricates neither water nor bridge objects") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.plan.watercoursePlans.size() == 1U &&
                    stable.plan.watercoursePlans[0].definitionFingerprint ==
                        plan->definitionFingerprint,
                "unchanged watercourse replays one deterministic semantic plan") &&
         expect(!rejected.receipt.accepted &&
                    rejected.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainPath &&
                    rejected.receipt.failedIndex == 0U,
                "uphill directed drainage fails at its exact source path");
}

bool terrainPathOperationsReconcileOwnershipOrderAndIdentity() {
  cr::CreativeAppState appState = makeAppState(205U);
  const cr::CreativeTerrainControlEdit manualControl{
      cr::CreativeTerrainEditKind::Upsert, {{40, 40}, 6U, 1U}};
  const cr::CreativeTerrainMutationReceipt manualTerrain =
      appState.facade.applyTerrainControlEdits({&manualControl, 1U});
  cr::CreativeTerrainOperationMutationRequest manualOperation;
  manualOperation.kind = cr::CreativeTerrainOperationMutationKind::Add;
  manualOperation.operationKind = cr::CreativeTerrainOperationKind::Path;
  manualOperation.path = terrainPath("manual", 30, 6U).recipe;
  const cr::CreativeTerrainOperationMutationReceipt manualAdded =
      appState.facade.applyTerrainOperationMutation(manualOperation);

  cr::CreativeWorldLayout layout;
  layout.stableKey = "path_layout";
  layout.terrainPaths = {
      terrainPath("road.a", 0, 4U),
      terrainPath("road.b", 10, 5U),
  };
  const std::string sourceA =
      cr::creativeWorldLayoutTerrainPathSourceKey(layout.stableKey, "road.a");
  const std::string sourceB =
      cr::creativeWorldLayoutTerrainPathSourceKey(layout.stableKey, "road.b");
  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt firstApplied =
      first.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, first.plan, "world_layout_path_first")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeTerrainOperation* firstA =
      findTerrainOperation(appState.facade.document(), sourceA);
  const cr::CreativeTerrainOperation* firstB =
      findTerrainOperation(appState.facade.document(), sourceB);
  const cr::CreativeTerrainOperationId firstAId =
      firstA != nullptr ? firstA->id : cr::kInvalidCreativeTerrainOperationId;
  const cr::CreativeTerrainOperationId firstBId =
      firstB != nullptr ? firstB->id : cr::kInvalidCreativeTerrainOperationId;
  const bool firstOwnershipValid =
      firstA != nullptr && firstB != nullptr &&
      firstA->owner == cr::CreativeTerrainOperationOwner::WorldLayout &&
      firstB->owner == cr::CreativeTerrainOperationOwner::WorldLayout;
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  cr::CreativeWorldLayout changed = layout;
  changed.terrainPaths[0].recipe.points[1].heightCells = 7U;
  changed.terrainPaths[0].recipe.points[1].halfWidthCells = 2U;
  const cr::CreativeWorldLayoutCompileResult update =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), changed);
  const cr::CreativeWorldLayoutApplyReceipt updated =
      update.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, update.plan, "world_layout_path_update")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeTerrainOperation* updatedA =
      findTerrainOperation(appState.facade.document(), sourceA);
  const bool updatedIdentityValid =
      updatedA != nullptr && updatedA->id == firstAId &&
      updatedA->path == changed.terrainPaths[0].recipe;

  cr::CreativeWorldLayout reordered = changed;
  std::swap(reordered.terrainPaths[0], reordered.terrainPaths[1]);
  const cr::CreativeWorldLayoutCompileResult reorder =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), reordered);
  const cr::CreativeWorldLayoutApplyReceipt reorderedApplied =
      reorder.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, reorder.plan, "world_layout_path_reorder")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const auto& reorderedOperations =
      appState.facade.document().terrainOperationStack().operations;
  const bool reorderedIdsStable =
      reorderedOperations.size() == 3U &&
      reorderedOperations[0].id == manualAdded.operationId &&
      reorderedOperations[1].sourceKey == sourceB &&
      reorderedOperations[1].id == firstBId &&
      reorderedOperations[2].sourceKey == sourceA &&
      reorderedOperations[2].id == firstAId;

  cr::CreativeWorldLayout removed = reordered;
  removed.terrainPaths.erase(removed.terrainPaths.begin());
  const cr::CreativeWorldLayoutCompileResult remove =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), removed);
  const cr::CreativeWorldLayoutApplyReceipt removedApplied =
      remove.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, remove.plan, "world_layout_path_remove")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const auto& preservedOperations =
      appState.facade.document().terrainOperationStack().operations;
  const bool preserveKeptManual =
      preservedOperations.size() == 2U &&
      preservedOperations[0].id == manualAdded.operationId &&
      preservedOperations[0].owner == cr::CreativeTerrainOperationOwner::Manual &&
      preservedOperations[1].sourceKey == sourceA &&
      preservedOperations[1].id == firstAId &&
      appState.facade.document().terrainField().controlAt({40, 40}) != nullptr;

  cr::CreativeWorldLayout replaced = removed;
  replaced.terrainOwnership = cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;
  const cr::CreativeWorldLayoutCompileResult replace =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), replaced);
  const cr::CreativeWorldLayoutApplyReceipt replacedApplied =
      replace.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, replace.plan, "world_layout_path_replace_all")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const auto& replacedOperations =
      appState.facade.document().terrainOperationStack().operations;
  const cr::CreativeWorldLayoutCompileResult replaceStable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), replaced);

  return expect(manualTerrain.accepted && manualAdded.accepted &&
                    first.receipt.accepted &&
                    first.receipt.terrainOperationMutationCount == 2U &&
                    first.plan.terrainOperationMutations.size() == 2U &&
                    firstApplied.accepted && firstApplied.changed &&
                    firstOwnershipValid,
                "first path compile preserves manual terrain and adds owned operations") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.plan.terrainOperationMutations.empty(),
                "unchanged paths compile to no operation churn") &&
         expect(update.receipt.accepted &&
                    update.plan.terrainOperationMutations.size() == 1U &&
                    update.plan.terrainOperationMutations[0].kind ==
                        cr::CreativeTerrainOperationMutationKind::Update &&
                    update.plan.terrainOperationMutations[0].operationId ==
                        firstAId &&
                    updated.accepted && updated.changed &&
                    updatedIdentityValid,
                "path recipe edits update the same operation identity") &&
         expect(reorder.receipt.accepted && reorderedApplied.accepted &&
                    reorderedApplied.changed && reorderedIdsStable,
                "path source order reorders only owned operations with stable ids") &&
         expect(remove.receipt.accepted && removedApplied.accepted &&
                    removedApplied.changed && preserveKeptManual,
                "PreserveExisting removes stale owned paths but keeps manual terrain") &&
         expect(replace.receipt.accepted && replacedApplied.accepted &&
                    replacedApplied.changed && replacedOperations.size() == 1U &&
                    replacedOperations[0].owner ==
                        cr::CreativeTerrainOperationOwner::WorldLayout &&
                    replacedOperations[0].sourceKey == sourceA &&
                    appState.facade.document().terrainField().controlAt({40, 40}) ==
                        nullptr &&
                    cr::creativeUndoDepth(appState.history) == 5U,
                "ReplaceAll removes manual terrain and rebuilds only desired paths") &&
         expect(replaceStable.receipt.accepted &&
                    replaceStable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    replaceStable.plan.terrainOperationMutations.empty(),
                "stable ReplaceAll compile preserves desired operation identity");
}

bool roadConstructionReconcilesAndKeepsTravelSurfaceTraversable() {
  cr::CreativeAppState appState = makeAppState(213U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "road_structure_layout";
  cr::CreativeWorldLayoutTerrainPath road = terrainPath("road.main", 5, 4U);
  road.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  road.recipe.points[0].amplitudeCells = 0U;
  road.recipe.points[1].amplitudeCells = 0U;
  road.recipe.road.shoulderWidthCells = 1U;
  road.recipe.road.edgeTreatment =
      cr::CreativeTerrainRoadEdgeTreatment::Curb;
  road.recipe.road.edgeWidthMeters = 0.2;
  road.recipe.road.edgeHeightMeters = 0.25;
  road.recipe.road.edgeMaterial = cr::CreativeStructuralMaterial::Stone;
  layout.terrainPaths.push_back(road);
  const std::string instanceKey = road.stableKey;

  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      first.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, first.plan, "world_layout_road_structure")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> generated =
      managedObjectIds(appState.facade.document(), instanceKey);

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &appState.facade.document();
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const bool centerlineBlocked = iggy3d::segmentHitsAnyPhysicsAabb(
      physics.colliders, {11.0F, 5.5F, -5.0F},
      {17.0F, 5.5F, -5.0F}, 0.05F, nullptr);
  const std::array<iggy3d::Vec3, 2U> waypoints{
      iggy3d::Vec3{11.0F, 5.0F, -5.0F},
      iggy3d::Vec3{17.0F, 5.0F, -5.0F},
  };
  const iggy3d::ReasoningGraph reasoning =
      iggy3d::buildReasoningGraph(baked.room, waypoints);
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  cr::CreativeWorldLayout withoutCurbs = layout;
  withoutCurbs.terrainPaths[0].recipe.road.edgeTreatment =
      cr::CreativeTerrainRoadEdgeTreatment::None;
  const cr::CreativeWorldLayoutCompileResult removal =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       withoutCurbs);
  const cr::CreativeWorldLayoutApplyReceipt removed =
      removal.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, removal.plan, "world_layout_road_remove_curbs")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> afterRemoval =
      managedObjectIds(appState.facade.document(), instanceKey);

  return expect(first.receipt.accepted &&
                    first.plan.objectRecipes.size() == 1U &&
                    first.plan.objectRecipes[0].kind ==
                        cr::CreativeRecipeKind::Road &&
                    !first.plan.objectRecipes[0].objects.empty() &&
                    applied.accepted && applied.changed &&
                    generated.size() ==
                        first.plan.objectRecipes[0].objects.size(),
                "World Layout applies one owned road structure recipe") &&
         expect(baked.receipt.accepted && physics.ok && !centerlineBlocked &&
                    reasoning.nodes.size() == 2U &&
                    reasoning.edges.size() == 1U,
                "curb geometry leaves the authored travel surface traversable") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.plan.objectRecipes.empty() &&
                    stable.plan.objectRecipePatches.empty(),
                "unchanged road structure reconciles without object churn") &&
         expect(removal.receipt.accepted &&
                    removal.plan.objectRemoveIds.size() == generated.size() &&
                    removed.accepted && removed.changed &&
                    afterRemoval.empty(),
                "disabling curbs removes every Road-owned generated member");
}

bool watercourseBridgeReconcilesStructureGradesCollisionAndTraversal() {
  cr::CreativeAppState appState = makeAppState(216U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "bridge_recipe_layout";

  cr::CreativeWorldLayoutTerrainPath river;
  river.stableKey = "river.bridge";
  river.recipe.kind = cr::CreativeTerrainPathKind::River;
  river.recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
  river.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  river.recipe.paintSurface = true;
  river.recipe.material = cr::CreativeTerrainMaterial::Sand;
  river.recipe.watercourse.bankSlopeCells = 1U;
  river.recipe.watercourse.nextCrossingId = 2U;
  river.recipe.watercourse.crossings = {{1U, 2U, 1U, 1U, 4U}};
  river.recipe.nextPointId = 4U;
  river.recipe.points = {
      {1U, {-4, 0}, 4U, 1U, 2U, 0},
      {2U, {0, 0}, 4U, 1U, 2U, 0},
      {3U, {4, 0}, 4U, 1U, 2U, 0},
  };
  layout.terrainPaths.push_back(river);

  cr::CreativeWorldLayoutObject bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  bridge.stableKey = "bridge.main";
  bridge.name = "Main Bridge";
  bridge.boundsCells = {{-1.0, 0.0, -3.0}, {1.0, 0.35, 3.0}};
  bridge.tags = {"world_layout:object"};
  bridge.usesBridgeRecipe = true;
  bridge.bridge.watercoursePathKey = river.stableKey;
  bridge.bridge.crossingId = 1U;
  layout.objects.push_back(bridge);

  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      first.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, first.plan, "world_layout_bridge_recipe")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> generated =
      managedObjectIds(appState.facade.document(), bridge.stableKey);
  const cr::CreativeObject* deck =
      findManaged(appState.facade.document(), bridge.stableKey, "deck");
  const bool deckReady =
      deck != nullptr && deck->kind == cr::CreativeObjectKind::Bridge;
  const cr::CreativeObjectId deckId = deck != nullptr ? deck->id : 0U;

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &appState.facade.document();
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::CollisionQueryResult deckTop =
      iggy3d::sampleSurfaceHeight(surfaces, {10.0F, 0.0F, -10.0F});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const bool centerlineBlocked = iggy3d::segmentHitsAnyPhysicsAabb(
      physics.colliders,
      {10.0F, deckTop.heightMeters + 0.5F, -13.0F},
      {10.0F, deckTop.heightMeters + 0.5F, -7.0F}, 0.05F, nullptr);
  const std::array<iggy3d::Vec3, 2U> bridgeWaypoints{
      iggy3d::Vec3{10.0F, deckTop.heightMeters, -12.0F},
      iggy3d::Vec3{10.0F, deckTop.heightMeters, -8.0F},
  };
  const iggy3d::ReasoningGraph bridgeReasoning =
      iggy3d::buildReasoningGraph(baked.room, bridgeWaypoints);

  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  cr::CreativeWorldLayout updatedLayout = layout;
  updatedLayout.objects[0].bridge.settings.deckWidthMeters = 3.0;
  updatedLayout.objects[0].bridge.settings.rails = false;
  updatedLayout.objects[0].bridge.settings.materials.deck =
      cr::CreativeStructuralMaterial::Stone;
  const cr::CreativeWorldLayoutCompileResult updated =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       updatedLayout);
  const cr::CreativeWorldLayoutApplyReceipt updatedApplied =
      updated.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, updated.plan, "world_layout_bridge_update")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> updatedMembers =
      managedObjectIds(appState.facade.document(), bridge.stableKey);
  const cr::CreativeObject* updatedDeck =
      findManaged(appState.facade.document(), bridge.stableKey, "deck");
  cr::CreativeStructuralMaterial updatedDeckMaterial =
      cr::CreativeStructuralMaterial::Count;
  const bool updatedDeckReady =
      updatedDeck != nullptr && updatedDeck->id == deckId &&
      cr::parseCreativeStructuralMaterialTag(updatedDeck->tags,
                                             updatedDeckMaterial);
  bool retainedIdentity = true;
  for (const auto& [key, id] : updatedMembers) {
    const auto prior = generated.find(key);
    retainedIdentity =
        retainedIdentity && prior != generated.end() && prior->second == id;
  }
  const cr::CreativeWorldLayoutCompileResult updatedStable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       updatedLayout);

  cr::CreativeWorldLayout missingPath = updatedLayout;
  missingPath.objects[0].bridge.watercoursePathKey = "river.missing";
  const cr::CreativeWorldLayoutCompileResult missingPathResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), missingPath);
  cr::CreativeWorldLayout missingCrossing = updatedLayout;
  missingCrossing.objects[0].bridge.crossingId = 99U;
  const cr::CreativeWorldLayoutCompileResult missingCrossingResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       missingCrossing);
  cr::CreativeWorldLayout duplicateAttachment = updatedLayout;
  cr::CreativeWorldLayoutObject duplicateBridge =
      duplicateAttachment.objects[0];
  duplicateBridge.stableKey = "bridge.duplicate";
  duplicateBridge.name = "Duplicate Bridge";
  duplicateAttachment.objects.push_back(std::move(duplicateBridge));
  const cr::CreativeWorldLayoutCompileResult duplicateAttachmentResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       duplicateAttachment);

  cr::CreativeWorldLayout removedLayout = updatedLayout;
  removedLayout.objects.clear();
  const cr::CreativeWorldLayoutCompileResult removal =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       removedLayout);
  const cr::CreativeWorldLayoutApplyReceipt removed =
      removal.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, removal.plan, "world_layout_bridge_remove")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> afterRemoval =
      managedObjectIds(appState.facade.document(), bridge.stableKey);
  const auto& operations =
      appState.facade.document().terrainOperationStack().operations;
  const bool bridgeGradesRemoved = std::none_of(
      operations.begin(), operations.end(),
      [](const cr::CreativeTerrainOperation& operation) {
        return operation.sourceKey.find("/bridge_approach/") !=
               std::string::npos;
      });

  return expect(first.receipt.accepted &&
                    first.receipt.watercourseCount == 1U &&
                    first.receipt.bridgeRecipeCount == 1U &&
                    first.receipt.bridgeGeneratedObjectCount == 5U &&
                    first.receipt.bridgeApproachGradeCount == 2U &&
                    first.plan.watercoursePlans.size() == 1U &&
                    first.plan.bridgePlans.size() == 1U &&
                    first.plan.terrainOperationMutations.size() == 3U &&
                    first.plan.objectRecipes.size() == 1U &&
                    first.plan.objectRecipes[0].kind ==
                        cr::CreativeRecipeKind::Bridge,
                "World Layout owns one attached bridge structure and two grades") &&
         expect(applied.accepted && applied.changed,
                "bridge recipe applies atomically") &&
         expect(generated.size() == 5U,
                "bridge recipe materializes every semantic member") &&
         expect(deckReady,
                "bridge recipe owns one walkable deck member") &&
         expect(baked.receipt.accepted && physics.ok &&
                    deckTop.status == iggy3d::CollisionQueryStatus::Hit &&
                    !centerlineBlocked && bridgeReasoning.nodes.size() == 2U &&
                    bridgeReasoning.edges.size() == 1U,
                "bridge deck keeps collision and reasoning traversal clear") &&
         expect(stable.receipt.accepted,
                "unchanged bridge remains a valid compile") &&
         expect(stable.receipt.status ==
                    cr::CreativeWorldLayoutStatus::NoChange,
                "unchanged bridge reports no semantic change") &&
         expect(stable.plan.objectRecipes.empty() &&
                    stable.plan.objectRecipePatches.empty(),
                "unchanged bridge regenerates without member churn") &&
         expect(updated.receipt.accepted &&
                    updated.receipt.bridgeGeneratedObjectCount == 3U &&
                    updatedApplied.accepted && updatedApplied.changed &&
                    updatedMembers.size() == 3U && retainedIdentity &&
                    updatedDeckReady &&
                    updatedDeckMaterial == cr::CreativeStructuralMaterial::Stone,
                "bridge source edits patch retained members and remove disabled rails") &&
         expect(updatedStable.receipt.accepted &&
                    updatedStable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    updatedStable.plan.objectRecipes.empty() &&
                    updatedStable.plan.objectRecipePatches.empty(),
                "edited bridge converges to one stable definition") &&
         expect(!missingPathResult.receipt.accepted &&
                    missingPathResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Object &&
                    missingPathResult.receipt.reasonCode ==
                        "creative_world_layout_bridge_watercourse_missing" &&
                    !missingCrossingResult.receipt.accepted &&
                    missingCrossingResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Object &&
                    missingCrossingResult.receipt.reasonCode ==
                        "creative_world_layout_bridge_crossing_missing",
                "missing bridge attachments fail at the exact source row") &&
         expect(!duplicateAttachmentResult.receipt.accepted &&
                    duplicateAttachmentResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Object &&
                    duplicateAttachmentResult.receipt.failedIndex == 1U &&
                    duplicateAttachmentResult.receipt.reasonCode ==
                        "creative_world_layout_bridge_attachment_duplicate",
                "one stable crossing has exactly one bridge owner") &&
         expect(removal.receipt.accepted &&
                    removal.plan.objectRemoveIds.size() ==
                        updatedMembers.size() &&
                    removed.accepted && removed.changed && afterRemoval.empty() &&
                    bridgeGradesRemoved,
                "removing bridge removes structure and owned approach grades");
}

bool terrainLandformsReconcileBeforePathsWithStableIdentity() {
  cr::CreativeAppState appState = makeAppState(207U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "site_layout";
  layout.terrainProfiles.push_back(landform("terrace.entry"));
  layout.terrainPaths.push_back(terrainPath("road.entry", 10, 4U));
  const std::string landformKey =
      cr::creativeWorldLayoutTerrainLandformSourceKey(
          layout.stableKey, "terrace.entry");
  const std::string pathKey = cr::creativeWorldLayoutTerrainPathSourceKey(
      layout.stableKey, "road.entry");

  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const bool firstOrder =
      first.plan.terrainOperationMutations.size() == 2U &&
      first.plan.terrainOperationMutations[0].operationKind ==
          cr::CreativeTerrainOperationKind::Landform &&
      first.plan.terrainOperationMutations[1].operationKind ==
          cr::CreativeTerrainOperationKind::Path;
  const cr::CreativeWorldLayoutApplyReceipt firstApplied =
      first.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, first.plan, "world_layout_landform_first")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeTerrainOperation* firstLandform =
      findTerrainOperation(appState.facade.document(), landformKey);
  const cr::CreativeTerrainOperation* firstPath =
      findTerrainOperation(appState.facade.document(), pathKey);
  const cr::CreativeTerrainOperationId landformId =
      firstLandform != nullptr
          ? firstLandform->id
          : cr::kInvalidCreativeTerrainOperationId;
  const cr::CreativeTerrainOperationId pathId =
      firstPath != nullptr ? firstPath->id
                           : cr::kInvalidCreativeTerrainOperationId;
  const bool firstOperationsValid =
      firstLandform != nullptr && firstPath != nullptr &&
      firstLandform->owner ==
          cr::CreativeTerrainOperationOwner::WorldLayout &&
      firstLandform->kind == cr::CreativeTerrainOperationKind::Landform;
  const bool exactTerrace =
      appState.facade.document().terrainHeightField().heightAt({0, 1}) == 2U &&
      appState.facade.document().terrainHeightField().heightAt({2, 1}) == 4U &&
      appState.facade.document().terrainHeightField().heightAt({4, 1}) == 6U &&
      appState.facade.document().terrainHeightField().heightAt({6, 1}) == 8U &&
      appState.facade.document().terrainMaterialField().materialAt({6, 1}) ==
          cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);

  cr::CreativeWorldLayout changed = layout;
  changed.terrainProfiles[0].kind = cr::CreativeTerrainRecipeKind::Cliff;
  changed.terrainProfiles[0].landform.kind =
      cr::CreativeTerrainLandformKind::Cliff;
  changed.terrainProfiles[0].landform.direction =
      cr::CreativeTerrainLandformDirection::NegativeX;
  const cr::CreativeWorldLayoutCompileResult update =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), changed);
  const cr::CreativeWorldLayoutApplyReceipt updated =
      update.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, update.plan, "world_layout_landform_update")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeTerrainOperation* updatedLandform =
      findTerrainOperation(appState.facade.document(), landformKey);
  const cr::CreativeTerrainOperation* updatedPath =
      findTerrainOperation(appState.facade.document(), pathKey);
  const bool updatedIdentityValid =
      updatedLandform != nullptr && updatedLandform->id == landformId &&
      updatedLandform->landform == changed.terrainProfiles[0].landform &&
      updatedPath != nullptr && updatedPath->id == pathId;

  cr::CreativeWorldLayout removed = changed;
  removed.terrainProfiles.clear();
  const cr::CreativeWorldLayoutCompileResult remove =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), removed);
  const cr::CreativeWorldLayoutApplyReceipt removedApplied =
      remove.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, remove.plan, "world_layout_landform_remove")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeTerrainOperation* remainingPath =
      findTerrainOperation(appState.facade.document(), pathKey);

  return expect(first.receipt.accepted && firstOrder &&
                    firstApplied.accepted && firstApplied.changed &&
                    firstOperationsValid &&
                    exactTerrace,
                "landform compiles before paths and owns exact site output") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.plan.terrainOperationMutations.empty(),
                "unchanged landform and path produce no operation churn") &&
         expect(update.receipt.accepted &&
                    update.plan.terrainOperationMutations.size() == 1U &&
                    update.plan.terrainOperationMutations[0].kind ==
                        cr::CreativeTerrainOperationMutationKind::Update &&
                    update.plan.terrainOperationMutations[0].operationId ==
                        landformId &&
                    updated.accepted && updated.changed &&
                    updatedIdentityValid,
                "landform variant edits retain source operation identity") &&
         expect(remove.receipt.accepted && removedApplied.accepted &&
                    removedApplied.changed &&
                    findTerrainOperation(appState.facade.document(),
                                         landformKey) == nullptr &&
                    remainingPath != nullptr && remainingPath->id == pathId,
                "removing a landform removes only its owned operation");
}

bool retainingEdgesReconcileCollisionAndStairTraversal() {
  cr::CreativeAppState appState = makeAppState(219U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "retaining_edge_layout";
  cr::CreativeWorldLayoutTerrainProfile terrace =
      landform("terrace.retained");
  terrace.landform.targetHeightCells = 4U;
  terrace.landform.terraceCount = 2U;
  terrace.usesRetainingEdgeRecipe = true;
  terrace.retainingEdge.terrainProfileKey = terrace.stableKey;
  terrace.retainingEdge.settings.selection =
      cr::CreativeRetainingEdgeSelection::Internal;
  terrace.retainingEdge.settings.transitionCount = 1U;
  terrace.retainingEdge.settings.transitions[0] = {
      cr::canonicalCreativeTerrainHardEdge({3, 1}, {4, 1}),
      cr::CreativeRetainingEdgeTransitionKind::Stair,
      3U,
  };
  layout.terrainProfiles.push_back(terrace);
  const std::string instanceKey = terrace.stableKey;

  const cr::CreativeWorldLayoutCompileResult first =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      first.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, first.plan, "world_layout_retaining_edge")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> generated =
      managedObjectIds(appState.facade.document(), instanceKey);

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &appState.facade.document();
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const bool neighboringRiserBlocked = iggy3d::segmentHitsAnyPhysicsAabb(
      physics.colliders, {12.75F, 4.0F, -8.0F},
      {14.25F, 4.0F, -8.0F}, 0.05F, nullptr);
  const iggy3d::ReasoningGraph reasoning =
      iggy3d::buildReasoningGraph(baked.room, {});
  const std::size_t stairAnchorCount = static_cast<std::size_t>(std::count_if(
      baked.room.anchors.begin(), baked.room.anchors.end(),
      [](const iggy3d::RoomAnchorAsset& anchor) {
        return anchor.kind == "stair";
      }));

  const cr::CreativeWorldLayoutCompileResult stable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  cr::CreativeWorldLayout updatedLayout = layout;
  updatedLayout.terrainProfiles[0].retainingEdge.settings.material =
      cr::CreativeStructuralMaterial::Brick;
  const cr::CreativeWorldLayoutCompileResult updated =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       updatedLayout);
  const cr::CreativeWorldLayoutApplyReceipt updatedApplied =
      updated.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, updated.plan, "world_layout_retaining_edge_update")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> updatedMembers =
      managedObjectIds(appState.facade.document(), instanceKey);
  bool updatedMaterial = !updatedMembers.empty();
  for (const auto& [stableKey, id] : updatedMembers) {
    const auto prior = generated.find(stableKey);
    const cr::CreativeObject* object =
        findManaged(appState.facade.document(), instanceKey, stableKey);
    cr::CreativeStructuralMaterial material =
        cr::CreativeStructuralMaterial::Count;
    updatedMaterial = updatedMaterial && prior != generated.end() &&
                      prior->second == id && object != nullptr &&
                      cr::parseCreativeStructuralMaterialTag(object->tags,
                                                             material) &&
                      material == cr::CreativeStructuralMaterial::Brick;
  }
  const cr::CreativeWorldLayoutCompileResult updatedStable =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       updatedLayout);

  cr::CreativeWorldLayout mismatched = updatedLayout;
  mismatched.terrainProfiles[0].retainingEdge.terrainProfileKey =
      "terrace.other";
  const cr::CreativeWorldLayoutCompileResult mismatchResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), mismatched);
  cr::CreativeWorldLayout sloped = updatedLayout;
  sloped.terrainProfiles[0].landform.edge =
      cr::CreativeTerrainLandformEdge::Slope;
  sloped.terrainProfiles[0].landform.edgeWidthCells = 2U;
  const cr::CreativeWorldLayoutCompileResult slopedResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), sloped);
  cr::CreativeWorldLayout overwritten = updatedLayout;
  cr::CreativeWorldLayoutTerrainProfile replacement =
      overwritten.terrainProfiles[0];
  replacement.stableKey = "terrace.replacement";
  replacement.usesRetainingEdgeRecipe = false;
  replacement.retainingEdge = {};
  overwritten.terrainProfiles.push_back(std::move(replacement));
  const cr::CreativeWorldLayoutCompileResult overwrittenResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       overwritten);
  cr::CreativeWorldLayout malformedDisabled = updatedLayout;
  malformedDisabled.terrainProfiles[0].usesRetainingEdgeRecipe = false;
  malformedDisabled.terrainProfiles[0]
      .retainingEdge.settings.transitionCount =
      cr::kCreativeRetainingEdgeTransitionCapacity + 1U;
  const cr::CreativeWorldLayoutCompileResult malformedDisabledResult =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       malformedDisabled);

  cr::CreativeWorldLayout removedLayout = updatedLayout;
  removedLayout.terrainProfiles[0].usesRetainingEdgeRecipe = false;
  const cr::CreativeWorldLayoutCompileResult removal =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       removedLayout);
  const cr::CreativeWorldLayoutApplyReceipt removed =
      removal.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, removal.plan, "world_layout_retaining_edge_remove")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const std::map<std::string, cr::CreativeObjectId> afterRemoval =
      managedObjectIds(appState.facade.document(), instanceKey);

  return expect(first.receipt.accepted &&
                    first.receipt.retainingEdgeRecipeCount == 1U &&
                    first.receipt.retainingEdgeGeneratedObjectCount > 0U &&
                    first.plan.retainingEdgePlans.size() == 1U &&
                    first.plan.objectRecipes.size() == 1U &&
                    first.plan.objectRecipes[0].kind ==
                        cr::CreativeRecipeKind::RetainingEdge &&
                    first.plan.retainingEdgePlans[0].receipt.stairCount == 1U,
                "World Layout owns one retaining structure over final hard edges") &&
         expect(applied.accepted && applied.changed &&
                    generated.size() ==
                        first.receipt.retainingEdgeGeneratedObjectCount,
                "retaining structure materializes atomically") &&
         expect(baked.receipt.accepted && physics.ok &&
                    neighboringRiserBlocked && stairAnchorCount == 2U &&
                    reasoning.nodes.size() == 2U &&
                    reasoning.edges.size() == 1U,
                "retained wall blocks while the authored stair seam remains traversable") &&
         expect(stable.receipt.accepted &&
                    stable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange &&
                    stable.plan.objectRecipes.empty() &&
                    stable.plan.objectRecipePatches.empty(),
                "unchanged retaining source converges without object churn") &&
         expect(updated.receipt.accepted && updatedApplied.accepted &&
                    updatedApplied.changed &&
                    updatedMembers.size() == generated.size() &&
                    updatedMaterial,
                "retaining material edit patches every member without identity churn") &&
         expect(updatedStable.receipt.accepted &&
                    updatedStable.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange,
                "edited retaining source converges to one stable definition") &&
         expect(!mismatchResult.receipt.accepted &&
                    mismatchResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainProfile &&
                    mismatchResult.receipt.reasonCode ==
                        "creative_world_layout_retaining_edge_source_invalid" &&
                    !slopedResult.receipt.accepted &&
                    slopedResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainProfile &&
                    slopedResult.receipt.reasonCode ==
                        "creative_world_layout_retaining_edge_source_invalid",
                "invalid retaining attachments fail at the exact profile row") &&
         expect(!overwrittenResult.receipt.accepted &&
                    overwrittenResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainProfile &&
                    overwrittenResult.receipt.failedIndex == 0U &&
                    overwrittenResult.receipt.reasonCode ==
                        "creative_world_layout_retaining_edge_recipe_rejected" &&
                    overwrittenResult.receipt.kernelReasonCode ==
                        "creative_retaining_edge_recipe_no_matching_edges",
                "retaining source cannot decorate a later landform owner's seams") &&
         expect(!malformedDisabledResult.receipt.accepted &&
                    malformedDisabledResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainProfile &&
                    malformedDisabledResult.receipt.reasonCode ==
                        "creative_world_layout_retaining_edge_source_invalid",
                "disabled retaining data remains valid and saveable") &&
         expect(removal.receipt.accepted &&
                    removal.plan.objectRemoveIds.size() ==
                        updatedMembers.size() &&
                    removed.accepted && removed.changed &&
                    afterRemoval.empty(),
                "disabling retaining output removes every owned member");
}

bool terrainPathOperationCapacityFailsAtExactSource() {
  const cr::CreativeDocument document = makeDocument(206U);
  const std::uint64_t revisionBefore = document.revision();
  cr::CreativeWorldLayout layout;
  layout.stableKey = "path_capacity_layout";
  layout.terrainPaths.reserve(cr::kCreativeTerrainOperationCapacity + 1U);
  for (std::size_t index = 0U;
       index <= cr::kCreativeTerrainOperationCapacity; ++index) {
    layout.terrainPaths.push_back(terrainPath(
        "road." + std::to_string(index), static_cast<std::int32_t>(index),
        4U));
  }

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  return expect(!compiled.receipt.accepted &&
                    compiled.receipt.status ==
                        cr::CreativeWorldLayoutStatus::MutationRejected &&
                    compiled.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainPath &&
                    compiled.receipt.failedIndex ==
                        cr::kCreativeTerrainOperationCapacity &&
                    compiled.receipt.reasonCode ==
                        "creative_terrain_operation_capacity_exceeded",
                "path operation capacity reports the exact rejected source") &&
         expect(document.terrainOperationStack().operations.empty() &&
                    document.revision() == revisionBefore,
                "capacity rejection cannot partially mutate the live document");
}

bool terrainPathNetworkRequiresExactIntersectionAndBridgeSeams() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "path_network_layout";

  const auto gradeApproach = [](std::string key,
                                cr::CreativeTerrainCoord2 start,
                                cr::CreativeTerrainCoord2 end,
                                std::uint16_t halfWidth) {
    cr::CreativeWorldLayoutTerrainPath path;
    path.stableKey = std::move(key);
    path.recipe.kind = cr::CreativeTerrainPathKind::Road;
    path.recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
    path.recipe.curve = cr::CreativeTerrainPathCurvePolicy::Linear;
    path.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
    path.recipe.falloffCells = 2U;
    path.recipe.material = cr::CreativeTerrainMaterial::Dirt;
    path.recipe.nextPointId = 3U;
    path.recipe.points = {
        {1U, start, 4U, halfWidth, 0U, 0},
        {2U, end, 4U, halfWidth, 0U, 0},
    };
    return path;
  };

  cr::CreativeWorldLayoutTerrainPath main = terrainPath("main", 0, 4U, 2U);
  main.recipe.endJoin = cr::CreativeTerrainPathEndpointJoin::Intersection;
  cr::CreativeWorldLayoutTerrainPath branch =
      terrainPath("branch", -4, 4U, 2U);
  branch.recipe.points[0].coord = {8, -4};
  branch.recipe.points[1].coord = {8, 0};
  branch.recipe.endJoin = cr::CreativeTerrainPathEndpointJoin::Intersection;
  cr::CreativeWorldLayoutTerrainPath bridgeApproach = gradeApproach(
      "bridge.approach.west", {8, 0}, {12, 0}, 2U);
  bridgeApproach.recipe.startJoin =
      cr::CreativeTerrainPathEndpointJoin::Intersection;
  bridgeApproach.recipe.endJoin =
      cr::CreativeTerrainPathEndpointJoin::Bridge;
  cr::CreativeWorldLayoutTerrainPath bridgeExit = gradeApproach(
      "bridge.approach.east", {17, 0}, {21, 0}, 2U);
  bridgeExit.recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::Bridge;
  cr::CreativeWorldLayoutTerrainPath padApproach = gradeApproach(
      "building.pad.approach", {20, 0}, {24, 0}, 1U);
  padApproach.recipe.endJoin =
      cr::CreativeTerrainPathEndpointJoin::BuildingPad;
  layout.terrainPaths = {main, branch, bridgeApproach, bridgeExit, padApproach};

  cr::CreativeWorldLayoutObject bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  bridge.stableKey = "bridge.main";
  bridge.name = "Main Bridge";
  bridge.boundsCells = {{12.0, 0.0, -2.0}, {18.0, 1.0, 3.0}};
  layout.objects.push_back(bridge);

  cr::CreativeWorldLayoutBuilding building;
  building.rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  building.stableKey = "building.main";
  building.name = "Main Building";
  building.rootFootprint = {{24, -2}, {30, 3}};
  layout.buildings.push_back(building);
  layout.boxes.push_back(
      {0U, cr::CreativeObjectKind::Floor, "pad", "Main Building Pad",
       building.rootFootprint, 0.0, 1U});

  cr::CreativeAppState appState = makeAppState(207U);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(), layout);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      compiled.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlanWithHistory(
                appState, compiled.plan, "world_layout_path_network")
          : cr::CreativeWorldLayoutApplyReceipt{};
  const bool networkApplied =
      compiled.receipt.accepted && applied.accepted && applied.changed &&
      appState.facade.document().terrainOperationStack().operations.size() ==
          5U &&
      std::all_of(
          appState.facade.document().terrainOperationStack().operations.begin(),
          appState.facade.document().terrainOperationStack().operations.end(),
          [](const cr::CreativeTerrainOperation& operation) {
            return operation.owner ==
                       cr::CreativeTerrainOperationOwner::WorldLayout &&
                   operation.kind == cr::CreativeTerrainOperationKind::Path;
          });
  if (!networkApplied) {
    std::cerr << "path network compile status="
              << static_cast<unsigned>(compiled.receipt.status)
              << " reason=" << compiled.receipt.reasonCode
              << " kernel=" << compiled.receipt.kernelReasonCode
              << " failedTable="
              << static_cast<unsigned>(compiled.receipt.failedTable)
              << " failedIndex=" << compiled.receipt.failedIndex << '\n';
  }

  cr::CreativeWorldLayout unmatchedIntersection;
  unmatchedIntersection.stableKey = "unmatched_intersection";
  unmatchedIntersection.terrainPaths = {main};
  const cr::CreativeWorldLayoutCompileResult intersectionRejected =
      cr::buildCreativeWorldLayoutPlan(makeDocument(208U),
                                       unmatchedIntersection);

  cr::CreativeWorldLayout mismatchedHeight = layout;
  mismatchedHeight.terrainPaths[1].recipe.points.back().heightCells = 5U;
  const cr::CreativeWorldLayoutCompileResult heightRejected =
      cr::buildCreativeWorldLayoutPlan(makeDocument(209U), mismatchedHeight);

  cr::CreativeWorldLayout unmatchedBridge = layout;
  unmatchedBridge.objects.clear();
  const cr::CreativeWorldLayoutCompileResult bridgeRejected =
      cr::buildCreativeWorldLayoutPlan(makeDocument(210U), unmatchedBridge);

  cr::CreativeWorldLayout oneSidedBridge = layout;
  oneSidedBridge.terrainPaths.erase(oneSidedBridge.terrainPaths.begin() + 3U);
  const cr::CreativeWorldLayoutCompileResult bridgePairRejected =
      cr::buildCreativeWorldLayoutPlan(makeDocument(211U), oneSidedBridge);

  cr::CreativeWorldLayout unmatchedPad = layout;
  unmatchedPad.buildings.clear();
  unmatchedPad.boxes.clear();
  const cr::CreativeWorldLayoutCompileResult padRejected =
      cr::buildCreativeWorldLayoutPlan(makeDocument(212U), unmatchedPad);

  return expect(networkApplied,
                "matched intersections and bridge approaches compile together") &&
         expect(!intersectionRejected.receipt.accepted &&
                    intersectionRejected.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainPath &&
                    intersectionRejected.receipt.reasonCode ==
                        "creative_world_layout_path_intersection_unmatched",
                "unmatched intersection endpoints fail closed") &&
         expect(!heightRejected.receipt.accepted &&
                    heightRejected.receipt.reasonCode ==
                        "creative_world_layout_path_intersection_height_mismatch",
                "intersection endpoints require one exact authored elevation") &&
         expect(!bridgeRejected.receipt.accepted &&
                    bridgeRejected.receipt.failedIndex == 2U &&
                    bridgeRejected.receipt.reasonCode ==
                        "creative_world_layout_path_bridge_endpoint_unmatched",
                "bridge joins require an actual bridge footprint edge") &&
         expect(!bridgePairRejected.receipt.accepted &&
                    bridgePairRejected.receipt.reasonCode ==
                        "creative_world_layout_bridge_approach_pair_required",
                "bridge terrain integration requires both exact approaches") &&
         expect(!padRejected.receipt.accepted &&
                    padRejected.receipt.reasonCode ==
                        "creative_world_layout_path_building_pad_unmatched",
                "building-pad joins require one exact host footprint");
}

bool invalidAndStaleSourcesFailClosed() {
  cr::CreativeDocument document = makeDocument(204U);
  cr::CreativeWorldLayout duplicate = smallHouseLayout();
  duplicate.boxes[1].stableKey = duplicate.boxes[0].stableKey;
  const cr::CreativeWorldLayoutCompileResult duplicateResult =
      cr::buildCreativeWorldLayoutPlan(document, duplicate);

  cr::CreativeWorldLayout duplicateLevel = transformableBuildingLayout();
  cr::CreativeWorldLayoutLevel conflictingLevel = duplicateLevel.levels[0];
  conflictingLevel.stableKey = "conflicting_level";
  conflictingLevel.name = "Conflicting Level";
  duplicateLevel.levels.push_back(std::move(conflictingLevel));
  const cr::CreativeWorldLayoutCompileResult duplicateLevelResult =
      cr::buildCreativeWorldLayoutPlan(document, duplicateLevel);

  cr::CreativeWorldLayout duplicateConnector =
      verticalConnectorBuildingLayout();
  duplicateConnector.verticalConnectors[0].stableKey =
      duplicateConnector.rooms[0].stableKey;
  const cr::CreativeWorldLayoutCompileResult duplicateConnectorResult =
      cr::buildCreativeWorldLayoutPlan(document, duplicateConnector);

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
         expect(!duplicateLevelResult.receipt.accepted &&
                    duplicateLevelResult.receipt.status ==
                        cr::CreativeWorldLayoutStatus::InvalidSymbol &&
                    duplicateLevelResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::Level &&
                    duplicateLevelResult.receipt.failedIndex == 1U,
                "duplicate story elevations reject at the exact level") &&
         expect(!duplicateConnectorResult.receipt.accepted &&
                    duplicateConnectorResult.receipt.status ==
                        cr::CreativeWorldLayoutStatus::DuplicateStableKey &&
                    duplicateConnectorResult.receipt.failedTable ==
                        cr::CreativeWorldLayoutTable::VerticalConnector,
                "connector keys share the global layout identity namespace") &&
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

bool generatedOutputAdoptionIsBoundedToInvertibleSources() {
  cr::CreativeWorldLayout objectLayout;
  objectLayout.stableKey = "adoption_layout";
  cr::CreativeWorldLayoutObject sourceObject;
  sourceObject.kind = cr::CreativeObjectKind::Crate;
  sourceObject.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  sourceObject.stableKey = "crate";
  sourceObject.name = "Adoptable Crate";
  sourceObject.pointCells = {2.0, 1.0, 3.0};
  objectLayout.objects.push_back(sourceObject);

  cr::Facade objectFacade;
  static_cast<void>(objectFacade.installDocument(makeDocument(991U)));
  const cr::CreativeWorldLayoutCompileResult objectCompiled =
      cr::buildCreativeWorldLayoutPlan(objectFacade.document(), objectLayout);
  const cr::CreativeWorldLayoutApplyReceipt objectApplied =
      objectCompiled.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlan(objectFacade,
                                             objectCompiled.plan)
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeObject* generatedSource =
      findManaged(objectFacade.document(),
                  "adoption_layout.objects.crate", "crate");
  if (!objectApplied.accepted || generatedSource == nullptr) {
    return expect(false, "direct object adoption fixture generated");
  }

  cr::CreativeObject editedObject = *generatedSource;
  const cr::CreativeVec3 delta{3.0, 2.0, 1.0};
  editedObject.transform.position = {15.0, 4.0, -6.0};
  editedObject.bounds.min.x += delta.x;
  editedObject.bounds.min.y += delta.y;
  editedObject.bounds.min.z += delta.z;
  editedObject.bounds.max.x += delta.x;
  editedObject.bounds.max.y += delta.y;
  editedObject.bounds.max.z += delta.z;
  editedObject.transform.rotationEulerRadians.y = 0.5;
  editedObject.transform.scale = {1.25, 2.0, 0.75};
  editedObject.name = "Adopted Crate";
  const cr::CreativeWorldLayoutAdoptionResult pointAdoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          objectLayout, editedObject,
          objectFacade.document().gridSettings());

  cr::CreativeObject tiltedObject = editedObject;
  tiltedObject.transform.rotationEulerRadians.x = 0.25;
  const cr::CreativeWorldLayoutAdoptionResult tiltedAdoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          objectLayout, tiltedObject,
          objectFacade.document().gridSettings());
  cr::CreativeObject nonFiniteYawObject = editedObject;
  nonFiniteYawObject.transform.rotationEulerRadians.y =
      std::numeric_limits<double>::infinity();
  const cr::CreativeWorldLayoutAdoptionResult nonFiniteYawAdoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          objectLayout, nonFiniteYawObject,
          objectFacade.document().gridSettings());

  cr::CreativeWorldLayout houseLayout = smallHouseLayout();
  houseLayout.buildings[0].tags.push_back("adoption:test");
  cr::Facade houseFacade;
  static_cast<void>(houseFacade.installDocument(makeDocument(992U)));
  const cr::CreativeWorldLayoutCompileResult houseCompiled =
      cr::buildCreativeWorldLayoutPlan(houseFacade.document(), houseLayout);
  const cr::CreativeWorldLayoutApplyReceipt houseApplied =
      houseCompiled.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlan(houseFacade, houseCompiled.plan)
          : cr::CreativeWorldLayoutApplyReceipt{};
  const cr::CreativeObject* floor = nullptr;
  const cr::CreativeObject* wall = nullptr;
  for (const cr::CreativeObject& object : houseFacade.document().objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(houseLayout, object);
    if (provenance.table == cr::CreativeWorldLayoutTable::Box &&
        provenance.index == 0U) {
      floor = &object;
    } else if (provenance.table == cr::CreativeWorldLayoutTable::Wall) {
      wall = &object;
    }
  }
  if (!houseApplied.accepted || floor == nullptr || wall == nullptr) {
    return expect(false, "box adoption fixture generated");
  }
  cr::CreativeObject scaledFloor = *floor;
  scaledFloor.transform.scale.x = 2.0;
  const cr::CreativeWorldLayoutAdoptionResult boxAdoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          houseLayout, scaledFloor, houseFacade.document().gridSettings());
  cr::CreativeObject wrongLayerFloor = scaledFloor;
  ++wrongLayerFloor.layerId;
  const cr::CreativeWorldLayoutAdoptionResult wrongLayerAdoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          houseLayout, wrongLayerFloor,
          houseFacade.document().gridSettings());
  const cr::CreativeWorldLayoutAdoptionResult wallAdoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          houseLayout, *wall, houseFacade.document().gridSettings());

  return expect(pointAdoption.accepted && pointAdoption.changed &&
                    pointAdoption.mode ==
                        cr::CreativeWorldLayoutAdoptionMode::Exact &&
                    pointAdoption.table ==
                        cr::CreativeWorldLayoutTable::Object &&
                    pointAdoption.candidate.objects[0].name ==
                        "Adopted Crate" &&
                    sameVec3(pointAdoption.candidate.objects[0].pointCells,
                             {5.0, 3.0, 4.0}) &&
                    pointAdoption.candidate.objects[0].yawRadians == 0.5 &&
                    sameVec3(pointAdoption.candidate.objects[0].scale,
                             {1.25, 2.0, 0.75}),
                "point object transform maps exactly back to source cells") &&
         expect(!tiltedAdoption.accepted &&
                    tiltedAdoption.status ==
                        cr::CreativeWorldLayoutAdoptionStatus::
                            NonInvertibleTransform,
                "point object pitch cannot masquerade as source yaw") &&
         expect(!nonFiniteYawAdoption.accepted &&
                    nonFiniteYawAdoption.status ==
                        cr::CreativeWorldLayoutAdoptionStatus::
                            NonInvertibleTransform,
                "point object rejects non-finite source yaw") &&
         expect(boxAdoption.accepted && boxAdoption.changed &&
                    boxAdoption.mode ==
                        cr::CreativeWorldLayoutAdoptionMode::Canonicalized &&
                    boxAdoption.candidate.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{-3, 0} &&
                    boxAdoption.candidate.boxes[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{9, 4} &&
                    boxAdoption.candidate.boxes[0].anchorLayer == 0.0 &&
                    boxAdoption.candidate.boxes[0].layerCount == 1U,
                "axis-aligned floor scale folds into semantic dimensions") &&
         expect(!wrongLayerAdoption.accepted &&
                    wrongLayerAdoption.status ==
                        cr::CreativeWorldLayoutAdoptionStatus::ObjectMismatch,
                "box adoption refuses a layer the source cannot own") &&
         expect(!wallAdoption.accepted &&
                    wallAdoption.status ==
                        cr::CreativeWorldLayoutAdoptionStatus::
                            UnsupportedSource,
                "generated wall fragment refuses an invented inverse edit");
}

}  // namespace

int main() {
  const bool ok =
      terrainGroundedBuildingsShiftAsOneAndFillRelief() &&
      stagedLandformGroundsBuildingAgainstPreviewTerrain() &&
      denseTerrainRevisionInvalidatesGroundedPlans() &&
      structuralSurfacesCompileFromExplicitPlanesOnNonUnitGrid() &&
      twoDimensionalBuildingCompilesToExactThreeDimensionalOutput() &&
      rebuildingAndDeletingLayoutNeverDuplicatesOutput() &&
      selectiveRegenerationPreservesIdentityAndManualRefinement() &&
      refinedOutputBlocksSourceChangesUntilExplicitlyRegenerated() &&
      detachResolutionPreservesRefinementAndCreatesFreshManagedOutput() &&
      removedSourceCannotSilentlyDeleteRefinedOutput() &&
      conflictDecisionsAreExactCompleteAndIndependentlyApplied() &&
      buildingRegenerationIsolatedToChangedOwnershipGroup() &&
      openingEditsPatchGeometryWithoutIdentityChurn() &&
      refinementOnUnchangedMemberSurvivesSiblingSourceEdit() &&
      exactMemberConflictChoicesPreserveIdentityAndRejectStaleState() &&
      linkedRemovedMemberCanDetachOrExplicitlyRemove() &&
      authoredChildBlocksRemovalOfManagedParent() &&
      unversionedGeneratedGroupsMigrateOnceThenRemainStable() &&
      authoritativeTerrainAndMaterialApplyAsOneHistoryStep() &&
      watercourseCompileRetainsCanonicalAttachmentsWithoutWaterObjects() &&
      terrainPathOperationsReconcileOwnershipOrderAndIdentity() &&
      roadConstructionReconcilesAndKeepsTravelSurfaceTraversable() &&
      watercourseBridgeReconcilesStructureGradesCollisionAndTraversal() &&
      terrainLandformsReconcileBeforePathsWithStableIdentity() &&
      retainingEdgesReconcileCollisionAndStairTraversal() &&
      terrainPathOperationCapacityFailsAtExactSource() &&
      terrainPathNetworkRequiresExactIntersectionAndBridgeSeams() &&
      buildingTransformPreservesHostedOpeningSemantics() &&
      buildingTransformsRoundTripAndRejectOverflow() &&
      buildingEditKernelsAreAtomicAndRemapOwnership() &&
      roofAperturesFollowBuildingOwnershipAndTemplateSync() &&
      verticalConnectorOwnershipFollowsBuildingKernels() &&
      explicitTopologyFollowsBuildingOwnershipKernels() &&
      buildingTemplatesNormalizeTransformPersistAndStamp() &&
      buildingTemplatePlacementAnalysisAndDetachAreExact() &&
      buildingTemplateSyncIsSafeAtomicAndPersistent() &&
      generatedOutputAdoptionIsBoundedToInvertibleSources() &&
      invalidAndStaleSourcesFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
