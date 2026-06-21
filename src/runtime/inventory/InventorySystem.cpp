#include "runtime/inventory/InventorySystem.hpp"

#include <limits>

namespace iggy3d {

namespace {

InventoryOperationResult resultFor(const InventoryItemRequest& request,
                                   InventoryStatus status,
                                   std::uint32_t finalCount = 0,
                                   bool mutated = false) {
  return {status, request.playerSlot, request.itemId, request.count, finalCount, mutated};
}

bool requestShapeValid(const InventoryItemRequest& request, InventoryStatus& status) {
  if (!isValidPlayerSlotId(request.playerSlot)) {
    status = InventoryStatus::InvalidPlayerSlot;
    return false;
  }
  if (request.itemId.empty()) {
    status = InventoryStatus::InvalidItemId;
    return false;
  }
  if (request.count == 0U) {
    status = InventoryStatus::InvalidCount;
    return false;
  }
  return true;
}

PlayerInventory* findMutableInventory(InventoryState& state, PlayerSlotId playerSlot) {
  for (PlayerInventory& inventory : state.players) {
    if (inventory.playerSlot == playerSlot) {
      return &inventory;
    }
  }
  return nullptr;
}

InventoryStack* findMutableStack(PlayerInventory& inventory, const std::string& itemId) {
  for (InventoryStack& stack : inventory.stacks) {
    if (stack.itemId == itemId) {
      return &stack;
    }
  }
  return nullptr;
}

const InventoryStack* findStack(const PlayerInventory& inventory, const std::string& itemId) {
  for (const InventoryStack& stack : inventory.stacks) {
    if (stack.itemId == itemId) {
      return &stack;
    }
  }
  return nullptr;
}

bool inventoryStructureValid(const PlayerInventory& inventory) {
  if (!isValidPlayerSlotId(inventory.playerSlot)) {
    return false;
  }
  for (const InventoryStack& stack : inventory.stacks) {
    if (stack.itemId.empty() || stack.count == 0U) {
      return false;
    }
  }
  return true;
}

}  // namespace

const PlayerInventory* findInventory(const InventoryState& state, PlayerSlotId playerSlot) {
  if (!isValidPlayerSlotId(playerSlot)) {
    return nullptr;
  }
  for (const PlayerInventory& inventory : state.players) {
    if (inventory.playerSlot == playerSlot) {
      return &inventory;
    }
  }
  return nullptr;
}

bool hasItem(
    const InventoryState& state,
    PlayerSlotId playerSlot,
    const std::string& itemId,
    std::uint32_t count) {
  if (!isValidPlayerSlotId(playerSlot) || itemId.empty() || count == 0U) {
    return false;
  }
  const PlayerInventory* inventory = findInventory(state, playerSlot);
  if (inventory == nullptr) {
    return false;
  }
  const InventoryStack* stack = findStack(*inventory, itemId);
  return stack != nullptr && stack->count >= count;
}

InventoryOperationResult addItem(InventoryState& state, const InventoryItemRequest& request) {
  InventoryStatus invalidStatus = InventoryStatus::Ok;
  if (!requestShapeValid(request, invalidStatus)) {
    return resultFor(request, invalidStatus);
  }
  PlayerInventory* inventory = findMutableInventory(state, request.playerSlot);
  if (inventory == nullptr) {
    return resultFor(request, InventoryStatus::InvalidPlayerSlot);
  }
  if (!inventoryStructureValid(*inventory)) {
    return resultFor(request, InventoryStatus::InvalidState);
  }

  InventoryStack* stack = findMutableStack(*inventory, request.itemId);
  if (stack != nullptr) {
    if (request.count > std::numeric_limits<std::uint32_t>::max() - stack->count) {
      return resultFor(request, InventoryStatus::InvalidCount, stack->count);
    }
    stack->count += request.count;
    return resultFor(request, InventoryStatus::Ok, stack->count, true);
  }

  inventory->stacks.push_back({request.itemId, request.count});
  return resultFor(request, InventoryStatus::Ok, request.count, true);
}

InventoryOperationResult removeItem(InventoryState& state, const InventoryItemRequest& request) {
  InventoryStatus invalidStatus = InventoryStatus::Ok;
  if (!requestShapeValid(request, invalidStatus)) {
    return resultFor(request, invalidStatus);
  }
  PlayerInventory* inventory = findMutableInventory(state, request.playerSlot);
  if (inventory == nullptr) {
    return resultFor(request, InventoryStatus::InvalidPlayerSlot);
  }
  if (!inventoryStructureValid(*inventory)) {
    return resultFor(request, InventoryStatus::InvalidState);
  }

  for (auto it = inventory->stacks.begin(); it != inventory->stacks.end(); ++it) {
    if (it->itemId == request.itemId) {
      if (it->count < request.count) {
        return resultFor(request, InventoryStatus::InsufficientCount, it->count);
      }
      const std::uint32_t finalCount = it->count - request.count;
      if (finalCount == 0U) {
        inventory->stacks.erase(it);
      } else {
        it->count = finalCount;
      }
      return resultFor(request, InventoryStatus::Ok, finalCount, true);
    }
  }
  return resultFor(request, InventoryStatus::MissingItem);
}

}  // namespace iggy3d
