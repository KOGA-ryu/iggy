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
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeCatalogQueryCapacity = 64;
inline constexpr std::size_t kCreativeToolWheelCapacity = 9;
inline constexpr std::size_t kCreativeCatalogActionCapacity = 9;
inline constexpr std::size_t kCreativeCatalogShapePresetCount = 6;

enum class CreativeCatalogPage : std::uint8_t {
  Structure,
  Terrain,
  Movement,
  Logic,
  Dressing,
  Media,
  Gameplay,
  Testing,
  Helpers,
  Tools,
  Experimental,
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
  std::uint64_t contentHash = 0U;
  CreativeBounds sourceBounds{};
  StaticMeshAuthoringMetadata authoringMetadata;
  std::vector<StaticMeshCollisionPart> collisionParts;
  std::vector<StaticMeshAttachmentSocket> attachmentSockets;
  std::vector<StaticMeshMaterialVariant> materialVariants;
  std::size_t materialCount = 0U;
  StaticMeshAssetThumbnail thumbnail;
  bool authoredComposite = false;
};

struct CreativeCatalogAssetFailure {
  std::string label;
  std::string sourcePath;
  std::string reasonCode;
};

struct CreativeCatalogEntry {
  CreativeCatalogPage page = CreativeCatalogPage::Count;
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
  std::uint64_t assetContentHash = 0U;
  StaticMeshAuthoringMetadata assetAuthoringMetadata;
  std::vector<StaticMeshCollisionPart> assetCollisionParts;
  std::vector<StaticMeshAttachmentSocket> assetAttachmentSockets;
  std::vector<StaticMeshMaterialVariant> assetMaterialVariants;
  std::size_t assetMaterialCount = 0U;
  StaticMeshAssetThumbnail assetThumbnail;
  bool authoredComposite = false;
};

enum class CreativeCatalogAssetInspectionStatus : std::uint8_t {
  NotAsset,
  Ready,
  Missing,
};

struct CreativeCatalogAssetInspection {
  CreativeCatalogAssetInspectionStatus status =
      CreativeCatalogAssetInspectionStatus::NotAsset;
  CreativeBoundsMetrics bounds;
  CreativeVec3 pivotFromCenter;
  StaticMeshCollisionMode collisionMode = StaticMeshCollisionMode::Invalid;
  std::uint64_t contentHash = 0U;
  std::size_t collisionPartCount = 0U;
  std::size_t socketCount = 0U;
  std::size_t materialCount = 0U;
  std::size_t materialVariantCount = 0U;
  bool walkable = false;
  bool thumbnailAvailable = false;
  bool authoredComposite = false;
};

// Product code supplies tool rows to the generic catalog. This keeps maturity
// and exposure policy out of the input model and prevents the catalog from
// becoming a second tool registry.
struct CreativeCatalogToolSpec {
  CreativeHeldItemKind kind = CreativeHeldItemKind::Count;
  std::string_view label;
  std::string_view aliases;
  bool experimental = true;
  bool defaultWheelEligible = false;
  std::string_view purpose{};
  std::string_view maturity{};
  std::string_view lifecycle{};
  std::string_view inputHint{};
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
  CreativeCatalogPage page = CreativeCatalogPage::Structure;
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
[[nodiscard]] bool creativeCatalogPageIsCreatorCategory(
    CreativeCatalogPage page) noexcept;
[[nodiscard]] std::string_view creativeCatalogAssetPhysicsLabel(
    const StaticMeshAuthoringMetadata& metadata) noexcept;
[[nodiscard]] CreativeCatalogState makeCreativeCatalog(
    std::span<const CreativeObjectKind> materialPalette,
    std::span<const CreativeCatalogAsset> assets = {},
    std::size_t rejectedAssetCount = 0U,
    std::span<const CreativeCatalogAssetFailure> assetFailures = {},
    std::span<const CreativeCatalogToolSpec> toolSpecs = {});
[[nodiscard]] bool appendCreativeCatalogAsset(
    CreativeCatalogState& catalog,
    const CreativeCatalogAsset& asset);
[[nodiscard]] bool updateCreativeCatalogAsset(
    CreativeCatalogState& catalog,
    const CreativeCatalogAsset& asset);
[[nodiscard]] bool removeCreativeCatalogAsset(
    CreativeCatalogState& catalog,
    std::string_view assetId,
    std::size_t* removedEntryIndex = nullptr);

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
[[nodiscard]] CreativeCatalogAssetInspection inspectCreativeCatalogAsset(
    const CreativeCatalogEntry& entry) noexcept;
[[nodiscard]] CreativeHotbarEntry resolveCreativeCatalogHotbarEntry(
    const CreativeCatalogEntry& entry,
    CreativeObjectKind activeMaterial,
    std::size_t assetMaterialVariantIndex = 0U) noexcept;
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
[[nodiscard]] bool removeCreativeToolWheelCatalogEntry(
    CreativeToolWheelState& wheel,
    std::size_t removedCatalogEntryIndex) noexcept;
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
