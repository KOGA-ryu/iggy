#include "app/frontend/SettingsMenu.hpp"

namespace iggy3d {

std::string_view frontendInputBackendName(FrontendInputBackend backend) {
  switch (backend) {
    case FrontendInputBackend::Keyboard:
      return "keyboard";
    case FrontendInputBackend::Gamepad:
      return "gamepad";
    case FrontendInputBackend::Auto:
      return "auto";
    case FrontendInputBackend::Scripted:
      return "scripted";
  }
  return "keyboard";
}

std::string_view frontendSettingsTabName(FrontendSettingsTab tab) {
  switch (tab) {
    case FrontendSettingsTab::None:
      return "none";
    case FrontendSettingsTab::Input:
      return "input";
    case FrontendSettingsTab::Controls:
      return "controls";
    case FrontendSettingsTab::Camera:
      return "camera";
    case FrontendSettingsTab::Gameplay:
      return "gameplay";
    case FrontendSettingsTab::VideoDisplay:
      return "video_display";
    case FrontendSettingsTab::Audio:
      return "audio";
    case FrontendSettingsTab::Accessibility:
      return "accessibility";
    case FrontendSettingsTab::Developer:
      return "developer";
  }
  return "none";
}

std::string_view frontendCameraModeName(FrontendCameraMode mode) {
  switch (mode) {
    case FrontendCameraMode::FirstPerson:
      return "first_person";
    case FrontendCameraMode::ThirdPerson:
      return "third_person";
    case FrontendCameraMode::Tactical:
      return "tactical";
  }
  return "first_person";
}

std::string_view frontendRendererChoiceName(FrontendRendererChoice renderer) {
  switch (renderer) {
    case FrontendRendererChoice::Auto:
      return "auto";
    case FrontendRendererChoice::Null:
      return "null";
    case FrontendRendererChoice::Vulkan:
      return "vulkan";
  }
  return "auto";
}

std::string_view frontendWindowModeName(FrontendWindowMode mode) {
  switch (mode) {
    case FrontendWindowMode::NoWindow:
      return "no_window";
    case FrontendWindowMode::Window:
      return "window";
  }
  return "no_window";
}

const std::vector<FrontendSettingsTab>& settingsTabOrder() {
  static const std::vector<FrontendSettingsTab> tabs = {
      FrontendSettingsTab::Input,
      FrontendSettingsTab::Controls,
      FrontendSettingsTab::Camera,
      FrontendSettingsTab::Gameplay,
      FrontendSettingsTab::VideoDisplay,
      FrontendSettingsTab::Audio,
      FrontendSettingsTab::Accessibility,
      FrontendSettingsTab::Developer,
  };
  return tabs;
}

std::string_view defaultSettingsRowName(FrontendSettingsTab tab) {
  switch (tab) {
    case FrontendSettingsTab::Input:
      return "input_backend";
    case FrontendSettingsTab::Controls:
      return "look_sensitivity";
    case FrontendSettingsTab::Camera:
      return "camera_mode";
    case FrontendSettingsTab::Gameplay:
      return "difficulty";
    case FrontendSettingsTab::VideoDisplay:
      return "renderer";
    case FrontendSettingsTab::Audio:
      return "master_volume";
    case FrontendSettingsTab::Accessibility:
      return "high_contrast";
    case FrontendSettingsTab::Developer:
      return "dev_tools_enabled";
    case FrontendSettingsTab::None:
      break;
  }
  return "none";
}

FrontendSettings defaultFrontendSettings() {
  return {};
}

void restoreFrontendSettingsDefaults(FrontendSettings& settings) {
  settings = defaultFrontendSettings();
}

void applyFrontendSettingsDraft(FrontendSettings& current,
                                const FrontendSettings& draft) {
  current = draft;
  current.rendererChangePending = current.renderer != defaultFrontendSettings().renderer;
}

}  // namespace iggy3d
