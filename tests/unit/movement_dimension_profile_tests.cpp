// ma1s1 — the movement dimension profile mirror + loader precedence + coherence + save id.
// GATE 1: earth_standard maps to the app tuning constant (the anti-drift guard).
// GATE 2: giant_lowgrav row, dash-coherence validation (fail-closed), and the saved profile id
// (conditional wire key + carry across load).

#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "config/MovementDimensionProfile.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "runtime/save/SaveEnvelope.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool hasDiagnosticCode(const iggy3d::ScenarioLoadResult& result, std::string_view code) {
  for (const iggy3d::Diagnostic& diagnostic : result.diagnostics) {
    if (diagnostic.code == code) {
      return true;
    }
  }
  return false;
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

  // Explicit distance OVERRIDES the profile's field (coherent value: 16.6 x 0.18 = 2.988 <= 3.500).
  const iggy3d::ScenarioLoadResult override =
      iggy3d::parseScenarioText(scenario("movement_profile = \"earth_standard\"\n"
                                         "movement_distance_meters = 3.500\n"));
  ok = ok && expect(override.status == iggy3d::ScenarioLoadStatus::Ok, "override scenario parses") &&
       expect(override.seed.config.movementDistanceMeters == 3.5F,
              "an explicit movement_distance_meters overrides the profile");
  return ok;
}

// GATE 2 --------------------------------------------------------------------------------------

bool coherenceValidator() {
  const iggy3d::MovementDimensionProfile* earth =
      iggy3d::movementDimensionProfileById("earth_standard");
  const iggy3d::MovementDimensionProfile* giant =
      iggy3d::movementDimensionProfileById("giant_lowgrav");
  if (!expect(earth != nullptr && giant != nullptr, "both rows exist")) {
    return false;
  }
  // earth dash 16.6 x 0.18 = 2.988 <= 3.0 coherent; <= 2.5 incoherent.
  return expect(iggy3d::isCoherentMovementProfile(*earth, 3.0F), "earth coherent at 3.0") &&
         expect(!iggy3d::isCoherentMovementProfile(*earth, 2.5F), "earth incoherent at 2.5") &&
         // giant dash 22.0 x 0.18 = 3.96 <= 4.5 coherent.
         expect(iggy3d::isCoherentMovementProfile(*giant, 4.5F), "giant coherent at 4.5") &&
         expect(!iggy3d::isCoherentMovementProfile(*giant, 3.0F), "giant incoherent at 3.0");
}

bool loaderGate2() {
  // Unknown profile id -> fail-closed.
  const iggy3d::ScenarioLoadResult unknown =
      iggy3d::parseScenarioText(scenario("movement_profile = \"no_such_world\"\n"
                                         "movement_distance_meters = 3.000\n"));
  bool ok = expect(unknown.status != iggy3d::ScenarioLoadStatus::Ok,
                   "unknown movement_profile is rejected") &&
            expect(hasDiagnosticCode(unknown, "scenario.unknown_movement_profile"),
                   "unknown movement_profile diagnostic code");

  // Incoherent explicit distance (16.6 x 0.18 = 2.988 > 2.000) -> fail-closed.
  const iggy3d::ScenarioLoadResult incoherent =
      iggy3d::parseScenarioText(scenario("movement_profile = \"earth_standard\"\n"
                                         "movement_distance_meters = 2.000\n"));
  ok = ok && expect(incoherent.status != iggy3d::ScenarioLoadStatus::Ok,
                    "incoherent movement profile is rejected") &&
       expect(hasDiagnosticCode(incoherent, "scenario.incoherent_movement_profile"),
              "incoherent movement profile diagnostic code");

  // giant_lowgrav resolves: id + its own limit + a mirrored world field.
  const iggy3d::ScenarioLoadResult giant =
      iggy3d::parseScenarioText(scenario("movement_profile = \"giant_lowgrav\"\n"));
  ok = ok && expect(giant.status == iggy3d::ScenarioLoadStatus::Ok, "giant scenario parses") &&
       expect(giant.seed.movementProfile.id == "giant_lowgrav", "giant profile resolved") &&
       expect(giant.seed.config.movementDistanceMeters == 4.5F, "giant limit taken from row") &&
       expect(giant.seed.movementProfile.gravityMetersPerSecondSquared == 11.0F,
              "giant gravity mirror carried on the seed");
  return ok;
}

// Build a session from a scenario TOML (fail hard in the test if it doesn't parse/create).
std::optional<iggy3d::Session> sessionFromScenario(const std::string& extraDefaults) {
  const iggy3d::ScenarioLoadResult loaded = iggy3d::parseScenarioText(scenario(extraDefaults));
  if (loaded.status != iggy3d::ScenarioLoadStatus::Ok) {
    return std::nullopt;
  }
  iggy3d::SessionCreateRequest request;
  request.config = loaded.seed.config;
  request.seed = loaded.seed;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  if (created.status != iggy3d::ResultStatus::Ok) {
    return std::nullopt;
  }
  return std::move(created.value);
}

bool sessionCreateRejectsIncoherentProfile() {
  // Bypass the loader's own guard: hand a giant profile with an under-sized admission limit.
  const iggy3d::ScenarioLoadResult loaded =
      iggy3d::parseScenarioText(scenario("movement_profile = \"giant_lowgrav\"\n"));
  if (!expect(loaded.status == iggy3d::ScenarioLoadStatus::Ok, "seed parses for create guard")) {
    return false;
  }
  iggy3d::SessionCreateRequest request;
  request.seed = loaded.seed;
  request.config = loaded.seed.config;
  request.config.movementDistanceMeters = 1.000F;  // giant dash 3.96 > 1.0 -> incoherent
  const iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  return expect(created.status != iggy3d::ResultStatus::Ok,
                "Session::create rejects an incoherent movement profile") &&
         expect(created.error.code == "session.movement_profile_incoherent",
                "create rejection code");
}

bool saveRoundTripCarriesProfileId() {
  std::optional<iggy3d::Session> giant = sessionFromScenario("movement_profile = \"giant_lowgrav\"\n");
  std::optional<iggy3d::Session> earth = sessionFromScenario("movement_profile = \"earth_standard\"\n");
  if (!expect(giant.has_value() && earth.has_value(), "giant + earth sessions created")) {
    return false;
  }

  // giant: the id is written on the wire, and decode reconstructs it.
  const iggy3d::SaveStateResult giantSaved = iggy3d::saveSessionStateEncoded(giant->state());
  bool ok = expect(giantSaved.status == iggy3d::SaveLoadStatus::Ok, "giant save ok") &&
            expect(giantSaved.encodedSaveText.find("session.movementProfileId=giant_lowgrav") !=
                       std::string::npos,
                   "giant profile id written to the wire");
  const iggy3d::SaveDecodeResult giantDecoded =
      iggy3d::decodeSaveEnvelope(giantSaved.encodedSaveText);
  ok = ok && expect(giantDecoded.status == iggy3d::SaveCodecStatus::Ok, "giant decode ok") &&
       expect(giantDecoded.envelope.session.movementProfileId == "giant_lowgrav",
              "giant profile id decoded");

  // earth (the default) omits the key entirely -> byte-identical to pre-MA1 saves.
  const iggy3d::SaveStateResult earthSaved = iggy3d::saveSessionStateEncoded(earth->state());
  ok = ok && expect(earthSaved.status == iggy3d::SaveLoadStatus::Ok, "earth save ok") &&
       expect(earthSaved.encodedSaveText.find("movementProfileId") == std::string::npos,
              "earth omits the movementProfileId key");

  // Round-trip into a fresh giant session: numerics survive load (carried across replaceStateFromLoad).
  std::optional<iggy3d::Session> destination =
      sessionFromScenario("movement_profile = \"giant_lowgrav\"\n");
  if (!expect(destination.has_value(), "giant destination session created")) {
    return false;
  }
  const iggy3d::SaveCompatibilityRequest compat{giantSaved.envelope,
                                                giant->state().identity.packageId,
                                                giant->state().identity.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(*destination, giantSaved.encodedSaveText, compat);
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::Ok, "giant load ok") &&
       expect(destination->state().movementProfile.id == "giant_lowgrav",
              "loaded session retains giant profile") &&
       expect(destination->state().movementProfile.gravityMetersPerSecondSquared == 11.0F,
              "loaded giant numerics carried (not stored on the wire)");
  return ok;
}

}  // namespace

int main() {
  const bool ok = earthMapsByteIdenticalToAppTuning() && byIdLookup() &&
                  loaderResolvesAndPrecedence() && coherenceValidator() && loaderGate2() &&
                  sessionCreateRejectsIncoherentProfile() && saveRoundTripCarriesProfileId();
  return ok ? 0 : 1;
}
