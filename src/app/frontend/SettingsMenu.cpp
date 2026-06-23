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

FrontendSettings defaultFrontendSettings() {
  return {};
}

void restoreFrontendSettingsDefaults(FrontendSettings& settings) {
  settings = defaultFrontendSettings();
}

}  // namespace iggy3d
