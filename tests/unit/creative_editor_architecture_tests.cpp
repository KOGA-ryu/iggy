#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= 1.0e-9;
}

app::CreativeDesktopCommandResult dispatch(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context,
    app::CreativeDesktopCommandPayload payload = {}) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

struct EditorFixture {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor;
  std::string saveId = "architecture_test";
};

bool initialize(EditorFixture& fixture) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Architecture Editor Test");
  static_cast<void>(document.assignId(9901U));
  cr::CreativeGridSettings grid = document.gridSettings();
  grid.cellSizeMeters = 0.5;
  if (!document.setGridSettings(grid) ||
      !fixture.appState.facade.installDocument(std::move(document)).accepted) {
    return false;
  }

  app::resetCreativeEditorWorldLayout(fixture.editor.worldLayout,
                                      "architecture_editor_test");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {20, 16}};
  settings.shell.floorTopLayer = 2.0;
  settings.shell.wallHeightCells = 4U;
  settings.shell.floorThicknessLayers = 2U;
  settings.shell.roofThicknessLayers = 1U;
  settings.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2;
  settings.storeys.count = 2U;
  settings.storeys.connectStoreys = true;
  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingBlockout(
          fixture.editor.worldLayout, settings);
  if (!created.accepted || !created.changed) {
    return false;
  }

  const app::CreativeDesktopCommandContext context{
      fixture.appState, fixture.editor, std::filesystem::path{},
      &fixture.saveId};
  const app::CreativeDesktopCommandResult generated = dispatch(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  return generated.accepted && generated.changed &&
         fixture.editor.worldLayout.generatedRevision ==
             fixture.editor.worldLayout.revision;
}

bool profilePreviewApplyAndUndoShareOneTransaction() {
  EditorFixture fixture;
  if (!expect(initialize(fixture),
              "architecture editor fixture generates one estate")) {
    return false;
  }
  app::CreativeEditorWorldLayoutState& state = fixture.editor.worldLayout;
  const app::CreativeDesktopCommandContext context{
      fixture.appState, fixture.editor, std::filesystem::path{},
      &fixture.saveId};
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint original =
      cr::fingerprintCreativeWorldLayoutBuilding(state.source, 0U);
  const std::uint64_t sourceRevisionBefore = state.revision;
  const std::uint64_t documentRevisionBefore =
      fixture.appState.facade.document().revision();
  const std::uint64_t undoBefore =
      cr::creativeUndoDepth(fixture.appState.history);

  const cr::CreativeWorldLayoutArchitecturalProfile profile =
      cr::defaultCreativeWorldLayoutArchitecturalProfile(
          cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const app::CreativeDesktopWorldLayoutBuildingArchitecturePayload payload{
      0U, state.source.buildings[0].stableKey, profile};
  const app::CreativeDesktopCommandResult previewed = dispatch(
      app::CreativeDesktopCommandId::WorldLayoutPreviewBuildingArchitecture,
      context, payload);
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint afterPreview =
      cr::fingerprintCreativeWorldLayoutBuilding(state.source, 0U);
  const bool previewStayedTransient =
      previewed.accepted && previewed.changed && previewed.sceneChanged &&
      app::creativeEditorWorldLayoutPreviewActive(state) &&
      afterPreview == original && state.revision == sourceRevisionBefore &&
      fixture.appState.facade.document().revision() ==
          documentRevisionBefore &&
      cr::creativeUndoDepth(fixture.appState.history) == undoBefore;

  const app::CreativeDesktopCommandResult applied = dispatch(
      app::CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture,
      context, payload);
  const cr::CreativeWorldLayoutBuildingDimensions appliedDimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          fixture.appState.facade.document().gridSettings(), state.source,
          0U);
  const bool appliedOnce =
      applied.accepted && applied.changed && applied.worldLayoutChanged &&
      applied.sceneChanged && !app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.revision == sourceRevisionBefore + 1U &&
      state.generatedRevision == state.revision &&
      cr::creativeUndoDepth(fixture.appState.history) == undoBefore + 1U &&
      appliedDimensions.accepted &&
      near(appliedDimensions.minimumFloorToFloorMeters, 3.0) &&
      near(appliedDimensions.exteriorFacadeHeightMeters, 6.0);

  const app::CreativeDesktopCommandResult undone = dispatch(
      app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint afterUndo =
      cr::fingerprintCreativeWorldLayoutBuilding(state.source, 0U);
  const cr::CreativeWorldLayoutBuildingDimensions undoDimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          fixture.appState.facade.document().gridSettings(), state.source,
          0U);
  const bool undoRestored =
      undone.accepted && undone.changed && afterUndo == original &&
      undoDimensions.accepted &&
      near(undoDimensions.minimumFloorToFloorMeters, 2.0) &&
      near(undoDimensions.exteriorFacadeHeightMeters, 4.0);

  const app::CreativeDesktopCommandResult redone = dispatch(
      app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeWorldLayoutBuildingDimensions redoDimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          fixture.appState.facade.document().gridSettings(), state.source,
          0U);

  return expect(previewStayedTransient,
                "profile preview changes only transient scene output") &&
         expect(appliedOnce,
                "profile apply commits source scene and exactly one undo") &&
         expect(undoRestored,
                "profile undo restores exact source scale and geometry") &&
         expect(redone.accepted && redone.changed && redoDimensions.accepted &&
                    near(redoDimensions.minimumFloorToFloorMeters, 3.0),
                "profile redo restores the normalized estate");
}

bool staleOrInvalidCommandsMutateNothing() {
  EditorFixture fixture;
  if (!expect(initialize(fixture),
              "architecture rejection fixture generates one estate")) {
    return false;
  }
  app::CreativeEditorWorldLayoutState& state = fixture.editor.worldLayout;
  const app::CreativeDesktopCommandContext context{
      fixture.appState, fixture.editor, std::filesystem::path{},
      &fixture.saveId};
  const std::uint64_t sourceRevisionBefore = state.revision;
  const std::uint64_t documentRevisionBefore =
      fixture.appState.facade.document().revision();
  const std::uint64_t undoBefore =
      cr::creativeUndoDepth(fixture.appState.history);

  cr::CreativeWorldLayoutArchitecturalProfile profile =
      cr::defaultCreativeWorldLayoutArchitecturalProfile(
          cr::CreativeWorldLayoutArchitecturalProfileKind::Custom);
  profile.floorToFloorMeters = 3.1;
  const app::CreativeDesktopCommandResult invalid = dispatch(
      app::CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture,
      context,
      app::CreativeDesktopWorldLayoutBuildingArchitecturePayload{
          0U, state.source.buildings[0].stableKey, profile});
  profile.floorToFloorMeters = 3.0;
  const app::CreativeDesktopCommandResult stale = dispatch(
      app::CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture,
      context,
      app::CreativeDesktopWorldLayoutBuildingArchitecturePayload{
          0U, "stale-building-key", profile});
  const app::CreativeDesktopCommandResult mismatch = dispatch(
      app::CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture,
      context, app::CreativeDesktopWorldLayoutBuildingSelectionPayload{0U});

  return expect(!invalid.accepted && !invalid.changed &&
                    invalid.message ==
                        "creative_world_layout_architecture_profile_"
                        "unrepresentable",
                "non-grid custom profile rejects at the semantic boundary") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.message ==
                        "building architecture apply: stale target",
                "stale building key cannot redirect an architecture edit") &&
         expect(!mismatch.accepted && !mismatch.changed &&
                    mismatch.message ==
                        "building architecture apply: payload mismatch",
                "wrong payload alternative remains a no-op") &&
         expect(state.revision == sourceRevisionBefore &&
                    fixture.appState.facade.document().revision() ==
                        documentRevisionBefore &&
                    cr::creativeUndoDepth(fixture.appState.history) ==
                        undoBefore,
                "all rejected architecture commands preserve transaction state");
}

}  // namespace

int main() {
  const bool ok = profilePreviewApplyAndUndoShareOneTransaction() &&
                  staleOrInvalidCommandsMutateNothing();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
