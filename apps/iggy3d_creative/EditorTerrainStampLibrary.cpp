#include "EditorTerrainStampLibrary.hpp"

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::string_view kAssetPrefix = "terrain_stamp_";

[[nodiscard]] bool validFilesystemAssetId(std::string_view assetId) noexcept {
  return !assetId.empty() &&
         assetId.size() <= cr::kCreativeTerrainStampAssetIdCapacity &&
         std::all_of(assetId.begin(), assetId.end(), [](char value) {
           return (value >= 'a' && value <= 'z') ||
                  (value >= 'A' && value <= 'Z') ||
                  (value >= '0' && value <= '9') || value == '_' ||
                  value == '-';
         });
}

[[nodiscard]] std::string assetIdForOrdinal(std::uint64_t ordinal) {
  std::ostringstream stream;
  stream << kAssetPrefix << std::setw(4) << std::setfill('0') << ordinal;
  return stream.str();
}

[[nodiscard]] std::uint64_t assetOrdinal(std::string_view assetId) noexcept {
  if (!assetId.starts_with(kAssetPrefix)) {
    return 0U;
  }
  const std::string_view digits = assetId.substr(kAssetPrefix.size());
  std::uint64_t ordinal = 0U;
  const auto parsed = std::from_chars(digits.data(),
                                      digits.data() + digits.size(), ordinal);
  return parsed.ec == std::errc{} &&
                 parsed.ptr == digits.data() + digits.size()
             ? ordinal
             : 0U;
}

[[nodiscard]] std::filesystem::path assetPath(
    const CreativeEditorTerrainStampLibraryState& state,
    std::string_view assetId) {
  return state.root /
         (std::string(assetId) +
          std::string(kCreativeEditorTerrainStampAssetExtension));
}

[[nodiscard]] bool readAssetFile(const std::filesystem::path& path,
                                 std::vector<std::uint8_t>& output) {
  std::error_code error;
  const std::uintmax_t size = std::filesystem::file_size(path, error);
  if (error || size == 0U ||
      size > kCreativeEditorTerrainStampAssetMaxBytes) {
    return false;
  }
  output.resize(static_cast<std::size_t>(size));
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    output.clear();
    return false;
  }
  stream.read(reinterpret_cast<char*>(output.data()),
              static_cast<std::streamsize>(output.size()));
  return stream.good() ||
         (stream.eof() &&
          stream.gcount() == static_cast<std::streamsize>(output.size()));
}

[[nodiscard]] bool writeAssetFileAtomically(
    const std::filesystem::path& path,
    std::span<const std::uint8_t> bytes) {
  std::filesystem::path temporary = path;
  temporary += ".tmp";
  {
    std::ofstream stream(temporary,
                         std::ios::binary | std::ios::trunc);
    if (!stream) {
      return false;
    }
    stream.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    stream.flush();
    if (!stream) {
      std::error_code cleanupError;
      std::filesystem::remove(temporary, cleanupError);
      return false;
    }
  }
  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    std::error_code cleanupError;
    std::filesystem::remove(temporary, cleanupError);
    return false;
  }
  return true;
}

[[nodiscard]] bool selectedRegionBounds(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2& minimumCoord,
    cr::CreativeTerrainCoord2& maximumCoord) noexcept {
  const cr::CreativeVolumeSelection& selection = editor.volume.selection;
  if (!cr::creativeVolumeSelectionValid(selection)) {
    return false;
  }
  minimumCoord = {
      std::min(selection.firstCell.x, selection.secondCell.x),
      std::min(selection.firstCell.z, selection.secondCell.z)};
  maximumCoord = {
      std::max(selection.firstCell.x, selection.secondCell.x),
      std::max(selection.firstCell.z, selection.secondCell.z)};
  return true;
}

void setStatus(CreativeEditorTerrainStampLibraryState& state,
               std::string message) {
  state.statusMessage = std::move(message);
}

}  // namespace

std::string nextCreativeEditorTerrainStampAssetId(
    const CreativeEditorTerrainStampLibraryState& state) {
  std::uint64_t ordinal = state.nextAssetOrdinal;
  for (;;) {
    const std::string assetId = assetIdForOrdinal(ordinal);
    std::error_code error;
    const bool fileExists =
        std::filesystem::exists(assetPath(state, assetId), error);
    if (error) {
      return {};
    }
    if (cr::findCreativeTerrainStamp(state.library, assetId) == nullptr &&
        !fileExists) {
      return assetId;
    }
    if (ordinal == std::numeric_limits<std::uint64_t>::max()) {
      return {};
    }
    ++ordinal;
  }
}

CreativeEditorTerrainStampLibraryLoadReceipt
loadCreativeEditorTerrainStampLibrary(
    CreativeEditorTerrainStampLibraryState& state,
    const std::filesystem::path& creativeSaveRoot) {
  CreativeEditorTerrainStampLibraryLoadReceipt receipt;
  receipt.requested = true;
  state = {};
  state.root = creativeSaveRoot / "terrain_stamps";
  std::error_code error;
  std::filesystem::create_directories(state.root, error);
  if (error) {
    receipt.reasonCode =
        "creative_editor_terrain_stamp_library_root_create_failed";
    setStatus(state, receipt.reasonCode);
    state.root.clear();
    return receipt;
  }

  std::vector<std::filesystem::path> paths;
  for (std::filesystem::directory_iterator iterator(state.root, error), end;
       !error && iterator != end; iterator.increment(error)) {
    std::error_code typeError;
    if (iterator->is_regular_file(typeError) && !typeError &&
        iterator->path().extension() ==
            kCreativeEditorTerrainStampAssetExtension) {
      paths.push_back(iterator->path());
    }
  }
  if (error) {
    receipt.reasonCode =
        "creative_editor_terrain_stamp_library_scan_failed";
    setStatus(state, receipt.reasonCode);
    state.root.clear();
    return receipt;
  }
  std::sort(paths.begin(), paths.end());

  std::uint64_t maximumOrdinal = 0U;
  for (const std::filesystem::path& path : paths) {
    const std::string filenameId = path.stem().string();
    maximumOrdinal = std::max(maximumOrdinal, assetOrdinal(filenameId));
    std::vector<std::uint8_t> bytes;
    if (!readAssetFile(path, bytes)) {
      ++receipt.rejectedCount;
      continue;
    }
    cr::CreativeTerrainStampAssetDecodeResult decoded =
        cr::decodeCreativeTerrainStamp(bytes);
    if (!decoded.accepted || !validFilesystemAssetId(filenameId) ||
        decoded.stamp.assetId != filenameId) {
      ++receipt.rejectedCount;
      continue;
    }
    const cr::CreativeTerrainStampLibraryMutationReceipt installed =
        cr::installCreativeTerrainStamp(state.library, decoded.stamp);
    if (!installed.accepted || !installed.changed) {
      ++receipt.rejectedCount;
      continue;
    }
    ++receipt.loadedCount;
  }
  state.nextAssetOrdinal =
      maximumOrdinal == std::numeric_limits<std::uint64_t>::max()
          ? maximumOrdinal
          : maximumOrdinal + 1U;
  if (!state.library.stamps.empty()) {
    static_cast<void>(cr::selectCreativeTerrainStamp(
        state.library, state.library.stamps.front().assetId));
  }
  receipt.accepted = true;
  receipt.reasonCode = "creative_editor_terrain_stamp_library_ready";
  setStatus(state, std::to_string(receipt.loadedCount) +
                       " terrain stamp(s) loaded" +
                       (receipt.rejectedCount == 0U
                            ? std::string{}
                            : ", " + std::to_string(receipt.rejectedCount) +
                                  " rejected"));
  return receipt;
}

CreativeEditorTerrainStampAssetReceipt persistCreativeEditorTerrainStampAsset(
    CreativeEditorTerrainStampLibraryState& state,
    const cr::CreativeTerrainStamp& stamp,
    bool replaceExisting) {
  CreativeEditorTerrainStampAssetReceipt receipt;
  receipt.requested = true;
  receipt.assetId = stamp.assetId;
  receipt.label = stamp.label;
  if (state.root.empty() || !validFilesystemAssetId(stamp.assetId)) {
    receipt.reasonCode =
        state.root.empty()
            ? "creative_editor_terrain_stamp_library_not_loaded"
            : "creative_editor_terrain_stamp_asset_id_invalid";
    setStatus(state, receipt.reasonCode);
    return receipt;
  }

  cr::CreativeTerrainStampLibrary staged = state.library;
  receipt.mutation =
      cr::installCreativeTerrainStamp(staged, stamp, replaceExisting);
  if (!receipt.mutation.accepted) {
    receipt.reasonCode = std::string(receipt.mutation.reasonCode);
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  if (!receipt.mutation.changed) {
    std::vector<std::uint8_t> currentBytes;
    const cr::CreativeTerrainStampAssetDecodeResult current =
        readAssetFile(assetPath(state, stamp.assetId), currentBytes)
            ? cr::decodeCreativeTerrainStamp(currentBytes)
            : cr::CreativeTerrainStampAssetDecodeResult{};
    if (current.accepted && current.stamp == stamp) {
      receipt.accepted = true;
      receipt.reasonCode = "creative_editor_terrain_stamp_asset_no_change";
      setStatus(state, "terrain stamp is already current");
      return receipt;
    }
  }

  const cr::CreativeTerrainStampAssetEncodeResult encoded =
      cr::encodeCreativeTerrainStamp(stamp);
  if (!encoded.accepted ||
      !writeAssetFileAtomically(assetPath(state, stamp.assetId),
                                encoded.bytes)) {
    receipt.reasonCode =
        "creative_editor_terrain_stamp_asset_write_failed";
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  receipt.durableWriteOk = true;
  state.library = std::move(staged);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.reasonCode = "creative_editor_terrain_stamp_asset_saved";
  setStatus(state, "terrain stamp saved");
  return receipt;
}

CreativeEditorTerrainStampAssetReceipt
saveCreativeEditorTerrainSelectionAsStamp(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view requestedLabel) {
  CreativeEditorTerrainStampAssetReceipt receipt;
  receipt.requested = true;
  CreativeEditorTerrainStampLibraryState& state = editor.terrainStamps;
  cr::CreativeTerrainCoord2 minimumCoord{};
  cr::CreativeTerrainCoord2 maximumCoord{};
  if (state.root.empty() ||
      !selectedRegionBounds(editor, minimumCoord, maximumCoord)) {
    receipt.reasonCode = state.root.empty()
                             ? "creative_editor_terrain_stamp_library_not_loaded"
                             : "creative_editor_terrain_stamp_selection_invalid";
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  receipt.assetId = nextCreativeEditorTerrainStampAssetId(state);
  if (receipt.assetId.empty()) {
    receipt.reasonCode = "creative_editor_terrain_stamp_asset_id_exhausted";
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  receipt.label = requestedLabel.empty()
                      ? "Terrain Stamp " +
                            std::to_string(state.nextAssetOrdinal)
                      : std::string(requestedLabel);
  const cr::CreativeDocument& document = appState.facade.document();
  cr::CreativeTerrainStamp stamp;
  receipt.copy = cr::copyCreativeTerrainRegionToStamp(
      document.id(), document.revision(),
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), document.terrainHeightField(),
          document.terrainHardEdges()),
      document.terrainMaterialField(), minimumCoord, maximumCoord,
      receipt.assetId, receipt.label, 1U, stamp);
  if (!receipt.copy.accepted) {
    receipt.reasonCode = std::string(receipt.copy.reasonCode);
    setStatus(state, receipt.reasonCode);
    return receipt;
  }

  const cr::CreativeTerrainStampCopyReceipt copy = receipt.copy;
  receipt = persistCreativeEditorTerrainStampAsset(state, stamp, false);
  receipt.copy = copy;
  if (!receipt.accepted) {
    return receipt;
  }
  appState.terrainStamp = stamp;
  const std::uint64_t writtenOrdinal = assetOrdinal(stamp.assetId);
  state.nextAssetOrdinal =
      writtenOrdinal == std::numeric_limits<std::uint64_t>::max()
          ? writtenOrdinal
          : std::max(state.nextAssetOrdinal, writtenOrdinal + 1U);
  return receipt;
}

CreativeEditorTerrainStampAssetReceipt selectCreativeEditorTerrainStampAsset(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view assetId) {
  CreativeEditorTerrainStampAssetReceipt receipt;
  receipt.requested = true;
  receipt.assetId = std::string(assetId);
  CreativeEditorTerrainStampLibraryState& state = editor.terrainStamps;
  receipt.mutation = cr::selectCreativeTerrainStamp(state.library, assetId);
  const cr::CreativeTerrainStamp* stamp =
      cr::selectedCreativeTerrainStamp(state.library);
  if (!receipt.mutation.accepted || stamp == nullptr) {
    receipt.reasonCode = std::string(receipt.mutation.reasonCode);
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  appState.terrainStamp = *stamp;
  const CreativeEditorTerrainStampReceipt preview =
      beginCreativeEditorTerrainStampPreview(appState, editor);
  if (!preview.accepted) {
    receipt.reasonCode = std::string(preview.reasonCode);
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  receipt.label = stamp->label;
  receipt.accepted = true;
  receipt.changed = receipt.mutation.changed || preview.changed;
  receipt.reasonCode = "creative_editor_terrain_stamp_asset_selected";
  setStatus(state, "terrain stamp ready to place");
  return receipt;
}

CreativeEditorTerrainStampAssetReceipt removeCreativeEditorTerrainStampAsset(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view assetId) {
  CreativeEditorTerrainStampAssetReceipt receipt;
  receipt.requested = true;
  receipt.assetId = std::string(assetId);
  CreativeEditorTerrainStampLibraryState& state = editor.terrainStamps;
  const cr::CreativeTerrainStamp* stamp =
      cr::findCreativeTerrainStamp(state.library, assetId);
  if (state.root.empty() || stamp == nullptr) {
    receipt.reasonCode = "creative_editor_terrain_stamp_asset_not_found";
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  receipt.label = stamp->label;
  const std::filesystem::path path = assetPath(state, assetId);
  std::error_code error;
  const bool exists = std::filesystem::exists(path, error);
  if (error || (exists && !std::filesystem::remove(path, error)) || error) {
    receipt.reasonCode = "creative_editor_terrain_stamp_asset_remove_failed";
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  receipt.mutation = cr::removeCreativeTerrainStamp(state.library, assetId);
  if (!receipt.mutation.accepted) {
    receipt.reasonCode = std::string(receipt.mutation.reasonCode);
    setStatus(state, receipt.reasonCode);
    return receipt;
  }
  static_cast<void>(cancelCreativeEditorTerrainStamp(editor));
  const cr::CreativeTerrainStamp* selected =
      cr::selectedCreativeTerrainStamp(state.library);
  if (selected == nullptr) {
    cr::clearCreativeTerrainStamp(appState.terrainStamp);
  } else {
    appState.terrainStamp = *selected;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.reasonCode = "creative_editor_terrain_stamp_asset_removed";
  setStatus(state, "terrain stamp removed");
  return receipt;
}

CreativeEditorTerrainStampAssetReceipt
repairCreativeEditorTerrainStampAssetSource(
    CreativeEditorTerrainStampLibraryState& state,
    const cr::CreativeTerrainStampRecipe& embeddedRecipe,
    bool replaceExisting) {
  CreativeEditorTerrainStampAssetReceipt receipt =
      persistCreativeEditorTerrainStampAsset(
          state, embeddedRecipe.stamp, replaceExisting);
  if (receipt.accepted) {
    receipt.reasonCode = receipt.changed
                             ? "creative_editor_terrain_stamp_source_repaired"
                             : "creative_editor_terrain_stamp_source_current";
    setStatus(state, receipt.changed ? "terrain stamp source repaired"
                                     : "terrain stamp source is current");
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
