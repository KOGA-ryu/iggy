#include "runtime/inventory/InventorySystem.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::InventoryState inventoryWithPlayerZero() {
  iggy3d::InventoryState inventory;
  inventory.players.push_back({0, {}});
  return inventory;
}

bool addItemAppendsAndIncrementsStack() {
  iggy3d::InventoryState inventory = inventoryWithPlayerZero();
  const iggy3d::InventoryOperationResult first =
      iggy3d::addItem(inventory, {0, "gold_key", 1});
  const iggy3d::InventoryOperationResult second =
      iggy3d::addItem(inventory, {0, "gold_key", 2});
  return expect(first.status == iggy3d::InventoryStatus::Ok && first.finalCount == 1U,
                "first add") &&
         expect(second.status == iggy3d::InventoryStatus::Ok && second.finalCount == 3U,
                "second add") &&
         expect(inventory.players[0].stacks.size() == 1U, "stack count") &&
         expect(inventory.players[0].stacks[0].count == 3U, "final count") &&
         expect(iggy3d::hasItem(inventory, 0, "gold_key", 3), "has item");
}

bool rejectsInvalidAddRequestsWithoutMutation() {
  iggy3d::InventoryState inventory = inventoryWithPlayerZero();
  bool ok = expect(iggy3d::addItem(inventory, {0, "", 1}).status ==
                       iggy3d::InventoryStatus::InvalidItemId,
                   "empty item") &&
            expect(iggy3d::addItem(inventory, {0, "gold_key", 0}).status ==
                       iggy3d::InventoryStatus::InvalidCount,
                   "zero count") &&
            expect(iggy3d::addItem(inventory, {9, "gold_key", 1}).status ==
                       iggy3d::InventoryStatus::InvalidPlayerSlot,
                   "missing slot");
  return ok && expect(inventory.players[0].stacks.empty(), "invalid add no mutation");
}

bool removeItemSubtractsAndErasesZeroStack() {
  iggy3d::InventoryState inventory = inventoryWithPlayerZero();
  (void)iggy3d::addItem(inventory, {0, "gold_key", 3});
  const iggy3d::InventoryOperationResult partial =
      iggy3d::removeItem(inventory, {0, "gold_key", 2});
  const iggy3d::InventoryOperationResult final =
      iggy3d::removeItem(inventory, {0, "gold_key", 1});
  return expect(partial.status == iggy3d::InventoryStatus::Ok && partial.finalCount == 1U,
                "partial remove") &&
         expect(final.status == iggy3d::InventoryStatus::Ok && final.finalCount == 0U,
                "final remove") &&
         expect(inventory.players[0].stacks.empty(), "stack erased");
}

bool removeFailuresAndZeroHasItemAreStructured() {
  iggy3d::InventoryState inventory = inventoryWithPlayerZero();
  (void)iggy3d::addItem(inventory, {0, "gold_key", 1});
  bool ok = expect(iggy3d::removeItem(inventory, {0, "missing", 1}).status ==
                       iggy3d::InventoryStatus::MissingItem,
                   "missing remove") &&
            expect(iggy3d::removeItem(inventory, {0, "gold_key", 2}).status ==
                       iggy3d::InventoryStatus::InsufficientCount,
                   "insufficient remove") &&
            expect(!iggy3d::hasItem(inventory, 0, "gold_key", 0), "zero has item false");
  return ok && expect(inventory.players[0].stacks[0].count == 1U, "failed remove unchanged");
}

}  // namespace

int main() {
  const bool ok = addItemAppendsAndIncrementsStack() && rejectsInvalidAddRequestsWithoutMutation() &&
                  removeItemSubtractsAndErasesZeroStack() &&
                  removeFailuresAndZeroHasItemAreStructured();
  return ok ? 0 : 1;
}
