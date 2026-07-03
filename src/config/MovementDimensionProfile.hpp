#pragma once

#include <cstddef>
#include <string_view>

namespace iggy3d {

// Movement-abilities dimension profile (MA1, M&A map v1.1): a RUNTIME/CONFIG-SIDE MIRROR of the
// app's ProductGameplayMovementTuning (its 26 numeric fields + 3 profile-name ids) PLUS the runtime
// admission limit movementDistanceMeters. LAYERING WALL (non-negotiable): nothing at/below
// src/config may include the app tuning header, so this mirror DUPLICATES the literals; the app maps
// mirror -> ProductGameplayMovementTuning at window-tuning init (a test asserts earth_standard maps
// BYTE-IDENTICAL to the app constant -- the anti-drift guard). One profile per scenario selects the
// world's movement feel as DATA: home worlds earth-like; the giant liminal dimension lighter-gravity
// / tuned-up. MovementParams values + slope-band spans are RESERVED future rows (not stored yet).
struct MovementDimensionProfile {
  std::string_view id = "earth_standard";  // selection key + saved id (static-literal backed)
  float movementDistanceMeters = 3.000F;   // the row's canonical admission limit

  // The 26 ProductGameplayMovementTuning numeric mirrors (SAME field names; earth_standard values).
  float walkSpeedMetersPerSecond = 3.3F;
  float sprintSpeedMetersPerSecond = 6.2F;
  float groundAccelerationMetersPerSecondSquared = 400.0F;
  float groundDecelerationMetersPerSecondSquared = 400.0F;
  float airControlMultiplier = 1.0F;
  float inputStepSeconds = 1.0F / 60.0F;
  float jumpImpulseMetersPerSecond = 15.8F;
  float gravityMetersPerSecondSquared = 18.0F;
  float coyoteTimeSeconds = 0.10F;
  float jumpBufferSeconds = 0.10F;
  float jumpCutMultiplier = 0.50F;
  float fallGravityMultiplier = 1.60F;
  float lookSensitivity = 1.0F;
  float invertLookEnabled = 0.0F;
  float dashSpeedMetersPerSecond = 16.6F;  // trimmed (16.6 x 0.18 = 2.988 m <= movementDistance 3.0)
  float dashDurationSeconds = 0.18F;
  float dashCooldownSeconds = 0.45F;
  float wallRunMinSpeedMetersPerSecond = 2.0F;
  float wallRunMaxWallNormalY = 0.25F;
  float wallRunDurationSeconds = 0.75F;
  float wallRunGravityMultiplier = 0.25F;
  float wallRunSpeedMultiplier = 1.0F;
  float wallJumpProbeMeters = 0.58F;
  float wallJumpPushMeters = 1.20F;
  float wallJumpRiseMeters = 0.45F;
  float wallJumpMinAirborneHeightMeters = 0.20F;

  // The 3 tuning-name ids.
  std::string_view walkProfile = "manual_first_person";
  std::string_view sprintProfile = "manual_first_person_sprint";
  std::string_view dashProfile = "manual_first_person_dash";

  // MA1 s2 sneak-stance knobs (per-dimension, EXTRA — NOT part of the 26-field app-tuning mirror,
  // so applyMovementDimensionProfileToTuning does NOT touch them). The speed half scales Move deltas;
  // the loudness half scales emitted footstep dB. Both in (0, 1]; loudness never below the floor
  // (sneak DECLARES noise, never silent — the sound law).
  float sneakSpeedMultiplier = 0.5F;
  float sneakLoudnessMultiplier = 0.35F;
};

// The sound-law floor: a sneak footstep is quieter but NEVER silent (loudness multiplier >= this).
inline constexpr float kMinSneakLoudnessMultiplier = 0.05F;

// Fail-closed lookup: the static row for `id`, or nullptr (caller maps null -> seed error).
const MovementDimensionProfile* movementDimensionProfileById(std::string_view id);

// M-LAB s1 (config function #1 of the two named cockpit additions): read-only enumeration over the
// static rows so the tuning cockpit can cycle dimensions without knowing their ids. byIndex returns
// nullptr out of range. NOTHING else about the row data moves.
std::size_t movementDimensionProfileCount();
const MovementDimensionProfile* movementDimensionProfileByIndex(std::size_t index);

// M-LAB s1 (config function #2): parse a `[movement_dimension_profile]` TOML row (the cockpit export
// schema) into `out`. Every mirror field is required; any missing/malformed key ⇒ false (fail-closed).
// Floats round-trip losslessly (std::from_chars). NOTE: `out`'s string_view fields (id + the 3 profile
// names) reference `tomlRow` -- the caller MUST keep that buffer alive for `out`'s lifetime. This is
// the round-trip's read half AND the reserved seam for a future file-backed / data-driven profile row.
bool parseMovementDimensionProfileRow(std::string_view tomlRow, MovementDimensionProfile& out);

// THE DASH COHERENCE VALIDATION (MA1): a resolved config is coherent iff a dash cannot travel
// farther than the admission limit -- dashSpeed x dashDuration <= resolvedMovementDistanceMeters (+
// finite guards). The dash>admission bug (a dash reported accepted then admission-rejected) dies
// here + at the reorder in Controller.
bool isCoherentMovementProfile(const MovementDimensionProfile& profile,
                               float resolvedMovementDistanceMeters);

}  // namespace iggy3d
