#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

}  // namespace


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

  StaticMeshMaterialTextureCreateInfo textureCreateInfo;
  textureCreateInfo.device = createInfo.device;
  textureCreateInfo.graphicsQueue = createInfo.graphicsQueue;
  textureCreateInfo.graphicsQueueFamily = createInfo.graphicsQueueFamily;
  staticMeshMaterialTextures_.create(textureCreateInfo, allocator_,
                                     &staticMeshAssets_);

  const StaticMeshAssetAtlasCpuGeometry assetAtlas =
      buildStaticMeshAssetAtlasCpuGeometry(
          &staticMeshAssets_, &staticMeshMaterialTextures_.resources());
  if (!assetAtlas.valid) {
    result.reason = {"vertex_buffer_create_failed",
                     "static mesh asset atlas build failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  staticMeshAssetAtlas_.assetDraws = assetAtlas.assetDraws;
  staticMeshAssetAtlas_.valid = true;
  if (!assetAtlas.vertices.empty()) {
    const VkDeviceSize assetVertexBytes = static_cast<VkDeviceSize>(
        assetAtlas.vertices.size() * sizeof(StaticMeshInstanceVertex));
    const VkDeviceSize assetIndexBytes = static_cast<VkDeviceSize>(
        assetAtlas.indices.size() * sizeof(std::uint32_t));
    if (!uploadBuffer(
            allocator_, createInfo.device, createInfo.graphicsQueue,
            createInfo.graphicsQueueFamily,
            "buffer.staging.upload.static_mesh_asset_atlas",
            "buffer.static_mesh_asset_atlas.vertices", assetVertexBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, assetAtlas.vertices.data(),
            staticMeshAssetAtlas_.vertexBuffer) ||
        !uploadBuffer(
            allocator_, createInfo.device, createInfo.graphicsQueue,
            createInfo.graphicsQueueFamily,
            "buffer.staging.upload.static_mesh_asset_atlas",
            "buffer.static_mesh_asset_atlas.indices", assetIndexBytes,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, assetAtlas.indices.data(),
            staticMeshAssetAtlas_.indexBuffer)) {
      result.reason = {"vertex_buffer_create_failed",
                       "static mesh asset atlas upload failed"};
      result.receipt = baseReceipt("fail", result.reason.code);
      return result;
    }
    staticMeshAssetAtlas_.vertexCount =
        static_cast<std::uint32_t>(assetAtlas.vertices.size());
    staticMeshAssetAtlas_.indexCount =
        static_cast<std::uint32_t>(assetAtlas.indices.size());
    staticMeshAssetAtlas_.ready = true;
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
      buildCreativePreviewCpuGeometry(&staticMeshAssets_);
  if (!creativePreview.ready) {
    result.reason = {"vertex_buffer_create_failed",
                     "creative preview geometry build failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  const VkDeviceSize previewVertexBytes = static_cast<VkDeviceSize>(
      creativePreview.vertices.size() * sizeof(FirstRoomVertex));
  const VkDeviceSize previewIndexBytes = static_cast<VkDeviceSize>(
      creativePreview.indices.size() * sizeof(std::uint32_t));
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
  creativePreviewGeometry_.assetDraws = creativePreview.assetDraws;
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
  appendReceiptField(
      result.receipt, "vertex_buffer_count",
      static_cast<std::uint64_t>(staticMeshAssetAtlas_.ready ? 3U : 2U));
  appendReceiptField(
      result.receipt, "index_buffer_count",
      static_cast<std::uint64_t>(staticMeshAssetAtlas_.ready ? 3U : 2U));
  appendReceiptField(
      result.receipt, "static_mesh_asset_atlas_vertex_count",
      static_cast<std::uint64_t>(staticMeshAssetAtlas_.vertexCount));
  appendReceiptField(
      result.receipt, "static_mesh_asset_atlas_index_count",
      static_cast<std::uint64_t>(staticMeshAssetAtlas_.indexCount));
  appendReceiptField(
      result.receipt, "static_mesh_asset_atlas_asset_count",
      static_cast<std::uint64_t>(staticMeshAssetAtlas_.assetDraws.size()));
  appendReceiptField(result.receipt, "creative_preview_draw_count",
                     static_cast<std::uint64_t>(
                         creativePreviewGeometry_.indexedDraws.size()));
  const StaticMeshMaterialTextureResources& materialTextures =
      staticMeshMaterialTextures_.resources();
  appendReceiptField(result.receipt, "static_mesh_texture_count",
                     static_cast<std::uint64_t>(
                         materialTextures.textures.size()));
  appendReceiptField(result.receipt, "static_mesh_texture_rejected_count",
                     static_cast<std::uint64_t>(
                         materialTextures.rejectedTextureCount));
  appendReceiptField(result.receipt, "static_mesh_texture_descriptor_ready",
                     materialTextures.ready);
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
  const std::uint64_t geometrySignature =
      room_mesh_detail::roomGeometrySignature(room);
  const std::uint64_t creativeWireframeDebugSignature =
      room_mesh_detail::creativeWireframeDebugGeometrySignature(
          creativeWireframeDebug);
  const bool cachedBaseGeometryReady =
      geometry_.indexCount == 0U ||
      (geometry_.vertexBuffer.allocation.buffer != VK_NULL_HANDLE &&
       geometry_.indexBuffer.allocation.buffer != VK_NULL_HANDLE);
  const bool cachedInstanceGeometryReady =
      geometry_.staticMeshInstanceCount == 0U ||
      (geometry_.staticMeshInstanceBuffer.allocation.buffer != VK_NULL_HANDLE &&
       !geometry_.staticMeshInstanceBatches.empty() &&
       staticMeshAssetAtlas_.ready &&
       staticMeshAssetAtlas_.vertexBuffer.allocation.buffer != VK_NULL_HANDLE &&
       staticMeshAssetAtlas_.indexBuffer.allocation.buffer != VK_NULL_HANDLE);
  if (geometry_.packageRoomGeometry && geometry_.sourceRoomAssetId == room.assetId &&
      geometry_.sourceRoomStaticMeshCount == room.meshes.size() &&
      geometry_.sourceRoomGeometrySignature == geometrySignature &&
      geometry_.sourceCreativeWireframeDebugSignature ==
          creativeWireframeDebugSignature &&
      (geometry_.indexCount > 0U ||
       geometry_.staticMeshInstanceCount > 0U) &&
      cachedBaseGeometryReady && cachedInstanceGeometryReady) {
    result.outcome = RenderOutcome::Ok;
    result.reason = {"packet6_resource_ready", "packet 6 resource ready"};
    result.receipt = baseReceipt("pass", result.reason.code);
    appendReceiptField(
        result.receipt, "vertex_buffer_count",
        static_cast<std::uint64_t>((geometry_.vertexCount > 0U ? 1U : 0U) +
                                   (geometry_.staticMeshInstanceCount > 0U
                                        ? 1U
                                        : 0U)));
    appendReceiptField(
        result.receipt, "index_buffer_count",
        static_cast<std::uint64_t>(geometry_.indexCount > 0U ? 1U : 0U));
    appendReceiptField(result.receipt, "room_asset_id", room.assetId);
    appendReceiptField(result.receipt, "mesh_draw_count",
                       static_cast<std::uint64_t>(
                           geometry_.indexedDraws.size() +
                           geometry_.staticMeshInstanceBatches.size()));
    appendReceiptField(
        result.receipt, "static_mesh_instance_count",
        static_cast<std::uint64_t>(geometry_.staticMeshInstanceCount));
    appendReceiptField(
        result.receipt, "static_mesh_instance_batch_count",
        static_cast<std::uint64_t>(
            geometry_.staticMeshInstanceBatches.size()));
    appendReceiptField(result.receipt, "static_mesh_instance_upload_count",
                       static_cast<std::uint64_t>(0));
    appendReceiptField(result.receipt, "static_mesh_asset_loaded_count",
                       static_cast<std::uint64_t>(
                           staticMeshAssets_.loadedAssetCount()));
    appendReceiptField(result.receipt, "static_mesh_asset_failed_count",
                       static_cast<std::uint64_t>(
                           staticMeshAssets_.failedAssetCount()));
    appendReceiptField(result.receipt, "static_mesh_texture_count",
                       static_cast<std::uint64_t>(
                           staticMeshMaterialTextures_.resources()
                               .textures.size()));
    appendReceiptField(result.receipt, "room_floor_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomFloorDrawCount));
    appendReceiptField(result.receipt, "room_wall_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomWallDrawCount));
    appendReceiptField(result.receipt, "room_grid_line_draw_count",
                       static_cast<std::uint64_t>(geometry_.roomGridLineDrawCount));
    appendReceiptField(result.receipt, "room_grid_visible", geometry_.roomGridVisible);
    appendReceiptField(result.receipt, "room_grid_truncated", geometry_.roomGridTruncated);
    room_mesh_detail::appendCreativeWireframeDebugReceiptFields(
        result.receipt, geometry_);
    return result;
  }

  const RoomMeshCpuGeometry cpuGeometry =
      buildRoomMeshCpuGeometry(room, creativeWireframeDebug,
                               staticMeshAssetAtlas_.assetDraws);
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
  if (!cpuGeometry.vertices.empty()) {
    if (!uploadBuffer(
            allocator_, createInfo_.device, createInfo_.graphicsQueue,
            createInfo_.graphicsQueueFamily, "buffer.staging.upload.room_mesh",
            "buffer.room_asset.vertices", vertexBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpuGeometry.vertices.data(),
            replacement.vertexBuffer)) {
      result.reason = {"vertex_buffer_create_failed",
                       "vertex buffer create failed"};
      result.receipt = baseReceipt("fail", result.reason.code);
      return result;
    }
    if (!uploadBuffer(
            allocator_, createInfo_.device, createInfo_.graphicsQueue,
            createInfo_.graphicsQueueFamily, "buffer.staging.upload.room_mesh",
            "buffer.room_asset.indices", indexBytes,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpuGeometry.indices.data(),
            replacement.indexBuffer)) {
      allocator_.destroyBuffer(replacement.vertexBuffer.allocation);
      result.reason = {"index_buffer_create_failed",
                       "index buffer create failed"};
      result.receipt = baseReceipt("fail", result.reason.code);
      return result;
    }
  }
  if (!cpuGeometry.staticMeshInstances.empty()) {
    const VkDeviceSize instanceBytes = static_cast<VkDeviceSize>(
        cpuGeometry.staticMeshInstances.size() *
        sizeof(StaticMeshInstanceTransform));
    if (!uploadBuffer(
            allocator_, createInfo_.device, createInfo_.graphicsQueue,
            createInfo_.graphicsQueueFamily,
            "buffer.staging.upload.room_static_mesh_instances",
            "buffer.room_asset.static_mesh_instances", instanceBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            cpuGeometry.staticMeshInstances.data(),
            replacement.staticMeshInstanceBuffer)) {
      allocator_.destroyBuffer(replacement.indexBuffer.allocation);
      allocator_.destroyBuffer(replacement.vertexBuffer.allocation);
      result.reason = {"vertex_buffer_create_failed",
                       "static mesh instance buffer create failed"};
      result.receipt = baseReceipt("fail", result.reason.code);
      return result;
    }
  }
  replacement.vertexCount = static_cast<std::uint32_t>(cpuGeometry.vertices.size());
  replacement.indexCount = static_cast<std::uint32_t>(cpuGeometry.indices.size());
  replacement.indexedDraws = cpuGeometry.indexedDraws;
  replacement.staticMeshInstanceCount =
      static_cast<std::uint32_t>(cpuGeometry.staticMeshInstances.size());
  replacement.staticMeshInstanceBatches =
      cpuGeometry.staticMeshInstanceBatches;
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
  appendReceiptField(
      result.receipt, "vertex_buffer_count",
      static_cast<std::uint64_t>((geometry_.vertexCount > 0U ? 1U : 0U) +
                                 (geometry_.staticMeshInstanceCount > 0U
                                      ? 1U
                                      : 0U)));
  appendReceiptField(
      result.receipt, "index_buffer_count",
      static_cast<std::uint64_t>(geometry_.indexCount > 0U ? 1U : 0U));
  appendReceiptField(result.receipt, "room_asset_id", room.assetId);
  appendReceiptField(result.receipt, "room_static_mesh_count",
                     static_cast<std::uint64_t>(room.meshes.size()));
  appendReceiptField(result.receipt, "mesh_draw_count",
                     static_cast<std::uint64_t>(
                         geometry_.indexedDraws.size() +
                         geometry_.staticMeshInstanceBatches.size()));
  appendReceiptField(
      result.receipt, "static_mesh_instance_count",
      static_cast<std::uint64_t>(geometry_.staticMeshInstanceCount));
  appendReceiptField(
      result.receipt, "static_mesh_instance_batch_count",
      static_cast<std::uint64_t>(
          geometry_.staticMeshInstanceBatches.size()));
  appendReceiptField(
      result.receipt, "static_mesh_instance_upload_count",
      static_cast<std::uint64_t>(geometry_.staticMeshInstanceCount > 0U ? 1U
                                                                       : 0U));
  appendReceiptField(result.receipt, "static_mesh_asset_loaded_count",
                     static_cast<std::uint64_t>(
                         staticMeshAssets_.loadedAssetCount()));
  appendReceiptField(result.receipt, "static_mesh_asset_failed_count",
                     static_cast<std::uint64_t>(
                         staticMeshAssets_.failedAssetCount()));
  appendReceiptField(result.receipt, "static_mesh_texture_count",
                     static_cast<std::uint64_t>(
                         staticMeshMaterialTextures_.resources()
                             .textures.size()));
  appendReceiptField(result.receipt, "room_floor_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomFloorDrawCount));
  appendReceiptField(result.receipt, "room_wall_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomWallDrawCount));
  appendReceiptField(result.receipt, "room_grid_line_draw_count",
                     static_cast<std::uint64_t>(geometry_.roomGridLineDrawCount));
  appendReceiptField(result.receipt, "room_grid_visible", geometry_.roomGridVisible);
  appendReceiptField(result.receipt, "room_grid_truncated", geometry_.roomGridTruncated);
  room_mesh_detail::appendCreativeWireframeDebugReceiptFields(result.receipt,
                                                               geometry_);
  appendReceiptField(result.receipt, "index_count", static_cast<std::uint64_t>(geometry_.indexCount));
  return result;
}

BufferImageResourcesResult BufferImageResources::prepareStaticMeshAssetReload() {
  BufferImageResourcesResult result;
  result.receipt = baseReceipt("fail", "static_mesh_asset_reload_not_ready");
  cancelStaticMeshAssetReload();
  if (!ready_ || !allocator_.ready()) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = {"static_mesh_asset_reload_not_ready",
                     "static mesh asset reload not ready"};
    return result;
  }

  pendingStaticMeshAssets_.setRoot(createInfo_.staticMeshAssetRoot);
  const CreativePreviewCpuGeometry preview =
      buildCreativePreviewCpuGeometry(&pendingStaticMeshAssets_);
  if (!preview.ready) {
    result.reason = {"static_mesh_asset_reload_preview_failed",
                     "static mesh asset reload preview failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    cancelStaticMeshAssetReload();
    return result;
  }

#if defined(IGGY3D_HAS_VULKAN)
  const VkDeviceSize vertexBytes = static_cast<VkDeviceSize>(
      preview.vertices.size() * sizeof(FirstRoomVertex));
  const VkDeviceSize indexBytes = static_cast<VkDeviceSize>(
      preview.indices.size() * sizeof(std::uint32_t));
  if (!uploadBuffer(
          allocator_, createInfo_.device, createInfo_.graphicsQueue,
          createInfo_.graphicsQueueFamily,
          "buffer.staging.upload.creative_preview_reload",
          "buffer.creative_preview.reload.vertices", vertexBytes,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, preview.vertices.data(),
          pendingCreativePreviewGeometry_.vertexBuffer)) {
    result.reason = {"static_mesh_asset_reload_preview_upload_failed",
                     "static mesh asset reload preview upload failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    cancelStaticMeshAssetReload();
    return result;
  }
  if (!uploadBuffer(
          allocator_, createInfo_.device, createInfo_.graphicsQueue,
          createInfo_.graphicsQueueFamily,
          "buffer.staging.upload.creative_preview_reload",
          "buffer.creative_preview.reload.indices", indexBytes,
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT, preview.indices.data(),
          pendingCreativePreviewGeometry_.indexBuffer)) {
    result.reason = {"static_mesh_asset_reload_preview_upload_failed",
                     "static mesh asset reload preview upload failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    cancelStaticMeshAssetReload();
    return result;
  }
  pendingCreativePreviewGeometry_.indexedDraws = preview.indexedDraws;
  pendingCreativePreviewGeometry_.assetDraws = preview.assetDraws;
  pendingCreativePreviewGeometry_.vertexCount =
      static_cast<std::uint32_t>(preview.vertices.size());
  pendingCreativePreviewGeometry_.indexCount =
      static_cast<std::uint32_t>(preview.indices.size());
  pendingCreativePreviewGeometry_.ready = true;

  StaticMeshMaterialTextureCreateInfo textureCreateInfo;
  textureCreateInfo.device = createInfo_.device;
  textureCreateInfo.graphicsQueue = createInfo_.graphicsQueue;
  textureCreateInfo.graphicsQueueFamily = createInfo_.graphicsQueueFamily;
  pendingStaticMeshMaterialTextures_.create(
      textureCreateInfo, allocator_, &pendingStaticMeshAssets_);
  if (!pendingStaticMeshMaterialTextures_.resources().ready) {
    result.outcome = RenderOutcome::OutOfMemory;
    result.reason = {"static_mesh_asset_reload_texture_prepare_failed",
                     "static mesh asset reload texture prepare failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    cancelStaticMeshAssetReload();
    return result;
  }

  const StaticMeshAssetAtlasCpuGeometry assetAtlas =
      buildStaticMeshAssetAtlasCpuGeometry(
          &pendingStaticMeshAssets_,
          &pendingStaticMeshMaterialTextures_.resources());
  if (!assetAtlas.valid) {
    result.outcome = RenderOutcome::OutOfMemory;
    result.reason = {"static_mesh_asset_reload_atlas_prepare_failed",
                     "static mesh asset reload atlas prepare failed"};
    result.receipt = baseReceipt("fail", result.reason.code);
    cancelStaticMeshAssetReload();
    return result;
  }
  pendingStaticMeshAssetAtlas_.assetDraws = assetAtlas.assetDraws;
  pendingStaticMeshAssetAtlas_.valid = true;
  if (!assetAtlas.vertices.empty()) {
    const VkDeviceSize assetVertexBytes = static_cast<VkDeviceSize>(
        assetAtlas.vertices.size() * sizeof(StaticMeshInstanceVertex));
    const VkDeviceSize assetIndexBytes = static_cast<VkDeviceSize>(
        assetAtlas.indices.size() * sizeof(std::uint32_t));
    if (!uploadBuffer(
            allocator_, createInfo_.device, createInfo_.graphicsQueue,
            createInfo_.graphicsQueueFamily,
            "buffer.staging.upload.static_mesh_asset_atlas_reload",
            "buffer.static_mesh_asset_atlas.reload.vertices",
            assetVertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            assetAtlas.vertices.data(),
            pendingStaticMeshAssetAtlas_.vertexBuffer) ||
        !uploadBuffer(
            allocator_, createInfo_.device, createInfo_.graphicsQueue,
            createInfo_.graphicsQueueFamily,
            "buffer.staging.upload.static_mesh_asset_atlas_reload",
            "buffer.static_mesh_asset_atlas.reload.indices", assetIndexBytes,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, assetAtlas.indices.data(),
            pendingStaticMeshAssetAtlas_.indexBuffer)) {
      result.outcome = RenderOutcome::OutOfMemory;
      result.reason = {"static_mesh_asset_reload_atlas_upload_failed",
                       "static mesh asset reload atlas upload failed"};
      result.receipt = baseReceipt("fail", result.reason.code);
      cancelStaticMeshAssetReload();
      return result;
    }
    pendingStaticMeshAssetAtlas_.vertexCount =
        static_cast<std::uint32_t>(assetAtlas.vertices.size());
    pendingStaticMeshAssetAtlas_.indexCount =
        static_cast<std::uint32_t>(assetAtlas.indices.size());
    pendingStaticMeshAssetAtlas_.ready = true;
  }
#endif

  staticMeshAssetReloadPending_ = true;
  result.outcome = RenderOutcome::Ok;
  result.reason = {"static_mesh_asset_reload_prepared",
                   "static mesh asset reload prepared"};
  result.receipt = baseReceipt("pass", result.reason.code);
  appendReceiptField(result.receipt, "creative_preview_draw_count",
                     static_cast<std::uint64_t>(
                         pendingCreativePreviewGeometry_.indexedDraws.size()));
  appendReceiptField(result.receipt, "static_mesh_asset_loaded_count",
                     static_cast<std::uint64_t>(
                         pendingStaticMeshAssets_.loadedAssetCount()));
  appendReceiptField(result.receipt, "static_mesh_asset_failed_count",
                     static_cast<std::uint64_t>(
                         pendingStaticMeshAssets_.failedAssetCount()));
  appendReceiptField(
      result.receipt, "static_mesh_texture_count",
      static_cast<std::uint64_t>(pendingStaticMeshMaterialTextures_
                                     .resources()
                                     .textures.size()));
  appendReceiptField(
      result.receipt, "static_mesh_asset_atlas_asset_count",
      static_cast<std::uint64_t>(
          pendingStaticMeshAssetAtlas_.assetDraws.size()));
  return result;
}

bool BufferImageResources::commitStaticMeshAssetReload() {
  if (!staticMeshAssetReloadPending_ ||
      !pendingCreativePreviewGeometry_.ready ||
      !pendingStaticMeshMaterialTextures_.resources().ready ||
      !pendingStaticMeshAssetAtlas_.valid) {
    return false;
  }
  destroyGeometryBuffers();
  destroyCreativePreviewBuffers();
  destroyStaticMeshAssetAtlasBuffers();
  staticMeshMaterialTextures_.destroy(createInfo_.device, allocator_);
  staticMeshMaterialTextures_.swap(pendingStaticMeshMaterialTextures_);
  staticMeshAssets_ = std::move(pendingStaticMeshAssets_);
  creativePreviewGeometry_ = std::move(pendingCreativePreviewGeometry_);
  staticMeshAssetAtlas_ = std::move(pendingStaticMeshAssetAtlas_);
  pendingStaticMeshAssets_ = {};
  pendingCreativePreviewGeometry_ = {};
  pendingStaticMeshAssetAtlas_ = {};
  staticMeshAssetReloadPending_ = false;
  return true;
}

void BufferImageResources::cancelStaticMeshAssetReload() {
  allocator_.destroyBuffer(
      pendingCreativePreviewGeometry_.indexBuffer.allocation);
  allocator_.destroyBuffer(
      pendingCreativePreviewGeometry_.vertexBuffer.allocation);
  pendingCreativePreviewGeometry_ = {};
  allocator_.destroyBuffer(pendingStaticMeshAssetAtlas_.indexBuffer.allocation);
  allocator_.destroyBuffer(
      pendingStaticMeshAssetAtlas_.vertexBuffer.allocation);
  pendingStaticMeshAssetAtlas_ = {};
  pendingStaticMeshMaterialTextures_.destroy(createInfo_.device, allocator_);
  pendingStaticMeshAssets_ = {};
  staticMeshAssetReloadPending_ = false;
}

RenderReceipt BufferImageResources::destroy() {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready");
#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo_.device != VK_NULL_HANDLE && depth_.depthImage.imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(createInfo_.device, depth_.depthImage.imageView, nullptr);
  }
#endif
  depth_.depthImage.imageView = {};
  cancelStaticMeshAssetReload();
  allocator_.destroyImage(depth_.depthImage.allocation);
  destroyGeometryBuffers();
  destroyCreativePreviewBuffers();
  destroyStaticMeshAssetAtlasBuffers();
  staticMeshMaterialTextures_.destroy(createInfo_.device, allocator_);
  allocator_.destroy();
  geometry_ = {};
  creativePreviewGeometry_ = {};
  staticMeshAssetAtlas_ = {};
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

const StaticMeshAssetAtlasResources&
BufferImageResources::staticMeshAssetAtlas() const {
  return staticMeshAssetAtlas_;
}

const StaticMeshMaterialTextureResources&
BufferImageResources::staticMeshMaterialTextures() const {
  return staticMeshMaterialTextures_.resources();
}

const StaticMeshMaterialTextureResources&
BufferImageResources::pendingStaticMeshMaterialTextures() const {
  return pendingStaticMeshMaterialTextures_.resources();
}

const DepthResourceRecord& BufferImageResources::depth() const {
  return depth_;
}

bool BufferImageResources::ready() const {
  return ready_;
}

void BufferImageResources::destroyGeometryBuffers() {
  allocator_.destroyBuffer(geometry_.staticMeshInstanceBuffer.allocation);
  allocator_.destroyBuffer(geometry_.indexBuffer.allocation);
  allocator_.destroyBuffer(geometry_.vertexBuffer.allocation);
  geometry_ = {};
}

void BufferImageResources::destroyCreativePreviewBuffers() {
  allocator_.destroyBuffer(creativePreviewGeometry_.indexBuffer.allocation);
  allocator_.destroyBuffer(creativePreviewGeometry_.vertexBuffer.allocation);
  creativePreviewGeometry_ = {};
}

void BufferImageResources::destroyStaticMeshAssetAtlasBuffers() {
  allocator_.destroyBuffer(staticMeshAssetAtlas_.indexBuffer.allocation);
  allocator_.destroyBuffer(staticMeshAssetAtlas_.vertexBuffer.allocation);
  staticMeshAssetAtlas_ = {};
}

}  // namespace iggy3d::vulkan
