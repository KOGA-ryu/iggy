#include "EditorGamepad.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] float normalizedAxis(Sint16 value) noexcept {
  return value < 0 ? static_cast<float>(value) / 32768.0F
                   : static_cast<float>(value) / 32767.0F;
}

struct ButtonMapping {
  iggy3d::creative::CreativeControllerButton button;
  SDL_GamepadButton sdlButton;
};

constexpr std::array kButtonMappings{
    ButtonMapping{iggy3d::creative::CreativeControllerButton::South,
                  SDL_GAMEPAD_BUTTON_SOUTH},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::East,
                  SDL_GAMEPAD_BUTTON_EAST},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::West,
                  SDL_GAMEPAD_BUTTON_WEST},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::North,
                  SDL_GAMEPAD_BUTTON_NORTH},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::Back,
                  SDL_GAMEPAD_BUTTON_BACK},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::Start,
                  SDL_GAMEPAD_BUTTON_START},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::LeftStick,
                  SDL_GAMEPAD_BUTTON_LEFT_STICK},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::RightStick,
                  SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::LeftShoulder,
                  SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::RightShoulder,
                  SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::DpadUp,
                  SDL_GAMEPAD_BUTTON_DPAD_UP},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::DpadDown,
                  SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::DpadLeft,
                  SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    ButtonMapping{iggy3d::creative::CreativeControllerButton::DpadRight,
                  SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
};

}  // namespace

CreativeEditorGamepad::CreativeEditorGamepad() {
  subsystemInitialized_ = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
  refreshConnection();
}

CreativeEditorGamepad::~CreativeEditorGamepad() {
  close();
  if (subsystemInitialized_) {
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
  }
}

iggy3d::creative::CreativeControllerSample CreativeEditorGamepad::sample() {
  ++sampleCount_;
  if (gamepad_ == nullptr || !SDL_GamepadConnected(gamepad_) ||
      sampleCount_ % 120U == 0U) {
    refreshConnection();
  }

  iggy3d::creative::CreativeControllerSample result;
  if (gamepad_ == nullptr) {
    return result;
  }

  result.connected = true;
  iggy3d::creative::setCreativeControllerAxis(
      result, iggy3d::creative::CreativeControllerAxis::LeftStickX,
      iggy3d::creative::canonicalCreativeStickComponent(
          SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX),
          iggy3d::creative::CreativeStickComponent::X));
  iggy3d::creative::setCreativeControllerAxis(
      result, iggy3d::creative::CreativeControllerAxis::LeftStickY,
      iggy3d::creative::canonicalCreativeStickComponent(
          SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY),
          iggy3d::creative::CreativeStickComponent::Y));
  iggy3d::creative::setCreativeControllerAxis(
      result, iggy3d::creative::CreativeControllerAxis::RightStickX,
      iggy3d::creative::canonicalCreativeStickComponent(
          SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX),
          iggy3d::creative::CreativeStickComponent::X));
  iggy3d::creative::setCreativeControllerAxis(
      result, iggy3d::creative::CreativeControllerAxis::RightStickY,
      iggy3d::creative::canonicalCreativeStickComponent(
          SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY),
          iggy3d::creative::CreativeStickComponent::Y));
  iggy3d::creative::setCreativeControllerAxis(
      result, iggy3d::creative::CreativeControllerAxis::LeftTrigger,
      std::max(0.0F, normalizedAxis(SDL_GetGamepadAxis(
                         gamepad_, SDL_GAMEPAD_AXIS_LEFT_TRIGGER))));
  iggy3d::creative::setCreativeControllerAxis(
      result, iggy3d::creative::CreativeControllerAxis::RightTrigger,
      std::max(0.0F, normalizedAxis(SDL_GetGamepadAxis(
                         gamepad_, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER))));
  for (const ButtonMapping& mapping : kButtonMappings) {
    iggy3d::creative::setCreativeControllerButton(
        result, mapping.button,
        SDL_GetGamepadButton(gamepad_, mapping.sdlButton));
  }
  return result;
}

void CreativeEditorGamepad::refreshConnection() {
  if (gamepad_ != nullptr && SDL_GamepadConnected(gamepad_)) {
    return;
  }
  close();
  if (!subsystemInitialized_) {
    return;
  }

  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  if (ids == nullptr || count <= 0) {
    SDL_free(ids);
    return;
  }
  gamepad_ = SDL_OpenGamepad(ids[0]);
  SDL_free(ids);
  if (gamepad_ != nullptr) {
    const char* name = SDL_GetGamepadName(gamepad_);
    SDL_Log("iggy3d_creative: gamepad connected name='%s'",
            name != nullptr ? name : "unknown");
  }
}

void CreativeEditorGamepad::close() {
  if (gamepad_ != nullptr) {
    SDL_CloseGamepad(gamepad_);
    gamepad_ = nullptr;
  }
}

}  // namespace iggy3d_creative_app
