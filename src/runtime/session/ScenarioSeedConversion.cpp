#include "runtime/session/ScenarioSeedConversion.hpp"

#include <utility>

#include "runtime/session/Session.hpp"

namespace iggy3d {
namespace {

template <typename T>
Result<T> invalid(std::string code, std::string message) {
  Result<T> result;
  result.error = {std::move(code), std::move(message)};
  return result;
}

template <typename T>
Result<T> converted(T value) {
  Result<T> result;
  result.status = ResultStatus::Ok;
  result.value = value;
  return result;
}

}  // namespace

Result<PlayerSlotKind> playerSlotKindFromScenario(ScenarioPlayerSlotKind kind) {
  switch (kind) {
    case ScenarioPlayerSlotKind::Unknown:
      return converted(PlayerSlotKind::Unknown);
    case ScenarioPlayerSlotKind::Local:
      return converted(PlayerSlotKind::Local);
    case ScenarioPlayerSlotKind::Remote:
      return converted(PlayerSlotKind::Remote);
    case ScenarioPlayerSlotKind::Ai:
      return converted(PlayerSlotKind::Ai);
    case ScenarioPlayerSlotKind::Observer:
      return converted(PlayerSlotKind::Observer);
  }
  return invalid<PlayerSlotKind>("scenario_seed.invalid_player_slot_kind",
                                 "scenario player slot kind is out of range");
}

Result<ClockMode> clockModeFromScenario(ScenarioClockMode mode) {
  switch (mode) {
    case ScenarioClockMode::Normal:
      return converted(ClockMode::Normal);
    case ScenarioClockMode::Slow:
      return converted(ClockMode::Slow);
    case ScenarioClockMode::Paused:
      return converted(ClockMode::Paused);
  }
  return invalid<ClockMode>("scenario_seed.invalid_clock_mode",
                            "scenario clock mode is out of range");
}

Result<CameraMode> cameraModeFromScenario(ScenarioCameraMode mode) {
  switch (mode) {
    case ScenarioCameraMode::FirstPerson:
      return converted(CameraMode::FirstPerson);
    case ScenarioCameraMode::ThirdPerson:
      return converted(CameraMode::ThirdPerson);
    case ScenarioCameraMode::TacticalOverhead:
      return converted(CameraMode::TacticalOverhead);
  }
  return invalid<CameraMode>("scenario_seed.invalid_camera_mode",
                             "scenario camera mode is out of range");
}

Result<PatrolMode> patrolModeFromScenario(ScenarioPatrolMode mode) {
  switch (mode) {
    case ScenarioPatrolMode::Loop:
      return converted(PatrolMode::Loop);
    case ScenarioPatrolMode::PingPong:
      return converted(PatrolMode::PingPong);
  }
  return invalid<PatrolMode>("scenario_seed.invalid_patrol_mode",
                             "scenario patrol mode is out of range");
}

}  // namespace iggy3d
