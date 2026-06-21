#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

struct InventoryStack {
  std::string itemId;
  std::uint32_t count = 0;
};

struct PlayerInventory {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::vector<InventoryStack> stacks;
};

struct InventoryState {
  std::vector<PlayerInventory> players;
};

}  // namespace iggy3d
