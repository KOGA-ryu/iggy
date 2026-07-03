// ma1s1 GATE 1 — the movement dimension profile mirror + loader precedence. STRUCTURAL /
// byte-identical: earth_standard maps to the app tuning constant (the anti-drift guard).

#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "config/MovementDimensionProfile.hpp"
#include "content/FixtureScenarioLoader.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

// A minimal valid scenario; `extraDefaults` injects movement_profile / movement_distance_meters.
std::string scenario(const std::string& extraDefaults) {
  return std::string(
             "[scenario]\n"
             "id = \"ma1_test\"\n"
             "[defaults]\n"
             "fixed_tick_rate_hz = 20\n"
             "interaction_range_meters = 1.500\n"
             "slow_time_scale = 0.250\n"
             "initial_clock = \"Normal\"\n"
             "default_realtime_camera = \"ThirdPerson\"\n"
             "default_tactical_camera = \"TacticalOverhead\"\n") +
         extraDefaults +
         "[[players]]\n"
         "slot = 0\n"
         "kind = \"Local\"\n"
         "actor = \"player\"\n"
         "[[entities]]\n"
         "stable_name = \"player\"\n"
         "kind = \"Player\"\n"
         "active = true\n"
         "persistent = true\n"
         "position = [1.000, 0.000, 1.000]\n"
         "bounds_min = [-0.250, 0.000, -0.250]\n"
         "bounds_max = [0.250, 1.800, 0.250]\n"
         "targetable = false\n"
         "target_actions = []\n"
         "[[objectives]]\n"
         "id = \"stay\"\n"
         "initial_status = \"Active\"\n"
         "condition = \"InventoryContains\"\n"
         "player_slot = 0\n"
         "item_id = \"phantom\"\n"
         "item_count = 1\n"
         "complete_status = \"Complete\"\n";
}

bool earthMapsByteIdenticalToAppTuning() {
  const iggy3d::MovementDimensionProfile* earth =
      iggy3d::movementDimensionProfileById("earth_standard");
  if (!expect(earth != nullptr, "earth_standard row exists")) {
    return false;
  }
  iggy3d::ProductGameplayMovementTuning tuning;
  tuning.walkSpeedMetersPerSecond = 0.0F;  // scribble to prove the map writes every field
  iggy3d::applyMovementDimensionProfileToTuning(*earth, tuning);
  const iggy3d::ProductGameplayMovementTuning& ref = iggy3d::kProductGameplayMovementTuning;
  const bool idsMatch = tuning.walkProfile == ref.walkProfile &&
                        tuning.sprintProfile == ref.sprintProfile &&
                        tuning.dashProfile == ref.dashProfile;
  const bool numericsMatch =
      tuning.walkSpeedMetersPerSecond == ref.walkSpeedMetersPerSecond &&
      tuning.sprintSpeedMetersPerSecond == ref.sprintSpeedMetersPerSecond &&
      tuning.groundAccelerationMetersPerSecondSquared == ref.groundAccelerationMetersPerSecondSquared &&
      tuning.groundDecelerationMetersPerSecondSquared == ref.groundDecelerationMetersPerSecondSquared &&
      tuning.airControlMultiplier == ref.airControlMultiplier &&
      tuning.inputStepSeconds == ref.inputStepSeconds &&
      tuning.jumpImpulseMetersPerSecond == ref.jumpImpulseMetersPerSecond &&
      tuning.gravityMetersPerSecondSquared == ref.gravityMetersPerSecondSquared &&
      tuning.coyoteTimeSeconds == ref.coyoteTimeSeconds &&
      tuning.jumpBufferSeconds == ref.jumpBufferSeconds &&
      tuning.jumpCutMultiplier == ref.jumpCutMultiplier &&
      tuning.fallGravityMultiplier == ref.fallGravityMultiplier &&
      tuning.lookSensitivity == ref.lookSensitivity &&
      tuning.invertLookEnabled == ref.invertLookEnabled &&
      tuning.dashSpeedMetersPerSecond == ref.dashSpeedMetersPerSecond &&
      tuning.dashDurationSeconds == ref.dashDurationSeconds &&
      tuning.dashCooldownSeconds == ref.dashCooldownSeconds &&
      tuning.wallRunMinSpeedMetersPerSecond == ref.wallRunMinSpeedMetersPerSecond &&
      tuning.wallRunMaxWallNormalY == ref.wallRunMaxWallNormalY &&
      tuning.wallRunDurationSeconds == ref.wallRunDurationSeconds &&
      tuning.wallRunGravityMultiplier == ref.wallRunGravityMultiplier &&
      tuning.wallRunSpeedMultiplier == ref.wallRunSpeedMultiplier &&
      tuning.wallJumpProbeMeters == ref.wallJumpProbeMeters &&
      tuning.wallJumpPushMeters == ref.wallJumpPushMeters &&
      tuning.wallJumpRiseMeters == ref.wallJumpRiseMeters &&
      tuning.wallJumpMinAirborneHeightMeters == ref.wallJumpMinAirborneHeightMeters;
  return expect(idsMatch, "earth_standard ids map identical to the app tuning") &&
         expect(numericsMatch, "earth_standard 26 numerics map BYTE-IDENTICAL to the app tuning");
}

bool byIdLookup() {
  return expect(iggy3d::movementDimensionProfileById("earth_standard") != nullptr,
                "earth_standard resolves") &&
         expect(iggy3d::movementDimensionProfileById("no_such_world") == nullptr,
                "an unknown profile id resolves to nullptr (fail-closed)");
}

bool loaderResolvesAndPrecedence() {
  // Explicit profile + explicit distance.
  const iggy3d::ScenarioLoadResult named =
      iggy3d::parseScenarioText(scenario("movement_profile = \"earth_standard\"\n"
                                         "movement_distance_meters = 3.000\n"));
  bool ok = expect(named.status == iggy3d::ScenarioLoadStatus::Ok, "named-profile scenario parses") &&
            expect(named.seed.movementProfile.id == "earth_standard", "profile resolved") &&
            expect(named.seed.config.movementDistanceMeters == 3.0F, "explicit distance kept");

  // No profile key -> earth_standard.
  const iggy3d::ScenarioLoadResult absent =
      iggy3d::parseScenarioText(scenario("movement_distance_meters = 3.000\n"));
  ok = ok && expect(absent.status == iggy3d::ScenarioLoadStatus::Ok, "no-profile scenario parses") &&
       expect(absent.seed.movementProfile.id == "earth_standard",
              "absent movement_profile defaults to earth_standard");

  // Omit movement_distance_meters + name a profile -> gets the profile's limit.
  const iggy3d::ScenarioLoadResult fromProfile =
      iggy3d::parseScenarioText(scenario("movement_profile = \"earth_standard\"\n"));
  ok = ok && expect(fromProfile.status == iggy3d::ScenarioLoadStatus::Ok,
                    "profile-only scenario parses (distance now optional)") &&
       expect(fromProfile.seed.config.movementDistanceMeters == 3.0F,
              "omitted distance takes the profile's limit");

  // Explicit distance OVERRIDES the profile's field.
  const iggy3d::ScenarioLoadResult override =
      iggy3d::parseScenarioText(scenario("movement_profile = \"earth_standard\"\n"
                                         "movement_distance_meters = 2.500\n"));
  ok = ok && expect(override.status == iggy3d::ScenarioLoadStatus::Ok, "override scenario parses") &&
       expect(override.seed.config.movementDistanceMeters == 2.5F,
              "an explicit movement_distance_meters overrides the profile");
  return ok;
}

}  // namespace

int main() {
  const bool ok =
      earthMapsByteIdenticalToAppTuning() && byIdLookup() && loaderResolvesAndPrecedence();
  return ok ? 0 : 1;
}
