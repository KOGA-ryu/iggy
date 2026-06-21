#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"

namespace iggy3d {

using PlayerSlotId = std::uint32_t;

inline constexpr PlayerSlotId kInvalidPlayerSlotId = static_cast<PlayerSlotId>(UINT32_MAX);

enum class PlayerSlotKind : std::uint8_t {
  Unknown,
  Local,
  Remote,
  Ai,
  Observer,
};

struct PlayerSlot {
  PlayerSlotId id = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
  EntityId actor;
  std::string stableName;
};

inline bool isValidPlayerSlotId(PlayerSlotId id) {
  return id != kInvalidPlayerSlotId;
}

inline bool isPlayableSlotKind(PlayerSlotKind kind) {
  return kind == PlayerSlotKind::Local || kind == PlayerSlotKind::Remote ||
         kind == PlayerSlotKind::Ai;
}

inline bool isActorControllingSlotKind(PlayerSlotKind kind) {
  return kind == PlayerSlotKind::Local || kind == PlayerSlotKind::Remote ||
         kind == PlayerSlotKind::Ai;
}

}  // namespace iggy3d
