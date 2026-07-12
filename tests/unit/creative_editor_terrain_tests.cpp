#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;
using namespace iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

void installDocument(cr::CreativeAppState& appState,
                     cr::CreativeDocumentId id) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Terrain Test");
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
}

CreativeEditorState terrainEditor(std::int32_t x, std::int32_t z) {
  CreativeEditorState editor;
  editor.frameIndex = 8U;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] =
      {cr::CreativeHeldItemKind::TerrainControl,
       cr::CreativeObjectKind::Unknown};
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.targetCell = {x, 0, z};
  return editor;
}

bool quickEditOwnsHeightAndRadiusWithoutNewBindings() {
  CreativeEditorState editor = terrainEditor(0, 0);
  const bool raised = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  const bool selectedRadius = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditNext);
  const bool widened = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditIncrease);

  return expect(raised && editor.terrain.heightCells == 5U,
                "D-pad right raises terrain height") &&
         expect(selectedRadius &&
                    editor.terrain.selectedSetting ==
                        CreativeEditorTerrainSetting::Radius,
                "D-pad down selects radius") &&
         expect(widened && editor.terrain.radiusCells == 5U &&
                    creativeEditorQuickEditStatusLabel(editor) == "RADIUS 5",
                "D-pad right widens radius and updates HUD");
}

bool editSampleRemoveAndUndoUseDocumentTruth() {
  cr::CreativeAppState appState;
  installDocument(appState, 401U);
  CreativeEditorState editor = terrainEditor(3, -2);
  editor.terrain.heightCells = 7U;
  editor.terrain.radiusCells = 5U;
  const bool placed =
      confirmCreativeEditorHeldItem(appState, editor, "test_terrain_place");
  const cr::CreativeTerrainControlPoint* stored =
      appState.facade.document().terrainField().controlAt({3, -2});
  bool ok = expect(placed && stored != nullptr &&
                       stored->heightCells == 7U && stored->radiusCells == 5U,
                   "X place stores one rod") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U,
                   "placement records one undo") &&
            expect(editor.interaction.placementFeedback.status ==
                       CreativeEditorPlacementFeedbackStatus::Placed,
                   "placement reports positive crosshair feedback");

  editor.terrain.heightCells = 2U;
  editor.terrain.radiusCells = 2U;
  const CreativeEditorTerrainEditReceipt sampled =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Sample,
          "test_terrain_sample");
  ok = expect(sampled.accepted && editor.terrain.heightCells == 7U &&
                  editor.terrain.radiusCells == 5U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "Square samples settings without history") &&
       ok;

  const CreativeEditorTerrainEditReceipt removed =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Remove,
          "test_terrain_remove");
  ok = expect(removed.accepted && removed.changed &&
                  appState.facade.document().terrainField().controlCount() == 0U &&
                  cr::creativeUndoDepth(appState.history) == 2U,
              "Circle removes rod and records history") &&
       ok;
  const bool undone = undoLastEdit(appState, "test_terrain_undo");
  return expect(undone &&
                    appState.facade.document().terrainField().controlAt({3, -2}) !=
                        nullptr,
                "undo restores authored rod") &&
         ok;
}

bool derivedSurfaceAndGuidesUseRevisionCaching() {
  cr::CreativeAppState appState;
  installDocument(appState, 402U);
  CreativeEditorState editor = terrainEditor(0, 0);
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  bool ok = expect(refreshCreativeEditorSceneCache(
                       cache, appState.facade.document(), grid) &&
                       cache.terrainSurfaceBuildCount == 1U,
                   "initial access builds empty terrain plan");
  for (std::int32_t x = 0; x < 8; ++x) {
    editor.interaction.target.grid.targetCell.x = x;
    std::vector<iggy3d::RenderCreativeWireframeDebugLine> guides;
    appendCreativeEditorTerrainOverlay(appState.facade.document(), editor,
                                       0.04F, guides);
    ok = expect(!guides.empty() &&
                    !refreshCreativeEditorSceneCache(
                        cache, appState.facade.document(), grid) &&
                    cache.terrainSurfaceBuildCount == 1U,
                "aim guides do not rebuild terrain") &&
         ok;
  }

  editor.interaction.target.grid.targetCell = {0, 0, 0};
  const CreativeEditorTerrainEditReceipt placed =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Upsert,
          "test_terrain_cache");
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> guides;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.04F,
                                     guides);
  const auto& surfaces = cache.preview.roomBake.room.spatialSurfaces;
  const bool hasWalkable = std::any_of(
      surfaces.begin(), surfaces.end(),
      [](const auto& surface) {
        return surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable;
      });
  const bool hasActorBlocker = std::any_of(
      surfaces.begin(), surfaces.end(),
      [](const auto& surface) { return surface.blocksActor; });
  const bool hasProjectileBlocker = std::any_of(
      surfaces.begin(), surfaces.end(),
      [](const auto& surface) { return surface.blocksProjectile; });
  return expect(placed.changed && refreshed &&
                    cache.terrainSurfaceBuildCount == 2U &&
                    !cache.terrainCuboids.empty(),
                "accepted rod rebuilds derived surface once") &&
         expect(cache.preview.roomBake.receipt.bakedVoxelCuboidCount > 0U,
                "terrain cuboids enter the room render and physics bake") &&
         expect(hasWalkable && hasActorBlocker && hasProjectileBlocker,
                "terrain tops walk and terrain columns block") &&
         expect(guides.size() >= 48U,
                "active tool shows stored rod and influence plus preview") &&
         expect(!refreshCreativeEditorSceneCache(
                    cache, appState.facade.document(), grid) &&
                    cache.terrainSurfaceBuildCount == 2U,
                "idle frame reuses rebuilt terrain") &&
         ok;
}

}  // namespace

int main() {
  return quickEditOwnsHeightAndRadiusWithoutNewBindings() &&
                 editSampleRemoveAndUndoUseDocumentTruth() &&
                 derivedSurfaceAndGuidesUseRevisionCaching()
             ? 0
             : 1;
}
