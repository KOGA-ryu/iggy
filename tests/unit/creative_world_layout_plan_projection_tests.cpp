#include "app/iggy3d/creative/world/WorldLayoutPlanProjection.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutWallOperations.hpp"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

constexpr double kEpsilon = 1.0e-8;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) noexcept {
  return std::abs(lhs - rhs) <= kEpsilon;
}

std::size_t countRole(const cr::CreativeWorldLayoutPlanProjection& plan,
                      cr::CreativeWorldLayoutPlanRole role,
                      cr::CreativeWorldLayoutPlanLayer layer) {
  std::size_t result = 0U;
  for (const cr::CreativeWorldLayoutPlanPrimitive& primitive :
       plan.primitives) {
    result += primitive.role == role && primitive.layer == layer ? 1U : 0U;
  }
  return result;
}

const cr::CreativeWorldLayoutPlanPrimitive* findSource(
    const cr::CreativeWorldLayoutPlanProjection& plan,
    cr::CreativeWorldLayoutPlanRole role,
    cr::CreativeWorldLayoutPlanLayer layer,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  for (const cr::CreativeWorldLayoutPlanPrimitive& primitive :
       plan.primitives) {
    if (primitive.role == role && primitive.layer == layer &&
        primitive.source.primaryTable == table &&
        primitive.source.primaryIndex == index) {
      return &primitive;
    }
  }
  return nullptr;
}

cr::CreativeWorldLayoutRoom room(std::size_t buildingIndex,
                                 std::size_t levelIndex,
                                 std::string_view key,
                                 cr::CreativeWorldLayoutRect footprint) {
  cr::CreativeWorldLayoutRoom result;
  result.buildingIndex = buildingIndex;
  result.levelIndex = levelIndex;
  result.stableKey = key;
  result.name = key;
  result.footprint = footprint;
  result.wallThicknessCells = 0.25;
  return result;
}

void appendExplicitLRoomTopology(cr::CreativeWorldLayout& layout) {
  const cr::CreativeTerrainCoord2 points[] = {
      {0, 0}, {6, 0}, {6, 2}, {2, 2}, {2, 6}, {0, 6},
  };
  for (std::size_t index = 0U; index < std::size(points); ++index) {
    layout.topologyVertices.push_back(
        {0U, "l_vertex_" + std::to_string(index), points[index]});
  }
  const std::pair<std::size_t, std::size_t> edges[] = {
      {0U, 1U}, {1U, 2U}, {3U, 2U},
      {3U, 4U}, {5U, 4U}, {0U, 5U},
  };
  const bool reversed[] = {false, false, true, false, true, true};
  for (std::size_t index = 0U; index < std::size(edges); ++index) {
    layout.topologyEdges.push_back(
        {0U, "l_edge_" + std::to_string(index), edges[index].first,
         edges[index].second, 0.25});
    layout.roomBoundaries.push_back({0U, index, index, reversed[index]});
  }
}

struct ProjectionFixture {
  cr::CreativeGridSettings grid;
  cr::CreativeWorldLayout layout;
  std::vector<cr::CreativeTerrainContourSegment> contours;
};

ProjectionFixture makeFixture() {
  ProjectionFixture fixture;
  fixture.grid.cellSizeMeters = 1.0;
  fixture.layout.stableKey = "plan_projection_fixture";

  cr::CreativeWorldLayoutBuilding primary;
  primary.stableKey = "primary";
  primary.name = "Primary Estate";
  primary.visible = true;
  fixture.layout.buildings.push_back(primary);
  cr::CreativeWorldLayoutBuilding neighbor;
  neighbor.stableKey = "neighbor";
  neighbor.name = "Neighbor Estate";
  neighbor.visible = true;
  fixture.layout.buildings.push_back(neighbor);

  cr::CreativeWorldLayoutLevel ground;
  ground.buildingIndex = 0U;
  ground.stableKey = "primary_ground";
  ground.name = "Primary Ground";
  ground.floorTopLayer = 0.0;
  ground.wallHeightCells = 3U;
  fixture.layout.levels.push_back(ground);

  cr::CreativeWorldLayoutLevel upper = ground;
  upper.stableKey = "primary_upper";
  upper.name = "Primary Upper";
  upper.floorTopLayer = 3.0;
  upper.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  upper.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  upper.roofOverhangCells = 0.5;
  fixture.layout.levels.push_back(upper);

  cr::CreativeWorldLayoutLevel neighborGround = ground;
  neighborGround.buildingIndex = 1U;
  neighborGround.stableKey = "neighbor_ground";
  neighborGround.name = "Neighbor Ground";
  fixture.layout.levels.push_back(neighborGround);

  fixture.layout.rooms.push_back(room(0U, 0U, "west_room", {{0, 0}, {4, 4}}));
  fixture.layout.rooms.push_back(room(0U, 0U, "east_room", {{4, 0}, {8, 4}}));
  fixture.layout.rooms.push_back(room(0U, 1U, "upper_room", {{0, 0}, {8, 4}}));
  fixture.layout.rooms.push_back(
      room(1U, 2U, "neighbor_room", {{10, 0}, {14, 4}}));

  cr::CreativeWorldLayoutWall partition;
  partition.buildingIndex = 0U;
  partition.stableKey = "partition";
  partition.name = "Partition";
  partition.start = {2, 0};
  partition.end = {2, 4};
  partition.baseLayer = 0.0;
  partition.heightCells = 3U;
  partition.thicknessCells = 0.2;
  fixture.layout.walls.push_back(partition);

  cr::CreativeWorldLayoutOpening groundDoor;
  groundDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  groundDoor.roomIndex = 0U;
  groundDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  groundDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  groundDoor.door.hingeSide = cr::CreativeDoorHingeSide::MinimumEdge;
  groundDoor.door.swingSide = cr::CreativeDoorSwingSide::PositiveNormal;
  groundDoor.door.initialState = cr::CreativeDoorInitialState::Open;
  groundDoor.stableKey = "ground_door";
  groundDoor.name = "Ground Door";
  groundDoor.centerOffsetCells = 2.0;
  groundDoor.widthCells = 1.0;
  groundDoor.cutoutBottomCells = 0.0;
  groundDoor.cutoutHeightCells = 2.1;
  fixture.layout.openings.push_back(groundDoor);

  cr::CreativeWorldLayoutOpening groundWindow;
  groundWindow.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  groundWindow.roomIndex = 1U;
  groundWindow.roomEdge = cr::CreativeWorldLayoutRoomEdge::East;
  groundWindow.kind = cr::CreativeBuildingOpeningKind::Window;
  groundWindow.stableKey = "ground_window";
  groundWindow.name = "Ground Window";
  groundWindow.centerOffsetCells = 2.0;
  groundWindow.widthCells = 1.0;
  groundWindow.cutoutBottomCells = 1.0;
  groundWindow.cutoutHeightCells = 1.4;
  groundWindow.window.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  fixture.layout.openings.push_back(groundWindow);

  cr::CreativeWorldLayoutOpening upperDoor = groundDoor;
  upperDoor.roomIndex = 2U;
  upperDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::South;
  upperDoor.door.initialState = cr::CreativeDoorInitialState::Closed;
  upperDoor.stableKey = "upper_door";
  upperDoor.name = "Upper Door";
  upperDoor.centerOffsetCells = 4.0;
  fixture.layout.openings.push_back(upperDoor);

  cr::CreativeWorldLayoutVerticalConnector stair;
  stair.buildingIndex = 0U;
  stair.lowerRoomIndex = 0U;
  stair.upperRoomIndex = 2U;
  stair.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Stair;
  stair.direction = cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
  stair.stableKey = "stair";
  stair.name = "Main Stair";
  stair.footprint = {{1, 1}, {3, 4}};
  fixture.layout.verticalConnectors.push_back(stair);

  cr::CreativeWorldLayoutTerrainProfile plateau;
  plateau.stableKey = "plateau";
  plateau.kind = cr::CreativeTerrainRecipeKind::Plateau;
  plateau.center = {20, 20};
  plateau.radiusCells = 3U;
  fixture.layout.terrainProfiles.push_back(plateau);

  cr::CreativeWorldLayoutTerrainPath road;
  road.stableKey = "road";
  road.recipe.kind = cr::CreativeTerrainPathKind::Road;
  road.recipe.elevation = cr::CreativeTerrainPathElevation::Follow;
  road.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Crowned;
  road.recipe.nextPointId = 4U;
  road.recipe.points = {
      {1U, {0, 10}, 4U, 1U, 0U, 0},
      {2U, {5, 10}, 4U, 1U, 0U, 0},
      {3U, {5, 15}, 4U, 1U, 0U, 0},
  };
  fixture.layout.terrainPaths.push_back(road);

  cr::CreativeWorldLayoutObject bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  bridge.stableKey = "bridge";
  bridge.name = "Bridge";
  bridge.boundsCells = {{8.0, 0.0, 6.0}, {10.0, 0.35, 8.0}};
  fixture.layout.objects.push_back(bridge);

  cr::CreativeWorldLayoutObject player;
  player.kind = cr::CreativeObjectKind::SpawnPoint;
  player.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  player.stableKey = "player";
  player.name = "Player Spawn";
  player.pointCells = {1.0, 0.0, 6.0};
  fixture.layout.objects.push_back(player);

  cr::CreativeWorldLayoutObject npc = player;
  npc.kind = cr::CreativeObjectKind::NpcSpawn;
  npc.stableKey = "npc";
  npc.name = "NPC Spawn";
  npc.pointCells = {2.0, 0.0, 6.0};
  fixture.layout.objects.push_back(npc);

  cr::CreativeWorldLayoutObject rock = player;
  rock.kind = cr::CreativeObjectKind::Rock;
  rock.stableKey = "rock";
  rock.name = "Rock";
  rock.pointCells = {6.0, 0.0, 6.0};
  rock.assetSourceBoundsMeters = {{-0.5, 0.0, -0.25}, {0.5, 1.0, 0.25}};
  rock.hasAssetSourceBounds = true;
  rock.yawRadians = 0.78539816339744830962;
  fixture.layout.objects.push_back(rock);

  cr::CreativeWorldLayoutObject upperProp = player;
  upperProp.kind = cr::CreativeObjectKind::Prop;
  upperProp.stableKey = "upper_prop";
  upperProp.name = "Upper Prop";
  upperProp.pointCells = {1.0, 3.0, 1.0};
  fixture.layout.objects.push_back(upperProp);

  cr::CreativeWorldLayoutObject enemy = player;
  enemy.kind = cr::CreativeObjectKind::EnemySpawn;
  enemy.stableKey = "enemy";
  enemy.name = "Enemy Spawn";
  enemy.pointCells = {3.0, 0.0, 6.0};
  fixture.layout.objects.push_back(enemy);

  fixture.contours = {
      {{-2.0, -2.0}, {2.0, -2.0}, 2U, false},
      {{-2.0, -1.0}, {2.0, -1.0}, 10U, true},
  };
  return fixture;
}

cr::CreativeWorldLayoutPlanProjection project(const ProjectionFixture& fixture,
                                               std::size_t activeLevelIndex) {
  return cr::projectCreativeWorldLayoutPlan(
      {&fixture.layout, fixture.grid, activeLevelIndex, fixture.contours});
}

bool vocabularyIsClosedAndNamed() {
  bool rolesNamed = true;
  for (std::size_t index = 0U;
       index < static_cast<std::size_t>(cr::CreativeWorldLayoutPlanRole::Count);
       ++index) {
    rolesNamed &= cr::toString(
                      static_cast<cr::CreativeWorldLayoutPlanRole>(index)) !=
                  "Unknown";
  }
  return expect(rolesNamed, "every plan role has a stable name") &&
         expect(cr::toString(cr::CreativeWorldLayoutPlanRole::Count) ==
                    "Unknown",
                "out-of-range plan role fails visibly") &&
         expect(cr::toString(
                    cr::CreativeWorldLayoutPlanLayer::LowerContext) ==
                    "LowerContext" &&
                    cr::toString(cr::CreativeWorldLayoutPlanLayer::Active) ==
                        "Active" &&
                    cr::toString(
                        cr::CreativeWorldLayoutPlanLayer::UpperContext) ==
                        "UpperContext" &&
                    cr::toString(cr::CreativeWorldLayoutPlanLayer::Overhead) ==
                        "Overhead",
                "plan layers own stable names");
}

bool groundDatumProjectsBothBuildingsAndCutOpenings() {
  const ProjectionFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 0U);
  const auto* neighborFloor =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Room, 3U);
  const auto* doorSwing =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::DoorSwing,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Opening, 0U);
  const auto* shared =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::SharedBoundary,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Room, 0U);
  const auto* partition =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::InteriorPartition,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Wall, 0U);

  bool wallCrossesDoor = false;
  for (const cr::CreativeWorldLayoutPlanPrimitive& primitive :
       plan.primitives) {
    if (primitive.role != cr::CreativeWorldLayoutPlanRole::ExteriorWall ||
        primitive.kind !=
            cr::CreativeWorldLayoutPlanPrimitiveKind::Segment ||
        !near(primitive.points[0].z, 0.0) ||
        !near(primitive.points[1].z, 0.0)) {
      continue;
    }
    const double minimumX =
        std::min(primitive.points[0].x, primitive.points[1].x);
    const double maximumX =
        std::max(primitive.points[0].x, primitive.points[1].x);
    wallCrossesDoor |= minimumX < 2.0 && maximumX > 2.0;
  }

  return expect(plan.accepted &&
                    plan.status ==
                        cr::CreativeWorldLayoutPlanProjectionStatus::Ready,
                "valid ground plan projects") &&
         expect(plan.receipt.activeLevelCount == 2U,
                "same-datum levels from both buildings are active") &&
         expect(neighborFloor != nullptr,
                "neighbor ground floor is visible beside primary estate") &&
         expect(doorSwing != nullptr &&
                    doorSwing->kind ==
                        cr::CreativeWorldLayoutPlanPrimitiveKind::Arc &&
                    near(doorSwing->points[0].x, 1.5) &&
                    near(doorSwing->points[0].z, 0.0) &&
                    near(doorSwing->radiusCells, 1.0) &&
                    near(doorSwing->startRadians, 0.0) &&
                    near(doorSwing->sweepRadians,
                         1.57079632679489661923),
                "door settings resolve exact hinge and positive-normal arc") &&
         expect(!wallCrossesDoor,
                "door cut physically interrupts the exterior wall") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::Window,
                          cr::CreativeWorldLayoutPlanLayer::Active) == 2U,
                "window projects two glazing lines") &&
         expect(countRole(plan,
                          cr::CreativeWorldLayoutPlanRole::WindowShutter,
                          cr::CreativeWorldLayoutPlanLayer::Active) == 1U,
                "paired shutters add an opaque plan line") &&
         expect(shared != nullptr &&
                    shared->source.secondaryTable ==
                        cr::CreativeWorldLayoutTable::Room &&
                    shared->source.secondaryIndex == 1U,
                "shared boundary retains both room owners") &&
         expect(partition != nullptr,
                "explicit wall remains an interior partition") &&
         expect(plan.bounds.valid && plan.bounds.minimum.x <= -2.0 &&
                    plan.bounds.maximum.x >= 23.0,
                "plan bounds include contours and terrain profile");
}

bool everyDoorSettingProjectsTheAuthoredHingeAndSwing() {
  struct DoorCase {
    cr::CreativeDoorHingeSide hinge;
    cr::CreativeDoorSwingSide swingSide;
    double hingeX;
    double sweep;
  };
  constexpr DoorCase kCases[] = {
      {cr::CreativeDoorHingeSide::MinimumEdge,
       cr::CreativeDoorSwingSide::NegativeNormal, 1.5,
       -1.57079632679489661923},
      {cr::CreativeDoorHingeSide::MinimumEdge,
       cr::CreativeDoorSwingSide::PositiveNormal, 1.5,
       1.57079632679489661923},
      {cr::CreativeDoorHingeSide::MaximumEdge,
       cr::CreativeDoorSwingSide::NegativeNormal, 2.5,
       1.57079632679489661923},
      {cr::CreativeDoorHingeSide::MaximumEdge,
       cr::CreativeDoorSwingSide::PositiveNormal, 2.5,
       -1.57079632679489661923},
  };
  for (const DoorCase& doorCase : kCases) {
    ProjectionFixture fixture = makeFixture();
    fixture.layout.openings[0].door.hingeSide = doorCase.hinge;
    fixture.layout.openings[0].door.swingSide = doorCase.swingSide;
    fixture.layout.openings[0].door.initialState =
        cr::CreativeDoorInitialState::Open;
    const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 0U);
    const auto* swing =
        findSource(plan, cr::CreativeWorldLayoutPlanRole::DoorSwing,
                   cr::CreativeWorldLayoutPlanLayer::Active,
                   cr::CreativeWorldLayoutTable::Opening, 0U);
    if (!expect(plan.accepted && swing != nullptr &&
                    near(swing->points[0].x, doorCase.hingeX) &&
                    near(swing->points[0].z, 0.0) &&
                    near(swing->sweepRadians, doorCase.sweep),
                "open door settings preserve hinge side and normal")) {
      return false;
    }
  }

  ProjectionFixture closedFixture = makeFixture();
  closedFixture.layout.openings[0].door.initialState =
      cr::CreativeDoorInitialState::Closed;
  const cr::CreativeWorldLayoutPlanProjection closed =
      project(closedFixture, 0U);
  const auto* closedLeaf =
      findSource(closed, cr::CreativeWorldLayoutPlanRole::Door,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Opening, 0U);
  const auto* closedSwing =
      findSource(closed, cr::CreativeWorldLayoutPlanRole::DoorSwing,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Opening, 0U);

  ProjectionFixture doubleFixture = makeFixture();
  doubleFixture.layout.openings[0].door.leafArrangement =
      cr::CreativeDoorLeafArrangement::Double;
  const cr::CreativeWorldLayoutPlanProjection doubleDoor =
      project(doubleFixture, 0U);
  return expect(closed.accepted && closedLeaf != nullptr &&
                    closedSwing != nullptr &&
                    near(closedLeaf->points[0].x, 1.5) &&
                    near(closedLeaf->points[1].x, 2.5) &&
                    near(closedLeaf->points[0].z, 0.0) &&
                    near(closedLeaf->points[1].z, 0.0),
                "closed door keeps its leaf closed and its swing legible") &&
         expect(doubleDoor.accepted &&
                    countRole(doubleDoor, cr::CreativeWorldLayoutPlanRole::Door,
                              cr::CreativeWorldLayoutPlanLayer::Active) == 2U &&
                    countRole(
                        doubleDoor,
                        cr::CreativeWorldLayoutPlanRole::DoorSwing,
                        cr::CreativeWorldLayoutPlanLayer::Active) == 2U,
                "double door projects two opposed leaves and two swing arcs");
}

bool openingFacingProjectsAnExplicitFrontSide() {
  ProjectionFixture positiveFixture = makeFixture();
  positiveFixture.layout.openings[0].door.initialState =
      cr::CreativeDoorInitialState::Closed;
  positiveFixture.layout.openings[0].facing =
      cr::CreativeBuildingOpeningFacing::PositiveNormal;
  const cr::CreativeWorldLayoutPlanProjection positive =
      project(positiveFixture, 0U);
  const auto* positiveFacing =
      findSource(positive, cr::CreativeWorldLayoutPlanRole::OpeningFacing,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Opening, 0U);

  ProjectionFixture negativeFixture = positiveFixture;
  negativeFixture.layout.openings[0].facing =
      cr::CreativeBuildingOpeningFacing::NegativeNormal;
  const cr::CreativeWorldLayoutPlanProjection negative =
      project(negativeFixture, 0U);
  const auto* negativeFacing =
      findSource(negative, cr::CreativeWorldLayoutPlanRole::OpeningFacing,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Opening, 0U);

  return expect(positive.accepted && positiveFacing != nullptr &&
                    near(positiveFacing->points[0].x, 2.0) &&
                    near(positiveFacing->points[0].z, 0.0) &&
                    near(positiveFacing->points[1].x, 2.0) &&
                    near(positiveFacing->points[1].z, 0.35),
                "positive opening facing projects toward the positive wall normal") &&
         expect(negative.accepted && negativeFacing != nullptr &&
                    near(negativeFacing->points[0].x, 2.0) &&
                    near(negativeFacing->points[0].z, 0.0) &&
                    near(negativeFacing->points[1].x, 2.0) &&
                    near(negativeFacing->points[1].z, -0.35),
                "negative opening facing projects toward the negative wall normal");
}

bool upperStoreyAddsContextRoofAndVerticalFiltering() {
  const ProjectionFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 1U);
  const auto* lowerRoom =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                 cr::CreativeWorldLayoutPlanLayer::LowerContext,
                 cr::CreativeWorldLayoutTable::Room, 0U);
  const auto* activeRoom =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Room, 2U);
  const auto* upperDoor =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::Door,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Opening, 2U);
  const auto* upperProp =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::Object,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Object, 4U);
  const auto* bridge =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::Bridge,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Object, 0U);

  return expect(plan.accepted && near(plan.activeFloorTopLayer, 3.0),
                "upper storey datum projects") &&
         expect(plan.receipt.activeLevelCount == 1U &&
                    plan.receipt.contextLevelCount == 1U &&
                    plan.receipt.lowerContextLevelCount == 1U &&
                    plan.receipt.upperContextLevelCount == 0U,
                "upper plan has one active and one lower context level") &&
         expect(lowerRoom != nullptr && activeRoom != nullptr,
                "lower room is context and upper room is active") &&
         expect(upperDoor != nullptr,
                "cut plane selects upper opening from merged facade") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::RoofOutline,
                          cr::CreativeWorldLayoutPlanLayer::Overhead) == 1U &&
                    countRole(
                        plan, cr::CreativeWorldLayoutPlanRole::RoofRidge,
                        cr::CreativeWorldLayoutPlanLayer::Overhead) == 1U,
                "top storey emits roof outline and gable ridge overhead") &&
         expect(upperProp != nullptr && bridge == nullptr,
                "objects are filtered to the active vertical storey band") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::Stair,
                          cr::CreativeWorldLayoutPlanLayer::Active) == 9U,
                "stair footprint axis arrow and five treads are explicit");
}

bool everySimpleRoofStyleProjectsItsAuthoredPlanSemantics() {
  ProjectionFixture fixture = makeFixture();
  cr::CreativeWorldLayoutLevel& roof = fixture.layout.levels[1];

  roof.roofStyle = cr::CreativeStructuralRoofStyle::Flat;
  const cr::CreativeWorldLayoutPlanProjection flat = project(fixture, 1U);

  roof.roofStyle = cr::CreativeStructuralRoofStyle::Shed;
  roof.roofSlopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::PositiveX;
  const cr::CreativeWorldLayoutPlanProjection shed = project(fixture, 1U);
  const auto* shedHigh = findSource(
      shed, cr::CreativeWorldLayoutPlanRole::RoofRidge,
      cr::CreativeWorldLayoutPlanLayer::Overhead,
      cr::CreativeWorldLayoutTable::Level, 1U);

  roof.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  roof.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  const cr::CreativeWorldLayoutPlanProjection gable = project(fixture, 1U);

  roof.roofStyle = cr::CreativeStructuralRoofStyle::Hip;
  const cr::CreativeWorldLayoutPlanProjection hip = project(fixture, 1U);
  bool hipMainRidge = false;
  std::size_t hipCornerCount = 0U;
  for (const cr::CreativeWorldLayoutPlanPrimitive& primitive :
       hip.primitives) {
    if (primitive.role != cr::CreativeWorldLayoutPlanRole::RoofRidge ||
        primitive.layer != cr::CreativeWorldLayoutPlanLayer::Overhead ||
        primitive.pointCount != 2U) {
      continue;
    }
    const cr::CreativeWorldLayoutPlanPoint first = primitive.points[0];
    const cr::CreativeWorldLayoutPlanPoint second = primitive.points[1];
    hipMainRidge |= near(first.x, 2.0) && near(first.z, 2.0) &&
                    near(second.x, 6.0) && near(second.z, 2.0);
    const bool firstCorner =
        (near(first.x, -0.5) || near(first.x, 8.5)) &&
        (near(first.z, -0.5) || near(first.z, 4.5));
    hipCornerCount += firstCorner ? 1U : 0U;
  }

  return expect(flat.accepted &&
                    countRole(flat,
                              cr::CreativeWorldLayoutPlanRole::RoofOutline,
                              cr::CreativeWorldLayoutPlanLayer::Overhead) ==
                        1U &&
                    countRole(flat,
                              cr::CreativeWorldLayoutPlanRole::RoofRidge,
                              cr::CreativeWorldLayoutPlanLayer::Overhead) ==
                        0U,
                "flat roof plan is one exact outline without invented ridge") &&
         expect(shed.accepted && shedHigh != nullptr &&
                    near(shedHigh->points[0].x, -0.5) &&
                    near(shedHigh->points[1].x, -0.5) &&
                    near(shedHigh->points[0].z, -0.5) &&
                    near(shedHigh->points[1].z, 4.5),
                "positive-x shed marks its authored high edge") &&
         expect(gable.accepted &&
                    countRole(gable,
                              cr::CreativeWorldLayoutPlanRole::RoofRidge,
                              cr::CreativeWorldLayoutPlanLayer::Overhead) ==
                        1U,
                "gable plan emits one full ridge") &&
         expect(hip.accepted &&
                    countRole(hip,
                              cr::CreativeWorldLayoutPlanRole::RoofRidge,
                              cr::CreativeWorldLayoutPlanLayer::Overhead) ==
                        5U &&
                    hipMainRidge && hipCornerCount == 4U,
                "hip plan emits one shortened ridge and four corner hips");
}

bool groundStoreyCanShowUpperContextIndependently() {
  const ProjectionFixture fixture = makeFixture();
  cr::CreativeWorldLayoutPlanProjectionRequest request;
  request.layout = &fixture.layout;
  request.grid = fixture.grid;
  request.activeLevelIndex = 0U;
  request.includeLowerLevelContext = false;
  request.includeUpperLevelContext = true;
  request.includeRoofOverhead = false;
  const cr::CreativeWorldLayoutPlanProjection plan =
      cr::projectCreativeWorldLayoutPlan(request);

  const auto* upperRoom =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                 cr::CreativeWorldLayoutPlanLayer::UpperContext,
                 cr::CreativeWorldLayoutTable::Room, 2U);
  const auto* activeRoom =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Room, 0U);

  return expect(plan.accepted && activeRoom != nullptr && upperRoom != nullptr,
                "ground storey projects active and upper context geometry") &&
         expect(plan.receipt.contextLevelCount == 1U &&
                    plan.receipt.lowerContextLevelCount == 0U &&
                    plan.receipt.upperContextLevelCount == 1U,
                "upper context receipt remains directionally explicit") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                          cr::CreativeWorldLayoutPlanLayer::LowerContext) ==
                        0U &&
                    plan.receipt.roofPrimitiveCount == 0U,
                "context and roof visibility flags remain independent");
}

bool displayFlagsOnlyRemoveTheirPresentationLayers() {
  const ProjectionFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutPlanProjection complete = project(fixture, 1U);
  cr::CreativeWorldLayoutPlanProjectionRequest request;
  request.layout = &fixture.layout;
  request.grid = fixture.grid;
  request.activeLevelIndex = 1U;
  request.contours = fixture.contours;
  request.includeLowerLevelContext = false;
  request.includeRoofOverhead = false;
  const cr::CreativeWorldLayoutPlanProjection reduced =
      cr::projectCreativeWorldLayoutPlan(request);

  return expect(complete.accepted && reduced.accepted,
                "display flags preserve a valid plan") &&
         expect(reduced.receipt.contextLevelCount == 0U &&
                    reduced.receipt.lowerContextLevelCount == 0U &&
                    reduced.receipt.upperContextLevelCount == 0U &&
                    reduced.receipt.roofPrimitiveCount == 0U,
                "disabled context and roof emit no presentation primitives") &&
         expect(findSource(reduced, cr::CreativeWorldLayoutPlanRole::RoomFloor,
                           cr::CreativeWorldLayoutPlanLayer::Active,
                           cr::CreativeWorldLayoutTable::Room, 2U) != nullptr &&
                    countRole(reduced,
                              cr::CreativeWorldLayoutPlanRole::RoomFloor,
                              cr::CreativeWorldLayoutPlanLayer::Active) ==
                        countRole(complete,
                                  cr::CreativeWorldLayoutPlanRole::RoomFloor,
                                  cr::CreativeWorldLayoutPlanLayer::Active),
                "active storey semantics survive visibility changes") &&
         expect(reduced.receipt.terrainPrimitiveCount ==
                        complete.receipt.terrainPrimitiveCount &&
                    reduced.receipt.contourPrimitiveCount ==
                        complete.receipt.contourPrimitiveCount,
                "architecture visibility does not alter terrain drafting");
}

bool levelNavigationUsesPhysicalDatums() {
  ProjectionFixture fixture = makeFixture();
  cr::CreativeWorldLayoutLevel neighborUpper = fixture.layout.levels[1U];
  neighborUpper.buildingIndex = 1U;
  neighborUpper.stableKey = "neighbor_upper";
  neighborUpper.name = "Neighbor Upper";
  fixture.layout.levels.push_back(neighborUpper);
  cr::CreativeWorldLayoutLevel primaryTop = fixture.layout.levels[1U];
  primaryTop.floorTopLayer = 6.0;
  primaryTop.stableKey = "primary_top";
  primaryTop.name = "Primary Top";
  fixture.layout.levels.push_back(primaryTop);
  cr::CreativeWorldLayoutBuilding hiddenBuilding =
      fixture.layout.buildings[1U];
  hiddenBuilding.stableKey = "hidden_neighbor";
  hiddenBuilding.name = "Hidden Neighbor";
  hiddenBuilding.visible = false;
  fixture.layout.buildings.push_back(hiddenBuilding);
  cr::CreativeWorldLayoutLevel hiddenMiddle = fixture.layout.levels[1U];
  hiddenMiddle.buildingIndex = 2U;
  hiddenMiddle.floorTopLayer = 1.5;
  hiddenMiddle.stableKey = "hidden_middle";
  hiddenMiddle.name = "Hidden Middle";
  fixture.layout.levels.push_back(hiddenMiddle);

  using Direction = cr::CreativeWorldLayoutLevelNavigationDirection;
  using Status = cr::CreativeWorldLayoutLevelNavigationStatus;
  const auto primaryUp =
      cr::navigateCreativeWorldLayoutLevel(fixture.layout, 0U,
                                           Direction::Higher);
  const auto neighborUp =
      cr::navigateCreativeWorldLayoutLevel(fixture.layout, 2U,
                                           Direction::Higher);
  const auto neighborUpperFallback = cr::navigateCreativeWorldLayoutLevel(
      fixture.layout, 3U, Direction::Higher);
  const auto topDown = cr::navigateCreativeWorldLayoutLevel(
      fixture.layout, 4U, Direction::Lower);
  const auto groundDown = cr::navigateCreativeWorldLayoutLevel(
      fixture.layout, 0U, Direction::Lower);
  const auto topUp = cr::navigateCreativeWorldLayoutLevel(
      fixture.layout, 4U, Direction::Higher);
  const auto invalidActive = cr::navigateCreativeWorldLayoutLevel(
      fixture.layout, fixture.layout.levels.size(), Direction::Higher);
  const auto invalidDirection = cr::navigateCreativeWorldLayoutLevel(
      fixture.layout, 0U, Direction::Count);

  cr::CreativeWorldLayout hiddenActive = fixture.layout;
  hiddenActive.buildings[0U].visible = false;
  const auto invalidHiddenActive = cr::navigateCreativeWorldLayoutLevel(
      hiddenActive, 0U, Direction::Higher);

  cr::CreativeWorldLayout malformed = fixture.layout;
  malformed.levels[3U].wallHeightCells = 0U;
  const auto invalidLayout = cr::navigateCreativeWorldLayoutLevel(
      malformed, 0U, Direction::Higher);

  return expect(primaryUp.status == Status::Ready &&
                    primaryUp.targetLevelIndex == 1U &&
                    near(primaryUp.targetFloorTopLayer, 3.0),
                "higher chooses the nearest distinct physical datum") &&
         expect(neighborUp.status == Status::Ready &&
                    neighborUp.targetLevelIndex == 3U,
                "same-datum ties prefer the current building") &&
         expect(neighborUpperFallback.status == Status::Ready &&
                    neighborUpperFallback.targetLevelIndex == 4U &&
                    near(neighborUpperFallback.targetFloorTopLayer, 6.0),
                "navigation falls back across buildings deterministically") &&
         expect(topDown.status == Status::Ready &&
                    topDown.targetLevelIndex == 1U,
                "lower navigation ignores table order, hidden floors, and keeps ownership") &&
         expect(groundDown.status == Status::Boundary &&
                    topUp.status == Status::Boundary,
                "first and last physical floors report a boundary") &&
         expect(invalidActive.status == Status::InvalidActiveLevel &&
                    invalidHiddenActive.status ==
                        Status::InvalidActiveLevel &&
                    invalidDirection.status == Status::InvalidDirection &&
                    invalidLayout.status == Status::InvalidLayout,
                "invalid navigation inputs fail closed");
}

bool semanticSourcesResolveToExactStoreys() {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuilding west;
  west.stableKey = "west";
  west.name = "West";
  layout.buildings.push_back(west);
  cr::CreativeWorldLayoutBuilding east = west;
  east.stableKey = "east";
  east.name = "East";
  layout.buildings.push_back(east);

  const auto addLevel = [&layout](std::size_t buildingIndex,
                                  std::string stableKey, double datum) {
    cr::CreativeWorldLayoutLevel level;
    level.buildingIndex = buildingIndex;
    level.stableKey = std::move(stableKey);
    level.name = level.stableKey;
    level.floorTopLayer = datum;
    layout.levels.push_back(std::move(level));
  };
  addLevel(0U, "west_ground", 0.0);
  addLevel(0U, "west_upper", 3.0);
  addLevel(1U, "east_ground", 0.0);
  addLevel(1U, "east_upper", 3.0);

  const auto addRoom = [&layout](std::size_t buildingIndex,
                                 std::size_t levelIndex,
                                 std::string stableKey) {
    cr::CreativeWorldLayoutRoom room;
    room.buildingIndex = buildingIndex;
    room.levelIndex = levelIndex;
    room.stableKey = std::move(stableKey);
    room.name = room.stableKey;
    room.footprint = {{0, 0}, {4, 4}};
    layout.rooms.push_back(std::move(room));
  };
  addRoom(0U, 0U, "west_ground_room");
  addRoom(0U, 1U, "west_upper_room");
  addRoom(1U, 2U, "east_ground_room");
  addRoom(1U, 3U, "east_upper_room");

  cr::CreativeWorldLayoutBox groundFloor;
  groundFloor.buildingIndex = 0U;
  groundFloor.stableKey = "ground_floor";
  groundFloor.name = "Ground Floor";
  groundFloor.footprint = {{0, 0}, {4, 4}};
  groundFloor.anchorLayer = 0.0;
  layout.boxes.push_back(groundFloor);
  cr::CreativeWorldLayoutBox upperFloor = groundFloor;
  upperFloor.stableKey = "upper_floor";
  upperFloor.name = "Upper Floor";
  upperFloor.anchorLayer = 3.0;
  layout.boxes.push_back(upperFloor);

  cr::CreativeWorldLayoutWall groundWall;
  groundWall.buildingIndex = 0U;
  groundWall.stableKey = "ground_wall";
  groundWall.name = "Ground Wall";
  groundWall.start = {0, 0};
  groundWall.end = {4, 0};
  groundWall.baseLayer = 0.0;
  groundWall.heightCells = 3U;
  layout.walls.push_back(groundWall);
  cr::CreativeWorldLayoutWall upperWall = groundWall;
  upperWall.stableKey = "upper_wall";
  upperWall.name = "Upper Wall";
  upperWall.baseLayer = 3.0;
  layout.walls.push_back(upperWall);
  cr::CreativeWorldLayoutWall tallWall = groundWall;
  tallWall.stableKey = "tall_wall";
  tallWall.name = "Tall Wall";
  tallWall.heightCells = 6U;
  layout.walls.push_back(tallWall);

  cr::CreativeWorldLayoutOpening groundDoor;
  groundDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  groundDoor.wallIndex = 2U;
  groundDoor.stableKey = "ground_door";
  groundDoor.name = "Ground Door";
  layout.openings.push_back(groundDoor);
  cr::CreativeWorldLayoutOpening upperWindow = groundDoor;
  upperWindow.stableKey = "upper_window";
  upperWindow.name = "Upper Window";
  upperWindow.kind = cr::CreativeBuildingOpeningKind::Window;
  upperWindow.cutoutBottomCells = 3.5;
  upperWindow.cutoutHeightCells = 1.0;
  layout.openings.push_back(upperWindow);

  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = 0U;
  connector.lowerRoomIndex = 0U;
  connector.upperRoomIndex = 1U;
  connector.stableKey = "stair";
  connector.name = "Stair";
  connector.footprint = {{1, 1}, {2, 3}};
  layout.verticalConnectors.push_back(connector);

  const std::size_t invalid = cr::kInvalidCreativeWorldLayoutIndex;
  return expect(
      cr::creativeWorldLayoutSourceLevelAtDatum(
          layout, cr::CreativeWorldLayoutTable::Room, 3U, 3.0) == 3U &&
          cr::creativeWorldLayoutSourceLevelAtDatum(
              layout, cr::CreativeWorldLayoutTable::Box, 0U, 0.0) == 0U &&
          cr::creativeWorldLayoutSourceLevelAtDatum(
              layout, cr::CreativeWorldLayoutTable::Box, 0U, 3.0) == invalid &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Wall, 0U, 0U) &&
          !cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Wall, 0U, 1U) &&
          !cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Wall, 1U, 0U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Wall, 1U, 1U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Wall, 2U, 0U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Wall, 2U, 1U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Opening, 0U, 0U) &&
          !cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Opening, 0U, 1U) &&
          !cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Opening, 1U, 0U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::Opening, 1U, 1U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
              0U) &&
          cr::creativeWorldLayoutSourceTouchesLevel(
              layout, cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
              1U),
      "semantic sources resolve to one floor while spanning structures stay intentional");
}

bool terrainContoursAndObjectsCarrySemanticMetadata() {
  const ProjectionFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 0U);
  const auto* plateau =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::TerrainProfile,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  const auto* road =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::TerrainPath,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::TerrainPath, 0U);
  const auto* rock =
      findSource(plan, cr::CreativeWorldLayoutPlanRole::Object,
                 cr::CreativeWorldLayoutPlanLayer::Active,
                 cr::CreativeWorldLayoutTable::Object, 3U);

  bool sawMajor = false;
  bool sawMinor = false;
  for (const cr::CreativeWorldLayoutPlanPrimitive& primitive :
       plan.primitives) {
    if (primitive.role == cr::CreativeWorldLayoutPlanRole::Contour) {
      sawMajor |= primitive.contourMajor;
      sawMinor |= !primitive.contourMajor;
    }
  }

  return expect(plateau != nullptr &&
                    plateau->kind ==
                        cr::CreativeWorldLayoutPlanPrimitiveKind::Circle &&
                    plateau->terrainKind ==
                        cr::CreativeTerrainRecipeKind::Plateau &&
                    near(plateau->radiusCells, 3.0),
                "terrain profile retains kind and radius") &&
         expect(road != nullptr &&
                    road->terrainKind == cr::CreativeTerrainRecipeKind::Road &&
                    near(road->widthCells, 3.0) &&
                    countRole(plan,
                              cr::CreativeWorldLayoutPlanRole::TerrainPath,
                              cr::CreativeWorldLayoutPlanLayer::Active) == 2U,
                "terrain path retains kind width and segment order") &&
         expect(sawMajor && sawMinor &&
                    plan.receipt.contourPrimitiveCount == 2U,
                "major and minor contour metadata survives projection") &&
         expect(rock != nullptr &&
                    rock->kind ==
                        cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon &&
                    rock->objectKind == cr::CreativeObjectKind::Rock &&
                    !near(rock->points[0].x, rock->points[1].x) &&
                    !near(rock->points[0].z, rock->points[1].z),
                "asset footprint retains kind and oriented geometry") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::Bridge,
                          cr::CreativeWorldLayoutPlanLayer::Active) == 1U &&
                    countRole(
                        plan, cr::CreativeWorldLayoutPlanRole::PlayerSpawn,
                        cr::CreativeWorldLayoutPlanLayer::Active) == 1U &&
                    countRole(plan,
                              cr::CreativeWorldLayoutPlanRole::NpcSpawn,
                              cr::CreativeWorldLayoutPlanLayer::Active) == 2U,
                "gameplay and bridge objects own semantic roles");
}

bool rampOmitsStairTreadsAndTerrainOnlyPlansRemainValid() {
  ProjectionFixture rampFixture = makeFixture();
  rampFixture.layout.verticalConnectors[0].kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  const cr::CreativeWorldLayoutPlanProjection ramp =
      project(rampFixture, 0U);

  cr::CreativeWorldLayout terrainOnly;
  terrainOnly.stableKey = "terrain_only";
  cr::CreativeWorldLayoutTerrainProfile hill;
  hill.stableKey = "hill";
  hill.kind = cr::CreativeTerrainRecipeKind::Hill;
  hill.center = {3, 4};
  hill.radiusCells = 2U;
  terrainOnly.terrainProfiles.push_back(hill);
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 1.0;
  cr::CreativeWorldLayoutPlanProjectionRequest terrainRequest;
  terrainRequest.layout = &terrainOnly;
  terrainRequest.grid = grid;
  terrainRequest.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeWorldLayoutPlanProjection terrain =
      cr::projectCreativeWorldLayoutPlan(terrainRequest);

  return expect(ramp.accepted &&
                    countRole(ramp, cr::CreativeWorldLayoutPlanRole::Ramp,
                              cr::CreativeWorldLayoutPlanLayer::Active) == 4U &&
                    countRole(ramp, cr::CreativeWorldLayoutPlanRole::Stair,
                              cr::CreativeWorldLayoutPlanLayer::Active) == 0U,
                "ramp emits footprint axis and arrow without stair treads") &&
         expect(terrain.accepted &&
                    terrain.receipt.activeLevelCount == 0U &&
                    countRole(
                        terrain,
                        cr::CreativeWorldLayoutPlanRole::TerrainProfile,
                        cr::CreativeWorldLayoutPlanLayer::Active) == 1U,
                "terrain-only source projects without a fabricated level");
}

bool invalidInputFailsAtomically() {
  ProjectionFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutPlanProjection absent =
      cr::projectCreativeWorldLayoutPlan({});

  cr::CreativeWorldLayoutPlanProjectionRequest badCut{
      &fixture.layout, fixture.grid, 0U, fixture.contours};
  badCut.cutPlaneHeightMeters = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutPlanProjection invalidCut =
      cr::projectCreativeWorldLayoutPlan(badCut);

  const cr::CreativeWorldLayoutPlanProjection invalidLevel =
      cr::projectCreativeWorldLayoutPlan(
          {&fixture.layout, fixture.grid, 99U, fixture.contours});

  fixture.layout.openings[0].widthCells = 30.0;
  const cr::CreativeWorldLayoutPlanProjection invalidLayout =
      project(fixture, 0U);

  ProjectionFixture contourFixture = makeFixture();
  contourFixture.contours[0].start.x =
      std::numeric_limits<double>::infinity();
  const cr::CreativeWorldLayoutPlanProjection invalidContour =
      project(contourFixture, 0U);

  return expect(!absent.accepted &&
                    absent.status ==
                        cr::CreativeWorldLayoutPlanProjectionStatus::
                            NotRequested &&
                    absent.primitives.empty(),
                "null layout remains not requested") &&
         expect(!invalidCut.accepted &&
                    invalidCut.status ==
                        cr::CreativeWorldLayoutPlanProjectionStatus::
                            InvalidRequest &&
                    invalidCut.primitives.empty(),
                "non-finite cut plane fails atomically") &&
         expect(!invalidLevel.accepted &&
                    invalidLevel.status ==
                        cr::CreativeWorldLayoutPlanProjectionStatus::
                            InvalidActiveLevel &&
                    invalidLevel.primitives.empty(),
                "invalid active level fails atomically") &&
         expect(!invalidLayout.accepted &&
                    invalidLayout.status ==
                        cr::CreativeWorldLayoutPlanProjectionStatus::
                            InvalidLayout &&
                    invalidLayout.primitives.empty(),
                "invalid opening geometry leaves no partial plan") &&
         expect(!invalidContour.accepted &&
                    invalidContour.status ==
                        cr::CreativeWorldLayoutPlanProjectionStatus::
                            InvalidLayout &&
                    invalidContour.primitives.empty(),
                "invalid contour geometry leaves no partial plan");
}

bool orthogonalRoomProjectionPreservesItsConcaveBoundary() {
  ProjectionFixture fixture;
  fixture.grid.cellSizeMeters = 1.0;
  fixture.layout.stableKey = "orthogonal_projection";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {6, 6}};
  fixture.layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  fixture.layout.levels.push_back(level);
  fixture.layout.rooms.push_back(
      room(0U, 0U, "orthogonal_room", {{0, 0}, {6, 6}}));
  appendExplicitLRoomTopology(fixture.layout);

  const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 0U);
  double projectedFloorArea = 0.0;
  bool notchCovered = false;
  for (const cr::CreativeWorldLayoutPlanPrimitive& primitive :
       plan.primitives) {
    if (primitive.role != cr::CreativeWorldLayoutPlanRole::RoomFloor ||
        primitive.layer != cr::CreativeWorldLayoutPlanLayer::Active ||
        primitive.kind != cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon ||
        primitive.pointCount != 4U) {
      continue;
    }
    double minimumX = primitive.points[0].x;
    double maximumX = primitive.points[0].x;
    double minimumZ = primitive.points[0].z;
    double maximumZ = primitive.points[0].z;
    for (std::size_t pointIndex = 1U; pointIndex < primitive.pointCount;
         ++pointIndex) {
      minimumX = std::min(minimumX, primitive.points[pointIndex].x);
      maximumX = std::max(maximumX, primitive.points[pointIndex].x);
      minimumZ = std::min(minimumZ, primitive.points[pointIndex].z);
      maximumZ = std::max(maximumZ, primitive.points[pointIndex].z);
    }
    projectedFloorArea += (maximumX - minimumX) * (maximumZ - minimumZ);
    notchCovered = notchCovered ||
                   (5.0 > minimumX && 5.0 < maximumX &&
                    5.0 > minimumZ && 5.0 < maximumZ);
  }

  return expect(plan.accepted,
                "orthogonal room projects through the canonical graph") &&
         expect(plan.receipt.roomPrimitiveCount == 2U &&
                    near(projectedFloorArea, 20.0) && !notchCovered,
                "floor projection decomposes the L without filling its notch") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::ExteriorWall,
                          cr::CreativeWorldLayoutPlanLayer::Active) == 6U,
                "the six canonical L-room edges remain exterior walls") &&
         expect(countRole(plan, cr::CreativeWorldLayoutPlanRole::RoofOutline,
                          cr::CreativeWorldLayoutPlanLayer::Overhead) == 6U,
                "flat irregular roof follows the six exterior edges");
}

bool splitCollinearWallsKeepExactPlanSources() {
  ProjectionFixture fixture;
  fixture.grid.cellSizeMeters = 1.0;
  fixture.layout.stableKey = "split_wall_projection";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {8, 4}};
  fixture.layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  fixture.layout.levels.push_back(level);
  fixture.layout.rooms.push_back(
      room(0U, 0U, "room", {{0, 0}, {8, 4}}));
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(fixture.layout);
  if (!materialized.accepted) {
    return expect(false, "split wall projection fixture materializes");
  }
  std::size_t northEdge = cr::kInvalidCreativeWorldLayoutIndex;
  for (std::size_t edgeIndex = 0U;
       edgeIndex < materialized.edited.topologyEdges.size(); ++edgeIndex) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        materialized.edited.topologyEdges[edgeIndex];
    const cr::CreativeTerrainCoord2 start =
        materialized.edited.topologyVertices[edge.startVertexIndex].position;
    const cr::CreativeTerrainCoord2 end =
        materialized.edited.topologyVertices[edge.endVertexIndex].position;
    if (start.z == 0 && end.z == 0) {
      northEdge = edgeIndex;
      break;
    }
  }
  const cr::CreativeWorldLayoutWallOperationResult split =
      cr::splitCreativeWorldLayoutWall(
          materialized.edited,
          {northEdge, 3U, "north_split_vertex", "north_split_wall"});
  if (!split.accepted) {
    return expect(false, "canonical north wall splits for projection");
  }
  fixture.layout = split.edited;
  const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 0U);
  const auto* first = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::ExteriorWall,
      cr::CreativeWorldLayoutPlanLayer::Active,
      cr::CreativeWorldLayoutTable::TopologyEdge, northEdge);
  const auto* second = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::ExteriorWall,
      cr::CreativeWorldLayoutPlanLayer::Active,
      cr::CreativeWorldLayoutTable::TopologyEdge,
      split.newTopologyEdgeIndex);
  return expect(plan.accepted && first != nullptr && second != nullptr,
                "each split wall has one selectable plan primitive") &&
         expect(first->pointCount == 2U && second->pointCount == 2U &&
                    near(first->points[0].x, 0.0) &&
                    near(first->points[1].x, 3.0) &&
                    near(second->points[0].x, 3.0) &&
                    near(second->points[1].x, 8.0),
                "plan wall geometry stops at exact canonical split bounds");
}

bool roofAperturesProjectExactAuthoredSymbols() {
  ProjectionFixture fixture = makeFixture();
  cr::CreativeWorldLayoutRoofAperture skylight;
  skylight.levelIndex = 1U;
  skylight.kind = cr::CreativeStructuralRoofApertureKind::Skylight;
  skylight.stableKey = "upper_skylight";
  skylight.name = "Upper Skylight";
  skylight.minimumXCells = 1.0;
  skylight.maximumXCells = 2.0;
  skylight.minimumZCells = 0.5;
  skylight.maximumZCells = 1.2;
  fixture.layout.roofApertures.push_back(skylight);
  cr::CreativeWorldLayoutRoofAperture clearance = skylight;
  clearance.kind =
      cr::CreativeStructuralRoofApertureKind::ChimneyClearance;
  clearance.stableKey = "upper_chimney_clearance";
  clearance.name = "Upper Chimney Clearance";
  clearance.minimumXCells = 4.0;
  clearance.maximumXCells = 5.0;
  fixture.layout.roofApertures.push_back(clearance);

  const cr::CreativeWorldLayoutPlanProjection plan = project(fixture, 1U);
  const auto* skylightPrimitive = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::RoofSkylight,
      cr::CreativeWorldLayoutPlanLayer::Overhead,
      cr::CreativeWorldLayoutTable::RoofAperture, 0U);
  const auto* clearancePrimitive = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::RoofClearance,
      cr::CreativeWorldLayoutPlanLayer::Overhead,
      cr::CreativeWorldLayoutTable::RoofAperture, 1U);
  const auto exactBounds = [](const cr::CreativeWorldLayoutPlanPrimitive* value,
                              double minimumX, double maximumX) {
    return value != nullptr && value->pointCount == 4U &&
           near(value->points[0].x, minimumX) &&
           near(value->points[0].z, 0.5) &&
           near(value->points[1].x, maximumX) &&
           near(value->points[1].z, 0.5) &&
           near(value->points[2].x, maximumX) &&
           near(value->points[2].z, 1.2) &&
           near(value->points[3].x, minimumX) &&
           near(value->points[3].z, 1.2);
  };
  return expect(plan.accepted &&
                    plan.receipt.roofAperturePrimitiveCount == 2U,
                "roof apertures have a dedicated projection receipt") &&
         expect(exactBounds(skylightPrimitive, 1.0, 2.0) &&
                    exactBounds(clearancePrimitive, 4.0, 5.0),
                "roof aperture symbols preserve exact authored plan bounds");
}

bool boundedLandformsProjectExactRectangles() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "landform_projection";
  cr::CreativeWorldLayoutTerrainProfile cliff;
  cliff.stableKey = "cliff.west";
  cliff.kind = cr::CreativeTerrainRecipeKind::Cliff;
  cliff.usesLandformRecipe = true;
  cliff.landform.kind = cr::CreativeTerrainLandformKind::Cliff;
  cliff.landform.bounds = {{-6, 3}, 10U, 5U};
  cliff.landform.baseHeightCells = 2U;
  cliff.landform.targetHeightCells = 9U;
  cliff.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  cliff.landform.edgeWidthCells = 0U;
  layout.terrainProfiles.push_back(cliff);
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 1.0;
  cr::CreativeWorldLayoutPlanProjectionRequest request;
  request.layout = &layout;
  request.grid = grid;
  const cr::CreativeWorldLayoutPlanProjection plan =
      cr::projectCreativeWorldLayoutPlan(request);
  const auto* primitive = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::TerrainProfile,
      cr::CreativeWorldLayoutPlanLayer::Active,
      cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  return expect(plan.accepted && primitive != nullptr &&
                    primitive->kind ==
                        cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon &&
                    primitive->terrainKind ==
                        cr::CreativeTerrainRecipeKind::Cliff &&
                    primitive->pointCount == 4U &&
                    primitive->points[0] ==
                        cr::CreativeWorldLayoutPlanPoint{-6.0, 3.0} &&
                    primitive->points[1] ==
                        cr::CreativeWorldLayoutPlanPoint{4.0, 3.0} &&
                    primitive->points[2] ==
                        cr::CreativeWorldLayoutPlanPoint{4.0, 8.0} &&
                    primitive->points[3] ==
                        cr::CreativeWorldLayoutPlanPoint{-6.0, 8.0},
                "bounded landform projection uses exact authored tile edges");
}

bool retainingTransitionsProjectOnExactSharedBoundaries() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "retaining_transition_projection";
  cr::CreativeWorldLayoutTerrainProfile terrace;
  terrace.stableKey = "terrace.central";
  terrace.kind = cr::CreativeTerrainRecipeKind::Terrace;
  terrace.usesLandformRecipe = true;
  terrace.landform.kind = cr::CreativeTerrainLandformKind::Terrace;
  terrace.landform.bounds = {{-6, 3}, 10U, 6U};
  terrace.landform.baseHeightCells = 2U;
  terrace.landform.targetHeightCells = 6U;
  terrace.landform.terraceCount = 2U;
  terrace.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  terrace.landform.edgeWidthCells = 0U;
  terrace.usesRetainingEdgeRecipe = true;
  terrace.retainingEdge.terrainProfileKey = terrace.stableKey;
  terrace.retainingEdge.settings.transitionCount = 2U;
  terrace.retainingEdge.settings.transitions[0] = {
      cr::canonicalCreativeTerrainHardEdge({-3, 4}, {-2, 4}),
      cr::CreativeRetainingEdgeTransitionKind::Stair,
      3U,
  };
  terrace.retainingEdge.settings.transitions[1] = {
      cr::canonicalCreativeTerrainHardEdge({1, 5}, {1, 6}),
      cr::CreativeRetainingEdgeTransitionKind::Ramp,
      5U,
  };
  layout.terrainProfiles.push_back(terrace);

  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 1.0;
  cr::CreativeWorldLayoutPlanProjectionRequest request;
  request.layout = &layout;
  request.grid = grid;
  const cr::CreativeWorldLayoutPlanProjection plan =
      cr::projectCreativeWorldLayoutPlan(request);
  const auto* stair = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::Stair,
      cr::CreativeWorldLayoutPlanLayer::Active,
      cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  const auto* ramp = findSource(
      plan, cr::CreativeWorldLayoutPlanRole::Ramp,
      cr::CreativeWorldLayoutPlanLayer::Active,
      cr::CreativeWorldLayoutTable::TerrainProfile, 0U);

  cr::CreativeWorldLayout invalid = layout;
  invalid.terrainProfiles[0].retainingEdge.terrainProfileKey =
      "terrace.other";
  cr::CreativeWorldLayoutPlanProjectionRequest invalidRequest = request;
  invalidRequest.layout = &invalid;
  const cr::CreativeWorldLayoutPlanProjection rejected =
      cr::projectCreativeWorldLayoutPlan(invalidRequest);

  return expect(plan.accepted &&
                    plan.receipt.terrainPrimitiveCount == 3U &&
                    stair != nullptr && ramp != nullptr &&
                    stair->kind ==
                        cr::CreativeWorldLayoutPlanPrimitiveKind::Segment &&
                    stair->points[0] ==
                        cr::CreativeWorldLayoutPlanPoint{-2.5, 3.5} &&
                    stair->points[1] ==
                        cr::CreativeWorldLayoutPlanPoint{-2.5, 4.5} &&
                    ramp->kind ==
                        cr::CreativeWorldLayoutPlanPrimitiveKind::Segment &&
                    ramp->points[0] ==
                        cr::CreativeWorldLayoutPlanPoint{0.5, 5.5} &&
                    ramp->points[1] ==
                        cr::CreativeWorldLayoutPlanPoint{1.5, 5.5},
                "retaining stair and ramp mark exact shared cell boundaries") &&
         expect(!rejected.accepted && rejected.primitives.empty(),
                "invalid retaining projection fails atomically");
}

bool projectionIsDeterministic() {
  const ProjectionFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutPlanProjection first = project(fixture, 0U);
  const cr::CreativeWorldLayoutPlanProjection second = project(fixture, 0U);
  return expect(first.accepted && second.accepted &&
                    first.activeFloorTopLayer == second.activeFloorTopLayer &&
                    first.bounds == second.bounds &&
                    first.receipt == second.receipt &&
                    first.primitives == second.primitives,
                "identical source produces byte-comparable ordered geometry");
}

}  // namespace

int main() {
  const bool ok = vocabularyIsClosedAndNamed() &&
                  groundDatumProjectsBothBuildingsAndCutOpenings() &&
                  everyDoorSettingProjectsTheAuthoredHingeAndSwing() &&
                  openingFacingProjectsAnExplicitFrontSide() &&
                  upperStoreyAddsContextRoofAndVerticalFiltering() &&
                  everySimpleRoofStyleProjectsItsAuthoredPlanSemantics() &&
                  groundStoreyCanShowUpperContextIndependently() &&
                  displayFlagsOnlyRemoveTheirPresentationLayers() &&
                  levelNavigationUsesPhysicalDatums() &&
                  semanticSourcesResolveToExactStoreys() &&
                  terrainContoursAndObjectsCarrySemanticMetadata() &&
                  rampOmitsStairTreadsAndTerrainOnlyPlansRemainValid() &&
                  invalidInputFailsAtomically() &&
                  orthogonalRoomProjectionPreservesItsConcaveBoundary() &&
                  splitCollinearWallsKeepExactPlanSources() &&
                  roofAperturesProjectExactAuthoredSymbols() &&
                  boundedLandformsProjectExactRectangles() &&
                  retainingTransitionsProjectOnExactSharedBoundaries() &&
                  projectionIsDeterministic();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_world_layout_plan_projection_tests: PASS\n";
  return EXIT_SUCCESS;
}
