#include "app/platform/SdlWindow.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <string>

namespace iggy3d {
namespace {

std::uint32_t toExtent(int value) {
  return value <= 0 ? 0U : static_cast<std::uint32_t>(value);
}

constexpr const char* kMouseCaptureStatuses[] = {
    "mouse_capture_released",
    "mouse_capture_active",
};

}  // namespace

SdlWindow::SdlWindow(const SdlWindowCreateInfo& createInfo) {
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    return;
  }
  videoInitialized_ = true;

  SDL_WindowFlags flags = 0;
  if (createInfo.resizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  }
  if (createInfo.highDpi) {
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  }
  if (createInfo.vulkan) {
    flags |= SDL_WINDOW_VULKAN;
  }

  window_ = SDL_CreateWindow(createInfo.title.c_str(), static_cast<int>(createInfo.width),
                             static_cast<int>(createInfo.height), flags);
  if (window_ != nullptr) {
    refreshExtents();
  }
}

SdlWindow::~SdlWindow() {
  release();
}

SdlWindow::SdlWindow(SdlWindow&& other) noexcept
    : window_(other.window_),
      eventState_(other.eventState_),
      eventHook_(other.eventHook_),
      desktopFreePointerMode_(other.desktopFreePointerMode_),
      videoInitialized_(other.videoInitialized_) {
  other.window_ = nullptr;
  other.videoInitialized_ = false;
  other.eventState_ = {};
  other.eventHook_ = {};
  other.desktopFreePointerMode_ = false;
}

SdlWindow& SdlWindow::operator=(SdlWindow&& other) noexcept {
  if (this != &other) {
    release();
    window_ = other.window_;
    eventState_ = other.eventState_;
    eventHook_ = other.eventHook_;
    desktopFreePointerMode_ = other.desktopFreePointerMode_;
    videoInitialized_ = other.videoInitialized_;
    other.window_ = nullptr;
    other.videoInitialized_ = false;
    other.eventState_ = {};
    other.eventHook_ = {};
    other.desktopFreePointerMode_ = false;
  }
  return *this;
}

bool SdlWindow::isOpen() const {
  return window_ != nullptr && !eventState_.quitRequested;
}

bool SdlWindow::isDrawable() const {
  return isOpen() && !eventState_.minimized && eventState_.drawableWidth > 0U &&
         eventState_.drawableHeight > 0U;
}

SdlDrawableExtent SdlWindow::drawableExtent() const {
  return {eventState_.drawableWidth, eventState_.drawableHeight};
}

const SdlWindowEventState& SdlWindow::eventState() const {
  return eventState_;
}

void SdlWindow::setTitle(std::string_view title) {
  if (window_ == nullptr) {
    return;
  }
  const std::string ownedTitle(title);
  (void)SDL_SetWindowTitle(window_, ownedTitle.c_str());
}

SdlMouseCaptureResult SdlWindow::applyRelativeMouseMode(bool enabled) {
  SdlMouseCaptureResult result;
  result.requested = enabled;
  // branch-gate: BG-1075
  if (window_ == nullptr) {
    result.status = "mouse_capture_window_unavailable";
    result.reasonCode = result.status;
    return result;
  }
  // branch-gate: BG-1075
  if (!SDL_SetWindowRelativeMouseMode(window_, enabled)) {
    result.status = "mouse_capture_set_failed";
    result.reasonCode = result.status;
    result.active = SDL_GetWindowRelativeMouseMode(window_);
    return result;
  }

  result.active = SDL_GetWindowRelativeMouseMode(window_);
  result.status =
      kMouseCaptureStatuses[static_cast<unsigned>(result.active)];
  result.reasonCode = result.status;
  return result;
}

SdlMouseCaptureResult SdlWindow::setRelativeMouseMode(bool enabled) {
  // In desktop free-pointer mode a grab request is suppressed to keep the
  // cursor free; a release request still passes through. Only
  // setViewportPointerCapture() may grab. Outside desktop mode this is a
  // straight pass-through — identical to the historical behavior.
  if (desktopFreePointerMode_ && enabled) {
    SdlMouseCaptureResult result = applyRelativeMouseMode(false);
    result.requested = true;
    result.status = "mouse_capture_suppressed_desktop_free_pointer";
    result.reasonCode = result.status;
    return result;
  }
  return applyRelativeMouseMode(enabled);
}

void SdlWindow::setDesktopFreePointerMode(bool enabled) {
  desktopFreePointerMode_ = enabled;
  if (enabled) {
    static_cast<void>(applyRelativeMouseMode(false));
  }
}

SdlMouseCaptureResult SdlWindow::setViewportPointerCapture(bool captured) {
  // The one path that may enter relative mode while the desktop free-pointer
  // gate is active (viewport fly-look). Bypasses the suppression in
  // setRelativeMouseMode by calling the raw applier directly.
  return applyRelativeMouseMode(captured);
}

bool SdlWindow::setTextInputActive(bool enabled) {
  if (window_ == nullptr) {
    return false;
  }
  if (SDL_TextInputActive(window_) == enabled) {
    return true;
  }
  return enabled ? SDL_StartTextInput(window_) : SDL_StopTextInput(window_);
}

void SdlWindow::centerPointer() {
  if (window_ == nullptr) {
    return;
  }
  int width = 0;
  int height = 0;
  SDL_GetWindowSize(window_, &width, &height);
  eventState_.pointerX = static_cast<float>(width) * 0.5F;
  eventState_.pointerY = static_cast<float>(height) * 0.5F;
  eventState_.pointerMoved = false;
  SDL_WarpMouseInWindow(window_, eventState_.pointerX, eventState_.pointerY);
}

void SdlWindow::setEventHook(SdlWindowEventHook hook) {
  eventHook_ = hook;
}

void SdlWindow::pollEvents() {
  eventState_.resized = false;
  eventState_.restored = false;
  eventState_.mouseWheelY = 0.0F;
  eventState_.pointerMoved = false;
  eventState_.primaryPointerPressed = false;
  eventState_.textInput.clear();
  eventState_.backspacePressCount = 0;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (eventHook_.onEvent != nullptr) {
      eventHook_.onEvent(eventHook_.context, event);
    }
    switch (event.type) {
      case SDL_EVENT_QUIT:
        eventState_.quitRequested = true;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        eventState_.resized = true;
        refreshExtents();
        break;
      case SDL_EVENT_WINDOW_MINIMIZED:
        eventState_.minimized = true;
        refreshExtents();
        break;
      case SDL_EVENT_WINDOW_RESTORED:
        eventState_.minimized = false;
        eventState_.restored = true;
        refreshExtents();
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
        eventState_.focused = true;
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        eventState_.focused = false;
        break;
      case SDL_EVENT_MOUSE_WHEEL: {
        const float direction =
            event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0F : 1.0F;
        eventState_.mouseWheelY += event.wheel.y * direction;
        break;
      }
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        eventState_.pointerX = event.button.x;
        eventState_.pointerY = event.button.y;
        eventState_.primaryPointerPressed =
            eventState_.primaryPointerPressed ||
            event.button.button == SDL_BUTTON_LEFT;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        eventState_.pointerX = event.motion.x;
        eventState_.pointerY = event.motion.y;
        eventState_.pointerMoved = true;
        break;
      case SDL_EVENT_TEXT_INPUT:
        if (event.text.text != nullptr) {
          eventState_.textInput.append(event.text.text);
        }
        break;
      case SDL_EVENT_KEY_DOWN:
        if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {
          ++eventState_.backspacePressCount;
        }
        break;
      default:
        break;
    }
  }
  SDL_GetMouseState(&eventState_.pointerX, &eventState_.pointerY);
  refreshExtents();
}

SDL_Window* SdlWindow::nativeWindow() const {
  return window_;
}

void SdlWindow::release() {
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  if (videoInitialized_) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    videoInitialized_ = false;
  }
}

void SdlWindow::refreshExtents() {
  if (window_ == nullptr) {
    eventState_.windowWidth = 0;
    eventState_.windowHeight = 0;
    eventState_.drawableWidth = 0;
    eventState_.drawableHeight = 0;
    return;
  }

  int windowWidth = 0;
  int windowHeight = 0;
  int drawableWidth = 0;
  int drawableHeight = 0;
  SDL_GetWindowSize(window_, &windowWidth, &windowHeight);
  SDL_GetWindowSizeInPixels(window_, &drawableWidth, &drawableHeight);
  eventState_.windowWidth = toExtent(windowWidth);
  eventState_.windowHeight = toExtent(windowHeight);
  eventState_.drawableWidth = toExtent(drawableWidth);
  eventState_.drawableHeight = toExtent(drawableHeight);
  eventState_.minimized = eventState_.minimized || eventState_.drawableWidth == 0U ||
                          eventState_.drawableHeight == 0U;
}

}  // namespace iggy3d
