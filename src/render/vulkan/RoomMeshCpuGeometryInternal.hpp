#pragma once

#include "render/vulkan/BufferImageResources.hpp"

#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::vulkan::room_mesh_detail {

inline constexpr float kRoomMeshEpsilon = 0.0001F;
inline constexpr float kRoomMeshQuantizeScale = 10000.0F;

inline bool near(float lhs, float rhs) noexcept {
  return std::fabs(lhs - rhs) <= kRoomMeshEpsilon;
}

inline std::int64_t quantized(float value) noexcept {
  return static_cast<std::int64_t>(
      std::llround(value * kRoomMeshQuantizeScale));
}

inline bool finitePositive(float value) noexcept {
  return std::isfinite(value) && value > kRoomMeshEpsilon;
}

inline bool finiteVec3(Vec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

inline bool hasRotation(Vec3 rotationEulerRadians) noexcept {
  return finiteVec3(rotationEulerRadians) &&
         (!near(rotationEulerRadians.x, 0.0F) ||
          !near(rotationEulerRadians.y, 0.0F) ||
          !near(rotationEulerRadians.z, 0.0F));
}

inline bool externalStaticMeshId(std::string_view meshId,
                                 std::string_view& assetId) noexcept {
  constexpr std::string_view prefix = "asset:";
  if (!meshId.starts_with(prefix) || meshId.size() == prefix.size()) {
    return false;
  }
  assetId = meshId.substr(prefix.size());
  return true;
}

struct FloorDraw {
  Vec3 position;
  Vec3 size;
  Vec3 rotationEulerRadians;
};

struct WallBoxDraw {
  Vec3 position;
  Vec3 size;
  Vec3 rotationEulerRadians;
};

[[nodiscard]] Vec3 colorForRoomRole(const std::string& role);

[[nodiscard]] bool appendFloorPlaneIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color);

[[nodiscard]] bool appendSurfacePatches(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    std::span<const SceneRoomSurfacePatchItem> patches);

[[nodiscard]] bool canAppendSurfacePatches(
    const std::vector<FirstRoomVertex>& vertices,
    std::span<const SceneRoomSurfacePatchItem> patches);

[[nodiscard]] bool appendBoxIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians = {});

[[nodiscard]] bool appendRampWedgeIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians);

[[nodiscard]] bool appendOpenFrameIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians);

[[nodiscard]] bool appendStairStepsIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians,
    std::uint16_t segmentCount);

void appendCreativeTargetWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color);

void appendCreativePathWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color);

[[nodiscard]] bool appendCreativeGeneratedPreviewShape(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    RenderCreativePreviewGeometryProfile profile,
    std::uint16_t proceduralSegmentCount,
    Vec3 size,
    Vec3 color);

[[nodiscard]] Vec3 componentProduct(Vec3 lhs, Vec3 rhs) noexcept;

[[nodiscard]] bool appendStaticMeshAsset(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    const SceneRoomMeshItem& item,
    const StaticMeshAsset& asset,
    const StaticMeshMaterialTextureResources* materialTextures);

[[nodiscard]] std::vector<FloorDraw> buildOptimizedFloorDraws(
    const SceneRoomProjection& room);
[[nodiscard]] bool canEmitFloorDraw(const FloorDraw& floor) noexcept;
[[nodiscard]] bool wallBoxForMesh(
    const SceneRoomMeshItem& mesh, WallBoxDraw& draw) noexcept;
[[nodiscard]] bool buildOptimizedWallDraws(
    const SceneRoomProjection& room,
    std::vector<WallBoxDraw>& wallDraws);

[[nodiscard]] std::uint64_t roomGeometrySignature(
    const SceneRoomProjection& room);

[[nodiscard]] std::uint64_t creativeWireframeDebugGeometrySignature(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug);

void appendCreativeWireframeDebugReceiptFields(
    RenderReceipt& receipt, const FirstRoomGeometryResources& geometry);

}  // namespace iggy3d::vulkan::room_mesh_detail
