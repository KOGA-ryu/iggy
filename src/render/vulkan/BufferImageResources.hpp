#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "projection/scene/SceneItem.hpp"
#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/VulkanMemoryAllocator.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkImageView = void*;
using VkQueue = void*;
using VkCommandPool = void*;
struct VkExtent2D {
  std::uint32_t width{};
  std::uint32_t height{};
};
#endif

namespace iggy3d::vulkan {

inline constexpr std::size_t kCreativePreviewGeometryDrawRangeCount = 5U;

[[nodiscard]] constexpr std::uint32_t creativePreviewGeometryDrawIndex(
    RenderCreativePreviewRole role,
    bool includePathWireframe) noexcept {
  switch (role) {
    case RenderCreativePreviewRole::Held:
      return 0U;
    case RenderCreativePreviewRole::PlacementValid:
      return includePathWireframe ? 3U : 1U;
    case RenderCreativePreviewRole::PlacementInvalid:
      return includePathWireframe ? 4U : 2U;
    case RenderCreativePreviewRole::Count:
      return static_cast<std::uint32_t>(
          kCreativePreviewGeometryDrawRangeCount);
  }
  return static_cast<std::uint32_t>(kCreativePreviewGeometryDrawRangeCount);
}

struct GpuBufferRecord {
  VulkanBufferAllocation allocation;
  std::string allocationName;
};

struct GpuImageRecord {
  VulkanImageAllocation allocation;
  VkImageView imageView{};
  VkFormat format{};
  VkExtent3D extent{};
  std::string allocationName;
};

struct FirstRoomGeometryResources {
  GpuBufferRecord vertexBuffer;
  GpuBufferRecord indexBuffer;
  std::uint32_t vertexCount = 0;
  std::uint32_t indexCount = 0;
  std::vector<IndexedDrawRange> indexedDraws;
  std::string sourceRoomAssetId;
  std::size_t sourceRoomStaticMeshCount = 0;
  std::uint64_t sourceRoomGeometrySignature = 0;
  std::size_t roomFloorDrawCount = 0;
  std::size_t roomWallDrawCount = 0;
  std::size_t roomGridLineDrawCount = 0;
  std::size_t creativeWireframeDebugLineInputCount = 0;
  std::size_t creativeWireframeDebugGeometryDrawCount = 0;
  std::size_t creativeWireframeDebugGeometrySkippedCount = 0;
  std::string creativeWireframeDebugGeometryStatus =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  std::string creativeWireframeDebugGeometryReasonCode =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  std::uint64_t sourceCreativeWireframeDebugSignature = 0;
  bool roomGridVisible = false;
  bool roomGridTruncated = false;
  bool packageRoomGeometry = false;
  bool indexedDraw = false;
};

struct CreativePreviewGeometryResources {
  GpuBufferRecord vertexBuffer;
  GpuBufferRecord indexBuffer;
  std::array<IndexedDrawRange, kCreativePreviewGeometryDrawRangeCount>
      indexedDraws{};
  std::uint32_t vertexCount = 0;
  std::uint32_t indexCount = 0;
  bool ready = false;
};

struct RoomMeshCpuGeometry {
  std::vector<FirstRoomVertex> vertices;
  std::vector<std::uint16_t> indices;
  std::vector<IndexedDrawRange> indexedDraws;
  std::string sourceRoomAssetId;
  std::size_t sourceRoomStaticMeshCount = 0;
  std::uint64_t sourceRoomGeometrySignature = 0;
  std::size_t roomFloorDrawCount = 0;
  std::size_t roomWallDrawCount = 0;
  std::size_t roomGridLineDrawCount = 0;
  std::size_t creativeWireframeDebugLineInputCount = 0;
  std::size_t creativeWireframeDebugGeometryDrawCount = 0;
  std::size_t creativeWireframeDebugGeometrySkippedCount = 0;
  std::string creativeWireframeDebugGeometryStatus =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  std::string creativeWireframeDebugGeometryReasonCode =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  std::uint64_t sourceCreativeWireframeDebugSignature = 0;
  bool roomGridVisible = false;
  bool roomGridTruncated = false;
  bool ready = false;
};

struct CreativeWireframeDebugCpuGeometry {
  std::vector<FirstRoomVertex> vertices;
  std::vector<std::uint16_t> indices;
  std::vector<IndexedDrawRange> indexedDraws;
  std::size_t inputLineCount = 0;
  std::size_t emittedBoxCount = 0;
  std::size_t skippedCount = 0;
  std::uint64_t geometrySignature = 0;
  std::string status = "vulkan_creative_wireframe_debug_geometry_not_requested";
  std::string reasonCode =
      "vulkan_creative_wireframe_debug_geometry_not_requested";
  bool ready = false;
};

struct CreativePreviewCpuGeometry {
  std::vector<FirstRoomVertex> vertices;
  std::vector<std::uint16_t> indices;
  std::array<IndexedDrawRange, kCreativePreviewGeometryDrawRangeCount>
      indexedDraws{};
  bool ready = false;
};

struct DepthResourceRecord {
  GpuImageRecord depthImage;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  VkExtent2D extent{};
};

struct BufferImageResourcesCreateInfo {
  VkPhysicalDevice physicalDevice{};
  VkDevice device{};
  VkQueue graphicsQueue{};
  std::uint32_t graphicsQueueFamily = kInvalidVulkanQueueFamily;
  VkExtent2D extent{};
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
};

struct BufferImageResourcesResult {
  RenderOutcome outcome = RenderOutcome::OutOfMemory;
  RenderReason reason{"memory_allocation_failed", "memory allocation failed"};
  RenderReceipt receipt;
};

class BufferImageResources {
public:
  BufferImageResources() = default;
  ~BufferImageResources();

  BufferImageResources(const BufferImageResources&) = delete;
  BufferImageResources& operator=(const BufferImageResources&) = delete;

  BufferImageResourcesResult createFirstRoomResources(
      const BufferImageResourcesCreateInfo& createInfo);
  BufferImageResourcesResult createRoomMeshResources(
      const SceneRoomProjection& room,
      const RenderCreativeWireframeDebugFrame* creativeWireframeDebug = nullptr);
  RenderReceipt destroy();

  const FirstRoomGeometryResources& geometry() const;
  const CreativePreviewGeometryResources& creativePreviewGeometry() const;
  const DepthResourceRecord& depth() const;
  bool ready() const;

private:
  void destroyGeometryBuffers();
  void destroyCreativePreviewBuffers();

  VulkanMemoryAllocator allocator_;
  FirstRoomGeometryResources geometry_;
  CreativePreviewGeometryResources creativePreviewGeometry_;
  DepthResourceRecord depth_;
  BufferImageResourcesCreateInfo createInfo_;
  bool ready_ = false;
};

std::vector<FirstRoomVertex> firstRoomBootstrapVertices();
std::vector<std::uint16_t> firstRoomBootstrapIndices();
RoomMeshCpuGeometry buildRoomMeshCpuGeometry(const SceneRoomProjection& room);
RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug);
CreativeWireframeDebugCpuGeometry buildCreativeWireframeDebugCpuGeometry(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug);
CreativePreviewCpuGeometry buildCreativePreviewCpuGeometry();

}  // namespace iggy3d::vulkan
