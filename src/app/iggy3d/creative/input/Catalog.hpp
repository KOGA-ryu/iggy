#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"
#include "content/assets/StaticMeshAuthoringMetadata.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeCatalogQueryCapacity = 64;
inline constexpr std::size_t kCreativeToolWheelCapacity = 9;
inline constexpr std::size_t kCreativeCatalogActionCapacity = 9;
inline constexpr std::size_t kCreativeCatalogShapePresetCount = 6;

enum class CreativeCatalogPage : std::uint8_t {
  Build,
  Assets,
  Actions,
  Count,
};

enum class CreativeCatalogEntryCategory : std::uint8_t {
  Material,
  Asset,
  AssetFailure,
  Command,
  Tool,
};

enum class CreativeCatalogCommand : std::uint8_t {
  None,
  ReloadAssets,
};

struct CreativeCatalogAsset {
  CreativeObjectKind objectKind = CreativeObjectKind::Prop;
  std::string assetId;
  std::string label;
  CreativeBounds sourceBounds{};
  StaticMeshAuthoringMetadata authoringMetadata;
  bool authoredComposite = false;
};

struct CreativeCatalogAssetFailure {
  std::string label;
  std::string sourcePath;
  std::string reasonCode;
};

struct CreativeCatalogEntry {
  CreativeCatalogEntryCategory category =
      CreativeCatalogEntryCategory::Material;
  CreativeHotbarEntry hotbarEntry{};
  // Marks entries admitted to the default wheel. Any Tool entry can be
  // assigned to a wheel sector by the user.
  bool toolWheelEligible = false;
  CreativeCatalogCommand command = CreativeCatalogCommand::None;
  std::string label;
  std::string searchText;
  std::string detail;
  StaticMeshAuthoringMetadata assetAuthoringMetadata;
  bool authoredComposite = false;
};

struct CreativeCatalogShapeSelection {
  CreativeShapeBrushKind kind = CreativeShapeBrushKind::Box;
  CreativeShapeBrushAxis axis = CreativeShapeBrushAxis::Y;
};

struct CreativeCatalogActionEntry {
  CreativeInputActionId action = CreativeInputActionId::Undo;
  std::string_view label;
  bool confirmationRequired = false;
};

struct CreativeCatalogActionActivation {
  CreativeInputActionId action = CreativeInputActionId::Undo;
  bool requested = false;
  bool confirmationArmed = false;
};

struct CreativeCatalogActionAvailability {
  bool undoAvailable = false;
  bool redoAvailable = false;
  bool copyAvailable = false;
  bool cutAvailable = false;
  bool duplicateAvailable = false;
  bool clipboardAvailable = false;
  bool saveAvailable = true;
  bool newAvailable = true;
  bool loadAvailable = true;
};

struct CreativeCatalogState {
  bool open = false;
  CreativeCatalogPage page = CreativeCatalogPage::Build;
  std::string query;
  std::vector<CreativeCatalogEntry> entries;
  std::vector<std::size_t> filteredEntryIndices;
  std::size_t selectedFilteredIndex = 0;
  std::size_t selectedActionIndex = 0;
  std::size_t rejectedAssetCount = 0;
  std::optional<CreativeInputActionId> pendingActionConfirmation;
};

struct CreativeToolWheelState {
  bool open = false;
  std::array<std::size_t, kCreativeToolWheelCapacity> catalogEntryIndices{};
  std::size_t entryCount = 0;
  std::size_t selectedIndex = 0;
  bool capacityExceeded = false;
};

[[nodiscard]] std::string_view toString(
    CreativeCatalogEntryCategory category) noexcept;
[[nodiscard]] std::string_view toString(CreativeCatalogPage page) noexcept;
[[nodiscard]] std::string_view creativeCatalogAssetPhysicsLabel(
    const StaticMeshAuthoringMetadata& metadata) noexcept;
[[nodiscard]] CreativeCatalogState makeCreativeCatalog(
    std::span<const CreativeObjectKind> materialPalette,
    std::span<const CreativeCatalogAsset> assets = {},
    std::size_t rejectedAssetCount = 0U,
    std::span<const CreativeCatalogAssetFailure> assetFailures = {});
[[nodiscard]] bool appendCreativeCatalogAsset(
    CreativeCatalogState& catalog,
    const CreativeCatalogAsset& asset);

[[nodiscard]] std::span<const CreativeCatalogActionEntry>
creativeCatalogActionEntries() noexcept;
[[nodiscard]] bool setCreativeCatalogPage(CreativeCatalogState& catalog,
                                          CreativeCatalogPage page);
[[nodiscard]] bool moveCreativeCatalogPage(CreativeCatalogState& catalog,
                                           std::int32_t steps);
[[nodiscard]] bool moveCreativeCatalogActionSelection(
    CreativeCatalogState& catalog,
    std::int32_t steps) noexcept;
[[nodiscard]] bool selectCreativeCatalogActionIndex(
    CreativeCatalogState& catalog,
    std::size_t actionIndex) noexcept;
[[nodiscard]] const CreativeCatalogActionEntry*
selectedCreativeCatalogAction(const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] CreativeCatalogActionActivation
activateSelectedCreativeCatalogAction(CreativeCatalogState& catalog) noexcept;
[[nodiscard]] bool creativeCatalogActionAvailable(
    CreativeInputActionId action,
    const CreativeCatalogActionAvailability& availability) noexcept;

[[nodiscard]] bool setCreativeCatalogOpen(CreativeCatalogState& catalog,
                                          bool open) noexcept;
[[nodiscard]] bool setCreativeCatalogQuery(CreativeCatalogState& catalog,
                                           std::string_view query);
[[nodiscard]] bool appendCreativeCatalogQueryText(
    CreativeCatalogState& catalog,
    std::string_view text);
[[nodiscard]] bool eraseCreativeCatalogQuery(CreativeCatalogState& catalog,
                                             std::size_t count = 1U);
[[nodiscard]] bool moveCreativeCatalogSelection(CreativeCatalogState& catalog,
                                                std::int32_t steps) noexcept;
[[nodiscard]] bool selectCreativeCatalogFilteredIndex(
    CreativeCatalogState& catalog,
    std::size_t filteredIndex) noexcept;

[[nodiscard]] const CreativeCatalogEntry* selectedCreativeCatalogEntry(
    const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] std::optional<std::size_t> selectedCreativeCatalogEntryIndex(
    const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] const CreativeCatalogEntry* creativeCatalogEntryAtFilteredIndex(
    const CreativeCatalogState& catalog,
    std::size_t filteredIndex) noexcept;
[[nodiscard]] CreativeHotbarEntry resolveCreativeCatalogHotbarEntry(
    const CreativeCatalogEntry& entry,
    CreativeObjectKind activeMaterial) noexcept;
[[nodiscard]] bool creativeCatalogEntryAssignable(
    const CreativeCatalogEntry& entry) noexcept;
[[nodiscard]] bool creativeCatalogEntryRequestsAssetReload(
    const CreativeCatalogEntry& entry) noexcept;
[[nodiscard]] bool creativeCatalogEntryUsesShapeSelection(
    const CreativeCatalogEntry& entry) noexcept;
[[nodiscard]] CreativeCatalogShapeSelection normalizeCreativeCatalogShapeSelection(
    CreativeShapeBrushKind kind,
    CreativeShapeBrushAxis axis) noexcept;
[[nodiscard]] bool moveCreativeCatalogShapeSelection(
    CreativeCatalogShapeSelection& selection,
    std::int32_t steps) noexcept;
[[nodiscard]] std::string_view creativeCatalogShapeSelectionLabel(
    CreativeCatalogShapeSelection selection) noexcept;

[[nodiscard]] bool assignSelectedCreativeCatalogEntry(
    const CreativeCatalogState& catalog,
    CreativeHotbarState& hotbar,
    std::optional<std::size_t> slot = std::nullopt) noexcept;

[[nodiscard]] CreativeToolWheelState makeCreativeToolWheel(
    const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] bool setCreativeToolWheelOpen(CreativeToolWheelState& wheel,
                                            bool open) noexcept;
[[nodiscard]] bool selectCreativeToolWheelForHotbarEntry(
    CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog,
    CreativeHotbarEntry entry) noexcept;
[[nodiscard]] bool moveCreativeToolWheelSelection(
    CreativeToolWheelState& wheel,
    std::int32_t steps) noexcept;

// O(1). Entry zero is centered at up and remaining entries run clockwise.
// Invalid/non-finite directions and vectors inside the deadzone preserve the
// current selection.
[[nodiscard]] bool selectCreativeToolWheelDirection(
    CreativeToolWheelState& wheel,
    float x,
    float y,
    float deadzone = 0.35F) noexcept;

[[nodiscard]] const CreativeCatalogEntry* selectedCreativeToolWheelEntry(
    const CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] std::optional<std::size_t> creativeToolWheelSectorForCatalogEntry(
    const CreativeToolWheelState& wheel,
    std::size_t catalogEntryIndex) noexcept;
[[nodiscard]] bool assignCreativeToolWheelCatalogEntry(
    CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog,
    std::size_t sectorIndex,
    std::size_t catalogEntryIndex) noexcept;
[[nodiscard]] bool resetCreativeToolWheel(
    CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] bool isValidCreativeToolWheel(
    const CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog) noexcept;
[[nodiscard]] bool assignSelectedCreativeToolWheelEntry(
    const CreativeToolWheelState& wheel,
    const CreativeCatalogState& catalog,
    CreativeHotbarState& hotbar) noexcept;

}  // namespace iggy3d::creative
