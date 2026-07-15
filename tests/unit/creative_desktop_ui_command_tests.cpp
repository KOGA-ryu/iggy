#include "EditorDesktopCommands.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorPersistence.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/history/History.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

// UI-2a (docs/creative_desktop_ui_plan.md DD-7): the desktop UI emits semantic
// command IDs into a bounded frame; EditorDesktopCommands.cpp is the sole
// dispatcher, reusing the existing kernels with the same history discipline as
// the keyboard path. Fully headless — no ImGui, no window.

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeObjectId createCrate(cr::Facade& facade, double x) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.transform.position = {x, 0, 0};
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

void selectPrimary(cr::Facade& facade, cr::CreativeObjectId objectId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  input.pointer.modifiers = cr::kCreativeToolModifierNone;
  static_cast<void>(facade.dispatchToolInput(input));
}

app::CreativeDesktopCommandResult dispatchOne(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context,
    std::string arg = {}) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(arg));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

bool newDocumentReplacesAndClearsHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd New");
  static_cast<void>(document.assignId(410U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 0.0));
  static_cast<void>(createCrate(appState.facade, 2.0));
  selectPrimary(appState.facade,
                createCrate(appState.facade, 4.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{}, "seed"));

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult result =
      dispatchOne(app::CreativeDesktopCommandId::NewDocument, context);

  return expect(result.accepted && result.documentReplaced,
                "new document is accepted and replaces the document") &&
         expect(appState.facade.document().objectCount() == 0U,
                "new document is blank") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "new document clears undo history");
}

bool deleteAndDuplicateHitTheKernels() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Edit");
  static_cast<void>(document.assignId(411U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 0.0));
  const cr::CreativeObjectId target = createCrate(appState.facade, 2.0);
  selectPrimary(appState.facade, target);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const std::uint64_t before = appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult dup =
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context);
  const bool grew = appState.facade.document().objectCount() == before + 1U;
  const app::CreativeDesktopCommandResult del =
      dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);

  return expect(dup.accepted && dup.changed && grew,
                "duplicate adds one object via the kernel") &&
         expect(del.accepted && del.changed &&
                    appState.facade.document().objectCount() == before,
                "delete removes the duplicated object via the kernel");
}

bool undoRedoMoveTheHistoryRings() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Undo");
  static_cast<void>(document.assignId(412U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  static_cast<void>(
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context));
  const bool recorded = cr::creativeUndoDepth(appState.history) == 1U;
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undone = cr::creativeUndoDepth(appState.history) == 0U;
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redone = cr::creativeUndoDepth(appState.history) == 1U;

  return expect(recorded, "duplicate records one undo step") &&
         expect(undo.accepted && undone, "undo command pops the ring") &&
         expect(redo.accepted && redone, "redo command restores the ring");
}

bool saveAsRebindsTheActiveSlotAndClearsHistory() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_desktop_cmd_tests";
  std::filesystem::remove_all(saveRoot);
  std::filesystem::create_directories(saveRoot);

  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Save");
  static_cast<void>(document.assignId(413U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{}, "seed"));

  app::CreativeEditorState editor;
  std::string saveId = "world_start";
  const app::CreativeDesktopCommandContext context{appState, editor, saveRoot,
                                                    &saveId};

  const app::CreativeDesktopCommandResult saveAs =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocumentAs, context,
                  "world_named");
  const bool rebounded = saveId == "world_named";
  const bool historyCleared = cr::creativeUndoDepth(appState.history) == 0U;

  const app::CreativeDesktopCommandResult emptyName =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocumentAs, context,
                  std::string{});

  std::filesystem::remove_all(saveRoot);
  return expect(saveAs.accepted && rebounded,
                "save as accepts and rebinds the active save id") &&
         expect(historyCleared, "save clears undo history like the keyboard path") &&
         expect(!emptyName.accepted,
                "save as with an empty name is rejected");
}

bool playIsUnsupportedAndFrameIsBounded() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Misc");
  static_cast<void>(document.assignId(414U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult play =
      dispatchOne(app::CreativeDesktopCommandId::Play, context);

  app::CreativeDesktopCommandFrame frame;
  for (std::size_t index = 0U;
       index < app::kCreativeDesktopCommandCapacity + 4U; ++index) {
    frame.push(app::CreativeDesktopCommandId::Undo);
  }

  return expect(!play.accepted && !play.message.empty(),
                "play returns an unsupported result with a message") &&
         expect(frame.overflowed &&
                    frame.count == app::kCreativeDesktopCommandCapacity,
                "the command frame is bounded and records overflow");
}

}  // namespace

int main() {
  bool ok = true;
  ok = newDocumentReplacesAndClearsHistory() && ok;
  ok = deleteAndDuplicateHitTheKernels() && ok;
  ok = undoRedoMoveTheHistoryRings() && ok;
  ok = saveAsRebindsTheActiveSlotAndClearsHistory() && ok;
  ok = playIsUnsupportedAndFrameIsBounded() && ok;
  return ok ? 0 : 1;
}
