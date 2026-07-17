#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

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
                "authored local bounds preserve world center and swapped axes");
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

  return expect(plan.accepted &&
                    plan.objectKind == cr::CreativeObjectKind::Ramp,
                "ramp uses the vertical connector planner") &&
         expect(plan.riseMeters == 3.0 && plan.runMeters == 4.0 &&
                    plan.widthMeters == 2.0 && plan.stepCount == 0U,
                "ramp shares slope dimensions without publishing treads") &&
         expect(
             std::abs(plan.rotationEulerRadians.y - std::numbers::pi) < 1.0e-12,
             "negative Z ramp points from its low end toward its high end") &&
         expect(
             ramp != nullptr && ramp->stableKey == "house.main_ramp" &&
                 ramp->createRequest.hasTransformOverride &&
                 std::abs(ramp->createRequest.transform.rotationEulerRadians.y -
                          std::numbers::pi) < 1.0e-12,
             "compiler emits one directionally authored ramp");
}

bool invalidStoriesFootprintsAndLandingsFailClosed() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout wrongRise = twoStoreyLayout();
  wrongRise.levels[1].floorTopLayer = 2.5;
  cr::CreativeWorldLayout outside = twoStoreyLayout();
  outside.verticalConnectors[0].footprint = {{-1, 2}, {5, 4}};
  cr::CreativeWorldLayout noLanding = twoStoreyLayout();
  noLanding.verticalConnectors[0].footprint = {{0, 2}, {4, 4}};
  cr::CreativeWorldLayout steep = twoStoreyLayout();
  steep.verticalConnectors[0].footprint = {{1, 2}, {3, 4}};

  return expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, wrongRise, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLevels,
             "non-adjacent story elevations reject") &&
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
             "run shorter than rise rejects");
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
       connectorSurface->normal.x < 0.0F && connectorSurface->normal.y > 0.0F);
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
                "connector render and collision profiles stay in parity");
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

} // namespace

int main() {
  return stairPlanOwnsRiseDirectionAndStepParity() &&
                 rampPlanUsesTheSharedSlopeAndCompilerPath() &&
                 invalidStoriesFootprintsAndLandingsFailClosed() &&
                 stagedConnectorPlanningDoesNotMutateTheLayout() &&
                 oneConnectorOwnsEachAffectedSlab() &&
                 compilerCutsBothSlabsAndEmitsOneStair() &&
                 worldLayoutConnectorsRenderAsStairsAndRamps()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
