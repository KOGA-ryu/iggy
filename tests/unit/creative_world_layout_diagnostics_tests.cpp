#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-4F;
}

cr::CreativeAppState makeApp(std::string name, cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create(std::move(name));
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

app::CreativeEditorWorldLayoutState roomLayout() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "diagnostic_layout");
  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingShell(
          state, {{{10, 20}, {18, 26}}, 0.0, 4U, 0.25, 1U});
  if (!created.accepted) {
    std::cerr << "FAIL: diagnostic fixture building shell\n";
  }
  return state;
}

bool preflightCacheTracksBothTruthRevisions() {
  cr::CreativeAppState first = makeApp("Diagnostic First", 9201U);
  cr::CreativeAppState second = makeApp("Diagnostic Second", 9202U);
  app::CreativeEditorWorldLayoutState state = roomLayout();

  const auto& initial = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, first.facade.document(), state.source,
      state.revision);
  const bool initialReady = initial.ready && initial.issueCount == 0U;
  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, first.facade.document(), state.source,
      state.revision));
  const std::uint64_t afterIdle = state.diagnosticCache.buildCount;

  state.source.rooms[0].name.clear();
  ++state.revision;
  const auto& invalid = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, first.facade.document(), state.source,
      state.revision);
  const bool exactFailure =
      !invalid.ready && invalid.issueCount == 1U &&
      invalid.issues[0].table == cr::CreativeWorldLayoutTable::Room &&
      invalid.issues[0].index == 0U &&
      invalid.issues[0].status == cr::CreativeWorldLayoutStatus::InvalidSymbol;
  const std::uint64_t afterLayoutEdit = state.diagnosticCache.buildCount;

  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, second.facade.document(), state.source,
      state.revision));
  const std::uint64_t afterDocumentChange = state.diagnosticCache.buildCount;

  return expect(initialReady, "valid layout preflight is ready") &&
         expect(afterIdle == 1U,
                "idle frames reuse one authoritative compile") &&
         expect(exactFailure,
                "compiler failure table and index survive projection") &&
         expect(afterLayoutEdit == 2U,
                "layout revision invalidates preflight once") &&
         expect(afterDocumentChange == 3U,
                "document identity invalidates preflight once");
}

bool diagnosticFocusSelectsFramesAndPreservesSource() {
  app::CreativeEditorWorldLayoutState state = roomLayout();
  state.canvasPixelsPerCell = 10.0F;
  state.elevationPixelsPerCell = 12.0F;
  state.elevationAxis = app::CreativeEditorWorldLayoutElevationAxis::X;
  const std::uint64_t sourceRevision = state.revision;

  const app::CreativeEditorWorldLayoutEditReceipt room =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::Room, 0U);
  const bool roomFocused =
      room.accepted &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index == 0U && state.activeLevelIndex == 0U &&
      near(state.canvasPanX, -140.0F) && near(state.canvasPanZ, -230.0F) &&
      near(state.elevationPanHorizontal, -168.0F);

  const app::CreativeEditorWorldLayoutEditReceipt level =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::Level, 0U);
  const bool levelFocused =
      level.accepted &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Building &&
      state.selection.index == 0U && state.activeLevelIndex == 0U;

  const app::CreativeEditorWorldLayoutEditReceipt invalid =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::Room, 99U);
  return expect(roomFocused,
                "room issue selects its source and centers both views") &&
         expect(levelFocused,
                "level issue selects its owning building and level") &&
         expect(!invalid.accepted,
                "out-of-range diagnostic target is rejected") &&
         expect(state.revision == sourceRevision,
                "diagnostic navigation never mutates source truth");
}

bool diagnosticFocusRoutesThroughTypedDispatcher() {
  cr::CreativeAppState live = makeApp("Diagnostic Command", 9203U);
  app::CreativeEditorState editor;
  editor.worldLayout = roomLayout();
  editor.worldLayout.tool = app::CreativeEditorWorldLayoutTool::Room;
  editor.worldLayout.selection = {};
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};
  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutFocusSource,
             app::CreativeDesktopWorldLayoutSourcePayload{
                 cr::CreativeWorldLayoutTable::Room, 0U, {}});
  const app::CreativeDesktopCommandResult result =
      app::dispatchCreativeDesktopCommands(frame, context);

  return expect(result.accepted && result.changed,
                "typed diagnostic command is accepted") &&
         expect(!result.worldLayoutChanged && !result.sceneChanged,
                "diagnostic command changes only editor view state") &&
         expect(editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Room &&
                    editor.worldLayout.selection.index == 0U,
                "dispatcher focuses the requested source symbol");
}

}  // namespace

int main() {
  const bool ok = preflightCacheTracksBothTruthRevisions() &&
                  diagnosticFocusSelectsFramesAndPreservesSource() &&
                  diagnosticFocusRoutesThroughTypedDispatcher();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
