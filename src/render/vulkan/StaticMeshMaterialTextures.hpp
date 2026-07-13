#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "content/assets/StaticMeshAsset.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/VulkanMemoryAllocator.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkDescriptorPool = void*;
using VkDescriptorSet = void*;
using VkDescriptorSetLayout = void*;
using VkImageView = void*;
using VkSampler = void*;
#endif

namespace iggy3d::vulkan {

struct StaticMeshTextureCpuRecord {
  std::uint64_t contentHash = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  StaticMeshTextureWrap wrapS = StaticMeshTextureWrap::Repeat;
  StaticMeshTextureWrap wrapT = StaticMeshTextureWrap::Repeat;
  StaticMeshTextureFilter minFilter = StaticMeshTextureFilter::Linear;
  StaticMeshTextureFilter magFilter = StaticMeshTextureFilter::Linear;
  std::vector<std::uint8_t> rgba8;
};

struct StaticMeshMaterialTextureBinding {
  std::string assetId;
  std::uint32_t materialIndex = 0;
  std::uint32_t textureIndex = kInvalidMaterialTextureIndex;
};

struct StaticMeshTextureCpuResources {
  std::vector<StaticMeshTextureCpuRecord> textures;
  std::vector<StaticMeshMaterialTextureBinding> materialBindings;
  std::size_t rejectedTextureCount = 0;
};

struct StaticMeshTextureGpuRecord {
  VulkanImageAllocation image;
  VkImageView imageView{};
  VkSampler sampler{};
  VkDescriptorSet descriptorSet{};
  std::uint64_t contentHash = 0;
};

struct StaticMeshMaterialTextureResources {
  VkDescriptorSetLayout descriptorSetLayout{};
  VkDescriptorPool descriptorPool{};
  std::vector<StaticMeshTextureGpuRecord> textures;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<StaticMeshMaterialTextureBinding> materialBindings;
  std::size_t rejectedTextureCount = 0;
  bool ready = false;
};

struct StaticMeshMaterialTextureCreateInfo {
  VkDevice device{};
  VkQueue graphicsQueue{};
  std::uint32_t graphicsQueueFamily = kInvalidVulkanQueueFamily;
};

[[nodiscard]] StaticMeshTextureCpuResources
buildStaticMeshTextureCpuResources(StaticMeshAssetCache* staticMeshAssets);
[[nodiscard]] std::uint32_t findStaticMeshMaterialTextureIndex(
    const std::vector<StaticMeshMaterialTextureBinding>& bindings,
    std::string_view assetId,
    std::uint32_t materialIndex) noexcept;

class StaticMeshMaterialTextureStore {
public:
  StaticMeshMaterialTextureStore() = default;
  StaticMeshMaterialTextureStore(const StaticMeshMaterialTextureStore&) =
      delete;
  StaticMeshMaterialTextureStore& operator=(
      const StaticMeshMaterialTextureStore&) = delete;

  void create(const StaticMeshMaterialTextureCreateInfo& createInfo,
              VulkanMemoryAllocator& allocator,
              StaticMeshAssetCache* staticMeshAssets);
  void destroy(VkDevice device, VulkanMemoryAllocator& allocator);
  void swap(StaticMeshMaterialTextureStore& other) noexcept;

  [[nodiscard]] const StaticMeshMaterialTextureResources& resources() const
      noexcept;

private:
  StaticMeshMaterialTextureResources resources_;
};

}  // namespace iggy3d::vulkan
