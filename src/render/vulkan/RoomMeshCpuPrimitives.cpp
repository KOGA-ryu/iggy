#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include "content/assets/GeneratedGeometry.hpp"
#include "core/math/EulerRotation.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace iggy3d::vulkan::room_mesh_detail {

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
                     Vec3 rotationEulerRadians) {
  if (!canAppendBox(vertices)) {
    return false;
  }
  appendBox(vertices, indices, draws, center, size, color,
            rotationEulerRadians);
  return true;
}

bool appendRampWedgeIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians) {
  return appendRampWedgeIfFits<std::uint16_t>(
      vertices, indices, draws, center, size, color, rotationEulerRadians);
}

bool appendOpenFrameIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians) {
  return appendOpenFrameIfFits<std::uint16_t>(
      vertices, indices, draws, center, size, color, rotationEulerRadians);
}

bool appendStairStepsIfFits(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& draws,
    Vec3 center,
    Vec3 size,
    Vec3 color,
    Vec3 rotationEulerRadians,
    std::uint16_t segmentCount) {
  return appendStairStepsIfFits<std::uint16_t>(
      vertices, indices, draws, center, size, color, rotationEulerRadians,
      segmentCount);
}

void appendCreativeTargetWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color) {
  appendCreativeTargetWireframe<std::uint32_t>(
      vertices, indices, componentDraws, color);
}

void appendCreativePathWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color) {
  appendCreativePathWireframe<std::uint32_t>(
      vertices, indices, componentDraws, color);
}

}  // namespace iggy3d::vulkan::room_mesh_detail
