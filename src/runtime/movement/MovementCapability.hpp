#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d {

// MA4 (M3): NPC movement capability as DATA. A guard's class decides which reasoning-graph edge kinds
// it may traverse -- a grounded guard walks around the wall a climber routes over. This is a LEAF
// header in runtime/movement (enum + pure string helpers ONLY, no ai include) so `ai -> movement`
// stays the single cross-directory include direction: ma4s2 puts this class on MovementRequest, and
// movement must never include ai. The cost mapping (class -> TravelCostConfig) lives in runtime/ai.
//
// APPEND-ONLY: leaper + flier are RESERVED (declared now for the enum's future, no v1 behavior --
// they map to grounded's config until their own slices land).
enum class MovementCapabilityClass : std::uint8_t {
  grounded,
  climber,
  leaper,  // RESERVED (no v1 behavior)
  flier,   // RESERVED (no v1 behavior)
};

inline constexpr std::string_view movementCapabilityClassName(MovementCapabilityClass capability) {
  switch (capability) {
    case MovementCapabilityClass::grounded:
      return "grounded";
    case MovementCapabilityClass::climber:
      return "climber";
    case MovementCapabilityClass::leaper:
      return "leaper";
    case MovementCapabilityClass::flier:
      return "flier";
  }
  return "grounded";
}

// Fail-closed parse (loader use): an unrecognized string leaves `out` untouched and returns false so
// the caller emits its invalid-enum diagnostic. ABSENT is the caller's concern (it defaults grounded).
inline bool movementCapabilityClassFromString(std::string_view text, MovementCapabilityClass& out) {
  if (text == "grounded") {
    out = MovementCapabilityClass::grounded;
    return true;
  }
  if (text == "climber") {
    out = MovementCapabilityClass::climber;
    return true;
  }
  if (text == "leaper") {
    out = MovementCapabilityClass::leaper;
    return true;
  }
  if (text == "flier") {
    out = MovementCapabilityClass::flier;
    return true;
  }
  return false;
}

}  // namespace iggy3d
