#include "EditorAssetLibrary.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <utility>

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

struct WorkspaceBuildResult {
  bool accepted = false;
  cr::CreativeAppState appState;
  std::string reasonCode = "creative_authored_asset_workspace_not_requested";
};

[[nodiscard]] char asciiLower(char value) noexcept {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] bool equalFolded(std::string_view left,
                               std::string_view right) noexcept {
  return left.size() == right.size() &&
         std::equal(
             left.begin(), left.end(), right.begin(),
             [](char a, char b) { return asciiLower(a) == asciiLower(b); });
}

[[nodiscard]] cr::CreativeAuthoredAssetDefinition*
mutableDefinition(CreativeEditorAuthoredAssetLibrary& library,
                  std::string_view assetId) noexcept {
  const auto found = std::find_if(
      library.definitions.begin(), library.definitions.end(),
      [assetId](const cr::CreativeAuthoredAssetDefinition& definition) {
        return definition.assetId == assetId;
      });
  return found == library.definitions.end() ? nullptr : &*found;
}

[[nodiscard]] bool
labelAvailable(const CreativeEditorAuthoredAssetLibrary& library,
               std::string_view label,
               std::string_view excludedAssetId = {}) noexcept {
  return std::none_of(
      library.definitions.begin(), library.definitions.end(),
      [label,
       excludedAssetId](const cr::CreativeAuthoredAssetDefinition& definition) {
        return definition.assetId != excludedAssetId &&
               equalFolded(definition.label, label);
      });
}

[[nodiscard]] std::string
duplicateLabel(const CreativeEditorAuthoredAssetLibrary& library,
               std::string_view sourceLabel) {
  for (std::size_t suffix = 1U; suffix < 10'000U; ++suffix) {
    const std::string candidate =
        std::string(sourceLabel) +
        (suffix == 1U ? " Copy" : " Copy " + std::to_string(suffix));
    const std::string sanitized =
        normalizeCreativeEditorAssetLibraryText(candidate);
    if (!sanitized.empty() && labelAvailable(library, sanitized)) {
      return sanitized;
    }
  }
  return {};
}

[[nodiscard]] WorkspaceBuildResult
materializeDefinition(const cr::CreativeAuthoredAssetDefinition& definition,
                      cr::CreativeDocumentId documentId) {
  WorkspaceBuildResult result;
  if (documentId == cr::kInvalidDocumentId ||
      !cr::isValidCreativeAuthoredAssetId(definition.assetId) ||
      definition.content.objects.empty()) {
    result.reasonCode = "creative_authored_asset_workspace_invalid";
    return result;
  }
  cr::CreativeDocument document =
      cr::CreativeDocument::create(definition.label);
  if (!document.assignId(documentId)) {
    result.reasonCode = "creative_authored_asset_workspace_id_invalid";
    return result;
  }
  cr::CreativeClipboardPasteRequest paste;
  paste.offset = definition.content.hasPlacementAnchor
                     ? cr::CreativeVec3{-definition.content.placementAnchor.x,
                                        -definition.content.placementAnchor.y,
                                        -definition.content.placementAnchor.z}
                     : cr::CreativeVec3{};
  paste.appendCopySuffix = false;
  paste.externalParentPolicy =
      cr::CreativeClipboardExternalParentPolicy::Detach;
  const cr::CreativeClipboardPasteReceipt pasted =
      cr::pasteCreativeClipboardAtomically(document, definition.content, paste);
  if (!pasted.accepted || pasted.pastedObjectCount == 0U) {
    result.reasonCode = pasted.reasonCode;
    return result;
  }
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      result.appState.facade.installDocument(std::move(document));
  if (!installed.accepted) {
    result.reasonCode = installed.reasonCode;
    return result;
  }
  result.accepted = true;
  result.reasonCode = "creative_authored_asset_workspace_ready";
  return result;
}

[[nodiscard]] std::vector<cr::CreativeObjectId>
workspaceRoots(const cr::CreativeDocument& document) {
  std::vector<cr::CreativeObjectId> roots;
  roots.reserve(document.objectCount());
  for (const cr::CreativeObject& object : document.objects()) {
    if (!object.parentId.has_value()) {
      roots.push_back(object.id);
    }
  }
  return roots;
}

[[nodiscard]] cr::CreativeAuthoredAssetCaptureResult
captureWorkspace(const cr::CreativeDocument& document, std::string_view assetId,
                 std::string_view label,
                 cr::CreativeDocumentId definitionDocumentId) {
  cr::CreativeAuthoredAssetCaptureResult rejected;
  rejected.requested = true;
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.kind == cr::CreativeObjectKind::PrefabInstance &&
        object.assetId == assetId) {
      rejected.status = cr::CreativeAuthoredAssetStatus::InvalidDefinition;
      rejected.reasonCode = "creative_authored_asset_recursive_definition";
      return rejected;
    }
  }
  const std::vector<cr::CreativeObjectId> roots = workspaceRoots(document);
  cr::CreativeAuthoredAssetCaptureRequest request;
  request.sourceDocument = &document;
  request.selectedObjectIds = roots;
  request.assetId = assetId;
  request.label = label;
  request.definitionDocumentId = definitionDocumentId;
  return cr::captureCreativeAuthoredAsset(request);
}

void advanceDocumentIds(CreativeEditorAuthoredAssetLibrary& library) noexcept {
  if (library.nextDocumentId <=
      std::numeric_limits<cr::CreativeDocumentId>::max() - 2U) {
    library.nextDocumentId += 2U;
  }
}

[[nodiscard]] CreativeEditorDocumentTransientState
takeDocumentTransientState(CreativeEditorState& editor) {
  CreativeEditorDocumentTransientState state;
  state.synchronizedHeldItemKind =
      editor.interaction.synchronizedHeldItemKind;
  state.target = std::move(editor.interaction.target);
  state.placementFeedback = std::move(editor.interaction.placementFeedback);
  state.materialBrushPivot = std::move(editor.interaction.materialBrushPivot);
  state.materialStroke = std::move(editor.interaction.materialStroke);
  state.structuralSpan = std::move(editor.interaction.structuralSpan);
  state.structuralSpanEdit =
      std::move(editor.interaction.structuralSpanEdit);
  state.assetScatter = std::move(editor.interaction.assetScatter);
  state.authoredAssetStroke = std::move(editor.interaction.authoredAssetStroke);
  state.connectedFill = std::move(editor.interaction.connectedFill);
  state.surfaceExtrude = std::move(editor.interaction.surfaceExtrude);
  state.roomPlacement = std::move(editor.interaction.roomPlacement);
  state.moveTargetId = editor.interaction.moveTargetId;
  state.groupFocus = std::move(editor.groupFocus);
  state.pattern = std::move(editor.pattern);
  state.assetReplacement = std::move(editor.assetReplacement);
  state.transform = std::move(editor.transform);
  state.terrain = std::move(editor.terrain);
  state.terrainPaint = std::move(editor.terrainPaint);
  state.volume = std::move(editor.volume);
  state.logicLinks = std::move(editor.logicLinks);
  state.movingPlatformPreview = std::move(editor.movingPlatformPreview);

  editor.interaction.synchronizedHeldItemKind =
      cr::CreativeHeldItemKind::Count;
  editor.interaction.target = {};
  editor.interaction.placementFeedback = {};
  editor.interaction.materialBrushPivot = {};
  editor.interaction.materialStroke = {};
  editor.interaction.structuralSpan = {};
  editor.interaction.structuralSpanEdit = {};
  editor.interaction.assetScatter = {};
  editor.interaction.authoredAssetStroke = {};
  editor.interaction.connectedFill = {};
  editor.interaction.surfaceExtrude = {};
  editor.interaction.roomPlacement = {};
  editor.interaction.movingPlatformPathEdit = {};
  editor.interaction.moveTargetId = cr::kInvalidObjectId;
  editor.groupFocus = {};
  editor.pattern = {};
  editor.assetReplacement = {};
  editor.transform = {};
  editor.terrain = {};
  editor.terrainPaint = {};
  editor.volume = {};
  editor.logicLinks = {};
  editor.movingPlatformPreview = {};
  return state;
}

void restoreDocumentTransientState(CreativeEditorState& editor,
                                   CreativeEditorDocumentTransientState state) {
  editor.interaction.synchronizedHeldItemKind =
      state.synchronizedHeldItemKind;
  editor.interaction.target = std::move(state.target);
  editor.interaction.placementFeedback = std::move(state.placementFeedback);
  editor.interaction.materialBrushPivot = std::move(state.materialBrushPivot);
  editor.interaction.materialStroke = std::move(state.materialStroke);
  editor.interaction.structuralSpan = std::move(state.structuralSpan);
  editor.interaction.structuralSpanEdit =
      std::move(state.structuralSpanEdit);
  editor.interaction.assetScatter = std::move(state.assetScatter);
  editor.interaction.authoredAssetStroke = std::move(state.authoredAssetStroke);
  editor.interaction.connectedFill = std::move(state.connectedFill);
  editor.interaction.surfaceExtrude = std::move(state.surfaceExtrude);
  editor.interaction.roomPlacement = std::move(state.roomPlacement);
  editor.interaction.moveTargetId = state.moveTargetId;
  editor.groupFocus = std::move(state.groupFocus);
  editor.pattern = std::move(state.pattern);
  editor.assetReplacement = std::move(state.assetReplacement);
  editor.transform = std::move(state.transform);
  editor.terrain = std::move(state.terrain);
  editor.terrainPaint = std::move(state.terrainPaint);
  editor.volume = std::move(state.volume);
  editor.logicLinks = std::move(state.logicLinks);
  editor.movingPlatformPreview = std::move(state.movingPlatformPreview);
}

void restoreMapCamera(CreativeEditorState& editor) noexcept {
  const CreativeEditorAuthoredAssetEditSession& session = editor.assetEdit;
  editor.flyPos = session.mapFlyPosition;
  editor.yawDegrees = session.mapYawDegrees;
  editor.pitchDegrees = session.mapPitchDegrees;
}

void frameAssetWorkspace(CreativeEditorState& editor,
                         const cr::CreativeBounds& sourceBounds) noexcept {
  const cr::CreativeBoundsMetrics metrics =
      cr::measureCreativeBounds(sourceBounds);
  if (!metrics.valid) {
    editor.flyPos = {0.0F, 4.0F, 8.0F};
    editor.yawDegrees = 0.0F;
    editor.pitchDegrees = -20.0F;
    return;
  }
  const double longest =
      std::max({metrics.size.x, metrics.size.y, metrics.size.z, 1.0});
  editor.flyPos = {static_cast<float>(metrics.center.x),
                   static_cast<float>(metrics.center.y + longest * 1.25),
                   static_cast<float>(metrics.center.z + longest * 2.75)};
  editor.yawDegrees = 0.0F;
  editor.pitchDegrees = -22.0F;
}

void refreshEditDirtyState(CreativeEditorState& editor) {
  CreativeEditorAuthoredAssetEditSession& session = editor.assetEdit;
  if (!session.active) {
    return;
  }
  const cr::CreativeDocument& document = session.workspace.facade.document();
  if (session.observedDocumentRevision == document.revision()) {
    return;
  }
  session.observedDocumentRevision = document.revision();
  const cr::CreativeAuthoredAssetCaptureResult capture =
      captureWorkspace(document, session.assetId, session.label,
                       editor.authoredAssets.nextDocumentId + 1U);
  const cr::CreativeAuthoredAssetFingerprint fingerprint =
      capture.accepted
          ? cr::fingerprintCreativeAuthoredAssetDefinition(capture.definition)
          : cr::CreativeAuthoredAssetFingerprint{};
  session.dirty = !fingerprint.valid || !session.initialFingerprint.valid ||
                  fingerprint.value != session.initialFingerprint.value;
}

} // namespace

std::string normalizeCreativeEditorAssetLibraryText(std::string_view text) {
  std::string result;
  result.reserve(std::min(text.size(), kCreativeEditorAssetLabelCapacity));
  bool previousSpace = true;
  for (char character : text) {
    const unsigned char byte = static_cast<unsigned char>(character);
    if (byte < 32U || byte > 126U) {
      continue;
    }
    const bool space = std::isspace(byte) != 0;
    if (space) {
      if (!previousSpace && result.size() < kCreativeEditorAssetLabelCapacity) {
        result.push_back(' ');
      }
    } else if (result.size() < kCreativeEditorAssetLabelCapacity) {
      result.push_back(character);
    }
    previousSpace = space;
  }
  while (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }
  return result;
}

void refreshCreativeEditorAuthoredAssetEditDirtyState(
    CreativeEditorState& editor) {
  refreshEditDirtyState(editor);
}

std::string_view toString(CreativeEditorAssetLibraryAction action) noexcept {
  switch (action) {
    case CreativeEditorAssetLibraryAction::Edit:
      return "EDIT SOURCE";
    case CreativeEditorAssetLibraryAction::Rename:
      return "RENAME";
    case CreativeEditorAssetLibraryAction::Duplicate:
      return "DUPLICATE";
    case CreativeEditorAssetLibraryAction::Delete:
      return "DELETE";
    case CreativeEditorAssetLibraryAction::Count:
      break;
  }
  return "UNKNOWN";
}

std::string_view toString(CreativeEditorAssetEditMenuAction action) noexcept {
  switch (action) {
    case CreativeEditorAssetEditMenuAction::ContinueEditing:
      return "CONTINUE EDITING";
    case CreativeEditorAssetEditMenuAction::SaveAndExit:
      return "SAVE AND EXIT";
    case CreativeEditorAssetEditMenuAction::DiscardAndExit:
      return "DISCARD AND EXIT";
    case CreativeEditorAssetEditMenuAction::Count:
      break;
  }
  return "UNKNOWN";
}

CreativeEditorAuthoredAssetReferenceSummary
summarizeCreativeEditorAuthoredAssetReferences(
    const cr::CreativeDocument& mapDocument,
    const CreativeEditorAuthoredAssetLibrary& library,
    std::string_view assetId) noexcept {
  CreativeEditorAuthoredAssetReferenceSummary summary;
  for (const cr::CreativeObject& object : mapDocument.objects()) {
    summary.mapInstanceCount +=
        object.kind == cr::CreativeObjectKind::PrefabInstance &&
                object.assetId == assetId
            ? 1U
            : 0U;
  }
  for (const cr::CreativeAuthoredAssetDefinition& definition :
       library.definitions) {
    if (definition.assetId == assetId) {
      continue;
    }
    summary.authoredAssetDependencyCount += static_cast<std::size_t>(
        std::count_if(definition.content.objects.begin(),
                      definition.content.objects.end(),
                      [assetId](const cr::CreativeObject& object) {
                        return object.kind ==
                                   cr::CreativeObjectKind::PrefabInstance &&
                               object.assetId == assetId;
                      }));
  }
  return summary;
}

CreativeEditorAuthoredAssetMutationReceipt
renameCreativeEditorAuthoredAsset(CreativeEditorAuthoredAssetLibrary& library,
                                  std::string_view assetId,
                                  std::string_view requestedLabel) {
  CreativeEditorAuthoredAssetMutationReceipt receipt;
  receipt.requested = true;
  receipt.assetId = std::string(assetId);
  receipt.label = normalizeCreativeEditorAssetLibraryText(requestedLabel);
  cr::CreativeAuthoredAssetDefinition* definition =
      mutableDefinition(library, assetId);
  if (definition == nullptr) {
    receipt.reasonCode = "creative_asset_library_source_missing";
    return receipt;
  }
  if (receipt.label.empty() ||
      !labelAvailable(library, receipt.label, assetId)) {
    receipt.reasonCode = "creative_asset_library_label_invalid";
    return receipt;
  }
  if (definition->label == receipt.label) {
    receipt.reasonCode = "creative_asset_library_label_unchanged";
    return receipt;
  }
  WorkspaceBuildResult workspace =
      materializeDefinition(*definition, library.nextDocumentId);
  const cr::CreativeAuthoredAssetCaptureResult capture =
      workspace.accepted
          ? captureWorkspace(workspace.appState.facade.document(), assetId,
                             receipt.label, library.nextDocumentId + 1U)
          : cr::CreativeAuthoredAssetCaptureResult{};
  if (!workspace.accepted || !capture.accepted) {
    receipt.reasonCode = workspace.accepted ? std::string(capture.reasonCode)
                                            : workspace.reasonCode;
    return receipt;
  }
  const CreativeEditorAuthoredAssetDurableWriteReceipt write =
      writeCreativeEditorAuthoredAssetDocument(library, assetId, receipt.label,
                                               capture.storageDocument, true);
  receipt.durableWriteOk = write.accepted;
  if (!write.accepted) {
    receipt.reasonCode = write.reasonCode;
    return receipt;
  }
  *definition = capture.definition;
  advanceDocumentIds(library);
  receipt.accepted = true;
  receipt.reasonCode = "creative_asset_library_renamed";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetMutationReceipt duplicateCreativeEditorAuthoredAsset(
    CreativeEditorAuthoredAssetLibrary& library, std::string_view assetId) {
  CreativeEditorAuthoredAssetMutationReceipt receipt;
  receipt.requested = true;
  const cr::CreativeAuthoredAssetDefinition* source =
      findCreativeEditorAuthoredAsset(library, assetId);
  if (source == nullptr) {
    receipt.reasonCode = "creative_asset_library_source_missing";
    return receipt;
  }
  receipt.assetId = nextCreativeEditorAuthoredAssetId(library);
  receipt.label = duplicateLabel(library, source->label);
  if (receipt.label.empty()) {
    receipt.reasonCode = "creative_asset_library_label_capacity_exceeded";
    return receipt;
  }
  WorkspaceBuildResult workspace =
      materializeDefinition(*source, library.nextDocumentId);
  const cr::CreativeAuthoredAssetCaptureResult capture =
      workspace.accepted
          ? captureWorkspace(workspace.appState.facade.document(),
                             receipt.assetId, receipt.label,
                             library.nextDocumentId + 1U)
          : cr::CreativeAuthoredAssetCaptureResult{};
  if (!workspace.accepted || !capture.accepted) {
    receipt.reasonCode = workspace.accepted ? std::string(capture.reasonCode)
                                            : workspace.reasonCode;
    return receipt;
  }
  const CreativeEditorAuthoredAssetDurableWriteReceipt write =
      writeCreativeEditorAuthoredAssetDocument(library, receipt.assetId,
                                               receipt.label,
                                               capture.storageDocument, false);
  receipt.durableWriteOk = write.accepted;
  if (!write.accepted) {
    receipt.reasonCode = write.reasonCode;
    return receipt;
  }
  library.definitions.push_back(capture.definition);
  advanceCreativeEditorAuthoredAssetOrdinal(library);
  advanceDocumentIds(library);
  receipt.accepted = true;
  receipt.reasonCode = "creative_asset_library_duplicated";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetMutationReceipt
deleteCreativeEditorAuthoredAsset(const cr::CreativeDocument& mapDocument,
                                  CreativeEditorAuthoredAssetLibrary& library,
                                  std::string_view assetId) {
  CreativeEditorAuthoredAssetMutationReceipt receipt;
  receipt.requested = true;
  receipt.assetId = std::string(assetId);
  const cr::CreativeAuthoredAssetDefinition* definition =
      findCreativeEditorAuthoredAsset(library, assetId);
  if (definition == nullptr) {
    receipt.reasonCode = "creative_asset_library_source_missing";
    return receipt;
  }
  receipt.label = definition->label;
  receipt.references = summarizeCreativeEditorAuthoredAssetReferences(
      mapDocument, library, assetId);
  if (receipt.references.total() > 0U) {
    receipt.reasonCode = "creative_asset_library_delete_referenced";
    return receipt;
  }
  const iggy3d::ProductSaveSoftDeleteResult removed =
      iggy3d::softDeleteProductSave({library.root, std::string(assetId)});
  receipt.durableWriteOk = removed.ok;
  if (!removed.ok) {
    receipt.reasonCode = removed.reasonCode;
    return receipt;
  }
  std::erase_if(library.definitions,
                [assetId](const cr::CreativeAuthoredAssetDefinition& item) {
                  return item.assetId == assetId;
                });
  receipt.accepted = true;
  receipt.reasonCode = "creative_asset_library_deleted";
  library.statusLabel = receipt.reasonCode;
  return receipt;
}

CreativeEditorAuthoredAssetMutationReceipt
beginCreativeEditorAuthoredAssetEdit(CreativeEditorState& editor,
                                     std::string_view assetId) {
  CreativeEditorAuthoredAssetMutationReceipt receipt;
  receipt.requested = true;
  receipt.assetId = std::string(assetId);
  const cr::CreativeAuthoredAssetDefinition* definition =
      findCreativeEditorAuthoredAsset(editor.authoredAssets, assetId);
  if (editor.assetEdit.active || definition == nullptr) {
    receipt.reasonCode = editor.assetEdit.active
                             ? "creative_authored_asset_edit_already_active"
                             : "creative_asset_library_source_missing";
    return receipt;
  }
  WorkspaceBuildResult workspace =
      materializeDefinition(*definition, editor.authoredAssets.nextDocumentId);
  if (!workspace.accepted) {
    receipt.reasonCode = workspace.reasonCode;
    return receipt;
  }
  CreativeEditorAuthoredAssetEditSession session;
  session.active = true;
  session.assetId = definition->assetId;
  session.label = definition->label;
  session.workspace = std::move(workspace.appState);
  session.initialFingerprint =
      cr::fingerprintCreativeAuthoredAssetDefinition(*definition);
  session.observedDocumentRevision =
      session.workspace.facade.document().revision();
  session.mapFlyPosition = editor.flyPos;
  session.mapYawDegrees = editor.yawDegrees;
  session.mapPitchDegrees = editor.pitchDegrees;
  session.mapDocumentState = takeDocumentTransientState(editor);
  session.statusLabel = "creative_authored_asset_edit_ready";
  editor.assetEdit = std::move(session);
  frameAssetWorkspace(editor, definition->sourceBounds);
  receipt.accepted = true;
  receipt.label = definition->label;
  receipt.reasonCode = "creative_authored_asset_edit_ready";
  return receipt;
}

CreativeEditorAuthoredAssetMutationReceipt
saveCreativeEditorAuthoredAssetEdit(CreativeEditorState& editor) {
  CreativeEditorAuthoredAssetMutationReceipt receipt;
  receipt.requested = true;
  CreativeEditorAuthoredAssetEditSession& session = editor.assetEdit;
  receipt.assetId = session.assetId;
  receipt.label = session.label;
  if (!session.active) {
    receipt.reasonCode = "creative_authored_asset_edit_inactive";
    return receipt;
  }
  cr::CreativeAuthoredAssetDefinition* definition =
      mutableDefinition(editor.authoredAssets, session.assetId);
  if (definition == nullptr) {
    receipt.reasonCode = "creative_asset_library_source_missing";
    session.statusLabel = receipt.reasonCode;
    return receipt;
  }
  finalizeCreativeEditorContinuousGestures(session.workspace, editor,
                                           "creative_authored_asset_edit_save");
  const cr::CreativeAuthoredAssetCaptureResult capture = captureWorkspace(
      session.workspace.facade.document(), session.assetId, session.label,
      editor.authoredAssets.nextDocumentId + 1U);
  if (!capture.accepted) {
    receipt.reasonCode = capture.reasonCode;
    session.statusLabel = receipt.reasonCode;
    return receipt;
  }
  const CreativeEditorAuthoredAssetDurableWriteReceipt write =
      writeCreativeEditorAuthoredAssetDocument(editor.authoredAssets,
                                               session.assetId, session.label,
                                               capture.storageDocument, true);
  receipt.durableWriteOk = write.accepted;
  if (!write.accepted) {
    receipt.reasonCode = write.reasonCode;
    session.statusLabel = receipt.reasonCode;
    return receipt;
  }
  *definition = capture.definition;
  advanceDocumentIds(editor.authoredAssets);
  restoreMapCamera(editor);
  CreativeEditorDocumentTransientState mapDocumentState =
      std::move(session.mapDocumentState);
  editor.assetEdit = {};
  restoreDocumentTransientState(editor, std::move(mapDocumentState));
  static_cast<void>(
      refreshCreativeEditorAuthoredAssetReferences(editor, *definition));
  editor.assetLibrary.statusLabel = "creative_authored_asset_edit_saved";
  receipt.accepted = true;
  receipt.reasonCode = "creative_authored_asset_edit_saved";
  return receipt;
}

bool cancelCreativeEditorAuthoredAssetEdit(
    CreativeEditorState& editor, std::string_view reasonCode) {
  if (!editor.assetEdit.active) {
    return false;
  }
  restoreMapCamera(editor);
  CreativeEditorDocumentTransientState mapDocumentState =
      std::move(editor.assetEdit.mapDocumentState);
  editor.assetEdit = {};
  restoreDocumentTransientState(editor, std::move(mapDocumentState));
  editor.assetLibrary.statusLabel = std::string(reasonCode);
  return true;
}

cr::CreativeAppState&
activeCreativeEditorAppState(CreativeEditorState& editor,
                             cr::CreativeAppState& mapAppState) noexcept {
  return editor.assetEdit.active ? editor.assetEdit.workspace : mapAppState;
}

const cr::CreativeAppState&
activeCreativeEditorAppState(const CreativeEditorState& editor,
                             const cr::CreativeAppState& mapAppState) noexcept {
  return editor.assetEdit.active ? editor.assetEdit.workspace : mapAppState;
}

} // namespace iggy3d_creative_app
