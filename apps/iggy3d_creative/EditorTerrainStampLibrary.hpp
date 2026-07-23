#pragma once

#include "app/iggy3d/creative/tools/TerrainStampLibrary.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

inline constexpr std::string_view kCreativeEditorTerrainStampAssetExtension =
    ".igts";
inline constexpr std::size_t kCreativeEditorTerrainStampAssetMaxBytes =
    64U * 1024U;

struct CreativeEditorTerrainStampLibraryState {
  std::filesystem::path root;
  iggy3d::creative::CreativeTerrainStampLibrary library;
  std::uint64_t nextAssetOrdinal = 1U;
  std::string captureLabel = "Terrain Stamp";
  std::string statusMessage = "terrain stamp library not loaded";
};

struct CreativeEditorTerrainStampLibraryLoadReceipt {
  bool requested = false;
  bool accepted = false;
  std::size_t loadedCount = 0U;
  std::size_t rejectedCount = 0U;
  std::string reasonCode =
      "creative_editor_terrain_stamp_library_load_not_requested";
};

struct CreativeEditorTerrainStampAssetReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool durableWriteOk = false;
  std::string assetId;
  std::string label;
  iggy3d::creative::CreativeTerrainStampCopyReceipt copy{};
  iggy3d::creative::CreativeTerrainStampLibraryMutationReceipt mutation{};
  std::string reasonCode =
      "creative_editor_terrain_stamp_asset_not_requested";
};

[[nodiscard]] std::string nextCreativeEditorTerrainStampAssetId(
    const CreativeEditorTerrainStampLibraryState& state);

[[nodiscard]] CreativeEditorTerrainStampLibraryLoadReceipt
loadCreativeEditorTerrainStampLibrary(
    CreativeEditorTerrainStampLibraryState& state,
    const std::filesystem::path& creativeSaveRoot);

[[nodiscard]] CreativeEditorTerrainStampAssetReceipt
persistCreativeEditorTerrainStampAsset(
    CreativeEditorTerrainStampLibraryState& state,
    const iggy3d::creative::CreativeTerrainStamp& stamp,
    bool replaceExisting = false);

[[nodiscard]] CreativeEditorTerrainStampAssetReceipt
saveCreativeEditorTerrainSelectionAsStamp(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view label);

[[nodiscard]] CreativeEditorTerrainStampAssetReceipt
selectCreativeEditorTerrainStampAsset(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view assetId);

[[nodiscard]] CreativeEditorTerrainStampAssetReceipt
removeCreativeEditorTerrainStampAsset(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view assetId);

[[nodiscard]] CreativeEditorTerrainStampAssetReceipt
repairCreativeEditorTerrainStampAssetSource(
    CreativeEditorTerrainStampLibraryState& state,
    const iggy3d::creative::CreativeTerrainStampRecipe& embeddedRecipe,
    bool replaceExisting);

}  // namespace iggy3d_creative_app
