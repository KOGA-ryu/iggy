#include "render/vulkan/BufferImageResources.hpp"

#include <algorithm>
#include <iterator>
#include <limits>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

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
  return {0.36F, 0.42F, 0.48F};
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
  if (geometry_.packageRoomGeometry && geometry_.sourceRoomAssetId == room.assetId &&
      geometry_.sourceRoomStaticMeshCount == room.meshes.size() && geometry_.indexCount > 0U &&
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
    return result;
  }

  std::vector<FirstRoomVertex> vertices;
  std::vector<std::uint16_t> indices;
  std::vector<IndexedDrawRange> draws;
  vertices.reserve(room.meshes.size() * 8U);
  indices.reserve(room.meshes.size() * 72U);
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (vertices.size() + 8U >
        static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
      result.reason = {"vertex_buffer_create_failed", "vertex buffer create failed"};
      result.receipt = baseReceipt("fail", result.reason.code);
      return result;
    }
    appendBox(vertices, indices, draws, mesh.position, mesh.size, colorForRoomRole(mesh.role));
  }
  if (vertices.empty() || indices.empty() || draws.empty()) {
    result.reason = {"vertex_buffer_create_failed", "vertex buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }

#if defined(IGGY3D_HAS_VULKAN)
  FirstRoomGeometryResources replacement;
  const VkDeviceSize vertexBytes =
      static_cast<VkDeviceSize>(vertices.size() * sizeof(FirstRoomVertex));
  const VkDeviceSize indexBytes = static_cast<VkDeviceSize>(indices.size() * sizeof(std::uint16_t));
  if (!uploadBuffer(allocator_, createInfo_.device, createInfo_.graphicsQueue,
                    createInfo_.graphicsQueueFamily, "buffer.staging.upload.room_mesh",
                    "buffer.room_asset.vertices", vertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    vertices.data(), replacement.vertexBuffer)) {
    result.reason = {"vertex_buffer_create_failed", "vertex buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  if (!uploadBuffer(allocator_, createInfo_.device, createInfo_.graphicsQueue,
                    createInfo_.graphicsQueueFamily, "buffer.staging.upload.room_mesh",
                    "buffer.room_asset.indices", indexBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    indices.data(), replacement.indexBuffer)) {
    allocator_.destroyBuffer(replacement.vertexBuffer.allocation);
    result.reason = {"index_buffer_create_failed", "index buffer create failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  replacement.vertexCount = static_cast<std::uint32_t>(vertices.size());
  replacement.indexCount = static_cast<std::uint32_t>(indices.size());
  replacement.indexedDraws = std::move(draws);
  replacement.sourceRoomAssetId = room.assetId;
  replacement.sourceRoomStaticMeshCount = room.meshes.size();
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
