#include "core/result/Result.hpp"
#include "runtime/session/ScenarioSeedConversion.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool validValuesConvert() {
  const auto player = iggy3d::playerSlotKindFromScenario(
      iggy3d::ScenarioPlayerSlotKind::Local);
  const auto clock = iggy3d::clockModeFromScenario(iggy3d::ScenarioClockMode::Slow);
  const auto camera = iggy3d::cameraModeFromScenario(
      iggy3d::ScenarioCameraMode::TacticalOverhead);
  const auto patrol = iggy3d::patrolModeFromScenario(
      iggy3d::ScenarioPatrolMode::PingPong);
  return expect(player.status == iggy3d::ResultStatus::Ok &&
                    player.value == iggy3d::PlayerSlotKind::Local,
                "player slot conversion") &&
         expect(clock.status == iggy3d::ResultStatus::Ok &&
                    clock.value == iggy3d::ClockMode::Slow,
                "clock conversion") &&
         expect(camera.status == iggy3d::ResultStatus::Ok &&
                    camera.value == iggy3d::CameraMode::TacticalOverhead,
                "camera conversion") &&
         expect(patrol.status == iggy3d::ResultStatus::Ok &&
                    patrol.value == iggy3d::PatrolMode::PingPong,
                "patrol conversion");
}

bool invalidValuesFailClosed() {
  const auto player = iggy3d::playerSlotKindFromScenario(
      static_cast<iggy3d::ScenarioPlayerSlotKind>(99U));
  const auto clock = iggy3d::clockModeFromScenario(
      static_cast<iggy3d::ScenarioClockMode>(99U));
  const auto camera = iggy3d::cameraModeFromScenario(
      static_cast<iggy3d::ScenarioCameraMode>(99U));
  const auto patrol = iggy3d::patrolModeFromScenario(
      static_cast<iggy3d::ScenarioPatrolMode>(99U));
  return expect(player.status == iggy3d::ResultStatus::Error &&
                    player.error.code == "scenario_seed.invalid_player_slot_kind",
                "invalid player slot fails closed") &&
         expect(clock.status == iggy3d::ResultStatus::Error &&
                    clock.error.code == "scenario_seed.invalid_clock_mode",
                "invalid clock fails closed") &&
         expect(camera.status == iggy3d::ResultStatus::Error &&
                    camera.error.code == "scenario_seed.invalid_camera_mode",
                "invalid camera fails closed") &&
         expect(patrol.status == iggy3d::ResultStatus::Error &&
                    patrol.error.code == "scenario_seed.invalid_patrol_mode",
                "invalid patrol fails closed");
}

}  // namespace

int main() {
  return validValuesConvert() && invalidValuesFailClosed() ? 0 : 1;
}
