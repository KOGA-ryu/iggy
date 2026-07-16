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

[[nodiscard]] cr::CreativeCatalogAsset authoredCatalogAsset(
    const cr::CreativeAuthoredAssetDefinition& definition) {
  cr::CreativeCatalogAsset asset;
  asset.objectKind = cr::CreativeObjectKind::PrefabInstance;
  asset.assetId = definition.assetId;
  asset.label = definition.label;
  asset.sourceBounds = definition.sourceBounds;
  asset.authoredComposite = true;
  return asset;
}

[[nodiscard]] iggy3d::ProductCreativeSaveWriteResult
writeAuthoredAssetDocument(
    const CreativeEditorAuthoredAssetLibrary& library,
    std::string_view assetId,
    std::string_view label,
    const cr::CreativeDocument& document,
    bool replacing) {
  const std::string timestamp = iggy3d::productSaveTimestampNowUtc();
  iggy3d::ProductCreativeSaveWriteRequest request;
  request.saveRoot = library.root;
  request.saveIdHint = assetId;
  request.attemptToken = replacing ? "authored_asset_update"
                                   : "authored_asset_write";
  request.document = &document;
  request.packageId = kAuthoredAssetPackageId;
  request.scenarioId = kAuthoredAssetScenarioId;
  request.worldId = assetId;
  request.worldTitle = label;
  request.saveTitle = label;
  request.saveType = "creative-authored-asset";
  request.createdAtUtc = replacing ? std::string{} : timestamp;
  request.savedAtUtc = timestamp;
  return iggy3d::writeCreativeDocumentSaveDurably(request);
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
  request.instanceTransform = admission.plan.transform;
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

std::string nextCreativeEditorAuthoredAssetId(
    const CreativeEditorAuthoredAssetLibrary& library) {
  std::uint64_t ordinal = library.nextAssetOrdinal;
  std::string assetId = assetIdForOrdinal(ordinal);
  while (findCreativeEditorAuthoredAsset(library, assetId) != nullptr) {
    assetId = assetIdForOrdinal(++ordinal);
  }
  return assetId;
}

void advanceCreativeEditorAuthoredAssetOrdinal(
    CreativeEditorAuthoredAssetLibrary& library) noexcept {
  ++library.nextAssetOrdinal;
  while (findCreativeEditorAuthoredAsset(
             library, assetIdForOrdinal(library.nextAssetOrdinal)) != nullptr) {
    ++library.nextAssetOrdinal;
  }
}

std::vector<cr::CreativeCatalogAsset> creativeEditorAuthoredAssetCatalogEntries(
    const CreativeEditorAuthoredAssetLibrary& library) {
  std::vector<cr::CreativeCatalogAsset> entries;
  entries.reserve(library.definitions.size());
  for (const cr::CreativeAuthoredAssetDefinition& definition :
       library.definitions) {
    entries.push_back(authoredCatalogAsset(definition));
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
  receipt.assetId = nextCreativeEditorAuthoredAssetId(library);
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

  const iggy3d::ProductCreativeSaveWriteResult write =
      writeAuthoredAssetDocument(library, receipt.assetId, receipt.label,
                                 receipt.capture.storageDocument, false);
  receipt.durableWriteOk = write.ok;
  if (!write.ok) {
    receipt.reasonCode = write.reasonCode;
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  library.definitions.push_back(receipt.capture.definition);
  advanceCreativeEditorAuthoredAssetOrdinal(library);
  ++library.nextDocumentId;
  receipt.accepted = true;
  receipt.reasonCode = "creative_authored_asset_saved";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetUpdateReceipt
updateCreativeEditorAuthoredAssetFromInstance(
    cr::CreativeAppState& appState,
    CreativeEditorAuthoredAssetLibrary& library,
    cr::CreativeObjectId instanceRootObjectId) {
  CreativeEditorAuthoredAssetUpdateReceipt receipt;
  receipt.requested = true;
  receipt.instanceRootObjectId = instanceRootObjectId;
  const cr::CreativeObject* instance =
      appState.facade.findObject(instanceRootObjectId);
  if (instance == nullptr ||
      instance->kind != cr::CreativeObjectKind::PrefabInstance) {
    receipt.reasonCode = "creative_authored_asset_update_instance_missing";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }
  if (instance->locked) {
    receipt.reasonCode = "creative_authored_asset_update_instance_locked";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }
  receipt.assetId = instance->assetId;
  const cr::CreativeAuthoredAssetDefinition* existing =
      findCreativeEditorAuthoredAsset(library, receipt.assetId);
  if (existing == nullptr) {
    receipt.reasonCode = "creative_authored_asset_update_source_missing";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  cr::CreativeAuthoredAssetInstanceCaptureRequest captureRequest;
  captureRequest.sourceDocument = &appState.facade.document();
  captureRequest.existingDefinition = existing;
  captureRequest.instanceRootObjectId = instanceRootObjectId;
  captureRequest.definitionDocumentId = library.nextDocumentId;
  receipt.capture =
      cr::captureCreativeAuthoredAssetInstance(captureRequest);
  if (!receipt.capture.accepted) {
    receipt.reasonCode = receipt.capture.reasonCode;
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  const iggy3d::ProductCreativeSaveWriteResult write =
      writeAuthoredAssetDocument(
          library, receipt.assetId, receipt.capture.definition.label,
          receipt.capture.storageDocument, true);
  receipt.durableWriteOk = write.ok;
  if (!write.ok) {
    receipt.reasonCode = write.reasonCode;
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  const auto definition = std::find_if(
      library.definitions.begin(), library.definitions.end(),
      [&receipt](const cr::CreativeAuthoredAssetDefinition& candidate) {
        return candidate.assetId == receipt.assetId;
      });
  if (definition == library.definitions.end()) {
    receipt.reasonCode = "creative_authored_asset_update_source_missing";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }
  receipt.provenance =
      appState.facade.acknowledgeAuthoredAssetInstanceSource(
          receipt.capture.definition, instanceRootObjectId);
  if (!receipt.provenance.committed ||
      !cr::documentMutationSucceeded(receipt.provenance.status)) {
    receipt.reasonCode = "creative_authored_asset_update_provenance_rejected";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }
  *definition = receipt.capture.definition;
  ++library.nextDocumentId;
  receipt.accepted = true;
  receipt.reasonCode = "creative_authored_asset_updated";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetInstanceRefreshReceipt
refreshCreativeEditorAuthoredAssetInstances(
    cr::CreativeAppState& appState,
    CreativeEditorAuthoredAssetLibrary& library,
    cr::CreativeObjectId instanceRootObjectId,
    cr::CreativeAuthoredAssetRefreshMode mode) {
  CreativeEditorAuthoredAssetInstanceRefreshReceipt receipt;
  receipt.requested = true;
  receipt.instanceRootObjectId = instanceRootObjectId;
  const cr::CreativeObject* instance =
      appState.facade.findObject(instanceRootObjectId);
  if (instance == nullptr ||
      instance->kind != cr::CreativeObjectKind::PrefabInstance) {
    receipt.reasonCode = "creative_authored_asset_refresh_instance_missing";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }
  const cr::CreativeAuthoredAssetDefinition* definition =
      findCreativeEditorAuthoredAsset(library, instance->assetId);
  if (definition == nullptr) {
    receipt.reasonCode = "creative_authored_asset_refresh_source_missing";
    library.statusLabel = receipt.reasonCode;
    return receipt;
  }

  std::string_view transactionSource =
      "creative_authored_asset_force_refresh_all";
  switch (mode) {
    case cr::CreativeAuthoredAssetRefreshMode::SelectedInstance:
      transactionSource = "creative_authored_asset_refresh_instance";
      break;
    case cr::CreativeAuthoredAssetRefreshMode::SafeInstances:
      transactionSource = "creative_authored_asset_refresh_safe_instances";
      break;
    case cr::CreativeAuthoredAssetRefreshMode::ForceAll:
      break;
  }
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, transactionSource);
  receipt.refresh = appState.facade.refreshAuthoredAssetInstances(
      *definition, instanceRootObjectId, mode);
  receipt.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.refresh.accepted && receipt.refresh.changed,
      receipt.refresh.reasonCode);
  receipt.accepted = receipt.refresh.accepted && receipt.refresh.changed &&
                     receipt.history.accepted;
  if (receipt.accepted) {
    receipt.reasonCode = "creative_authored_asset_instances_refreshed";
  } else if (!receipt.refresh.accepted) {
    receipt.reasonCode = receipt.refresh.reasonCode;
  } else {
    receipt.reasonCode = receipt.history.reasonCode;
  }
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetReferenceRefreshReceipt
refreshCreativeEditorAuthoredAssetReferences(
    CreativeEditorState& editor,
    const cr::CreativeAuthoredAssetDefinition& definition) {
  CreativeEditorAuthoredAssetReferenceRefreshReceipt receipt;
  const cr::CreativeCatalogAsset asset = authoredCatalogAsset(definition);
  receipt.catalogUpdated =
      cr::updateCreativeCatalogAsset(editor.catalog.model, asset) ||
      cr::appendCreativeCatalogAsset(editor.catalog.model, asset);
  for (cr::CreativeHotbarEntry& entry :
       editor.interaction.hotbar.entries) {
    if (cr::creativeHotbarAssetId(entry) != definition.assetId) {
      continue;
    }
    if (cr::setCreativeHotbarAsset(entry, definition.assetId,
                                   definition.sourceBounds)) {
      ++receipt.hotbarSlotCount;
    }
  }
  receipt.accepted = receipt.catalogUpdated;
  return receipt;
}

CreativeEditorAuthoredAssetDurableWriteReceipt
writeCreativeEditorAuthoredAssetDocument(
    const CreativeEditorAuthoredAssetLibrary& library, std::string_view assetId,
    std::string_view label, const cr::CreativeDocument& document,
    bool replacing) {
  CreativeEditorAuthoredAssetDurableWriteReceipt receipt;
  const iggy3d::ProductCreativeSaveWriteResult write =
      writeAuthoredAssetDocument(library, assetId, label, document, replacing);
  receipt.accepted = write.ok;
  receipt.reasonCode = write.reasonCode;
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
