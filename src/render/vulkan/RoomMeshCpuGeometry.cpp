#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include "content/assets/GeneratedGeometry.hpp"
#include "core/math/EulerRotation.hpp"
#include "render/mesh/BeanMesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace iggy3d::vulkan {

using room_mesh_detail::appendBoxIfFits;
using room_mesh_detail::appendCreativeGeneratedPreviewShape;
using room_mesh_detail::appendCreativePathWireframe;
using room_mesh_detail::appendCreativeTargetWireframe;
using room_mesh_detail::appendFloorPlaneIfFits;
using room_mesh_detail::appendOpenFrameIfFits;
using room_mesh_detail::appendRampWedgeIfFits;
using room_mesh_detail::appendStaticMeshAsset;
using room_mesh_detail::appendStairStepsIfFits;
using room_mesh_detail::appendSurfacePatches;
using room_mesh_detail::buildOptimizedFloorDraws;
using room_mesh_detail::buildOptimizedWallDraws;
using room_mesh_detail::canEmitFloorDraw;
using room_mesh_detail::canAppendSurfacePatches;
using room_mesh_detail::colorForRoomRole;
using room_mesh_detail::componentProduct;
using room_mesh_detail::externalStaticMeshId;
using room_mesh_detail::finitePositive;
using room_mesh_detail::finiteVec3;
using room_mesh_detail::FloorDraw;
using room_mesh_detail::hasRotation;
using room_mesh_detail::kRoomMeshEpsilon;
using room_mesh_detail::near;
using room_mesh_detail::quantized;
using room_mesh_detail::wallBoxForMesh;
using room_mesh_detail::WallBoxDraw;

namespace {


void hashByte(std::uint64_t& hash, std::uint8_t value) {
  hash ^= value;
  hash *= 1099511628211ULL;
}

void hashString(std::uint64_t& hash, const std::string& value) {
  for (const char character : value) {
    hashByte(hash, static_cast<std::uint8_t>(character));
  }
  hashByte(hash, 0U);
}

void hashFloat(std::uint64_t& hash, float value) {
  std::uint32_t bits = 0U;
  static_assert(sizeof(bits) == sizeof(value));
  std::memcpy(&bits, &value, sizeof(bits));
  for (std::uint32_t shift = 0U; shift < 32U; shift += 8U) {
    hashByte(hash, static_cast<std::uint8_t>((bits >> shift) & 0xFFU));
  }
}

void hashUint16(std::uint64_t& hash, std::uint16_t value) {
  hashByte(hash, static_cast<std::uint8_t>(value & 0xFFU));
  hashByte(hash, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void hashBool(std::uint64_t& hash, bool value) {
  hashByte(hash, value ? 1U : 0U);
}

void hashVec3(std::uint64_t& hash, Vec3 value) {
  hashFloat(hash, value.x);
  hashFloat(hash, value.y);
  hashFloat(hash, value.z);
}

std::uint64_t roomGeometrySignature(const SceneRoomProjection& room) {
  std::uint64_t hash = 1469598103934665603ULL;
  hashString(hash, room.assetId);
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    hashString(hash, mesh.id);
    hashString(hash, mesh.meshId);
    hashString(hash, mesh.role);
    hashString(hash, mesh.materialId);
    hashFloat(hash, mesh.position.x);
    hashFloat(hash, mesh.position.y);
    hashFloat(hash, mesh.position.z);
    hashFloat(hash, mesh.size.x);
    hashFloat(hash, mesh.size.y);
    hashFloat(hash, mesh.size.z);
    hashVec3(hash, mesh.rotationEulerRadians);
    hashUint16(hash, mesh.proceduralSegmentCount);
    hashBool(hash, mesh.hasWallSegment);
    if (mesh.hasWallSegment) {
      hashVec3(hash, mesh.wallStartMeters);
      hashVec3(hash, mesh.wallEndMeters);
      hashFloat(hash, mesh.wallBottomY);
      hashFloat(hash, mesh.wallHeightMeters);
      hashFloat(hash, mesh.wallThicknessMeters);
    }
  }
  for (const SceneRoomSurfacePatchItem& patch : room.surfacePatches) {
    hashString(hash, patch.role);
    hashVec3(hash, patch.center);
    for (const Vec3 corner : patch.corners) {
      hashVec3(hash, corner);
    }
  }
  return hash;
}


void setCreativeWireframeDebugGeometryStatus(
    CreativeWireframeDebugCpuGeometry& geometry,
    std::string_view status) {
  geometry.status = std::string(status);
  geometry.reasonCode = geometry.status;
}

bool creativeDebugLineBox(const RenderCreativeWireframeDebugLine& line,
                          Vec3& center,
                          Vec3& size,
                          Vec3& color,
                          Vec3& rotationEulerRadians) {
  if (!finiteVec3(line.start) || !finiteVec3(line.end) ||
      !std::isfinite(line.thickness) || line.thickness <= 0.0F ||
      !std::isfinite(line.color.r) || !std::isfinite(line.color.g) ||
      !std::isfinite(line.color.b)) {
    return false;
  }

  const float dx = line.end.x - line.start.x;
  const float dy = line.end.y - line.start.y;
  const float dz = line.end.z - line.start.z;
  const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (!std::isfinite(length) || near(length, 0.0F)) {
    return false;
  }

  center = {(line.start.x + line.end.x) * 0.5F,
            (line.start.y + line.end.y) * 0.5F,
            (line.start.z + line.end.z) * 0.5F};
  const float thickness = std::max(line.thickness, 0.001F);
  size = {length, thickness, thickness};
  const float horizontalLength = std::sqrt(dx * dx + dy * dy);
  rotationEulerRadians =
      {0.0F, std::atan2(-dz, horizontalLength), std::atan2(dy, dx)};
  color = {line.color.r, line.color.g, line.color.b};
  return true;
}

std::uint64_t creativeWireframeDebugGeometrySignature(
    const RenderCreativeWireframeDebugFrame* frame) {
  std::uint64_t hash = 1469598103934665603ULL;
  if (frame == nullptr || !frame->available) {
    hashString(hash, "not_requested");
    return hash;
  }
  hashString(hash, frame->visible ? "visible" : "hidden");
  hashByte(hash, static_cast<std::uint8_t>(frame->lineCount & 0xFFU));
  hashByte(hash, static_cast<std::uint8_t>((frame->lineCount >> 8U) & 0xFFU));
  if (frame->lines == nullptr || frame->lineCount == 0U) {
    return hash;
  }
  for (std::size_t index = 0; index < frame->lineCount; ++index) {
    const RenderCreativeWireframeDebugLine& line = frame->lines[index];
    hashVec3(hash, line.start);
    hashVec3(hash, line.end);
    hashFloat(hash, line.color.r);
    hashFloat(hash, line.color.g);
    hashFloat(hash, line.color.b);
    hashFloat(hash, line.color.a);
    hashFloat(hash, line.thickness);
  }
  return hash;
}

void appendCreativeWireframeDebugReceiptFields(
    RenderReceipt& receipt,
    const FirstRoomGeometryResources& geometry) {
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_input_line_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugLineInputCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_draw_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugGeometryDrawCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_box_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugGeometryDrawCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_skipped_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugGeometrySkippedCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_status",
                     geometry.creativeWireframeDebugGeometryStatus);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_reason_code",
                     geometry.creativeWireframeDebugGeometryReasonCode);
}

bool appendFloorGrid(std::vector<FirstRoomVertex>& vertices,
                     std::vector<std::uint16_t>& indices,
                     std::vector<IndexedDrawRange>& draws,
                     Vec3 center,
                     Vec3 size,
                     std::size_t& lineCount) {
  constexpr float kGridThickness = 0.035F;
  constexpr float kGridHeight = 0.012F;
  constexpr float kGridLift = 0.008F;
  const float halfX = std::max(size.x * 0.5F, 0.001F);
  const float halfZ = std::max(size.z * 0.5F, 0.001F);
  const float y = center.y + std::max(size.y * 0.5F, 0.001F) + kGridLift;
  const Vec3 color = colorForRoomRole("grid");
  const Vec3 xLineSize{halfX * 2.0F + kGridThickness, kGridHeight, kGridThickness};
  const Vec3 zLineSize{kGridThickness, kGridHeight, halfZ * 2.0F + kGridThickness};
  const Vec3 lineCenters[4] = {
      {center.x, y, center.z - halfZ},
      {center.x, y, center.z + halfZ},
      {center.x - halfX, y, center.z},
      {center.x + halfX, y, center.z},
  };
  const Vec3 lineSizes[4] = {xLineSize, xLineSize, zLineSize, zLineSize};
  for (std::size_t i = 0; i < 4U; ++i) {
    if (!appendBoxIfFits(vertices, indices, draws, lineCenters[i], lineSizes[i], color)) {
      return false;
    }
    ++lineCount;
  }
  return true;
}

bool appendWallGrid(std::vector<FirstRoomVertex>& vertices,
                    std::vector<std::uint16_t>& indices,
                    std::vector<IndexedDrawRange>& draws,
                    Vec3 center,
                    Vec3 size,
                    std::size_t& lineCount) {
  constexpr float kGridThickness = 0.035F;
  constexpr float kGridLift = 0.010F;
  const float halfX = std::max(size.x * 0.5F, 0.001F);
  const float halfY = std::max(size.y * 0.5F, 0.001F);
  const float halfZ = std::max(size.z * 0.5F, 0.001F);
  const Vec3 color = colorForRoomRole("grid");
  const float topY = center.y + halfY + kGridLift;
  const Vec3 topXLineSize{halfX * 2.0F + kGridThickness, kGridThickness,
                          kGridThickness};
  const Vec3 topZLineSize{kGridThickness, kGridThickness,
                          halfZ * 2.0F + kGridThickness};
  const Vec3 verticalSize{kGridThickness, halfY * 2.0F + kGridThickness,
                          kGridThickness};
  const Vec3 lineCenters[8] = {
      {center.x, topY, center.z - halfZ},
      {center.x, topY, center.z + halfZ},
      {center.x - halfX, topY, center.z},
      {center.x + halfX, topY, center.z},
      {center.x - halfX, center.y, center.z - halfZ},
      {center.x + halfX, center.y, center.z - halfZ},
      {center.x - halfX, center.y, center.z + halfZ},
      {center.x + halfX, center.y, center.z + halfZ},
  };
  const Vec3 lineSizes[8] = {topXLineSize, topXLineSize, topZLineSize, topZLineSize,
                             verticalSize, verticalSize, verticalSize, verticalSize};
  for (std::size_t i = 0; i < 8U; ++i) {
    if (!appendBoxIfFits(vertices, indices, draws, lineCenters[i], lineSizes[i], color)) {
      return false;
    }
    ++lineCount;
  }
  return true;
}

bool appendBean(std::vector<FirstRoomVertex>& vertices,
                std::vector<std::uint16_t>& indices,
                std::vector<IndexedDrawRange>& draws,
                Vec3 center,
                Vec3 size,
                Vec3 color,
                SceneModelKind kind) {
  const BeanMesh bean = buildBeanMesh(kind);
  if (bean.vertices.empty() || bean.indices.empty() ||
      vertices.size() + bean.vertices.size() >
          static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
  const float sx = std::max(size.x, 0.001F);
  const float sy = std::max(size.y, 0.001F);
  const float sz = std::max(size.z, 0.001F);
  for (const BeanMeshVertex& vertex : bean.vertices) {
    const Vec3 world{
        center.x + vertex.position.x * sx,
        center.y + vertex.position.y * sy,
        center.z + vertex.position.z * sz,
    };
    vertices.push_back({{world.x, world.y, world.z}, {color.x, color.y, color.z}});
  }

  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  for (const std::uint16_t index : bean.indices) {
    indices.push_back(static_cast<std::uint16_t>(base + index));
  }
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
  return true;
}


bool appendStaticMeshPreviewRole(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    const StaticMeshAsset& asset,
    RenderCreativePreviewRole role,
    Vec3 color,
    IndexedDrawRange& range) {
  constexpr std::size_t kTargetWireframeVertexCount = 12U * 8U;
  const Vec3 size = asset.boundsMax - asset.boundsMin;
  const bool target = role != RenderCreativePreviewRole::Held;
  const std::size_t extraVertices =
      asset.vertices.size() + (target ? kTargetWireframeVertexCount : 0U);
  if (!asset.hasBounds || !finiteVec3(size) || size.x <= 0.0F ||
      size.y <= 0.0F || size.z <= 0.0F || asset.vertices.empty() ||
      vertices.size() + extraVertices >
          static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
    return false;
  }
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    if (primitive.firstIndex > asset.indices.size() ||
        primitive.indexCount > asset.indices.size() - primitive.firstIndex) {
      return false;
    }
    for (std::uint32_t offset = 0; offset < primitive.indexCount; ++offset) {
      if (asset.indices[primitive.firstIndex + offset] >=
          asset.vertices.size()) {
        return false;
      }
    }
  }

  const float inset = target ? 0.96F : 1.0F;
  const Vec3 center = (asset.boundsMin + asset.boundsMax) * 0.5F;
  const Vec3 scale{inset / size.x, inset / size.y, inset / size.z};
  const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
  for (const StaticMeshVertex& source : asset.vertices) {
    const Vec3 position = componentProduct(source.position - center, scale);
    if (!finiteVec3(position)) {
      return false;
    }
    vertices.push_back({{position.x, position.y, position.z},
                        {color.x, color.y, color.z}});
  }
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    for (std::uint32_t offset = 0; offset < primitive.indexCount; ++offset) {
      indices.push_back(base + asset.indices[primitive.firstIndex + offset]);
    }
  }
  if (target) {
    appendCreativeTargetWireframe(vertices, indices, componentDraws, color);
  }
  range.indexCount =
      static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  return range.indexCount > 0U;
}

// A thin, double-sided horizontal triangle from `start` to `end` (a debug
// "gaze blade" showing an NPC's vision direction and range). Base half-width
// is `halfWidth`. Returns false on a degenerate segment or vertex overflow.
bool appendGazeBlade(std::vector<FirstRoomVertex>& vertices,
                     std::vector<std::uint16_t>& indices,
                     std::vector<IndexedDrawRange>& draws,
                     Vec3 start,
                     Vec3 end,
                     float halfWidth,
                     Vec3 color) {
  const float dx = end.x - start.x;
  const float dz = end.z - start.z;
  const float lengthSq = dx * dx + dz * dz;
  if (!std::isfinite(lengthSq) || lengthSq < 1.0e-6F ||
      vertices.size() + 3 >
          static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }
  const float width = std::max(halfWidth, 0.001F);
  const float inv = 1.0F / std::sqrt(lengthSq);
  const float perpX = -dz * inv * width;
  const float perpZ = dx * inv * width;
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
  vertices.push_back({{start.x, start.y, start.z}, {color.x, color.y, color.z}});
  vertices.push_back({{end.x + perpX, end.y, end.z + perpZ},
                      {color.x, color.y, color.z}});
  vertices.push_back({{end.x - perpX, end.y, end.z - perpZ},
                      {color.x, color.y, color.z}});
  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  const std::uint16_t winding[6] = {
      base, static_cast<std::uint16_t>(base + 1),
      static_cast<std::uint16_t>(base + 2), base,
      static_cast<std::uint16_t>(base + 2),
      static_cast<std::uint16_t>(base + 1)};
  for (const std::uint16_t index : winding) {
    indices.push_back(index);
  }
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
  return true;
}

}  // namespace

std::vector<FirstRoomVertex> firstRoomBootstrapVertices() {
  return {{{-1.5F, 0.0F, -1.5F}, {0.35F, 0.38F, 0.42F}},
          {{1.5F, 0.0F, -1.5F}, {0.35F, 0.38F, 0.42F}},
          {{1.5F, 0.0F, 1.5F}, {0.48F, 0.52F, 0.56F}},
          {{-1.5F, 0.0F, 1.5F}, {0.48F, 0.52F, 0.56F}}};
}

std::vector<std::uint16_t> firstRoomBootstrapIndices() {
  return {0U, 1U, 2U, 2U, 3U, 0U};
}

CreativeWireframeDebugCpuGeometry buildCreativeWireframeDebugCpuGeometry(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  CreativeWireframeDebugCpuGeometry result;
  result.geometrySignature =
      creativeWireframeDebugGeometrySignature(creativeWireframeDebug);

  if (creativeWireframeDebug == nullptr || !creativeWireframeDebug->available) {
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_not_requested");
    return result;
  }

  result.inputLineCount = creativeWireframeDebug->lineCount;
  if (creativeWireframeDebug->lineCount == 0U) {
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_no_lines");
    return result;
  }

  if (creativeWireframeDebug->lines == nullptr) {
    result.skippedCount = creativeWireframeDebug->lineCount;
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_source_missing");
    return result;
  }

  result.vertices.reserve(creativeWireframeDebug->lineCount * 8U);
  result.indices.reserve(creativeWireframeDebug->lineCount * 72U);
  for (std::size_t index = 0; index < creativeWireframeDebug->lineCount;
       ++index) {
    Vec3 center;
    Vec3 size;
    Vec3 color;
    Vec3 rotationEulerRadians;
    if (!creativeDebugLineBox(creativeWireframeDebug->lines[index],
                              center,
                              size,
                              color,
                              rotationEulerRadians) ||
        !appendBoxIfFits(result.vertices,
                         result.indices,
                         result.indexedDraws,
                         center,
                         size,
                         color,
                         rotationEulerRadians)) {
      ++result.skippedCount;
      continue;
    }
    ++result.emittedBoxCount;
  }

  result.ready = result.emittedBoxCount > 0U && !result.vertices.empty() &&
                 !result.indices.empty() && !result.indexedDraws.empty();
  if (result.ready) {
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_built");
    return result;
  }

  setCreativeWireframeDebugGeometryStatus(
      result,
      "vulkan_creative_wireframe_debug_geometry_no_geometry");
  return result;
}

void appendCreativeWireframeDebugGeometry(RoomMeshCpuGeometry& roomGeometry,
                                          const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  const CreativeWireframeDebugCpuGeometry debugGeometry =
      buildCreativeWireframeDebugCpuGeometry(creativeWireframeDebug);
  roomGeometry.creativeWireframeDebugLineInputCount =
      debugGeometry.inputLineCount;
  roomGeometry.creativeWireframeDebugGeometryDrawCount =
      debugGeometry.emittedBoxCount;
  roomGeometry.creativeWireframeDebugGeometrySkippedCount =
      debugGeometry.skippedCount;
  roomGeometry.creativeWireframeDebugGeometryStatus = debugGeometry.status;
  roomGeometry.creativeWireframeDebugGeometryReasonCode =
      debugGeometry.reasonCode;
  roomGeometry.sourceCreativeWireframeDebugSignature =
      debugGeometry.geometrySignature;

  if (!debugGeometry.ready) {
    return;
  }

  if (roomGeometry.vertices.size() + debugGeometry.vertices.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    roomGeometry.creativeWireframeDebugGeometryDrawCount = 0;
    roomGeometry.creativeWireframeDebugGeometrySkippedCount =
        debugGeometry.inputLineCount;
    roomGeometry.creativeWireframeDebugGeometryStatus =
        "vulkan_creative_wireframe_debug_geometry_no_geometry";
    roomGeometry.creativeWireframeDebugGeometryReasonCode =
        roomGeometry.creativeWireframeDebugGeometryStatus;
    return;
  }

  const std::uint16_t vertexBase =
      static_cast<std::uint16_t>(roomGeometry.vertices.size());
  const std::uint32_t indexBase =
      static_cast<std::uint32_t>(roomGeometry.indices.size());
  roomGeometry.vertices.insert(roomGeometry.vertices.end(),
                               debugGeometry.vertices.begin(),
                               debugGeometry.vertices.end());
  roomGeometry.indices.reserve(roomGeometry.indices.size() +
                               debugGeometry.indices.size());
  for (const std::uint16_t index : debugGeometry.indices) {
    roomGeometry.indices.push_back(static_cast<std::uint16_t>(vertexBase + index));
  }
  roomGeometry.indexedDraws.reserve(roomGeometry.indexedDraws.size() +
                                    debugGeometry.indexedDraws.size());
  for (IndexedDrawRange draw : debugGeometry.indexedDraws) {
    draw.firstIndex += indexBase;
    roomGeometry.indexedDraws.push_back(draw);
  }
}

namespace {

const StaticMeshAssetDrawRanges* findStaticMeshAssetDrawRanges(
    std::span<const StaticMeshAssetDrawRanges> assets,
    std::string_view assetId,
    std::size_t& index) noexcept {
  const auto found = std::lower_bound(
      assets.begin(), assets.end(), assetId,
      [](const StaticMeshAssetDrawRanges& asset, std::string_view id) {
        return asset.assetId < id;
      });
  if (found == assets.end() || found->assetId != assetId) {
    return nullptr;
  }
  index = static_cast<std::size_t>(found - assets.begin());
  return &*found;
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometryImpl(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets,
    const StaticMeshMaterialTextureResources* materialTextures,
    const std::span<const StaticMeshAssetDrawRanges>* staticMeshAssetDraws) {
  RoomMeshCpuGeometry result;
  result.sourceRoomAssetId = room.assetId;
  result.sourceRoomStaticMeshCount = room.meshes.size();
  result.sourceRoomGeometrySignature = roomGeometrySignature(room);
  if (room.meshes.empty() && room.surfacePatches.empty()) {
    return result;
  }

  result.vertices.reserve(room.meshes.size() * 16U +
                          room.surfacePatches.size() * 5U);
  result.indices.reserve(room.meshes.size() * 144U +
                         room.surfacePatches.size() * 24U);
  std::vector<std::vector<StaticMeshInstanceTransform>> instanceGroups;
  if (staticMeshAssetDraws != nullptr) {
    instanceGroups.resize(staticMeshAssetDraws->size());
  }

  const std::vector<FloorDraw> floorDraws = buildOptimizedFloorDraws(room);
  std::vector<WallBoxDraw> wallDraws;
  if (!buildOptimizedWallDraws(room, wallDraws)) {
    result.vertices.clear();
    result.indices.clear();
    result.indexedDraws.clear();
    return result;
  }
  for (const FloorDraw& floor : floorDraws) {
    if (!canEmitFloorDraw(floor)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    const bool appended =
        hasRotation(floor.rotationEulerRadians)
            ? appendBoxIfFits(result.vertices, result.indices,
                              result.indexedDraws, floor.position, floor.size,
                              colorForRoomRole("floor"),
                              floor.rotationEulerRadians)
            : appendFloorPlaneIfFits(result.vertices, result.indices,
                                     result.indexedDraws, floor.position,
                                     floor.size, colorForRoomRole("floor"));
    if (!appended) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    ++result.roomFloorDrawCount;
  }
  for (const WallBoxDraw& wall : wallDraws) {
    if (!appendBoxIfFits(result.vertices, result.indices, result.indexedDraws,
                         wall.position, wall.size, colorForRoomRole("wall"),
                         wall.rotationEulerRadians)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    ++result.roomWallDrawCount;
  }

  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (!finiteVec3(mesh.rotationEulerRadians)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    std::string_view externalAssetId;
    if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
      if (staticMeshAssetDraws != nullptr) {
        std::size_t assetIndex = 0U;
        const StaticMeshAssetDrawRanges* asset =
            findStaticMeshAssetDrawRanges(*staticMeshAssetDraws,
                                          externalAssetId, assetIndex);
        StaticMeshInstanceTransform transform;
        if (asset != nullptr && buildStaticMeshInstanceTransform(
                                    mesh, asset->boundsMin, asset->boundsMax,
                                    transform)) {
          instanceGroups[assetIndex].push_back(transform);
          continue;
        }
        if (!appendBoxIfFits(result.vertices, result.indices,
                             result.indexedDraws, mesh.position, mesh.size,
                             {1.0F, 0.0F, 1.0F},
                             mesh.rotationEulerRadians)) {
          result.vertices.clear();
          result.indices.clear();
          result.indexedDraws.clear();
          return result;
        }
        continue;
      }
      const StaticMeshAsset* asset =
          staticMeshAssets != nullptr
              ? staticMeshAssets->find(externalAssetId)
              : nullptr;
      if (asset != nullptr &&
          appendStaticMeshAsset(result.vertices, result.indices,
                                result.indexedDraws, mesh, *asset,
                                materialTextures)) {
        continue;
      }
      if (!appendBoxIfFits(result.vertices, result.indices,
                           result.indexedDraws, mesh.position, mesh.size,
                           {1.0F, 0.0F, 1.0F},
                           mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.role == "terrain") {
      continue;
    }
    if (mesh.role == "floor") {
      if (hasRotation(mesh.rotationEulerRadians)) {
        continue;
      }
      std::size_t gridLines = 0;
      if (!appendFloorGrid(result.vertices, result.indices, result.indexedDraws,
                           mesh.position, mesh.size, gridLines)) {
        result.roomGridTruncated = true;
      }
      result.roomGridLineDrawCount += gridLines;
      result.roomGridVisible = result.roomGridVisible || gridLines > 0U;
      continue;
    }
    if (mesh.role == "wall") {
      WallBoxDraw wallDraw;
      if (!wallBoxForMesh(mesh, wallDraw)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      if (hasRotation(wallDraw.rotationEulerRadians)) {
        continue;
      }
      std::size_t gridLines = 0;
      if (!appendWallGrid(result.vertices, result.indices, result.indexedDraws,
                          wallDraw.position, wallDraw.size, gridLines)) {
        result.roomGridTruncated = true;
      }
      result.roomGridLineDrawCount += gridLines;
      result.roomGridVisible = result.roomGridVisible || gridLines > 0U;
      continue;
    }

    if (mesh.role == "npc_gaze_perceived" || mesh.role == "npc_gaze_blocked" ||
        mesh.role == "npc_gaze_scan") {
      // Debug aid: a failed blade is skipped, never nukes the frame.
      (void)appendGazeBlade(result.vertices, result.indices, result.indexedDraws,
                            mesh.wallStartMeters, mesh.wallEndMeters,
                            mesh.wallThicknessMeters, colorForRoomRole(mesh.role));
      continue;
    }

    if (mesh.meshId == "creative_ramp_wedge") {
      if (!appendRampWedgeIfFits(result.vertices, result.indices,
                                 result.indexedDraws, mesh.position, mesh.size,
                                 colorForRoomRole(mesh.role),
                                 mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.meshId == "creative_open_frame") {
      if (!appendOpenFrameIfFits(
              result.vertices, result.indices, result.indexedDraws,
              mesh.position, mesh.size, colorForRoomRole(mesh.role),
              mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.meshId == "creative_stair_steps") {
      if (!appendStairStepsIfFits(
              result.vertices, result.indices, result.indexedDraws,
              mesh.position, mesh.size, colorForRoomRole(mesh.role),
              mesh.rotationEulerRadians, mesh.proceduralSegmentCount)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }

    SceneModelKind beanKind = SceneModelKind::PlayerBean;
    if (parseSceneModelId(mesh.role, beanKind)) {
      if (!appendBean(result.vertices, result.indices, result.indexedDraws,
                      mesh.position, mesh.size, colorForRoomRole(mesh.role),
                      beanKind)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
    } else {
      if (!appendBoxIfFits(result.vertices, result.indices,
                           result.indexedDraws, mesh.position, mesh.size,
                           colorForRoomRole(mesh.role),
                           mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      if (mesh.role == "grid") {
        ++result.roomGridLineDrawCount;
        result.roomGridVisible = true;
      }
    }
  }

  const bool useSurfacePatches =
      canAppendSurfacePatches(result.vertices, room.surfacePatches);
  if (useSurfacePatches) {
    static_cast<void>(appendSurfacePatches(
        result.vertices, result.indices, result.indexedDraws,
        room.surfacePatches));
    ++result.roomFloorDrawCount;
  } else {
    for (const SceneRoomMeshItem& mesh : room.meshes) {
      if (mesh.role != "terrain") {
        continue;
      }
      std::string_view externalAssetId;
      if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
        continue;
      }
      const FloorDraw terrain{mesh.position, mesh.size,
                              mesh.rotationEulerRadians};
      if (!canEmitFloorDraw(terrain) ||
          !appendFloorPlaneIfFits(result.vertices, result.indices,
                                  result.indexedDraws, terrain.position,
                                  terrain.size, colorForRoomRole("terrain"))) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      ++result.roomFloorDrawCount;
    }
  }

  if (staticMeshAssetDraws != nullptr) {
    for (std::size_t assetIndex = 0U; assetIndex < instanceGroups.size();
         ++assetIndex) {
      const std::vector<StaticMeshInstanceTransform>& instances =
          instanceGroups[assetIndex];
      if (instances.empty()) {
        continue;
      }
      if (instances.size() > std::numeric_limits<std::uint32_t>::max() ||
          result.staticMeshInstances.size() >
              std::numeric_limits<std::uint32_t>::max() - instances.size()) {
        result.staticMeshInstances.clear();
        result.staticMeshInstanceBatches.clear();
        return result;
      }
      const std::uint32_t firstInstance =
          static_cast<std::uint32_t>(result.staticMeshInstances.size());
      const std::uint32_t instanceCount =
          static_cast<std::uint32_t>(instances.size());
      result.staticMeshInstances.insert(result.staticMeshInstances.end(),
                                        instances.begin(), instances.end());
      for (const IndexedDrawRange& draw :
           (*staticMeshAssetDraws)[assetIndex].indexedDraws) {
        if (draw.indexCount == 0U) {
          continue;
        }
        result.staticMeshInstanceBatches.push_back(
            {draw.firstIndex, draw.indexCount, draw.materialTextureIndex,
             firstInstance, instanceCount});
      }
    }
  }

  appendCreativeWireframeDebugGeometry(result, creativeWireframeDebug);
  const bool baseGeometryReady = !result.vertices.empty() &&
                                 !result.indices.empty() &&
                                 !result.indexedDraws.empty();
  const bool instanceGeometryReady = !result.staticMeshInstances.empty() &&
                                     !result.staticMeshInstanceBatches.empty();
  result.ready = baseGeometryReady || instanceGeometryReady;
  return result;
}

}  // namespace

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets,
    const StaticMeshMaterialTextureResources* materialTextures) {
  return buildRoomMeshCpuGeometryImpl(room, creativeWireframeDebug,
                                      staticMeshAssets, materialTextures,
                                      nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    std::span<const StaticMeshAssetDrawRanges> staticMeshAssetDraws) {
  return buildRoomMeshCpuGeometryImpl(room, creativeWireframeDebug, nullptr,
                                      nullptr, &staticMeshAssetDraws);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(const SceneRoomProjection& room) {
  return buildRoomMeshCpuGeometry(room, nullptr, nullptr, nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  return buildRoomMeshCpuGeometry(room, creativeWireframeDebug, nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets) {
  return buildRoomMeshCpuGeometry(room, creativeWireframeDebug,
                                  staticMeshAssets, nullptr);
}

CreativePreviewCpuGeometry buildCreativePreviewCpuGeometry(
    StaticMeshAssetCache* staticMeshAssets) {
  CreativePreviewCpuGeometry result;
  result.indexedDraws.resize(kCreativePreviewGeometryDrawRangeCount);
  constexpr std::array<std::string_view, kRenderCreativePreviewRoleCount>
      roles{
          "editor_ghost_select",
          "editor_ghost_valid",
          "editor_ghost_invalid",
          "editor_ghost_route",
      };
  for (std::size_t profileSlot = 0;
       profileSlot < kCreativePreviewGeometryProfileSlotCount; ++profileSlot) {
    RenderCreativePreviewGeometryProfile profile =
        RenderCreativePreviewGeometryProfile::Box;
    std::uint16_t proceduralSegmentCount = 0U;
    if (profileSlot == 1U) {
      profile = RenderCreativePreviewGeometryProfile::RampWedge;
    } else if (profileSlot == 2U) {
      profile = RenderCreativePreviewGeometryProfile::OpenFrame;
    } else if (profileSlot >= 3U) {
      profile = RenderCreativePreviewGeometryProfile::StairSteps;
      proceduralSegmentCount =
          static_cast<std::uint16_t>(profileSlot - 2U);
    }

    for (std::size_t roleIndex = 0; roleIndex < roles.size(); ++roleIndex) {
      const RenderCreativePreviewRole role =
          static_cast<RenderCreativePreviewRole>(roleIndex);
      const std::uint32_t firstIndex =
          static_cast<std::uint32_t>(result.indices.size());
      std::vector<IndexedDrawRange> componentDraws;
      const Vec3 color = colorForRoomRole(std::string(roles[roleIndex]));
      const Vec3 solidSize =
          role == RenderCreativePreviewRole::Held
              ? Vec3{1.0F, 1.0F, 1.0F}
              : Vec3{0.96F, 0.96F, 0.96F};
      if (!appendCreativeGeneratedPreviewShape(
              result.vertices, result.indices, componentDraws, profile,
              proceduralSegmentCount, solidSize, color)) {
        return result;
      }
      if (role == RenderCreativePreviewRole::Held) {
        result.indexedDraws[creativePreviewGeometryDrawIndex(
            role, false, profile, proceduralSegmentCount)] = {
            firstIndex,
            static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
        continue;
      }

      appendCreativeTargetWireframe(result.vertices, result.indices,
                                    componentDraws, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(
          role, false, profile, proceduralSegmentCount)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
      appendCreativePathWireframe(result.vertices, result.indices,
                                  componentDraws, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(
          role, true, profile, proceduralSegmentCount)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
    }
  }

  if (staticMeshAssets != nullptr) {
    const StaticMeshAssetCatalog catalog =
        discoverStaticMeshAssetCatalog(staticMeshAssets->root());
    result.assetDraws.reserve(catalog.entries.size());
    for (const StaticMeshAssetCatalogEntry& entry : catalog.entries) {
      const StaticMeshAsset* asset = staticMeshAssets->find(entry.assetId);
      if (asset == nullptr) {
        continue;
      }
      const std::size_t vertexStart = result.vertices.size();
      const std::size_t indexStart = result.indices.size();
      const std::size_t drawStart = result.indexedDraws.size();
      CreativePreviewGeometryResources::AssetDrawRanges assetDraw;
      assetDraw.assetId = entry.assetId;
      bool complete = true;
      for (std::size_t roleIndex = 0;
           roleIndex < kRenderCreativePreviewRoleCount; ++roleIndex) {
        const RenderCreativePreviewRole role =
            static_cast<RenderCreativePreviewRole>(roleIndex);
        const std::string_view roleName = roles[roleIndex];
        IndexedDrawRange draw;
        std::vector<IndexedDrawRange> componentDraws;
        if (!appendStaticMeshPreviewRole(
                result.vertices, result.indices, componentDraws, *asset, role,
                colorForRoomRole(std::string(roleName)), draw)) {
          complete = false;
          break;
        }
        assetDraw.geometryDrawIndices[roleIndex] =
            static_cast<std::uint32_t>(result.indexedDraws.size());
        result.indexedDraws.push_back(draw);
      }
      if (!complete) {
        result.vertices.resize(vertexStart);
        result.indices.resize(indexStart);
        result.indexedDraws.resize(drawStart);
        continue;
      }
      result.assetDraws.push_back(std::move(assetDraw));
    }
  }
  result.ready = !result.vertices.empty() && !result.indices.empty();
  return result;
}

CreativePreviewCpuGeometry buildCreativePreviewCpuGeometry() {
  return buildCreativePreviewCpuGeometry(nullptr);
}

std::uint32_t resolveCreativePreviewGeometryDrawIndex(
    const CreativePreviewGeometryResources& geometry,
    const RenderCreativePreviewItem& item) noexcept {
  const std::string_view assetId = renderCreativePreviewAssetId(item);
  if (!assetId.empty()) {
    const auto found = std::find_if(
        geometry.assetDraws.begin(), geometry.assetDraws.end(),
        [assetId](const CreativePreviewGeometryResources::AssetDrawRanges& draw) {
          return draw.assetId == assetId;
        });
    const std::size_t roleIndex = static_cast<std::size_t>(item.role);
    if (found != geometry.assetDraws.end() &&
        roleIndex < found->geometryDrawIndices.size()) {
      return found->geometryDrawIndices[roleIndex];
    }
  }
  return creativePreviewGeometryDrawIndex(item.role,
                                          item.includePathWireframe,
                                          item.geometryProfile,
                                          item.proceduralSegmentCount);
}

std::uint64_t room_mesh_detail::roomGeometrySignature(
    const SceneRoomProjection& room) {
  return ::iggy3d::vulkan::roomGeometrySignature(room);
}

std::uint64_t room_mesh_detail::creativeWireframeDebugGeometrySignature(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  return ::iggy3d::vulkan::creativeWireframeDebugGeometrySignature(
      creativeWireframeDebug);
}

void room_mesh_detail::appendCreativeWireframeDebugReceiptFields(
    RenderReceipt& receipt, const FirstRoomGeometryResources& geometry) {
  ::iggy3d::vulkan::appendCreativeWireframeDebugReceiptFields(receipt,
                                                              geometry);
}

}  // namespace iggy3d::vulkan
