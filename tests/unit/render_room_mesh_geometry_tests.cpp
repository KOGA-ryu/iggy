#include "app/iggy3d/AsciiRoomGrid.hpp"
#include "app/iggy3d/AsciiRoomSource.hpp"
#include "app/iggy3d/AsciiRoomToAuthoredRoom.hpp"
#include "app/iggy3d/AsciiRoomToRoomAsset.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/session/SessionState.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <string_view>

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
  constexpr std::size_t kWallCount = 8U;
  constexpr std::size_t kMeshCount = kFloorCount + kWallCount;
  constexpr std::size_t kFloorGridLineCount = kFloorCount * 4U;
  constexpr std::size_t kWallGridLineCount = kWallCount * 8U;
  constexpr std::size_t kGridLineCount = kFloorGridLineCount + kWallGridLineCount;
  constexpr std::size_t kDrawCount = kMeshCount + kGridLineCount;
  constexpr std::size_t kBoxVertexCount = 8U;
  constexpr std::size_t kBoxIndexCount = 72U;

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
  ok = expect(countProjectedRole(projection.room, "wall") == kWallCount,
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
  ok = expect(geometry.roomWallDrawCount == kWallCount,
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
  ok = expect(geometry.vertices.size() == kDrawCount * kBoxVertexCount,
              "geometry vertex count") &&
       ok;
  ok = expect(geometry.indices.size() == kDrawCount * kBoxIndexCount,
              "geometry index count") &&
       ok;
  ok = expect(geometry.indexedDraws.size() == kDrawCount,
              "geometry draw count") &&
       ok;
  for (std::size_t i = 0; i < geometry.indexedDraws.size(); ++i) {
    ok = expect(geometry.indexedDraws[i].firstIndex == i * kBoxIndexCount,
                "draw first index") &&
         ok;
    ok = expect(geometry.indexedDraws[i].indexCount == kBoxIndexCount,
                "draw index count") &&
         ok;
  }
  ok = expect(countVerticesWithColor(geometry.vertices, 0.30F, 0.32F, 0.34F) ==
                  kFloorCount * kBoxVertexCount,
              "floor vertex color count") &&
       ok;
  ok = expect(countVerticesWithColor(geometry.vertices, 0.42F, 0.43F, 0.46F) ==
                  kWallCount * kBoxVertexCount,
              "wall vertex color count") &&
       ok;
  ok = expect(countVerticesWithColor(geometry.vertices, 0.78F, 0.82F, 0.86F) ==
                  kGridLineCount * kBoxVertexCount,
              "grid vertex color count") &&
       ok;
  return ok;
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
  ok = roomGeometrySignatureTracksAsciiRoomShape() && ok;
  ok = emptyProjectionDoesNotBuildRoomGeometry() && ok;
  return ok ? 0 : 1;
}
