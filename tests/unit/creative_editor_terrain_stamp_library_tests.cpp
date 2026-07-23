#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorTerrainStampLibrary.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

namespace {
namespace cr = iggy3d::creative;
using namespace iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

class TemporaryDirectory {
 public:
  TemporaryDirectory() {
    path_ = std::filesystem::temp_directory_path() /
            "iggy3d_terrain_stamp_library_tests";
    std::error_code error;
    std::filesystem::remove_all(path_, error);
    error.clear();
    std::filesystem::create_directories(path_, error);
  }

  ~TemporaryDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  [[nodiscard]] const std::filesystem::path& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

void installTerrainDocument(cr::CreativeAppState& appState) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Terrain Stamp Library Test");
  static_cast<void>(document.assignId(711U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const std::array edits{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 1}, 7U, 1U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(edits));
  const std::array materials{
      cr::CreativeTerrainMaterialEdit{
          cr::CreativeTerrainMaterialEditKind::Set,
          {2, 1}, cr::CreativeTerrainMaterial::Stone},
  };
  static_cast<void>(appState.facade.applyTerrainMaterialEdits(materials));
}

void selectRegion(CreativeEditorState& editor) {
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First, {0, 0, 0}));
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::Second, {2, 0, 1}));
}

bool namedCaptureSurvivesColdRestartExactly() {
  TemporaryDirectory temporary;
  cr::CreativeAppState appState;
  installTerrainDocument(appState);
  CreativeEditorState editor;
  selectRegion(editor);
  const auto loaded =
      loadCreativeEditorTerrainStampLibrary(editor.terrainStamps,
                                            temporary.path());
  const auto saved = saveCreativeEditorTerrainSelectionAsStamp(
      appState, editor, "Stone Rise");
  const cr::CreativeTerrainStamp captured = appState.terrainStamp;
  const std::filesystem::path assetPath =
      editor.terrainStamps.root /
      (captured.assetId +
       std::string(kCreativeEditorTerrainStampAssetExtension));

  CreativeEditorTerrainStampLibraryState restarted;
  const auto reloaded =
      loadCreativeEditorTerrainStampLibrary(restarted, temporary.path());
  const cr::CreativeTerrainStamp* restored =
      cr::findCreativeTerrainStamp(restarted.library, captured.assetId);
  return expect(loaded.accepted && saved.accepted && saved.changed &&
                    saved.durableWriteOk && saved.copy.accepted &&
                    captured.assetId == "terrain_stamp_0001" &&
                    captured.label == "Stone Rise" &&
                    captured.widthCells == 3U &&
                    captured.depthCells == 2U &&
                    std::filesystem::is_regular_file(assetPath),
                "selected composed terrain saves as one named durable stamp") &&
         expect(reloaded.accepted && reloaded.loadedCount == 1U &&
                    reloaded.rejectedCount == 0U && restored != nullptr &&
                    *restored == captured &&
                    restarted.library.selectedAssetId == captured.assetId &&
                    restarted.nextAssetOrdinal == 2U &&
                    nextCreativeEditorTerrainStampAssetId(restarted) ==
                        "terrain_stamp_0002",
                "cold restart restores exact stamp identity height and material");
}

bool selectionDeleteAndEmbeddedRepairAreExplicit() {
  TemporaryDirectory temporary;
  cr::CreativeAppState appState;
  installTerrainDocument(appState);
  CreativeEditorState editor;
  selectRegion(editor);
  static_cast<void>(loadCreativeEditorTerrainStampLibrary(
      editor.terrainStamps, temporary.path()));
  const auto saved = saveCreativeEditorTerrainSelectionAsStamp(
      appState, editor, "Repair Source");
  cr::CreativeTerrainStampRecipe embedded;
  embedded.stamp = appState.terrainStamp;
  embedded.targetMinimum = {20, 30};

  const auto selected = selectCreativeEditorTerrainStampAsset(
      appState, editor, embedded.stamp.assetId);
  const bool previewActive = editor.terrain.region.stamp.active;
  const auto removed = removeCreativeEditorTerrainStampAsset(
      appState, editor, embedded.stamp.assetId);
  const auto missing = cr::creativeTerrainStampSourceStatus(
      editor.terrainStamps.library, embedded);
  const auto repaired = repairCreativeEditorTerrainStampAssetSource(
      editor.terrainStamps, embedded, false);
  const auto available = cr::creativeTerrainStampSourceStatus(
      editor.terrainStamps.library, embedded);

  cr::CreativeTerrainStampRecipe changed = embedded;
  changed.stamp.heights[0U] += 1U;
  changed.stamp.minimumHeightCells = cr::kCreativeTerrainMaximumHeightCells;
  for (const std::uint16_t height : changed.stamp.heights) {
    if (height != 0U) {
      changed.stamp.minimumHeightCells =
          std::min(changed.stamp.minimumHeightCells, height);
    }
  }
  changed.stamp.contentSignature =
      cr::creativeTerrainStampContentSignature(changed.stamp);
  const auto conflict = repairCreativeEditorTerrainStampAssetSource(
      editor.terrainStamps, changed, false);
  const auto replaced = repairCreativeEditorTerrainStampAssetSource(
      editor.terrainStamps, changed, true);

  CreativeEditorTerrainStampLibraryState restarted;
  const auto reloaded =
      loadCreativeEditorTerrainStampLibrary(restarted, temporary.path());
  const cr::CreativeTerrainStamp* restored =
      cr::findCreativeTerrainStamp(restarted.library,
                                   changed.stamp.assetId);
  return expect(saved.accepted && selected.accepted && previewActive &&
                    removed.accepted && removed.changed &&
                    !editor.terrain.region.stamp.active &&
                    cr::creativeTerrainStampEmpty(appState.terrainStamp) &&
                    missing == cr::CreativeTerrainStampSourceStatus::Missing,
                "catalog selection starts preview and deletion exposes missing source") &&
         expect(repaired.accepted && repaired.changed &&
                    repaired.durableWriteOk &&
                    available ==
                        cr::CreativeTerrainStampSourceStatus::Available &&
                    !conflict.accepted && replaced.accepted &&
                    replaced.changed && reloaded.accepted &&
                    restored != nullptr && *restored == changed.stamp,
                "embedded baked recipe repairs missing or drifted source explicitly");
}

bool invalidSelectionAndCorruptFilesDoNotLeakState() {
  TemporaryDirectory temporary;
  cr::CreativeAppState appState;
  installTerrainDocument(appState);
  CreativeEditorState editor;
  static_cast<void>(loadCreativeEditorTerrainStampLibrary(
      editor.terrainStamps, temporary.path()));
  const auto invalid = saveCreativeEditorTerrainSelectionAsStamp(
      appState, editor, "No Selection");
  const bool remainedEmpty = editor.terrainStamps.library.stamps.empty() &&
                             cr::creativeTerrainStampEmpty(
                                 appState.terrainStamp);
  cr::CreativeTerrainStamp unsafe;
  unsafe.assetId = "../outside";
  unsafe.label = "Unsafe";
  unsafe.widthCells = 1U;
  unsafe.depthCells = 1U;
  unsafe.minimumHeightCells = 2U;
  unsafe.heights = {2U};
  unsafe.materials = {cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Grass)};
  unsafe.contentSignature = cr::creativeTerrainStampContentSignature(unsafe);
  const auto unsafePersist = persistCreativeEditorTerrainStampAsset(
      editor.terrainStamps, unsafe, false);

  const std::filesystem::path corruptPath =
      editor.terrainStamps.root / "terrain_stamp_0007.igts";
  {
    std::ofstream stream(corruptPath, std::ios::binary | std::ios::trunc);
    stream << "not a terrain stamp";
  }
  CreativeEditorTerrainStampLibraryState restarted;
  const auto reloaded =
      loadCreativeEditorTerrainStampLibrary(restarted, temporary.path());
  return expect(!invalid.accepted && !unsafePersist.accepted &&
                    remainedEmpty &&
                    !std::filesystem::exists(temporary.path() / "outside.igts"),
                "invalid selection and unsafe identity leave state and disk untouched") &&
         expect(reloaded.accepted && reloaded.loadedCount == 0U &&
                    reloaded.rejectedCount == 1U &&
                    restarted.library.stamps.empty() &&
                    restarted.nextAssetOrdinal == 8U,
                "corrupt asset is rejected without hiding its occupied ordinal");
}

CreativeDesktopCommandResult dispatch(
    CreativeDesktopCommandId id,
    CreativeDesktopCommandPayload payload,
    const CreativeDesktopCommandContext& context) {
  CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return dispatchCreativeDesktopCommands(frame, context);
}

bool semanticCommandsOwnCatalogAndDuplicateStampPayload() {
  TemporaryDirectory temporary;
  cr::CreativeAppState appState;
  installTerrainDocument(appState);
  CreativeEditorState editor;
  selectRegion(editor);
  static_cast<void>(loadCreativeEditorTerrainStampLibrary(
      editor.terrainStamps, temporary.path()));
  const CreativeDesktopCommandContext context{
      appState, editor, temporary.path(), nullptr, nullptr, nullptr};

  const auto saved = dispatch(
      CreativeDesktopCommandId::TerrainStampSaveSelection,
      CreativeDesktopTerrainStampPayload{
          {}, "Command Stamp", cr::kInvalidCreativeTerrainOperationId, false},
      context);
  const std::string assetId = editor.terrainStamps.library.selectedAssetId;
  const auto selected = dispatch(
      CreativeDesktopCommandId::TerrainStampSelect,
      CreativeDesktopTerrainStampPayload{
          assetId, {}, cr::kInvalidCreativeTerrainOperationId, false},
      context);
  const bool previewStarted = editor.terrain.region.stamp.active;

  cr::CreativeTerrainOperationMutationRequest add;
  add.kind = cr::CreativeTerrainOperationMutationKind::Add;
  add.owner = cr::CreativeTerrainOperationOwner::Manual;
  add.operationKind = cr::CreativeTerrainOperationKind::Stamp;
  add.stamp.stamp = appState.terrainStamp;
  add.stamp.targetMinimum = {20, 20};
  add.stamp.elevationMode = cr::CreativeTerrainStampElevationMode::Absolute;
  const cr::CreativeTerrainOperationMutationReceipt added =
      appState.facade.applyTerrainOperationMutation(add);
  const auto deleted = dispatch(
      CreativeDesktopCommandId::TerrainStampDelete,
      CreativeDesktopTerrainStampPayload{
          assetId, {}, cr::kInvalidCreativeTerrainOperationId, false},
      context);
  const auto repaired = dispatch(
      CreativeDesktopCommandId::TerrainStampRepairSource,
      CreativeDesktopTerrainStampPayload{
          assetId, {}, added.operationId, false},
      context);
  const auto duplicated = dispatch(
      CreativeDesktopCommandId::TerrainOperationDuplicate,
      CreativeDesktopTerrainOperationPayload{
          added.operationId, true, 0U},
      context);

  const auto& operations =
      appState.facade.document().terrainOperationStack().operations;
  return expect(saved.accepted && saved.changed &&
                    saved.lastCommand ==
                        CreativeDesktopCommandId::TerrainStampSaveSelection &&
                    selected.accepted && selected.sceneChanged &&
                    previewStarted && added.accepted && deleted.accepted &&
                    !editor.terrain.region.stamp.active && repaired.accepted &&
                    repaired.changed &&
                    cr::creativeTerrainStampSourceStatus(
                        editor.terrainStamps.library, add.stamp) ==
                        cr::CreativeTerrainStampSourceStatus::Available,
                "semantic commands own save place delete and source repair") &&
         expect(duplicated.accepted && duplicated.changed &&
                    duplicated.sceneChanged && operations.size() == 2U &&
                    operations[0U].kind ==
                        cr::CreativeTerrainOperationKind::Stamp &&
                    operations[1U].kind ==
                        cr::CreativeTerrainOperationKind::Stamp &&
                    operations[1U].stamp == operations[0U].stamp,
                "terrain operation duplication retains the exact baked stamp payload");
}

}  // namespace

int main() {
  bool ok = true;
  ok = namedCaptureSurvivesColdRestartExactly() && ok;
  ok = selectionDeleteAndEmbeddedRepairAreExplicit() && ok;
  ok = invalidSelectionAndCorruptFilesDoNotLeakState() && ok;
  ok = semanticCommandsOwnCatalogAndDuplicateStampPayload() && ok;
  return ok ? 0 : 1;
}
