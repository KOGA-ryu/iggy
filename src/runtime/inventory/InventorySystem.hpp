#pragma once

#include <cstdint>
#include <string>

#include "runtime/inventory/InventoryState.hpp"
#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

enum class InventoryStatus : std::uint8_t {
  Ok,
  InvalidState,
  InvalidPlayerSlot,
  InvalidItemId,
  InvalidCount,
  MissingItem,
  InsufficientCount,
};

struct InventoryItemRequest {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t count = 0;
};

struct InventoryOperationResult {
  InventoryStatus status = InventoryStatus::InvalidState;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t requestedCount = 0;
  std::uint32_t finalCount = 0;
  bool mutated = false;
};

InventoryOperationResult addItem(InventoryState& state, const InventoryItemRequest& request);
InventoryOperationResult removeItem(InventoryState& state, const InventoryItemRequest& request);
bool hasItem(
    const InventoryState& state,
    PlayerSlotId playerSlot,
    const std::string& itemId,
    std::uint32_t count);
const PlayerInventory* findInventory(const InventoryState& state, PlayerSlotId playerSlot);

}  // namespace iggy3d
