#include "app/iggy3d/gameplay/MovementTuningCockpit.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <system_error>

#include "app/iggy3d/gameplay/MovementTuning.hpp"

namespace iggy3d {
namespace {

// App-side lossless float text, mirroring runtime/save's formatFloatLossless (to_chars shortest form
// round-trips exactly through from_chars). Kept local so the cockpit does not reach into the save lane.
std::string formatFloatLossless(float value) {
  std::array<char, 32> buffer{};
  const std::to_chars_result result =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  return std::string(buffer.data(), result.ptr);
}

void appendFloat(std::string& out, std::string_view key, float value) {
  out += key;
  out += " = ";
  out += formatFloatLossless(value);
  out += '\n';
}

void appendString(std::string& out, std::string_view key, std::string_view value) {
  out += key;
  out += " = \"";
  out += value;
  out += "\"\n";
}

// Next free export index under <saveRoot>/movement_profiles/, mirroring nextProductWorldId: scan the
// existing cockpit_export_<n> files, return max+1 (0 when the dir is absent/empty). Collision-safe,
// deterministic, no wall-clock.
std::uint64_t nextCockpitExportIndex(const std::filesystem::path& exportDir) {
  constexpr std::string_view kPrefix = "cockpit_export_";
  constexpr std::string_view kSuffix = ".movementprofile.toml";
  std::uint64_t maxIndex = 0;
  bool any = false;
  std::error_code ec;
  if (!std::filesystem::exists(exportDir, ec)) {
    return 0;
  }
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(exportDir, ec)) {
    if (ec) {
      break;
    }
    const std::string name = entry.path().filename().string();
    if (name.size() <= kPrefix.size() + kSuffix.size() ||
        std::string_view(name).substr(0, kPrefix.size()) != kPrefix ||
        std::string_view(name).substr(name.size() - kSuffix.size()) != kSuffix) {
      continue;
    }
    const std::string_view digits =
        std::string_view(name).substr(kPrefix.size(), name.size() - kPrefix.size() - kSuffix.size());
    std::uint64_t value = 0;
    const std::from_chars_result parsed =
        std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (parsed.ec == std::errc{} && parsed.ptr == digits.data() + digits.size()) {
      maxIndex = any ? std::max(maxIndex, value) : value;
      any = true;
    }
  }
  return any ? maxIndex + 1U : 0U;
}

}  // namespace

std::string serializeMovementDimensionProfileRow(const MovementDimensionProfile& row) {
  std::string out = "[movement_dimension_profile]\n";
  appendString(out, "id", row.id);
  appendFloat(out, "movement_distance_meters", row.movementDistanceMeters);
  appendFloat(out, "walk_speed_meters_per_second", row.walkSpeedMetersPerSecond);
  appendFloat(out, "sprint_speed_meters_per_second", row.sprintSpeedMetersPerSecond);
  appendFloat(out, "ground_acceleration_meters_per_second_squared",
              row.groundAccelerationMetersPerSecondSquared);
  appendFloat(out, "ground_deceleration_meters_per_second_squared",
              row.groundDecelerationMetersPerSecondSquared);
  appendFloat(out, "air_control_multiplier", row.airControlMultiplier);
  appendFloat(out, "input_step_seconds", row.inputStepSeconds);
  appendFloat(out, "jump_impulse_meters_per_second", row.jumpImpulseMetersPerSecond);
  appendFloat(out, "gravity_meters_per_second_squared", row.gravityMetersPerSecondSquared);
  appendFloat(out, "coyote_time_seconds", row.coyoteTimeSeconds);
  appendFloat(out, "jump_buffer_seconds", row.jumpBufferSeconds);
  appendFloat(out, "jump_cut_multiplier", row.jumpCutMultiplier);
  appendFloat(out, "fall_gravity_multiplier", row.fallGravityMultiplier);
  appendFloat(out, "look_sensitivity", row.lookSensitivity);
  appendFloat(out, "invert_look_enabled", row.invertLookEnabled);
  appendFloat(out, "dash_speed_meters_per_second", row.dashSpeedMetersPerSecond);
  appendFloat(out, "dash_duration_seconds", row.dashDurationSeconds);
  appendFloat(out, "dash_cooldown_seconds", row.dashCooldownSeconds);
  appendFloat(out, "wall_run_min_speed_meters_per_second", row.wallRunMinSpeedMetersPerSecond);
  appendFloat(out, "wall_run_max_wall_normal_y", row.wallRunMaxWallNormalY);
  appendFloat(out, "wall_run_duration_seconds", row.wallRunDurationSeconds);
  appendFloat(out, "wall_run_gravity_multiplier", row.wallRunGravityMultiplier);
  appendFloat(out, "wall_run_speed_multiplier", row.wallRunSpeedMultiplier);
  appendFloat(out, "wall_jump_probe_meters", row.wallJumpProbeMeters);
  appendFloat(out, "wall_jump_push_meters", row.wallJumpPushMeters);
  appendFloat(out, "wall_jump_rise_meters", row.wallJumpRiseMeters);
  appendFloat(out, "wall_jump_min_airborne_height_meters", row.wallJumpMinAirborneHeightMeters);
  appendFloat(out, "sneak_speed_multiplier", row.sneakSpeedMultiplier);
  appendFloat(out, "sneak_loudness_multiplier", row.sneakLoudnessMultiplier);
  appendString(out, "walk_profile", row.walkProfile);
  appendString(out, "sprint_profile", row.sprintProfile);
  appendString(out, "dash_profile", row.dashProfile);
  return out;
}

MovementTuningSwapResult applyMovementTuningProfileSwap(ProductAppWindowState& window,
                                                        float sessionAdmissionLimitMeters) {
  MovementTuningSwapResult result;
  const std::size_t count = movementDimensionProfileCount();
  if (count == 0) {
    window.gameplayMovementTuningStatus = "movement_tuning_profile_swap_unavailable";
    window.gameplayMovementTuningReasonCode = window.gameplayMovementTuningStatus;
    return result;
  }
  // Find the current row's index by the last-applied id (default earth ⇒ index 0), then advance.
  std::size_t currentIndex = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const MovementDimensionProfile* row = movementDimensionProfileByIndex(i);
    if (row != nullptr && row->id == window.gameplayMovementTuningProfileId) {
      currentIndex = i;
      break;
    }
  }
  const std::size_t nextIndex = (currentIndex + 1U) % count;
  const MovementDimensionProfile* nextRow = movementDimensionProfileByIndex(nextIndex);
  if (nextRow == nullptr) {
    window.gameplayMovementTuningStatus = "movement_tuning_profile_swap_unavailable";
    window.gameplayMovementTuningReasonCode = window.gameplayMovementTuningStatus;
    return result;
  }

  applyMovementDimensionProfileToTuning(*nextRow, window.gameplayMovementTuning);
  window.gameplayMovementTuningProfileId = std::string(nextRow->id);
  result.applied = true;
  result.appliedProfileId = window.gameplayMovementTuningProfileId;
  // The swap changes APP tuning only; the hashed session admission limit stays. Flag the honest gap.
  result.admissionMismatch = nextRow->movementDistanceMeters != sessionAdmissionLimitMeters;
  window.gameplayMovementTuningStatus =
      "movement_tuning_profile_applied:" + result.appliedProfileId;
  window.gameplayMovementTuningReasonCode =
      result.admissionMismatch ? "admission_limit_unswapped"
                               : window.gameplayMovementTuningStatus;
  return result;
}

MovementTuningExportResult exportMovementTuningProfile(ProductAppWindowState& window,
                                                       const std::filesystem::path& saveRoot) {
  MovementTuningExportResult result;
  const MovementDimensionProfile* lastAppliedRow =
      movementDimensionProfileById(window.gameplayMovementTuningProfileId);
  if (lastAppliedRow == nullptr) {
    lastAppliedRow = movementDimensionProfileByIndex(0);  // fail-safe to earth_standard
  }

  const std::filesystem::path exportDir = saveRoot / "movement_profiles";
  const std::uint64_t index = nextCockpitExportIndex(exportDir);
  const std::string exportId = "cockpit_export_" + std::to_string(index);
  const MovementDimensionProfile assembled = movementDimensionProfileFromCockpit(
      window.gameplayMovementTuning, *lastAppliedRow, exportId);

  // Fail closed: the cockpit refuses to export a row MA1's own validator would reject.
  if (!isCoherentMovementProfile(assembled, assembled.movementDistanceMeters)) {
    result.reasonCode = "movement_tuning_export_rejected:incoherent";
    window.gameplayMovementTuningStatus = result.reasonCode;
    window.gameplayMovementTuningReasonCode = result.reasonCode;
    return result;  // write NOTHING
  }

  std::error_code ec;
  std::filesystem::create_directories(exportDir, ec);
  const std::filesystem::path path = exportDir / (exportId + ".movementprofile.toml");
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    result.reasonCode = "movement_tuning_export_failed:io";
    window.gameplayMovementTuningStatus = result.reasonCode;
    window.gameplayMovementTuningReasonCode = result.reasonCode;
    return result;
  }
  file << serializeMovementDimensionProfileRow(assembled);
  file.close();

  result.ok = true;
  result.exportId = exportId;
  result.path = path;
  result.reasonCode = "movement_tuning_exported:" + exportId;
  window.gameplayMovementTuningStatus = result.reasonCode;
  window.gameplayMovementTuningReasonCode = result.reasonCode;
  return result;
}

}  // namespace iggy3d
