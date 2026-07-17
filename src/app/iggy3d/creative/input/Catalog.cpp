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
    CatalogToolSpec{CreativeHeldItemKind::ObjectGroup, "Group",
                    "group ungroup combine assembly parent selection"},
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
                    "volume region copy duplicate", false},
    CatalogToolSpec{CreativeHeldItemKind::ConnectedFill, "Connected Fill",
                    "flood paint bucket connected material region"},
    CatalogToolSpec{CreativeHeldItemKind::SurfaceExtrude, "Surface Extrude",
                    "extrude inset pull push connected face surface"},
    CatalogToolSpec{CreativeHeldItemKind::LinearArray, "Array",
                    "array pattern repeat duplicate line radial ring"},
    CatalogToolSpec{CreativeHeldItemKind::TerrainControl, "Terrain Rod",
                    "terrain heightfield rod elevation radius landscape ground",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainPaint, "Terrain Paint",
                    "terrain surface material grass dirt stone sand paint",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainGrade, "Terrain Grade",
                    "terrain grade ramp slope hill smooth landscape ground",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainSculpt, "Terrain Sculpt",
                    "terrain sculpt raise lower flatten smooth brush plateau "
                    "landscape",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainProfile, "Terrain Profile",
                    "terrain profile hill basin ring crater ridge wave ripple "
                    "elevation landscape",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainPath, "Terrain Path",
                    "terrain path road river ridge trench embankment route "
                    "landscape",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::TerrainRegion, "Terrain Region",
                    "terrain region area raise lower flatten smooth erase "
                    "landscape selection",
                    false},
    CatalogToolSpec{CreativeHeldItemKind::LogicLink, "Logic Link",
                    "connect wire switch lever button door logic", false},
    CatalogToolSpec{CreativeHeldItemKind::BuildingRoom, "Room",
                    "building floor walls shell footprint architecture",
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

[[nodiscard]] bool containsSearchWord(std::string_view text,
                                      std::string_view word) noexcept {
  if (word.empty() || word.find(' ') != std::string_view::npos) {
    return false;
  }
  std::size_t offset = text.find(word);
  while (offset != std::string_view::npos) {
    const std::size_t end = offset + word.size();
    const bool startsWord = offset == 0U || text[offset - 1U] == ' ';
    const bool endsWord = end == text.size() || text[end] == ' ';
    if (startsWord && endsWord) {
      return true;
    }
    offset = text.find(word, offset + 1U);
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> selectedEntryIndex(
    const CreativeCatalogState& catalog) noexcept {
  if (catalog.selectedFilteredIndex >= catalog.filteredEntryIndices.size()) {
    return std::nullopt;
  }
  return catalog.filteredEntryIndices[catalog.selectedFilteredIndex];
}

[[nodiscard]] bool entryVisibleOnPage(
    const CreativeCatalogEntry& entry,
    CreativeCatalogPage page,
    bool globalCreatorSearch) noexcept {
  return globalCreatorSearch
             ? creativeCatalogPageIsCreatorCategory(entry.page)
             : entry.page == page;
}

[[nodiscard]] CreativeCatalogPage catalogPageForObjectCategory(
    CreativeObjectCategory category) noexcept {
  switch (category) {
    case CreativeObjectCategory::Structural:
      return CreativeCatalogPage::Structure;
    case CreativeObjectCategory::TerrainOrVolume:
      return CreativeCatalogPage::Terrain;
    case CreativeObjectCategory::NavigationOrMovement:
      return CreativeCatalogPage::Movement;
    case CreativeObjectCategory::Logic:
      return CreativeCatalogPage::Logic;
    case CreativeObjectCategory::VisualDressing:
      return CreativeCatalogPage::Dressing;
    case CreativeObjectCategory::LightSoundOrCamera:
      return CreativeCatalogPage::Media;
    case CreativeObjectCategory::Gameplay:
      return CreativeCatalogPage::Gameplay;
    case CreativeObjectCategory::Testing:
      return CreativeCatalogPage::Testing;
    case CreativeObjectCategory::AuthoringMeta:
      return CreativeCatalogPage::Helpers;
    case CreativeObjectCategory::Unknown:
      return CreativeCatalogPage::Count;
  }
  return CreativeCatalogPage::Count;
}

void refreshFilter(CreativeCatalogState& catalog) {
  const std::optional<std::size_t> previous = selectedEntryIndex(catalog);
  const std::string query = lowerAscii(catalog.query);
  const bool globalCreatorSearch =
      !query.empty() && creativeCatalogPageIsCreatorCategory(catalog.page);
  const bool exactWordMode =
      !query.empty() &&
      std::any_of(catalog.entries.begin(), catalog.entries.end(),
                  [&catalog, &query, globalCreatorSearch](
                      const CreativeCatalogEntry& entry) {
                    return entryVisibleOnPage(entry, catalog.page,
                                              globalCreatorSearch) &&
                           containsSearchWord(entry.searchText, query);
                  });
  catalog.filteredEntryIndices.clear();
  catalog.filteredEntryIndices.reserve(catalog.entries.size());
  for (std::size_t index = 0; index < catalog.entries.size(); ++index) {
    if (!entryVisibleOnPage(catalog.entries[index], catalog.page,
                            globalCreatorSearch)) {
      continue;
    }
    const bool matches =
        query.empty() ||
        (exactWordMode
             ? containsSearchWord(catalog.entries[index].searchText, query)
             : catalog.entries[index].searchText.find(query) !=
                   std::string::npos);
    if (matches) {
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

[[nodiscard]] bool buildCatalogAssetEntry(
    const CreativeCatalogAsset& asset,
    CreativeCatalogEntry& entry) {
  if (asset.objectKind == CreativeObjectKind::Unknown ||
      asset.assetId.empty()) {
    return false;
  }
  entry = {};
  entry.page = CreativeCatalogPage::Assets;
  entry.category = CreativeCatalogEntryCategory::Asset;
  entry.hotbarEntry = {CreativeHeldItemKind::Material, asset.objectKind};
  if (!setCreativeHotbarAsset(entry.hotbarEntry, asset.assetId,
                              asset.sourceBounds)) {
    return false;
  }
  entry.label = asset.label.empty() ? asset.assetId : asset.label;
  entry.assetAuthoringMetadata = asset.authoringMetadata;
  entry.authoredComposite = asset.authoredComposite;
  entry.searchText = asset.authoredComposite
                         ? lowerAscii(entry.label + " " + asset.assetId +
                                      " authored reusable composite prefab")
                         : lowerAscii(
                               entry.label + " " + asset.assetId + " " +
                               asset.authoringMetadata.categoryId + " " +
                               std::string(toString(
                                   asset.authoringMetadata.collisionMode)) +
                               " " + std::string(toString(
                                         asset.authoringMetadata.status)) +
                               (asset.authoringMetadata.walkable
                                    ? " walkable"
                                    : "") +
                               " asset imported glb blender mesh");
  return true;
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
    case CreativeCatalogEntryCategory::Asset: return "Asset";
    case CreativeCatalogEntryCategory::AssetFailure: return "Asset Failure";
    case CreativeCatalogEntryCategory::Command: return "Command";
    case CreativeCatalogEntryCategory::Tool: return "Tool";
  }
  return "Unknown";
}

std::string_view toString(CreativeCatalogPage page) noexcept {
  switch (page) {
    case CreativeCatalogPage::Structure: return "Structure";
    case CreativeCatalogPage::Terrain: return "Terrain";
    case CreativeCatalogPage::Movement: return "Movement";
    case CreativeCatalogPage::Logic: return "Logic";
    case CreativeCatalogPage::Dressing: return "Dressing";
    case CreativeCatalogPage::Media: return "Media";
    case CreativeCatalogPage::Gameplay: return "Gameplay";
    case CreativeCatalogPage::Testing: return "Testing";
    case CreativeCatalogPage::Helpers: return "Helpers";
    case CreativeCatalogPage::Tools: return "Tools";
    case CreativeCatalogPage::Assets: return "Assets";
    case CreativeCatalogPage::Actions: return "Actions";
    case CreativeCatalogPage::Count: break;
  }
  return "Unknown";
}

bool creativeCatalogPageIsCreatorCategory(
    CreativeCatalogPage page) noexcept {
  return page >= CreativeCatalogPage::Structure &&
         page <= CreativeCatalogPage::Tools;
}

std::string_view creativeCatalogAssetPhysicsLabel(
    const StaticMeshAuthoringMetadata& metadata) noexcept {
  if (metadata.status == StaticMeshAuthoringMetadataStatus::Invalid ||
      metadata.collisionMode == StaticMeshCollisionMode::Invalid) {
    return "INVALID";
  }
  if (metadata.status ==
          StaticMeshAuthoringMetadataStatus::UnsupportedCollision ||
      metadata.collisionMode == StaticMeshCollisionMode::Convex ||
      metadata.collisionMode == StaticMeshCollisionMode::Mesh) {
    return "UNSUPPORTED";
  }
  if (metadata.collisionMode == StaticMeshCollisionMode::None) {
    return "DECOR";
  }
  if (metadata.collisionMode == StaticMeshCollisionMode::CompoundBounds) {
    return metadata.walkable ? "COMPOUND WALKABLE" : "COMPOUND";
  }
  if (metadata.walkable) {
    return "SOLID WALKABLE";
  }
  return !metadata.collisionSpecified ? "SOLID DEFAULT" : "SOLID";
}

CreativeCatalogState makeCreativeCatalog(
    std::span<const CreativeObjectKind> materialPalette,
    std::span<const CreativeCatalogAsset> assets,
    std::size_t rejectedAssetCount,
    std::span<const CreativeCatalogAssetFailure> assetFailures) {
  CreativeCatalogState catalog;
  catalog.rejectedAssetCount = rejectedAssetCount;
  catalog.entries.reserve(materialPalette.size() + kToolSpecs.size() +
                          assets.size() + assetFailures.size() + 1U);

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
    entry.page = catalogPageForObjectCategory(descriptor.category);
    if (entry.page == CreativeCatalogPage::Count) {
      continue;
    }
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
    entry.page = CreativeCatalogPage::Tools;
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
  for (const CreativeCatalogAsset& asset : assets) {
    CreativeCatalogEntry entry;
    if (!buildCatalogAssetEntry(asset, entry)) {
      continue;
    }
    catalog.entries.push_back(std::move(entry));
  }
  for (const CreativeCatalogAssetFailure& failure : assetFailures) {
    CreativeCatalogEntry entry;
    entry.page = CreativeCatalogPage::Assets;
    entry.category = CreativeCatalogEntryCategory::AssetFailure;
    entry.label = failure.label.empty() ? failure.sourcePath : failure.label;
    entry.detail = failure.reasonCode;
    if (!failure.sourcePath.empty()) {
      entry.detail.append(" | ");
      entry.detail.append(failure.sourcePath);
    }
    entry.searchText = lowerAscii(
        entry.label + " " + failure.sourcePath + " " + failure.reasonCode +
        " failed rejected broken asset glb blender");
    catalog.entries.push_back(std::move(entry));
  }
  CreativeCatalogEntry reload;
  reload.page = CreativeCatalogPage::Assets;
  reload.category = CreativeCatalogEntryCategory::Command;
  reload.command = CreativeCatalogCommand::ReloadAssets;
  reload.label = "Reload Assets";
  reload.searchText = "reload assets refresh rescan blender glb";
  reload.detail = "Rescan GLB files and rebuild imported mesh resources";
  catalog.entries.push_back(std::move(reload));
  refreshFilter(catalog);
  return catalog;
}

bool appendCreativeCatalogAsset(CreativeCatalogState& catalog,
                                const CreativeCatalogAsset& asset) {
  if (std::any_of(catalog.entries.begin(), catalog.entries.end(),
                  [&asset](const CreativeCatalogEntry& entry) {
                    return entry.category ==
                               CreativeCatalogEntryCategory::Asset &&
                           creativeHotbarAssetId(entry.hotbarEntry) ==
                               asset.assetId;
                  })) {
    return false;
  }
  CreativeCatalogEntry entry;
  if (!buildCatalogAssetEntry(asset, entry)) {
    return false;
  }
  catalog.entries.push_back(std::move(entry));
  refreshFilter(catalog);
  return true;
}

bool updateCreativeCatalogAsset(CreativeCatalogState& catalog,
                                const CreativeCatalogAsset& asset) {
  const auto found = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [&asset](const CreativeCatalogEntry& entry) {
        return entry.category == CreativeCatalogEntryCategory::Asset &&
               creativeHotbarAssetId(entry.hotbarEntry) == asset.assetId;
      });
  if (found == catalog.entries.end()) {
    return false;
  }
  CreativeCatalogEntry replacement;
  if (!buildCatalogAssetEntry(asset, replacement)) {
    return false;
  }
  *found = std::move(replacement);
  refreshFilter(catalog);
  return true;
}

bool removeCreativeCatalogAsset(CreativeCatalogState& catalog,
                                std::string_view assetId,
                                std::size_t* removedEntryIndex) {
  const auto found = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [assetId](const CreativeCatalogEntry& entry) {
        return entry.category == CreativeCatalogEntryCategory::Asset &&
               creativeHotbarAssetId(entry.hotbarEntry) == assetId;
      });
  if (found == catalog.entries.end()) {
    return false;
  }
  const std::size_t index =
      static_cast<std::size_t>(std::distance(catalog.entries.begin(), found));
  catalog.entries.erase(found);
  if (removedEntryIndex != nullptr) {
    *removedEntryIndex = index;
  }
  refreshFilter(catalog);
  return true;
}

std::span<const CreativeCatalogActionEntry>
creativeCatalogActionEntries() noexcept {
  return kActionEntries;
}

bool setCreativeCatalogPage(CreativeCatalogState& catalog,
                            CreativeCatalogPage page) {
  if (page == CreativeCatalogPage::Count || catalog.page == page) {
    return false;
  }
  catalog.page = page;
  catalog.pendingActionConfirmation.reset();
  refreshFilter(catalog);
  return true;
}

bool moveCreativeCatalogPage(CreativeCatalogState& catalog,
                             std::int32_t steps) {
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
      return availability.copyAvailable;
    case CreativeInputActionId::CutSelection:
      return availability.cutAvailable;
    case CreativeInputActionId::DuplicateSelection:
      return availability.duplicateAvailable;
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
  if (!creativeCatalogEntryAssignable(entry)) {
    return {};
  }
  CreativeHotbarEntry resolved = entry.hotbarEntry;
  if (entry.category == CreativeCatalogEntryCategory::Tool) {
    static_cast<void>(
        applyCreativeHeldItemMaterial(resolved, activeMaterial));
  }
  return resolved;
}

bool creativeCatalogEntryAssignable(
    const CreativeCatalogEntry& entry) noexcept {
  return entry.category == CreativeCatalogEntryCategory::Material ||
         entry.category == CreativeCatalogEntryCategory::Asset ||
         entry.category == CreativeCatalogEntryCategory::Tool;
}

bool creativeCatalogEntryRequestsAssetReload(
    const CreativeCatalogEntry& entry) noexcept {
  return entry.category == CreativeCatalogEntryCategory::Command &&
         entry.command == CreativeCatalogCommand::ReloadAssets;
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

}  // namespace iggy3d::creative
