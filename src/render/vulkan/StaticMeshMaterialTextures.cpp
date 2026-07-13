#include "render/vulkan/StaticMeshMaterialTextures.hpp"

#include <algorithm>
#include <utility>

namespace iggy3d::vulkan {
namespace {

#if defined(IGGY3D_HAS_VULKAN)
bool copyBufferToImage(VkDevice device,
                       VkQueue queue,
                       std::uint32_t queueFamily,
                       VkBuffer source,
                       VkImage destination,
                       VkExtent3D extent) {
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

  VkImageMemoryBarrier toTransfer{};
  toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toTransfer.image = destination;
  toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  toTransfer.subresourceRange.levelCount = 1U;
  toTransfer.subresourceRange.layerCount = 1U;
  vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0U, 0U, nullptr, 0U,
                       nullptr, 1U, &toTransfer);

  VkBufferImageCopy copy{};
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.layerCount = 1U;
  copy.imageExtent = extent;
  vkCmdCopyBufferToImage(command, source, destination,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &copy);

  VkImageMemoryBarrier toShaderRead = toTransfer;
  toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0U, 0U, nullptr,
                       0U, nullptr, 1U, &toShaderRead);

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
  const bool submitted =
      vkQueueSubmit(queue, 1U, &submitInfo, fence) == VK_SUCCESS;
  const bool completed =
      submitted && vkWaitForFences(device, 1U, &fence, VK_TRUE,
                                   1'000'000'000ULL) == VK_SUCCESS;
  vkDestroyFence(device, fence, nullptr);
  vkDestroyCommandPool(device, pool, nullptr);
  return completed;
}

[[nodiscard]] VkSamplerAddressMode samplerAddressMode(
    StaticMeshTextureWrap wrap) noexcept {
  switch (wrap) {
    case StaticMeshTextureWrap::ClampToEdge:
      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case StaticMeshTextureWrap::MirroredRepeat:
      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case StaticMeshTextureWrap::Repeat:
      return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }
  return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

[[nodiscard]] VkFilter samplerFilter(StaticMeshTextureFilter filter) noexcept {
  return filter == StaticMeshTextureFilter::Nearest ? VK_FILTER_NEAREST
                                                     : VK_FILTER_LINEAR;
}

bool uploadTexture(VulkanMemoryAllocator& allocator,
                   const StaticMeshMaterialTextureCreateInfo& createInfo,
                   const StaticMeshTextureCpuRecord& source,
                   StaticMeshTextureGpuRecord& output) {
  if (source.width == 0U || source.height == 0U || source.rgba8.empty()) {
    return false;
  }
  VulkanAllocationResult staging = allocator.createBuffer(
      "buffer.staging.upload.static_mesh_texture",
      static_cast<VkDeviceSize>(source.rgba8.size()),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      source.rgba8.data());
  if (staging.outcome != RenderOutcome::Ok) {
    return false;
  }
  const VkExtent3D extent{source.width, source.height, 1U};
  VulkanAllocationResult image = allocator.createImage(
      "image.static_mesh.base_color", extent,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (image.outcome != RenderOutcome::Ok) {
    allocator.destroyBuffer(staging.buffer);
    return false;
  }
  const bool copied = copyBufferToImage(
      createInfo.device, createInfo.graphicsQueue,
      createInfo.graphicsQueueFamily, staging.buffer.buffer,
      image.image.image, extent);
  allocator.destroyBuffer(staging.buffer);
  if (!copied) {
    allocator.destroyImage(image.image);
    return false;
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image.image.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.levelCount = 1U;
  viewInfo.subresourceRange.layerCount = 1U;
  if (vkCreateImageView(createInfo.device, &viewInfo, nullptr,
                        &output.imageView) != VK_SUCCESS) {
    allocator.destroyImage(image.image);
    return false;
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = samplerFilter(source.magFilter);
  samplerInfo.minFilter = samplerFilter(source.minFilter);
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.addressModeU = samplerAddressMode(source.wrapS);
  samplerInfo.addressModeV = samplerAddressMode(source.wrapT);
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.maxLod = 0.0F;
  if (vkCreateSampler(createInfo.device, &samplerInfo, nullptr,
                      &output.sampler) != VK_SUCCESS) {
    vkDestroyImageView(createInfo.device, output.imageView, nullptr);
    output.imageView = {};
    allocator.destroyImage(image.image);
    return false;
  }
  output.image = image.image;
  output.contentHash = source.contentHash;
  return true;
}
#endif

}  // namespace

std::uint32_t findStaticMeshMaterialTextureIndex(
    const std::vector<StaticMeshMaterialTextureBinding>& bindings,
    std::string_view assetId,
    std::uint32_t materialIndex) noexcept {
  const auto found = std::lower_bound(
      bindings.begin(), bindings.end(), std::pair{assetId, materialIndex},
      [](const StaticMeshMaterialTextureBinding& binding,
         const std::pair<std::string_view, std::uint32_t>& key) {
        return std::pair<std::string_view, std::uint32_t>{
                   binding.assetId, binding.materialIndex} < key;
      });
  return found != bindings.end() && found->assetId == assetId &&
                 found->materialIndex == materialIndex
             ? found->textureIndex
             : kInvalidMaterialTextureIndex;
}

StaticMeshTextureCpuResources buildStaticMeshTextureCpuResources(
    StaticMeshAssetCache* staticMeshAssets) {
  StaticMeshTextureCpuResources result;
  if (staticMeshAssets == nullptr) {
    return result;
  }
  const StaticMeshAssetCatalog catalog =
      discoverStaticMeshAssetCatalog(staticMeshAssets->root());
  for (const StaticMeshAssetCatalogEntry& entry : catalog.entries) {
    const StaticMeshAsset* asset = staticMeshAssets->find(entry.assetId);
    if (asset == nullptr) {
      ++result.rejectedTextureCount;
      continue;
    }
    result.rejectedTextureCount += asset->textureFailureCount;
    for (std::size_t materialIndex = 0U;
         materialIndex < asset->materials.size(); ++materialIndex) {
      const StaticMeshMaterial& material = asset->materials[materialIndex];
      if (material.baseColorImageIndex == kInvalidStaticMeshImageIndex) {
        continue;
      }
      if (material.baseColorImageIndex >= asset->images.size()) {
        ++result.rejectedTextureCount;
        continue;
      }
      const StaticMeshImage& image = asset->images[material.baseColorImageIndex];
      const auto sameTexture = [&](const StaticMeshTextureCpuRecord& texture) {
        return texture.contentHash == image.contentHash &&
               texture.width == image.width && texture.height == image.height &&
               texture.wrapS == material.wrapS &&
               texture.wrapT == material.wrapT &&
               texture.minFilter == material.minFilter &&
               texture.magFilter == material.magFilter &&
               texture.rgba8 == image.rgba8;
      };
      auto texture = std::find_if(result.textures.begin(),
                                  result.textures.end(), sameTexture);
      std::uint32_t textureIndex = 0U;
      if (texture == result.textures.end()) {
        StaticMeshTextureCpuRecord record;
        record.contentHash = image.contentHash;
        record.width = image.width;
        record.height = image.height;
        record.wrapS = material.wrapS;
        record.wrapT = material.wrapT;
        record.minFilter = material.minFilter;
        record.magFilter = material.magFilter;
        record.rgba8 = image.rgba8;
        textureIndex = static_cast<std::uint32_t>(result.textures.size());
        result.textures.push_back(std::move(record));
      } else {
        textureIndex = static_cast<std::uint32_t>(
            std::distance(result.textures.begin(), texture));
      }
      result.materialBindings.push_back(
          {asset->id, static_cast<std::uint32_t>(materialIndex),
           textureIndex});
    }
  }
  std::sort(result.materialBindings.begin(), result.materialBindings.end(),
            [](const StaticMeshMaterialTextureBinding& lhs,
               const StaticMeshMaterialTextureBinding& rhs) {
              return std::pair{lhs.assetId, lhs.materialIndex} <
                     std::pair{rhs.assetId, rhs.materialIndex};
            });
  return result;
}

void StaticMeshMaterialTextureStore::create(
    const StaticMeshMaterialTextureCreateInfo& createInfo,
    VulkanMemoryAllocator& allocator,
    StaticMeshAssetCache* staticMeshAssets) {
  destroy(createInfo.device, allocator);
  const StaticMeshTextureCpuResources cpu =
      buildStaticMeshTextureCpuResources(staticMeshAssets);
  resources_.rejectedTextureCount = cpu.rejectedTextureCount;
#if defined(IGGY3D_HAS_VULKAN)
  VkDescriptorSetLayoutBinding textureBinding{};
  textureBinding.binding = 0U;
  textureBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureBinding.descriptorCount = 1U;
  textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1U;
  layoutInfo.pBindings = &textureBinding;
  if (vkCreateDescriptorSetLayout(createInfo.device, &layoutInfo, nullptr,
                                  &resources_.descriptorSetLayout) !=
      VK_SUCCESS) {
    resources_.rejectedTextureCount += cpu.textures.size();
    return;
  }
  resources_.ready = true;

  std::vector<std::uint32_t> cpuToGpu(
      cpu.textures.size(), kInvalidMaterialTextureIndex);
  if (!cpu.textures.empty()) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount =
        static_cast<std::uint32_t>(cpu.textures.size());
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = static_cast<std::uint32_t>(cpu.textures.size());
    poolInfo.poolSizeCount = 1U;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(createInfo.device, &poolInfo, nullptr,
                               &resources_.descriptorPool) != VK_SUCCESS) {
      resources_.rejectedTextureCount += cpu.textures.size();
    } else {
      resources_.textures.reserve(cpu.textures.size());
      resources_.descriptorSets.reserve(cpu.textures.size());
      for (std::size_t textureIndex = 0U;
           textureIndex < cpu.textures.size(); ++textureIndex) {
        StaticMeshTextureGpuRecord texture;
        if (!uploadTexture(allocator, createInfo, cpu.textures[textureIndex],
                           texture)) {
          ++resources_.rejectedTextureCount;
          continue;
        }
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = resources_.descriptorPool;
        allocateInfo.descriptorSetCount = 1U;
        allocateInfo.pSetLayouts = &resources_.descriptorSetLayout;
        if (vkAllocateDescriptorSets(createInfo.device, &allocateInfo,
                                     &texture.descriptorSet) != VK_SUCCESS) {
          vkDestroySampler(createInfo.device, texture.sampler, nullptr);
          vkDestroyImageView(createInfo.device, texture.imageView, nullptr);
          allocator.destroyImage(texture.image);
          ++resources_.rejectedTextureCount;
          continue;
        }
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = texture.sampler;
        imageInfo.imageView = texture.imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = texture.descriptorSet;
        write.dstBinding = 0U;
        write.descriptorCount = 1U;
        write.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(createInfo.device, 1U, &write, 0U, nullptr);
        cpuToGpu[textureIndex] =
            static_cast<std::uint32_t>(resources_.textures.size());
        resources_.descriptorSets.push_back(texture.descriptorSet);
        resources_.textures.push_back(std::move(texture));
      }
    }
  }
  resources_.materialBindings.reserve(cpu.materialBindings.size());
  for (StaticMeshMaterialTextureBinding binding : cpu.materialBindings) {
    if (binding.textureIndex >= cpuToGpu.size() ||
        cpuToGpu[binding.textureIndex] == kInvalidMaterialTextureIndex) {
      continue;
    }
    binding.textureIndex = cpuToGpu[binding.textureIndex];
    resources_.materialBindings.push_back(std::move(binding));
  }
#else
  (void)createInfo;
  (void)allocator;
#endif
}

void StaticMeshMaterialTextureStore::destroy(
    VkDevice device,
    VulkanMemoryAllocator& allocator) {
#if defined(IGGY3D_HAS_VULKAN)
  if (device != VK_NULL_HANDLE) {
    if (resources_.descriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(device, resources_.descriptorPool, nullptr);
    }
    for (StaticMeshTextureGpuRecord& texture : resources_.textures) {
      if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, texture.sampler, nullptr);
      }
      if (texture.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, texture.imageView, nullptr);
      }
      allocator.destroyImage(texture.image);
    }
    if (resources_.descriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(device, resources_.descriptorSetLayout,
                                   nullptr);
    }
  }
#else
  (void)device;
  (void)allocator;
#endif
  resources_ = {};
}

const StaticMeshMaterialTextureResources&
StaticMeshMaterialTextureStore::resources() const noexcept {
  return resources_;
}

}  // namespace iggy3d::vulkan
