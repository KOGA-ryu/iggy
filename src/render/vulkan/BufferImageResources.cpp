#include "render/vulkan/BufferImageResources.hpp"

#include "core/math/EulerRotation.hpp"
#include "render/mesh/BeanMesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

constexpr float kRoomMeshEpsilon = 0.0001F;
constexpr float kRoomMeshQuantizeScale = 10000.0F;

RenderReceipt baseReceipt(std::string_view result, std::string_view reasonCode) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/BufferImageResources.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "memory_allocator", "manual_packet6_bootstrap");
  appendReceiptField(receipt, "vertex_buffer_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "index_buffer_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "depth_image_created", false);
  appendReceiptField(receipt, "per_frame_allocation_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_input_line_count",
                     static_cast<std::uint64_t>(0));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_draw_count",
                     static_cast<std::uint64_t>(0));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_box_count",
                     static_cast<std::uint64_t>(0));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_skipped_count",
                     static_cast<std::uint64_t>(0));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_status",
                     "vulkan_creative_wireframe_debug_geometry_not_requested");
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_reason_code",
                     "vulkan_creative_wireframe_debug_geometry_not_requested");
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

#if defined(IGGY3D_HAS_VULKAN)
bool copyBuffer(VkDevice device,
                VkQueue queue,
                std::uint32_t queueFamily,
                VkBuffer source,
                VkBuffer destination,
                VkDeviceSize size) {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  VkCommandPool pool = VK_NULL_HANDLE;
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
    return false;
  }
  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = pool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1U;
  VkCommandBuffer command = VK_NULL_HANDLE;
  if (vkAllocateCommandBuffers(device, &allocateInfo, &command) != VK_SUCCESS) {
    vkDestroyCommandPool(device, pool, nullptr);
    return false;
  }
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(command, &beginInfo) != VK_SUCCESS) {
    vkDestroyCommandPool(device, pool, nullptr);
    return false;
  }
  VkBufferCopy region{};
  region.size = size;
  vkCmdCopyBuffer(command, source, destination, 1U, &region);
  if (vkEndCommandBuffer(command) != VK_SUCCESS) {
    vkDestroyCommandPool(device, pool, nullptr);
    return false;
  }
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence = VK_NULL_HANDLE;
  if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
    vkDestroyCommandPool(device, pool, nullptr);
    return false;
  }
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1U;
  submitInfo.pCommandBuffers = &command;
  const bool submitted = vkQueueSubmit(queue, 1U, &submitInfo, fence) == VK_SUCCESS;
  const bool completed =
      submitted && vkWaitForFences(device, 1U, &fence, VK_TRUE, 1'000'000'000ULL) == VK_SUCCESS;
  vkDestroyFence(device, fence, nullptr);
  vkDestroyCommandPool(device, pool, nullptr);
  return completed;
}

bool uploadBuffer(VulkanMemoryAllocator& allocator,
                  VkDevice device,
                  VkQueue queue,
                  std::uint32_t queueFamily,
                  std::string_view stagingName,
                  std::string_view resourceName,
                  VkDeviceSize byteCount,
                  VkBufferUsageFlags usage,
                  const void* bytes,
                  GpuBufferRecord& out) {
  VulkanAllocationResult staging =
      allocator.createBuffer(stagingName, byteCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             bytes);
  if (staging.outcome != RenderOutcome::Ok) {
    return false;
  }
  VulkanAllocationResult destination =
      allocator.createBuffer(resourceName, byteCount, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (destination.outcome != RenderOutcome::Ok) {
    allocator.destroyBuffer(staging.buffer);
    return false;
  }
  const bool copied =
      copyBuffer(device, queue, queueFamily, staging.buffer.buffer, destination.buffer.buffer,
                 byteCount);
  allocator.destroyBuffer(staging.buffer);
  if (!copied) {
    allocator.destroyBuffer(destination.buffer);
    return false;
  }
  out.allocation = destination.buffer;
  out.allocationName = std::string(resourceName);
  return true;
}
#endif

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

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= kRoomMeshEpsilon;
}

std::int64_t quantized(float value) {
  return static_cast<std::int64_t>(std::llround(value * kRoomMeshQuantizeScale));
}

bool finitePositive(float value) {
  return std::isfinite(value) && value > kRoomMeshEpsilon;
}

bool finiteVec3(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool hasRotation(Vec3 rotationEulerRadians) {
  return finiteVec3(rotationEulerRadians) &&
         (!near(rotationEulerRadians.x, 0.0F) ||
          !near(rotationEulerRadians.y, 0.0F) ||
          !near(rotationEulerRadians.z, 0.0F));
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

void appendTriangle(std::vector<std::uint16_t>& indices,
                    std::uint16_t a,
                    std::uint16_t b,
                    std::uint16_t c);
bool finiteVec3(Vec3 value);

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

void appendTriangle(std::vector<std::uint16_t>& indices,
                    std::uint16_t a,
                    std::uint16_t b,
                    std::uint16_t c) {
  indices.push_back(a);
  indices.push_back(b);
  indices.push_back(c);
  indices.push_back(c);
  indices.push_back(b);
  indices.push_back(a);
}

void appendBox(std::vector<FirstRoomVertex>& vertices,
               std::vector<std::uint16_t>& indices,
               std::vector<IndexedDrawRange>& draws,
               Vec3 center,
               Vec3 size,
               Vec3 color,
               Vec3 rotationEulerRadians = {}) {
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
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

void appendCreativeTargetPreview(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    Vec3 color) {
  constexpr float kSolidInset = 0.96F;
  constexpr float kWireThickness = 0.015F;
  constexpr float kWireCenter =
      0.5F - kWireThickness * 0.5F;
  appendBox(vertices, indices, componentDraws, {},
            {kSolidInset, kSolidInset, kSolidInset}, color);
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

void appendCreativePathWireframe(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint16_t>& indices,
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
                          Vec3& color) {
  if (!finiteVec3(line.start) || !finiteVec3(line.end) ||
      !std::isfinite(line.thickness) || line.thickness <= 0.0F ||
      !std::isfinite(line.color.r) || !std::isfinite(line.color.g) ||
      !std::isfinite(line.color.b)) {
    return false;
  }

  const float dx = line.end.x - line.start.x;
  const float dy = line.end.y - line.start.y;
  const float dz = line.end.z - line.start.z;
  const bool movesX = !near(dx, 0.0F);
  const bool movesY = !near(dy, 0.0F);
  const bool movesZ = !near(dz, 0.0F);
  const std::uint8_t movedAxisCount =
      static_cast<std::uint8_t>(movesX ? 1U : 0U) +
      static_cast<std::uint8_t>(movesY ? 1U : 0U) +
      static_cast<std::uint8_t>(movesZ ? 1U : 0U);
  if (movedAxisCount != 1U) {
    return false;
  }

  center = {(line.start.x + line.end.x) * 0.5F,
            (line.start.y + line.end.y) * 0.5F,
            (line.start.z + line.end.z) * 0.5F};
  const float thickness = std::max(line.thickness, 0.001F);
  if (movesX) {
    size = {std::fabs(dx), thickness, thickness};
  } else if (movesY) {
    size = {thickness, std::fabs(dy), thickness};
  } else {
    size = {thickness, thickness, std::fabs(dz)};
  }
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

[[nodiscard]] bool externalStaticMeshId(std::string_view meshId,
                                        std::string_view& assetId) noexcept {
  constexpr std::string_view prefix = "asset:";
  if (!meshId.starts_with(prefix) || meshId.size() == prefix.size()) {
    return false;
  }
  assetId = meshId.substr(prefix.size());
  return true;
}

[[nodiscard]] Vec3 componentProduct(Vec3 lhs, Vec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] Vec3 importedTriangleColor(
    const StaticMeshAsset& asset,
    std::uint32_t materialIndex,
    Vec3 worldNormal) noexcept {
  Vec3 base{0.48F, 0.44F, 0.38F};
  if (materialIndex < asset.materials.size()) {
    const StaticMeshMaterial& material = asset.materials[materialIndex];
    if (std::isfinite(material.baseColorFactor[0]) &&
        std::isfinite(material.baseColorFactor[1]) &&
        std::isfinite(material.baseColorFactor[2])) {
      base = {std::clamp(material.baseColorFactor[0], 0.0F, 1.0F),
              std::clamp(material.baseColorFactor[1], 0.0F, 1.0F),
              std::clamp(material.baseColorFactor[2], 0.0F, 1.0F)};
    }
  }
  const Vec3 light = normalized(Vec3{-0.35F, 0.85F, 0.40F});
  const float brightness =
      0.52F + 0.48F * std::max(0.0F, dot(worldNormal, light));
  return base * brightness;
}

bool appendStaticMeshAsset(std::vector<FirstRoomVertex>& vertices,
                           std::vector<std::uint16_t>& indices,
                           std::vector<IndexedDrawRange>& draws,
                           const SceneRoomMeshItem& item,
                           const StaticMeshAsset& asset) {
  const Vec3 inputSize = asset.boundsMax - asset.boundsMin;
  if (!asset.hasBounds || !finiteVec3(inputSize) ||
      inputSize.x <= 0.0F || inputSize.y <= 0.0F || inputSize.z <= 0.0F ||
      !finiteVec3(item.position) || !finiteVec3(item.size) ||
      item.size.x <= 0.0F || item.size.y <= 0.0F || item.size.z <= 0.0F) {
    return false;
  }
  const Vec3 inputCenter = (asset.boundsMin + asset.boundsMax) * 0.5F;
  const Vec3 scale{item.size.x / inputSize.x, item.size.y / inputSize.y,
                   item.size.z / inputSize.z};
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    if (primitive.indexCount == 0U || primitive.indexCount % 3U != 0U ||
        primitive.firstIndex > asset.indices.size() ||
        primitive.indexCount > asset.indices.size() - primitive.firstIndex ||
        vertices.size() + primitive.indexCount >
            static_cast<std::size_t>(
                std::numeric_limits<std::uint16_t>::max())) {
      return false;
    }
    IndexedDrawRange draw;
    draw.firstIndex = static_cast<std::uint32_t>(indices.size());
    for (std::uint32_t triangle = 0U; triangle < primitive.indexCount;
         triangle += 3U) {
      Vec3 world[3]{};
      for (std::uint32_t corner = 0U; corner < 3U; ++corner) {
        const std::uint32_t sourceIndex =
            asset.indices[primitive.firstIndex + triangle + corner];
        if (sourceIndex >= asset.vertices.size()) {
          return false;
        }
        const Vec3 local = componentProduct(
            asset.vertices[sourceIndex].position - inputCenter, scale);
        world[corner] = item.position +
                        rotateEulerXyz(local, item.rotationEulerRadians);
        if (!finiteVec3(world[corner])) {
          return false;
        }
      }
      const Vec3 faceNormal =
          normalized(cross(world[1] - world[0], world[2] - world[0]));
      const Vec3 color =
          importedTriangleColor(asset, primitive.materialIndex, faceNormal);
      for (Vec3 position : world) {
        const std::uint16_t index =
            static_cast<std::uint16_t>(vertices.size());
        vertices.push_back({{position.x, position.y, position.z},
                            {color.x, color.y, color.z}});
        indices.push_back(index);
      }
    }
    draw.indexCount =
        static_cast<std::uint32_t>(indices.size()) - draw.firstIndex;
    draws.push_back(draw);
  }
  return true;
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

struct FloorMergeKey {
  std::string materialId;
  std::int64_t positionY = 0;
  std::int64_t sizeX = 0;
  std::int64_t sizeY = 0;
  std::int64_t sizeZ = 0;

  bool operator<(const FloorMergeKey& rhs) const {
    return std::tie(materialId, positionY, sizeX, sizeY, sizeZ) <
           std::tie(rhs.materialId, rhs.positionY, rhs.sizeX, rhs.sizeY, rhs.sizeZ);
  }
};

struct FloorCell {
  std::int64_t x = 0;
  std::int64_t z = 0;
  Vec3 position;
  Vec3 size;

  bool operator<(const FloorCell& rhs) const {
    return std::tie(z, x) < std::tie(rhs.z, rhs.x);
  }
};

struct FloorDraw {
  Vec3 position;
  Vec3 size;
  Vec3 rotationEulerRadians;
};

bool floorToGridCell(const SceneRoomMeshItem& mesh,
                     FloorMergeKey& key,
                     FloorCell& cell) {
  if (mesh.role != "floor" || !std::isfinite(mesh.position.x) ||
      !std::isfinite(mesh.position.y) || !std::isfinite(mesh.position.z) ||
      !finitePositive(mesh.size.x) || !finitePositive(mesh.size.y) ||
      !finitePositive(mesh.size.z) || hasRotation(mesh.rotationEulerRadians)) {
    return false;
  }

  const float gridX = mesh.position.x / mesh.size.x;
  const float gridZ = mesh.position.z / mesh.size.z;
  const auto roundedX = static_cast<std::int64_t>(std::llround(gridX));
  const auto roundedZ = static_cast<std::int64_t>(std::llround(gridZ));
  if (!near(gridX, static_cast<float>(roundedX)) ||
      !near(gridZ, static_cast<float>(roundedZ))) {
    return false;
  }

  key.materialId = mesh.materialId;
  key.positionY = quantized(mesh.position.y);
  key.sizeX = quantized(mesh.size.x);
  key.sizeY = quantized(mesh.size.y);
  key.sizeZ = quantized(mesh.size.z);
  cell.x = roundedX;
  cell.z = roundedZ;
  cell.position = mesh.position;
  cell.size = mesh.size;
  return true;
}

void appendFloorRectsForGroup(const std::vector<FloorCell>& cells,
                              std::vector<FloorDraw>& floorDraws) {
  std::map<std::pair<std::int64_t, std::int64_t>, FloorCell> remaining;
  std::vector<FloorCell> duplicates;
  for (const FloorCell& cell : cells) {
    const auto key = std::make_pair(cell.x, cell.z);
    if (!remaining.emplace(key, cell).second) {
      duplicates.push_back(cell);
    }
  }

  for (const FloorCell& duplicate : duplicates) {
    floorDraws.push_back({duplicate.position, duplicate.size, {}});
  }

  while (!remaining.empty()) {
    const FloorCell origin = remaining.begin()->second;
    std::int64_t width = 1;
    while (remaining.contains({origin.x + width, origin.z})) {
      ++width;
    }

    std::int64_t height = 1;
    bool canGrow = true;
    while (canGrow) {
      for (std::int64_t dx = 0; dx < width; ++dx) {
        if (!remaining.contains({origin.x + dx, origin.z + height})) {
          canGrow = false;
          break;
        }
      }
      if (canGrow) {
        ++height;
      }
    }

    FloorDraw draw;
    draw.size = {static_cast<float>(width) * origin.size.x,
                 origin.size.y,
                 static_cast<float>(height) * origin.size.z};
    draw.position = {
        origin.position.x + (static_cast<float>(width - 1) * origin.size.x * 0.5F),
        origin.position.y,
        origin.position.z + (static_cast<float>(height - 1) * origin.size.z * 0.5F),
    };
    floorDraws.push_back(draw);

    for (std::int64_t dz = 0; dz < height; ++dz) {
      for (std::int64_t dx = 0; dx < width; ++dx) {
        remaining.erase({origin.x + dx, origin.z + dz});
      }
    }
  }
}

std::vector<FloorDraw> buildOptimizedFloorDraws(const SceneRoomProjection& room) {
  std::map<FloorMergeKey, std::vector<FloorCell>> groups;
  std::vector<FloorDraw> floorDraws;
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role != "floor") {
      continue;
    }
    std::string_view externalAssetId;
    if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
      continue;
    }
    FloorMergeKey key;
    FloorCell cell;
    if (!floorToGridCell(mesh, key, cell)) {
      floorDraws.push_back(
          {mesh.position, mesh.size, mesh.rotationEulerRadians});
      continue;
    }
    groups[key].push_back(cell);
  }

  for (const auto& [key, cells] : groups) {
    (void)key;
    appendFloorRectsForGroup(cells, floorDraws);
  }
  return floorDraws;
}

bool canEmitFloorDraw(const FloorDraw& floor) {
  return std::isfinite(floor.position.x) && std::isfinite(floor.position.y) &&
         std::isfinite(floor.position.z) && finitePositive(floor.size.x) &&
         finitePositive(floor.size.y) && finitePositive(floor.size.z) &&
         finiteVec3(floor.rotationEulerRadians);
}

struct WallBoxDraw {
  Vec3 position;
  Vec3 size;
  Vec3 rotationEulerRadians;
};

enum class WallRunOrientation : std::uint8_t {
  AlongX,
  AlongZ,
};

struct WallSegmentSource {
  WallRunOrientation orientation = WallRunOrientation::AlongX;
  std::string materialId;
  std::int64_t constantAxis = 0;
  std::int64_t endpointY = 0;
  std::int64_t bottomY = 0;
  std::int64_t height = 0;
  std::int64_t thickness = 0;
  float constantAxisMeters = 0.0F;
  float endpointYMeters = 0.0F;
  float bottomYMeters = 0.0F;
  float heightMeters = 0.0F;
  float thicknessMeters = 0.0F;
  float minCoordMeters = 0.0F;
  float maxCoordMeters = 0.0F;
};

struct WallRunKey {
  std::string materialId;
  WallRunOrientation orientation = WallRunOrientation::AlongX;
  std::int64_t constantAxis = 0;
  std::int64_t endpointY = 0;
  std::int64_t bottomY = 0;
  std::int64_t height = 0;
  std::int64_t thickness = 0;

  bool operator<(const WallRunKey& rhs) const {
    return std::tie(materialId, orientation, constantAxis, endpointY, bottomY,
                    height, thickness) <
           std::tie(rhs.materialId, rhs.orientation, rhs.constantAxis, rhs.endpointY,
                    rhs.bottomY, rhs.height, rhs.thickness);
  }
};

bool wallBoxFromSegment(const SceneRoomMeshItem& mesh, WallBoxDraw& draw) {
  if (!finiteVec3(mesh.wallStartMeters) || !finiteVec3(mesh.wallEndMeters) ||
      !std::isfinite(mesh.wallBottomY) || !finitePositive(mesh.wallHeightMeters) ||
      !finitePositive(mesh.wallThicknessMeters) ||
      !near(mesh.wallStartMeters.y, mesh.wallEndMeters.y)) {
    return false;
  }

  const float dx = mesh.wallEndMeters.x - mesh.wallStartMeters.x;
  const float dz = mesh.wallEndMeters.z - mesh.wallStartMeters.z;
  const bool runsAlongX = !near(dx, 0.0F) && near(dz, 0.0F);
  const bool runsAlongZ = near(dx, 0.0F) && !near(dz, 0.0F);
  if (!runsAlongX && !runsAlongZ) {
    return false;
  }

  const float length = runsAlongX ? std::fabs(dx) : std::fabs(dz);
  if (!finitePositive(length)) {
    return false;
  }

  draw.position = {(mesh.wallStartMeters.x + mesh.wallEndMeters.x) * 0.5F,
                   mesh.wallBottomY + mesh.wallHeightMeters * 0.5F,
                   (mesh.wallStartMeters.z + mesh.wallEndMeters.z) * 0.5F};
  draw.size = runsAlongX
                  ? Vec3{length, mesh.wallHeightMeters, mesh.wallThicknessMeters}
                  : Vec3{mesh.wallThicknessMeters, mesh.wallHeightMeters, length};
  draw.rotationEulerRadians = {};
  return true;
}

bool wallSegmentSourceFromMesh(const SceneRoomMeshItem& mesh, WallSegmentSource& source) {
  if (mesh.role != "wall" || !mesh.hasWallSegment ||
      !finiteVec3(mesh.wallStartMeters) || !finiteVec3(mesh.wallEndMeters) ||
      !std::isfinite(mesh.wallBottomY) || !finitePositive(mesh.wallHeightMeters) ||
      !finitePositive(mesh.wallThicknessMeters) ||
      !near(mesh.wallStartMeters.y, mesh.wallEndMeters.y)) {
    return false;
  }

  const float dx = mesh.wallEndMeters.x - mesh.wallStartMeters.x;
  const float dz = mesh.wallEndMeters.z - mesh.wallStartMeters.z;
  const bool runsAlongX = !near(dx, 0.0F) && near(dz, 0.0F);
  const bool runsAlongZ = near(dx, 0.0F) && !near(dz, 0.0F);
  if (!runsAlongX && !runsAlongZ) {
    return false;
  }

  source.orientation = runsAlongX ? WallRunOrientation::AlongX
                                  : WallRunOrientation::AlongZ;
  source.materialId = mesh.materialId;
  source.endpointYMeters = mesh.wallStartMeters.y;
  source.bottomYMeters = mesh.wallBottomY;
  source.heightMeters = mesh.wallHeightMeters;
  source.thicknessMeters = mesh.wallThicknessMeters;
  if (runsAlongX) {
    source.constantAxisMeters = mesh.wallStartMeters.z;
    source.minCoordMeters = std::min(mesh.wallStartMeters.x, mesh.wallEndMeters.x);
    source.maxCoordMeters = std::max(mesh.wallStartMeters.x, mesh.wallEndMeters.x);
  } else {
    source.constantAxisMeters = mesh.wallStartMeters.x;
    source.minCoordMeters = std::min(mesh.wallStartMeters.z, mesh.wallEndMeters.z);
    source.maxCoordMeters = std::max(mesh.wallStartMeters.z, mesh.wallEndMeters.z);
  }
  if (!finitePositive(source.maxCoordMeters - source.minCoordMeters)) {
    return false;
  }

  source.constantAxis = quantized(source.constantAxisMeters);
  source.endpointY = quantized(source.endpointYMeters);
  source.bottomY = quantized(source.bottomYMeters);
  source.height = quantized(source.heightMeters);
  source.thickness = quantized(source.thicknessMeters);
  return true;
}

bool wallBoxForMesh(const SceneRoomMeshItem& mesh, WallBoxDraw& draw) {
  if (mesh.hasWallSegment) {
    return wallBoxFromSegment(mesh, draw);
  }
  draw.position = mesh.position;
  draw.size = mesh.size;
  draw.rotationEulerRadians = mesh.rotationEulerRadians;
  return std::isfinite(draw.position.x) && std::isfinite(draw.position.y) &&
         std::isfinite(draw.position.z) && finitePositive(draw.size.x) &&
         finitePositive(draw.size.y) && finitePositive(draw.size.z) &&
         finiteVec3(draw.rotationEulerRadians);
}

WallBoxDraw wallBoxFromRun(const WallSegmentSource& run) {
  const float length = run.maxCoordMeters - run.minCoordMeters;
  WallBoxDraw draw;
  if (run.orientation == WallRunOrientation::AlongX) {
    draw.position = {(run.minCoordMeters + run.maxCoordMeters) * 0.5F,
                     run.bottomYMeters + run.heightMeters * 0.5F,
                     run.constantAxisMeters};
    draw.size = {length, run.heightMeters, run.thicknessMeters};
  } else {
    draw.position = {run.constantAxisMeters,
                     run.bottomYMeters + run.heightMeters * 0.5F,
                     (run.minCoordMeters + run.maxCoordMeters) * 0.5F};
    draw.size = {run.thicknessMeters, run.heightMeters, length};
  }
  return draw;
}

std::vector<WallBoxDraw> appendWallRunsForGroup(std::vector<WallSegmentSource> segments) {
  std::vector<WallBoxDraw> draws;
  std::sort(segments.begin(), segments.end(),
            [](const WallSegmentSource& lhs, const WallSegmentSource& rhs) {
              return std::tie(lhs.minCoordMeters, lhs.maxCoordMeters) <
                     std::tie(rhs.minCoordMeters, rhs.maxCoordMeters);
            });

  std::size_t index = 0;
  while (index < segments.size()) {
    WallSegmentSource run = segments[index];
    ++index;
    while (index < segments.size() && near(run.maxCoordMeters, segments[index].minCoordMeters)) {
      run.maxCoordMeters = segments[index].maxCoordMeters;
      ++index;
    }
    draws.push_back(wallBoxFromRun(run));
  }
  return draws;
}

bool buildOptimizedWallDraws(const SceneRoomProjection& room,
                             std::vector<WallBoxDraw>& wallDraws) {
  std::map<WallRunKey, std::vector<WallSegmentSource>> groups;
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role != "wall") {
      continue;
    }
    std::string_view externalAssetId;
    if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
      continue;
    }
    if (!mesh.hasWallSegment) {
      WallBoxDraw fallback;
      if (!wallBoxForMesh(mesh, fallback)) {
        return false;
      }
      wallDraws.push_back(fallback);
      continue;
    }

    WallSegmentSource segment;
    if (!wallSegmentSourceFromMesh(mesh, segment)) {
      return false;
    }
    const WallRunKey key{segment.materialId,
                         segment.orientation,
                         segment.constantAxis,
                         segment.endpointY,
                         segment.bottomY,
                         segment.height,
                         segment.thickness};
    groups[key].push_back(segment);
  }

  for (auto& [key, segments] : groups) {
    (void)key;
    std::vector<WallBoxDraw> merged = appendWallRunsForGroup(std::move(segments));
    wallDraws.insert(wallDraws.end(), merged.begin(), merged.end());
  }
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
    if (!creativeDebugLineBox(creativeWireframeDebug->lines[index],
                              center,
                              size,
                              color) ||
        !appendBoxIfFits(result.vertices,
                         result.indices,
                         result.indexedDraws,
                         center,
                         size,
                         color)) {
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

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets) {
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
      const StaticMeshAsset* asset =
          staticMeshAssets != nullptr
              ? staticMeshAssets->find(externalAssetId)
              : nullptr;
      if (asset != nullptr &&
          appendStaticMeshAsset(result.vertices, result.indices,
                                result.indexedDraws, mesh, *asset)) {
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

  appendCreativeWireframeDebugGeometry(result, creativeWireframeDebug);
  result.ready = !result.vertices.empty() && !result.indices.empty() &&
                 !result.indexedDraws.empty();
  return result;
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(const SceneRoomProjection& room) {
  return buildRoomMeshCpuGeometry(room, nullptr, nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  return buildRoomMeshCpuGeometry(room, creativeWireframeDebug, nullptr);
}

CreativePreviewCpuGeometry buildCreativePreviewCpuGeometry() {
  CreativePreviewCpuGeometry result;
  constexpr std::array<std::string_view, kRenderCreativePreviewRoleCount>
      roles{
          "editor_ghost_select",
          "editor_ghost_valid",
          "editor_ghost_invalid",
      };
  for (std::size_t roleIndex = 0; roleIndex < roles.size(); ++roleIndex) {
    const RenderCreativePreviewRole role =
        static_cast<RenderCreativePreviewRole>(roleIndex);
    const std::uint32_t firstIndex =
        static_cast<std::uint32_t>(result.indices.size());
    std::vector<IndexedDrawRange> componentDraws;
    const Vec3 color = colorForRoomRole(std::string(roles[roleIndex]));
    if (role == RenderCreativePreviewRole::Held) {
      appendBox(result.vertices, result.indices, componentDraws, {},
                {1.0F, 1.0F, 1.0F}, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(role, false)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
    } else {
      appendCreativeTargetPreview(result.vertices, result.indices,
                                  componentDraws, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(role, false)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
      appendCreativePathWireframe(result.vertices, result.indices,
                                  componentDraws, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(role, true)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
    }
  }
  result.ready = !result.vertices.empty() && !result.indices.empty();
  return result;
}

BufferImageResources::~BufferImageResources() {
  destroy();
}

BufferImageResourcesResult BufferImageResources::createFirstRoomResources(
    const BufferImageResourcesCreateInfo& createInfo) {
  createInfo_ = createInfo;
  staticMeshAssets_.setRoot(createInfo.staticMeshAssetRoot);
  BufferImageResourcesResult result;
  result.receipt = baseReceipt("fail", "memory_allocation_failed");
#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo.device == VK_NULL_HANDLE || createInfo.physicalDevice == VK_NULL_HANDLE ||
      createInfo.graphicsQueue == VK_NULL_HANDLE || createInfo.graphicsQueueFamily == kInvalidVulkanQueueFamily ||
      createInfo.extent.width == 0U || createInfo.extent.height == 0U) {
    result.reason = {"memory_allocator_create_failed", "memory allocator create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  VulkanAllocatorCreateInfo allocatorInfo;
  allocatorInfo.physicalDevice = createInfo.physicalDevice;
  allocatorInfo.device = createInfo.device;
  allocator_.create(allocatorInfo);
  if (!allocator_.ready()) {
    result.reason = {"memory_allocator_create_failed", "memory allocator create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }

  const std::vector<FirstRoomVertex> vertices = firstRoomBootstrapVertices();
  const std::vector<std::uint16_t> indices = firstRoomBootstrapIndices();
  const VkDeviceSize vertexBytes =
      static_cast<VkDeviceSize>(vertices.size() * sizeof(FirstRoomVertex));
  const VkDeviceSize indexBytes = static_cast<VkDeviceSize>(indices.size() * sizeof(std::uint16_t));

  if (!uploadBuffer(allocator_, createInfo.device, createInfo.graphicsQueue,
                    createInfo.graphicsQueueFamily, "buffer.staging.upload.packet6",
                    "buffer.first_room.vertices", vertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    vertices.data(), geometry_.vertexBuffer)) {
    result.reason = {"vertex_buffer_create_failed", "vertex buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  if (!uploadBuffer(allocator_, createInfo.device, createInfo.graphicsQueue,
                    createInfo.graphicsQueueFamily, "buffer.staging.upload.packet6",
                    "buffer.first_room.indices", indexBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    indices.data(), geometry_.indexBuffer)) {
    result.reason = {"index_buffer_create_failed", "index buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  geometry_.vertexCount = static_cast<std::uint32_t>(vertices.size());
  geometry_.indexCount = static_cast<std::uint32_t>(indices.size());
  geometry_.indexedDraws = {{0U, geometry_.indexCount}};
  geometry_.sourceRoomAssetId.clear();
  geometry_.sourceRoomStaticMeshCount = 0;
  geometry_.sourceRoomGeometrySignature = 0;
  geometry_.sourceCreativeWireframeDebugSignature = 0;
  geometry_.creativeWireframeDebugLineInputCount = 0;
  geometry_.creativeWireframeDebugGeometryDrawCount = 0;
  geometry_.creativeWireframeDebugGeometrySkippedCount = 0;
  geometry_.creativeWireframeDebugGeometryStatus =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  geometry_.creativeWireframeDebugGeometryReasonCode =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  geometry_.roomFloorDrawCount = 0;
  geometry_.roomWallDrawCount = 0;
  geometry_.roomGridLineDrawCount = 0;
  geometry_.roomGridVisible = false;
  geometry_.roomGridTruncated = false;
  geometry_.packageRoomGeometry = false;
  geometry_.indexedDraw = true;

  const CreativePreviewCpuGeometry creativePreview =
      buildCreativePreviewCpuGeometry();
  if (!creativePreview.ready) {
    result.reason = {"vertex_buffer_create_failed",
                     "creative preview geometry build failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  const VkDeviceSize previewVertexBytes = static_cast<VkDeviceSize>(
      creativePreview.vertices.size() * sizeof(FirstRoomVertex));
  const VkDeviceSize previewIndexBytes = static_cast<VkDeviceSize>(
      creativePreview.indices.size() * sizeof(std::uint16_t));
  if (!uploadBuffer(
          allocator_, createInfo.device, createInfo.graphicsQueue,
          createInfo.graphicsQueueFamily,
          "buffer.staging.upload.creative_preview",
          "buffer.creative_preview.vertices", previewVertexBytes,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, creativePreview.vertices.data(),
          creativePreviewGeometry_.vertexBuffer)) {
    result.reason = {"vertex_buffer_create_failed",
                     "creative preview vertex buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  if (!uploadBuffer(
          allocator_, createInfo.device, createInfo.graphicsQueue,
          createInfo.graphicsQueueFamily,
          "buffer.staging.upload.creative_preview",
          "buffer.creative_preview.indices", previewIndexBytes,
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT, creativePreview.indices.data(),
          creativePreviewGeometry_.indexBuffer)) {
    allocator_.destroyBuffer(
        creativePreviewGeometry_.vertexBuffer.allocation);
    creativePreviewGeometry_ = {};
    result.reason = {"index_buffer_create_failed",
                     "creative preview index buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  creativePreviewGeometry_.indexedDraws = creativePreview.indexedDraws;
  creativePreviewGeometry_.vertexCount =
      static_cast<std::uint32_t>(creativePreview.vertices.size());
  creativePreviewGeometry_.indexCount =
      static_cast<std::uint32_t>(creativePreview.indices.size());
  creativePreviewGeometry_.ready = true;

  VkExtent3D depthExtent{createInfo.extent.width, createInfo.extent.height, 1U};
  VulkanAllocationResult depthImage = allocator_.createImage(
      "image.depth.swapchain_extent", depthExtent, createInfo.depthFormat,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (depthImage.outcome != RenderOutcome::Ok) {
    result.reason = {"depth_resource_create_failed", "depth resource create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = depthImage.image.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = createInfo.depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0U;
  viewInfo.subresourceRange.levelCount = 1U;
  viewInfo.subresourceRange.baseArrayLayer = 0U;
  viewInfo.subresourceRange.layerCount = 1U;
  VkImageView view = VK_NULL_HANDLE;
  if (vkCreateImageView(createInfo.device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
    allocator_.destroyImage(depthImage.image);
    result.reason = {"depth_resource_create_failed", "depth resource create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  depth_.depthImage.allocation = depthImage.image;
  depth_.depthImage.imageView = view;
  depth_.depthImage.format = createInfo.depthFormat;
  depth_.depthImage.extent = depthExtent;
  depth_.depthImage.allocationName = "image.depth.swapchain_extent";
  depth_.depthFormat = createInfo.depthFormat;
  depth_.extent = createInfo.extent;
  ready_ = true;
#else
  (void)createInfo;
#endif
  result.outcome = RenderOutcome::Ok;
  result.reason = {"packet6_resource_ready", "packet 6 resource ready"};
  result.receipt = baseReceipt("pass", result.reason.code);
  appendReceiptField(result.receipt, "allocation_count",
                     static_cast<std::uint64_t>(allocator_.allocations().size()));
  appendReceiptField(result.receipt, "allocation_names",
                     allocationNamesCsv(allocator_.allocations()));
  appendReceiptField(result.receipt, "vertex_buffer_count", static_cast<std::uint64_t>(2));
  appendReceiptField(result.receipt, "index_buffer_count", static_cast<std::uint64_t>(2));
  appendReceiptField(result.receipt, "creative_preview_draw_count",
                     static_cast<std::uint64_t>(
                         creativePreviewGeometry_.indexedDraws.size()));
  appendReceiptField(result.receipt, "depth_image_created", true);
  appendReceiptField(result.receipt, "depth_extent",
                     std::to_string(createInfo.extent.width) + "x" +
                         std::to_string(createInfo.extent.height));
  return result;
}

BufferImageResourcesResult BufferImageResources::createRoomMeshResources(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  BufferImageResourcesResult result;
  result.receipt = baseReceipt("fail", "memory_allocation_failed");
  if (!ready_ || !allocator_.ready() ||
      (room.meshes.empty() && room.surfacePatches.empty()) ||
      depth_.extent.width == 0U ||
      depth_.extent.height == 0U) {
    result.reason = {"memory_allocator_create_failed", "memory allocator create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  const std::uint64_t geometrySignature = roomGeometrySignature(room);
  const std::uint64_t creativeWireframeDebugSignature =
      creativeWireframeDebugGeometrySignature(creativeWireframeDebug);
  if (geometry_.packageRoomGeometry && geometry_.sourceRoomAssetId == room.assetId &&
      geometry_.sourceRoomStaticMeshCount == room.meshes.size() &&
      geometry_.sourceRoomGeometrySignature == geometrySignature &&
      geometry_.sourceCreativeWireframeDebugSignature ==
          creativeWireframeDebugSignature &&
      geometry_.indexCount > 0U &&
      geometry_.vertexBuffer.allocation.buffer != VK_NULL_HANDLE &&
      geometry_.indexBuffer.allocation.buffer != VK_NULL_HANDLE) {
    result.outcome = RenderOutcome::Ok;
    result.reason = {"packet6_resource_ready", "packet 6 resource ready"};
    result.receipt = baseReceipt("pass", result.reason.code);
    appendReceiptField(result.receipt, "vertex_buffer_count", static_cast<std::uint64_t>(1));
    appendReceiptField(result.receipt, "index_buffer_count", static_cast<std::uint64_t>(1));
    appendReceiptField(result.receipt, "room_asset_id", room.assetId);
    appendReceiptField(result.receipt, "mesh_draw_count",
                       static_cast<std::uint64_t>(geometry_.indexedDraws.size()));
    appendReceiptField(result.receipt, "static_mesh_asset_loaded_count",
                       static_cast<std::uint64_t>(
                           staticMeshAssets_.loadedAssetCount()));
    appendReceiptField(result.receipt, "static_mesh_asset_failed_count",
                       static_cast<std::uint64_t>(
                           staticMeshAssets_.failedAssetCount()));
    appendReceiptField(result.receipt, "room_floor_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomFloorDrawCount));
    appendReceiptField(result.receipt, "room_wall_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomWallDrawCount));
    appendReceiptField(result.receipt, "room_grid_line_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomGridLineDrawCount));
    appendReceiptField(result.receipt, "room_grid_visible", geometry_.roomGridVisible);
    appendReceiptField(result.receipt, "room_grid_truncated", geometry_.roomGridTruncated);
    appendCreativeWireframeDebugReceiptFields(result.receipt, geometry_);
    return result;
  }

  const RoomMeshCpuGeometry cpuGeometry =
      buildRoomMeshCpuGeometry(room, creativeWireframeDebug,
                               &staticMeshAssets_);
  if (!cpuGeometry.ready) {
    result.reason = {"vertex_buffer_create_failed", "vertex buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }

#if defined(IGGY3D_HAS_VULKAN)
  FirstRoomGeometryResources replacement;
  const VkDeviceSize vertexBytes =
      static_cast<VkDeviceSize>(cpuGeometry.vertices.size() * sizeof(FirstRoomVertex));
  const VkDeviceSize indexBytes =
      static_cast<VkDeviceSize>(cpuGeometry.indices.size() * sizeof(std::uint16_t));
  if (!uploadBuffer(allocator_, createInfo_.device, createInfo_.graphicsQueue,
                    createInfo_.graphicsQueueFamily, "buffer.staging.upload.room_mesh",
                    "buffer.room_asset.vertices", vertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    cpuGeometry.vertices.data(), replacement.vertexBuffer)) {
    result.reason = {"vertex_buffer_create_failed", "vertex buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  if (!uploadBuffer(allocator_, createInfo_.device, createInfo_.graphicsQueue,
                    createInfo_.graphicsQueueFamily, "buffer.staging.upload.room_mesh",
                    "buffer.room_asset.indices", indexBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    cpuGeometry.indices.data(), replacement.indexBuffer)) {
    allocator_.destroyBuffer(replacement.vertexBuffer.allocation);
    result.reason = {"index_buffer_create_failed", "index buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  replacement.vertexCount = static_cast<std::uint32_t>(cpuGeometry.vertices.size());
  replacement.indexCount = static_cast<std::uint32_t>(cpuGeometry.indices.size());
  replacement.indexedDraws = cpuGeometry.indexedDraws;
  replacement.sourceRoomAssetId = cpuGeometry.sourceRoomAssetId;
  replacement.sourceRoomStaticMeshCount = cpuGeometry.sourceRoomStaticMeshCount;
  replacement.sourceRoomGeometrySignature = cpuGeometry.sourceRoomGeometrySignature;
  replacement.sourceCreativeWireframeDebugSignature =
      cpuGeometry.sourceCreativeWireframeDebugSignature;
  replacement.roomFloorDrawCount = cpuGeometry.roomFloorDrawCount;
  replacement.roomWallDrawCount = cpuGeometry.roomWallDrawCount;
  replacement.roomGridLineDrawCount = cpuGeometry.roomGridLineDrawCount;
  replacement.creativeWireframeDebugLineInputCount =
      cpuGeometry.creativeWireframeDebugLineInputCount;
  replacement.creativeWireframeDebugGeometryDrawCount =
      cpuGeometry.creativeWireframeDebugGeometryDrawCount;
  replacement.creativeWireframeDebugGeometrySkippedCount =
      cpuGeometry.creativeWireframeDebugGeometrySkippedCount;
  replacement.creativeWireframeDebugGeometryStatus =
      cpuGeometry.creativeWireframeDebugGeometryStatus;
  replacement.creativeWireframeDebugGeometryReasonCode =
      cpuGeometry.creativeWireframeDebugGeometryReasonCode;
  replacement.roomGridVisible = cpuGeometry.roomGridVisible;
  replacement.roomGridTruncated = cpuGeometry.roomGridTruncated;
  replacement.packageRoomGeometry = true;
  replacement.indexedDraw = true;
  destroyGeometryBuffers();
  geometry_ = std::move(replacement);
#else
  (void)room;
#endif

  result.outcome = RenderOutcome::Ok;
  result.reason = {"packet6_resource_ready", "packet 6 resource ready"};
  result.receipt = baseReceipt("pass", result.reason.code);
  appendReceiptField(result.receipt, "allocation_count",
                     static_cast<std::uint64_t>(allocator_.allocations().size()));
  appendReceiptField(result.receipt, "allocation_names",
                     allocationNamesCsv(allocator_.allocations()));
  appendReceiptField(result.receipt, "vertex_buffer_count", static_cast<std::uint64_t>(1));
  appendReceiptField(result.receipt, "index_buffer_count", static_cast<std::uint64_t>(1));
  appendReceiptField(result.receipt, "room_asset_id", room.assetId);
  appendReceiptField(result.receipt, "room_static_mesh_count",
                     static_cast<std::uint64_t>(room.meshes.size()));
  appendReceiptField(result.receipt, "mesh_draw_count",
                     static_cast<std::uint64_t>(geometry_.indexedDraws.size()));
  appendReceiptField(result.receipt, "static_mesh_asset_loaded_count",
                     static_cast<std::uint64_t>(
                         staticMeshAssets_.loadedAssetCount()));
  appendReceiptField(result.receipt, "static_mesh_asset_failed_count",
                     static_cast<std::uint64_t>(
                         staticMeshAssets_.failedAssetCount()));
  appendReceiptField(result.receipt, "room_floor_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomFloorDrawCount));
  appendReceiptField(result.receipt, "room_wall_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomWallDrawCount));
  appendReceiptField(result.receipt, "room_grid_line_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomGridLineDrawCount));
  appendReceiptField(result.receipt, "room_grid_visible", geometry_.roomGridVisible);
  appendReceiptField(result.receipt, "room_grid_truncated", geometry_.roomGridTruncated);
  appendCreativeWireframeDebugReceiptFields(result.receipt, geometry_);
  appendReceiptField(result.receipt, "index_count", static_cast<std::uint64_t>(geometry_.indexCount));
  return result;
}

RenderReceipt BufferImageResources::destroy() {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready");
#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo_.device != VK_NULL_HANDLE && depth_.depthImage.imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(createInfo_.device, depth_.depthImage.imageView, nullptr);
  }
#endif
  depth_.depthImage.imageView = {};
  allocator_.destroyImage(depth_.depthImage.allocation);
  destroyGeometryBuffers();
  destroyCreativePreviewBuffers();
  allocator_.destroy();
  geometry_ = {};
  creativePreviewGeometry_ = {};
  depth_ = {};
  createInfo_ = {};
  ready_ = false;
  return receipt;
}

const FirstRoomGeometryResources& BufferImageResources::geometry() const {
  return geometry_;
}

const CreativePreviewGeometryResources&
BufferImageResources::creativePreviewGeometry() const {
  return creativePreviewGeometry_;
}

const DepthResourceRecord& BufferImageResources::depth() const {
  return depth_;
}

bool BufferImageResources::ready() const {
  return ready_;
}

void BufferImageResources::destroyGeometryBuffers() {
  allocator_.destroyBuffer(geometry_.indexBuffer.allocation);
  allocator_.destroyBuffer(geometry_.vertexBuffer.allocation);
  geometry_ = {};
}

void BufferImageResources::destroyCreativePreviewBuffers() {
  allocator_.destroyBuffer(creativePreviewGeometry_.indexBuffer.allocation);
  allocator_.destroyBuffer(creativePreviewGeometry_.vertexBuffer.allocation);
  creativePreviewGeometry_ = {};
}

}  // namespace iggy3d::vulkan
