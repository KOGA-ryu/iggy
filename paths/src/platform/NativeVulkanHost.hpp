#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

union SDL_Event;

namespace paths {

struct NativeLaunchConfig {
  bool offscreen = false;
  std::uint32_t width = 1440;
  std::uint32_t height = 900;
};

struct CapturePaths {
  std::filesystem::path screenshotPath, rawPath, metaPath, hashPath;
};
[[nodiscard]] CapturePaths capturePaths(const std::filesystem::path& screenshot);

enum class FrameStatus { Rendered, Skipped, Closed, Failed };
struct NativeFrameResult {
  FrameStatus status = FrameStatus::Failed;
  std::string error;
};

// Owns the sole native window, Vulkan resources and ImGui frame lifecycle.
// The draw callback only presents UI; neither it nor main sees GPU objects.
class NativeVulkanHost {
public:
  explicit NativeVulkanHost(const NativeLaunchConfig& config);
  ~NativeVulkanHost();
  NativeVulkanHost(const NativeVulkanHost&) = delete;
  NativeVulkanHost& operator=(const NativeVulkanHost&) = delete;
  [[nodiscard]] NativeFrameResult frame(
      const std::function<void(const SDL_Event&)>& onEvent,
      const std::function<void()>& draw);
  [[nodiscard]] bool capture(const CapturePaths& paths, std::string& error);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace paths
