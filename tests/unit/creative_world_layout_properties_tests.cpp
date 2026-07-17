#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

app::CreativeEditorWorldLayoutState shellState() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "properties_test");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {8, 6}}, 0.0, 3U, 0.25, 1U});
  if (!shell.accepted) {
    std::cerr << "FAIL: properties fixture shell\n";
  }
  return state;
}

bool levelSettingsAreOwnedAndValidated() {
  app::CreativeEditorWorldLayoutState state = shellState();
  app::CreativeEditorWorldLayoutLevelSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutLevelSettings(
      state, 0U, settings);
  const std::uint64_t before = state.revision;
  settings.name = "Upper Hall";
  settings.floorTopLayer = 1.0;
  settings.wallHeightCells = 4U;
  settings.floorThicknessLayers = 2U;
  settings.ceilingThicknessLayers = 2U;
  settings.roofThicknessLayers = 2U;
  settings.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  settings.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  settings.roofPitchDegrees = 35.0;
  settings.roofOverhangCells = 0.5;
  const auto updated = app::setCreativeEditorWorldLayoutLevelSettings(
      state, 0U, settings);

  const auto added = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  app::CreativeEditorWorldLayoutLevelSettings conflicting;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      state, 1U, conflicting));
  conflicting.floorTopLayer = settings.floorTopLayer;
  const std::uint64_t beforeConflict = state.revision;
  const auto rejected = app::setCreativeEditorWorldLayoutLevelSettings(
      state, 1U, conflicting);
  const auto focused = app::focusCreativeEditorWorldLayoutSource(
      state, cr::CreativeWorldLayoutTable::Level, 0U);

  return expect(read && updated.accepted && updated.changed,
                "level settings accept a valid complete edit") &&
         expect(state.source.levels[0].name == "Upper Hall" &&
                    state.source.levels[0].ceilingThicknessLayers == 2U &&
                    state.source.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Gable &&
                    before + 2U == state.revision,
                "level settings and level add each advance source once") &&
         expect(added.accepted && added.changed && !rejected.accepted &&
                    !rejected.changed && state.revision == beforeConflict,
                "duplicate level elevations reject without revision drift") &&
         expect(focused.accepted &&
                    state.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Level &&
                    state.selection.index == 0U &&
                    state.activeLevelIndex == 0U &&
                    app::creativeEditorWorldLayoutSelectedBuilding(state) ==
                        0U,
                "a level is a real selection with an owning building");
}

bool terrainProfileSettingsAreBounded() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "profile_1";
  profile.kind = cr::CreativeTerrainRecipeKind::Hill;
  profile.center = {4, 5};
  profile.baseHeightCells = 4U;
  profile.radiusCells = 4U;
  profile.amplitudeCells = 4U;
  profile.spacingCells = 1U;
  state.source.terrainProfiles.push_back(profile);

  app::CreativeEditorWorldLayoutTerrainProfileSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);
  settings.kind = cr::CreativeTerrainRecipeKind::Ridge;
  settings.center = {12, -3};
  settings.baseHeightCells = 7U;
  settings.radiusCells = 8U;
  settings.amplitudeCells = 2U;
  settings.spacingCells = 2U;
  settings.direction =
      cr::CreativeTerrainProfileDirection::NegativeXPositiveZ;
  settings.frequency = 2U;
  const auto updated = app::setCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;
  settings.radiusCells = 3U;
  const auto rejected = app::setCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    state.source.terrainProfiles[0].kind ==
                        cr::CreativeTerrainRecipeKind::Ridge &&
                    state.source.terrainProfiles[0].blend ==
                        cr::CreativeTerrainProfileBlend::Set &&
                    state.source.terrainProfiles[0].rodPolicy ==
                        cr::CreativeTerrainProfileRodPolicy::Fill,
                "terrain profiles retain absolute World Layout semantics") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate,
                "unsupported terrain sampling values reject atomically");
}

bool terrainPathSettingsUseTheSharedRecipe() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "path_1";
  path.kind = cr::CreativeTerrainRecipeKind::Road;
  path.firstPointIndex = 0U;
  path.pointCount = 2U;
  path.elevation = cr::CreativeTerrainPathElevation::Level;
  state.source.terrainPathPoints = {{{0, 0}, 4U}, {{5, 0}, 4U}};
  state.source.terrainPaths.push_back(path);

  app::CreativeEditorWorldLayoutTerrainPathSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutTerrainPathSettings(
      state, 0U, settings);
  settings.kind = cr::CreativeTerrainRecipeKind::River;
  settings.elevation = cr::CreativeTerrainPathElevation::Grade;
  settings.halfWidthCells = 2U;
  settings.amplitudeCells = 4U;
  settings.paintSurface = true;
  settings.material = cr::CreativeTerrainMaterial::Sand;
  settings.points[1] = {{7, 3}, 8U};
  const auto updated = app::setCreativeEditorWorldLayoutTerrainPathSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;
  settings.points[1] = settings.points[0];
  const auto rejected = app::setCreativeEditorWorldLayoutTerrainPathSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    state.source.terrainPaths[0].kind ==
                        cr::CreativeTerrainRecipeKind::River &&
                    state.source.terrainPathPoints[1].coord ==
                        cr::CreativeTerrainCoord2{7, 3} &&
                    state.source.terrainPathPoints[1].heightCells == 8U,
                "path properties and control points update together") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate &&
                    state.source.terrainPathPoints[1].coord ==
                        cr::CreativeTerrainCoord2{7, 3},
                "the terrain path kernel rejects degenerate segments");
}

bool objectSettingsPreserveSemanticIdentity() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Rock;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  object.stableKey = "rock_1";
  object.name = "Boulder";
  object.assetId = "boulder_01";
  object.boundsCells = {{1.0, 0.0, 1.0}, {3.0, 2.0, 3.0}};
  object.tags = {"world_layout:object"};
  state.source.objects.push_back(object);

  app::CreativeEditorWorldLayoutObjectSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  settings.name = "Gate Boulder";
  settings.assetId = "boulder_gate";
  settings.boundsCells = {{2.0, 0.0, 2.0}, {5.0, 3.0, 5.0}};
  settings.visible = false;
  const auto updated = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;
  settings.boundsCells.max = settings.boundsCells.min;
  const auto rejected = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    state.source.objects[0].name == "Gate Boulder" &&
                    state.source.objects[0].kind ==
                        cr::CreativeObjectKind::Rock &&
                    state.source.objects[0].mode ==
                        cr::CreativeObjectLibraryPlacementMode::Bounds &&
                    state.source.objects[0].stableKey == "rock_1" &&
                    state.source.objects[0].tags ==
                        std::vector<std::string>{"world_layout:object"} &&
                    !state.source.objects[0].visible,
                "object edits preserve kind, mode, tags, and stable key") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate,
                "zero-volume object bounds reject atomically");
}

cr::CreativeAppState makeApp() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Properties");
  static_cast<void>(document.assignId(9501U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool typedSettingsCommandsGuardIdentityAndPreview() {
  cr::CreativeAppState live = makeApp();
  app::CreativeEditorState editor;
  editor.worldLayout = shellState();
  const auto preview = app::previewCreativeEditorWorldLayout(
      editor.worldLayout, live.facade.document());
  app::CreativeEditorWorldLayoutLevelSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      editor.worldLayout, 0U, settings));
  settings.name = "Command Level";
  const std::string levelKey = editor.worldLayout.source.levels[0].stableKey;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};

  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
             app::CreativeDesktopWorldLayoutLevelSettingsPayload{
                 0U, levelKey, settings});
  const app::CreativeDesktopCommandResult applied =
      app::dispatchCreativeDesktopCommands(frame, context);

  app::CreativeDesktopCommandFrame staleFrame;
  staleFrame.push(app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
                  app::CreativeDesktopWorldLayoutLevelSettingsPayload{
                      0U, "stale_level", settings});
  const app::CreativeDesktopCommandResult stale =
      app::dispatchCreativeDesktopCommands(staleFrame, context);

  return expect(preview.accepted && applied.accepted && applied.changed &&
                    applied.worldLayoutChanged && applied.sceneChanged &&
                    editor.worldLayout.source.levels[0].name ==
                        "Command Level" &&
                    !app::creativeEditorWorldLayoutPreviewActive(
                        editor.worldLayout),
                "typed property commands invalidate an exact preview") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.message == "layout level settings: stale target",
                "stable keys prevent delayed property retargeting");
}

}  // namespace

int main() {
  const bool ok = levelSettingsAreOwnedAndValidated() &&
                  terrainProfileSettingsAreBounded() &&
                  terrainPathSettingsUseTheSharedRecipe() &&
                  objectSettingsPreserveSemanticIdentity() &&
                  typedSettingsCommandsGuardIdentityAndPreview();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
