#include "content/assets/StaticMeshAsset.hpp"
#include "content/assets/StaticMeshAuthoringMetadata.hpp"
#include "content/assets/ImageDecode.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

void writeLittleEndianU32(std::ofstream& output, std::uint32_t value) {
  const std::array<char, 4> bytes{
      static_cast<char>(value & 0xFFU),
      static_cast<char>((value >> 8U) & 0xFFU),
      static_cast<char>((value >> 16U) & 0xFFU),
      static_cast<char>((value >> 24U) & 0xFFU),
  };
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool writeJsonOnlyGlb(const std::filesystem::path& path,
                      std::string document) {
  while (document.size() % 4U != 0U) {
    document.push_back(' ');
  }
  if (document.size() >
      std::numeric_limits<std::uint32_t>::max() - 20U) {
    return false;
  }
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    return false;
  }
  constexpr std::uint32_t kGlbMagic = 0x46546C67U;
  constexpr std::uint32_t kGlbVersion = 2U;
  constexpr std::uint32_t kJsonChunkType = 0x4E4F534AU;
  const std::uint32_t jsonLength =
      static_cast<std::uint32_t>(document.size());
  writeLittleEndianU32(output, kGlbMagic);
  writeLittleEndianU32(output, kGlbVersion);
  writeLittleEndianU32(output, 20U + jsonLength);
  writeLittleEndianU32(output, jsonLength);
  writeLittleEndianU32(output, kJsonChunkType);
  output.write(document.data(), static_cast<std::streamsize>(document.size()));
  return output.good();
}

bool importsBoulderFixture() {
  const iggy3d::StaticMeshImportResult result = iggy3d::importStaticMeshGlb(
      "assets/creative/boulder_01.glb", "boulder_01");
  return expect(result.ok(), "boulder imports") &&
         expect(result.reasonCode == "static_mesh_imported",
                "boulder import reason") &&
         expect(result.asset.vertices.size() == 18U, "boulder vertex count") &&
         expect(result.asset.indices.size() == 96U, "boulder index count") &&
         expect(result.asset.primitives.size() == 1U,
                "boulder primitive count") &&
         expect(result.asset.materials.size() == 1U,
                "boulder material count") &&
         expect(result.asset.materials[0].name == "Boulder Granite",
                "boulder material name") &&
         expect(result.asset.materials[0].baseColorFactor[0] > 0.4F &&
                    result.asset.materials[0].baseColorFactor[0] < 0.43F,
                "boulder base color") &&
         expect(result.asset.hasBounds, "boulder bounds") &&
         expect(result.asset.authoringMetadata.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::Authored &&
                    result.asset.authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !result.asset.authoringMetadata.walkable &&
                    result.asset.authoringMetadata.categoryId == "boulder",
                "boulder authoring metadata imports") &&
         expect(result.asset.boundsMin.y < -0.65F &&
                    result.asset.boundsMax.y > 0.71F,
                "boulder vertical extent") &&
         expect(result.asset.contentHash == 0x415827A5D6160646ULL,
                "boulder content hash");
}

bool rejectsUnsafeAndMissingAssetIds() {
  const iggy3d::StaticMeshImportResult unsafe =
      iggy3d::importStaticMeshGlb("assets/creative/boulder_01.glb", "../rock");
  const iggy3d::StaticMeshImportResult missing =
      iggy3d::importStaticMeshGlb("assets/creative/missing.glb", "missing");
  return expect(!unsafe.ok() &&
                    unsafe.status ==
                        iggy3d::StaticMeshImportStatus::InvalidAssetId,
                "unsafe asset id rejected") &&
         expect(!missing.ok() &&
                    missing.status ==
                        iggy3d::StaticMeshImportStatus::FileNotFound,
                "missing asset rejected");
}

iggy3d::SceneRoomProjection roomWith(std::string meshId,
                                     std::string role = "prop") {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.assetId = "static_mesh_test_room";
  iggy3d::SceneRoomMeshItem mesh;
  mesh.id = "rock";
  mesh.meshId = std::move(meshId);
  mesh.role = std::move(role);
  mesh.materialId = "creative_prop";
  mesh.position = {2.0F, 1.5F, -3.0F};
  mesh.size = {3.0F, 2.0F, 4.0F};
  mesh.rotationEulerRadians = {0.0F, 0.5F, 0.0F};
  room.meshes.push_back(std::move(mesh));
  return room;
}

iggy3d::Vec3 transformInstancePoint(
    const iggy3d::vulkan::StaticMeshInstanceTransform& transform,
    iggy3d::Vec3 point) {
  return {
      transform.modelColumn0[0] * point.x +
          transform.modelColumn1[0] * point.y +
          transform.modelColumn2[0] * point.z + transform.modelColumn3[0],
      transform.modelColumn0[1] * point.x +
          transform.modelColumn1[1] * point.y +
          transform.modelColumn2[1] * point.z + transform.modelColumn3[1],
      transform.modelColumn0[2] * point.x +
          transform.modelColumn1[2] * point.y +
          transform.modelColumn2[2] * point.z + transform.modelColumn3[2],
  };
}

iggy3d::Vec3 transformInstanceNormal(
    const iggy3d::vulkan::StaticMeshInstanceTransform& transform,
    iggy3d::Vec3 normal) {
  iggy3d::Vec3 world{
      transform.normalColumn0[0] * normal.x +
          transform.normalColumn1[0] * normal.y +
          transform.normalColumn2[0] * normal.z,
      transform.normalColumn0[1] * normal.x +
          transform.normalColumn1[1] * normal.y +
          transform.normalColumn2[1] * normal.z,
      transform.normalColumn0[2] * normal.x +
          transform.normalColumn1[2] * normal.y +
          transform.normalColumn2[2] * normal.z,
  };
  const float length =
      std::sqrt(world.x * world.x + world.y * world.y + world.z * world.z);
  if (length > 0.000001F) {
    world.x /= length;
    world.y /= length;
    world.z /= length;
  }
  return world;
}

bool near(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::abs(lhs - rhs) <= epsilon;
}

bool cacheReusesImportAndRendererEmitsIrregularGeometry() {
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::StaticMeshAsset* first = cache.find("boulder_01");
  const iggy3d::StaticMeshAsset* second = cache.find("boulder_01");
  const iggy3d::SceneRoomProjection room = roomWith("asset:boulder_01");
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr, &cache);
  return expect(first != nullptr && second == first, "cache returns same asset") &&
         expect(cache.loadedAssetCount() == 1U, "cache imports once") &&
         expect(cache.failedAssetCount() == 0U, "cache has no failures") &&
         expect(geometry.ready, "imported room geometry ready") &&
         expect(geometry.vertices.size() == 96U,
                "triangle-expanded imported vertex count") &&
         expect(geometry.indices.size() == 96U,
                "imported geometry index count") &&
         expect(geometry.indexedDraws.size() == 1U,
                "one imported primitive draw") &&
         expect(std::isfinite(geometry.vertices.front().position[0]),
                "imported world transform finite");
}

bool repeatedAssetsUseOneAtlasMeshAndCompactTransforms() {
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::vulkan::StaticMeshAssetAtlasCpuGeometry atlas =
      iggy3d::vulkan::buildStaticMeshAssetAtlasCpuGeometry(&cache);
  const auto boulder = std::find_if(
      atlas.assetDraws.begin(), atlas.assetDraws.end(),
      [](const iggy3d::vulkan::StaticMeshAssetDrawRanges& asset) {
        return asset.assetId == "boulder_01";
      });
  if (!expect(atlas.valid && boulder != atlas.assetDraws.end() &&
                  boulder->indexedDraws.size() == 1U,
              "boulder exists once in immutable asset atlas")) {
    return false;
  }

  iggy3d::SceneRoomProjection repeated;
  repeated.loaded = true;
  repeated.assetId = "repeated_asset_room";
  repeated.meshes.reserve(300U);
  for (std::size_t index = 0U; index < 300U; ++index) {
    iggy3d::SceneRoomMeshItem mesh =
        roomWith("asset:boulder_01").meshes.front();
    mesh.id = "rock_" + std::to_string(index);
    mesh.position.x += static_cast<float>(index % 30U) * 4.0F;
    mesh.position.z += static_cast<float>(index / 30U) * 5.0F;
    repeated.meshes.push_back(std::move(mesh));
  }
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(
          repeated, nullptr, atlas.assetDraws);
  const iggy3d::vulkan::StaticMeshInstanceBatch* batch =
      geometry.staticMeshInstanceBatches.empty()
          ? nullptr
          : &geometry.staticMeshInstanceBatches.front();

  const iggy3d::SceneRoomProjection one = roomWith("asset:boulder_01");
  const iggy3d::vulkan::RoomMeshCpuGeometry legacy =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(one, nullptr, &cache);
  const std::uint32_t firstAtlasIndex =
      atlas.indices[boulder->indexedDraws.front().firstIndex];
  const iggy3d::vulkan::StaticMeshInstanceVertex& firstAtlasVertex =
      atlas.vertices[firstAtlasIndex];
  const iggy3d::Vec3 transformed = transformInstancePoint(
      geometry.staticMeshInstances.front(),
      {firstAtlasVertex.position[0], firstAtlasVertex.position[1],
       firstAtlasVertex.position[2]});
  const iggy3d::Vec3 transformedNormal = transformInstanceNormal(
      geometry.staticMeshInstances.front(),
      {firstAtlasVertex.normal[0], firstAtlasVertex.normal[1],
       firstAtlasVertex.normal[2]});
  iggy3d::Vec3 light{-0.35F, 0.85F, 0.40F};
  const float lightLength =
      std::sqrt(light.x * light.x + light.y * light.y + light.z * light.z);
  light = {light.x / lightLength, light.y / lightLength,
           light.z / lightLength};
  const float brightness =
      0.52F + 0.48F * std::max(
                           0.0F, transformedNormal.x * light.x +
                                     transformedNormal.y * light.y +
                                     transformedNormal.z * light.z);

  return expect(geometry.ready, "instanced room geometry ready") &&
         expect(geometry.vertices.empty() && geometry.indices.empty() &&
                    geometry.indexedDraws.empty(),
                "room geometry does not duplicate imported triangles") &&
         expect(boulder->indexedDraws.front().indexCount == 96U,
                "one immutable boulder triangle range retained") &&
         expect(geometry.staticMeshInstances.size() == 300U,
                "three hundred compact transforms emitted") &&
         expect(geometry.staticMeshInstanceBatches.size() == 1U &&
                    batch != nullptr && batch->instanceCount == 300U &&
                    batch->firstInstance == 0U &&
                    batch->firstIndex ==
                        boulder->indexedDraws.front().firstIndex &&
                    batch->indexCount == 96U,
                "one instanced draw owns all repeated boulders") &&
         expect(legacy.ready && legacy.vertices.size() == 96U &&
                    near(transformed.x, legacy.vertices.front().position[0]) &&
                    near(transformed.y, legacy.vertices.front().position[1]) &&
                    near(transformed.z, legacy.vertices.front().position[2]),
                "instance transform preserves legacy world geometry") &&
         expect(near(firstAtlasVertex.baseColor[0] * brightness,
                     legacy.vertices.front().color[0]) &&
                    near(firstAtlasVertex.baseColor[1] * brightness,
                         legacy.vertices.front().color[1]) &&
                    near(firstAtlasVertex.baseColor[2] * brightness,
                         legacy.vertices.front().color[2]),
                "instance normal transform preserves legacy face lighting");
}

bool instanceTransformRejectsInvalidGeometry() {
  iggy3d::SceneRoomMeshItem item =
      roomWith("asset:boulder_01").meshes.front();
  iggy3d::vulkan::StaticMeshInstanceTransform output;
  output.modelColumn0.fill(1.0F);
  const bool flatBounds =
      iggy3d::vulkan::buildStaticMeshInstanceTransform(
          item, {-1.0F, 0.0F, -1.0F}, {1.0F, 0.0F, 1.0F}, output);
  const bool clearedAfterFlatBounds =
      std::all_of(output.modelColumn0.begin(), output.modelColumn0.end(),
                  [](float value) { return value == 0.0F; });
  item.position.x = std::numeric_limits<float>::quiet_NaN();
  const bool nonFiniteItem =
      iggy3d::vulkan::buildStaticMeshInstanceTransform(
          item, {-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, output);
  return expect(!flatBounds && clearedAfterFlatBounds,
                "degenerate asset bounds fail closed") &&
         expect(!nonFiniteItem,
                "non-finite instance transform input fails closed");
}

bool missingAssetIsVisibleAndMemoized() {
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::SceneRoomProjection room = roomWith("asset:not_here");
  const iggy3d::vulkan::RoomMeshCpuGeometry first =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr, &cache);
  const iggy3d::vulkan::RoomMeshCpuGeometry second =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr, &cache);
  const iggy3d::vulkan::StaticMeshAssetAtlasCpuGeometry atlas =
      iggy3d::vulkan::buildStaticMeshAssetAtlasCpuGeometry(&cache);
  const iggy3d::vulkan::RoomMeshCpuGeometry instanced =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr,
                                               atlas.assetDraws);
  return expect(first.ready && second.ready, "missing asset proxy ready") &&
         expect(first.vertices.size() == 8U, "missing asset uses box proxy") &&
         expect(first.vertices[0].color[0] == 1.0F &&
                    first.vertices[0].color[1] == 0.0F &&
                    first.vertices[0].color[2] == 1.0F,
                "missing asset proxy is magenta") &&
         expect(cache.failedAssetCount() == 1U,
                "missing asset failure memoized") &&
         expect(cache.failureReason("not_here") ==
                    "static_mesh_file_not_found",
                "missing asset reason retained") &&
         expect(instanced.ready && instanced.vertices.size() == 8U &&
                    instanced.staticMeshInstances.empty() &&
                    instanced.vertices[0].color[0] == 1.0F &&
                    instanced.vertices[0].color[2] == 1.0F,
                "instanced path retains visible missing-asset proxy");
}

bool windowMaterialsDriveDistinctGeneratedColors() {
  iggy3d::SceneRoomProjection glazing = roomWith("creative_box_proxy");
  glazing.meshes.front().materialId = "creative_window_glass";
  iggy3d::SceneRoomProjection shutters = roomWith("creative_box_proxy");
  shutters.meshes.front().materialId = "creative_window_shutter";
  const iggy3d::vulkan::RoomMeshCpuGeometry glazingGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(glazing);
  const iggy3d::vulkan::RoomMeshCpuGeometry shutterGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(shutters);

  return expect(glazingGeometry.ready && shutterGeometry.ready &&
                    glazingGeometry.vertices.size() == 8U &&
                    shutterGeometry.vertices.size() == 8U,
                "window material fixtures render") &&
         expect(glazingGeometry.vertices.front().color[0] == 0.36F &&
                    glazingGeometry.vertices.front().color[1] == 0.68F &&
                    glazingGeometry.vertices.front().color[2] == 0.78F,
                "glazing renders with cool glass color") &&
         expect(shutterGeometry.vertices.front().color[0] == 0.38F &&
                    shutterGeometry.vertices.front().color[1] == 0.23F &&
                    shutterGeometry.vertices.front().color[2] == 0.12F,
                "shutters render with timber color");
}

bool generatedSlabsPreserveVolumeMaterialAndSemanticRole() {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.assetId = "semantic_slab_room";

  iggy3d::SceneRoomMeshItem floor;
  floor.id = "floor";
  floor.meshId = "creative_walkable_slab";
  floor.role = "floor";
  floor.semanticRole = "Floor";
  floor.materialId = "creative_wall_stone";
  floor.position = {0.0F, 0.25F, 0.0F};
  floor.size = {2.0F, 0.5F, 2.0F};
  room.meshes.push_back(floor);
  const iggy3d::vulkan::RoomMeshCpuGeometry floorOnlyGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  iggy3d::SceneRoomMeshItem roof = floor;
  roof.id = "roof";
  roof.semanticRole = "Roof";
  roof.materialId = "creative_prop";
  roof.position = {4.0F, 3.125F, 0.0F};
  roof.size.y = 0.25F;
  room.meshes.push_back(roof);

  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  const auto countColor = [&](std::array<float, 3U> color) {
    return static_cast<std::size_t>(std::count_if(
        geometry.vertices.begin(), geometry.vertices.end(),
        [&](const iggy3d::vulkan::FirstRoomVertex& vertex) {
          return near(vertex.color[0], color[0]) &&
                 near(vertex.color[1], color[1]) &&
                 near(vertex.color[2], color[2]);
        }));
  };
  const auto hasColoredVertexAtHeight =
      [&](std::array<float, 3U> color, float height) {
        return std::any_of(
            geometry.vertices.begin(), geometry.vertices.end(),
            [&](const iggy3d::vulkan::FirstRoomVertex& vertex) {
              return near(vertex.color[0], color[0]) &&
                     near(vertex.color[1], color[1]) &&
                     near(vertex.color[2], color[2]) &&
                     near(vertex.position[1], height);
            });
      };
  constexpr std::array<float, 3U> kStone{0.43F, 0.46F, 0.48F};
  constexpr std::array<float, 3U> kRoof{0.34F, 0.25F, 0.20F};

  return expect(geometry.ready && geometry.roomFloorDrawCount == 2U,
                "floor and roof remain separate optimized draws") &&
         expect(floorOnlyGeometry.ready &&
                    floorOnlyGeometry.roomGridLineDrawCount > 0U &&
                    geometry.roomGridLineDrawCount ==
                        floorOnlyGeometry.roomGridLineDrawCount,
                "roof semantics do not inherit the editable floor grid") &&
         expect(countColor(kStone) == 8U && countColor(kRoof) == 8U,
                "optimized slabs retain material and semantic colors") &&
         expect(hasColoredVertexAtHeight(kStone, 0.0F) &&
                    hasColoredVertexAtHeight(kStone, 0.5F) &&
                    hasColoredVertexAtHeight(kRoof, 3.0F) &&
                    hasColoredVertexAtHeight(kRoof, 3.25F),
                "optimized slabs render bottom and top faces at full thickness");
}

bool externalFloorAndWallBypassGeneratedBatching() {
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::vulkan::RoomMeshCpuGeometry floor =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(
          roomWith("asset:boulder_01", "floor"), nullptr, &cache);
  const iggy3d::vulkan::RoomMeshCpuGeometry wall =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(
          roomWith("asset:boulder_01", "wall"), nullptr, &cache);
  return expect(floor.ready && wall.ready,
                "external structural geometry ready") &&
         expect(floor.vertices.size() == 96U &&
                    wall.vertices.size() == 96U,
                "external structural geometry not duplicated by batching");
}

bool generatedTraversalProfilesEmitBoundedGeometry() {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.assetId = "generated_traversal_room";
  iggy3d::SceneRoomMeshItem ramp;
  ramp.id = "ramp";
  ramp.meshId = "creative_ramp_wedge";
  ramp.role = "prop";
  ramp.materialId = "creative_prop";
  ramp.position = {-2.0F, 0.5F, 0.0F};
  ramp.size = {2.0F, 1.0F, 4.0F};
  room.meshes.push_back(ramp);
  iggy3d::SceneRoomMeshItem stair;
  stair.id = "stair";
  stair.meshId = "creative_stair_steps";
  stair.role = "prop";
  stair.materialId = "creative_prop";
  stair.position = {2.0F, 0.5F, 0.0F};
  stair.size = {2.0F, 1.0F, 3.0F};
  stair.proceduralSegmentCount = 4U;
  room.meshes.push_back(stair);
  iggy3d::SceneRoomMeshItem arch;
  arch.id = "arch";
  arch.meshId = "creative_open_frame";
  arch.role = "prop";
  arch.materialId = "creative_prop";
  arch.position = {8.0F, 1.5F, 0.0F};
  arch.size = {3.0F, 3.0F, 0.5F};
  room.meshes.push_back(arch);

  const iggy3d::vulkan::RoomMeshCpuGeometry fourSteps =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  room.meshes[1].proceduralSegmentCount = 5U;
  const iggy3d::vulkan::RoomMeshCpuGeometry fiveSteps =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  const iggy3d::vulkan::CreativePreviewCpuGeometry preview =
      iggy3d::vulkan::buildCreativePreviewCpuGeometry();
  iggy3d::vulkan::CreativePreviewGeometryResources resources;
  resources.indexedDraws = preview.indexedDraws;
  iggy3d::RenderCreativePreviewItem box;
  box.role = iggy3d::RenderCreativePreviewRole::Held;
  iggy3d::RenderCreativePreviewItem rampPreview = box;
  rampPreview.geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::RampWedge;
  iggy3d::RenderCreativePreviewItem stairPreview = box;
  stairPreview.geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::StairSteps;
  stairPreview.proceduralSegmentCount = 4U;
  iggy3d::RenderCreativePreviewItem stairTarget = stairPreview;
  stairTarget.role = iggy3d::RenderCreativePreviewRole::PlacementValid;
  iggy3d::RenderCreativePreviewItem archPreview = box;
  archPreview.geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::OpenFrame;
  iggy3d::RenderCreativePreviewItem archTarget = archPreview;
  archTarget.role = iggy3d::RenderCreativePreviewRole::PlacementValid;
  const std::uint32_t boxDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources, box);
  const std::uint32_t rampDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources,
                                                               rampPreview);
  const std::uint32_t stairDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources,
                                                               stairPreview);
  const std::uint32_t stairTargetDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources,
                                                               stairTarget);
  const std::uint32_t archDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources,
                                                               archPreview);
  const std::uint32_t archTargetDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources,
                                                               archTarget);

  return expect(fourSteps.ready && fourSteps.vertices.size() == 62U &&
                    fourSteps.indices.size() == 552U &&
                    fourSteps.indexedDraws.size() == 8U,
                "ramp stairs and open frame emit bounded geometry") &&
         expect(near(fourSteps.vertices[0].position[1], 0.0F) &&
                    near(fourSteps.vertices[4].position[1], 1.0F) &&
                    near(fourSteps.vertices[6].position[1], 0.0F) &&
                    near(fourSteps.vertices[37].position[1], 1.0F),
                "generated vertices preserve ramp and stair vertical extents") &&
         expect(near(fourSteps.vertices[38].position[0], 6.5F) &&
                    near(fourSteps.vertices[40].position[0], 7.1F) &&
                    near(fourSteps.vertices[46].position[0], 8.9F) &&
                    near(fourSteps.vertices[54].position[1], 2.25F) &&
                    near(fourSteps.vertices[56].position[1], 3.0F),
                "open frame mesh preserves two piers and a clear opening") &&
         expect(fiveSteps.ready && fiveSteps.vertices.size() == 70U &&
                    fiveSteps.indices.size() == 624U &&
                    fiveSteps.sourceRoomGeometrySignature !=
                        fourSteps.sourceRoomGeometrySignature,
                "segment-count changes invalidate room geometry signature") &&
         expect(preview.ready && preview.indexedDraws.size() ==
                                       iggy3d::vulkan::
                                           kCreativePreviewGeometryDrawRangeCount &&
                    boxDraw != rampDraw && rampDraw != stairDraw &&
                    stairDraw != archDraw &&
                    stairDraw != stairTargetDraw &&
                    archDraw != archTargetDraw &&
                    stairTargetDraw < preview.indexedDraws.size() &&
                    archTargetDraw < preview.indexedDraws.size() &&
                    preview.indexedDraws[boxDraw].indexCount == 72U &&
                    preview.indexedDraws[rampDraw].indexCount == 48U &&
                    preview.indexedDraws[archDraw].indexCount == 216U &&
                    preview.indexedDraws[stairDraw].indexCount == 288U &&
                    preview.indexedDraws[archTargetDraw].indexCount >
                        preview.indexedDraws[archDraw].indexCount &&
                    preview.indexedDraws[stairTargetDraw].indexCount >
                        preview.indexedDraws[stairDraw].indexCount,
                "preview atlas owns exact generated profile draw ranges");
}

bool hipRoofProfileEmitsTaperedPanelGeometry() {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.assetId = "hip_roof_profile";
  iggy3d::SceneRoomMeshItem panel;
  panel.id = "hip_north";
  panel.meshId = "creative_hip_roof_panel";
  panel.role = "prop";
  panel.materialId = "creative_structural_stone";
  panel.position = {0.0F, 3.0F, 0.0F};
  panel.size = {10.0F, 0.25F, std::sqrt(32.0F)};
  panel.rotationEulerRadians = {
      static_cast<float>(-std::numbers::pi / 4.0), 0.0F, 0.0F};
  room.meshes.push_back(panel);
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  room.meshes[0].size.x = 2.0F;
  const iggy3d::vulkan::RoomMeshCpuGeometry invalid =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  return expect(geometry.ready && geometry.vertices.size() == 8U &&
                    geometry.indices.size() == 72U &&
                    geometry.indexedDraws.size() == 1U,
                "hip panel emits one bounded eight-vertex prism") &&
         expect(near(geometry.vertices[0].position[0], -5.0F) &&
                    near(geometry.vertices[1].position[0], 5.0F) &&
                    near(geometry.vertices[2].position[0], 1.0F) &&
                    near(geometry.vertices[3].position[0], -1.0F) &&
                    near(geometry.vertices[4].position[0], -5.0F) &&
                    near(geometry.vertices[5].position[0], 5.0F) &&
                    near(geometry.vertices[6].position[0], 1.0F) &&
                    near(geometry.vertices[7].position[0], -1.0F),
                "hip panel narrows its high edge to the canonical ridge") &&
         expect(!invalid.ready && invalid.vertices.empty() &&
                    invalid.indices.empty() && invalid.indexedDraws.empty(),
                "impossible hip taper fails atomically instead of drawing a box");
}

bool discoveryAndPreviewAtlasCoverEveryValidFixture() {
  struct ModularAssetExpectation {
    std::string_view assetId;
    std::string_view category;
    iggy3d::StaticMeshCollisionMode collision;
    bool walkable = false;
    std::size_t collisionPartCount = 0U;
    std::size_t walkablePartCount = 0U;
    std::size_t socketCount = 0U;
    std::size_t plugCount = 0U;
    std::size_t receiverCount = 0U;
  };
  constexpr std::array kModularAssets{
      ModularAssetExpectation{"homestead/modular/beam_4x0p4", "structure",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/ceiling_4x4", "ceiling",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/door_leaf_1p1x2p2", "door",
                              iggy3d::StaticMeshCollisionMode::Bounds, false,
                              0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "homestead/modular/door_frame_1p5x2p46", "prop",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U, 0U,
          1U, 0U, 1U},
      ModularAssetExpectation{"homestead/modular/floor_4x4", "floor",
                              iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{"homestead/modular/foundation_4x4", "foundation",
                              iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{"homestead/modular/gable_cap_4x1p5", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"homestead/modular/lintel_2x0p6", "wall",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/pillar_0p5x3", "structure",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/modular/roof_gable_slope_4p4x2p2", "roof",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"homestead/modular/roof_ridge_4p4", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"homestead/modular/roof_shed_4x4", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"homestead/modular/wall_full_4x3", "wall",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/wall_half_2x3", "wall",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/wall_low_4x1p2", "wall",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/wall_pier_1x3", "wall",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"homestead/modular/window_frame_1p5x1p2", "window",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "homestead/modular/stair_straight_2x3x1p5", "stairs",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 6U},
      ModularAssetExpectation{"homestead/modular/porch_4x2x0p5", "structure",
                              iggy3d::StaticMeshCollisionMode::CompoundBounds,
                              true, 3U, 3U},
      ModularAssetExpectation{"homestead/modular/bridge_4x2", "bridge",
                              iggy3d::StaticMeshCollisionMode::CompoundBounds,
                              true, 3U, 1U},
  };
  constexpr std::array kInteriorAssets{
      ModularAssetExpectation{
          "homestead/interior/bed_single_2p1x1p1", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/interior/chair_ladderback_0p56", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/interior/bench_rustic_1p6", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/interior/bookshelf_1p1x2p1", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 0U,
          1U},
      ModularAssetExpectation{
          "homestead/interior/dresser_1p3", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 0U,
          1U},
      ModularAssetExpectation{
          "homestead/interior/storage_chest_1p1", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 0U,
          1U},
      ModularAssetExpectation{
          "homestead/interior/hearth_stone_1p8", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 0U,
          1U},
      ModularAssetExpectation{
          "homestead/interior/barrel_oak_0p8", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/interior/crate_wood_0p6", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 2U, 1U,
          1U},
      ModularAssetExpectation{
          "homestead/interior/lantern_iron_0p7", "prop",
          iggy3d::StaticMeshCollisionMode::None, false, 0U, 0U, 1U, 1U,
          0U},
  };
  constexpr std::array kYardAssets{
      ModularAssetExpectation{
          "homestead/yard/fence_2m", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/yard/fence_4m", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/yard/fence_corner_2x2", "prop",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 7U},
      ModularAssetExpectation{
          "homestead/yard/gate_frame_1p8", "prop",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U, 0U,
          1U, 0U, 1U},
      ModularAssetExpectation{
          "homestead/yard/gate_leaf_1p4", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U,
          0U},
      ModularAssetExpectation{
          "homestead/yard/well_roofed_1p8", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/yard/handcart_2p4", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/yard/signpost_2p2", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 0U,
          1U},
      ModularAssetExpectation{
          "homestead/yard/woodpile_1p5", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/yard/trough_1p6", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "homestead/yard/hay_bale_1p0", "prop",
          iggy3d::StaticMeshCollisionMode::Bounds},
  };
  constexpr std::array kWoodlandAssets{
      ModularAssetExpectation{
          "woodland/broadleaf_young_4p7m", "tree",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "woodland/broadleaf_mature_7p3m", "tree",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "woodland/broadleaf_ancient_9p5m", "tree",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "woodland/pine_medium_6p3m", "tree",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "woodland/pine_tall_9p3m", "tree",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "woodland/dead_tree_snag_6m", "tree",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "woodland/tree_stump_0p8m", "log",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "woodland/fallen_branch_pile_2m", "log",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "woodland/shrub_low_1p2m", "shrub",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "woodland/shrub_dense_1p8m", "shrub",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "woodland/fern_cluster_1p2m", "foliage",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "woodland/reed_grass_clump_1p4m", "foliage",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "woodland/rock_cluster_small_1p5m", "rock",
          iggy3d::StaticMeshCollisionMode::None},
  };
  constexpr std::array kInfrastructureAssets{
      ModularAssetExpectation{
          "infrastructure/stone_path_straight_2x2", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 4U, 4U},
      ModularAssetExpectation{
          "infrastructure/stone_path_straight_4x2", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 8U, 8U},
      ModularAssetExpectation{
          "infrastructure/stone_path_turn_4x4", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 7U, 7U},
      ModularAssetExpectation{
          "infrastructure/stone_path_t_4x4", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 10U, 10U},
      ModularAssetExpectation{
          "infrastructure/stone_path_cross_4x4", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 12U, 12U},
      ModularAssetExpectation{
          "infrastructure/stone_path_end_2x2", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 4U, 4U},
      ModularAssetExpectation{
          "infrastructure/doorway_threshold_2x1", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 2U, 2U},
      ModularAssetExpectation{
          "infrastructure/stepping_stones_4x1p5", "walkway",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 7U, 7U},
      ModularAssetExpectation{
          "infrastructure/retaining_wall_2x1p2", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "infrastructure/retaining_wall_4x1p2", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "infrastructure/retaining_wall_corner_2x2x1p2", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 2U},
      ModularAssetExpectation{
          "infrastructure/retaining_wall_end_1x1p2", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 1U},
      ModularAssetExpectation{
          "infrastructure/terrain_steps_2x3x1p2", "stairs",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 6U},
      ModularAssetExpectation{
          "infrastructure/drainage_culvert_2x2", "bridge",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 4U},
      ModularAssetExpectation{
          "infrastructure/roadside_marker_0p4x1p2", "marker",
          iggy3d::StaticMeshCollisionMode::Bounds},
  };
  constexpr std::array kRockCliffAssets{
      ModularAssetExpectation{
          "rock_cliff/boulder_medium_1p8m", "rock",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "rock_cliff/boulder_large_3m", "rock",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "rock_cliff/rock_outcrop_low_3x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "rock_cliff/rock_outcrop_tall_3x3", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 5U},
      ModularAssetExpectation{
          "rock_cliff/cliff_face_2x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "rock_cliff/cliff_face_4x3", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 12U},
      ModularAssetExpectation{
          "rock_cliff/cliff_corner_inner_2x2x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 8U},
      ModularAssetExpectation{
          "rock_cliff/cliff_corner_outer_2x2x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 8U},
      ModularAssetExpectation{
          "rock_cliff/cliff_cap_walkable_4x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 2U},
      ModularAssetExpectation{
          "rock_cliff/natural_stone_steps_2x3x1p5", "stairs",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 6U},
      ModularAssetExpectation{
          "rock_cliff/cave_mouth_arch_4x3p5", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 9U},
      ModularAssetExpectation{
          "rock_cliff/scree_pile_3x2", "rock",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "rock_cliff/rubble_cluster_2x1p5", "rock",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "rock_cliff/terrain_transition_left_4x2x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "rock_cliff/terrain_transition_right_4x2x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "rock_cliff/overlook_ledge_4x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 5U, 2U},
  };
  constexpr std::array kCaveAssets{
      ModularAssetExpectation{
          "cave/floor_4x4", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 6U},
      ModularAssetExpectation{
          "cave/ramp_4x4x1p5", "stairs",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 8U, 8U},
      ModularAssetExpectation{
          "cave/wall_straight_4x3", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 8U},
      ModularAssetExpectation{
          "cave/wall_corner_inner_4x4x3", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 12U},
      ModularAssetExpectation{
          "cave/wall_corner_outer_4x4x3", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 12U},
      ModularAssetExpectation{
          "cave/ceiling_4x4", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 6U},
      ModularAssetExpectation{
          "cave/tunnel_straight_4x4x3p5", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 20U, 6U},
      ModularAssetExpectation{
          "cave/tunnel_turn_6x6x3p5", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 30U, 5U},
      ModularAssetExpectation{
          "cave/tunnel_t_junction_6x6x3p5", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 28U, 5U},
      ModularAssetExpectation{
          "cave/tunnel_dead_end_4x4x3p5", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 26U, 6U},
      ModularAssetExpectation{
          "cave/chamber_8x8x4", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 40U, 9U},
      ModularAssetExpectation{
          "cave/rock_column_1p4x3p2", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U},
      ModularAssetExpectation{
          "cave/stalactite_cluster_2x2x3", "decor",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "cave/stalagmite_cluster_2x2x2", "decor",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "cave/collapsed_passage_4x3", "structure",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 9U},
      ModularAssetExpectation{
          "cave/ledge_walkable_4x2", "terrain",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 5U, 2U},
  };
  constexpr std::array kStealthBlockoutAssets{
      ModularAssetExpectation{
          "stealth_blockout/cover_low_wall_2x1p1", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/cover_high_wall_2x2", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/cover_crate_1m", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/cover_crate_0p5", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/cover_barrel_0p6x1p1", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/cover_sandbag_run_2x0p9", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/clamber_block_0p6", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "stealth_blockout/clamber_block_1p0", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "stealth_blockout/clamber_block_1p4", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "stealth_blockout/clamber_block_1p8", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "stealth_blockout/clamber_fail_2p0", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "stealth_blockout/vault_rail_2x1p0", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/platform_2x2x0p5", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "stealth_blockout/stair_run_2x3x1p5", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 6U},
      ModularAssetExpectation{
          "stealth_blockout/ramp_2x2x1", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 8U, 8U},
      ModularAssetExpectation{
          "stealth_blockout/ledge_shelf_1x1p8", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 2U, 1U},
      ModularAssetExpectation{
          "stealth_blockout/wall_seg_2x2", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/wall_seg_4x2", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/wall_window_2x2", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 2U},
      ModularAssetExpectation{
          "stealth_blockout/doorway_2x1", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U},
      ModularAssetExpectation{
          "stealth_blockout/pillar_0p5x3", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/corner_l_2x2", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 2U},
      ModularAssetExpectation{
          "stealth_blockout/guard_post_1p8", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/player_gauge_1p8", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "stealth_blockout/giant_stair_4x6x3", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 6U, 6U},
      ModularAssetExpectation{
          "stealth_blockout/giant_shelf_2x3p6", "stealth_blockout",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 2U, 1U},
  };
  // ASSET-CAL-1 Batch 0: calibration and pipeline proofs.
  constexpr std::array kCalibrationAssets{
      ModularAssetExpectation{"calibration/grid_1m_10x10", "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"calibration/human_gauge_1p8m", "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"calibration/door_clearance_0p9x2p1",
                              "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"calibration/storey_3m", "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"calibration/pivot_hinge", "calibration",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"calibration/socket_receiver", "calibration",
                              iggy3d::StaticMeshCollisionMode::None, false, 0U,
                              0U, 1U, 0U, 1U},
      ModularAssetExpectation{"calibration/socket_plug", "calibration",
                              iggy3d::StaticMeshCollisionMode::None, false, 0U,
                              0U, 1U, 1U, 0U},
      ModularAssetExpectation{"calibration/collision_compound", "calibration",
                              iggy3d::StaticMeshCollisionMode::CompoundBounds,
                              true, 2U, 1U},
      ModularAssetExpectation{"calibration/material_base_color",
                              "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"calibration/material_texture_uv", "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"calibration/material_missing_texture",
                              "calibration",
                              iggy3d::StaticMeshCollisionMode::None},
  };

  // ASSET-CAL-1 Batch 1 seed: building-closure architecture assets.
  constexpr std::array kArchitectureAssets{
      ModularAssetExpectation{
          "architecture/openings/door_frame_standard", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U, 0U, 1U,
          0U, 1U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_standard_closed", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_standard_open", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/traversal/stair_straight_3m", "traversal",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 16U, 16U},
      ModularAssetExpectation{
          "architecture/traversal/stair_landing_2x2m", "traversal",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "architecture/structural/railing_straight_2m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/roof/ridge_cap_straight_4m", "roof",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"architecture/roof/ridge_cap_end", "roof",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      // ASSET-BLD-1 phase 4: building-closure tranche 1.
      ModularAssetExpectation{
          "architecture/openings/door_frame_wide", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U, 0U, 1U,
          0U, 1U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_wide_closed", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_wide_open", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_frame_standard", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U, 0U, 3U,
          0U, 3U},
      ModularAssetExpectation{
          "architecture/openings/window_frame_small", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "architecture/openings/window_shutter_left_closed", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_shutter_left_open", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_shutter_right_closed", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_shutter_right_open", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_mullion_cross", "openings",
          iggy3d::StaticMeshCollisionMode::None, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_sill_standard", "openings",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/openings/window_lintel_standard", "openings",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/traversal/stair_straight_3m_with_rails", "traversal",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 24U, 16U},
      ModularAssetExpectation{
          "architecture/traversal/stair_rail_slope_3m", "traversal",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/traversal/stair_newel_post", "traversal",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/roof/ridge_cap_straight_2m", "roof",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"architecture/roof/eave_trim_2m", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"architecture/roof/eave_trim_4m", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"architecture/roof/eave_outer_corner", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"architecture/roof/fascia_end", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"architecture/roof/gable_cap_4m", "roof",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"architecture/roof/gutter_straight_2m", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"architecture/roof/downspout_3m", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{"architecture/roof/downspout_outlet", "roof",
                              iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/roof/chimney_stack_short", "roof",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 0U, 1U},
      ModularAssetExpectation{
          "architecture/roof/chimney_cap", "roof",
          iggy3d::StaticMeshCollisionMode::None, false, 0U, 0U, 1U, 1U, 0U},
      // ASSET-BLD-2: double doors, window completion, structural core.
      ModularAssetExpectation{
          "architecture/openings/door_frame_double", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U, 0U, 2U,
          0U, 2U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_double_left_closed", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_double_left_open", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_double_right_closed", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/door_leaf_double_right_open", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/openings/window_frame_wide", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "architecture/openings/window_frame_tall", "openings",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 4U},
      ModularAssetExpectation{
          "architecture/openings/window_bars_standard", "openings",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{
          "architecture/structural/foundation_plinth_straight_2m",
          "structural", iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/foundation_plinth_straight_4m",
          "structural", iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/foundation_plinth_inner_corner",
          "structural", iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/foundation_plinth_outer_corner",
          "structural", iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/foundation_plinth_end", "structural",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/post_square_0p3x3m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/post_square_0p5x3m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/column_round_0p5x3m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds, false, 0U, 0U, 2U, 1U, 1U},
      ModularAssetExpectation{
          "architecture/structural/column_round_base", "structural",
          iggy3d::StaticMeshCollisionMode::None, false, 0U, 0U, 1U, 0U, 1U},
      ModularAssetExpectation{
          "architecture/structural/column_round_cap", "structural",
          iggy3d::StaticMeshCollisionMode::None, false, 0U, 0U, 1U, 1U, 0U},
      ModularAssetExpectation{"architecture/structural/beam_2m", "structural",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"architecture/structural/beam_4m", "structural",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{"architecture/structural/beam_6m", "structural",
                              iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/beam_end_cap", "structural",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/brace_left", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/brace_right", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      // ASSET-BLD-3: Batch 1 closure — arches, buttresses, piers, parapets,
      // balconies, small railings, quarter-turn stair, stepped ramps.
      ModularAssetExpectation{
          "architecture/traversal/stair_quarter_turn_3m", "traversal",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 16U, 16U},
      ModularAssetExpectation{
          "architecture/traversal/stair_landing_2x4m", "traversal",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "architecture/traversal/ramp_2x3x1p5m", "traversal",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 8U, 8U},
      ModularAssetExpectation{
          "architecture/traversal/ramp_2x6x3m", "traversal",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 16U, 16U},
      ModularAssetExpectation{
          "architecture/traversal/ramp_landing_2x2m", "traversal",
          iggy3d::StaticMeshCollisionMode::Bounds, true},
      ModularAssetExpectation{
          "architecture/structural/arch_1p2m", "structural",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U},
      ModularAssetExpectation{
          "architecture/structural/arch_2m", "structural",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U},
      ModularAssetExpectation{
          "architecture/structural/arch_4m", "structural",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, false, 3U},
      ModularAssetExpectation{
          "architecture/structural/buttress_low", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/buttress_tall", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/wall_pier_0p5x3m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/wall_pier_1x3m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/parapet_straight_2m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/parapet_straight_4m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/parapet_inner_corner", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/parapet_outer_corner", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/parapet_end", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/balcony_deck_2x1p5m", "structural",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 3U, 1U},
      ModularAssetExpectation{
          "architecture/structural/balcony_deck_4x1p5m", "structural",
          iggy3d::StaticMeshCollisionMode::CompoundBounds, true, 4U, 1U},
      ModularAssetExpectation{
          "architecture/structural/balcony_bracket", "structural",
          iggy3d::StaticMeshCollisionMode::None},
      ModularAssetExpectation{
          "architecture/structural/railing_straight_1m", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/railing_corner", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
      ModularAssetExpectation{
          "architecture/structural/railing_end_post", "structural",
          iggy3d::StaticMeshCollisionMode::Bounds},
  };
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  const iggy3d::StaticMeshAssetCatalog missing =
      iggy3d::discoverStaticMeshAssetCatalog("assets/not-a-directory");
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::vulkan::CreativePreviewCpuGeometry preview =
      iggy3d::vulkan::buildCreativePreviewCpuGeometry(&cache);
  const auto boulder = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const iggy3d::StaticMeshAssetCatalogEntry& entry) {
        return entry.assetId == "boulder_01";
      });
  const auto walkway = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const iggy3d::StaticMeshAssetCatalogEntry& entry) {
        return entry.assetId == "walkway_stone_01";
      });
  const auto wall = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const iggy3d::StaticMeshAssetCatalogEntry& entry) {
        return entry.assetId == "homestead/wall_bay_basic";
      });
  const auto table = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const iggy3d::StaticMeshAssetCatalogEntry& entry) {
        return entry.assetId ==
               "homestead/turned_leg_apron_table_v1";
      });
  const auto riverbankBoulder = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const iggy3d::StaticMeshAssetCatalogEntry& entry) {
        return entry.assetId == "riverbank/irregular_boulder_01";
      });
  const auto fallenLog = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const iggy3d::StaticMeshAssetCatalogEntry& entry) {
        return entry.assetId == "riverbank/fallen_log_01";
      });

  iggy3d::vulkan::CreativePreviewGeometryResources resources;
  resources.indexedDraws = preview.indexedDraws;
  resources.assetDraws = preview.assetDraws;
  iggy3d::RenderCreativePreviewItem held;
  held.role = iggy3d::RenderCreativePreviewRole::Held;
  static_cast<void>(iggy3d::setRenderCreativePreviewAssetId(
      held, "boulder_01"));
  iggy3d::RenderCreativePreviewItem target = held;
  target.role = iggy3d::RenderCreativePreviewRole::PlacementValid;
  const std::uint32_t heldDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources, held);
  const std::uint32_t targetDraw =
      iggy3d::vulkan::resolveCreativePreviewGeometryDrawIndex(resources, target);
  const auto maximumPreviewIndex =
      std::max_element(preview.indices.begin(), preview.indices.end());
  const auto validateAssetFamily = [&](const auto& expectedAssets,
                                       std::string_view family) {
    bool assetsValid = true;
    for (const ModularAssetExpectation& expected : expectedAssets) {
      const iggy3d::StaticMeshAssetCatalogEntry* entry =
          catalog.find(expected.assetId);
      const bool validBounds =
          entry != nullptr && std::isfinite(entry->boundsMin.x) &&
          std::isfinite(entry->boundsMin.y) &&
          std::isfinite(entry->boundsMin.z) &&
          std::isfinite(entry->boundsMax.x) &&
          std::isfinite(entry->boundsMax.y) &&
          std::isfinite(entry->boundsMax.z) &&
          entry->boundsMin.x < entry->boundsMax.x &&
          entry->boundsMin.y < entry->boundsMax.y &&
          entry->boundsMin.z < entry->boundsMax.z;
      const bool validMetadata =
          entry != nullptr &&
          entry->authoringMetadata.status ==
              iggy3d::StaticMeshAuthoringMetadataStatus::Authored &&
          entry->authoringMetadata.categoryId == expected.category &&
          entry->authoringMetadata.collisionMode == expected.collision &&
          entry->authoringMetadata.walkable == expected.walkable;
      const bool validCollisionParts =
          entry != nullptr &&
          entry->collisionParts.size() == expected.collisionPartCount &&
          static_cast<std::size_t>(std::count_if(
              entry->collisionParts.begin(), entry->collisionParts.end(),
              [](const iggy3d::StaticMeshCollisionPart& part) {
                return part.walkable;
              })) == expected.walkablePartCount;
      const bool validSockets =
          entry != nullptr &&
          entry->attachmentSockets.size() == expected.socketCount &&
          static_cast<std::size_t>(std::count_if(
              entry->attachmentSockets.begin(), entry->attachmentSockets.end(),
              [](const iggy3d::StaticMeshAttachmentSocket& socket) {
                return socket.role ==
                       iggy3d::StaticMeshAttachmentSocketRole::Plug;
              })) == expected.plugCount &&
          static_cast<std::size_t>(std::count_if(
              entry->attachmentSockets.begin(), entry->attachmentSockets.end(),
              [](const iggy3d::StaticMeshAttachmentSocket& socket) {
                return socket.role ==
                       iggy3d::StaticMeshAttachmentSocketRole::Receiver;
              })) == expected.receiverCount;
      const std::string message = std::string(family) +
                                  " asset imports with authored contract: " +
                                  std::string(expected.assetId);
      assetsValid =
          expect(validBounds && validMetadata && validCollisionParts &&
                     validSockets,
                 message.c_str()) &&
          assetsValid;
    }
    return assetsValid;
  };
  const bool modularAssetsValid =
      validateAssetFamily(kModularAssets, "modular");
  const bool interiorAssetsValid =
      validateAssetFamily(kInteriorAssets, "interior");
  const bool yardAssetsValid = validateAssetFamily(kYardAssets, "yard");
  const bool woodlandAssetsValid =
      validateAssetFamily(kWoodlandAssets, "woodland");
  const bool infrastructureAssetsValid =
      validateAssetFamily(kInfrastructureAssets, "infrastructure");
  const bool rockCliffAssetsValid =
      validateAssetFamily(kRockCliffAssets, "rock and cliff");
  const bool caveAssetsValid = validateAssetFamily(kCaveAssets, "cave");
  const bool stealthBlockoutAssetsValid =
      validateAssetFamily(kStealthBlockoutAssets, "stealth blockout");
  const bool calibrationAssetsValid =
      validateAssetFamily(kCalibrationAssets, "calibration");
  const bool architectureAssetsValid =
      validateAssetFamily(kArchitectureAssets, "architecture");

  return expect(catalog.failures.empty() &&
                    catalog.entries.size() ==
                        6U + kModularAssets.size() + kInteriorAssets.size() +
                            kYardAssets.size() + kWoodlandAssets.size() +
                            kInfrastructureAssets.size() +
                            kRockCliffAssets.size() + kCaveAssets.size() +
                            kStealthBlockoutAssets.size() +
                            kCalibrationAssets.size() +
                            kArchitectureAssets.size(),
                "catalog discovers every valid GLB fixture") &&
         modularAssetsValid &&
         interiorAssetsValid &&
         yardAssetsValid &&
         woodlandAssetsValid &&
         infrastructureAssetsValid &&
         rockCliffAssetsValid &&
         caveAssetsValid &&
         stealthBlockoutAssetsValid &&
         calibrationAssetsValid &&
         architectureAssetsValid &&
         expect(boulder != catalog.entries.end() &&
                    boulder->label == "Boulder 01" &&
                    boulder->authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !boulder->authoringMetadata.walkable &&
                    walkway != catalog.entries.end() &&
                    walkway->label == "Walkway Stone 01" &&
                    walkway->boundsMax.x - walkway->boundsMin.x > 2.9F &&
                    walkway->boundsMin.x < walkway->boundsMax.x &&
                    walkway->boundsMin.y < walkway->boundsMax.y &&
                    walkway->boundsMin.z < walkway->boundsMax.z &&
                    walkway->authoringMetadata.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Authored &&
                    walkway->authoringMetadata.walkable &&
                    walkway->authoringMetadata.categoryId == "walkway" &&
                    wall != catalog.entries.end() &&
                    wall->label == "Wall Bay Basic" &&
                    wall->authoringMetadata.categoryId == "wall" &&
                    wall->authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !wall->authoringMetadata.walkable &&
                    table != catalog.entries.end() &&
                    table->label == "Turned Leg Apron Table V1" &&
                    table->authoringMetadata.categoryId == "furniture" &&
                    table->authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !table->authoringMetadata.walkable &&
                    riverbankBoulder != catalog.entries.end() &&
                    riverbankBoulder->label == "Irregular Boulder 01" &&
                    riverbankBoulder->authoringMetadata.categoryId ==
                        "boulder" &&
                    riverbankBoulder->authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !riverbankBoulder->authoringMetadata.walkable &&
                    riverbankBoulder->boundsMax.x -
                            riverbankBoulder->boundsMin.x >
                        1.4F &&
                    riverbankBoulder->boundsMax.y -
                            riverbankBoulder->boundsMin.y >
                        1.4F &&
                    riverbankBoulder->boundsMax.z -
                            riverbankBoulder->boundsMin.z >
                        1.4F &&
                    fallenLog != catalog.entries.end() &&
                    fallenLog->label == "Fallen Log 01" &&
                    fallenLog->authoringMetadata.categoryId == "log" &&
                    fallenLog->authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !fallenLog->authoringMetadata.walkable &&
                    fallenLog->boundsMax.x - fallenLog->boundsMin.x >
                        2.3F &&
                    fallenLog->boundsMax.y - fallenLog->boundsMin.y >
                        0.65F &&
                    fallenLog->boundsMax.z - fallenLog->boundsMin.z >
                        0.65F &&
                    fallenLog->boundsMax.x - fallenLog->boundsMin.x >
                        2.0F *
                            (fallenLog->boundsMax.y -
                             fallenLog->boundsMin.y) &&
                    fallenLog->boundsMax.x - fallenLog->boundsMin.x >
                        2.0F *
                            (fallenLog->boundsMax.z -
                             fallenLog->boundsMin.z) &&
                    catalog.find("walkway_stone_01") == &*walkway &&
                    catalog.find("homestead/wall_bay_basic") == &*wall &&
                    catalog.find("homestead/turned_leg_apron_table_v1") ==
                        &*table &&
                    catalog.find("riverbank/irregular_boulder_01") ==
                        &*riverbankBoulder &&
                    catalog.find("riverbank/fallen_log_01") == &*fallenLog &&
                    catalog.find("missing") == nullptr,
                "discovery retains labels, bounds, metadata, and lookup") &&
         expect(missing.entries.empty() && missing.failures.size() == 1U &&
                    missing.failures[0].reasonCode ==
                        "static_mesh_asset_root_not_directory",
                "missing catalog root reports one explicit failure") &&
         expect(preview.ready &&
                    preview.assetDraws.size() == catalog.entries.size() &&
                    preview.indexedDraws.size() ==
                        iggy3d::vulkan::kCreativePreviewGeometryDrawRangeCount +
                            catalog.entries.size() *
                                iggy3d::kRenderCreativePreviewRoleCount,
                "startup atlas contains four colored roles per asset") &&
         expect(preview.vertices.size() >
                        std::numeric_limits<std::uint16_t>::max() &&
                    maximumPreviewIndex != preview.indices.end() &&
                    *maximumPreviewIndex >
                        std::numeric_limits<std::uint16_t>::max() &&
                    *maximumPreviewIndex < preview.vertices.size(),
                "preview atlas crosses the 16-bit index ceiling safely") &&
         expect(heldDraw >=
                    iggy3d::vulkan::kCreativePreviewGeometryDrawRangeCount &&
                    targetDraw != heldDraw &&
                    targetDraw < resources.indexedDraws.size() &&
                    resources.indexedDraws[targetDraw].indexCount >
                        resources.indexedDraws[heldDraw].indexCount,
                "renderer resolves exact held and outlined target ranges");
}

bool texturedFixtureBuildsOneCachedMaterialBinding() {
  const iggy3d::StaticMeshImportResult walkway =
      iggy3d::importStaticMeshGlb(
          "assets/creative/walkway_stone_01.glb", "walkway_stone_01");
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::vulkan::StaticMeshTextureCpuResources textures =
      iggy3d::vulkan::buildStaticMeshTextureCpuResources(&cache);
  iggy3d::vulkan::StaticMeshMaterialTextureResources materialTextures;
  materialTextures.materialBindings = textures.materialBindings;
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(
          roomWith("asset:walkway_stone_01"), nullptr, &cache,
          &materialTextures);
  const iggy3d::vulkan::StaticMeshAssetAtlasCpuGeometry atlas =
      iggy3d::vulkan::buildStaticMeshAssetAtlasCpuGeometry(
          &cache, &materialTextures);
  const auto walkwayDraw = std::find_if(
      atlas.assetDraws.begin(), atlas.assetDraws.end(),
      [](const iggy3d::vulkan::StaticMeshAssetDrawRanges& asset) {
        return asset.assetId == "walkway_stone_01";
      });
  const iggy3d::vulkan::RoomMeshCpuGeometry instanced =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(
          roomWith("asset:walkway_stone_01"), nullptr, atlas.assetDraws);
  const std::array<std::uint8_t, 4> invalidBytes{0U, 1U, 2U, 3U};
  const iggy3d::DecodedImageRgba8 invalid =
      iggy3d::decodeImageRgba8(invalidBytes);

  bool hasNonzeroUv = false;
  for (const iggy3d::vulkan::FirstRoomVertex& vertex : geometry.vertices) {
    hasNonzeroUv = hasNonzeroUv || vertex.uv0[0] != 0.0F ||
                                      vertex.uv0[1] != 0.0F;
  }
  return expect(walkway.ok() && walkway.asset.images.size() == 1U &&
                    walkway.asset.textureFailureCount == 0U,
                "embedded PNG decodes without rejecting the mesh") &&
         expect(walkway.asset.images[0].width == 4U &&
                    walkway.asset.images[0].height == 4U &&
                    walkway.asset.images[0].rgba8.size() == 64U,
                "decoded image is bounded RGBA8") &&
         expect(walkway.asset.materials[0].baseColorImageIndex == 0U &&
                    walkway.asset.primitives[0].hasTexcoord0,
                "material and primitive retain texture ownership") &&
         expect(textures.textures.size() == 8U &&
                    textures.materialBindings.size() == 83U &&
                    textures.rejectedTextureCount == 1U,
                "startup cache dedups palette copies to unique textures and "
                "rejects only the severed missing-texture probe") &&
         expect(geometry.ready && geometry.indexedDraws.size() == 1U &&
                    geometry.indexedDraws[0].materialTextureIndex <
                        textures.materialBindings.size() &&
                    hasNonzeroUv,
                "room draw carries texture binding and transformed UVs") &&
         expect(atlas.valid && walkwayDraw != atlas.assetDraws.end() &&
                    walkwayDraw->indexedDraws.size() == 1U &&
                    walkwayDraw->indexedDraws[0].materialTextureIndex <
                        textures.materialBindings.size() &&
                    instanced.ready && instanced.vertices.empty() &&
                    instanced.staticMeshInstances.size() == 1U &&
                    instanced.staticMeshInstanceBatches.size() == 1U &&
                    instanced.staticMeshInstanceBatches[0].materialTextureIndex ==
                        walkwayDraw->indexedDraws[0].materialTextureIndex,
                "instanced atlas retains textured primitive binding") &&
         expect(!invalid.ok() &&
                    invalid.reasonCode == "image_dimensions_invalid",
                "malformed image fails without an unsafe allocation");
}

bool importsBase64DataUriTexture() {
  constexpr std::string_view kGeometryBase64 =
      "AAAAvwAAAL8AAAC/AAAAPwAAAL8AAAC/AAAAPwAAAD8AAAC/AAAAvwAAAD8A"
      "AAC/AAAAvwAAAL8AAAA/AAAAPwAAAL8AAAA/AAAAPwAAAD8AAAA/AAAAvwAA"
      "AD8AAAA/AAAAAAAAAAAAAIA/AAAAAAAAgD8AAIA/AAAAAAAAgD8AAAAAAAAA"
      "AAAAgD8AAAAAAACAPwAAgD8AAAAAAACAPwAAAQACAAAAAgADAAQABgAFAAQABw"
      "AGAAAAAwAHAAAABwAEAAEABQAGAAEABgACAAMAAgAGAAMABgAHAAAABAAFAAAA"
      "BQABAA==";
  constexpr std::string_view kPngBase64 =
      "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP4"
      "z8DwHwAFAAH/VscvDQAAAABJRU5ErkJggg==";
  const std::string document =
      R"json({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"TEXCOORD_0":1},"indices":2,"material":0}]}],"materials":[{"pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}],"images":[{"uri":"data:image/png;base64,)json" +
      std::string(kPngBase64) +
      R"json("}],"textures":[{"source":0}],"buffers":[{"byteLength":232,"uri":"data:application/octet-stream;base64,)json" +
      std::string(kGeometryBase64) +
      R"json("}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":96,"target":34962},{"buffer":0,"byteOffset":96,"byteLength":64,"target":34962},{"buffer":0,"byteOffset":160,"byteLength":72,"target":34963}],"accessors":[{"bufferView":0,"componentType":5126,"count":8,"type":"VEC3","min":[-0.5,-0.5,-0.5],"max":[0.5,0.5,0.5]},{"bufferView":1,"componentType":5126,"count":8,"type":"VEC2","min":[0,0],"max":[1,1]},{"bufferView":2,"componentType":5123,"count":36,"type":"SCALAR","min":[0],"max":[7]}]})json";
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      "iggy3d_static_mesh_data_uri_test.gltf";
  {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << document;
  }
  const iggy3d::StaticMeshImportResult imported =
      iggy3d::importStaticMeshGlb(path, "data_uri_fixture");
  std::error_code removeError;
  std::filesystem::remove(path, removeError);
  return expect(imported.ok(), "base64 data URI mesh imports") &&
         expect(imported.asset.images.size() == 1U &&
                    imported.asset.images[0].source ==
                        "embedded://data_uri" &&
                    imported.asset.images[0].width == 1U &&
                    imported.asset.images[0].height == 1U,
                "base64 image decodes through bounded material importer") &&
         expect(imported.asset.materials.size() == 1U &&
                    imported.asset.materials[0].baseColorImageIndex == 0U &&
                    imported.asset.textureFailureCount == 0U,
                "base64 material retains its decoded image binding") &&
         expect(imported.asset.authoringMetadata.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::DefaultsApplied &&
                    imported.asset.authoringMetadata.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds &&
                    !imported.asset.authoringMetadata.walkable,
                "assets without extras receive explicit safe defaults");
}

bool importsMaterialVariantsAndRejectsDuplicateNames() {
  constexpr std::string_view kGeometryBase64 =
      "AAAAvwAAAL8AAAC/AAAAPwAAAL8AAAC/AAAAPwAAAD8AAAC/AAAAvwAAAD8A"
      "AAC/AAAAvwAAAL8AAAA/AAAAPwAAAL8AAAA/AAAAPwAAAD8AAAA/AAAAvwAA"
      "AD8AAAA/AAAAAAAAAAAAAIA/AAAAAAAAgD8AAIA/AAAAAAAAgD8AAAAAAAAA"
      "AAAAgD8AAAAAAACAPwAAgD8AAAAAAACAPwAAAQACAAAAAgADAAQABgAFAAQABw"
      "AGAAAAAwAHAAAABwAEAAEABQAGAAEABgACAAMAAgAGAAMABgAHAAAABAAFAAAA"
      "BQABAA==";
  const std::string prefix =
      R"json({"asset":{"version":"2.0"},"extensionsUsed":["KHR_materials_variants"],"extensions":{"KHR_materials_variants":{"variants":[{"name":"Oak"},{"name":")json";
  const std::string suffix =
      R"json("}]}},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0}],"meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1,"material":0,"extensions":{"KHR_materials_variants":{"mappings":[{"material":1,"variants":[0]},{"material":2,"variants":[1]}]}}}]}],"materials":[{"name":"Default","pbrMetallicRoughness":{"baseColorFactor":[0.5,0.5,0.5,1]}},{"name":"Oak Material","pbrMetallicRoughness":{"baseColorFactor":[0.4,0.2,0.1,1]}},{"name":"Painted Material","pbrMetallicRoughness":{"baseColorFactor":[0.1,0.3,0.8,1]}}],"buffers":[{"byteLength":232,"uri":"data:application/octet-stream;base64,)json" +
      std::string(kGeometryBase64) +
      R"json("}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":96,"target":34962},{"buffer":0,"byteOffset":160,"byteLength":72,"target":34963}],"accessors":[{"bufferView":0,"componentType":5126,"count":8,"type":"VEC3","min":[-0.5,-0.5,-0.5],"max":[0.5,0.5,0.5]},{"bufferView":1,"componentType":5123,"count":36,"type":"SCALAR","min":[0],"max":[7]}]})json";
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_static_mesh_material_variants_test";
  std::error_code removeError;
  std::filesystem::remove_all(root, removeError);
  removeError.clear();
  std::filesystem::create_directories(root, removeError);
  const std::filesystem::path validPath = root / "variant_fixture.glb";
  const std::filesystem::path duplicatePath =
      root / "duplicate_variant_fixture.gltf";
  const std::string validDocument = prefix + "Painted" + suffix;
  if (removeError || !writeJsonOnlyGlb(validPath, validDocument)) {
    std::filesystem::remove_all(root, removeError);
    return expect(false, "material variant GLB fixture written");
  }
  {
    std::ofstream output(duplicatePath, std::ios::binary | std::ios::trunc);
    output << prefix << "Oak" << suffix;
  }
  const iggy3d::StaticMeshImportResult imported =
      iggy3d::importStaticMeshGlb(validPath, "variant_fixture");
  const iggy3d::StaticMeshImportResult duplicate =
      iggy3d::importStaticMeshGlb(duplicatePath, "duplicate_variant_fixture");
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot(root);
  const iggy3d::vulkan::StaticMeshAssetAtlasCpuGeometry atlas =
      iggy3d::vulkan::buildStaticMeshAssetAtlasCpuGeometry(&cache);
  const auto assetDraw = std::find_if(
      atlas.assetDraws.begin(), atlas.assetDraws.end(),
      [](const iggy3d::vulkan::StaticMeshAssetDrawRanges& candidate) {
        return candidate.assetId == "variant_fixture";
      });
  iggy3d::SceneRoomProjection room = roomWith("asset:variant_fixture");
  room.meshes.front().materialVariant = "Painted";
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr,
                                               atlas.assetDraws);
  const iggy3d::vulkan::StaticMeshMaterialVariantDrawRanges* paintedDraw =
      assetDraw != atlas.assetDraws.end() &&
              assetDraw->materialVariants.size() == 2U
          ? &assetDraw->materialVariants[1]
          : nullptr;
  const iggy3d::vulkan::StaticMeshInstanceBatch* paintedBatch =
      geometry.staticMeshInstanceBatches.size() == 1U
          ? &geometry.staticMeshInstanceBatches.front()
          : nullptr;
  const bool rendererUsesPaintedVariant =
      assetDraw != atlas.assetDraws.end() && paintedDraw != nullptr &&
      assetDraw->indexedDraws.size() == 1U &&
      paintedDraw->indexedDraws.size() == 1U && paintedBatch != nullptr &&
      paintedDraw->indexedDraws[0].firstIndex !=
          assetDraw->indexedDraws[0].firstIndex &&
      paintedBatch->firstIndex == paintedDraw->indexedDraws[0].firstIndex &&
      paintedDraw->indexedDraws[0].firstIndex < atlas.indices.size() &&
      assetDraw->indexedDraws[0].firstIndex < atlas.indices.size() &&
      atlas.indices[paintedDraw->indexedDraws[0].firstIndex] <
          atlas.vertices.size() &&
      atlas.indices[assetDraw->indexedDraws[0].firstIndex] <
          atlas.vertices.size() &&
      near(atlas.vertices[atlas.indices[paintedDraw->indexedDraws[0].firstIndex]]
                   .baseColor[2],
           0.8F) &&
      near(atlas.vertices[atlas.indices[assetDraw->indexedDraws[0].firstIndex]]
                   .baseColor[2],
           0.5F);
  std::filesystem::remove_all(root, removeError);

  return expect(imported.ok() && imported.asset.materialVariants.size() == 2U,
                "material variant names import") &&
         expect(imported.asset.materialVariants[0].name == "Oak" &&
                    imported.asset.materialVariants[1].name == "Painted",
                "material variant order stays authored") &&
         expect(imported.asset.primitives.size() == 1U &&
                    imported.asset.primitives[0].materialIndex == 0U &&
                    imported.asset.primitives[0].variantMaterialIndices ==
                        std::vector<std::uint32_t>{1U, 2U},
                "primitive variant mappings resolve to materials") &&
         expect(atlas.valid && rendererUsesPaintedVariant,
                "selected material variant owns the instanced renderer draw") &&
         expect(!duplicate.ok() &&
                    duplicate.status ==
                        iggy3d::StaticMeshImportStatus::ValidationFailed &&
                    duplicate.reasonCode ==
                        "static_mesh_material_variants_invalid",
                "duplicate material variant names fail closed");
}

bool authoringMetadataKernelIsBoundedAndFailClosed() {
  const std::array<std::string_view, 1> authored{
      R"json({"iggy_collision":"bounds","iggy_walkable":true,"iggy_category":"walkway","ignored":{"nested":[1,true,null]}})json"};
  const std::array<std::string_view, 1> decor{
      R"json({"iggy_collision":"none","iggy_walkable":false})json"};
  const std::array<std::string_view, 1> unsupported{
      R"json({"iggy_collision":"convex"})json"};
  const std::array<std::string_view, 1> compound{
      R"json({"iggy_collision":"compound_bounds","iggy_category":"stairs"})json"};
  const std::array<std::string_view, 1> compoundGlobalWalkable{
      R"json({"iggy_collision":"compound_bounds","iggy_walkable":true})json"};
  const std::array<std::string_view, 2> conflict{
      R"json({"iggy_collision":"bounds"})json",
      R"json({"iggy_collision":"none"})json"};
  const std::array<std::string_view, 1> impossible{
      R"json({"iggy_collision":"none","iggy_walkable":true})json"};
  const std::array<std::string_view, 1> duplicate{
      R"json({"iggy_collision":"bounds","iggy_collision":"bounds"})json"};
  const std::array<std::string_view, 1> malformedUnknown{
      R"json({"ignored":wat})json"};
  const std::string oversized(4097U, 'x');
  const std::array<std::string_view, 1> oversizedInput{oversized};
  const std::vector<std::string_view> tooMany(257U, "{}");

  const iggy3d::StaticMeshAuthoringMetadata defaults =
      iggy3d::detail::parseStaticMeshAuthoringMetadata({});
  const iggy3d::StaticMeshAuthoringMetadata walkable =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(authored);
  const iggy3d::StaticMeshAuthoringMetadata noCollision =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(decor);
  const iggy3d::StaticMeshAuthoringMetadata deferred =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(unsupported);
  const iggy3d::StaticMeshAuthoringMetadata compoundBounds =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(compound);
  const iggy3d::StaticMeshAuthoringMetadata invalidCompoundWalkable =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(compoundGlobalWalkable);
  const iggy3d::StaticMeshAuthoringMetadata conflicting =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(conflict);
  const iggy3d::StaticMeshAuthoringMetadata invalidPair =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(impossible);
  const iggy3d::StaticMeshAuthoringMetadata tooLarge =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(oversizedInput);
  const iggy3d::StaticMeshAuthoringMetadata duplicateKey =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(duplicate);
  const iggy3d::StaticMeshAuthoringMetadata malformed =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(malformedUnknown);
  const iggy3d::StaticMeshAuthoringMetadata tooManyObjects =
      iggy3d::detail::parseStaticMeshAuthoringMetadata(tooMany);

  return expect(defaults.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::DefaultsApplied &&
                    defaults.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Bounds,
                "empty metadata has explicit bounds defaults") &&
         expect(walkable.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::Authored &&
                    walkable.walkable && walkable.categoryId == "walkway",
                "authored bounds and walkable metadata parse") &&
         expect(noCollision.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::Authored &&
                    noCollision.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::None &&
                    !noCollision.walkable,
                "render-only metadata parses") &&
         expect(deferred.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::
                        UnsupportedCollision &&
                    deferred.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::Convex,
                "deferred collision mode remains visible") &&
         expect(compoundBounds.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Authored &&
                    compoundBounds.collisionMode ==
                        iggy3d::StaticMeshCollisionMode::CompoundBounds &&
                    compoundBounds.categoryId == "stairs" &&
                    iggy3d::toString(compoundBounds.collisionMode) ==
                        "compound_bounds",
                "compound bounds metadata is explicit") &&
         expect(conflicting.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    invalidPair.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    invalidCompoundWalkable.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    tooLarge.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    duplicateKey.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    malformed.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    tooManyObjects.status ==
                        iggy3d::StaticMeshAuthoringMetadataStatus::Invalid,
                "conflicts and bounded malformed extras fail closed");
}

bool collisionPartMetadataKernelIsBoundedAndFailClosed() {
  const iggy3d::StaticMeshCollisionPartMetadata absent =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(
          R"json({"iggy_category":"stairs"})json");
  const iggy3d::StaticMeshCollisionPartMetadata solid =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(
          R"json({"iggy_collision_part":"bounds"})json");
  const iggy3d::StaticMeshCollisionPartMetadata walkable =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(
          R"json({"iggy_collision":"compound_bounds","iggy_collision_part":"bounds","iggy_collision_part_walkable":true})json");
  const iggy3d::StaticMeshCollisionPartMetadata orphanWalkable =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(
          R"json({"iggy_collision_part_walkable":true})json");
  const iggy3d::StaticMeshCollisionPartMetadata unsupportedKind =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(
          R"json({"iggy_collision_part":"mesh"})json");
  const iggy3d::StaticMeshCollisionPartMetadata duplicate =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(
          R"json({"iggy_collision_part":"bounds","iggy_collision_part":"bounds"})json");
  const std::string oversized(4097U, 'x');
  const iggy3d::StaticMeshCollisionPartMetadata tooLarge =
      iggy3d::detail::parseStaticMeshCollisionPartMetadata(oversized);

  return expect(absent.status ==
                    iggy3d::StaticMeshCollisionPartMetadataStatus::NotAuthored,
                "unrelated node extras do not invent collision parts") &&
         expect(
             solid.status ==
                     iggy3d::StaticMeshCollisionPartMetadataStatus::Authored &&
                 !solid.walkable,
             "solid collision part parses") &&
         expect(
             walkable.status ==
                     iggy3d::StaticMeshCollisionPartMetadataStatus::Authored &&
                 walkable.walkable,
             "walkable collision part parses independently") &&
         expect(
             orphanWalkable.status ==
                     iggy3d::StaticMeshCollisionPartMetadataStatus::Invalid &&
                 unsupportedKind.status ==
                     iggy3d::StaticMeshCollisionPartMetadataStatus::Invalid &&
                 duplicate.status ==
                     iggy3d::StaticMeshCollisionPartMetadataStatus::Invalid &&
                 tooLarge.status ==
                     iggy3d::StaticMeshCollisionPartMetadataStatus::Invalid,
             "partial, unsupported, duplicate, and oversized parts fail "
             "closed");
}

bool attachmentSocketMetadataKernelIsBoundedAndFailClosed() {
  const iggy3d::StaticMeshAttachmentSocketMetadata absent =
      iggy3d::detail::parseStaticMeshAttachmentSocketMetadata(
          R"json({"iggy_category":"door"})json");
  const iggy3d::StaticMeshAttachmentSocketMetadata receiver =
      iggy3d::detail::parseStaticMeshAttachmentSocketMetadata(
          R"json({"iggy_socket":"door_frame","iggy_socket_role":"receiver","iggy_socket_compatibility":"door.frame"})json");
  const iggy3d::StaticMeshAttachmentSocketMetadata plug =
      iggy3d::detail::parseStaticMeshAttachmentSocketMetadata(
          R"json({"iggy_socket":"door_leaf","iggy_socket_role":"plug","iggy_socket_compatibility":"door.frame"})json");
  const iggy3d::StaticMeshAttachmentSocketMetadata partial =
      iggy3d::detail::parseStaticMeshAttachmentSocketMetadata(
          R"json({"iggy_socket":"orphan"})json");
  const iggy3d::StaticMeshAttachmentSocketMetadata invalidRole =
      iggy3d::detail::parseStaticMeshAttachmentSocketMetadata(
          R"json({"iggy_socket":"door","iggy_socket_role":"hinge","iggy_socket_compatibility":"door.frame"})json");
  const iggy3d::StaticMeshAttachmentSocketMetadata invalidName =
      iggy3d::detail::parseStaticMeshAttachmentSocketMetadata(
          R"json({"iggy_socket":"bad socket","iggy_socket_role":"plug","iggy_socket_compatibility":"door.frame"})json");

  return expect(
             absent.status ==
                 iggy3d::StaticMeshAttachmentSocketMetadataStatus::NotAuthored,
             "unrelated extras do not invent sockets") &&
         expect(receiver.status ==
                        iggy3d::StaticMeshAttachmentSocketMetadataStatus::Authored &&
                    receiver.role ==
                        iggy3d::StaticMeshAttachmentSocketRole::Receiver &&
                    receiver.name == "door_frame" &&
                    receiver.compatibility == "door.frame" &&
                    plug.status ==
                        iggy3d::StaticMeshAttachmentSocketMetadataStatus::Authored &&
                    plug.role ==
                        iggy3d::StaticMeshAttachmentSocketRole::Plug,
                "receiver and plug metadata parse") &&
         expect(partial.status ==
                        iggy3d::StaticMeshAttachmentSocketMetadataStatus::Invalid &&
                    invalidRole.status ==
                        iggy3d::StaticMeshAttachmentSocketMetadataStatus::Invalid &&
                    invalidName.status ==
                        iggy3d::StaticMeshAttachmentSocketMetadataStatus::Invalid,
                "partial and malformed sockets fail closed");
}

bool thumbnailKernelIsDeterministicBoundedAndGeometryBacked() {
  const iggy3d::StaticMeshImportResult imported = iggy3d::importStaticMeshGlb(
      "assets/creative/boulder_01.glb", "boulder_01");
  if (!expect(imported.ok(), "thumbnail fixture imports")) {
    return false;
  }
  const iggy3d::StaticMeshAssetThumbnail first =
      iggy3d::buildStaticMeshAssetThumbnail(imported.asset);
  const iggy3d::StaticMeshAssetThumbnail second =
      iggy3d::buildStaticMeshAssetThumbnail(imported.asset);
  const bool pixelsEqual = std::equal(
      first.pixels.begin(), first.pixels.end(), second.pixels.begin(),
      [](iggy3d::StaticMeshThumbnailPixel lhs,
         iggy3d::StaticMeshThumbnailPixel rhs) {
        return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b &&
               lhs.a == rhs.a;
      });
  const bool hasBoundsInk = std::any_of(
      first.pixels.begin(), first.pixels.end(),
      [](iggy3d::StaticMeshThumbnailPixel pixel) {
        return pixel.a == 255U && pixel.b > pixel.r && pixel.g > pixel.r;
      });
  const iggy3d::StaticMeshAssetThumbnail empty =
      iggy3d::buildStaticMeshAssetThumbnail({});
  return expect(first.valid && first.coveredPixelCount > 40U &&
                    first.coveredPixelCount <=
                        iggy3d::kStaticMeshThumbnailPixelCount,
                "thumbnail contains bounded geometry-backed pixels") &&
         expect(pixelsEqual &&
                    first.coveredPixelCount == second.coveredPixelCount,
                "thumbnail generation is deterministic") &&
         expect(hasBoundsInk,
                "thumbnail overlays the exact imported bounds") &&
         expect(!empty.valid && empty.coveredPixelCount == 0U,
                "invalid geometry does not fabricate a thumbnail");
}

}  // namespace

int main() {
  const bool ok = importsBoulderFixture() &&
                  rejectsUnsafeAndMissingAssetIds() &&
                  cacheReusesImportAndRendererEmitsIrregularGeometry() &&
                  repeatedAssetsUseOneAtlasMeshAndCompactTransforms() &&
                  instanceTransformRejectsInvalidGeometry() &&
                  missingAssetIsVisibleAndMemoized() &&
                  windowMaterialsDriveDistinctGeneratedColors() &&
                  generatedSlabsPreserveVolumeMaterialAndSemanticRole() &&
                  externalFloorAndWallBypassGeneratedBatching() &&
                  generatedTraversalProfilesEmitBoundedGeometry() &&
                  hipRoofProfileEmitsTaperedPanelGeometry() &&
                  discoveryAndPreviewAtlasCoverEveryValidFixture() &&
                  texturedFixtureBuildsOneCachedMaterialBinding() &&
                  importsBase64DataUriTexture() &&
                  importsMaterialVariantsAndRejectsDuplicateNames() &&
                  authoringMetadataKernelIsBoundedAndFailClosed() &&
                  collisionPartMetadataKernelIsBoundedAndFailClosed() &&
                  attachmentSocketMetadataKernelIsBoundedAndFailClosed() &&
                  thumbnailKernelIsDeterministicBoundedAndGeometryBacked();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: static mesh assets\n";
  return 0;
}
