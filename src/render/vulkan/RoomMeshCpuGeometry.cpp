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

using room_mesh_detail::appendStaticMeshAsset;
using room_mesh_detail::buildOptimizedFloorDraws;
using room_mesh_detail::buildOptimizedWallDraws;
using room_mesh_detail::canEmitFloorDraw;
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

Vec3 colorForRoomRole(const std::string& role) {
  if (role == "editor_ghost_valid") {
    return {0.20F, 0.82F, 0.48F};
  }
  if (role == "editor_ghost_invalid") {
    return {0.92F, 0.24F, 0.20F};
  }
  if (role == "editor_ghost_select") {
    return {0.95F, 0.86F, 0.28F};
  }
  if (role == "editor_ghost_route") {
    return {0.18F, 0.78F, 0.95F};
  }
  if (role == "floor") {
    return {0.30F, 0.32F, 0.34F};
  }
  if (role == "terrain") {
    return {0.24F, 0.46F, 0.22F};
  }
  if (role == "terrain_grass") {
    return {0.22F, 0.52F, 0.20F};
  }
  if (role == "terrain_dirt") {
    return {0.42F, 0.25F, 0.10F};
  }
  if (role == "terrain_stone") {
    return {0.42F, 0.44F, 0.46F};
  }
  if (role == "terrain_sand") {
    return {0.78F, 0.68F, 0.38F};
  }
  if (role == "wall") {
    return {0.42F, 0.43F, 0.46F};
  }
  if (role == "opening") {
    return {0.56F, 0.56F, 0.60F};
  }
  if (role == "prop") {
    return {0.45F, 0.28F, 0.12F};
  }
  if (role == "grid") {
    return {0.78F, 0.82F, 0.86F};
  }
  if (role == "ledge") {
    return {0.30F, 0.52F, 0.70F};
  }
  if (role == "rail") {
    return {0.88F, 0.74F, 0.28F};
  }
  if (role == "hazard" || role == "dash") {
    return {0.72F, 0.20F, 0.18F};
  }
  if (role == "spell") {
    return {0.34F, 0.62F, 0.88F};
  }
  if (role == "npc_gaze_perceived") {
    return {0.24F, 0.78F, 0.36F};
  }
  if (role == "npc_gaze_blocked") {
    return {0.94F, 0.78F, 0.24F};
  }
  if (role == "npc_gaze_scan") {
    return {0.48F, 0.54F, 0.58F};
  }
  if (role == "bean_player") {
    return {0.22F, 0.56F, 0.92F};
  }
  if (role == "bean_npc") {
    return {0.84F, 0.68F, 0.24F};
  }
  if (role == "bean_codex_probe") {
    return {0.72F, 0.38F, 0.92F};
  }
  return {0.36F, 0.42F, 0.48F};
}

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

template <typename Index>
void appendTriangle(std::vector<Index>& indices,
                    std::uint32_t a,
                    std::uint32_t b,
                    std::uint32_t c);

bool canAppendPlane(const std::vector<FirstRoomVertex>& vertices) {
  return vertices.size() + 4U <=
         static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max());
}

void appendFloorPlane(std::vector<FirstRoomVertex>& vertices,
                      std::vector<std::uint16_t>& indices,
                      std::vector<IndexedDrawRange>& draws,
                      Vec3 center,
                      Vec3 size,
                      Vec3 color) {
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
  const float hx = std::max(size.x * 0.5F, 0.001F);
  const float hz = std::max(size.z * 0.5F, 0.001F);
  const float y = center.y + std::max(size.y * 0.5F, 0.001F);
  const FirstRoomVertex planeVertices[4] = {
      {{center.x - hx, y, center.z - hz}, {color.x, color.y, color.z}},
      {{center.x + hx, y, center.z - hz}, {color.x, color.y, color.z}},
      {{center.x + hx, y, center.z + hz}, {color.x, color.y, color.z}},
      {{center.x - hx, y, center.z + hz}, {color.x, color.y, color.z}},
  };
  vertices.insert(vertices.end(), std::begin(planeVertices), std::end(planeVertices));
  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  appendTriangle(indices, base + 0U, base + 1U, base + 2U);
  appendTriangle(indices, base + 0U, base + 2U, base + 3U);
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
}

bool appendFloorPlaneIfFits(std::vector<FirstRoomVertex>& vertices,
                            std::vector<std::uint16_t>& indices,
                            std::vector<IndexedDrawRange>& draws,
                            Vec3 center,
                            Vec3 size,
                            Vec3 color) {
  if (!canAppendPlane(vertices)) {
    return false;
  }
  appendFloorPlane(vertices, indices, draws, center, size, color);
  return true;
}

bool appendSurfacePatches(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    std::span<const SceneRoomSurfacePatchItem> patches) {
  if (patches.empty()) {
    return true;
  }
  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  for (const SceneRoomSurfacePatchItem& patch : patches) {
    if (vertices.size() + patch.corners.size() + 1U >
        static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
      return false;
    }
    if (!finiteVec3(patch.center)) {
      return false;
    }
    for (const Vec3 corner : patch.corners) {
      if (!finiteVec3(corner)) {
        return false;
      }
    }
    const Vec3 color = colorForRoomRole(patch.role);
    const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
    vertices.push_back({{patch.center.x, patch.center.y, patch.center.z},
                        {color.x, color.y, color.z}});
    for (const Vec3 corner : patch.corners) {
      vertices.push_back(
          {{corner.x, corner.y, corner.z}, {color.x, color.y, color.z}});
    }
    appendTriangle(indices, base + 0U, base + 1U, base + 2U);
    appendTriangle(indices, base + 0U, base + 2U, base + 3U);
    appendTriangle(indices, base + 0U, base + 3U, base + 4U);
    appendTriangle(indices, base + 0U, base + 4U, base + 1U);
  }
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
  return true;
}

bool canAppendSurfacePatches(
    const std::vector<FirstRoomVertex>& vertices,
    std::span<const SceneRoomSurfacePatchItem> patches) {
  if (patches.empty()) {
    return false;
  }
  constexpr std::size_t maximumVertexCount =
      static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max());
  if (vertices.size() > maximumVertexCount ||
      patches.size() > (maximumVertexCount - vertices.size()) / 5U) {
    return false;
  }
  return std::all_of(
      patches.begin(), patches.end(), [](const SceneRoomSurfacePatchItem& patch) {
        return finiteVec3(patch.center) &&
               std::all_of(patch.corners.begin(), patch.corners.end(),
                           [](Vec3 corner) { return finiteVec3(corner); });
      });
}

template <typename Index>
void appendTriangle(std::vector<Index>& indices,
                    std::uint32_t a,
                    std::uint32_t b,
                    std::uint32_t c) {
  indices.push_back(static_cast<Index>(a));
  indices.push_back(static_cast<Index>(b));
  indices.push_back(static_cast<Index>(c));
  indices.push_back(static_cast<Index>(c));
  indices.push_back(static_cast<Index>(b));
  indices.push_back(static_cast<Index>(a));
}

template <typename Index>
void appendBox(std::vector<FirstRoomVertex>& vertices,
               std::vector<Index>& indices,
               std::vector<IndexedDrawRange>& draws,
               Vec3 center,
               Vec3 size,
               Vec3 color,
               Vec3 rotationEulerRadians = {}) {
  const Index base = static_cast<Index>(vertices.size());
  const float hx = std::max(size.x * 0.5F, 0.001F);
  const float hy = std::max(size.y * 0.5F, 0.001F);
  const float hz = std::max(size.z * 0.5F, 0.001F);
  const Vec3 local[8] = {
      {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz},
      {-hx, -hy, hz},  {hx, -hy, hz},  {hx, hy, hz},  {-hx, hy, hz},
  };
  FirstRoomVertex boxVertices[8]{};
  for (std::size_t index = 0; index < std::size(local); ++index) {
    const Vec3 position =
        center + rotateEulerXyz(local[index], rotationEulerRadians);
    boxVertices[index] = FirstRoomVertex{
        {position.x, position.y, position.z},
        {color.x, color.y, color.z}};
  }
  vertices.insert(vertices.end(), std::begin(boxVertices), std::end(boxVertices));
  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  appendTriangle(indices, base + 0U, base + 1U, base + 2U);
  appendTriangle(indices, base + 0U, base + 2U, base + 3U);
  appendTriangle(indices, base + 4U, base + 6U, base + 5U);
  appendTriangle(indices, base + 4U, base + 7U, base + 6U);
  appendTriangle(indices, base + 0U, base + 3U, base + 7U);
  appendTriangle(indices, base + 0U, base + 7U, base + 4U);
  appendTriangle(indices, base + 1U, base + 5U, base + 6U);
  appendTriangle(indices, base + 1U, base + 6U, base + 2U);
  appendTriangle(indices, base + 3U, base + 2U, base + 6U);
  appendTriangle(indices, base + 3U, base + 6U, base + 7U);
  appendTriangle(indices, base + 0U, base + 4U, base + 5U);
  appendTriangle(indices, base + 0U, base + 5U, base + 1U);
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
}

template <typename Index>
bool appendRampWedgeIfFits(std::vector<FirstRoomVertex>& vertices,
                           std::vector<Index>& indices,
                           std::vector<IndexedDrawRange>& draws,
                           Vec3 center,
                           Vec3 size,
                           Vec3 color,
                           Vec3 rotationEulerRadians) {
  const std::size_t maximumVertexCount =
      static_cast<std::size_t>(std::numeric_limits<Index>::max());
  if (vertices.size() > maximumVertexCount - 6U ||
      !finiteVec3(center) || !finiteVec3(rotationEulerRadians) ||
      !finitePositive(size.x) || !finitePositive(size.y) ||
      !finitePositive(size.z)) {
    return false;
  }
  const float hx = size.x * 0.5F;
  const float hy = size.y * 0.5F;
  const float hz = size.z * 0.5F;
  const Vec3 local[6] = {
      {-hx, -hy, -hz}, {hx, -hy, -hz}, {-hx, -hy, hz},
      {hx, -hy, hz},   {-hx, hy, hz},   {hx, hy, hz},
  };
  const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
  for (const Vec3 point : local) {
    const Vec3 position = center + rotateEulerXyz(point, rotationEulerRadians);
    vertices.push_back({{position.x, position.y, position.z},
                        {color.x, color.y, color.z}});
  }
  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  appendTriangle(indices, base + 0U, base + 1U, base + 3U);
  appendTriangle(indices, base + 0U, base + 3U, base + 2U);
  appendTriangle(indices, base + 2U, base + 3U, base + 5U);
  appendTriangle(indices, base + 2U, base + 5U, base + 4U);
  appendTriangle(indices, base + 0U, base + 4U, base + 5U);
  appendTriangle(indices, base + 0U, base + 5U, base + 1U);
  appendTriangle(indices, base + 0U, base + 2U, base + 4U);
  appendTriangle(indices, base + 1U, base + 5U, base + 3U);
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
  return true;
}

template <typename Index>
bool appendOpenFrameIfFits(std::vector<FirstRoomVertex>& vertices,
                           std::vector<Index>& indices,
                           std::vector<IndexedDrawRange>& draws,
                           Vec3 center,
                           Vec3 size,
                           Vec3 color,
                           Vec3 rotationEulerRadians) {
  constexpr std::size_t kPartVertexCount = 8U;
  const std::size_t maximumVertexCount =
      static_cast<std::size_t>(std::numeric_limits<Index>::max());
  if (vertices.size() > maximumVertexCount ||
      kGeneratedOpenFramePartCount >
          (maximumVertexCount - vertices.size()) / kPartVertexCount ||
      !finiteVec3(center) || !finiteVec3(rotationEulerRadians)) {
    return false;
  }
  const GeneratedOpenFrameLayout layout = generatedOpenFrameLayout(size);
  if (!layout.valid) {
    return false;
  }

  for (const GeneratedOpenFramePart& part : layout.parts) {
    appendBox(vertices, indices, draws,
              center + rotateEulerXyz(part.center, rotationEulerRadians),
              part.size, color, rotationEulerRadians);
  }
  return true;
}

template <typename Index>
bool appendStairStepsIfFits(std::vector<FirstRoomVertex>& vertices,
                            std::vector<Index>& indices,
                            std::vector<IndexedDrawRange>& draws,
                            Vec3 center,
                            Vec3 size,
                            Vec3 color,
                            Vec3 rotationEulerRadians,
                            std::uint16_t segmentCount) {
  constexpr std::size_t kBoxVertexCount = 8U;
  const std::size_t maximumVertexCount =
      static_cast<std::size_t>(std::numeric_limits<Index>::max());
  if (segmentCount == 0U || vertices.size() > maximumVertexCount ||
      static_cast<std::size_t>(segmentCount) >
          (maximumVertexCount - vertices.size()) / kBoxVertexCount ||
      !finiteVec3(center) || !finiteVec3(rotationEulerRadians) ||
      !finitePositive(size.x) || !finitePositive(size.y) ||
      !finitePositive(size.z)) {
    return false;
  }
  const float inverseCount = 1.0F / static_cast<float>(segmentCount);
  const float segmentDepth = size.z * inverseCount;
  const float segmentRise = size.y * inverseCount;
  for (std::uint16_t index = 0U; index < segmentCount; ++index) {
    const float height = segmentRise * static_cast<float>(index + 1U);
    const Vec3 localCenter{
        0.0F, -size.y * 0.5F + height * 0.5F,
        -size.z * 0.5F + segmentDepth * (static_cast<float>(index) + 0.5F)};
    appendBox(vertices, indices, draws,
              center + rotateEulerXyz(localCenter, rotationEulerRadians),
              {size.x, height, segmentDepth}, color, rotationEulerRadians);
  }
  return true;
}

template <typename Index>
void appendCreativeTargetWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<Index>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color);

template <typename Index>
void appendCreativeTargetWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<Index>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color) {
  constexpr float kWireThickness = 0.015F;
  constexpr float kWireCenter =
      0.5F - kWireThickness * 0.5F;
  for (float first : {-kWireCenter, kWireCenter}) {
    for (float second : {-kWireCenter, kWireCenter}) {
      appendBox(vertices, indices, componentDraws, {0.0F, first, second},
                {1.0F, kWireThickness, kWireThickness}, color);
      appendBox(vertices, indices, componentDraws, {first, 0.0F, second},
                {kWireThickness, 1.0F, kWireThickness}, color);
      appendBox(vertices, indices, componentDraws, {first, second, 0.0F},
                {kWireThickness, kWireThickness, 1.0F}, color);
    }
  }
}

template <typename Index>
void appendCreativePathWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<Index>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color) {
  constexpr float kEndpoint = 1.0F / 2.16F;
  constexpr float kHorizontalThickness = 0.025F;
  constexpr float kVerticalThickness = 0.20F;
  appendBox(vertices, indices, componentDraws,
            {0.0F, 0.0F, -kEndpoint},
            {kEndpoint * 2.0F, kVerticalThickness,
             kHorizontalThickness},
            color);
  appendBox(vertices, indices, componentDraws,
            {kEndpoint, 0.0F, 0.0F},
            {kHorizontalThickness, kVerticalThickness,
             kEndpoint * 2.0F},
            color);
}

bool appendCreativeGeneratedPreviewShape(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    RenderCreativePreviewGeometryProfile profile,
    std::uint16_t proceduralSegmentCount,
    Vec3 size,
    Vec3 color) {
  switch (profile) {
    case RenderCreativePreviewGeometryProfile::Box:
      appendBox(vertices, indices, componentDraws, {}, size, color);
      return true;
    case RenderCreativePreviewGeometryProfile::RampWedge:
      return appendRampWedgeIfFits(vertices, indices, componentDraws, {}, size,
                                   color, {});
    case RenderCreativePreviewGeometryProfile::OpenFrame:
      return appendOpenFrameIfFits(vertices, indices, componentDraws, {}, size,
                                   color, {});
    case RenderCreativePreviewGeometryProfile::StairSteps:
      return appendStairStepsIfFits(vertices, indices, componentDraws, {}, size,
                                    color, {}, proceduralSegmentCount);
    case RenderCreativePreviewGeometryProfile::Count:
      return false;
  }
  return false;
}

bool canAppendBox(const std::vector<FirstRoomVertex>& vertices) {
  return vertices.size() + 8U <=
         static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max());
}

bool appendBoxIfFits(std::vector<FirstRoomVertex>& vertices,
                     std::vector<std::uint16_t>& indices,
                     std::vector<IndexedDrawRange>& draws,
                     Vec3 center,
                     Vec3 size,
                     Vec3 color,
                     Vec3 rotationEulerRadians = {}) {
  if (!canAppendBox(vertices)) {
    return false;
  }
  appendBox(vertices, indices, draws, center, size, color,
            rotationEulerRadians);
  return true;
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
    if (!canAppendBox(result.vertices)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    appendBox(result.vertices, result.indices, result.indexedDraws,
              wall.position, wall.size, colorForRoomRole("wall"),
              wall.rotationEulerRadians);
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
      if (!canAppendBox(result.vertices)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      appendBox(result.vertices, result.indices, result.indexedDraws,
                mesh.position, mesh.size, colorForRoomRole(mesh.role),
                mesh.rotationEulerRadians);
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
