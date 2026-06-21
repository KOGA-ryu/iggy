#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanMemoryAllocator.hpp"
#include "render/vulkan/VulkanTypes.hpp"

namespace iggy3d::vulkan {

struct NormalizedCapture {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::string sourceFormat = "unavailable";
  std::vector<std::uint8_t> rgba;
};

struct FrameCaptureArtifacts {
  std::filesystem::path screenshotPath;
  std::filesystem::path rawPath;
  std::filesystem::path metaPath;
  std::filesystem::path hashPath;
};

struct FrameCaptureCreateInfo {
  VkPhysicalDevice physicalDevice{};
  VkDevice device{};
  VkExtent2D extent{};
  VkFormat colorFormat{};
};

struct FrameCaptureResult {
  bool written = false;
  std::string hash = "none";
  double nonBackgroundPixelCoverage = 0.0;
  RenderReceipt receipt;
};

class FrameCapture {
public:
  FrameCapture() = default;
  ~FrameCapture();

  FrameCapture(const FrameCapture&) = delete;
  FrameCapture& operator=(const FrameCapture&) = delete;

  RenderReceipt create(const FrameCaptureCreateInfo& createInfo);
  void destroy();

  VkBuffer buffer() const;
  VkDeviceSize bufferSizeBytes() const;
  VkExtent2D extent() const;
  bool ready() const;
  NormalizedCapture readMappedRgba() const;

private:
  VulkanMemoryAllocator allocator_;
  VulkanBufferAllocation readback_;
  VkExtent2D extent_{};
  VkDevice device_{};
  VkFormat colorFormat_{};
  bool ready_ = false;
};

std::string sha256NormalizedRgb(const NormalizedCapture& capture);
std::string_view captureFormatName(VkFormat format);
NormalizedCapture normalizeCapturePixels(const std::uint8_t* source,
                                         std::size_t sourceSize,
                                         std::uint32_t width,
                                         std::uint32_t height,
                                         VkFormat sourceFormat);
double nonBackgroundPixelCoverage(const NormalizedCapture& capture,
                                  std::uint8_t backgroundR,
                                  std::uint8_t backgroundG,
                                  std::uint8_t backgroundB);
FrameCaptureResult writePacket7CaptureArtifacts(const NormalizedCapture& capture,
                                                const FrameCaptureArtifacts& artifacts);

}  // namespace iggy3d::vulkan
