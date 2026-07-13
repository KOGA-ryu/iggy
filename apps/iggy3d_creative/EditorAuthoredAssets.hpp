#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "EditorEdits.hpp"
#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorAuthoredAssetLibrary {
  std::filesystem::path root;
  std::vector<iggy3d::creative::CreativeAuthoredAssetDefinition> definitions;
  std::uint64_t nextAssetOrdinal = 1U;
  iggy3d::creative::CreativeDocumentId nextDocumentId = 1'000'000U;
  std::string statusLabel;
};

struct CreativeEditorAuthoredAssetLoadReceipt {
  bool requested = false;
  bool accepted = false;
  std::size_t loadedCount = 0U;
  std::size_t rejectedCount = 0U;
  std::string reasonCode = "creative_authored_asset_load_not_requested";
};

struct CreativeEditorAuthoredAssetSaveReceipt {
  bool requested = false;
  bool accepted = false;
  std::string assetId;
  std::string label;
  iggy3d::creative::CreativeAuthoredAssetCaptureResult capture;
  bool durableWriteOk = false;
  std::string reasonCode = "creative_authored_asset_save_not_requested";
};

struct CreativeAuthoredAssetStrokeState {
  iggy3d::creative::CreativeWorldGestureRepeatState repeat{};
  StandaloneEditTransaction transaction{};
  iggy3d::creative::CreativeWorldGestureVisitedKeys visited{};
  CreativeBrushPlacementAdmission preview{};
  std::uint16_t acceptedMutationCount = 0U;
  bool capacityReached = false;
};

[[nodiscard]] const iggy3d::creative::CreativeAuthoredAssetDefinition*
findCreativeEditorAuthoredAsset(
    const CreativeEditorAuthoredAssetLibrary& library,
    std::string_view assetId) noexcept;

[[nodiscard]] std::vector<iggy3d::creative::CreativeCatalogAsset>
creativeEditorAuthoredAssetCatalogEntries(
    const CreativeEditorAuthoredAssetLibrary& library);

[[nodiscard]] CreativeEditorAuthoredAssetLoadReceipt
loadCreativeEditorAuthoredAssetLibrary(
    CreativeEditorAuthoredAssetLibrary& library,
    const std::filesystem::path& creativeSaveRoot);

[[nodiscard]] CreativeEditorAuthoredAssetSaveReceipt
saveCreativeEditorSelectionAsAuthoredAsset(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorAuthoredAssetLibrary& library,
    std::string_view label = {});

[[nodiscard]] bool creativeEditorUsesAuthoredAsset(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const CreativeEditorAuthoredAssetLibrary& library) noexcept;

void processCreativeAuthoredAssetFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);

void finalizeCreativeAuthoredAssetStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);

}  // namespace iggy3d_creative_app
