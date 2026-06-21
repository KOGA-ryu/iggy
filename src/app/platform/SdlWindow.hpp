#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;

namespace iggy3d {

struct SdlWindowCreateInfo {
  std::string title = "iggy3d";
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
  bool resizable = true;
  bool highDpi = true;
  bool vulkan = false;
};

struct SdlDrawableExtent {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

struct SdlWindowEventState {
  bool quitRequested = false;
  bool resized = false;
  bool minimized = false;
  bool restored = false;
  bool focused = true;
  std::uint32_t windowWidth = 0;
  std::uint32_t windowHeight = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

class SdlWindow {
public:
  explicit SdlWindow(const SdlWindowCreateInfo& createInfo);
  ~SdlWindow();

  SdlWindow(const SdlWindow&) = delete;
  SdlWindow& operator=(const SdlWindow&) = delete;
  SdlWindow(SdlWindow&& other) noexcept;
  SdlWindow& operator=(SdlWindow&& other) noexcept;

  bool isOpen() const;
  bool isDrawable() const;
  SdlDrawableExtent drawableExtent() const;
  const SdlWindowEventState& eventState() const;
  void pollEvents();

  SDL_Window* nativeWindow() const;

private:
  void release();
  void refreshExtents();

  SDL_Window* window_ = nullptr;
  SdlWindowEventState eventState_;
  bool videoInitialized_ = false;
};

}  // namespace iggy3d
