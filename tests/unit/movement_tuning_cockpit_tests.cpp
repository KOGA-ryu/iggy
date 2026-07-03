// M-LAB s1 GATE 2 -- the tuning cockpit: dimension enumeration, hot-swap (+ honest admission marker),
// and lossless export/round-trip through the commissioned config parser. These drive the REAL handler
// functions (the action path the input layer calls) with explicit inputs.

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/MovementTuningCockpit.hpp"
#include "config/MovementDimensionProfile.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

// Field-by-field equality of the two mirror rows' numerics + string ids (byte-exact).
bool rowsEqual(const iggy3d::MovementDimensionProfile& a,
               const iggy3d::MovementDimensionProfile& b) {
  return a.id == b.id && a.walkProfile == b.walkProfile && a.sprintProfile == b.sprintProfile &&
         a.dashProfile == b.dashProfile && a.movementDistanceMeters == b.movementDistanceMeters &&
         a.walkSpeedMetersPerSecond == b.walkSpeedMetersPerSecond &&
         a.sprintSpeedMetersPerSecond == b.sprintSpeedMetersPerSecond &&
         a.groundAccelerationMetersPerSecondSquared == b.groundAccelerationMetersPerSecondSquared &&
         a.groundDecelerationMetersPerSecondSquared == b.groundDecelerationMetersPerSecondSquared &&
         a.airControlMultiplier == b.airControlMultiplier &&
         a.inputStepSeconds == b.inputStepSeconds &&
         a.jumpImpulseMetersPerSecond == b.jumpImpulseMetersPerSecond &&
         a.gravityMetersPerSecondSquared == b.gravityMetersPerSecondSquared &&
         a.coyoteTimeSeconds == b.coyoteTimeSeconds && a.jumpBufferSeconds == b.jumpBufferSeconds &&
         a.jumpCutMultiplier == b.jumpCutMultiplier &&
         a.fallGravityMultiplier == b.fallGravityMultiplier &&
         a.lookSensitivity == b.lookSensitivity && a.invertLookEnabled == b.invertLookEnabled &&
         a.dashSpeedMetersPerSecond == b.dashSpeedMetersPerSecond &&
         a.dashDurationSeconds == b.dashDurationSeconds &&
         a.dashCooldownSeconds == b.dashCooldownSeconds &&
         a.wallRunMinSpeedMetersPerSecond == b.wallRunMinSpeedMetersPerSecond &&
         a.wallRunMaxWallNormalY == b.wallRunMaxWallNormalY &&
         a.wallRunDurationSeconds == b.wallRunDurationSeconds &&
         a.wallRunGravityMultiplier == b.wallRunGravityMultiplier &&
         a.wallRunSpeedMultiplier == b.wallRunSpeedMultiplier &&
         a.wallJumpProbeMeters == b.wallJumpProbeMeters &&
         a.wallJumpPushMeters == b.wallJumpPushMeters &&
         a.wallJumpRiseMeters == b.wallJumpRiseMeters &&
         a.wallJumpMinAirborneHeightMeters == b.wallJumpMinAirborneHeightMeters &&
         a.sneakSpeedMultiplier == b.sneakSpeedMultiplier &&
         a.sneakLoudnessMultiplier == b.sneakLoudnessMultiplier;
}

// The window tuning equals the row's app-tuning mapping, field for field (reuses the anti-drift idea).
bool tuningMatchesRowMapping(const iggy3d::ProductGameplayMovementTuning& tuning,
                             const iggy3d::MovementDimensionProfile& row) {
  iggy3d::ProductGameplayMovementTuning expected;
  iggy3d::applyMovementDimensionProfileToTuning(row, expected);
  return tuning.walkSpeedMetersPerSecond == expected.walkSpeedMetersPerSecond &&
         tuning.jumpImpulseMetersPerSecond == expected.jumpImpulseMetersPerSecond &&
         tuning.gravityMetersPerSecondSquared == expected.gravityMetersPerSecondSquared &&
         tuning.dashSpeedMetersPerSecond == expected.dashSpeedMetersPerSecond &&
         tuning.wallRunDurationSeconds == expected.wallRunDurationSeconds &&
         tuning.wallJumpProbeMeters == expected.wallJumpProbeMeters;
}

bool configEnumeratorEnumeratesRows() {
  const bool ok =
      expect(iggy3d::movementDimensionProfileCount() == 2U, "two dimension rows enumerated") &&
      expect(iggy3d::movementDimensionProfileByIndex(0) != nullptr &&
                 iggy3d::movementDimensionProfileByIndex(0)->id == "earth_standard",
             "index 0 is earth_standard") &&
      expect(iggy3d::movementDimensionProfileByIndex(1) != nullptr &&
                 iggy3d::movementDimensionProfileByIndex(1)->id == "giant_lowgrav",
             "index 1 is giant_lowgrav") &&
      expect(iggy3d::movementDimensionProfileByIndex(2) == nullptr,
             "out-of-range index is nullptr (fail-closed)");
  return ok;
}

bool serializeParseRoundTripsLosslessly() {
  // Assemble a TUNED row from the cockpit (a scribbled tuning + earth's runtime fields).
  iggy3d::ProductGameplayMovementTuning tuning;
  tuning.jumpImpulseMetersPerSecond = 17.37F;  // an awkward value to stress shortest-form floats
  tuning.dashSpeedMetersPerSecond = 15.125F;
  tuning.wallJumpPushMeters = 1.234567F;
  const iggy3d::MovementDimensionProfile& earth = *iggy3d::movementDimensionProfileByIndex(0);
  const iggy3d::MovementDimensionProfile assembled =
      iggy3d::movementDimensionProfileFromCockpit(tuning, earth, "cockpit_export_7");

  const std::string toml = iggy3d::serializeMovementDimensionProfileRow(assembled);
  iggy3d::MovementDimensionProfile parsed;
  const bool parsedOk = iggy3d::parseMovementDimensionProfileRow(toml, parsed);
  return expect(parsedOk, "the serialized row parses") &&
         expect(rowsEqual(assembled, parsed), "every field round-trips byte-exact") &&
         expect(parsed.id == "cockpit_export_7", "export id round-trips") &&
         expect(parsed.movementDistanceMeters == earth.movementDistanceMeters,
                "carried admission limit matches the last-applied row") &&
         expect(parsed.sneakLoudnessMultiplier == earth.sneakLoudnessMultiplier,
                "carried sneak loudness matches the last-applied row");
}

bool malformedRowFailsClosed() {
  iggy3d::MovementDimensionProfile parsed;
  // A row missing the dash keys must be rejected (fail-closed), not partially parsed.
  const std::string incomplete = "[movement_dimension_profile]\nid = \"x\"\n";
  return expect(!iggy3d::parseMovementDimensionProfileRow(incomplete, parsed),
                "an incomplete row is rejected");
}

bool hotSwapAppliesRowFlagsMismatchAndKeepsTickStep() {
  iggy3d::ProductAppWindowState window;  // defaults: earth id + earth tuning
  const float earthLimit = 3.0F;         // a session launched on earth_standard

  const iggy3d::MovementTuningSwapResult toGiant =
      iggy3d::applyMovementTuningProfileSwap(window, earthLimit);
  const iggy3d::MovementDimensionProfile& giant = *iggy3d::movementDimensionProfileByIndex(1);
  bool ok = expect(toGiant.applied && toGiant.appliedProfileId == "giant_lowgrav",
                   "swap advances earth -> giant") &&
            expect(tuningMatchesRowMapping(window.gameplayMovementTuning, giant),
                   "window tuning now equals the giant mapping") &&
            expect(window.gameplayMovementTuningStatus == "movement_tuning_profile_applied:giant_lowgrav",
                   "receipt names the applied row") &&
            expect(toGiant.admissionMismatch &&
                       window.gameplayMovementTuningReasonCode == "admission_limit_unswapped",
                   "giant into an earth session flags admission_limit_unswapped");

  const iggy3d::MovementTuningSwapResult toEarth =
      iggy3d::applyMovementTuningProfileSwap(window, earthLimit);
  const iggy3d::MovementDimensionProfile& earth = *iggy3d::movementDimensionProfileByIndex(0);
  ok = ok && expect(toEarth.applied && toEarth.appliedProfileId == "earth_standard",
                    "swap wraps giant -> earth") &&
       expect(tuningMatchesRowMapping(window.gameplayMovementTuning, earth),
              "window tuning back to earth (byte-identical mapping)") &&
       expect(!toEarth.admissionMismatch &&
                  window.gameplayMovementTuningReasonCode ==
                      "movement_tuning_profile_applied:earth_standard",
              "earth into an earth session has NO admission mismatch");

  // The exclusion invariant: inputStepSeconds is never disturbed by a swap (both rows carry 1/60).
  ok = ok && expect(window.gameplayMovementTuning.inputStepSeconds == 1.0F / 60.0F,
                    "inputStepSeconds unchanged across swaps (the named exclusion holds)");
  return ok;
}

std::filesystem::path freshExportRoot() {
  std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_movement_cockpit_tests";
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  std::filesystem::create_directories(root, ec);
  return root;
}

bool exportRejectsIncoherentTuning() {
  const std::filesystem::path root = freshExportRoot();
  iggy3d::ProductAppWindowState window;  // earth defaults (limit 3.0)
  // Tune the dash so it outruns earth's admission limit (40 * 0.18 = 7.2 m > 3.0) -> incoherent.
  window.gameplayMovementTuning.dashSpeedMetersPerSecond = 40.0F;

  const iggy3d::MovementTuningExportResult result =
      iggy3d::exportMovementTuningProfile(window, root);
  std::error_code ec;
  const bool anyWritten = std::filesystem::exists(root / "movement_profiles", ec) &&
                          !std::filesystem::is_empty(root / "movement_profiles", ec);
  return expect(!result.ok, "an incoherent tuned state is not exported") &&
         expect(result.reasonCode == "movement_tuning_export_rejected:incoherent",
                "the rejection names the incoherence") &&
         expect(!anyWritten, "no file is written on rejection");
}

bool exportNumbersFilesAndRoundTripsFromDisk() {
  const std::filesystem::path root = freshExportRoot();
  iggy3d::ProductAppWindowState window;  // earth defaults, coherent
  window.gameplayMovementTuning.jumpImpulseMetersPerSecond = 16.25F;

  const iggy3d::MovementTuningExportResult first =
      iggy3d::exportMovementTuningProfile(window, root);
  const iggy3d::MovementTuningExportResult second =
      iggy3d::exportMovementTuningProfile(window, root);
  bool ok = expect(first.ok && first.exportId == "cockpit_export_0", "first export is _0") &&
            expect(second.ok && second.exportId == "cockpit_export_1", "second export is _1");

  // Read the first file back off disk and round-trip it through the config parser.
  std::ifstream file(first.path, std::ios::binary);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string toml = buffer.str();
  iggy3d::MovementDimensionProfile parsed;
  ok = ok && expect(iggy3d::parseMovementDimensionProfileRow(toml, parsed),
                    "the on-disk export parses") &&
       expect(parsed.id == "cockpit_export_0", "on-disk id round-trips") &&
       expect(parsed.jumpImpulseMetersPerSecond == 16.25F, "the tuned value round-trips off disk");

  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  return ok;
}

}  // namespace

int main() {
  const bool ok = configEnumeratorEnumeratesRows() && serializeParseRoundTripsLosslessly() &&
                  malformedRowFailsClosed() && hotSwapAppliesRowFlagsMismatchAndKeepsTickStep() &&
                  exportRejectsIncoherentTuning() && exportNumbersFilesAndRoundTripsFromDisk();
  return ok ? 0 : 1;
}
