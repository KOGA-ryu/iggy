#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkBuffer = void*;
using VkImage = void*;
using VkDeviceMemory = void*;
using VkDeviceSize = std::uint64_t;
using VkBufferUsageFlags = std::uint32_t;
using VkImageUsageFlags = std::uint32_t;
using VkMemoryPropertyFlags = std::uint32_t;
using VkFormat = std::uint32_t;
struct VkExtent3D {
  std::uint32_t width{};
  std::uint32_t height{};
  std::uint32_t depth{};
};
#endif

namespace iggy3d::vulkan {

struct AllocationBudgetSnapshot {
  bool budgetAvailable = false;
  std::uint64_t heapBudgetBytes = 0;
  std::uint64_t heapUsageBytes = 0;
  std::uint32_t allocationCount = 0;
};

struct NamedAllocation {
  std::string name;
  VkDeviceMemory memory{};
  VkDeviceSize sizeBytes = 0;
  std::uint32_t memoryTypeIndex = 0;
};

struct VulkanAllocatorCreateInfo {
  VkPhysicalDevice physicalDevice{};
  VkDevice device{};
  std::uint32_t vulkanApiVersion = 0;
};

struct VulkanAllocationRequest {
  std::string allocationName;
  VkMemoryPropertyFlags requiredFlags{};
  VkMemoryPropertyFlags preferredFlags{};
};

struct VulkanBufferAllocation {
  VkBuffer buffer{};
  NamedAllocation allocation;
  VkDeviceSize sizeBytes = 0;
  VkBufferUsageFlags usage{};
};

struct VulkanImageAllocation {
  VkImage image{};
  NamedAllocation allocation;
  VkFormat format{};
  VkExtent3D extent{};
  VkImageUsageFlags usage{};
};

struct VulkanAllocationResult {
  VulkanBufferAllocation buffer;
  VulkanImageAllocation image;
  RenderOutcome outcome = RenderOutcome::OutOfMemory;
  RenderReason reason{"memory_allocation_failed", "memory allocation failed"};
  RenderReceipt receipt;
};

class VulkanMemoryAllocator {
public:
  RenderReceipt create(const VulkanAllocatorCreateInfo& createInfo);
  RenderReceipt destroy();

  VulkanAllocationResult createBuffer(std::string_view name,
                                      VkDeviceSize sizeBytes,
                                      VkBufferUsageFlags usage,
                                      VkMemoryPropertyFlags requiredFlags,
                                      const void* initialData = nullptr);
  VulkanAllocationResult createImage(std::string_view name,
                                     VkExtent3D extent,
                                     VkFormat format,
                                     VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags requiredFlags);
  RenderReceipt destroyBuffer(VulkanBufferAllocation& buffer);
  RenderReceipt destroyImage(VulkanImageAllocation& image);

  const std::vector<NamedAllocation>& allocations() const;
  bool ready() const;

private:
  VkPhysicalDevice physicalDevice_{};
  VkDevice device_{};
  std::vector<NamedAllocation> allocations_;
  bool ready_ = false;
};

std::string allocationNamesCsv(const std::vector<NamedAllocation>& allocations);
RenderReceipt makeMemoryBudgetReceipt(const AllocationBudgetSnapshot& budget,
                                      std::string_view result,
                                      std::string_view reasonCode);

}  // namespace iggy3d::vulkan
