#include "app/iggy3d/creative/input/Catalog.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <iterator>
#include <string>
#include <utility>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d::creative {
namespace {

struct CatalogToolSpec {
  CreativeHeldItemKind kind = CreativeHeldItemKind::ObjectSelect;
  std::string_view label;
  std::string_view aliases;
  bool toolWheelEligible = true;
};

struct CatalogShapePreset {
  CreativeCatalogShapeSelection selection{};
  std::string_view label;
};

constexpr std::array<CatalogShapePreset, kCreativeCatalogShapePresetCount>
    kShapePresets{{
        {{CreativeShapeBrushKind::Box, CreativeShapeBrushAxis::Y}, "Box"},
        {{CreativeShapeBrushKind::Line, CreativeShapeBrushAxis::Y}, "Line"},
        {{CreativeShapeBrushKind::Ellipsoid, CreativeShapeBrushAxis::Y},
         "Ellipsoid"},
        {{CreativeShapeBrushKind::Cylinder, CreativeShapeBrushAxis::X},
         "Cylinder X"},
        {{CreativeShapeBrushKind::Cylinder, CreativeShapeBrushAxis::Y},
         "Cylinder Y"},
        {{CreativeShapeBrushKind::Cylinder, CreativeShapeBrushAxis::Z},
         "Cylinder Z"},
    }};

constexpr std::array kToolSpecs{
    CatalogToolSpec{CreativeHeldItemKind::MaterialBrush, "Brush",
                    "paint sculpt material sphere cube cylinder"},
    CatalogToolSpec{CreativeHeldItemKind::ObjectSelect, "Object Select",
                    "select pick object", false},
    CatalogToolSpec{CreativeHeldItemKind::ObjectMove, "Transform",
                    "object move drag translate rotate mirror copy"},
    CatalogToolSpec{CreativeHeldItemKind::VolumeSelect, "Region Select",
                    "volume region wand corner selection", false},
    CatalogToolSpec{CreativeHeldItemKind::VolumeFill, "Fill",
                    "volume region solid create"},
    CatalogToolSpec{CreativeHeldItemKind::VolumeHollow, "Hollow",
                    "volume region shell walls"},
    CatalogToolSpec{CreativeHeldItemKind::VolumeReplace, "Replace",
                    "volume region swap material"},
    CatalogToolSpec{CreativeHeldItemKind::VolumeErase, "Erase",
                    "volume region remove delete", false},
    CatalogToolSpec{CreativeHeldItemKind::VolumeClone, "Clone",
                    "volume region copy duplicate"},
    CatalogToolSpec{CreativeHeldItemKind::ConnectedFill, "Connected Fill",
                    "flood paint bucket connected material region"},
    CatalogToolSpec{CreativeHeldItemKind::SurfaceExtrude, "Surface Extrude",
                    "extrude inset pull push connected face surface"},
    CatalogToolSpec{CreativeHeldItemKind::LinearArray, "Array",
                    "array pattern repeat duplicate line radial ring"},
    CatalogToolSpec{CreativeHeldItemKind::TerrainControl, "Terrain Rod",
                    "terrain heightfield rod elevation radius landscape ground",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainGrade, "Terrain Grade",
                    "terrain grade ramp slope hill smooth landscape ground",
                    false},
};

constexpr std::array kActionEntries{
    CreativeCatalogActionEntry{CreativeInputActionId::Undo, "Undo"},
    CreativeCatalogActionEntry{CreativeInputActionId::Redo, "Redo"},
    CreativeCatalogActionEntry{CreativeInputActionId::CopySelection, "Copy"},
    CreativeCatalogActionEntry{CreativeInputActionId::CutSelection, "Cut"},
    CreativeCatalogActionEntry{CreativeInputActionId::PasteClipboard, "Paste"},
    CreativeCatalogActionEntry{CreativeInputActionId::DuplicateSelection,
                               "Duplicate"},
    CreativeCatalogActionEntry{CreativeInputActionId::Save, "Save"},
    CreativeCatalogActionEntry{CreativeInputActionId::NewDocument, "New", true},
    CreativeCatalogActionEntry{CreativeInputActionId::Load, "Load", true},
};

static_assert(kActionEntries.size() == kCreativeCatalogActionCapacity);

[[nodiscard]] char asciiLower(char value) noexcept {
  return static_cast<char>(
      std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] std::string lowerAscii(std::string_view value) {
  std::string output;
  output.reserve(value.size());
  std::transform(value.begin(), value.end(), std::back_inserter(output),
                 asciiLower);
  return output;
}

[[nodiscard]] std::string sanitizedQuery(std::string_view value) {
  std::string output;
  output.reserve(std::min(value.size(), kCreativeCatalogQueryCapacity));
  for (char character : value) {
    const unsigned char byte = static_cast<unsigned char>(character);
    if (byte >= 32U && byte <= 126U) {
      output.push_back(character);
      if (output.size() == kCreativeCatalogQueryCapacity) {
        break;
      }
    }
  }
  return output;
}

[[nodiscard]] std::optional<std::size_t> selectedEntryIndex(
    const CreativeCatalogState& catalog) noexcept {
  if (catalog.selectedFilteredIndex >= catalog.filteredEntryIndices.size()) {
    return std::nullopt;
  }
  return catalog.filteredEntryIndices[catalog.selectedFilteredIndex];
}

void refreshFilter(CreativeCatalogState& catalog) {
  const std::optional<std::size_t> previous = selectedEntryIndex(catalog);
  const std::string query = lowerAscii(catalog.query);
  catalog.filteredEntryIndices.clear();
  catalog.filteredEntryIndices.reserve(catalog.entries.size());
  for (std::size_t index = 0; index < catalog.entries.size(); ++index) {
    if (query.empty() ||
        catalog.entries[index].searchText.find(query) != std::string::npos) {
      catalog.filteredEntryIndices.push_back(index);
    }
  }

  catalog.selectedFilteredIndex = 0;
  if (!previous.has_value()) {
    return;
  }
  const auto found = std::find(catalog.filteredEntryIndices.begin(),
                               catalog.filteredEntryIndices.end(), *previous);
  if (found != catalog.filteredEntryIndices.end()) {
    catalog.selectedFilteredIndex = static_cast<std::size_t>(
        std::distance(catalog.filteredEntryIndices.begin(), found));
  }
}

[[nodiscard]] std::string descriptorLabel(
    const CreativeObjectDescriptor& descriptor) {
  if (!descriptor.displayName.empty()) {
    return std::string(descriptor.displayName);
  }
  if (!descriptor.name.empty()) {
    return std::string(descriptor.name);
  }
  return std::string(toString(descriptor.kind));
}

[[nodiscard]] std::string descriptorSearchText(
    const CreativeObjectDescriptor& descriptor,
    std::string_view label) {
  std::string text;
  text.reserve(label.size() + descriptor.name.size() +
               descriptor.purpose.size() + 32U);
  text.append(label);
  text.push_back(' ');
  text.append(descriptor.name);
  text.push_back(' ');
  text.append(toString(descriptor.category));
  text.push_back(' ');
  text.append(descriptor.purpose);
  return lowerAscii(text);
}

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
                       hotbar.selectedSlot != targetSlot;
  hotbar.entries[targetSlot] = entry;
  hotbar.selectedSlot = static_cast<std::uint8_t>(targetSlot);
  return changed;
}

[[nodiscard]] bool sameShapeSelection(
    CreativeCatalogShapeSelection lhs,
    CreativeCatalogShapeSelection rhs) noexcept {
  return lhs.kind == rhs.kind && lhs.axis == rhs.axis;
}

[[nodiscard]] std::size_t shapePresetIndex(
    CreativeCatalogShapeSelection selection) noexcept {
  const auto found = std::find_if(
      kShapePresets.begin(), kShapePresets.end(),
      [selection](const CatalogShapePreset& preset) {
        if (preset.selection.kind != selection.kind) {
          return false;
        }
        return selection.kind != CreativeShapeBrushKind::Cylinder ||
               preset.selection.axis == selection.axis;
      });
  return found == kShapePresets.end()
             ? 0U
             : static_cast<std::size_t>(
                   std::distance(kShapePresets.begin(), found));
}

}  // namespace

std::string_view toString(CreativeCatalogEntryCategory category) noexcept {
  switch (category) {
    case CreativeCatalogEntryCategory::Material: return "Material";
    case CreativeCatalogEntryCategory::Tool: return "Tool";
  }
  return "Unknown";
}

std::string_view toString(CreativeCatalogPage page) noexcept {
  switch (page) {
    case CreativeCatalogPage::Build: return "Build";
    case CreativeCatalogPage::Actions: return "Actions";
    case CreativeCatalogPage::Count: break;
  }
  return "Unknown";
}

CreativeCatalogState makeCreativeCatalog(
    std::span<const CreativeObjectKind> materialPalette) {
  CreativeCatalogState catalog;
  catalog.entries.reserve(materialPalette.size() + kToolSpecs.size());

  CreativeObjectKind defaultMaterial = CreativeObjectKind::Unknown;
  for (CreativeObjectKind kind : materialPalette) {
    if (kind == CreativeObjectKind::Unknown ||
        std::any_of(catalog.entries.begin(), catalog.entries.end(),
                    [kind](const CreativeCatalogEntry& entry) {
                      return entry.category ==
                                 CreativeCatalogEntryCategory::Material &&
                             entry.hotbarEntry.objectKind == kind;
                    })) {
      continue;
    }
    const CreativeObjectDescriptor& descriptor = describeObject(kind);
    if (!descriptorShowsInAuthoringBrushPalette(descriptor)) {
      continue;
    }
    if (defaultMaterial == CreativeObjectKind::Unknown) {
      defaultMaterial = kind;
    }
    CreativeCatalogEntry entry;
    entry.category = CreativeCatalogEntryCategory::Material;
    entry.hotbarEntry = {CreativeHeldItemKind::Material, kind};
    entry.label = descriptorLabel(descriptor);
    entry.searchText = descriptorSearchText(descriptor, entry.label);
    catalog.entries.push_back(std::move(entry));
  }

  for (const CatalogToolSpec& spec : kToolSpecs) {
    if (creativeHeldItemUsesMaterial(spec.kind) &&
        defaultMaterial == CreativeObjectKind::Unknown) {
      continue;
    }
    CreativeCatalogEntry entry;
    entry.category = CreativeCatalogEntryCategory::Tool;
    entry.hotbarEntry.kind = spec.kind;
    entry.toolWheelEligible = spec.toolWheelEligible;
    entry.hotbarEntry.objectKind =
        creativeHeldItemUsesMaterial(spec.kind)
            ? defaultMaterial
            : CreativeObjectKind::Unknown;
    entry.label = spec.label;
    entry.searchText = lowerAscii(std::string(spec.label) + " " +
                                  std::string(spec.aliases));
    catalog.entries.push_back(std::move(entry));
  }
  refreshFilter(catalog);
  return catalog;
}

std::span<const CreativeCatalogActionEntry>
creativeCatalogActionEntries() noexcept {
  return kActionEntries;
}

bool setCreativeCatalogPage(CreativeCatalogState& catalog,
                            CreativeCatalogPage page) noexcept {
  if (page == CreativeCatalogPage::Count || catalog.page == page) {
    return false;
  }
  catalog.page = page;
  catalog.pendingActionConfirmation.reset();
  return true;
}

bool moveCreativeCatalogPage(CreativeCatalogState& catalog,
                             std::int32_t steps) noexcept {
  if (steps == 0) {
    return false;
  }
  const CreativeWrappedIndexResult next = stepCreativeWrappedIndex(
      static_cast<std::size_t>(catalog.page),
      static_cast<std::size_t>(CreativeCatalogPage::Count), steps);
  if (!next.valid) {
    return false;
  }
  return setCreativeCatalogPage(
      catalog, static_cast<CreativeCatalogPage>(next.index));
}

bool moveCreativeCatalogActionSelection(CreativeCatalogState& catalog,
                                        std::int32_t steps) noexcept {
  if (steps == 0 || kActionEntries.empty()) {
    return false;
  }
  const CreativeWrappedIndexResult next = stepCreativeWrappedIndex(
      catalog.selectedActionIndex, kActionEntries.size(), steps);
  if (!next.valid || !next.changed) {
    return false;
  }
  catalog.selectedActionIndex = next.index;
  catalog.pendingActionConfirmation.reset();
  return true;
}

bool selectCreativeCatalogActionIndex(CreativeCatalogState& catalog,
                                      std::size_t actionIndex) noexcept {
  if (actionIndex >= kActionEntries.size() ||
      catalog.selectedActionIndex == actionIndex) {
    return false;
  }
  catalog.selectedActionIndex = actionIndex;
  catalog.pendingActionConfirmation.reset();
  return true;
}

const CreativeCatalogActionEntry* selectedCreativeCatalogAction(
    const CreativeCatalogState& catalog) noexcept {
  return catalog.selectedActionIndex < kActionEntries.size()
             ? &kActionEntries[catalog.selectedActionIndex]
             : nullptr;
}

CreativeCatalogActionActivation activateSelectedCreativeCatalogAction(
    CreativeCatalogState& catalog) noexcept {
  CreativeCatalogActionActivation result;
  const CreativeCatalogActionEntry* selected =
      selectedCreativeCatalogAction(catalog);
  if (selected == nullptr) {
    return result;
  }
  result.action = selected->action;
  if (selected->confirmationRequired &&
      catalog.pendingActionConfirmation != selected->action) {
    catalog.pendingActionConfirmation = selected->action;
    result.confirmationArmed = true;
    return result;
  }
  catalog.pendingActionConfirmation.reset();
  result.requested = true;
  return result;
}

bool creativeCatalogActionAvailable(
    CreativeInputActionId action,
    const CreativeCatalogActionAvailability& availability) noexcept {
  switch (action) {
    case CreativeInputActionId::Undo: return availability.undoAvailable;
    case CreativeInputActionId::Redo: return availability.redoAvailable;
    case CreativeInputActionId::CopySelection:
    case CreativeInputActionId::CutSelection:
    case CreativeInputActionId::DuplicateSelection:
      return availability.selectionAvailable;
    case CreativeInputActionId::PasteClipboard:
      return availability.clipboardAvailable;
    case CreativeInputActionId::Save: return availability.saveAvailable;
    case CreativeInputActionId::NewDocument: return availability.newAvailable;
    case CreativeInputActionId::Load: return availability.loadAvailable;
    default: return false;
  }
}

bool setCreativeCatalogOpen(CreativeCatalogState& catalog,
                            bool open) noexcept {
  if (catalog.open == open) {
    return false;
  }
  catalog.open = open;
  if (!open) {
    catalog.pendingActionConfirmation.reset();
  }
  return true;
}

bool setCreativeCatalogQuery(CreativeCatalogState& catalog,
                             std::string_view query) {
  std::string sanitized = sanitizedQuery(query);
  if (catalog.query == sanitized) {
    return false;
  }
  catalog.query = std::move(sanitized);
  refreshFilter(catalog);
  return true;
}

bool appendCreativeCatalogQueryText(CreativeCatalogState& catalog,
                                    std::string_view text) {
  std::string next = catalog.query;
  next.append(text);
  return setCreativeCatalogQuery(catalog, next);
}

bool eraseCreativeCatalogQuery(CreativeCatalogState& catalog,
                               std::size_t count) {
  if (count == 0U || catalog.query.empty()) {
    return false;
  }
  const std::size_t eraseCount = std::min(count, catalog.query.size());
  catalog.query.erase(catalog.query.size() - eraseCount);
  refreshFilter(catalog);
  return true;
}

bool moveCreativeCatalogSelection(CreativeCatalogState& catalog,
                                  std::int32_t steps) noexcept {
  if (steps == 0 || catalog.filteredEntryIndices.empty()) {
    return false;
  }
  const CreativeWrappedIndexResult next = stepCreativeWrappedIndex(
      catalog.selectedFilteredIndex, catalog.filteredEntryIndices.size(),
      steps);
  if (!next.valid) {
    return false;
  }
  catalog.selectedFilteredIndex = next.index;
  return next.changed;
}

bool selectCreativeCatalogFilteredIndex(CreativeCatalogState& catalog,
                                        std::size_t filteredIndex) noexcept {
  if (filteredIndex >= catalog.filteredEntryIndices.size() ||
      catalog.selectedFilteredIndex == filteredIndex) {
    return false;
  }
  catalog.selectedFilteredIndex = filteredIndex;
  return true;
}

const CreativeCatalogEntry* selectedCreativeCatalogEntry(
    const CreativeCatalogState& catalog) noexcept {
  return creativeCatalogEntryAtFilteredIndex(catalog,
                                             catalog.selectedFilteredIndex);
}

std::optional<std::size_t> selectedCreativeCatalogEntryIndex(
    const CreativeCatalogState& catalog) noexcept {
  if (catalog.selectedFilteredIndex >= catalog.filteredEntryIndices.size()) {
    return std::nullopt;
  }
  const std::size_t entryIndex =
      catalog.filteredEntryIndices[catalog.selectedFilteredIndex];
  return entryIndex < catalog.entries.size()
             ? std::optional<std::size_t>{entryIndex}
             : std::nullopt;
}

const CreativeCatalogEntry* creativeCatalogEntryAtFilteredIndex(
    const CreativeCatalogState& catalog,
    std::size_t filteredIndex) noexcept {
  if (filteredIndex >= catalog.filteredEntryIndices.size()) {
    return nullptr;
  }
  const std::size_t entryIndex = catalog.filteredEntryIndices[filteredIndex];
  return entryIndex < catalog.entries.size() ? &catalog.entries[entryIndex]
                                             : nullptr;
}

CreativeHotbarEntry resolveCreativeCatalogHotbarEntry(
    const CreativeCatalogEntry& entry,
    CreativeObjectKind activeMaterial) noexcept {
  CreativeHotbarEntry resolved = entry.hotbarEntry;
  if (entry.category == CreativeCatalogEntryCategory::Tool) {
    static_cast<void>(
        applyCreativeHeldItemMaterial(resolved, activeMaterial));
  }
  return resolved;
}

bool creativeCatalogEntryUsesShapeSelection(
    const CreativeCatalogEntry& entry) noexcept {
  return entry.category == CreativeCatalogEntryCategory::Tool &&
         creativeHeldItemUsesDirectShapeGesture(entry.hotbarEntry.kind);
}

CreativeCatalogShapeSelection normalizeCreativeCatalogShapeSelection(
    CreativeShapeBrushKind kind,
    CreativeShapeBrushAxis axis) noexcept {
  const CreativeCatalogShapeSelection requested{kind, axis};
  return kShapePresets[shapePresetIndex(requested)].selection;
}

bool moveCreativeCatalogShapeSelection(
    CreativeCatalogShapeSelection& selection,
    std::int32_t steps) noexcept {
  if (steps == 0) {
    return false;
  }
  const CreativeWrappedIndexResult next = stepCreativeWrappedIndex(
      shapePresetIndex(selection), kShapePresets.size(), steps);
  if (!next.valid) {
    return false;
  }
  const CreativeCatalogShapeSelection nextSelection =
      kShapePresets[next.index].selection;
  const bool changed = !sameShapeSelection(selection, nextSelection);
  selection = nextSelection;
  return changed;
}

std::string_view creativeCatalogShapeSelectionLabel(
    CreativeCatalogShapeSelection selection) noexcept {
  return kShapePresets[shapePresetIndex(selection)].label;
}

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
