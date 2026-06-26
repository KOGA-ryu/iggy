#include "render/vulkan/BufferImageResources.hpp"

#include "render/mesh/BeanMesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <set>
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
    hashString(hash, mesh.role);
    hashString(hash, mesh.materialId);
    hashFloat(hash, mesh.position.x);
    hashFloat(hash, mesh.position.y);
    hashFloat(hash, mesh.position.z);
    hashFloat(hash, mesh.size.x);
    hashFloat(hash, mesh.size.y);
    hashFloat(hash, mesh.size.z);
    hashBool(hash, mesh.hasWallSegment);
    if (mesh.hasWallSegment) {
      hashVec3(hash, mesh.wallStartMeters);
      hashVec3(hash, mesh.wallEndMeters);
      hashFloat(hash, mesh.wallBottomY);
      hashFloat(hash, mesh.wallHeightMeters);
      hashFloat(hash, mesh.wallThicknessMeters);
    }
  }
  return hash;
}

void appendTriangle(std::vector<std::uint16_t>& indices,
                    std::uint16_t a,
                    std::uint16_t b,
                    std::uint16_t c);

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
               Vec3 color) {
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
  const float hx = std::max(size.x * 0.5F, 0.001F);
  const float hy = std::max(size.y * 0.5F, 0.001F);
  const float hz = std::max(size.z * 0.5F, 0.001F);
  const FirstRoomVertex boxVertices[8] = {
      {{center.x - hx, center.y - hy, center.z - hz}, {color.x, color.y, color.z}},
      {{center.x + hx, center.y - hy, center.z - hz}, {color.x, color.y, color.z}},
      {{center.x + hx, center.y + hy, center.z - hz}, {color.x, color.y, color.z}},
      {{center.x - hx, center.y + hy, center.z - hz}, {color.x, color.y, color.z}},
      {{center.x - hx, center.y - hy, center.z + hz}, {color.x, color.y, color.z}},
      {{center.x + hx, center.y - hy, center.z + hz}, {color.x, color.y, color.z}},
      {{center.x + hx, center.y + hy, center.z + hz}, {color.x, color.y, color.z}},
      {{center.x - hx, center.y + hy, center.z + hz}, {color.x, color.y, color.z}},
  };
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

bool canAppendBox(const std::vector<FirstRoomVertex>& vertices) {
  return vertices.size() + 8U <=
         static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max());
}

bool appendBoxIfFits(std::vector<FirstRoomVertex>& vertices,
                     std::vector<std::uint16_t>& indices,
                     std::vector<IndexedDrawRange>& draws,
                     Vec3 center,
                     Vec3 size,
                     Vec3 color) {
  if (!canAppendBox(vertices)) {
    return false;
  }
  appendBox(vertices, indices, draws, center, size, color);
  return true;
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
                BeanModelKind kind) {
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
};

bool floorToGridCell(const SceneRoomMeshItem& mesh,
                     FloorMergeKey& key,
                     FloorCell& cell) {
  if (mesh.role != "floor" || !std::isfinite(mesh.position.x) ||
      !std::isfinite(mesh.position.y) || !std::isfinite(mesh.position.z) ||
      !finitePositive(mesh.size.x) || !finitePositive(mesh.size.y) ||
      !finitePositive(mesh.size.z)) {
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
    floorDraws.push_back({duplicate.position, duplicate.size});
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
    FloorMergeKey key;
    FloorCell cell;
    if (!floorToGridCell(mesh, key, cell)) {
      floorDraws.push_back({mesh.position, mesh.size});
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
         finitePositive(floor.size.y) && finitePositive(floor.size.z);
}

struct WallBoxDraw {
  Vec3 position;
  Vec3 size;
};

bool finiteVec3(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool wallBoxFromSegment(const SceneRoomMeshItem& mesh, WallBoxDraw& draw) {
  if (!finiteVec3(mesh.wallStartMeters) || !finiteVec3(mesh.wallEndMeters) ||
      !std::isfinite(mesh.wallBottomY) || !finitePositive(mesh.wallHeightMeters) ||
      !finitePositive(mesh.wallThicknessMeters)) {
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
  return true;
}

bool wallBoxForMesh(const SceneRoomMeshItem& mesh, WallBoxDraw& draw) {
  if (mesh.hasWallSegment) {
    return wallBoxFromSegment(mesh, draw);
  }
  draw.position = mesh.position;
  draw.size = mesh.size;
  return std::isfinite(draw.position.x) && std::isfinite(draw.position.y) &&
         std::isfinite(draw.position.z) && finitePositive(draw.size.x) &&
         finitePositive(draw.size.y) && finitePositive(draw.size.z);
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

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(const SceneRoomProjection& room) {
  RoomMeshCpuGeometry result;
  result.sourceRoomAssetId = room.assetId;
  result.sourceRoomStaticMeshCount = room.meshes.size();
  result.sourceRoomGeometrySignature = roomGeometrySignature(room);
  if (room.meshes.empty()) {
    return result;
  }

  result.vertices.reserve(room.meshes.size() * 16U);
  result.indices.reserve(room.meshes.size() * 144U);

  const std::vector<FloorDraw> floorDraws = buildOptimizedFloorDraws(room);
  for (const FloorDraw& floor : floorDraws) {
    if (!canEmitFloorDraw(floor)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    if (!appendFloorPlaneIfFits(result.vertices,
                                result.indices,
                                result.indexedDraws,
                                floor.position,
                                floor.size,
                                colorForRoomRole("floor"))) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    ++result.roomFloorDrawCount;
  }

  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role == "floor") {
      std::size_t gridLines = 0;
      if (!appendFloorGrid(result.vertices, result.indices, result.indexedDraws,
                           mesh.position, mesh.size, gridLines)) {
        result.roomGridTruncated = true;
      }
      result.roomGridLineDrawCount += gridLines;
      result.roomGridVisible = result.roomGridVisible || gridLines > 0U;
      continue;
    }

    BeanModelKind beanKind = BeanModelKind::Player;
    if (parseBeanModelId(mesh.role, beanKind)) {
      if (!appendBean(result.vertices, result.indices, result.indexedDraws,
                      mesh.position, mesh.size, colorForRoomRole(mesh.role),
                      beanKind)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
    } else {
      Vec3 meshPosition = mesh.position;
      Vec3 meshSize = mesh.size;
      if (mesh.role == "wall") {
        WallBoxDraw wallDraw;
        if (!wallBoxForMesh(mesh, wallDraw)) {
          result.vertices.clear();
          result.indices.clear();
          result.indexedDraws.clear();
          return result;
        }
        meshPosition = wallDraw.position;
        meshSize = wallDraw.size;
      }
      if (!canAppendBox(result.vertices)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      appendBox(result.vertices, result.indices, result.indexedDraws,
                meshPosition, meshSize, colorForRoomRole(mesh.role));
      if (mesh.role == "wall") {
        ++result.roomWallDrawCount;
        std::size_t gridLines = 0;
        if (!appendWallGrid(result.vertices, result.indices, result.indexedDraws,
                            meshPosition, meshSize, gridLines)) {
          result.roomGridTruncated = true;
        }
        result.roomGridLineDrawCount += gridLines;
        result.roomGridVisible = result.roomGridVisible || gridLines > 0U;
      } else if (mesh.role == "grid") {
        ++result.roomGridLineDrawCount;
        result.roomGridVisible = true;
      }
    }
  }

  result.ready = !result.vertices.empty() && !result.indices.empty() &&
                 !result.indexedDraws.empty();
  return result;
}

BufferImageResources::~BufferImageResources() {
  destroy();
}

BufferImageResourcesResult BufferImageResources::createFirstRoomResources(
    const BufferImageResourcesCreateInfo& createInfo) {
  createInfo_ = createInfo;
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
  geometry_.roomFloorDrawCount = 0;
  geometry_.roomWallDrawCount = 0;
  geometry_.roomGridLineDrawCount = 0;
  geometry_.roomGridVisible = false;
  geometry_.roomGridTruncated = false;
  geometry_.packageRoomGeometry = false;
  geometry_.indexedDraw = true;

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
  appendReceiptField(result.receipt, "vertex_buffer_count", static_cast<std::uint64_t>(1));
  appendReceiptField(result.receipt, "index_buffer_count", static_cast<std::uint64_t>(1));
  appendReceiptField(result.receipt, "depth_image_created", true);
  appendReceiptField(result.receipt, "depth_extent",
                     std::to_string(createInfo.extent.width) + "x" +
                         std::to_string(createInfo.extent.height));
  return result;
}

BufferImageResourcesResult BufferImageResources::createRoomMeshResources(
    const SceneRoomProjection& room) {
  BufferImageResourcesResult result;
  result.receipt = baseReceipt("fail", "memory_allocation_failed");
  if (!ready_ || !allocator_.ready() || room.meshes.empty() || depth_.extent.width == 0U ||
      depth_.extent.height == 0U) {
    result.reason = {"memory_allocator_create_failed", "memory allocator create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  const std::uint64_t geometrySignature = roomGeometrySignature(room);
  if (geometry_.packageRoomGeometry && geometry_.sourceRoomAssetId == room.assetId &&
      geometry_.sourceRoomStaticMeshCount == room.meshes.size() &&
      geometry_.sourceRoomGeometrySignature == geometrySignature && geometry_.indexCount > 0U &&
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
    appendReceiptField(result.receipt, "room_floor_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomFloorDrawCount));
    appendReceiptField(result.receipt, "room_wall_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomWallDrawCount));
    appendReceiptField(result.receipt, "room_grid_line_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomGridLineDrawCount));
    appendReceiptField(result.receipt, "room_grid_visible", geometry_.roomGridVisible);
    appendReceiptField(result.receipt, "room_grid_truncated", geometry_.roomGridTruncated);
    return result;
  }

  const RoomMeshCpuGeometry cpuGeometry = buildRoomMeshCpuGeometry(room);
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
  replacement.roomFloorDrawCount = cpuGeometry.roomFloorDrawCount;
  replacement.roomWallDrawCount = cpuGeometry.roomWallDrawCount;
  replacement.roomGridLineDrawCount = cpuGeometry.roomGridLineDrawCount;
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
  appendReceiptField(result.receipt, "room_floor_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomFloorDrawCount));
  appendReceiptField(result.receipt, "room_wall_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomWallDrawCount));
  appendReceiptField(result.receipt, "room_grid_line_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomGridLineDrawCount));
  appendReceiptField(result.receipt, "room_grid_visible", geometry_.roomGridVisible);
  appendReceiptField(result.receipt, "room_grid_truncated", geometry_.roomGridTruncated);
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
  allocator_.destroy();
  geometry_ = {};
  depth_ = {};
  createInfo_ = {};
  ready_ = false;
  return receipt;
}

const FirstRoomGeometryResources& BufferImageResources::geometry() const {
  return geometry_;
}

const DepthResourceRecord& BufferImageResources::depth() const {
  return depth_;
}

const VulkanMemoryAllocator& BufferImageResources::allocator() const {
  return allocator_;
}

bool BufferImageResources::ready() const {
  return ready_;
}

void BufferImageResources::destroyGeometryBuffers() {
  allocator_.destroyBuffer(geometry_.indexBuffer.allocation);
  allocator_.destroyBuffer(geometry_.vertexBuffer.allocation);
  geometry_ = {};
}

}  // namespace iggy3d::vulkan
