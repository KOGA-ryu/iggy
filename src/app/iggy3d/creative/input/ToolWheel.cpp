#include "app/iggy3d/creative/input/Catalog.hpp"

#include <algorithm>
#include <utility>

#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool assignHotbarEntry(
    CreativeHotbarEntry entry,
    CreativeHotbarState& hotbar,
    std::size_t targetSlot) noexcept {
  if (targetSlot >= kCreativeHotbarSlotCount) {
    return false;
  }
  const CreativeHotbarEntry before = hotbar.entries[targetSlot];
  const bool changed = before.kind != entry.kind ||
                       before.objectKind != entry.objectKind ||
                       before.assetId != entry.assetId ||
                       before.hasAssetBounds != entry.hasAssetBounds ||
                       !creativeBoundsExactlyEqual(before.assetSourceBounds,
                                                   entry.assetSourceBounds) ||
                       hotbar.selectedSlot != targetSlot;
  hotbar.entries[targetSlot] = entry;
  hotbar.selectedSlot = static_cast<std::uint8_t>(targetSlot);
  return changed;
}

}  // namespace

bool assignSelectedCreativeCatalogEntry(
    const CreativeCatalogState& catalog,
    CreativeHotbarState& hotbar,
    std::optional<std::size_t> slot) noexcept {
  const CreativeCatalogEntry* entry = selectedCreativeCatalogEntry(catalog);
  const std::size_t targetSlot =
      slot.value_or(static_cast<std::size_t>(hotbar.selectedSlot));
  if (entry == nullptr) {
    return false;
  }
  return assignHotbarEntry(entry->hotbarEntry, hotbar, targetSlot);
}

CreativeToolWheelState makeCreativeToolWheel(
    const CreativeCatalogState& catalog) noexcept {
  CreativeToolWheelState wheel;
  for (std::size_t index = 0; index < catalog.entries.size(); ++index) {
    if (catalog.entries[index].category != CreativeCatalogEntryCategory::Tool ||
        !catalog.entries[index].toolWheelEligible) {
      continue;
    }
    if (wheel.entryCount == kCreativeToolWheelCapacity) {
      wheel.capacityExceeded = true;
      continue;
    }
    wheel.catalogEntryIndices[wheel.entryCount++] = index;
  }
  return wheel;
}

bool setCreativeToolWheelOpen(CreativeToolWheelState& wheel,
                              bool open) noexcept {
  if (wheel.open == open) {
    return false;
  }
  wheel.open = open;
  return true;
}

bool selectCreativeToolWheelForHotbarEntry(
    CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog,
    CreativeHotbarEntry entry) noexcept {
  for (std::size_t index = 0; index < wheel.entryCount; ++index) {
    const std::size_t catalogIndex = wheel.catalogEntryIndices[index];
    if (catalogIndex < catalog.entries.size() &&
        catalog.entries[catalogIndex].hotbarEntry.kind == entry.kind) {
      const bool changed = wheel.selectedIndex != index;
      wheel.selectedIndex = index;
      return changed;
    }
  }
  return false;
}

bool moveCreativeToolWheelSelection(CreativeToolWheelState& wheel,
                                    std::int32_t steps) noexcept {
  if (steps == 0 || wheel.entryCount == 0U) {
    return false;
  }
  const CreativeWrappedIndexResult next =
      stepCreativeWrappedIndex(wheel.selectedIndex, wheel.entryCount, steps);
  if (!next.valid) {
    return false;
  }
  wheel.selectedIndex = next.index;
  return next.changed;
}

bool selectCreativeToolWheelDirection(CreativeToolWheelState& wheel,
                                      float x,
                                      float y,
                                      float deadzone) noexcept {
  const CreativeRadialSectorResult sector =
      resolveCreativeRadialSector(x, y, wheel.entryCount, deadzone);
  if (!sector.valid) {
    return false;
  }
  const bool changed = wheel.selectedIndex != sector.index;
  wheel.selectedIndex = sector.index;
  return changed;
}

const CreativeCatalogEntry* selectedCreativeToolWheelEntry(
    const CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog) noexcept {
  if (wheel.selectedIndex >= wheel.entryCount) {
    return nullptr;
  }
  const std::size_t catalogIndex =
      wheel.catalogEntryIndices[wheel.selectedIndex];
  return catalogIndex < catalog.entries.size() ? &catalog.entries[catalogIndex]
                                               : nullptr;
}

std::optional<std::size_t> creativeToolWheelSectorForCatalogEntry(
    const CreativeToolWheelState& wheel,
    std::size_t catalogEntryIndex) noexcept {
  for (std::size_t index = 0; index < wheel.entryCount; ++index) {
    if (wheel.catalogEntryIndices[index] == catalogEntryIndex) {
      return index;
    }
  }
  return std::nullopt;
}

bool assignCreativeToolWheelCatalogEntry(
    CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog,
    std::size_t sectorIndex,
    std::size_t catalogEntryIndex) noexcept {
  if (sectorIndex >= wheel.entryCount ||
      catalogEntryIndex >= catalog.entries.size() ||
      catalog.entries[catalogEntryIndex].category !=
          CreativeCatalogEntryCategory::Tool) {
    return false;
  }
  const std::optional<std::size_t> existing =
      creativeToolWheelSectorForCatalogEntry(wheel, catalogEntryIndex);
  if (existing == sectorIndex) {
    return false;
  }
  if (existing.has_value()) {
    std::swap(wheel.catalogEntryIndices[sectorIndex],
              wheel.catalogEntryIndices[*existing]);
  } else {
    wheel.catalogEntryIndices[sectorIndex] = catalogEntryIndex;
  }
  wheel.selectedIndex = sectorIndex;
  return true;
}

bool resetCreativeToolWheel(CreativeToolWheelState& wheel,
                            const CreativeCatalogState& catalog) noexcept {
  const CreativeToolWheelState defaults = makeCreativeToolWheel(catalog);
  const bool changed = wheel.entryCount != defaults.entryCount ||
                       wheel.capacityExceeded != defaults.capacityExceeded ||
                       !std::equal(wheel.catalogEntryIndices.begin(),
                                   wheel.catalogEntryIndices.end(),
                                   defaults.catalogEntryIndices.begin());
  wheel = defaults;
  return changed;
}

bool isValidCreativeToolWheel(const CreativeToolWheelState& wheel,
                              const CreativeCatalogState& catalog) noexcept {
  if (wheel.entryCount > wheel.catalogEntryIndices.size() ||
      (wheel.entryCount == 0U ? wheel.selectedIndex != 0U
                              : wheel.selectedIndex >= wheel.entryCount)) {
    return false;
  }
  for (std::size_t index = 0; index < wheel.entryCount; ++index) {
    const std::size_t catalogIndex = wheel.catalogEntryIndices[index];
    if (catalogIndex >= catalog.entries.size() ||
        catalog.entries[catalogIndex].category !=
            CreativeCatalogEntryCategory::Tool) {
      return false;
    }
    for (std::size_t previous = 0; previous < index; ++previous) {
      if (wheel.catalogEntryIndices[previous] == catalogIndex) {
        return false;
      }
    }
  }
  return true;
}

bool assignSelectedCreativeToolWheelEntry(
    const CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog,
    CreativeHotbarState& hotbar) noexcept {
  const CreativeCatalogEntry* entry =
      selectedCreativeToolWheelEntry(wheel, catalog);
  if (entry == nullptr) {
    return false;
  }
  return assignHotbarEntry(entry->hotbarEntry, hotbar,
                           static_cast<std::size_t>(hotbar.selectedSlot));
}

}  // namespace iggy3d::creative
