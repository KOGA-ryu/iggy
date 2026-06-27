#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToRoomAsset.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/session/SessionState.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::abs(lhs - rhs) < 0.0001F;
}

iggy3d::AsciiRoomToRoomAssetResult buildAsciiRoomAsset(std::string_view text,
                                                       std::string_view roomId) {
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(text, "tests/render_room_mesh_geometry.iggyroom.txt");
  const iggy3d::AsciiRoomGridBuildResult grid =
      iggy3d::buildAsciiRoomGrid(source);
  iggy3d::AsciiRoomCompileConfig compileConfig;
  compileConfig.roomId = std::string(roomId);
  compileConfig.sourceName = "tests/render_room_mesh_geometry.iggyroom.txt";
  compileConfig.tileSizeMeters = 1.0F;
  compileConfig.floorThicknessMeters = 0.10F;
  compileConfig.wallHeightMeters = 2.50F;
  compileConfig.wallThicknessMeters = 1.0F;
  compileConfig.centerOnOrigin = true;
  const iggy3d::AsciiRoomAuthoredRoomResult authored =
      iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid, compileConfig);

  iggy3d::AsciiRoomToRoomAssetConfig assetConfig;
  assetConfig.roomId = std::string(roomId);
  assetConfig.sourceName = "tests/render_room_mesh_geometry.iggyroom.txt";
  return iggy3d::buildRoomAssetFromAsciiRoom(authored, assetConfig);
}

std::size_t countProjectedRole(const iggy3d::SceneRoomProjection& room,
                               std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

bool vertexHasColor(const iggy3d::vulkan::FirstRoomVertex& vertex,
                    float red,
                    float green,
                    float blue) {
  return near(vertex.color[0], red) && near(vertex.color[1], green) &&
         near(vertex.color[2], blue);
}

std::size_t countVerticesWithColor(
    const std::vector<iggy3d::vulkan::FirstRoomVertex>& vertices,
    float red,
    float green,
    float blue) {
  std::size_t count = 0;
  for (const iggy3d::vulkan::FirstRoomVertex& vertex : vertices) {
    if (vertexHasColor(vertex, red, green, blue)) {
      ++count;
    }
  }
  return count;
}

struct VertexBounds {
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();
  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();
};

VertexBounds boundsForVertexRange(
    const std::vector<iggy3d::vulkan::FirstRoomVertex>& vertices,
    std::size_t first,
    std::size_t count) {
  VertexBounds bounds;
  const std::size_t end = std::min(vertices.size(), first + count);
  for (std::size_t index = first; index < end; ++index) {
    bounds.minX = std::min(bounds.minX, vertices[index].position[0]);
    bounds.maxX = std::max(bounds.maxX, vertices[index].position[0]);
    bounds.minY = std::min(bounds.minY, vertices[index].position[1]);
    bounds.maxY = std::max(bounds.maxY, vertices[index].position[1]);
    bounds.minZ = std::min(bounds.minZ, vertices[index].position[2]);
    bounds.maxZ = std::max(bounds.maxZ, vertices[index].position[2]);
  }
  return bounds;
}

iggy3d::SceneRoomMeshItem floorMesh(std::string id,
                                    float x,
                                    float y,
                                    float z,
                                    std::string materialId = "debug_floor",
                                    iggy3d::Vec3 size = {1.0F, 0.10F, 1.0F}) {
  iggy3d::SceneRoomMeshItem mesh;
  mesh.id = std::move(id);
  mesh.role = "floor";
  mesh.materialId = std::move(materialId);
  mesh.position = {x, y, z};
  mesh.size = size;
  return mesh;
}

iggy3d::SceneRoomMeshItem wallMesh(std::string id,
                                   float x,
                                   float y,
                                   float z,
                                   std::string materialId = "debug_wall",
                                   iggy3d::Vec3 size = {1.0F, 2.5F, 1.0F}) {
  iggy3d::SceneRoomMeshItem mesh;
  mesh.id = std::move(id);
  mesh.role = "wall";
  mesh.materialId = std::move(materialId);
  mesh.position = {x, y, z};
  mesh.size = size;
  return mesh;
}

iggy3d::SceneRoomMeshItem wallSegmentMesh(std::string id,
                                          iggy3d::Vec3 start,
                                          iggy3d::Vec3 end,
                                          float bottomY,
                                          float height,
                                          float thickness,
                                          std::string materialId = "debug_wall") {
  iggy3d::SceneRoomMeshItem mesh;
  mesh.id = std::move(id);
  mesh.role = "wall";
  mesh.materialId = std::move(materialId);
  mesh.position = {(start.x + end.x) * 0.5F, bottomY + height * 0.5F,
                   (start.z + end.z) * 0.5F};
  mesh.size = {std::sqrt((end.x - start.x) * (end.x - start.x) +
                         (end.y - start.y) * (end.y - start.y) +
                         (end.z - start.z) * (end.z - start.z)),
               height,
               thickness};
  mesh.hasWallSegment = true;
  mesh.wallStartMeters = start;
  mesh.wallEndMeters = end;
  mesh.wallBottomY = bottomY;
  mesh.wallHeightMeters = height;
  mesh.wallThicknessMeters = thickness;
  return mesh;
}

iggy3d::SceneRoomProjection roomProjection(std::vector<iggy3d::SceneRoomMeshItem> meshes) {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.assetId = "manual_room";
  room.version = 1;
  room.staticMeshCount = meshes.size();
  room.meshes = std::move(meshes);
  return room;
}

bool asciiFloorsAndWallsBuildVulkanRoomGeometry() {
  const iggy3d::AsciiRoomToRoomAssetResult asset = buildAsciiRoomAsset(
      "###\n"
      "#P#\n"
      "###\n",
      "vulkan_ascii_room");
  iggy3d::SessionState state;
  const iggy3d::SceneProjectionResult projection =
      iggy3d::buildSceneProjection(state, &asset.room);
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(projection.room);

  constexpr std::size_t kFloorCount = 1U;
  constexpr std::size_t kSourceWallCount = 8U;
  constexpr std::size_t kOptimizedWallDrawCount = 4U;
  constexpr std::size_t kMeshCount = kFloorCount + kSourceWallCount;
  constexpr std::size_t kFloorGridLineCount = kFloorCount * 4U;
  constexpr std::size_t kWallGridLineCount = kSourceWallCount * 8U;
  constexpr std::size_t kGridLineCount = kFloorGridLineCount + kWallGridLineCount;
  constexpr std::size_t kDrawCount =
      kFloorCount + kOptimizedWallDrawCount + kGridLineCount;
  constexpr std::size_t kFloorPlaneVertexCount = 4U;
  constexpr std::size_t kFloorPlaneIndexCount = 12U;
  constexpr std::size_t kBoxVertexCount = 8U;
  constexpr std::size_t kBoxIndexCount = 72U;
  constexpr std::size_t kBoxDrawCount = kOptimizedWallDrawCount + kGridLineCount;

  bool ok = true;
  ok = expect(asset.ok, "ascii room asset ok") && ok;
  ok = expect(projection.room.loaded, "room projection loaded") && ok;
  ok = expect(projection.room.assetId == "vulkan_ascii_room",
              "projection room asset id") &&
       ok;
  ok = expect(projection.room.floorVisible, "floor visible") && ok;
  ok = expect(projection.room.wallVisible, "wall visible") && ok;
  ok = expect(countProjectedRole(projection.room, "floor") == kFloorCount,
              "projected floor count") &&
       ok;
  ok = expect(countProjectedRole(projection.room, "wall") == kSourceWallCount,
              "projected wall count") &&
       ok;
  ok = expect(geometry.ready, "room mesh geometry ready") && ok;
  ok = expect(geometry.sourceRoomAssetId == "vulkan_ascii_room",
              "geometry room asset id") &&
       ok;
  ok = expect(geometry.sourceRoomStaticMeshCount == kMeshCount,
              "geometry source mesh count") &&
       ok;
  ok = expect(geometry.roomFloorDrawCount == kFloorCount,
              "geometry floor draw count") &&
       ok;
  ok = expect(geometry.roomWallDrawCount == kOptimizedWallDrawCount,
              "geometry wall draw count") &&
       ok;
  ok = expect(geometry.roomGridLineDrawCount == kGridLineCount,
              "geometry grid line draw count") &&
       ok;
  ok = expect(geometry.roomGridVisible, "geometry grid visible") && ok;
  ok = expect(!geometry.roomGridTruncated, "geometry grid not truncated") && ok;
  ok = expect(geometry.sourceRoomGeometrySignature != 0U,
              "geometry signature present") &&
       ok;
  ok = expect(geometry.vertices.size() ==
                  kFloorCount * kFloorPlaneVertexCount +
                      kBoxDrawCount * kBoxVertexCount,
              "geometry vertex count") &&
       ok;
  ok = expect(geometry.indices.size() ==
                  kFloorCount * kFloorPlaneIndexCount +
                      kBoxDrawCount * kBoxIndexCount,
              "geometry index count") &&
       ok;
  ok = expect(geometry.indexedDraws.size() == kDrawCount,
              "geometry draw count") &&
       ok;
  for (std::size_t i = 0; i < geometry.indexedDraws.size(); ++i) {
    const std::uint32_t expectedFirst =
        i == 0U ? 0U
                : static_cast<std::uint32_t>(kFloorPlaneIndexCount +
                                             (i - 1U) * kBoxIndexCount);
    const std::uint32_t expectedCount =
        i == 0U ? kFloorPlaneIndexCount : kBoxIndexCount;
    ok = expect(geometry.indexedDraws[i].firstIndex == expectedFirst,
                "draw first index") &&
         ok;
    ok = expect(geometry.indexedDraws[i].indexCount == expectedCount,
                "draw index count") &&
         ok;
  }
  ok = expect(countVerticesWithColor(geometry.vertices, 0.30F, 0.32F, 0.34F) ==
                  kFloorCount * kFloorPlaneVertexCount,
              "floor vertex color count") &&
       ok;
  ok = expect(countVerticesWithColor(geometry.vertices, 0.42F, 0.43F, 0.46F) ==
                  kOptimizedWallDrawCount * kBoxVertexCount,
              "wall vertex color count") &&
       ok;
  ok = expect(countVerticesWithColor(geometry.vertices, 0.78F, 0.82F, 0.86F) ==
                  kGridLineCount * kBoxVertexCount,
              "grid vertex color count") &&
       ok;
  return ok;
}

bool compatibleFloorBlockMergesButPreservesSourceGrid() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      floorMesh("floor_1", 0.0F, -0.05F, 0.0F),
      floorMesh("floor_2", 1.0F, -0.05F, 0.0F),
      floorMesh("floor_3", 0.0F, -0.05F, 1.0F),
      floorMesh("floor_4", 1.0F, -0.05F, 1.0F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  constexpr std::size_t kSourceFloorCount = 4U;
  constexpr std::size_t kOptimizedFloorDrawCount = 1U;
  constexpr std::size_t kFloorGridLineCount = kSourceFloorCount * 4U;
  constexpr std::size_t kOptimizedDrawCount =
      kOptimizedFloorDrawCount + kFloorGridLineCount;
  constexpr std::size_t kNaiveFloorDrawCount = kSourceFloorCount + kFloorGridLineCount;
  constexpr std::size_t kPlaneVertexCount = 4U;
  constexpr std::size_t kPlaneIndexCount = 12U;
  constexpr std::size_t kBoxVertexCount = 8U;
  constexpr std::size_t kBoxIndexCount = 72U;
  constexpr std::size_t kOptimizedVertexCount =
      kPlaneVertexCount + kFloorGridLineCount * kBoxVertexCount;
  constexpr std::size_t kOptimizedIndexCount =
      kPlaneIndexCount + kFloorGridLineCount * kBoxIndexCount;
  constexpr std::size_t kNaiveVertexCount = kNaiveFloorDrawCount * kBoxVertexCount;
  constexpr std::size_t kNaiveIndexCount = kNaiveFloorDrawCount * kBoxIndexCount;

  bool ok = true;
  ok = expect(geometry.ready, "2x2 floor geometry ready") && ok;
  ok = expect(geometry.sourceRoomStaticMeshCount == kSourceFloorCount,
              "2x2 source mesh count preserved") &&
       ok;
  ok = expect(geometry.roomFloorDrawCount == kOptimizedFloorDrawCount,
              "2x2 floors merge to one draw") &&
       ok;
  ok = expect(geometry.roomGridLineDrawCount == kFloorGridLineCount,
              "2x2 source grid lines preserved") &&
       ok;
  ok = expect(geometry.indexedDraws.size() == kOptimizedDrawCount,
              "2x2 optimized draw count") &&
       ok;
  ok = expect(geometry.vertices.size() == kOptimizedVertexCount,
              "2x2 optimized vertex count") &&
       ok;
  ok = expect(geometry.indices.size() == kOptimizedIndexCount,
              "2x2 optimized index count") &&
       ok;
  ok = expect(geometry.vertices.size() < kNaiveVertexCount,
              "2x2 fewer vertices than naive floor boxes") &&
       ok;
  ok = expect(geometry.indices.size() < kNaiveIndexCount,
              "2x2 fewer indices than naive floor boxes") &&
       ok;
  ok = expect(geometry.indexedDraws.front().indexCount == kPlaneIndexCount,
              "2x2 merged floor emits plane") &&
       ok;
  return ok;
}

bool nonAdjacentFloorsDoNotMerge() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      floorMesh("floor_1", 0.0F, -0.05F, 0.0F),
      floorMesh("floor_2", 2.0F, -0.05F, 0.0F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready, "non-adjacent geometry ready") &&
         expect(geometry.sourceRoomStaticMeshCount == 2U,
                "non-adjacent source count") &&
         expect(geometry.roomFloorDrawCount == 2U,
                "non-adjacent floors do not merge") &&
         expect(geometry.roomGridLineDrawCount == 8U,
                "non-adjacent grid count");
}

bool mismatchedFloorMaterialDoesNotMergeAndAffectsSignature() {
  iggy3d::SceneRoomProjection sameMaterial = roomProjection({
      floorMesh("floor_1", 0.0F, -0.05F, 0.0F),
      floorMesh("floor_2", 1.0F, -0.05F, 0.0F),
  });
  iggy3d::SceneRoomProjection mixedMaterial = roomProjection({
      floorMesh("floor_1", 0.0F, -0.05F, 0.0F),
      floorMesh("floor_2", 1.0F, -0.05F, 0.0F, "painted_floor"),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry sameGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(sameMaterial);
  const iggy3d::vulkan::RoomMeshCpuGeometry mixedGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(mixedMaterial);
  return expect(sameGeometry.ready, "same material geometry ready") &&
         expect(mixedGeometry.ready, "mixed material geometry ready") &&
         expect(sameGeometry.roomFloorDrawCount == 1U,
                "same material floors merge") &&
         expect(mixedGeometry.roomFloorDrawCount == 2U,
                "mismatched material floors do not merge") &&
         expect(sameGeometry.sourceRoomGeometrySignature !=
                    mixedGeometry.sourceRoomGeometrySignature,
                "material changes geometry signature");
}

bool mismatchedFloorYOrSizeDoesNotMerge() {
  const iggy3d::SceneRoomProjection yMismatch = roomProjection({
      floorMesh("floor_1", 0.0F, -0.05F, 0.0F),
      floorMesh("floor_2", 1.0F, 0.00F, 0.0F),
  });
  const iggy3d::SceneRoomProjection sizeMismatch = roomProjection({
      floorMesh("floor_1", 0.0F, -0.05F, 0.0F),
      floorMesh("floor_2", 1.0F, -0.05F, 0.0F, "debug_floor",
                {2.0F, 0.10F, 1.0F}),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry yGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(yMismatch);
  const iggy3d::vulkan::RoomMeshCpuGeometry sizeGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(sizeMismatch);
  return expect(yGeometry.ready, "floor y mismatch geometry ready") &&
         expect(sizeGeometry.ready, "floor size mismatch geometry ready") &&
         expect(yGeometry.roomFloorDrawCount == 2U,
                "floor y mismatch does not merge") &&
         expect(sizeGeometry.roomFloorDrawCount == 2U,
                "floor size mismatch does not merge");
}

bool wallsRemainUnmerged() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallMesh("wall_1", 0.5F, 1.25F, 0.0F),
      wallMesh("wall_2", 1.5F, 1.25F, 0.0F),
      wallMesh("wall_3", 2.5F, 1.25F, 0.0F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready, "wall run geometry ready") &&
         expect(geometry.sourceRoomStaticMeshCount == 3U,
                "wall run source mesh count") &&
         expect(geometry.roomWallDrawCount == 3U,
                "walls remain unmerged") &&
         expect(geometry.roomGridLineDrawCount == 24U,
                "wall grid remains per source wall");
}

bool compatibleXWallSegmentsMergeButPreserveSourceGrid() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_3", {2.0F, 0.0F, 0.0F}, {3.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  const VertexBounds bounds = boundsForVertexRange(geometry.vertices, 0U, 8U);

  constexpr std::size_t kSourceWallCount = 3U;
  constexpr std::size_t kMergedWallDrawCount = 1U;
  constexpr std::size_t kWallGridLineCount = kSourceWallCount * 8U;
  constexpr std::size_t kBoxVertexCount = 8U;
  constexpr std::size_t kBoxIndexCount = 72U;
  constexpr std::size_t kOptimizedDrawCount = kMergedWallDrawCount + kWallGridLineCount;
  constexpr std::size_t kNaiveDrawCount = kSourceWallCount + kWallGridLineCount;

  bool ok = true;
  ok = expect(geometry.ready, "x run merge geometry ready") && ok;
  ok = expect(geometry.sourceRoomStaticMeshCount == kSourceWallCount,
              "x run source count preserved") &&
       ok;
  ok = expect(geometry.roomWallDrawCount == kMergedWallDrawCount,
              "x run wall draw merged") &&
       ok;
  ok = expect(geometry.roomGridLineDrawCount == kWallGridLineCount,
              "x run grid per source wall") &&
       ok;
  ok = expect(geometry.indexedDraws.size() == kOptimizedDrawCount,
              "x run optimized draw count") &&
       ok;
  ok = expect(geometry.vertices.size() == kOptimizedDrawCount * kBoxVertexCount,
              "x run optimized vertex count") &&
       ok;
  ok = expect(geometry.indices.size() == kOptimizedDrawCount * kBoxIndexCount,
              "x run optimized index count") &&
       ok;
  ok = expect(geometry.vertices.size() < kNaiveDrawCount * kBoxVertexCount,
              "x run fewer vertices than naive") &&
       ok;
  ok = expect(geometry.indices.size() < kNaiveDrawCount * kBoxIndexCount,
              "x run fewer indices than naive") &&
       ok;
  ok = expect(near(bounds.minX, 0.0F) && near(bounds.maxX, 3.0F),
              "x run merged x bounds") &&
       ok;
  ok = expect(near(bounds.minZ, -0.25F) && near(bounds.maxZ, 0.25F),
              "x run merged z thickness") &&
       ok;
  return ok;
}

bool compatibleZWallSegmentsMergeButPreserveSourceGrid() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 2.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_3", {0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 3.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  const VertexBounds bounds = boundsForVertexRange(geometry.vertices, 0U, 8U);

  bool ok = true;
  ok = expect(geometry.ready, "z run merge geometry ready") && ok;
  ok = expect(geometry.sourceRoomStaticMeshCount == 3U,
              "z run source count preserved") &&
       ok;
  ok = expect(geometry.roomWallDrawCount == 1U, "z run wall draw merged") && ok;
  ok = expect(geometry.roomGridLineDrawCount == 24U,
              "z run grid per source wall") &&
       ok;
  ok = expect(geometry.indexedDraws.size() == 25U, "z run draw count") && ok;
  ok = expect(geometry.vertices.size() == 200U, "z run vertex count") && ok;
  ok = expect(geometry.indices.size() == 1800U, "z run index count") && ok;
  ok = expect(near(bounds.minX, -0.25F) && near(bounds.maxX, 0.25F),
              "z run merged x thickness") &&
       ok;
  ok = expect(near(bounds.minZ, 0.0F) && near(bounds.maxZ, 3.0F),
              "z run merged z bounds") &&
       ok;
  return ok;
}

bool gappedWallSegmentsDoNotMerge() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {2.0F, 0.0F, 0.0F}, {3.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready, "gapped walls geometry ready") &&
         expect(geometry.roomWallDrawCount == 2U, "gapped walls do not merge") &&
         expect(geometry.roomGridLineDrawCount == 16U, "gapped walls grid count");
}

bool perpendicularWallSegmentsDoNotMerge() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallSegmentMesh("wall_x", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_z", {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 1.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready, "perpendicular walls geometry ready") &&
         expect(geometry.roomWallDrawCount == 2U,
                "perpendicular walls do not merge") &&
         expect(geometry.roomGridLineDrawCount == 16U,
                "perpendicular walls grid count");
}

bool mismatchedWallMaterialDoesNotMerge() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F, "debug_wall"),
      wallSegmentMesh("wall_2", {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F, "painted_wall"),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready, "wall material mismatch geometry ready") &&
         expect(geometry.roomWallDrawCount == 2U,
                "wall material mismatch does not merge");
}

bool mismatchedWallDimensionsDoNotMerge() {
  const iggy3d::SceneRoomProjection bottomMismatch = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.25F, 2.0F, 0.5F),
  });
  const iggy3d::SceneRoomProjection heightMismatch = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.0F, 2.5F, 0.5F),
  });
  const iggy3d::SceneRoomProjection thicknessMismatch = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {1.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.75F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry bottomGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(bottomMismatch);
  const iggy3d::vulkan::RoomMeshCpuGeometry heightGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(heightMismatch);
  const iggy3d::vulkan::RoomMeshCpuGeometry thicknessGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(thicknessMismatch);
  return expect(bottomGeometry.ready, "wall bottom mismatch geometry ready") &&
         expect(heightGeometry.ready, "wall height mismatch geometry ready") &&
         expect(thicknessGeometry.ready, "wall thickness mismatch geometry ready") &&
         expect(bottomGeometry.roomWallDrawCount == 2U,
                "wall bottom mismatch does not merge") &&
         expect(heightGeometry.roomWallDrawCount == 2U,
                "wall height mismatch does not merge") &&
         expect(thicknessGeometry.roomWallDrawCount == 2U,
                "wall thickness mismatch does not merge");
}

bool duplicateWallSegmentsDoNotCollapse() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallSegmentMesh("wall_1", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
      wallSegmentMesh("wall_2", {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready, "duplicate walls geometry ready") &&
         expect(geometry.sourceRoomStaticMeshCount == 2U,
                "duplicate source count preserved") &&
         expect(geometry.roomWallDrawCount == 2U,
                "duplicate walls do not collapse") &&
         expect(geometry.roomGridLineDrawCount == 16U,
                "duplicate wall grid per source wall");
}

bool orientedWallSegmentsRenderDistinctBoundsAndSignatures() {
  const iggy3d::SceneRoomProjection xRoom = roomProjection({
      wallSegmentMesh("wall_x", {0.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::SceneRoomProjection zRoom = roomProjection({
      wallSegmentMesh("wall_z", {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 2.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry xGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(xRoom);
  const iggy3d::vulkan::RoomMeshCpuGeometry zGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(zRoom);
  const VertexBounds xBounds = boundsForVertexRange(xGeometry.vertices, 0U, 8U);
  const VertexBounds zBounds = boundsForVertexRange(zGeometry.vertices, 0U, 8U);

  bool ok = true;
  ok = expect(xGeometry.ready, "x wall segment geometry ready") && ok;
  ok = expect(zGeometry.ready, "z wall segment geometry ready") && ok;
  ok = expect(xGeometry.roomWallDrawCount == 1U, "x wall draw count") && ok;
  ok = expect(zGeometry.roomWallDrawCount == 1U, "z wall draw count") && ok;
  ok = expect(near(xBounds.minX, 0.0F) && near(xBounds.maxX, 2.0F),
              "x wall x length bounds") &&
       ok;
  ok = expect(near(xBounds.minZ, -0.25F) && near(xBounds.maxZ, 0.25F),
              "x wall z thickness bounds") &&
       ok;
  ok = expect(near(zBounds.minX, -0.25F) && near(zBounds.maxX, 0.25F),
              "z wall x thickness bounds") &&
       ok;
  ok = expect(near(zBounds.minZ, 0.0F) && near(zBounds.maxZ, 2.0F),
              "z wall z length bounds") &&
       ok;
  ok = expect(xGeometry.sourceRoomGeometrySignature !=
                  zGeometry.sourceRoomGeometrySignature,
              "oriented wall signatures differ") &&
       ok;
  return ok;
}

bool wallWithoutSegmentUsesFallbackBoxPath() {
  const iggy3d::SceneRoomProjection room = roomProjection({
      wallMesh("wall_fallback", 1.0F, 1.0F, 2.0F, "debug_wall",
               {2.0F, 2.0F, 0.5F}),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  const VertexBounds bounds = boundsForVertexRange(geometry.vertices, 0U, 8U);
  return expect(geometry.ready, "fallback wall geometry ready") &&
         expect(geometry.roomWallDrawCount == 1U, "fallback wall draw count") &&
         expect(near(bounds.minX, 0.0F), "fallback min x") &&
         expect(near(bounds.maxX, 2.0F), "fallback max x") &&
         expect(near(bounds.minY, 0.0F), "fallback min y") &&
         expect(near(bounds.maxY, 2.0F), "fallback max y") &&
         expect(near(bounds.minZ, 1.75F), "fallback min z") &&
         expect(near(bounds.maxZ, 2.25F), "fallback max z");
}

bool wallSegmentEndpointChangesGeometrySignature() {
  const iggy3d::SceneRoomProjection shortRoom = roomProjection({
      wallSegmentMesh("wall", {0.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::SceneRoomProjection longRoom = roomProjection({
      wallSegmentMesh("wall", {0.0F, 0.0F, 0.0F}, {3.0F, 0.0F, 0.0F},
                      0.0F, 2.0F, 0.5F),
  });
  const iggy3d::vulkan::RoomMeshCpuGeometry shortGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(shortRoom);
  const iggy3d::vulkan::RoomMeshCpuGeometry longGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(longRoom);
  return expect(shortGeometry.ready, "short wall geometry ready") &&
         expect(longGeometry.ready, "long wall geometry ready") &&
         expect(shortGeometry.sourceRoomGeometrySignature !=
                    longGeometry.sourceRoomGeometrySignature,
                "wall endpoint changes signature");
}

bool roomGeometrySignatureTracksAsciiRoomShape() {
  const iggy3d::AsciiRoomToRoomAssetResult small = buildAsciiRoomAsset(
      "###\n"
      "#P#\n"
      "###\n",
      "vulkan_ascii_room");
  const iggy3d::AsciiRoomToRoomAssetResult wide = buildAsciiRoomAsset(
      "####\n"
      "#P.#\n"
      "####\n",
      "vulkan_ascii_room");
  iggy3d::SessionState state;
  const iggy3d::SceneProjectionResult smallProjection =
      iggy3d::buildSceneProjection(state, &small.room);
  const iggy3d::SceneProjectionResult wideProjection =
      iggy3d::buildSceneProjection(state, &wide.room);
  const iggy3d::vulkan::RoomMeshCpuGeometry smallGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(smallProjection.room);
  const iggy3d::vulkan::RoomMeshCpuGeometry wideGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(wideProjection.room);

  return expect(small.ok, "small room asset ok") &&
         expect(wide.ok, "wide room asset ok") &&
         expect(smallGeometry.ready, "small geometry ready") &&
         expect(wideGeometry.ready, "wide geometry ready") &&
         expect(wideGeometry.sourceRoomStaticMeshCount >
                    smallGeometry.sourceRoomStaticMeshCount,
                "wide room has more source meshes") &&
         expect(wideGeometry.vertices.size() > smallGeometry.vertices.size(),
                "wide room has more vertices") &&
         expect(wideGeometry.indices.size() > smallGeometry.indices.size(),
                "wide room has more indices") &&
         expect(wideGeometry.sourceRoomGeometrySignature !=
                    smallGeometry.sourceRoomGeometrySignature,
                "geometry signature changes");
}

bool emptyProjectionDoesNotBuildRoomGeometry() {
  const iggy3d::SceneRoomProjection empty;
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(empty);
  return expect(!geometry.ready, "empty geometry not ready") &&
         expect(geometry.vertices.empty(), "empty geometry vertices") &&
         expect(geometry.indices.empty(), "empty geometry indices") &&
         expect(geometry.indexedDraws.empty(), "empty geometry draws") &&
         expect(geometry.sourceRoomStaticMeshCount == 0U,
                "empty geometry source count");
}

}  // namespace

int main() {
  bool ok = true;
  ok = asciiFloorsAndWallsBuildVulkanRoomGeometry() && ok;
  ok = compatibleFloorBlockMergesButPreservesSourceGrid() && ok;
  ok = nonAdjacentFloorsDoNotMerge() && ok;
  ok = mismatchedFloorMaterialDoesNotMergeAndAffectsSignature() && ok;
  ok = mismatchedFloorYOrSizeDoesNotMerge() && ok;
  ok = wallsRemainUnmerged() && ok;
  ok = compatibleXWallSegmentsMergeButPreserveSourceGrid() && ok;
  ok = compatibleZWallSegmentsMergeButPreserveSourceGrid() && ok;
  ok = gappedWallSegmentsDoNotMerge() && ok;
  ok = perpendicularWallSegmentsDoNotMerge() && ok;
  ok = mismatchedWallMaterialDoesNotMerge() && ok;
  ok = mismatchedWallDimensionsDoNotMerge() && ok;
  ok = duplicateWallSegmentsDoNotCollapse() && ok;
  ok = orientedWallSegmentsRenderDistinctBoundsAndSignatures() && ok;
  ok = wallWithoutSegmentUsesFallbackBoxPath() && ok;
  ok = wallSegmentEndpointChangesGeometrySignature() && ok;
  ok = roomGeometrySignatureTracksAsciiRoomShape() && ok;
  ok = emptyProjectionDoesNotBuildRoomGeometry() && ok;
  return ok ? 0 : 1;
}
