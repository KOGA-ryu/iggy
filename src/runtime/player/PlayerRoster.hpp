#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

enum class PlayerRosterStatus : std::uint8_t {
  Ok,
  InvalidSlotId,
  DuplicateSlotId,
  MissingSlot,
  InvalidSlotKind,
  InvalidActor,
  ActorAlreadyBound,
};

struct PlayerRosterResult {
  PlayerRosterStatus status = PlayerRosterStatus::Ok;
  PlayerSlotId slotId = kInvalidPlayerSlotId;
  std::size_t index = 0;
};

class PlayerRoster {
public:
  const std::vector<PlayerSlot>& slots() const;
  bool empty() const;
  std::size_t size() const;

  PlayerRosterResult addSlot(PlayerSlot slot);

  const PlayerSlot* findSlot(PlayerSlotId slotId) const;
  EntityId actorForSlot(PlayerSlotId slotId) const;
  bool slotControlsActor(PlayerSlotId slotId, EntityId actor) const;

  void clear();

private:
  std::vector<PlayerSlot> slots_;
};

}  // namespace iggy3d
