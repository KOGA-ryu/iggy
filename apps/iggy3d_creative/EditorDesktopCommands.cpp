#include "EditorDesktopCommands.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorPersistence.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       std::string arg) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].arg = std::move(arg);
  ++count;
}

void CreativeDesktopCommandFrame::clear() noexcept {
  count = 0U;
  overflowed = false;
}

namespace {

void dispatchOne(const CreativeDesktopCommand& command,
                 const CreativeDesktopCommandContext& context,
                 CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  result.lastCommand = command.id;
  result.accepted = false;
  result.changed = false;
  result.documentReplaced = false;

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
      const bool loaded = loadStandaloneScene(appState, context.saveRoot, saveId);
      if (loaded) {
        clearEditHistory(appState.history, "desktop_open");
        resetCreativeEditorForDocumentReplacement(
            editor, appState.facade.document().id());
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
          saveStandaloneScene(appState.facade, context.saveRoot, saveId);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        clearEditHistory(appState.history, "desktop_save");
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved " + saveId : "save failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SaveDocumentAs: {
      if (command.arg.empty()) {
        result.message = "save as: empty name";
        break;
      }
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, command.arg);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        if (context.activeSaveId != nullptr) {
          *context.activeSaveId = command.arg;
        }
        clearEditHistory(appState.history, "desktop_save_as");
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved as " + command.arg
                          : "save as failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::Undo: {
      const bool ok = undoLastEdit(appState, "desktop_undo");
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "undo" : "nothing to undo";
      break;
    }
    case CreativeDesktopCommandId::Redo: {
      const bool ok = redoLastEdit(appState, "desktop_redo");
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "redo" : "nothing to redo";
      break;
    }
    case CreativeDesktopCommandId::DuplicateSelection: {
      const creative::CreativeDuplicateCommandReceipt receipt =
          duplicateSelectedObjectsWithUndo(
              appState, appState.history,
              creative::CreativeDuplicateCommandRequest{}, "desktop_duplicate");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = receipt.changed ? "duplicated selection"
                                       : "nothing to duplicate";
      break;
    }
    case CreativeDesktopCommandId::DeleteSelection: {
      const creative::CreativeDocumentRemoveReceipt receipt =
          deleteSelectedObject(appState, "desktop_delete", &appState.history);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = receipt.changed ? "deleted selection"
                                       : "nothing to delete";
      break;
    }
    case CreativeDesktopCommandId::Play:
      // No runtime/play mode exists in the standalone editor yet (plan DD-12).
      result.accepted = false;
      result.message = "play is not available in this build";
      break;
    case CreativeDesktopCommandId::None:
    case CreativeDesktopCommandId::Count:
      break;
  }
}

}  // namespace

CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context) {
  CreativeDesktopCommandResult result;
  const std::size_t count =
      frame.count < kCreativeDesktopCommandCapacity ? frame.count
                                                    : kCreativeDesktopCommandCapacity;
  for (std::size_t index = 0U; index < count; ++index) {
    dispatchOne(frame.commands[index], context, result);
  }
  return result;
}

}  // namespace iggy3d_creative_app
