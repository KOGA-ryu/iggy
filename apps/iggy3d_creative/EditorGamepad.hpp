#pragma once

#include <cstdint>

#include "app/iggy3d/creative/input/InputRouter.hpp"

struct SDL_Gamepad;

namespace iggy3d_creative_app {

class CreativeEditorGamepad {
public:
  CreativeEditorGamepad();
  ~CreativeEditorGamepad();

  CreativeEditorGamepad(const CreativeEditorGamepad&) = delete;
  CreativeEditorGamepad& operator=(const CreativeEditorGamepad&) = delete;

  [[nodiscard]] iggy3d::creative::CreativeControllerSample sample();

private:
  void refreshConnection();
  void close();

  SDL_Gamepad* gamepad_ = nullptr;
  bool subsystemInitialized_ = false;
  std::uint64_t sampleCount_ = 0;
};

}  // namespace iggy3d_creative_app
