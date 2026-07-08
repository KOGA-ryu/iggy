#include "CreativeEditorCommandInput.hpp"

#include <SDL3/SDL.h>

#include <string>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include "StandaloneBrushPalette.hpp"
#include "StandaloneDelete.hpp"
#include "StandalonePersistenceProof.hpp"
#include "StandaloneUndo.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;

void applyCreativeEditorCommandInput(
    const bool* keys,
    bool captureMode,
    creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  if (captureMode || keys == nullptr) {
    return;
  }

  const bool key1 = keys[SDL_SCANCODE_1] != 0;
  const bool key2 = keys[SDL_SCANCODE_2] != 0;
  const bool key3 = keys[SDL_SCANCODE_3] != 0;
  const bool keyB = keys[SDL_SCANCODE_B] != 0;
  const bool keyDelete = keys[SDL_SCANCODE_DELETE] != 0;
  const bool keyBackspace = keys[SDL_SCANCODE_BACKSPACE] != 0;
  const bool keyZ = keys[SDL_SCANCODE_Z] != 0;
  const SDL_Keymod modState = SDL_GetModState();
  const bool undoModifier =
      (modState & (SDL_KMOD_GUI | SDL_KMOD_CTRL)) != 0U;
  if (key1 && !editor.prevKey1) {
    editor.placeMode = false;  // '1' Select leaves Place mode.
    const bool ok = appState.facade.setActiveTool(creative::Tool::Select);
    SDL_Log("iggy3d_creative: setActiveTool(Select) accepted=%d placeMode=0",
            ok ? 1 : 0);
  }
  if (key2 && !editor.prevKey2) {
    editor.placeMode = false;  // '2' Move leaves Place mode.
    const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
    SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d placeMode=0",
            ok ? 1 : 0);
  }
  if (key3 && !editor.prevKey3) {
    editor.placeMode = true;  // '3' Place: app-level mode, not a kernel Tool.
    SDL_Log("iggy3d_creative: placeMode=1 brush='%s'",
            std::string(creative::toString(editor.placeBrush)).c_str());
  }
  if (keyB && !editor.prevKeyB) {
    editor.placeBrush = nextBrushKind(editor.brushPalette, editor.placeBrush);
    SDL_Log("iggy3d_creative: brush cycled -> '%s'",
            std::string(creative::toString(editor.placeBrush)).c_str());
  }
  if ((keyDelete && !editor.prevKeyDelete) ||
      (keyBackspace && !editor.prevKeyBackspace)) {
    (void)deleteSelectedObject(appState,
                               keyDelete ? "delete_key" : "backspace_key",
                               &editor.undoStack);
  }
  if (keyZ && !editor.prevKeyZ && undoModifier) {
    (void)undoLastSnapshot(appState, editor.undoStack, "keyboard_undo");
  }
  const bool keyF5 = keys[SDL_SCANCODE_F5] != 0;
  const bool keyF6 = keys[SDL_SCANCODE_F6] != 0;
  const bool keyF9 = keys[SDL_SCANCODE_F9] != 0;
  if (keyF5 && !editor.prevKeyF5) {
    const iggy3d::CreativeWorldSaveResult saveResult =
        saveStandaloneScene(appState.facade, saveRoot, saveId);
    if (saveResult.accepted && saveResult.saved) {
      clearUndoStack(editor.undoStack, "save_success");
    }
  }
  if (keyF6 && !editor.prevKeyF6) {
    clearToBlankScene(appState);
    clearUndoStack(editor.undoStack, "new_clear");
  }
  if (keyF9 && !editor.prevKeyF9) {
    const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
    if (loaded) {
      clearUndoStack(editor.undoStack, "load_success");
    }
  }
  editor.prevKey1 = key1;
  editor.prevKey2 = key2;
  editor.prevKey3 = key3;
  editor.prevKeyB = keyB;
  editor.prevKeyDelete = keyDelete;
  editor.prevKeyBackspace = keyBackspace;
  editor.prevKeyZ = keyZ;
  editor.prevKeyF5 = keyF5;
  editor.prevKeyF6 = keyF6;
  editor.prevKeyF9 = keyF9;
}

}  // namespace iggy3d_creative_app
