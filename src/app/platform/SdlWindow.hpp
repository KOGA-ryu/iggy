#pragma once

#include <cstdint>
#include <string>
#include <string_view>

struct SDL_Window;
union SDL_Event;

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
  float mouseWheelY = 0.0F;
  float pointerX = 0.0F;
  float pointerY = 0.0F;
  bool pointerMoved = false;
  bool primaryPointerPressed = false;
  std::string textInput;
  std::uint32_t backspacePressCount = 0;
  std::uint32_t windowWidth = 0;
  std::uint32_t windowHeight = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct SdlMouseCaptureResult {
  bool requested = false;
  bool active = false;
  std::string status = "mouse_capture_not_requested";
  std::string reasonCode = "mouse_capture_not_requested";
};

// Allocation-free observer over the raw SDL event stream. The hook fires for
// EVERY polled event — including kinds pollEvents itself discards (key-up,
// non-left button-down, button-up) — before the window's own handling, which
// always runs regardless of the hook. Context is caller-owned and must
// outlive the window (or be cleared via setEventHook({})).
struct SdlWindowEventHook {
  void (*onEvent)(void* context, const SDL_Event& event) = nullptr;
  void* context = nullptr;
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
  void setTitle(std::string_view title);
  SdlMouseCaptureResult setRelativeMouseMode(bool enabled);
  // Desktop free-pointer gate (plan DD-9). While enabled, the resting state is
  // a free cursor: setRelativeMouseMode(true) requests are suppressed so the
  // ~15 scattered modal-close re-grab sites cannot steal the pointer, and only
  // setViewportPointerCapture() can enter relative mode (viewport fly-look).
  // Disabled by default, so capture mode and shell-off behave exactly as before.
  void setDesktopFreePointerMode(bool enabled);
  SdlMouseCaptureResult setViewportPointerCapture(bool captured);
  bool setTextInputActive(bool enabled);
  void centerPointer();
  void setEventHook(SdlWindowEventHook hook);
  void pollEvents();

  SDL_Window* nativeWindow() const;

private:
  void release();
  void refreshExtents();

  SdlMouseCaptureResult applyRelativeMouseMode(bool enabled);

  SDL_Window* window_ = nullptr;
  SdlWindowEventState eventState_;
  SdlWindowEventHook eventHook_;
  bool desktopFreePointerMode_ = false;
  bool videoInitialized_ = false;
};

}  // namespace iggy3d
