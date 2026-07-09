#pragma once

#include <cstdint>

namespace iggy3d {

using ScenarioPlayerSlotId = std::uint32_t;
inline constexpr ScenarioPlayerSlotId kInvalidScenarioPlayerSlotId = UINT32_MAX;

enum class ScenarioPlayerSlotKind : std::uint8_t {
  Unknown,
  Local,
  Remote,
  Ai,
  Observer,
};

enum class ScenarioClockMode : std::uint8_t {
  Normal,
  Slow,
  Paused,
};

enum class ScenarioCameraMode : std::uint8_t {
  FirstPerson,
  ThirdPerson,
  TacticalOverhead,
};

enum class ScenarioPatrolMode : std::uint8_t {
  Loop,
  PingPong,
};

}  // namespace iggy3d
