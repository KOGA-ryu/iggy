#include "EditorAuthoredAssets.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

#include "EditorGroup.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/AssetScatter.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::string_view kAuthoredAssetPackageId =
    "iggy3d.authored_asset";
constexpr std::string_view kAuthoredAssetScenarioId =
    "creative.authored_asset";

[[nodiscard]] std::string assetIdForOrdinal(std::uint64_t ordinal) {
  std::ostringstream stream;
  stream << "authored_" << std::setw(4) << std::setfill('0') << ordinal;
  return stream.str();
}

[[nodiscard]] std::uint64_t assetOrdinal(std::string_view assetId) noexcept {
  constexpr std::string_view prefix = "authored_";
  if (!assetId.starts_with(prefix)) {
    return 0U;
  }
  std::uint64_t ordinal = 0U;
  const std::string_view digits = assetId.substr(prefix.size());
  const auto parsed = std::from_chars(digits.data(),
                                      digits.data() + digits.size(), ordinal);
  return parsed.ec == std::errc{} && parsed.ptr == digits.data() + digits.size()
             ? ordinal
             : 0U;
}

[[nodiscard]] std::string defaultAssetLabel(
    const cr::CreativeAppState& appState,
    std::uint64_t ordinal) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  if (cr::selectedTargetCount(selection) == 1U &&
      selection.selectedTarget.value != cr::kInvalidId) {
    const cr::CreativeObject* object = appState.facade.findObject(
        static_cast<cr::CreativeObjectId>(selection.selectedTarget.value));
    if (object != nullptr && !object->name.empty()) {
      return object->name;
    }
  }
  std::ostringstream stream;
  stream << "Authored " << std::setw(3) << std::setfill('0') << ordinal;
  return stream.str();
}

[[nodiscard]] std::vector<cr::CreativeObjectId> selectedObjectIds(
    const cr::CreativeSelectionState& selection) {
  std::vector<cr::CreativeObjectId> result;
  result.reserve(cr::selectedTargetCount(selection));
  for (cr::TargetRef target : cr::selectedTargetList(selection)) {
    if (target.value != cr::kInvalidId) {
      result.push_back(static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  return result;
}

[[nodiscard]] cr::CreativeObjectId authoredInstanceRoot(
    const cr::CreativeDocument& document,
    cr::CreativeObjectId objectId) noexcept {
  const cr::CreativeObject* object = document.findObject(objectId);
  std::size_t hops = 0U;
  while (object != nullptr && hops++ <= document.objectCount()) {
    if (object->kind == cr::CreativeObjectKind::PrefabInstance) {
      return object->id;
    }
    object = object->parentId.has_value()
                 ? document.findObject(*object->parentId)
                 : nullptr;
  }
  return cr::kInvalidObjectId;
}

[[nodiscard]] std::uint64_t removeVisitKey(
    cr::CreativeObjectId objectId) noexcept {
  return objectId == cr::kInvalidObjectId
             ? 0U
             : static_cast<std::uint64_t>(objectId) |
                   (std::uint64_t{1} << 63U);
}

[[nodiscard]] CreativeBrushPlacementAdmission placementPreview(
    const CreativeEditorState& editor,
    const cr::CreativeHotbarEntry& held) noexcept {
  return admitBrushPlacement(held, editor.interaction.target.grid,
                             editor.toolSettings.placementYaw);
}

void ensureTransaction(cr::CreativeAppState& appState,
                       CreativeAuthoredAssetStrokeState& stroke) {
  if (!stroke.transaction.active) {
    stroke.transaction = beginEditTransaction(
        appState.facade, "creative_authored_asset_stroke");
  }
}

void applyPlacement(cr::CreativeAppState& appState,
                    CreativeEditorState& editor,
                    const cr::CreativeHotbarEntry& held) {
  CreativeAuthoredAssetStrokeState& stroke =
      editor.interaction.authoredAssetStroke;
  const std::string_view assetId = cr::creativeHotbarAssetId(held);
  const cr::CreativeAuthoredAssetDefinition* definition =
      findCreativeEditorAuthoredAsset(editor.authoredAssets, assetId);
  const CreativeBrushPlacementAdmission admission =
      placementPreview(editor, held);
  if (definition == nullptr || !admission.allowed ||
      creativeBrushPlacementAlreadyExists(
          appState.facade.document(), admission.plan, assetId)) {
    setCreativeEditorPlacementFeedback(
        editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
        editor.frameIndex, cr::CreativeObjectKind::PrefabInstance);
    return;
  }
  const std::uint64_t key = cr::creativeAssetScatterSpatialKey(
      admission.plan.transform.position, editor.placeCellSize);
  if (key == 0U || cr::creativeWorldGestureVisited(stroke.visited, key)) {
    return;
  }
  if (stroke.visited.count >= stroke.visited.keys.size()) {
    stroke.capacityReached = true;
    setCreativeEditorPlacementFeedback(
        editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
        editor.frameIndex, cr::CreativeObjectKind::PrefabInstance);
    return;
  }

  ensureTransaction(appState, stroke);
  cr::CreativeAuthoredAssetPlacementRequest request;
  request.definition = definition;
  request.targetAnchor = admission.plan.transform.position;
  request.yawRadians = admission.plan.transform.rotationEulerRadians.y;
  const cr::CreativeObjectId activeParent =
      activeCreativeEditorGroupFocusId(editor.groupFocus);
  if (activeParent != cr::kInvalidObjectId) {
    request.parentId = activeParent;
  }
  const cr::CreativeAuthoredAssetInstanceReceipt receipt =
      appState.facade.instantiateAuthoredAsset(request);
  if (!receipt.accepted || !receipt.changed) {
    setCreativeEditorPlacementFeedback(
        editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
        editor.frameIndex, cr::CreativeObjectKind::PrefabInstance);
    return;
  }
  static_cast<void>(cr::rememberCreativeWorldGestureKey(stroke.visited, key));
  ++stroke.acceptedMutationCount;
  ++editor.placedCount;
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Placed,
      editor.frameIndex, cr::CreativeObjectKind::PrefabInstance,
      receipt.instanceRootObjectId);
}

void applyRemoval(cr::CreativeAppState& appState,
                  CreativeEditorState& editor) {
  CreativeAuthoredAssetStrokeState& stroke =
      editor.interaction.authoredAssetStroke;
  const cr::CreativeObjectId rootId = authoredInstanceRoot(
      appState.facade.document(), editor.interaction.target.objectId);
  const std::uint64_t key = removeVisitKey(rootId);
  if (key == 0U || cr::creativeWorldGestureVisited(stroke.visited, key)) {
    return;
  }
  if (stroke.visited.count >= stroke.visited.keys.size()) {
    stroke.capacityReached = true;
    return;
  }
  ensureTransaction(appState, stroke);
  const cr::CreativeDocumentRemoveReceipt receipt =
      appState.facade.removeDocumentObject(rootId);
  if (!receipt.accepted || !receipt.objectRemoved) {
    setCreativeEditorPlacementFeedback(
        editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
        editor.frameIndex, cr::CreativeObjectKind::PrefabInstance);
    return;
  }
  static_cast<void>(cr::rememberCreativeWorldGestureKey(stroke.visited, key));
  ++stroke.acceptedMutationCount;
  clearCreativeEditorPlacementFeedback(editor.interaction);
}

}  // namespace

const cr::CreativeAuthoredAssetDefinition* findCreativeEditorAuthoredAsset(
    const CreativeEditorAuthoredAssetLibrary& library,
    std::string_view assetId) noexcept {
  const auto found = std::find_if(
      library.definitions.begin(), library.definitions.end(),
      [assetId](const auto& definition) {
        return definition.assetId == assetId;
      });
  return found == library.definitions.end() ? nullptr : &*found;
}

std::vector<cr::CreativeCatalogAsset> creativeEditorAuthoredAssetCatalogEntries(
    const CreativeEditorAuthoredAssetLibrary& library) {
  std::vector<cr::CreativeCatalogAsset> entries;
  entries.reserve(library.definitions.size());
  for (const cr::CreativeAuthoredAssetDefinition& definition :
       library.definitions) {
    cr::CreativeCatalogAsset entry;
    entry.objectKind = cr::CreativeObjectKind::PrefabInstance;
    entry.assetId = definition.assetId;
    entry.label = definition.label;
    entry.sourceBounds = definition.sourceBounds;
    entry.authoredComposite = true;
    entries.push_back(std::move(entry));
  }
  return entries;
}

CreativeEditorAuthoredAssetLoadReceipt loadCreativeEditorAuthoredAssetLibrary(
    CreativeEditorAuthoredAssetLibrary& library,
    const std::filesystem::path& creativeSaveRoot) {
  CreativeEditorAuthoredAssetLoadReceipt receipt;
  receipt.requested = true;
  library = {};
  library.root = creativeSaveRoot / "authored_assets";
  std::error_code error;
  std::filesystem::create_directories(library.root, error);
  if (error) {
    receipt.reasonCode = "creative_authored_asset_root_create_failed";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      library.root, kAuthoredAssetPackageId, kAuthoredAssetScenarioId);
  std::uint64_t maximumOrdinal = 0U;
  for (const iggy3d::ProductSaveCatalogEntry& entry :
       scanned.catalog.catalog.entries) {
    maximumOrdinal = std::max(
        maximumOrdinal,
        std::max(assetOrdinal(entry.worldId), assetOrdinal(entry.saveId)));
    const bool authoredDocument =
        !entry.deleted && entry.compatible && !entry.corrupt &&
        entry.contentKind == iggy3d::ProductSaveContentKind::CreativeDocument;
    if (!authoredDocument) {
      ++receipt.rejectedCount;
      continue;
    }
    const iggy3d::ProductCreativeSaveLoadResult loaded =
        iggy3d::loadCreativeDocumentSave({entry.path});
    const std::string assetId = loaded.worldId.empty()
                                    ? entry.worldId
                                    : loaded.worldId;
    const std::string label = loaded.saveTitle.empty()
                                  ? loaded.worldTitle
                                  : loaded.saveTitle;
    cr::CreativeAuthoredAssetLoadResult definition =
        cr::loadCreativeAuthoredAssetDefinition(loaded.document, assetId,
                                                label);
    if (!loaded.ok || !definition.accepted ||
        findCreativeEditorAuthoredAsset(library, assetId) != nullptr) {
      ++receipt.rejectedCount;
      continue;
    }
    library.nextDocumentId = std::max(
        library.nextDocumentId, loaded.document.id() + 1U);
    library.definitions.push_back(std::move(definition.definition));
    ++receipt.loadedCount;
  }
  library.nextAssetOrdinal = maximumOrdinal + 1U;
  while (findCreativeEditorAuthoredAsset(
             library, assetIdForOrdinal(library.nextAssetOrdinal)) != nullptr) {
    ++library.nextAssetOrdinal;
  }
  receipt.accepted = scanned.catalog.ok;
  receipt.reasonCode = receipt.accepted
                           ? "creative_authored_asset_library_ready"
                           : "creative_authored_asset_library_scan_failed";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetSaveReceipt
saveCreativeEditorSelectionAsAuthoredAsset(
    cr::CreativeAppState& appState,
    CreativeEditorAuthoredAssetLibrary& library,
    std::string_view requestedLabel) {
  CreativeEditorAuthoredAssetSaveReceipt receipt;
  receipt.requested = true;
  receipt.assetId = assetIdForOrdinal(library.nextAssetOrdinal);
  receipt.label = requestedLabel.empty()
                      ? defaultAssetLabel(appState, library.nextAssetOrdinal)
                      : std::string(requestedLabel);
  const std::vector<cr::CreativeObjectId> selection =
      selectedObjectIds(appState.facade.selectionState());
  cr::CreativeAuthoredAssetCaptureRequest captureRequest;
  captureRequest.sourceDocument = &appState.facade.document();
  captureRequest.selectedObjectIds = selection;
  captureRequest.assetId = receipt.assetId;
  captureRequest.label = receipt.label;
  captureRequest.definitionDocumentId = library.nextDocumentId;
  receipt.capture = cr::captureCreativeAuthoredAsset(captureRequest);
  if (!receipt.capture.accepted) {
    receipt.reasonCode = receipt.capture.reasonCode;
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  const std::string timestamp = iggy3d::productSaveTimestampNowUtc();
  iggy3d::ProductCreativeSaveWriteRequest writeRequest;
  writeRequest.saveRoot = library.root;
  writeRequest.saveIdHint = receipt.assetId;
  writeRequest.attemptToken = "authored_asset_write";
  writeRequest.document = &receipt.capture.storageDocument;
  writeRequest.packageId = kAuthoredAssetPackageId;
  writeRequest.scenarioId = kAuthoredAssetScenarioId;
  writeRequest.worldId = receipt.assetId;
  writeRequest.worldTitle = receipt.label;
  writeRequest.saveTitle = receipt.label;
  writeRequest.saveType = "creative-authored-asset";
  writeRequest.createdAtUtc = timestamp;
  writeRequest.savedAtUtc = timestamp;
  const iggy3d::ProductCreativeSaveWriteResult write =
      iggy3d::writeCreativeDocumentSaveDurably(writeRequest);
  receipt.durableWriteOk = write.ok;
  if (!write.ok) {
    receipt.reasonCode = write.reasonCode;
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  library.definitions.push_back(receipt.capture.definition);
  ++library.nextAssetOrdinal;
  ++library.nextDocumentId;
  receipt.accepted = true;
  receipt.reasonCode = "creative_authored_asset_saved";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

bool creativeEditorUsesAuthoredAsset(
    const cr::CreativeHotbarEntry& held,
    const CreativeEditorAuthoredAssetLibrary& library) noexcept {
  return held.kind == cr::CreativeHeldItemKind::Material &&
         held.objectKind == cr::CreativeObjectKind::PrefabInstance &&
         held.hasAssetBounds &&
         findCreativeEditorAuthoredAsset(
             library, cr::creativeHotbarAssetId(held)) != nullptr;
}

void processCreativeAuthoredAssetFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  CreativeAuthoredAssetStrokeState& stroke =
      editor.interaction.authoredAssetStroke;
  if (!creativeEditorUsesAuthoredAsset(held, editor.authoredAssets)) {
    finalizeCreativeAuthoredAssetStroke(
        appState, editor, "creative_authored_asset_non_authored_tool");
    return;
  }
  stroke.preview = placementPreview(editor, held);
  const cr::CreativeWorldGestureRepeatResult repeat =
      cr::stepCreativeWorldGestureRepeat(
          stroke.repeat, cr::makeCreativeWorldStrokeRepeatRequest(
                             actions, monotonicTimeNanoseconds));
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeAuthoredAssetStroke(
        appState, editor, "creative_authored_asset_released");
    editor.interaction.authoredAssetStroke.preview =
        placementPreview(editor, held);
    return;
  }
  if (!repeat.mutationDue) {
    return;
  }
  if (repeat.dueKind == cr::CreativeWorldGestureKind::Remove) {
    applyRemoval(appState, editor);
  } else if (repeat.dueKind == cr::CreativeWorldGestureKind::Place) {
    applyPlacement(appState, editor, held);
  }
  stroke.preview = placementPreview(editor, held);
}

void finalizeCreativeAuthoredAssetStroke(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode) {
  CreativeAuthoredAssetStrokeState& stroke =
      editor.interaction.authoredAssetStroke;
  if (!stroke.repeat.active && !stroke.transaction.active &&
      !stroke.preview.allowed && stroke.visited.count == 0U &&
      stroke.acceptedMutationCount == 0U && !stroke.capacityReached) {
    return;
  }
  StandaloneEditTransaction transaction = std::move(stroke.transaction);
  const bool changed = stroke.acceptedMutationCount > 0U;
  stroke = {};
  if (transaction.active) {
    static_cast<void>(completeEditTransaction(
        appState.history, std::move(transaction), appState.facade, changed,
        reasonCode));
  }
}

}  // namespace iggy3d_creative_app
