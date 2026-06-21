#include "render/vulkan/DebugValidation.hpp"

#include <algorithm>
#include <cstring>
#include <string>

namespace iggy3d::vulkan {
namespace {

void appendValidationBase(RenderReceipt& receipt, std::string_view reasonCode) {
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/DebugValidation.cpp");
  appendReceiptField(receipt, "packet_order", "4");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "reason_code", reasonCode);
}

#if defined(IGGY3D_HAS_VULKAN)
bool layerAvailable(const char* requested) {
  std::uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> layers(layerCount);
  if (layerCount != 0U) {
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
  }
  return std::any_of(layers.begin(), layers.end(), [requested](const VkLayerProperties& layer) {
    return std::strcmp(layer.layerName, requested) == 0;
  });
}

bool extensionAvailable(const char* requested) {
  std::uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  if (extensionCount != 0U) {
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
  }
  return std::any_of(extensions.begin(), extensions.end(),
                     [requested](const VkExtensionProperties& extension) {
                       return std::strcmp(extension.extensionName, requested) == 0;
                     });
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData) {
  auto* validation = static_cast<DebugValidation*>(userData);
  if (validation != nullptr) {
    validation->recordMessage(severity, messageType,
                              callbackData == nullptr ? nullptr : callbackData->pMessage);
  }
  return VK_FALSE;
}
#endif

}  // namespace

DebugValidation::~DebugValidation() = default;

ValidationRequestMode validationRequestModeFromConfig(ValidationMode mode) {
  switch (mode) {
    case ValidationMode::Off:
      return ValidationRequestMode::Off;
    case ValidationMode::Optional:
      return ValidationRequestMode::Optional;
    case ValidationMode::Required:
      return ValidationRequestMode::Required;
  }
  return ValidationRequestMode::Off;
}

std::string_view validationRequestModeName(ValidationRequestMode mode) {
  switch (mode) {
    case ValidationRequestMode::Off:
      return "off";
    case ValidationRequestMode::Optional:
      return "optional";
    case ValidationRequestMode::Required:
      return "required";
  }
  return "off";
}

DebugValidationPlan DebugValidation::buildPlan(const DebugValidationConfig& config) {
  DebugValidationPlan plan;
  plan.validationLayerRequested = config.validationMode != ValidationRequestMode::Off;
  plan.validationLayerRequired = config.validationMode == ValidationRequestMode::Required;
  plan.debugUtilsRequested = config.debugMessengerRequested && plan.validationLayerRequested;
  plan.debugUtilsRequired = plan.debugUtilsRequested && config.strictVulkan;
  plan.syncValidationRequested = config.syncValidationMode != ValidationRequestMode::Off;
  plan.syncValidationRequired = config.syncValidationMode == ValidationRequestMode::Required;

#if defined(IGGY3D_HAS_VULKAN)
  plan.validationLayerFound =
      plan.validationLayerRequested && layerAvailable("VK_LAYER_KHRONOS_validation");
  plan.debugUtilsFound =
      plan.debugUtilsRequested && extensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else
  plan.validationLayerFound = false;
  plan.debugUtilsFound = false;
#endif
  if (plan.validationLayerFound) {
    plan.instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
  }
  if (plan.debugUtilsFound) {
    plan.instanceExtensions.push_back("VK_EXT_debug_utils");
  }

  const bool ok = (!plan.validationLayerRequired || plan.validationLayerFound) &&
                  (!plan.debugUtilsRequired || plan.debugUtilsFound) &&
                  (!plan.syncValidationRequired || plan.validationLayerFound);
  const std::string_view reason =
      ok ? "vulkan_smoke_pass"
         : (!plan.validationLayerFound && plan.validationLayerRequired)
               ? "validation_layer_missing"
               : (!plan.debugUtilsFound && plan.debugUtilsRequired)
                     ? "debug_utils_required_missing"
                     : "validation_layer_missing";
  appendValidationBase(plan.receipt, reason);
  appendReceiptField(plan.receipt, "validation_mode",
                     validationRequestModeName(config.validationMode));
  appendReceiptField(plan.receipt, "sync_validation_mode",
                     validationRequestModeName(config.syncValidationMode));
  appendReceiptField(plan.receipt, "validation",
                     plan.validationLayerFound ? "enabled"
                                               : (plan.validationLayerRequested ? "unavailable"
                                                                                : "disabled"));
  appendReceiptField(plan.receipt, "sync_validation",
                     plan.syncValidationRequested
                         ? (plan.validationLayerFound ? "enabled" : "unavailable")
                         : "disabled");
  appendReceiptField(plan.receipt, "validation_layer_requested", plan.validationLayerRequested);
  appendReceiptField(plan.receipt, "validation_layer_required", plan.validationLayerRequired);
  appendReceiptField(plan.receipt, "validation_layer_found", plan.validationLayerFound);
  appendReceiptField(plan.receipt, "validation_layer_name", "VK_LAYER_KHRONOS_validation");
  appendReceiptField(plan.receipt, "debug_utils_requested", plan.debugUtilsRequested);
  appendReceiptField(plan.receipt, "debug_utils_found", plan.debugUtilsFound);
  appendReceiptField(plan.receipt, "debug_messenger_created", false);
  appendReceiptField(plan.receipt, "validation_error_count", counters_.errorCount);
  appendReceiptField(plan.receipt, "validation_warning_count", counters_.warningCount);
  appendReceiptField(plan.receipt, "validation_info_count", counters_.infoCount);
  appendReceiptField(plan.receipt, "validation_verbose_count", counters_.verboseCount);
  appendReceiptField(plan.receipt, "sync_validation_error_count", counters_.syncErrorCount);
  appendReceiptField(plan.receipt, "sync_validation_warning_count", counters_.syncWarningCount);
  appendReceiptField(plan.receipt, "validation_log_path", "");
  appendReceiptField(plan.receipt, "result", ok ? "pass" : "fail");
  return plan;
}

RenderReceipt DebugValidation::createMessenger(VkInstance instance,
                                               const DebugValidationPlan& plan) {
  RenderReceipt receipt = plan.receipt;
#if defined(IGGY3D_HAS_VULKAN)
  if (instance != VK_NULL_HANDLE && plan.debugUtilsFound) {
    auto createFn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (createFn == nullptr) {
      appendReceiptField(receipt, "reason_code", "debug_utils_required_missing");
      appendReceiptField(receipt, "debug_messenger_created", false);
      appendReceiptField(receipt, "result", plan.debugUtilsRequired ? "fail" : "pass");
      return receipt;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = this;
    const VkResult created = createFn(instance, &createInfo, nullptr, &messenger_);
    appendReceiptField(receipt, "debug_messenger_created", created == VK_SUCCESS);
    if (created != VK_SUCCESS) {
      appendReceiptField(receipt, "reason_code", "debug_messenger_create_failed");
      appendReceiptField(receipt, "result", plan.debugUtilsRequired ? "fail" : "pass");
    }
  }
#else
  (void)instance;
  messenger_ = nullptr;
#endif
  return receipt;
}

RenderReceipt DebugValidation::destroyMessenger(VkInstance instance) {
  RenderReceipt receipt;
  appendValidationBase(receipt, "vulkan_smoke_pass");
#if defined(IGGY3D_HAS_VULKAN)
  const bool hadMessenger = messenger_ != VK_NULL_HANDLE;
  if (hadMessenger && instance != VK_NULL_HANDLE) {
    auto destroyFn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyFn != nullptr) {
      destroyFn(instance, messenger_, nullptr);
    }
  }
  messenger_ = VK_NULL_HANDLE;
#else
  (void)instance;
  const bool hadMessenger = messenger_ != nullptr;
  messenger_ = nullptr;
#endif
  appendReceiptField(receipt, "debug_messenger_had_handle", hadMessenger);
  appendReceiptField(receipt, "debug_messenger_destroyed", true);
  appendReceiptField(receipt, "result", "pass");
  return receipt;
}

const DebugValidationCounters& DebugValidation::counters() const {
  return counters_;
}

#if defined(IGGY3D_HAS_VULKAN)
void DebugValidation::recordMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                    const char* message) {
  if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0U) {
    ++counters_.errorCount;
  } else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0U) {
    ++counters_.warningCount;
  } else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0U) {
    ++counters_.infoCount;
  } else {
    ++counters_.verboseCount;
  }
  if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0U &&
      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0U) {
    ++counters_.syncErrorCount;
  }
  if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0U &&
      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0U) {
    ++counters_.syncWarningCount;
  }
  if (message != nullptr) {
    if (recentMessages_.size() == 8U) {
      recentMessages_.erase(recentMessages_.begin());
    }
    recentMessages_.emplace_back(message);
  }
}
#endif

}  // namespace iggy3d::vulkan
