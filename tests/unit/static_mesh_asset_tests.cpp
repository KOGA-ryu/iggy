#include "content/assets/StaticMeshAsset.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

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

bool missingAssetIsVisibleAndMemoized() {
  iggy3d::StaticMeshAssetCache cache;
  cache.setRoot("assets/creative");
  const iggy3d::SceneRoomProjection room = roomWith("asset:not_here");
  const iggy3d::vulkan::RoomMeshCpuGeometry first =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr, &cache);
  const iggy3d::vulkan::RoomMeshCpuGeometry second =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room, nullptr, &cache);
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
                "missing asset reason retained");
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

}  // namespace

int main() {
  const bool ok = importsBoulderFixture() &&
                  rejectsUnsafeAndMissingAssetIds() &&
                  cacheReusesImportAndRendererEmitsIrregularGeometry() &&
                  missingAssetIsVisibleAndMemoized() &&
                  externalFloorAndWallBypassGeneratedBatching();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: static mesh assets\n";
  return 0;
}
