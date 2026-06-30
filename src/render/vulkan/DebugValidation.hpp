#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "render/RendererConfig.hpp"
#include "render/vulkan/VulkanFunctions.hpp"

namespace iggy3d::vulkan {

enum class ValidationRequestMode : std::uint8_t {
  Off,
  Optional,
  Required,
};

struct DebugValidationConfig {
  ValidationRequestMode validationMode = ValidationRequestMode::Off;
  ValidationRequestMode syncValidationMode = ValidationRequestMode::Off;
  bool strictVulkan = false;
  bool debugMessengerRequested = false;
  std::filesystem::path diagnosticsDir;
};

struct DebugValidationPlan {
  bool validationLayerRequested = false;
  bool validationLayerRequired = false;
  bool validationLayerFound = false;
  bool debugUtilsRequested = false;
  bool debugUtilsRequired = false;
  bool debugUtilsFound = false;
  bool syncValidationRequested = false;
  bool syncValidationRequired = false;
  std::vector<const char*> instanceLayers;
  std::vector<const char*> instanceExtensions;
  RenderReceipt receipt;
};

struct DebugValidationCounters {
  std::uint64_t errorCount = 0;
  std::uint64_t warningCount = 0;
  std::uint64_t infoCount = 0;
  std::uint64_t verboseCount = 0;
  std::uint64_t syncErrorCount = 0;
  std::uint64_t syncWarningCount = 0;
};

class DebugValidation {
public:
  DebugValidation() = default;
  ~DebugValidation();

  DebugValidation(const DebugValidation&) = delete;
  DebugValidation& operator=(const DebugValidation&) = delete;

  DebugValidationPlan buildPlan(const DebugValidationConfig& config);
  RenderReceipt createMessenger(VkInstance instance, const DebugValidationPlan& plan);
  RenderReceipt destroyMessenger(VkInstance instance);

#if defined(IGGY3D_HAS_VULKAN)
  void recordMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                     const char* message);
#endif

private:
  DebugValidationCounters counters_;
  std::vector<std::string> recentMessages_;
#if defined(IGGY3D_HAS_VULKAN)
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
#else
  void* messenger_ = nullptr;
#endif
};

ValidationRequestMode validationRequestModeFromConfig(ValidationMode mode);
std::string_view validationRequestModeName(ValidationRequestMode mode);

}  // namespace iggy3d::vulkan
