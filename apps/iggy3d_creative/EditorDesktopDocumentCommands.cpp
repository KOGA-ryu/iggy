#include "EditorDesktopCommandsInternal.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorPersistence.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopDocumentCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  switch (command.id) {
    case CreativeDesktopCommandId::NewDocument:
      clearToBlankScene(appState);
      clearEditHistory(appState.history, "desktop_new");
      resetCreativeEditorForDocumentReplacement(
          editor, appState.facade.document().id());
      result.accepted = true;
      result.changed = true;
      result.documentReplaced = true;
      result.message = "new document";
      break;
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
        clearEditHistory(appState.history, "desktop_save");
        markCreativeEditorWorldLayoutSaved(editor.worldLayout);
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
        clearEditHistory(appState.history, "desktop_save_as");
        markCreativeEditorWorldLayoutSaved(editor.worldLayout);
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved as " + saveId
                          : "save as failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::Undo: {
      CreativeEditorWorldLayoutState* worldLayout =
          &activeAppState == &appState ? &editor.worldLayout : nullptr;
      const std::uint64_t layoutRevisionBefore =
          worldLayout != nullptr ? worldLayout->revision : 0U;
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
          worldLayout->revision != layoutRevisionBefore;
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
          worldLayout->revision != layoutRevisionBefore;
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
