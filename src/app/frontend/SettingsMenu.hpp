#pragma once

#include <string_view>

namespace iggy3d {

enum class FrontendInputBackend {
  Keyboard,
  Gamepad,
  Auto,
  Scripted,
};

struct FrontendSettings {
  FrontendInputBackend inputBackend = FrontendInputBackend::Keyboard;
  float lookSensitivity = 1.0F;
  bool invertLook = false;
};

std::string_view frontendInputBackendName(FrontendInputBackend backend);
FrontendSettings defaultFrontendSettings();
void restoreFrontendSettingsDefaults(FrontendSettings& settings);

}  // namespace iggy3d
