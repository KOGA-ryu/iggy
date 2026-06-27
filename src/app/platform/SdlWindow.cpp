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
      videoInitialized_(other.videoInitialized_) {
  other.window_ = nullptr;
  other.videoInitialized_ = false;
  other.eventState_ = {};
}

SdlWindow& SdlWindow::operator=(SdlWindow&& other) noexcept {
  if (this != &other) {
    release();
    window_ = other.window_;
    eventState_ = other.eventState_;
    videoInitialized_ = other.videoInitialized_;
    other.window_ = nullptr;
    other.videoInitialized_ = false;
    other.eventState_ = {};
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

SdlMouseCaptureResult SdlWindow::setRelativeMouseMode(bool enabled) {
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

void SdlWindow::pollEvents() {
  eventState_.resized = false;
  eventState_.restored = false;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      eventState_.quitRequested = true;
    } else if (event.type == SDL_EVENT_WINDOW_RESIZED ||
               event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
      eventState_.resized = true;
      refreshExtents();
    } else if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
      eventState_.minimized = true;
      refreshExtents();
    } else if (event.type == SDL_EVENT_WINDOW_RESTORED) {
      eventState_.minimized = false;
      eventState_.restored = true;
      refreshExtents();
    } else if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
      eventState_.focused = true;
    } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
      eventState_.focused = false;
    }
  }
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
