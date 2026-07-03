#include "config/MovementDimensionProfile.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <string_view>

namespace iggy3d {
namespace {

// The giant liminal dimension: lighter gravity, and dash AND movement limit raised TOGETHER (coherent
// -- 22.0 x 0.18 = 3.96 <= 4.5). Reviewer's coherent first values, A10-tunable; the POINT is the row
// exists + validates. Starts from earth defaults, then overrides the world's feel.
constexpr MovementDimensionProfile makeGiantLowgrav() {
  MovementDimensionProfile profile;
  profile.id = "giant_lowgrav";
  profile.movementDistanceMeters = 4.500F;
  profile.gravityMetersPerSecondSquared = 11.0F;
  profile.jumpImpulseMetersPerSecond = 19.5F;
  profile.wallRunDurationSeconds = 1.1F;
  profile.dashSpeedMetersPerSecond = 22.0F;
  // sneakSpeedMultiplier / sneakLoudnessMultiplier keep earth's defaults (0.5 / 0.35) -- the giant
  // world may retune its own sneak feel later; matching earth is a valid first value.
  return profile;
}

// The dimension rows (stable order): earth_standard (today's values, dash trimmed) + giant_lowgrav.
inline constexpr std::array<MovementDimensionProfile, 2> kMovementDimensionProfiles{{
    MovementDimensionProfile{},  // earth_standard (id defaults to "earth_standard")
    makeGiantLowgrav(),
}};

// M-LAB s1 export schema: snake_case key -> the mirror struct's float member. ONE source of truth for
// the parser; the app-side writer emits the same keys (the round-trip test catches any drift). Covers
// movement_distance_meters + the 26 numerics + the 2 sneak multipliers (id + the 3 profile names are
// string_view fields, handled separately). This table is private (anon ns) -- not exported surface.
struct FloatFieldKey {
  std::string_view key;
  float MovementDimensionProfile::* member;
};

constexpr std::array<FloatFieldKey, 29U> kProfileFloatFields{{
    {"movement_distance_meters", &MovementDimensionProfile::movementDistanceMeters},
    {"walk_speed_meters_per_second", &MovementDimensionProfile::walkSpeedMetersPerSecond},
    {"sprint_speed_meters_per_second", &MovementDimensionProfile::sprintSpeedMetersPerSecond},
    {"ground_acceleration_meters_per_second_squared",
     &MovementDimensionProfile::groundAccelerationMetersPerSecondSquared},
    {"ground_deceleration_meters_per_second_squared",
     &MovementDimensionProfile::groundDecelerationMetersPerSecondSquared},
    {"air_control_multiplier", &MovementDimensionProfile::airControlMultiplier},
    {"input_step_seconds", &MovementDimensionProfile::inputStepSeconds},
    {"jump_impulse_meters_per_second", &MovementDimensionProfile::jumpImpulseMetersPerSecond},
    {"gravity_meters_per_second_squared", &MovementDimensionProfile::gravityMetersPerSecondSquared},
    {"coyote_time_seconds", &MovementDimensionProfile::coyoteTimeSeconds},
    {"jump_buffer_seconds", &MovementDimensionProfile::jumpBufferSeconds},
    {"jump_cut_multiplier", &MovementDimensionProfile::jumpCutMultiplier},
    {"fall_gravity_multiplier", &MovementDimensionProfile::fallGravityMultiplier},
    {"look_sensitivity", &MovementDimensionProfile::lookSensitivity},
    {"invert_look_enabled", &MovementDimensionProfile::invertLookEnabled},
    {"dash_speed_meters_per_second", &MovementDimensionProfile::dashSpeedMetersPerSecond},
    {"dash_duration_seconds", &MovementDimensionProfile::dashDurationSeconds},
    {"dash_cooldown_seconds", &MovementDimensionProfile::dashCooldownSeconds},
    {"wall_run_min_speed_meters_per_second", &MovementDimensionProfile::wallRunMinSpeedMetersPerSecond},
    {"wall_run_max_wall_normal_y", &MovementDimensionProfile::wallRunMaxWallNormalY},
    {"wall_run_duration_seconds", &MovementDimensionProfile::wallRunDurationSeconds},
    {"wall_run_gravity_multiplier", &MovementDimensionProfile::wallRunGravityMultiplier},
    {"wall_run_speed_multiplier", &MovementDimensionProfile::wallRunSpeedMultiplier},
    {"wall_jump_probe_meters", &MovementDimensionProfile::wallJumpProbeMeters},
    {"wall_jump_push_meters", &MovementDimensionProfile::wallJumpPushMeters},
    {"wall_jump_rise_meters", &MovementDimensionProfile::wallJumpRiseMeters},
    {"wall_jump_min_airborne_height_meters",
     &MovementDimensionProfile::wallJumpMinAirborneHeightMeters},
    {"sneak_speed_multiplier", &MovementDimensionProfile::sneakSpeedMultiplier},
    {"sneak_loudness_multiplier", &MovementDimensionProfile::sneakLoudnessMultiplier},
}};

// Trim ASCII spaces/tabs/CR from both ends.
std::string_view trim(std::string_view text) {
  const std::size_t begin = text.find_first_not_of(" \t\r");
  if (begin == std::string_view::npos) {
    return {};
  }
  const std::size_t end = text.find_last_not_of(" \t\r");
  return text.substr(begin, end - begin + 1U);
}

// Line-scan for `key = value`; the value token (trimmed) goes to `valueOut`. The char after the key
// must be whitespace or '=' so no key can be a prefix of another (e.g. `air_control_multiplier`).
bool lineValue(std::string_view toml, std::string_view key, std::string_view& valueOut) {
  std::size_t pos = 0;
  while (pos <= toml.size()) {
    const std::size_t eol = toml.find('\n', pos);
    const std::string_view raw =
        toml.substr(pos, eol == std::string_view::npos ? std::string_view::npos : eol - pos);
    const std::string_view line = trim(raw);
    if (line.size() > key.size() && line.substr(0, key.size()) == key) {
      const std::string_view rest = trim(line.substr(key.size()));
      if (!rest.empty() && rest.front() == '=') {
        valueOut = trim(rest.substr(1));
        return true;
      }
    }
    if (eol == std::string_view::npos) {
      break;
    }
    pos = eol + 1U;
  }
  return false;
}

bool parseFloatField(std::string_view toml, std::string_view key, float& out) {
  std::string_view value;
  if (!lineValue(toml, key, value) || value.empty()) {
    return false;
  }
  const char* begin = value.data();
  const char* end = value.data() + value.size();
  const std::from_chars_result result = std::from_chars(begin, end, out);
  return result.ec == std::errc{} && result.ptr == end;  // whole token consumed, lossless
}

// A quoted string value -> the substring VIEW into `toml` (no allocation; caller keeps `toml` alive).
bool parseStringField(std::string_view toml, std::string_view key, std::string_view& out) {
  std::string_view value;
  if (!lineValue(toml, key, value) || value.size() < 2U ||
      value.front() != '"' || value.back() != '"') {
    return false;
  }
  out = value.substr(1U, value.size() - 2U);
  return true;
}

}  // namespace

const MovementDimensionProfile* movementDimensionProfileById(std::string_view id) {
  for (const MovementDimensionProfile& profile : kMovementDimensionProfiles) {
    if (profile.id == id) {
      return &profile;
    }
  }
  return nullptr;
}

std::size_t movementDimensionProfileCount() {
  return kMovementDimensionProfiles.size();
}

const MovementDimensionProfile* movementDimensionProfileByIndex(std::size_t index) {
  if (index >= kMovementDimensionProfiles.size()) {
    return nullptr;
  }
  return &kMovementDimensionProfiles[index];
}

bool parseMovementDimensionProfileRow(std::string_view tomlRow, MovementDimensionProfile& out) {
  MovementDimensionProfile parsed;  // start from defaults, then require EVERY key (fail-closed)
  for (const FloatFieldKey& field : kProfileFloatFields) {
    if (!parseFloatField(tomlRow, field.key, parsed.*(field.member))) {
      return false;
    }
  }
  if (!parseStringField(tomlRow, "id", parsed.id) ||
      !parseStringField(tomlRow, "walk_profile", parsed.walkProfile) ||
      !parseStringField(tomlRow, "sprint_profile", parsed.sprintProfile) ||
      !parseStringField(tomlRow, "dash_profile", parsed.dashProfile)) {
    return false;
  }
  out = parsed;
  return true;
}

bool isCoherentMovementProfile(const MovementDimensionProfile& profile,
                               float resolvedMovementDistanceMeters) {
  if (!std::isfinite(profile.dashSpeedMetersPerSecond) ||
      !std::isfinite(profile.dashDurationSeconds) ||
      !std::isfinite(resolvedMovementDistanceMeters)) {
    return false;
  }
  // MA1 s2: sneak multipliers slow + quiet, never stop or silence. Both in (0, 1]; loudness above the
  // floor (fail-closed -- a silent sneak would break the sound law the stealth sim depends on).
  if (!std::isfinite(profile.sneakSpeedMultiplier) ||
      !std::isfinite(profile.sneakLoudnessMultiplier) ||
      profile.sneakSpeedMultiplier <= 0.0F || profile.sneakSpeedMultiplier > 1.0F ||
      profile.sneakLoudnessMultiplier > 1.0F ||
      profile.sneakLoudnessMultiplier < kMinSneakLoudnessMultiplier) {
    return false;
  }
  return profile.dashSpeedMetersPerSecond * profile.dashDurationSeconds <=
         resolvedMovementDistanceMeters;
}

}  // namespace iggy3d
