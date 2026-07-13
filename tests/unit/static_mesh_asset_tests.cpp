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

bool discoveryAndPreviewAtlasCoverEveryValidFixture() {
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

  return expect(catalog.failures.empty() && catalog.entries.size() == 2U,
                "catalog discovers both valid GLB fixtures") &&
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
                    catalog.find("walkway_stone_01") == &*walkway &&
                    catalog.find("missing") == nullptr,
                "discovery retains labels, bounds, metadata, and lookup") &&
         expect(missing.entries.empty() && missing.failures.size() == 1U &&
                    missing.failures[0].reasonCode ==
                        "static_mesh_asset_root_not_directory",
                "missing catalog root reports one explicit failure") &&
         expect(preview.ready && preview.assetDraws.size() == 2U &&
                    preview.indexedDraws.size() ==
                        iggy3d::vulkan::kCreativePreviewGeometryDrawRangeCount +
                            2U * iggy3d::kRenderCreativePreviewRoleCount,
                "startup atlas contains three colored roles per asset") &&
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
         expect(textures.textures.size() == 1U &&
                    textures.materialBindings.size() == 1U &&
                    textures.rejectedTextureCount == 0U,
                "startup cache deduplicates one texture binding") &&
         expect(geometry.ready && geometry.indexedDraws.size() == 1U &&
                    geometry.indexedDraws[0].materialTextureIndex == 0U &&
                    hasNonzeroUv,
                "room draw carries texture binding and transformed UVs") &&
         expect(atlas.valid && walkwayDraw != atlas.assetDraws.end() &&
                    walkwayDraw->indexedDraws.size() == 1U &&
                    walkwayDraw->indexedDraws[0].materialTextureIndex == 0U &&
                    instanced.ready && instanced.vertices.empty() &&
                    instanced.staticMeshInstances.size() == 1U &&
                    instanced.staticMeshInstanceBatches.size() == 1U &&
                    instanced.staticMeshInstanceBatches[0]
                            .materialTextureIndex == 0U,
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

bool authoringMetadataKernelIsBoundedAndFailClosed() {
  const std::array<std::string_view, 1> authored{
      R"json({"iggy_collision":"bounds","iggy_walkable":true,"iggy_category":"walkway","ignored":{"nested":[1,true,null]}})json"};
  const std::array<std::string_view, 1> decor{
      R"json({"iggy_collision":"none","iggy_walkable":false})json"};
  const std::array<std::string_view, 1> unsupported{
      R"json({"iggy_collision":"convex"})json"};
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
         expect(conflicting.status ==
                    iggy3d::StaticMeshAuthoringMetadataStatus::Invalid &&
                    invalidPair.status ==
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

}  // namespace

int main() {
  const bool ok = importsBoulderFixture() &&
                  rejectsUnsafeAndMissingAssetIds() &&
                  cacheReusesImportAndRendererEmitsIrregularGeometry() &&
                  repeatedAssetsUseOneAtlasMeshAndCompactTransforms() &&
                  instanceTransformRejectsInvalidGeometry() &&
                  missingAssetIsVisibleAndMemoized() &&
                  externalFloorAndWallBypassGeneratedBatching() &&
                  discoveryAndPreviewAtlasCoverEveryValidFixture() &&
                  texturedFixtureBuildsOneCachedMaterialBinding() &&
                  importsBase64DataUriTexture() &&
                  authoringMetadataKernelIsBoundedAndFailClosed();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: static mesh assets\n";
  return 0;
}
