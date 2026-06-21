#pragma once

#include <cstdint>

namespace iggy3d {

enum class ClockMode : std::uint8_t {
  Normal,
  Slow,
  Paused,
};

struct ClockState {
  ClockMode mode = ClockMode::Normal;
  ClockMode previousUnpausedMode = ClockMode::Normal;
  float previousUnpausedTimeScale = 1.0F;
  std::uint64_t tickIndex = 0;
  std::uint32_t fixedTickRateHz = 20;
  float timeScale = 1.0F;
  bool stepRequested = false;
};

}  // namespace iggy3d
