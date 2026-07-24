#include "EditorDesktopCommandsInternal.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorPersistence.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

namespace {

bool makeReplacementRevisionDistinct(
    creative::CreativeDocument& replacement,
    std::uint64_t previousRevision) {
  if (replacement.revision() != previousRevision) {
    return true;
  }
  const std::string name{replacement.name()};
  return replacement.rename(name + " [regenerating]") &&
         replacement.rename(name) &&
         replacement.revision() != previousRevision;
}

bool installBuiltInMapTemplates(
    CreativeEditorWorldLayoutState& state,
    const std::filesystem::path& saveRoot,
    const creative::CreativeMapTemplateResult& map,
    std::string& reasonCode) {
  if (state.buildingTemplates.root.empty()) {
    const CreativeEditorWorldLayoutBuildingTemplateLoadReceipt loaded =
        loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
            state.buildingTemplates, saveRoot);
    if (!loaded.accepted) {
      reasonCode = loaded.reasonCode;
      return false;
    }
  }
  for (const creative::CreativeWorldLayoutBuildingTemplate& source :
       map.buildingTemplates) {
    const CreativeEditorWorldLayoutBuildingTemplateInstallReceipt installed =
        installCreativeEditorBuiltInWorldLayoutBuildingTemplate(
            state.buildingTemplates, source);
    if (!installed.accepted) {
      reasonCode = installed.reasonCode;
      return false;
    }
  }
  return true;
}

void regenerateMapTemplate(
    const CreativeDesktopMapTemplatePayload& payload,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  CreativeEditorWorldLayoutState& worldLayout = editor.worldLayout;
  if (&activeCreativeEditorAppState(editor, appState) != &appState) {
    result.message = "finish asset editing before regenerating the map";
    return;
  }
  if (!creative::isCreativeMapTemplateId(payload.templateId)) {
    result.message = "map regeneration: unknown template";
    return;
  }
  if (worldLayout.generatedRevision != worldLayout.revision) {
    result.message =
        "generate or undo pending layout edits before regenerating the map";
    return;
  }
  if (appState.history.maxDepth == 0U) {
    result.message = "map regeneration requires enabled undo history";
    return;
  }

  creative::CreativeMapTemplateResult map = creative::buildCreativeMapTemplate(
      payload.templateId, appState.facade.document().id());
  if (!map.accepted || !map.worldLayoutPresent) {
    result.message = "map regeneration failed: " + map.reasonCode;
    return;
  }
  std::string templateReason;
  if (!installBuiltInMapTemplates(worldLayout, context.saveRoot, map,
                                  templateReason)) {
    result.message = "map template update failed: " + templateReason;
    return;
  }

  creative::CreativeHistorySidecar beforeSidecar;
  if (!encodeCreativeEditorWorldLayoutHistorySidecar(
          worldLayout.generatedBaseline, beforeSidecar)) {
    result.message = "map regeneration history snapshot is invalid";
    return;
  }
  const creative::CreativeDocument beforeDocument = appState.facade.document();
  creative::CreativeDocumentHistoryTransaction transaction =
      beginEditTransaction(appState.facade,
                           "desktop_regenerate_map_template",
                           std::move(beforeSidecar));
  if (!transaction.active ||
      !makeReplacementRevisionDistinct(map.document,
                                       beforeDocument.revision())) {
    result.message = "map regeneration transaction could not start";
    return;
  }

  const std::uint64_t objectCount = map.document.objectCount();
  const creative::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(map.document));
  if (!installed.accepted) {
    cancelEditTransaction(transaction);
    result.message = "map regeneration document install failed";
    return;
  }
  const creative::CreativeHistoryRecordReceipt recorded =
      completeEditTransaction(appState.history, std::move(transaction),
                              appState.facade, true,
                              "desktop_regenerate_map_template");
  if (!recorded.accepted || !recorded.recorded) {
    static_cast<void>(appState.facade.installDocument(beforeDocument));
    result.message = "map regeneration history record failed";
    return;
  }

  resetCreativeEditorForDocumentReplacement(
      editor, appState.facade.document().id());
  installCreativeEditorWorldLayout(editor.worldLayout,
                                   std::move(map.worldLayout));
  editor.worldLayout.statusMessage =
      "map regenerated in 3D; save to keep it";
  result.accepted = true;
  result.changed = true;
  result.documentReplaced = true;
  result.sceneChanged = true;
  result.worldLayoutChanged = true;
  result.affectedObjectCount = objectCount;
  result.message = "regenerated " + payload.templateId +
                   "; Undo restores the previous document";
}

}  // namespace

bool dispatchCreativeDesktopDocumentCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  switch (command.id) {
    case CreativeDesktopCommandId::NewDocument: {
      const creative::CreativeFacadeDocumentInstallReceipt replaced =
          clearToBlankScene(appState);
      if (replaced.accepted) {
        clearEditHistory(appState.history, "desktop_new");
        resetCreativeEditorForDocumentReplacement(
            editor, appState.facade.document().id());
        clearCreativeEditorDocumentSavePoint(
            editor.persistence, appState.facade.document().id());
      }
      result.accepted = replaced.accepted;
      result.changed = replaced.changed;
      result.documentReplaced = replaced.accepted && replaced.changed;
      result.message = replaced.accepted
                           ? "new document"
                           : std::string(replaced.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::OpenDocument: {
      const std::string saveId =
          context.activeSaveId != nullptr ? *context.activeSaveId : std::string{};
      creative::CreativeWorldLayout loadedLayout;
      const bool loaded = loadStandaloneScene(
          appState, context.saveRoot, saveId, &loadedLayout);
      if (loaded) {
        clearEditHistory(appState.history, "desktop_open");
        resetCreativeEditorForDocumentReplacement(
            editor, appState.facade.document().id());
        installCreativeEditorWorldLayout(editor.worldLayout,
                                         std::move(loadedLayout));
        markCreativeEditorDocumentSaved(
            editor.persistence, appState.facade.document());
      }
      result.accepted = loaded;
      result.changed = loaded;
      result.documentReplaced = loaded;
      result.message = loaded ? "opened " + saveId : "open failed";
      break;
    }
    case CreativeDesktopCommandId::SaveDocument: {
      const std::string saveId =
          context.activeSaveId != nullptr ? *context.activeSaveId : std::string{};
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, saveId,
                              &editor.worldLayout.source,
                              editor.worldLayout.generatedRevision ==
                                  editor.worldLayout.revision);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        markCreativeEditorWorldLayoutSaved(editor.worldLayout);
        markCreativeEditorDocumentSaved(
            editor.persistence, appState.facade.document());
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved " + saveId : "save failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SaveDocumentAs: {
      const auto* payload = payloadAs<CreativeDesktopSaveAsPayload>(command);
      const std::string saveId = payload != nullptr ? payload->saveId : std::string{};
      if (saveId.empty()) {
        result.message = "save as: empty name";
        break;
      }
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, saveId,
                              &editor.worldLayout.source,
                              editor.worldLayout.generatedRevision ==
                                  editor.worldLayout.revision);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        if (context.activeSaveId != nullptr) {
          *context.activeSaveId = saveId;
        }
        markCreativeEditorWorldLayoutSaved(editor.worldLayout);
        markCreativeEditorDocumentSaved(
            editor.persistence, appState.facade.document());
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved as " + saveId
                          : "save as failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::RegenerateMapTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopMapTemplatePayload>(command);
      if (payload == nullptr) {
        result.message = "map regeneration: payload mismatch";
        break;
      }
      regenerateMapTemplate(*payload, context, result);
      break;
    }
    case CreativeDesktopCommandId::Undo: {
      CreativeEditorWorldLayoutState* worldLayout =
          &activeAppState == &appState ? &editor.worldLayout : nullptr;
      const std::uint64_t layoutRevisionBefore =
          worldLayout != nullptr ? worldLayout->revision : 0U;
      const std::uint64_t layoutEpochBefore =
          worldLayout != nullptr ? worldLayout->sourceEpoch : 0U;
      const std::uint64_t documentRevisionBefore =
          activeAppState.facade.document().revision();
      const bool previewWasActive =
          worldLayout != nullptr &&
          creativeEditorWorldLayoutPreviewActive(*worldLayout);
      const bool ok =
          undoLastEdit(activeAppState, "desktop_undo", worldLayout);
      result.accepted = ok;
      result.changed = ok;
      result.worldLayoutChanged =
          worldLayout != nullptr &&
          (worldLayout->revision != layoutRevisionBefore ||
           worldLayout->sourceEpoch != layoutEpochBefore);
      result.sceneChanged =
          ok && (previewWasActive ||
                 activeAppState.facade.document().revision() !=
                     documentRevisionBefore);
      result.message = result.worldLayoutChanged
                           ? worldLayout->statusMessage
                           : (ok ? "undo" : "nothing to undo");
      break;
    }
    case CreativeDesktopCommandId::Redo: {
      CreativeEditorWorldLayoutState* worldLayout =
          &activeAppState == &appState ? &editor.worldLayout : nullptr;
      const std::uint64_t layoutRevisionBefore =
          worldLayout != nullptr ? worldLayout->revision : 0U;
      const std::uint64_t layoutEpochBefore =
          worldLayout != nullptr ? worldLayout->sourceEpoch : 0U;
      const std::uint64_t documentRevisionBefore =
          activeAppState.facade.document().revision();
      const bool previewWasActive =
          worldLayout != nullptr &&
          creativeEditorWorldLayoutPreviewActive(*worldLayout);
      const bool ok =
          redoLastEdit(activeAppState, "desktop_redo", worldLayout);
      result.accepted = ok;
      result.changed = ok;
      result.worldLayoutChanged =
          worldLayout != nullptr &&
          (worldLayout->revision != layoutRevisionBefore ||
           worldLayout->sourceEpoch != layoutEpochBefore);
      result.sceneChanged =
          ok && (previewWasActive ||
                 activeAppState.facade.document().revision() !=
                     documentRevisionBefore);
      result.message = result.worldLayoutChanged
                           ? worldLayout->statusMessage
                           : (ok ? "redo" : "nothing to redo");
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
