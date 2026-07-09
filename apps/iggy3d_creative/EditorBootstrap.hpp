#pragma once

#include <memory>
#include <string>

#include "app/platform/SdlWindow.hpp"
#include "render/RendererConfig.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::RendererConfig makeCreativeVulkanRendererConfig();
[[nodiscard]] std::unique_ptr<iggy3d::VulkanBackend> createCreativeRenderer(
    iggy3d::SdlWindow& window);
[[nodiscard]] bool captureFrameToPng(iggy3d::VulkanBackend& backend,
                                     const std::string& pngPath);

}  // namespace iggy3d_creative_app
