#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
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

bool near(double lhs, double rhs, double tolerance = 1.0e-5) {
  return std::abs(lhs - rhs) <= tolerance;
}

struct CpuGeometryBounds {
  std::array<float, 3U> minimum{};
  std::array<float, 3U> maximum{};
  bool valid = false;
};

CpuGeometryBounds
measureCpuGeometry(const iggy3d::vulkan::RoomMeshCpuGeometry& geometry) {
  CpuGeometryBounds result;
  result.minimum.fill(std::numeric_limits<float>::infinity());
  result.maximum.fill(-std::numeric_limits<float>::infinity());
  for (const iggy3d::vulkan::FirstRoomVertex& vertex : geometry.vertices) {
    for (std::size_t axis = 0U; axis < result.minimum.size(); ++axis) {
      result.minimum[axis] =
          std::min(result.minimum[axis], vertex.position[axis]);
      result.maximum[axis] =
          std::max(result.maximum[axis], vertex.position[axis]);
    }
  }
  result.valid = !geometry.vertices.empty();
  return result;
}

cr::CreativeWorldLayout twoStoreyLayout() {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  layout.buildings.push_back(building);
  layout.levels.push_back({0U, "ground", "Ground", 0.0, 3U, 1U, 1U, 1U});
  layout.levels.push_back({0U, "upper", "Upper", 3.0, 3U, 1U, 1U, 1U});
  layout.rooms.push_back(
      {0U, 0U, "ground_room", "Ground Room", {{0, 0}, {8, 8}}, 0.25});
  layout.rooms.push_back(
      {0U, 1U, "upper_room", "Upper Room", {{0, 0}, {8, 8}}, 0.25});
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "main_stair",
       "Main Stair",
       {{1, 2}, {5, 4}}});
  return layout;
}

void replaceRoomsWithExplicitLTopology(cr::CreativeWorldLayout& layout) {
  const cr::CreativeTerrainCoord2 points[] = {
      {0, 0}, {8, 0}, {8, 4}, {4, 4}, {4, 8}, {0, 8},
  };
  const std::pair<std::size_t, std::size_t> edges[] = {
      {0U, 1U}, {1U, 2U}, {3U, 2U},
      {3U, 4U}, {5U, 4U}, {0U, 5U},
  };
  const bool reversed[] = {false, false, true, false, true, true};
  for (std::size_t levelIndex = 0U; levelIndex < 2U; ++levelIndex) {
    const std::size_t vertexBase = layout.topologyVertices.size();
    const std::size_t edgeBase = layout.topologyEdges.size();
    for (std::size_t pointIndex = 0U; pointIndex < std::size(points);
         ++pointIndex) {
      layout.topologyVertices.push_back(
          {levelIndex,
           "l" + std::to_string(levelIndex) + "_vertex_" +
               std::to_string(pointIndex),
           points[pointIndex]});
    }
    for (std::size_t edgeIndex = 0U; edgeIndex < std::size(edges);
         ++edgeIndex) {
      layout.topologyEdges.push_back(
          {levelIndex,
           "l" + std::to_string(levelIndex) + "_edge_" +
               std::to_string(edgeIndex),
           vertexBase + edges[edgeIndex].first,
           vertexBase + edges[edgeIndex].second, 0.25});
      layout.roomBoundaries.push_back(
          {levelIndex, edgeBase + edgeIndex, edgeIndex, reversed[edgeIndex]});
    }
  }
}

bool stairPlanOwnsRiseDirectionAndStepParity() {
  const cr::CreativeGridSettings grid{{10.0, 1.0, -5.0}, 1.0, {32, 16, 32}};
  const cr::CreativeWorldLayout layout = twoStoreyLayout();
  const auto plan =
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U);
  return expect(plan.accepted &&
                    plan.objectKind == cr::CreativeObjectKind::Stair,
                "valid two-storey stair is accepted") &&
         expect(plan.riseMeters == 3.0 && plan.runMeters == 4.0 &&
                    plan.widthMeters == 2.0 && plan.stepCount == 12U,
                "stair dimensions and descriptor-owned step count match") &&
         expect(plan.stair.accepted &&
                    plan.stair.riserHeightMeters == 0.25 &&
                    std::abs(plan.stair.treadDepthMeters - 1.0 / 3.0) <
                        1.0e-12 &&
                    plan.stair.socketCount == 4U,
                "world connector exposes one canonical stair schedule") &&
         expect(
             std::abs(plan.rotationEulerRadians.y - std::numbers::pi * 0.5) <
                 1.0e-12,
             "positive X stair rotates local positive Z toward positive X") &&
         expect(plan.authoredBounds.min.x == 12.0 &&
                    plan.authoredBounds.max.x == 14.0 &&
                    plan.authoredBounds.min.z == -4.0 &&
                    plan.authoredBounds.max.z == 0.0 &&
                    plan.authoredBounds.min.y == 1.0 &&
                    plan.authoredBounds.max.y == 4.0,
                "authored local bounds preserve world center and swapped axes") &&
         expect(plan.stair.lowerLanding.centerMeters.x == 10.5 &&
                    plan.stair.upperLanding.centerMeters.x == 15.5 &&
                    plan.stair.lowerLanding.centerMeters.y == 1.0 &&
                    plan.stair.upperLanding.centerMeters.y == 4.0,
                "landings follow the selected rise direction and levels");
}

bool rampPlanUsesTheSharedSlopeAndCompilerPath() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout layout = twoStoreyLayout();
  cr::CreativeWorldLayoutVerticalConnector& connector =
      layout.verticalConnectors.front();
  connector.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  connector.direction = cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
  connector.stableKey = "main_ramp";
  connector.name = "Main Ramp";
  connector.footprint = {{2, 1}, {4, 5}};
  connector.material = cr::CreativeStructuralMaterial::Stone;

  const auto plan =
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U);
  cr::CreativeDocument document = cr::CreativeDocument::create("Ramp Compile");
  static_cast<void>(document.assignId(72U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeRecipeObjectPlan* ramp = nullptr;
  if (compiled.receipt.accepted && !compiled.plan.objectRecipes.empty()) {
    const auto& objects = compiled.plan.objectRecipes.front().objects;
    const auto found = std::find_if(
        objects.begin(), objects.end(),
        [](const cr::CreativeRecipeObjectPlan& object) {
          return object.createRequest.kind == cr::CreativeObjectKind::Ramp;
        });
    if (found != objects.end()) {
      ramp = &*found;
    }
  }
  cr::CreativeStructuralMaterial compiledMaterial =
      cr::CreativeStructuralMaterial::Count;
  const bool materialTagged =
      ramp != nullptr &&
      cr::parseCreativeStructuralMaterialTag(ramp->createRequest.tags,
                                             compiledMaterial);

  return expect(plan.accepted &&
                    plan.objectKind == cr::CreativeObjectKind::Ramp,
                "ramp uses the vertical connector planner") &&
         expect(plan.riseMeters == 3.0 && plan.runMeters == 4.0 &&
                    plan.widthMeters == 2.0 && plan.stepCount == 0U,
                "ramp shares slope dimensions without publishing treads") &&
         expect(plan.ramp.accepted && plan.ramp.walkable &&
                    near(plan.ramp.slopeAngleDegrees, 36.8698976458) &&
                    plan.ramp.maximumWalkableSlopeDegrees == 40.0 &&
                    plan.ramp.socketCount == 2U &&
                    plan.ramp.material == cr::CreativeStructuralMaterial::Stone,
                "ramp recipe owns walkability sockets and material") &&
         expect(plan.ramp.sideEdges[0].lowMeters.y == 0.0 &&
                    plan.ramp.sideEdges[0].highMeters.y == 3.0 &&
                    plan.ramp.lowerLanding.centerMeters.z == 5.5 &&
                    plan.ramp.upperLanding.centerMeters.z == 0.5,
                "ramp publishes explicit side edges and landings") &&
         expect(
             std::abs(plan.rotationEulerRadians.y - std::numbers::pi) < 1.0e-12,
             "negative Z ramp points from its low end toward its high end") &&
         expect(
             ramp != nullptr && ramp->stableKey == "house.main_ramp" &&
                 ramp->createRequest.hasTransformOverride &&
                 materialTagged &&
                 compiledMaterial == cr::CreativeStructuralMaterial::Stone &&
                 std::abs(ramp->createRequest.transform.rotationEulerRadians.y -
                          std::numbers::pi) < 1.0e-12,
             "compiler emits one directionally authored ramp");
}

bool invalidStoriesFootprintsAndLandingsFailClosed() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout independentWallHeight = twoStoreyLayout();
  independentWallHeight.levels[0].wallHeightCells = 2U;
  cr::CreativeWorldLayout nonAdjacent = twoStoreyLayout();
  cr::CreativeWorldLayoutLevel middleLevel = nonAdjacent.levels[0];
  middleLevel.stableKey = "middle";
  middleLevel.name = "Middle";
  middleLevel.floorTopLayer = 1.5;
  nonAdjacent.levels.push_back(middleLevel);
  cr::CreativeWorldLayoutRoom middleRoom = nonAdjacent.rooms[0];
  middleRoom.levelIndex = 2U;
  middleRoom.stableKey = "middle_room";
  middleRoom.name = "Middle Room";
  nonAdjacent.rooms.push_back(middleRoom);
  cr::CreativeWorldLayout outside = twoStoreyLayout();
  outside.verticalConnectors[0].footprint = {{-1, 2}, {5, 4}};
  cr::CreativeWorldLayout noLanding = twoStoreyLayout();
  noLanding.verticalConnectors[0].footprint = {{0, 2}, {4, 4}};
  cr::CreativeWorldLayout steep = twoStoreyLayout();
  steep.verticalConnectors[0].footprint = {{1, 2}, {3, 4}};
  cr::CreativeWorldLayout rampAtFortyFive = twoStoreyLayout();
  rampAtFortyFive.verticalConnectors[0].kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  rampAtFortyFive.verticalConnectors[0].footprint = {{1, 2}, {4, 4}};
  cr::CreativeWorldLayout lowHeadroom = twoStoreyLayout();
  lowHeadroom.levels[1].wallHeightCells = 1U;

  const cr::CreativeWorldLayoutVerticalConnectorPlan independentPlan =
      cr::planCreativeWorldLayoutVerticalConnector(grid,
                                                   independentWallHeight, 0U);
  return expect(independentPlan.accepted &&
                    near(independentPlan.riseMeters, 3.0),
                "connector rise follows floor datums instead of wall height") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, nonAdjacent,
                                                          0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLevels,
             "connector cannot skip the nearest occupied storey") &&
         expect(cr::planCreativeWorldLayoutVerticalConnector(grid, outside, 0U)
                        .status ==
                    cr::CreativeWorldLayoutVerticalConnectorStatus::
                        InvalidFootprint,
                "opening outside either room rejects") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, noLanding, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLanding,
             "missing low landing rejects") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, steep, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidSlope,
             "run shorter than rise rejects") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(
                 grid, rampAtFortyFive, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidSlope,
             "45 degree ramp rejects against the runtime 40 degree limit") &&
         expect(cr::planCreativeWorldLayoutVerticalConnector(
                    grid, lowHeadroom, 0U)
                    .status ==
                    cr::CreativeWorldLayoutVerticalConnectorStatus::
                        InvalidHeadroom,
                "upper storey without player headroom rejects");
}

bool orthogonalRoomsRejectNotchFootprintsAndMissingLandings() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout valid = twoStoreyLayout();
  replaceRoomsWithExplicitLTopology(valid);
  valid.verticalConnectors[0].direction =
      cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
  valid.verticalConnectors[0].footprint = {{1, 1}, {3, 5}};

  cr::CreativeWorldLayout notch = valid;
  notch.verticalConnectors[0].direction =
      cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  notch.verticalConnectors[0].footprint = {{5, 5}, {7, 7}};

  cr::CreativeWorldLayout missingLanding = valid;
  missingLanding.verticalConnectors[0].direction =
      cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  missingLanding.verticalConnectors[0].footprint = {{2, 5}, {4, 7}};

  return expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, valid, 0U)
                 .accepted,
             "connector and both landings fit the covered L-room union") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, notch, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::
                     InvalidFootprint,
             "connector inside the L-room bounding-box notch rejects") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(
                 grid, missingLanding, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLanding,
             "landing in the L-room notch rejects after a covered footprint");
}

bool stagedConnectorPlanningDoesNotMutateTheLayout() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  const cr::CreativeWorldLayout layout = twoStoreyLayout();
  cr::CreativeWorldLayoutVerticalConnector candidate =
      layout.verticalConnectors.front();
  candidate.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  candidate.footprint = {{2, 2}, {6, 4}};
  const auto staged = cr::planCreativeWorldLayoutVerticalConnector(
      grid, layout, 0U, candidate);
  candidate.direction = cr::CreativeWorldLayoutVerticalDirection::Count;
  const auto rejected = cr::planCreativeWorldLayoutVerticalConnector(
      grid, layout, 0U, candidate);
  return expect(staged.accepted &&
                    staged.objectKind == cr::CreativeObjectKind::Ramp &&
                    staged.openingFootprint.minimum ==
                        cr::CreativeTerrainCoord2{2, 2},
                "staged connector validates through the canonical planner") &&
         expect(rejected.status ==
                    cr::CreativeWorldLayoutVerticalConnectorStatus::
                        InvalidConnector,
                "invalid staged connector rejects through the same planner") &&
         expect(layout.verticalConnectors[0].kind ==
                        cr::CreativeWorldLayoutVerticalConnectorKind::Stair &&
                    layout.verticalConnectors[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{1, 2},
                "staged planning never mutates the live layout");
}

bool oneConnectorOwnsEachAffectedSlab() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout layout = twoStoreyLayout();
  layout.verticalConnectors.push_back(layout.verticalConnectors.front());
  layout.verticalConnectors.back().stableKey = "second_stair";
  layout.verticalConnectors.back().footprint = {{2, 5}, {6, 7}};
  return expect(
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U).status ==
          cr::CreativeWorldLayoutVerticalConnectorStatus::SurfaceAlreadyCut,
      "a room surface cannot publish two competing cutout owners");
}

bool compilerCutsBothSlabsAndEmitsOneStair() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Vertical Connector Compile");
  static_cast<void>(document.assignId(71U));
  static_cast<void>(document.setGridSettings({{}, 1.0, {32, 16, 32}}));
  const cr::CreativeWorldLayout layout = twoStoreyLayout();
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  if (!compiled.receipt.accepted || compiled.plan.objectRecipes.size() != 1U) {
    return expect(false, "connector layout compiles as one building recipe");
  }

  const auto& objects = compiled.plan.objectRecipes.front().objects;
  const auto stair = std::find_if(
      objects.begin(), objects.end(),
      [](const cr::CreativeRecipeObjectPlan& value) {
        return value.createRequest.kind == cr::CreativeObjectKind::Stair;
      });
  const std::size_t lowerCeilingParts = static_cast<std::size_t>(std::count_if(
      objects.begin(), objects.end(),
      [](const cr::CreativeRecipeObjectPlan& value) {
        return value.stableKey.find("ground_room.ceiling.part.") !=
               std::string::npos;
      }));
  const std::size_t upperFloorParts = static_cast<std::size_t>(
      std::count_if(objects.begin(), objects.end(),
                    [](const cr::CreativeRecipeObjectPlan& value) {
                      return value.stableKey.find("upper_room.floor.part.") !=
                             std::string::npos;
                    }));

  return expect(stair != objects.end(), "compiler emits the semantic stair") &&
         expect(stair->stableKey == "house.main_stair" &&
                    stair->createRequest.hasTransformOverride &&
                    std::abs(
                        stair->createRequest.transform.rotationEulerRadians.y -
                        std::numbers::pi * 0.5) < 1.0e-12,
                "compiled stair keeps stable identity and direction") &&
         expect(lowerCeilingParts == 4U && upperFloorParts == 4U,
                "one opening partitions both affected structural slabs") &&
         expect(std::none_of(objects.begin(), objects.end(),
                             [](const cr::CreativeRecipeObjectPlan& value) {
                               return value.stableKey ==
                                          "house.ground_room.ceiling" ||
                                      value.stableKey ==
                                          "house.upper_room.floor";
                             }),
                "uncut full slabs are not emitted beneath cutout pieces");
}

bool connectorProducesSpecificRenderedGeometryAndCollision(
    cr::CreativeWorldLayoutVerticalConnectorKind connectorKind,
    cr::CreativeObjectKind objectKind, std::string_view expectedMeshId,
    std::size_t expectedVertexCount, std::size_t expectedIndexCount,
    std::size_t expectedDrawCount, std::size_t expectedSurfaceCount) {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout layout = twoStoreyLayout();
  cr::CreativeWorldLayoutVerticalConnector& connector =
      layout.verticalConnectors.front();
  connector.kind = connectorKind;
  connector.direction = cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  connector.stableKey = objectKind == cr::CreativeObjectKind::Stair
                            ? "rendered_stair"
                            : "rendered_ramp";
  connector.name = objectKind == cr::CreativeObjectKind::Stair
                       ? "Rendered Stair"
                       : "Rendered Ramp";
  if (objectKind == cr::CreativeObjectKind::Ramp) {
    connector.material = cr::CreativeStructuralMaterial::Stone;
  }

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Connector Render Contract");
  static_cast<void>(document.assignId(
      objectKind == cr::CreativeObjectKind::Stair ? 73U : 74U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutVerticalConnectorPlan connectorPlan =
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      compiled.receipt.accepted
          ? cr::previewCreativeWorldLayoutPlan(document, compiled.plan)
          : cr::CreativeWorldLayoutPreviewResult{};

  const cr::CreativeObject* object = nullptr;
  if (preview.accepted) {
    const auto found = std::find_if(
        preview.document.objects().begin(), preview.document.objects().end(),
        [objectKind](const cr::CreativeObject& candidate) {
          return candidate.kind == objectKind;
        });
    if (found != preview.document.objects().end()) {
      object = &*found;
    }
  }

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = preview.accepted ? &preview.document : nullptr;
  bakeRequest.roomId = "connector_render_contract";
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);

  const iggy3d::RoomStaticMeshAsset* bakedMesh = nullptr;
  const iggy3d::SceneRoomMeshItem* projectedMesh = nullptr;
  iggy3d::SceneProjectionResult projected;
  if (object != nullptr && baked.receipt.accepted) {
    const auto source = std::find_if(
        baked.staticMeshSources.begin(), baked.staticMeshSources.end(),
        [object](const cr::CreativeRoomBakeStaticMeshSource& candidate) {
          return candidate.objectId == object->id;
        });
    if (source != baked.staticMeshSources.end()) {
      const auto mesh = std::find_if(
          baked.room.staticMeshes.begin(), baked.room.staticMeshes.end(),
          [&source](const iggy3d::RoomStaticMeshAsset& candidate) {
            return candidate.id == source->staticMeshId;
          });
      if (mesh != baked.room.staticMeshes.end()) {
        bakedMesh = &*mesh;
      }
    }

    projected = iggy3d::buildSceneProjection({}, &baked.room);
    if (bakedMesh != nullptr) {
      const auto mesh = std::find_if(
          projected.room.meshes.begin(), projected.room.meshes.end(),
          [bakedMesh](const iggy3d::SceneRoomMeshItem& candidate) {
            return candidate.id == bakedMesh->id;
          });
      if (mesh != projected.room.meshes.end()) {
        projectedMesh = &*mesh;
      }
    }
  }

  iggy3d::SceneRoomProjection isolated;
  isolated.loaded = projectedMesh != nullptr;
  isolated.assetId = "isolated_connector";
  if (projectedMesh != nullptr) {
    isolated.meshes.push_back(*projectedMesh);
  }
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(isolated);
  const CpuGeometryBounds geometryBounds = measureCpuGeometry(geometry);
  const cr::CreativeTransformedBounds objectBounds =
      object == nullptr ? cr::CreativeTransformedBounds{}
                        : cr::resolveCreativeObjectBounds(*object);
  const std::size_t surfaceCount =
      object == nullptr
          ? 0U
          : static_cast<std::size_t>(std::count_if(
                baked.spatialSurfaceSources.begin(),
                baked.spatialSurfaceSources.end(),
                [object](
                    const cr::CreativeRoomBakeSpatialSurfaceSource& source) {
                  return source.objectId == object->id;
                }));
  const iggy3d::RoomSpatialSurface* connectorSurface = nullptr;
  if (surfaceCount > 0U && object != nullptr) {
    const auto source = std::find_if(
        baked.spatialSurfaceSources.begin(), baked.spatialSurfaceSources.end(),
        [object](const cr::CreativeRoomBakeSpatialSurfaceSource& candidate) {
          return candidate.objectId == object->id;
        });
    if (source != baked.spatialSurfaceSources.end()) {
      const auto surface = std::find_if(
          baked.room.spatialSurfaces.begin(), baked.room.spatialSurfaces.end(),
          [&source](const iggy3d::RoomSpatialSurface& candidate) {
            return candidate.id == source->surfaceId;
          });
      if (surface != baked.room.spatialSurfaces.end()) {
        connectorSurface = &*surface;
      }
    }
  }

  const bool rampSurfaceCorrect =
      objectKind != cr::CreativeObjectKind::Ramp ||
      (connectorSurface != nullptr &&
       connectorSurface->shape ==
           iggy3d::RoomSpatialSurfaceShape::HeightPatch &&
       connectorSurface->role == iggy3d::RoomSpatialSurfaceRole::Walkable &&
       near(connectorSurface->normal.x,
            connectorPlan.ramp.surfaceNormal.x) &&
       near(connectorSurface->normal.y,
            connectorPlan.ramp.surfaceNormal.y) &&
       near(connectorSurface->normal.z,
            connectorPlan.ramp.surfaceNormal.z));
  const bool stairSurfaceCorrect =
      objectKind != cr::CreativeObjectKind::Stair ||
      (connectorSurface != nullptr &&
       connectorSurface->shape == iggy3d::RoomSpatialSurfaceShape::Box &&
       connectorSurface->role == iggy3d::RoomSpatialSurfaceRole::Blocker);
  const bool geometryShapeCorrect =
      geometry.ready && geometry.vertices.size() == expectedVertexCount &&
      geometry.indices.size() == expectedIndexCount &&
      geometry.indexedDraws.size() == expectedDrawCount;
  if (!geometryShapeCorrect) {
    std::cerr << "connector geometry: ready=" << geometry.ready
              << " vertices=" << geometry.vertices.size()
              << " indices=" << geometry.indices.size()
              << " draws=" << geometry.indexedDraws.size() << '\n';
  }
  const std::size_t rampAnchorCount =
      static_cast<std::size_t>(std::count_if(
          baked.room.anchors.begin(), baked.room.anchors.end(),
          [](const iggy3d::RoomAnchorAsset& anchor) {
            return anchor.kind == "ramp";
          }));
  const iggy3d::ReasoningGraph reasoning =
      iggy3d::buildReasoningGraph(baked.room, {});
  const std::size_t rampReasoningNodeCount =
      static_cast<std::size_t>(std::count_if(
          reasoning.nodes.begin(), reasoning.nodes.end(),
          [](const iggy3d::ReasoningNode& node) {
            return node.kind == iggy3d::ReasoningNodeKind::ramp;
          }));
  const bool rampSemanticsCorrect =
      objectKind != cr::CreativeObjectKind::Ramp ||
      (bakedMesh != nullptr &&
       bakedMesh->materialId == "creative_wall_stone" &&
       rampAnchorCount == 2U && rampReasoningNodeCount == 2U);

  return expect(connectorPlan.accepted && compiled.receipt.accepted &&
                    preview.accepted && object != nullptr,
                "connector recipe reaches a preview document") &&
         expect(baked.receipt.accepted && bakedMesh != nullptr &&
                    bakedMesh->meshId == expectedMeshId &&
                    bakedMesh->proceduralSegmentCount ==
                        connectorPlan.stepCount,
                "room bake retains connector-specific generated geometry") &&
         expect(projected.room.loaded && projectedMesh != nullptr &&
                    projectedMesh->meshId == expectedMeshId &&
                    projectedMesh->proceduralSegmentCount ==
                        connectorPlan.stepCount,
                "scene projection retains connector mesh identity") &&
         expect(geometryShapeCorrect,
                "renderer emits connector-specific CPU geometry") &&
         expect(objectBounds.valid && geometryBounds.valid &&
                    near(geometryBounds.minimum[0],
                         objectBounds.worldBounds.min.x) &&
                    near(geometryBounds.minimum[1],
                         objectBounds.worldBounds.min.y) &&
                    near(geometryBounds.minimum[2],
                         objectBounds.worldBounds.min.z) &&
                    near(geometryBounds.maximum[0],
                         objectBounds.worldBounds.max.x) &&
                    near(geometryBounds.maximum[1],
                         objectBounds.worldBounds.max.y) &&
                    near(geometryBounds.maximum[2],
                         objectBounds.worldBounds.max.z),
                "rotated render geometry matches authored connector bounds") &&
         expect(surfaceCount == expectedSurfaceCount && rampSurfaceCorrect &&
                    stairSurfaceCorrect,
                "connector render and collision profiles stay in parity") &&
         expect(rampSemanticsCorrect,
                "ramp material and navigation semantics reach runtime");
}

bool worldLayoutConnectorsRenderAsStairsAndRamps() {
  return connectorProducesSpecificRenderedGeometryAndCollision(
             cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
             cr::CreativeObjectKind::Stair, "creative_stair_steps", 96U, 864U,
             12U, 36U) &&
         connectorProducesSpecificRenderedGeometryAndCollision(
             cr::CreativeWorldLayoutVerticalConnectorKind::Ramp,
             cr::CreativeObjectKind::Ramp, "creative_ramp_wedge", 6U, 48U, 1U,
             1U);
}

bool authoredStairSupportsFullMotorTraversal() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  const cr::CreativeWorldLayout layout = twoStoreyLayout();
  const cr::CreativeWorldLayoutVerticalConnectorPlan connector =
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U);

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Authored Stair Motor Traversal");
  static_cast<void>(document.assignId(75U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      compiled.receipt.accepted
          ? cr::previewCreativeWorldLayoutPlan(document, compiled.plan)
          : cr::CreativeWorldLayoutPreviewResult{};

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = preview.accepted ? &preview.document : nullptr;
  bakeRequest.roomId = "authored_stair_motor_traversal";
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);

  iggy3d::PlayerPhysicsMovePlannerConfig config;
  config.motor.skinMeters = 0.02F;
  config.motor.groundProbeDistanceMeters = 0.10F;
  config.motor.groundSnapDistanceMeters = 0.10F;
  config.maxStepHeightMeters = 0.35F;
  const cr::CreativeVec3 lower = connector.stair.lowerLanding.centerMeters;
  const cr::CreativeVec3 upper = connector.stair.upperLanding.centerMeters;
  const double horizontalDistance =
      std::hypot(upper.x - lower.x, upper.z - lower.z);
  const double directionX = (upper.x - lower.x) / horizontalDistance;
  const double directionZ = (upper.z - lower.z) / horizontalDistance;
  const std::size_t moveCount = static_cast<std::size_t>(std::llround(
      horizontalDistance / connector.stair.treadDepthMeters));

  iggy3d::Vec3 center{static_cast<float>(lower.x),
                      static_cast<float>(lower.y + 0.90),
                      static_cast<float>(lower.z)};
  std::size_t acceptedStepCount = 0U;
  bool everyMoveAccepted = true;
  for (std::size_t moveIndex = 0U; moveIndex < moveCount; ++moveIndex) {
    iggy3d::PlayerPhysicsMovePlannerRequest request;
    request.collisionSurfaces = &surfaces;
    request.startCenterMeters = center;
    request.bodyHalfExtentsMeters = {0.30F, 0.90F, 0.30F};
    request.desiredDisplacementMeters = {
        static_cast<float>(directionX * connector.stair.treadDepthMeters),
        0.0F,
        static_cast<float>(directionZ * connector.stair.treadDepthMeters)};
    request.config = config;
    const iggy3d::PlayerPhysicsMovePlannerResult planned =
        iggy3d::planPlayerPhysicsMove(request);
    everyMoveAccepted = everyMoveAccepted && planned.ok && planned.grounded &&
                        (!planned.stepAttempted || planned.stepAccepted);
    acceptedStepCount += planned.stepAccepted ? 1U : 0U;
    center = planned.finalCenterMeters;
  }

  return expect(connector.accepted && compiled.receipt.accepted &&
                    preview.accepted && baked.receipt.accepted,
                "authored stair reaches the runtime room") &&
         expect(moveCount == 15U && everyMoveAccepted &&
                    acceptedStepCount == connector.stepCount,
                "motor accepts every authored riser exactly once") &&
         expect(std::fabs(center.x - static_cast<float>(upper.x)) <= 0.01F &&
                    std::fabs(center.z - static_cast<float>(upper.z)) <= 0.01F &&
                    std::fabs(center.y -
                              static_cast<float>(upper.y + 0.90)) <= 0.01F,
                "motor finishes grounded on the authored upper landing");
}

bool authoredRampSupportsFullMotorTraversal() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout layout = twoStoreyLayout();
  layout.verticalConnectors[0].kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  layout.verticalConnectors[0].material =
      cr::CreativeStructuralMaterial::Stone;
  const cr::CreativeWorldLayoutVerticalConnectorPlan connector =
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U);

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Authored Ramp Motor Traversal");
  static_cast<void>(document.assignId(76U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      compiled.receipt.accepted
          ? cr::previewCreativeWorldLayoutPlan(document, compiled.plan)
          : cr::CreativeWorldLayoutPreviewResult{};

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = preview.accepted ? &preview.document : nullptr;
  bakeRequest.roomId = "authored_ramp_motor_traversal";
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);

  const cr::CreativeVec3 lower = connector.ramp.lowerLanding.centerMeters;
  const cr::CreativeVec3 upper = connector.ramp.upperLanding.centerMeters;
  const iggy3d::CollisionQueryResult upperLandingGround =
      iggy3d::sampleSurfaceHeightAtOrBelow(
          surfaces,
          {static_cast<float>(upper.x), static_cast<float>(upper.y),
           static_cast<float>(upper.z)},
          static_cast<float>(upper.y) +
              iggy3d::MovementParams{}.groundSnapMeters,
          iggy3d::MovementParams{}.radiusMeters);
  const double horizontalDistance =
      std::hypot(upper.x - lower.x, upper.z - lower.z);
  const double directionX = (upper.x - lower.x) / horizontalDistance;
  const double directionZ = (upper.z - lower.z) / horizontalDistance;
  constexpr double kMoveMeters = 0.10;
  const std::size_t moveCount = static_cast<std::size_t>(
      std::llround(horizontalDistance / kMoveMeters));

  iggy3d::WorldState world;
  iggy3d::EntityState player;
  player.id = {1U};
  player.stableName = "ramp_player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = iggy3d::identityTransform3();
  player.transform.position = {static_cast<float>(lower.x),
                               static_cast<float>(lower.y),
                               static_cast<float>(lower.z)};
  player.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                         {0.25F, 1.8F, 0.25F});
  static_cast<void>(world.seedEntity(player));
  iggy3d::RuntimeConfig runtimeConfig = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::MovementSystemContext context{&world, &runtimeConfig, &surfaces,
                                        true};
  bool everyMoveAccepted = true;
  for (std::size_t moveIndex = 0U; moveIndex < moveCount; ++moveIndex) {
    const iggy3d::EntityState* current = world.findById({1U});
    if (current == nullptr) {
      everyMoveAccepted = false;
      break;
    }
    iggy3d::MovementRequest request;
    request.actor = {1U};
    request.destination = {
        current->transform.position.x +
            static_cast<float>(directionX * kMoveMeters),
        current->transform.position.y,
        current->transform.position.z +
            static_cast<float>(directionZ * kMoveMeters)};
    request.mode = iggy3d::MovementMode::Walk;
    request.maxDistanceMeters = 1.0F;
    const iggy3d::MovementResult moved =
        iggy3d::executeMovement(context, request);
    everyMoveAccepted =
        everyMoveAccepted &&
        moved.blocked == iggy3d::MovementBlockedReason::None;
  }
  const iggy3d::EntityState* finalPlayer = world.findById({1U});
  const bool reachedUpperLanding =
      finalPlayer != nullptr &&
      std::fabs(finalPlayer->transform.position.x -
                static_cast<float>(upper.x)) <= 0.02F &&
      std::fabs(finalPlayer->transform.position.z -
                static_cast<float>(upper.z)) <= 0.02F &&
      std::fabs(finalPlayer->transform.position.y -
                upperLandingGround.heightMeters) <= 0.03F;
  if (!reachedUpperLanding) {
    if (finalPlayer != nullptr) {
      std::cerr << "ramp movement final foot: "
                << finalPlayer->transform.position.x << ' '
                << finalPlayer->transform.position.y << ' '
                << finalPlayer->transform.position.z << " expected "
                << upper.x << ' ' << upperLandingGround.heightMeters << ' '
                << upper.z << '\n';
    }
  }

  return expect(connector.accepted && connector.ramp.walkable &&
                    compiled.receipt.accepted && preview.accepted &&
                    baked.receipt.accepted,
                "authored ramp reaches the runtime room") &&
         expect(moveCount == 50U && everyMoveAccepted,
                "movement remains grounded across the complete ramp") &&
         expect(upperLandingGround.status ==
                    iggy3d::CollisionQueryStatus::Hit,
                "authored upper landing has exact walkable ground") &&
         expect(reachedUpperLanding,
                "movement finishes grounded on the authored ramp landing");
}

} // namespace

int main() {
  return stairPlanOwnsRiseDirectionAndStepParity() &&
                 rampPlanUsesTheSharedSlopeAndCompilerPath() &&
                 invalidStoriesFootprintsAndLandingsFailClosed() &&
                 orthogonalRoomsRejectNotchFootprintsAndMissingLandings() &&
                 stagedConnectorPlanningDoesNotMutateTheLayout() &&
                 oneConnectorOwnsEachAffectedSlab() &&
                 compilerCutsBothSlabsAndEmitsOneStair() &&
                 worldLayoutConnectorsRenderAsStairsAndRamps() &&
                 authoredStairSupportsFullMotorTraversal() &&
                 authoredRampSupportsFullMotorTraversal()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
