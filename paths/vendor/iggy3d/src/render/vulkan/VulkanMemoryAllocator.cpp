#include "render/vulkan/VulkanMemoryAllocator.hpp"

#include <algorithm>
#include <cstring>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

RenderReceipt baseReceipt(std::string_view result, std::string_view reasonCode) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/VulkanMemoryAllocator.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "memory_allocator", "manual_packet6_bootstrap");
  appendReceiptField(receipt, "per_frame_allocation_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

#if defined(IGGY3D_HAS_VULKAN)
bool findMemoryType(VkPhysicalDevice physicalDevice,
                    std::uint32_t typeBits,
                    VkMemoryPropertyFlags requiredFlags,
                    std::uint32_t& typeIndex) {
  VkPhysicalDeviceMemoryProperties properties{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
  for (std::uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
    const bool typeSupported = (typeBits & (1U << i)) != 0U;
    const bool flagsSatisfied =
        (properties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags;
    if (typeSupported && flagsSatisfied) {
      typeIndex = i;
      return true;
    }
  }
  return false;
}
#endif

void removeAllocation(std::vector<NamedAllocation>& allocations, VkDeviceMemory memory) {
  allocations.erase(std::remove_if(allocations.begin(), allocations.end(),
                                   [memory](const NamedAllocation& allocation) {
                                     return allocation.memory == memory;
                                   }),
                    allocations.end());
}

}  // namespace

std::string allocationNamesCsv(const std::vector<NamedAllocation>& allocations) {
  std::string names;
  for (std::size_t i = 0; i < allocations.size(); ++i) {
    if (i != 0U) {
      names.push_back(',');
    }
    names += allocations[i].name;
  }
  return names;
}

RenderReceipt makeMemoryBudgetReceipt(const AllocationBudgetSnapshot& budget,
                                      std::string_view result,
                                      std::string_view reasonCode) {
  RenderReceipt receipt = baseReceipt(result, reasonCode);
  appendReceiptField(receipt, "memory_budget_available", budget.budgetAvailable);
  appendReceiptField(receipt, "budget_bytes", budget.heapBudgetBytes);
  appendReceiptField(receipt, "used_budget_bytes", budget.heapUsageBytes);
  appendReceiptField(receipt, "allocation_count", static_cast<std::uint64_t>(budget.allocationCount));
  return receipt;
}

RenderReceipt VulkanMemoryAllocator::create(const VulkanAllocatorCreateInfo& createInfo) {
  physicalDevice_ = createInfo.physicalDevice;
  device_ = createInfo.device;
#if defined(IGGY3D_HAS_VULKAN)
  ready_ = physicalDevice_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE;
#else
  ready_ = false;
#endif
  RenderReceipt receipt = baseReceipt(ready_ ? "pass" : "fail",
                                      ready_ ? "packet6_resource_ready"
                                             : "memory_allocator_create_failed");
  appendReceiptField(receipt, "vulkan_api_version",
                     static_cast<std::uint64_t>(createInfo.vulkanApiVersion));
  appendReceiptField(receipt, "allocation_count", static_cast<std::uint64_t>(allocations_.size()));
  return receipt;
}

RenderReceipt VulkanMemoryAllocator::destroy() {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready");
  appendReceiptField(receipt, "allocation_count", static_cast<std::uint64_t>(allocations_.size()));
  ready_ = false;
  physicalDevice_ = {};
  device_ = {};
  allocations_.clear();
  return receipt;
}

VulkanAllocationResult VulkanMemoryAllocator::createBuffer(std::string_view name,
                                                           VkDeviceSize sizeBytes,
                                                           VkBufferUsageFlags usage,
                                                           VkMemoryPropertyFlags requiredFlags,
                                                           const void* initialData) {
  VulkanAllocationResult result;
  result.receipt = baseReceipt("fail", "memory_allocation_failed");
  appendReceiptField(result.receipt, "allocation_names", std::string(name));
#if defined(IGGY3D_HAS_VULKAN)
  if (!ready_ || sizeBytes == 0U) {
    return result;
  }
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeBytes;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkBuffer buffer = VK_NULL_HANDLE;
  if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    return result;
  }
  VkMemoryRequirements requirements{};
  vkGetBufferMemoryRequirements(device_, buffer, &requirements);
  std::uint32_t memoryType = 0U;
  if (!findMemoryType(physicalDevice_, requirements.memoryTypeBits, requiredFlags, memoryType)) {
    vkDestroyBuffer(device_, buffer, nullptr);
    return result;
  }
  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = memoryType;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  if (vkAllocateMemory(device_, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
    vkDestroyBuffer(device_, buffer, nullptr);
    return result;
  }
  if (initialData != nullptr) {
    void* mapped = nullptr;
    if (vkMapMemory(device_, memory, 0U, sizeBytes, 0U, &mapped) != VK_SUCCESS) {
      vkFreeMemory(device_, memory, nullptr);
      vkDestroyBuffer(device_, buffer, nullptr);
      return result;
    }
    std::memcpy(mapped, initialData, static_cast<std::size_t>(sizeBytes));
    vkUnmapMemory(device_, memory);
  }
  if (vkBindBufferMemory(device_, buffer, memory, 0U) != VK_SUCCESS) {
    vkFreeMemory(device_, memory, nullptr);
    vkDestroyBuffer(device_, buffer, nullptr);
    return result;
  }
  result.buffer.buffer = buffer;
  result.buffer.sizeBytes = sizeBytes;
  result.buffer.usage = usage;
  result.buffer.allocation = {std::string(name), memory, requirements.size, memoryType};
  allocations_.push_back(result.buffer.allocation);
  result.outcome = RenderOutcome::Ok;
  result.reason = {"packet6_resource_ready", "packet 6 resource ready"};
  result.receipt = baseReceipt("pass", result.reason.code);
  appendReceiptField(result.receipt, "allocation_names", std::string(name));
  appendReceiptField(result.receipt, "allocation_count", static_cast<std::uint64_t>(allocations_.size()));
#else
  (void)sizeBytes;
  (void)usage;
  (void)requiredFlags;
  (void)initialData;
#endif
  return result;
}

VulkanAllocationResult VulkanMemoryAllocator::createImage(std::string_view name,
                                                          VkExtent3D extent,
                                                          VkFormat format,
                                                          VkImageUsageFlags usage,
                                                          VkMemoryPropertyFlags requiredFlags) {
  VulkanAllocationResult result;
  result.receipt = baseReceipt("fail", "memory_allocation_failed");
  appendReceiptField(result.receipt, "allocation_names", std::string(name));
#if defined(IGGY3D_HAS_VULKAN)
  if (!ready_ || extent.width == 0U || extent.height == 0U || extent.depth == 0U) {
    return result;
  }
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent = extent;
  imageInfo.mipLevels = 1U;
  imageInfo.arrayLayers = 1U;
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkImage image = VK_NULL_HANDLE;
  if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    return result;
  }
  VkMemoryRequirements requirements{};
  vkGetImageMemoryRequirements(device_, image, &requirements);
  std::uint32_t memoryType = 0U;
  if (!findMemoryType(physicalDevice_, requirements.memoryTypeBits, requiredFlags, memoryType)) {
    vkDestroyImage(device_, image, nullptr);
    return result;
  }
  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = memoryType;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  if (vkAllocateMemory(device_, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
    vkDestroyImage(device_, image, nullptr);
    return result;
  }
  if (vkBindImageMemory(device_, image, memory, 0U) != VK_SUCCESS) {
    vkFreeMemory(device_, memory, nullptr);
    vkDestroyImage(device_, image, nullptr);
    return result;
  }
  result.image.image = image;
  result.image.format = format;
  result.image.extent = extent;
  result.image.usage = usage;
  result.image.allocation = {std::string(name), memory, requirements.size, memoryType};
  allocations_.push_back(result.image.allocation);
  result.outcome = RenderOutcome::Ok;
  result.reason = {"packet6_resource_ready", "packet 6 resource ready"};
  result.receipt = baseReceipt("pass", result.reason.code);
  appendReceiptField(result.receipt, "allocation_names", std::string(name));
  appendReceiptField(result.receipt, "allocation_count", static_cast<std::uint64_t>(allocations_.size()));
#else
  (void)extent;
  (void)format;
  (void)usage;
  (void)requiredFlags;
#endif
  return result;
}

RenderReceipt VulkanMemoryAllocator::destroyBuffer(VulkanBufferAllocation& buffer) {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready");
#if defined(IGGY3D_HAS_VULKAN)
  if (device_ != VK_NULL_HANDLE) {
    if (buffer.buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device_, buffer.buffer, nullptr);
    }
    if (buffer.allocation.memory != VK_NULL_HANDLE) {
      vkFreeMemory(device_, buffer.allocation.memory, nullptr);
    }
  }
#endif
  removeAllocation(allocations_, buffer.allocation.memory);
  buffer = {};
  appendReceiptField(receipt, "allocation_count", static_cast<std::uint64_t>(allocations_.size()));
  return receipt;
}

RenderReceipt VulkanMemoryAllocator::destroyImage(VulkanImageAllocation& image) {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready");
#if defined(IGGY3D_HAS_VULKAN)
  if (device_ != VK_NULL_HANDLE) {
    if (image.image != VK_NULL_HANDLE) {
      vkDestroyImage(device_, image.image, nullptr);
    }
    if (image.allocation.memory != VK_NULL_HANDLE) {
      vkFreeMemory(device_, image.allocation.memory, nullptr);
    }
  }
#endif
  removeAllocation(allocations_, image.allocation.memory);
  image = {};
  appendReceiptField(receipt, "allocation_count", static_cast<std::uint64_t>(allocations_.size()));
  return receipt;
}

const std::vector<NamedAllocation>& VulkanMemoryAllocator::allocations() const {
  return allocations_;
}

bool VulkanMemoryAllocator::ready() const {
  return ready_;
}

}  // namespace iggy3d::vulkan
