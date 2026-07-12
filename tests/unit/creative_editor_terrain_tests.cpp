#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/collision/CollisionQuery.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

bool approx(float lhs, float rhs, float epsilon = 0.001F) {
  return lhs >= rhs - epsilon && lhs <= rhs + epsilon;
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

CreativeEditorState terrainGradeEditor(std::int32_t x, std::int32_t z) {
  CreativeEditorState editor = terrainEditor(x, z);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::TerrainGrade;
  return editor;
}

CreativeEditorState terrainSculptEditor(std::int32_t x, std::int32_t z) {
  CreativeEditorState editor = terrainEditor(x, z);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::TerrainSculpt;
  return editor;
}

CreativeEditorState terrainProfileEditor(std::int32_t x, std::int32_t z) {
  CreativeEditorState editor = terrainEditor(x, z);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::TerrainProfile;
  return editor;
}

cr::CreativeWorldActionFrame strokeAction(
    cr::CreativeWorldActionId action,
    bool down,
    bool pressed = false,
    bool released = false) {
  cr::CreativeWorldActionFrame frame;
  const std::size_t index = static_cast<std::size_t>(action);
  frame.down[index] = down;
  frame.pressed[index] = pressed;
  frame.released[index] = released;
  return frame;
}

void setTerrainStrokeTarget(CreativeEditorState& editor,
                            std::int32_t x,
                            std::int32_t z) {
  editor.interaction.target = {};
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.targetCell = {x, 0, z};
  editor.terrain.hoverValid = false;
}

bool quickEditOwnsHeightAndRadiusWithoutNewBindings() {
  CreativeEditorState editor = terrainEditor(0, 0);
  const bool raised = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditPrevious);
  const std::uint16_t heightAfterRaise = editor.terrain.heightCells;
  const bool lowered = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditNext);
  const bool widened = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditIncrease);

  return expect(raised && heightAfterRaise == 5U,
                "D-pad up raises terrain height") &&
         expect(lowered && editor.terrain.heightCells == 4U,
                "D-pad down lowers terrain height") &&
         expect(widened && editor.terrain.radiusCells == 5U &&
                    creativeEditorQuickEditStatusLabel(editor) ==
                        "HEIGHT 4 | RADIUS 5",
                "D-pad right widens radius while HUD shows both values");
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

  WorldRay rodRay;
  rodRay.valid = true;
  rodRay.origin = {3.5F, 10.0F, -1.5F};
  rodRay.direction = {0.0F, -1.0F, 0.0F};
  updateCreativeEditorTerrainAim(editor.terrain, appState.facade.document(),
                                 rodRay, 3.0F);
  editor.terrain.heightCells = 2U;
  editor.terrain.radiusCells = 2U;
  const CreativeEditorTerrainEditReceipt sampled =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Sample,
          "test_terrain_sample");
  ok = expect(editor.terrain.hoverValid &&
                  editor.terrain.hoverCoord == cr::CreativeTerrainCoord2{3, -2},
              "center ray highlights the nearest authored rod") &&
       expect(sampled.accepted && editor.terrain.selectionValid &&
                  editor.terrain.selectedCoord ==
                      cr::CreativeTerrainCoord2{3, -2} &&
                  editor.terrain.heightCells == 7U &&
                  editor.terrain.radiusCells == 5U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "Square selects and samples without history") &&
       ok;

  static_cast<void>(processCreativeEditorTerrainQuickEdit(
      editor.terrain, cr::CreativeInputActionId::QuickEditPrevious));
  const cr::CreativeTerrainControlPoint* unchangedDraft =
      appState.facade.document().terrainField().controlAt({3, -2});
  ok = expect(editor.terrain.heightCells == 8U &&
                  unchangedDraft != nullptr &&
                  unchangedDraft->heightCells == 7U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "D-pad changes selected draft without mutating document truth") &&
       ok;

  const CreativeEditorTerrainEditReceipt cancelled =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Remove,
          "test_terrain_cancel");
  ok = expect(cancelled.accepted && cancelled.changed &&
                  !editor.terrain.selectionValid &&
                  editor.terrain.heightCells == 7U &&
                  appState.facade.document().terrainField().controlCount() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "Circle cancels a selected draft without history or deletion") &&
       ok;

  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      appState, editor, CreativeEditorTerrainEditKind::Sample,
      "test_terrain_reselect"));
  static_cast<void>(processCreativeEditorTerrainQuickEdit(
      editor.terrain, cr::CreativeInputActionId::QuickEditPrevious));
  clearCreativeEditorTerrainInteraction(editor.terrain,
                                        appState.facade.document().id());
  ok = expect(!editor.terrain.selectionValid &&
                  editor.terrain.heightCells == 7U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "tool interruption cancels and restores the selected draft") &&
       ok;
  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      appState, editor, CreativeEditorTerrainEditKind::Sample,
      "test_terrain_final_select"));
  static_cast<void>(processCreativeEditorTerrainQuickEdit(
      editor.terrain, cr::CreativeInputActionId::QuickEditPrevious));
  const CreativeEditorTerrainEditReceipt committed =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Upsert,
          "test_terrain_commit");
  const cr::CreativeTerrainControlPoint* updated =
      appState.facade.document().terrainField().controlAt({3, -2});
  ok = expect(committed.accepted && committed.changed &&
                  !editor.terrain.selectionValid && updated != nullptr &&
                  updated->heightCells == 8U &&
                  cr::creativeUndoDepth(appState.history) == 2U,
              "X commits one selected draft as exactly one undo entry") &&
       ok;

  updateCreativeEditorTerrainAim(editor.terrain, appState.facade.document(),
                                 rodRay, 2.0F);
  const CreativeEditorTerrainEditReceipt removed =
      applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Remove,
          "test_terrain_remove");
  ok = expect(removed.accepted && removed.changed &&
                  appState.facade.document().terrainField().controlCount() == 0U &&
                  cr::creativeUndoDepth(appState.history) == 3U,
              "Circle without a selected draft removes the highlighted rod") &&
       ok;
  const bool undone = undoLastEdit(appState, "test_terrain_undo");
  const cr::CreativeTerrainControlPoint* restored =
      appState.facade.document().terrainField().controlAt({3, -2});
  return expect(undone && restored != nullptr && restored->heightCells == 8U,
                "undo restores the committed authored rod") &&
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
  const bool hasHeightPatch = std::any_of(
      surfaces.begin(), surfaces.end(),
      [](const auto& surface) {
        return surface.shape ==
               iggy3d::RoomSpatialSurfaceShape::HeightPatch;
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
                "terrain cuboids enter the room render bake") &&
         expect(cache.preview.roomBake.receipt.usedSmoothTerrainCollision &&
                    cache.preview.roomBake.receipt
                            .bakedTerrainCliffBlockerCount > 0U &&
                    hasWalkable && hasHeightPatch && hasActorBlocker &&
                    hasProjectileBlocker,
                "terrain patches walk while exposed cliffs block") &&
         expect(guides.size() >= 48U,
                "active tool shows stored rod and influence plus preview") &&
         expect(!refreshCreativeEditorSceneCache(
                    cache, appState.facade.document(), grid) &&
                    cache.terrainSurfaceBuildCount == 2U,
                "idle frame reuses rebuilt terrain") &&
         ok;
}

bool semanticActionsRouteSelectionCommitCancelAndRemoval() {
  cr::CreativeAppState appState;
  installDocument(appState, 403U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainEditor(0, 0);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, 800U, 600U, now, false});
  };
  const auto gesture = [&](cr::CreativeWorldActionId action,
                           std::uint64_t now) {
    process(strokeAction(action, true, true), now);
    process(strokeAction(action, false, false, true), now + 1U);
  };

  gesture(cr::CreativeWorldActionId::Pick, 0U);
  bool ok = expect(editor.terrain.hoverValid &&
                       editor.terrain.selectionValid &&
                       editor.terrain.selectedCoord ==
                           cr::CreativeTerrainCoord2{0, 0},
                   "Square routes through hover into terrain selection");
  static_cast<void>(processCreativeEditorTerrainQuickEdit(
      editor.terrain, cr::CreativeInputActionId::QuickEditPrevious));
  gesture(cr::CreativeWorldActionId::Accept, 2U);
  const cr::CreativeTerrainControlPoint* committed =
      appState.facade.document().terrainField().controlAt({0, 0});
  ok = expect(committed != nullptr && committed->heightCells == 5U &&
                  !editor.terrain.selectionValid &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "X commits selected terrain draft through semantic dispatcher") &&
       ok;

  gesture(cr::CreativeWorldActionId::Pick, 4U);
  static_cast<void>(processCreativeEditorTerrainQuickEdit(
      editor.terrain, cr::CreativeInputActionId::QuickEditPrevious));
  gesture(cr::CreativeWorldActionId::Reject, 6U);
  const cr::CreativeTerrainControlPoint* afterCancel =
      appState.facade.document().terrainField().controlAt({0, 0});
  ok = expect(afterCancel != nullptr && afterCancel->heightCells == 5U &&
                  !editor.terrain.selectionValid &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "Circle cancels selected terrain draft through semantic dispatcher") &&
       ok;

  gesture(cr::CreativeWorldActionId::Reject, 8U);
  return expect(appState.facade.document().terrainField().controlCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 2U,
                "Circle removes highlighted rod when no draft is selected") &&
         ok;
}

bool terrainPaintStrokeRepeatsDeduplicatesCachesAndGroupsUndo() {
  cr::CreativeAppState appState;
  installDocument(appState, 407U);
  CreativeEditorState editor = terrainEditor(0, 0);
  editor.terrain.heightCells = 6U;
  editor.terrain.radiusCells = 3U;
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  bool ok = expect(refreshCreativeEditorSceneCache(
                       cache, appState.facade.document(), grid) &&
                       cache.terrainSurfaceBuildCount == 1U,
                   "stroke test begins from one empty terrain bake");

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  const bool firstRefresh = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  const cr::CreativeTerrainControlPoint* first =
      appState.facade.document().terrainField().controlAt({0, 0});
  ok = expect(first != nullptr && first->heightCells == 6U &&
                  editor.terrain.stroke.transaction.active &&
                  editor.terrain.stroke.visitedCount == 1U &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "X mutates immediately while history remains open") &&
       expect(firstRefresh && cache.terrainSurfaceBuildCount == 2U,
              "first accepted stroke mutation refreshes terrain once") &&
       ok;

  setTerrainStrokeTarget(editor, 1, 0);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds - 1U);
  ok = expect(appState.facade.document().terrainField().controlAt({1, 0}) ==
                  nullptr &&
                  !refreshCreativeEditorSceneCache(
                      cache, appState.facade.document(), grid),
              "held X does not repeat before 200 ms") &&
       ok;

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  const bool secondRefresh = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  const std::uint64_t revisionAfterSecond =
      appState.facade.document().revision();
  ok = expect(appState.facade.document().terrainField().controlAt({1, 0}) !=
                  nullptr &&
                  editor.terrain.stroke.acceptedMutationCount == 2U &&
                  editor.terrain.stroke.visitedCount == 2U,
              "held X paints a moved target at 200 ms") &&
       expect(secondRefresh && cache.terrainSurfaceBuildCount == 3U,
              "second accepted stroke mutation refreshes once") &&
       ok;

  setTerrainStrokeTarget(editor, 0, 0);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(appState.facade.document().revision() == revisionAfterSecond &&
                  !refreshCreativeEditorSceneCache(
                      cache, appState.facade.document(), grid),
              "revisiting a stroke coordinate does not mutate or rebake") &&
       ok;

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  const bool grouped = !editor.terrain.stroke.repeat.active &&
                       !editor.terrain.stroke.transaction.active &&
                       cr::creativeUndoDepth(appState.history) == 1U;
  const bool undone = undoLastEdit(appState, "test_terrain_stroke_undo");
  return expect(grouped,
                "release commits the complete paint stroke as one undo") &&
         expect(undone &&
                    appState.facade.document().terrainField().controlCount() ==
                        0U,
                "one undo removes every rod painted by the gesture") &&
         ok;
}

bool terrainSeedPreviewsStampsClearsAndGroupsHistory() {
  cr::CreativeAppState appState;
  installDocument(appState, 417U);
  CreativeEditorState editor = terrainEditor(0, 0);
  editor.terrain.selectionValid = true;
  editor.terrain.selectedOriginal = {{0, 0}, 6U, 3U};
  editor.terrain.heightCells = 9U;
  editor.terrain.radiusCells = 8U;
  editor.toolOptions.open = true;
  editor.toolOptions.targetEntry = editor.interaction.hotbar.entries[0];
  editor.toolOptions.draft = editor.toolSettings;
  editor.toolOptions.draft.terrainRodStampMode =
      cr::CreativeTerrainRodStampMode::Seed;
  editor.toolOptions.draft.terrainSeedRadius =
      cr::CreativeTerrainSeedRadius::TwoCells;
  editor.toolOptions.draft.terrainSeedSpacing =
      cr::CreativeTerrainSeedSpacing::TwoCells;
  editor.toolOptions.options = creativeEditorToolOptionsForEntry(
      editor.toolOptions.targetEntry, editor.toolOptions.draft);
  const bool optionsCommitted =
      activateCreativeEditorToolOptionsSelection(editor);

  std::vector<iggy3d::RenderCreativeWireframeDebugLine> previewLines;
  const std::uint64_t revisionBeforePreview =
      appState.facade.document().revision();
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     previewLines);
  bool ok = expect(optionsCommitted && !editor.terrain.selectionValid &&
                       editor.terrain.heightCells == 6U &&
                       editor.terrain.radiusCells == 3U &&
                       editor.toolOptions.options.count == 3U,
                   "entering Seed cancels a draft and exposes seed settings") &&
            expect(previewLines.size() > 60U &&
                       appState.facade.document().revision() ==
                           revisionBeforePreview,
                   "seed preview shows exact planned rods without mutation") &&
            expect(creativeEditorHeldItemStatusLabel(editor).find(
                       "SEED RADIUS 2 | SPACING 2") != std::string::npos,
                   "terrain HUD exposes seed footprint and lattice spacing");

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  const std::uint64_t revisionAfterSeed =
      appState.facade.document().revision();
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true), 200'000'000U);
  const cr::CreativeTerrainControlPoint* center =
      appState.facade.document().terrainField().controlAt({0, 0});
  ok = expect(appState.facade.document().terrainField().controlCount() == 5U &&
                  center != nullptr && center->heightCells == 6U &&
                  center->radiusCells == 3U &&
                  appState.facade.document().revision() == revisionAfterSeed &&
                  editor.terrain.stroke.acceptedMutationCount == 1U &&
                  editor.terrain.stroke.visitedCount == 1U,
              "X seeds one canonical disk and stationary repeat is deduplicated") &&
       ok;
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
      200'000'001U);
  ok = expect(cr::creativeUndoDepth(appState.history) == 1U &&
                  !editor.terrain.stroke.transaction.active,
              "seed hold commits one history record") &&
       ok;

  editor.terrain.selectionValid = true;
  editor.terrain.selectedCoord = {0, 0};
  editor.terrain.selectedOriginal = *center;
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, true, true),
      300'000'000U);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, false, false, true),
      300'000'001U);
  ok = expect(appState.facade.document().terrainField().controlCount() == 0U &&
                  cr::creativeUndoDepth(appState.history) == 2U &&
                  !editor.terrain.selectionValid,
              "Circle clears the seed disk even when a prior draft was selected") &&
       ok;
  ok = expect(undoLastEdit(appState, "test_terrain_seed_clear_undo") &&
                  appState.facade.document().terrainField().controlCount() == 5U,
              "undo restores the cleared seed disk") &&
       ok;
  return expect(undoLastEdit(appState, "test_terrain_seed_stamp_undo") &&
                    appState.facade.document().terrainField().controlCount() ==
                        0U,
                "second undo removes the original seed stamp") &&
         ok;
}

bool terrainCancelGestureCannotFallThroughIntoEraseStroke() {
  cr::CreativeAppState appState;
  installDocument(appState, 408U);
  const std::array initial{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 4U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{1, 0}, 5U, 2U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(initial));
  appState.history = {};
  CreativeEditorState editor = terrainEditor(0, 0);
  editor.terrain.hoverValid = true;
  editor.terrain.hoverCoord = {0, 0};
  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      appState, editor, CreativeEditorTerrainEditKind::Sample,
      "test_terrain_stroke_select"));
  static_cast<void>(processCreativeEditorTerrainQuickEdit(
      editor.terrain, cr::CreativeInputActionId::QuickEditPrevious));

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, true, true), 0U);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  bool ok = expect(!editor.terrain.selectionValid &&
                       editor.terrain.stroke.cancelOnly &&
                       appState.facade.document().terrainField().controlCount() ==
                           2U &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "holding Circle after draft cancel cannot erase the rod");
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, false, false, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);

  editor.terrain.hoverValid = true;
  editor.terrain.hoverCoord = {0, 0};
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, true, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  editor.terrain.hoverValid = true;
  editor.terrain.hoverCoord = {1, 0};
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, true),
      3U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Reject, false, false, true),
      3U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  const bool groupedErase =
      appState.facade.document().terrainField().controlCount() == 0U &&
      cr::creativeUndoDepth(appState.history) == 1U;
  const bool undone = undoLastEdit(appState, "test_terrain_erase_stroke_undo");
  return expect(groupedErase,
                "held Circle erases moved rod targets in one transaction") &&
         expect(undone &&
                    appState.facade.document().terrainField().controlCount() ==
                        2U,
                "one undo restores the complete erase stroke") &&
         ok;
}

bool terrainStrokeCapacityStopsFurtherMutation() {
  cr::CreativeAppState appState;
  installDocument(appState, 409U);
  CreativeEditorState editor = terrainEditor(0, 0);
  for (std::size_t index = 0U;
       index < kCreativeTerrainStrokeVisitedCapacity; ++index) {
    setTerrainStrokeTarget(editor, static_cast<std::int32_t>(index), 0);
    processCreativeTerrainStrokeFrame(
        appState, editor,
        strokeAction(cr::CreativeWorldActionId::Accept, true, index == 0U),
        index * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  }
  setTerrainStrokeTarget(
      editor, static_cast<std::int32_t>(kCreativeTerrainStrokeVisitedCapacity),
      0);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true),
      kCreativeTerrainStrokeVisitedCapacity *
          cr::kCreativeMaterialStrokeRepeatNanoseconds);
  const bool bounded =
      editor.terrain.stroke.capacityReached &&
      editor.terrain.stroke.visitedCount ==
          kCreativeTerrainStrokeVisitedCapacity &&
      editor.terrain.stroke.acceptedMutationCount ==
          kCreativeTerrainStrokeVisitedCapacity &&
      appState.facade.document().terrainField().controlCount() ==
          cr::kCreativeTerrainControlCapacity &&
      editor.interaction.placementFeedback.status ==
          CreativeEditorPlacementFeedbackStatus::Rejected;
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
      (kCreativeTerrainStrokeVisitedCapacity + 1U) *
          cr::kCreativeMaterialStrokeRepeatNanoseconds);
  const bool grouped = cr::creativeUndoDepth(appState.history) == 1U;
  const bool undone = undoLastEdit(appState, "test_terrain_capacity_undo");
  return expect(bounded,
                "terrain stroke stops and rejects at 256 unique coordinates") &&
         expect(grouped && undone &&
                    appState.facade.document().terrainField().controlCount() ==
                        0U,
                "capacity-bound stroke still records exactly one undo");
}

bool terrainStrokeInterruptionFinalizesChangedAndEmptyGestures() {
  cr::CreativeAppState appState;
  installDocument(appState, 410U);
  CreativeEditorState editor = terrainEditor(0, 0);
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  finalizeCreativeEditorContinuousGestures(
      appState, editor, "test_terrain_stroke_modal_interrupt");
  bool ok = expect(cr::creativeUndoDepth(appState.history) == 1U &&
                       !editor.terrain.stroke.repeat.active &&
                       !editor.terrain.stroke.transaction.active,
                   "modal interruption commits a changed terrain stroke");

  editor.interaction.target = {};
  editor.terrain.hoverValid = false;
  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  finalizeCreativeEditorContinuousGestures(
      appState, editor, "test_terrain_stroke_empty_interrupt");
  return expect(cr::creativeUndoDepth(appState.history) == 1U &&
                    !editor.terrain.stroke.repeat.active &&
                    !editor.terrain.stroke.transaction.active,
                "interrupted empty terrain gesture records no history") &&
         ok;
}

bool terrainGradeAnchorsPreviewsAppliesAndUndoesOneBatch() {
  cr::CreativeAppState appState;
  installDocument(appState, 411U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 2U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainGradeEditor(0, 0);
  editor.terrain.hoverValid = true;
  editor.terrain.hoverCoord = {0, 0};
  const CreativeEditorTerrainGradeReceipt anchored =
      beginCreativeEditorTerrainGrade(appState, editor);
  bool ok = expect(anchored.accepted && anchored.changed &&
                       editor.terrain.grade.anchorValid &&
                       editor.terrain.grade.anchorCoord ==
                           cr::CreativeTerrainCoord2{0, 0} &&
                       editor.terrain.grade.anchorHeightCells == 2U &&
                       editor.terrain.grade.targetHeightCells == 2U,
                   "Square anchors the exact authored start rod");

  static_cast<void>(processCreativeEditorTerrainGradeQuickEdit(
      editor.terrain.grade, cr::CreativeInputActionId::QuickEditPrevious));
  static_cast<void>(processCreativeEditorTerrainGradeQuickEdit(
      editor.terrain.grade, cr::CreativeInputActionId::QuickEditPrevious));
  static_cast<void>(processCreativeEditorTerrainGradeQuickEdit(
      editor.terrain.grade, cr::CreativeInputActionId::QuickEditIncrease));
  setTerrainStrokeTarget(editor, 4, 2);
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  static_cast<void>(refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid));
  const std::uint64_t buildsBeforePreview = cache.terrainSurfaceBuildCount;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> preview;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.04F,
                                     preview);
  ok = expect(!preview.empty() &&
                  !refreshCreativeEditorSceneCache(
                      cache, appState.facade.document(), grid) &&
                  cache.terrainSurfaceBuildCount == buildsBeforePreview,
              "grade preview is visible without mutating or rebuilding terrain") &&
       expect(creativeEditorTerrainGradeQuickEditLabel(editor.terrain.grade) ==
                  "END HEIGHT 4 | WIDTH 11",
              "D-pad edits endpoint height and width without new bindings") &&
       ok;

  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const CreativeEditorTerrainGradeReceipt applied =
      applyCreativeEditorTerrainGradeWithHistory(
          appState, editor, "test_terrain_grade_apply");
  const cr::CreativeTerrainControlPoint* middle =
      appState.facade.document().terrainField().controlAt({2, 1});
  const cr::CreativeTerrainControlPoint* end =
      appState.facade.document().terrainField().controlAt({4, 2});
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  ok = expect(applied.accepted && applied.changed &&
                  applied.plan.items().size() == 5U &&
                  !editor.terrain.grade.anchorValid &&
                  appState.facade.document().terrainField().controlCount() ==
                      5U &&
                  middle != nullptr && middle->heightCells == 3U &&
                  middle->radiusCells == 5U && end != nullptr &&
                  end->heightCells == 4U && end->radiusCells == 5U,
              "X applies the exact previewed grade controls") &&
       expect(appState.facade.document().revision() == revisionBefore + 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U && refreshed &&
                  cache.terrainSurfaceBuildCount == buildsBeforePreview + 1U,
              "grade advances revision and rebuilds once as one undo entry") &&
       ok;

  const bool undone = undoLastEdit(appState, "test_terrain_grade_undo");
  const cr::CreativeTerrainControlPoint* restored =
      appState.facade.document().terrainField().controlAt({0, 0});
  return expect(undone &&
                    appState.facade.document().terrainField().controlCount() ==
                        1U &&
                    restored != nullptr && restored->heightCells == 2U &&
                    restored->radiusCells == 2U,
                "one undo restores all controls replaced by the grade") &&
         ok;
}

bool terrainGradeCancelAndCapacityFailureDoNotCreateHistory() {
  cr::CreativeAppState appState;
  installDocument(appState, 412U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 3U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainGradeEditor(0, 0);
  editor.terrain.hoverValid = true;
  editor.terrain.hoverCoord = {0, 0};
  static_cast<void>(beginCreativeEditorTerrainGrade(appState, editor));
  static_cast<void>(processCreativeEditorTerrainGradeQuickEdit(
      editor.terrain.grade, cr::CreativeInputActionId::QuickEditPrevious));
  const CreativeEditorTerrainGradeReceipt cancelled =
      cancelCreativeEditorTerrainGrade(editor);
  bool ok = expect(cancelled.accepted && cancelled.changed &&
                       !editor.terrain.grade.anchorValid &&
                       appState.facade.document().terrainField().controlCount() ==
                           1U &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "Circle cancels grade setup without document history");

  editor.terrain.hoverValid = true;
  editor.terrain.hoverCoord = {0, 0};
  static_cast<void>(beginCreativeEditorTerrainGrade(appState, editor));
  setTerrainStrokeTarget(editor, 256, 0);
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const CreativeEditorTerrainGradeReceipt rejected =
      applyCreativeEditorTerrainGradeWithHistory(
          appState, editor, "test_terrain_grade_capacity");
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.plan.status ==
                        cr::CreativeTerrainGradePlanStatus::CapacityExceeded &&
                    editor.terrain.grade.anchorValid &&
                    appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U &&
                    editor.interaction.placementFeedback.status ==
                        CreativeEditorPlacementFeedbackStatus::Rejected,
                "over-capacity grade fails before mutation and remains editable") &&
         ok;
}

bool terrainGradeRoutesSquareXAndCircleThroughWorldActions() {
  cr::CreativeAppState appState;
  installDocument(appState, 413U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 2U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainGradeEditor(0, 0);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, 800U, 600U, now, false});
  };
  const auto gesture = [&](cr::CreativeWorldActionId action,
                           std::uint64_t now) {
    process(strokeAction(action, true, true), now);
    process(strokeAction(action, false, false, true), now + 1U);
  };

  gesture(cr::CreativeWorldActionId::Pick, 0U);
  bool ok = expect(editor.terrain.grade.anchorValid &&
                       editor.terrain.grade.anchorCoord ==
                           cr::CreativeTerrainCoord2{0, 0},
                   "PS5 Square routes to grade start selection");
  static_cast<void>(processCreativeEditorTerrainGradeQuickEdit(
      editor.terrain.grade, cr::CreativeInputActionId::QuickEditPrevious));
  static_cast<void>(processCreativeEditorTerrainGradeQuickEdit(
      editor.terrain.grade, cr::CreativeInputActionId::QuickEditPrevious));
  camera.worldEye.x = 2.5F;
  gesture(cr::CreativeWorldActionId::Accept, 2U);
  const cr::CreativeTerrainControlPoint* endpoint =
      appState.facade.document().terrainField().controlAt({2, 0});
  ok = expect(endpoint != nullptr && endpoint->heightCells == 4U &&
                  !editor.terrain.grade.anchorValid &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "PS5 X routes to one grade batch") &&
       ok;

  camera.worldEye.x = 0.5F;
  gesture(cr::CreativeWorldActionId::Pick, 4U);
  gesture(cr::CreativeWorldActionId::Reject, 6U);
  return expect(!editor.terrain.grade.anchorValid &&
                    appState.facade.document().terrainField().controlCount() ==
                        3U &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "PS5 Circle cancels grade setup without mutation") &&
         ok;
}

bool terrainSculptSamplesFlattensAndUndoesOneBatch() {
  cr::CreativeAppState appState;
  installDocument(appState, 414U);
  const std::array initial{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 0}, 8U, 2U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(initial));
  appState.history = {};
  CreativeEditorState editor = terrainSculptEditor(1, 0);

  const CreativeEditorTerrainSculptReceipt sampled =
      sampleCreativeEditorTerrainSculptHeight(appState.facade.document(),
                                              editor);
  const bool stronger = processCreativeEditorTerrainSculptQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditPrevious);
  const bool narrowerFirst = processCreativeEditorTerrainSculptQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditDecrease);
  const bool narrowerSecond = processCreativeEditorTerrainSculptQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditDecrease);
  const CreativeEditorTerrainSculptReceipt applied =
      applyCreativeEditorTerrainSculptWithHistory(
          appState, editor, "test_terrain_sculpt_flatten");
  const cr::CreativeTerrainControlPoint* left =
      appState.facade.document().terrainField().controlAt({0, 0});
  const cr::CreativeTerrainControlPoint* right =
      appState.facade.document().terrainField().controlAt({2, 0});
  bool ok = expect(sampled.accepted && sampled.changed &&
                       editor.terrain.sculpt.targetHeightCells == 5U,
                   "Square samples the derived blended terrain height") &&
            expect(stronger && narrowerFirst && narrowerSecond &&
                       editor.toolSettings.terrainSculptStrength ==
                           cr::CreativeTerrainSculptStrength::TwoCells &&
                       editor.toolSettings.terrainSculptRadius ==
                           cr::CreativeTerrainSculptRadius::OneCell &&
                       creativeEditorTerrainSculptQuickEditLabel(editor) ==
                           "FLATTEN | RADIUS 1 | STRENGTH 2 | FALLOFF UNIFORM | TARGET 5",
                   "D-pad controls expose bounded sculpt strength and radius") &&
            expect(applied.accepted && applied.changed &&
                       applied.plan.editCount == 2U && left != nullptr &&
                       left->heightCells == 4U && left->radiusCells == 2U &&
                       right != nullptr && right->heightCells == 6U &&
                       right->radiusCells == 2U &&
                       cr::creativeUndoDepth(appState.history) == 1U,
                   "flatten edits existing rod heights atomically and preserves radii");
  ok = expect(undoLastEdit(appState, "test_terrain_sculpt_undo"),
              "sculpt batch can be undone") &&
       ok;
  left = appState.facade.document().terrainField().controlAt({0, 0});
  right = appState.facade.document().terrainField().controlAt({2, 0});
  return expect(left != nullptr && left->heightCells == 2U &&
                    right != nullptr && right->heightCells == 8U,
                "one undo restores the complete sculpt batch") &&
         ok;
}

bool terrainSculptHoldRepeatsAndCommitsOneUndo() {
  cr::CreativeAppState appState;
  installDocument(appState, 415U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 1U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainSculptEditor(0, 0);
  editor.terrain.sculpt.targetHeightCells = 10U;

  processCreativeTerrainSculptStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  const cr::CreativeTerrainControlPoint* control =
      appState.facade.document().terrainField().controlAt({0, 0});
  bool ok = expect(control != nullptr && control->heightCells == 2U &&
                       editor.terrain.sculpt.stroke.transaction.active &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "X applies immediately while history transaction stays lazy");
  processCreativeTerrainSculptStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true), 199'000'000U);
  control = appState.facade.document().terrainField().controlAt({0, 0});
  ok = expect(control != nullptr && control->heightCells == 2U,
              "hold does not repeat before 200 ms") &&
       ok;
  processCreativeTerrainSculptStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true), 200'000'000U);
  control = appState.facade.document().terrainField().controlAt({0, 0});
  ok = expect(control != nullptr && control->heightCells == 3U,
              "stationary hold repeats at 200 ms") &&
       ok;
  processCreativeTerrainSculptStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
      201'000'000U);
  ok = expect(!editor.terrain.sculpt.stroke.repeat.active &&
                  !editor.terrain.sculpt.stroke.transaction.active &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "release commits every repeated sculpt edit as one undo") &&
       ok;
  ok = expect(undoLastEdit(appState, "test_terrain_sculpt_hold_undo"),
              "held sculpt gesture undo applies") &&
       ok;
  control = appState.facade.document().terrainField().controlAt({0, 0});
  setTerrainStrokeTarget(editor, 100, 100);
  processCreativeTerrainSculptStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true),
      300'000'000U);
  processCreativeTerrainSculptStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
      301'000'000U);
  return expect(control != nullptr && control->heightCells == 1U &&
                    cr::creativeUndoDepth(appState.history) == 0U &&
                    !editor.terrain.sculpt.stroke.transaction.active,
                "brush with no existing rods rejects without history or densification") &&
         ok;
}

bool terrainSculptPreviewCachesAndUsesRuntimeSlopeBands() {
  cr::CreativeAppState appState;
  installDocument(appState, 416U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 2U, 4U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  CreativeEditorState editor = terrainSculptEditor(0, 0);
  const bool built = refreshCreativeEditorTerrainSculptPreview(
      editor.terrain, appState.facade.document(), editor);
  const bool reused = refreshCreativeEditorTerrainSculptPreview(
      editor.terrain, appState.facade.document(), editor);
  setTerrainStrokeTarget(editor, 1, 0);
  const bool moved = refreshCreativeEditorTerrainSculptPreview(
      editor.terrain, appState.facade.document(), editor);
  const bool radiusChanged = processCreativeEditorTerrainSculptQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  const bool rebuiltForRadius = refreshCreativeEditorTerrainSculptPreview(
      editor.terrain, appState.facade.document(), editor);
  editor.toolSettings.terrainSculptFalloff =
      cr::CreativeTerrainSculptFalloff::Linear;
  const bool rebuiltForFalloff = refreshCreativeEditorTerrainSculptPreview(
      editor.terrain, appState.facade.document(), editor);
  const cr::CreativeTerrainControlEdit raised{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 3U, 4U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&raised, 1U}));
  const bool rebuiltForRevision = refreshCreativeEditorTerrainSculptPreview(
      editor.terrain, appState.facade.document(), editor);
  bool ok = expect(built && !reused && moved && radiusChanged &&
                       rebuiltForRadius && rebuiltForFalloff &&
                       rebuiltForRevision &&
                       editor.terrain.sculpt.preview.buildCount == 5U &&
                       editor.terrain.sculpt.preview.falloff ==
                           cr::CreativeTerrainSculptFalloff::Linear,
                   "preview rebuilds only for aim settings or document revision") &&
            expect(editor.terrain.sculpt.preview.renderAccepted &&
                       !editor.terrain.sculpt.preview.patches.empty(),
                   "preview cache owns an admitted bounded terrain patch set");

  const auto planarPatch = [](std::int32_t x, double rise) {
    cr::CreativeTerrainSurfacePatch patch;
    patch.coord = {x, 0};
    const double minimumX = static_cast<double>(x);
    patch.center = {minimumX + 0.5, rise * 0.5, 0.5};
    patch.corners = {{{minimumX, 0.0, 0.0},
                      {minimumX + 1.0, rise, 0.0},
                      {minimumX + 1.0, rise, 1.0},
                      {minimumX, 0.0, 1.0}}};
    return patch;
  };
  CreativeTerrainSculptPreviewCache& preview =
      editor.terrain.sculpt.preview;
  preview.valid = true;
  preview.renderAccepted = true;
  preview.plan.accepted = true;
  preview.center = {0, 0};
  preview.radiusCells = 1U;
  preview.targetHeightCells = 4U;
  preview.patches = {planarPatch(0, 0.0), planarPatch(2, 0.364),
                     planarPatch(4, 1.192)};
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  appendCreativeEditorTerrainSculptOverlay(
      appState.facade.document(), editor, 0.1F, lines);
  const auto hasColor = [&lines](float red, float green) {
    return std::any_of(
        lines.begin(), lines.end(), [red, green](const auto& line) {
          return approx(line.color.r, red) && approx(line.color.g, green);
        });
  };
  return expect(hasColor(0.20F, 1.0F),
                "walkable sculpt preview triangles are green") &&
         expect(hasColor(1.0F, 0.82F),
                "careful-footing sculpt preview triangles are yellow") &&
         expect(hasColor(1.0F, 0.20F),
                "runtime-rejected sculpt preview triangles are red") &&
         ok;
}

bool terrainProfilePreviewApplyBaseLockAndUndoStayInParity() {
  cr::CreativeAppState appState;
  installDocument(appState, 417U);
  CreativeEditorState editor = terrainProfileEditor(0, 0);
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache sceneCache;
  const bool sceneBuilt = refreshCreativeEditorSceneCache(
      sceneCache, appState.facade.document(), grid);
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const bool previewBuilt = refreshCreativeEditorTerrainProfilePreview(
      editor.terrain, appState.facade.document(), editor);
  const cr::CreativeTerrainProfilePlan previewPlan =
      editor.terrain.profile.preview.plan;
  const bool previewReused = refreshCreativeEditorTerrainProfilePreview(
      editor.terrain, appState.facade.document(), editor);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> guides;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.08F,
                                     guides);

  bool ok = expect(sceneBuilt && previewBuilt && !previewReused &&
                       previewPlan.accepted && previewPlan.editCount == 49U &&
                       editor.terrain.profile.preview.renderAccepted &&
                       !editor.terrain.profile.preview.patches.empty(),
                   "profile preview builds once from exact bounded hill plan") &&
            expect(!guides.empty() &&
                       appState.facade.document().revision() ==
                           documentRevisionBefore &&
                       sceneCache.terrainSurfaceBuildCount == 1U,
                   "profile aiming draws guides without document or scene mutation");

  const CreativeEditorTerrainProfileReceipt applied =
      applyCreativeEditorTerrainProfileWithHistory(
          appState, editor, "test_terrain_profile_apply");
  const bool planParity =
      applied.plan.items().size() == previewPlan.items().size() &&
      std::equal(applied.plan.items().begin(), applied.plan.items().end(),
                 previewPlan.items().begin(),
                 [](const auto& lhs, const auto& rhs) {
                   return lhs.kind == rhs.kind && lhs.control == rhs.control;
                 });
  const bool sceneRefreshed = refreshCreativeEditorSceneCache(
      sceneCache, appState.facade.document(), grid);
  const bool sceneReused = refreshCreativeEditorSceneCache(
      sceneCache, appState.facade.document(), grid);
  ok = expect(applied.accepted && applied.changed && planParity &&
                  appState.facade.document().terrainField().controlCount() ==
                      49U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "X applies the exact preview as one Facade batch and undo") &&
       expect(sceneRefreshed && !sceneReused &&
                  sceneCache.terrainSurfaceBuildCount == 2U,
              "accepted profile stamp refreshes scene exactly once") &&
       ok;

  setTerrainStrokeTarget(editor, 0, 0);
  const cr::CreativeTerrainHeightSample expectedLockedSample =
      cr::sampleCreativeTerrainHeight(appState.facade.document().terrainField(),
                                      {0, 0});
  const CreativeEditorTerrainProfileReceipt locked =
      lockCreativeEditorTerrainProfileBase(appState.facade.document(), editor);
  setTerrainStrokeTarget(editor, 10, 0);
  const bool lockedPreview = refreshCreativeEditorTerrainProfilePreview(
      editor.terrain, appState.facade.document(), editor);
  const std::uint16_t lockedBase =
      editor.terrain.profile.resolvedBaseHeightCells;
  const CreativeEditorTerrainProfileReceipt unlocked =
      unlockCreativeEditorTerrainProfileBase(editor);
  const bool autoPreview = refreshCreativeEditorTerrainProfilePreview(
      editor.terrain, appState.facade.document(), editor);
  ok = expect(expectedLockedSample.present && locked.accepted &&
                  locked.changed &&
                  lockedBase == expectedLockedSample.heightCells &&
                  editor.terrain.profile.preview.resolvedBaseHeightCells == 4U &&
                  unlocked.accepted && unlocked.changed && lockedPreview &&
                  autoPreview,
              "Square locks sampled base and Circle restores aimed auto base") &&
       ok;

  const bool amplitudeRaised = processCreativeEditorTerrainProfileQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditPrevious);
  const bool radiusRaised = processCreativeEditorTerrainProfileQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  ok = expect(amplitudeRaised && radiusRaised &&
                  editor.toolSettings.terrainProfileAmplitude ==
                      cr::CreativeTerrainProfileAmplitude::EightCells &&
                  editor.toolSettings.terrainProfileRadius ==
                      cr::CreativeTerrainProfileRadius::EightCells &&
                  creativeEditorTerrainProfileQuickEditLabel(editor).find(
                      "AMP 8 | RADIUS 8") != std::string::npos,
              "D-pad quick edits own profile amplitude and radius") &&
       ok;

  const bool undone = undoLastEdit(appState, "test_terrain_profile_undo");
  return expect(undone &&
                    appState.facade.document().terrainField().controlCount() ==
                        0U,
                "one undo removes the complete profile stamp") &&
         ok;
}

bool terrainProfileWorldActionsArePressOnlyAndControllerNative() {
  cr::CreativeAppState appState;
  installDocument(appState, 418U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainProfileEditor(0, 0);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, 800U, 600U, now, false});
  };

  process(strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  const std::uint64_t revisionAfterPress =
      appState.facade.document().terrainField().revision();
  process(strokeAction(cr::CreativeWorldActionId::Accept, true), 200'000'000U);
  process(strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
          201'000'000U);
  bool ok = expect(revisionAfterPress > 1U &&
                       appState.facade.document().terrainField().revision() ==
                           revisionAfterPress &&
                       cr::creativeUndoDepth(appState.history) == 1U,
                   "PS5 X applies once and held frames never repeat profile") ;

  process(strokeAction(cr::CreativeWorldActionId::Pick, true, true),
          202'000'000U);
  process(strokeAction(cr::CreativeWorldActionId::Pick, false, false, true),
          203'000'000U);
  ok = expect(editor.terrain.profile.baseLocked,
              "PS5 Square locks profile base") &&
       ok;
  process(strokeAction(cr::CreativeWorldActionId::Reject, true, true),
          204'000'000U);
  process(strokeAction(cr::CreativeWorldActionId::Reject, false, false, true),
          205'000'000U);
  return expect(!editor.terrain.profile.baseLocked &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "PS5 Circle restores auto base without history") &&
         ok;
}

bool terrainProfileRejectionIsVisibleAtomicAndModalSafe() {
  cr::CreativeAppState appState;
  installDocument(appState, 419U);
  CreativeEditorState editor = terrainProfileEditor(0, 0);
  editor.toolSettings.terrainProfileKind =
      cr::CreativeTerrainProfileKind::Ripple;
  editor.toolSettings.terrainProfileRadius =
      cr::CreativeTerrainProfileRadius::TwoCells;
  const bool built = refreshCreativeEditorTerrainProfilePreview(
      editor.terrain, appState.facade.document(), editor);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> rejectedLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.08F,
                                     rejectedLines);
  const bool hasRed = std::any_of(
      rejectedLines.begin(), rejectedLines.end(), [](const auto& line) {
        return approx(line.color.r, 1.0F) && approx(line.color.g, 0.20F);
      });
  const CreativeEditorTerrainProfileReceipt rejected =
      applyCreativeEditorTerrainProfileWithHistory(
          appState, editor, "test_terrain_profile_rejected");
  editor.catalog.model.open = true;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> modalLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.08F,
                                     modalLines);
  editor.catalog.model.open = false;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> captureLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.08F,
                                     captureLines, true);
  return expect(built && !editor.terrain.profile.preview.plan.accepted &&
                    editor.terrain.profile.preview.plan.status ==
                        cr::CreativeTerrainProfilePlanStatus::UnderSampled &&
                    hasRed,
                "under-sampled ripple produces red bounded feedback") &&
         expect(!rejected.accepted && !rejected.changed &&
                    appState.facade.document().terrainField().controlCount() ==
                        0U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "rejected profile creates no mutation or history") &&
         expect(modalLines.empty() && captureLines.empty(),
                "catalog and capture ownership hide profile preview completely");
}

bool bentSurfacePatchesReachRendererAndRefreshWithHeight() {
  cr::CreativeAppState appState;
  installDocument(appState, 404U);
  const std::array edits{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 0}, 8U, 2U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(edits));
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  const iggy3d::vulkan::RoomMeshCpuGeometry first =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(cache.preview.scene.room);
  const bool hasBentPatch = std::any_of(
      cache.terrainSurfacePatches.begin(), cache.terrainSurfacePatches.end(),
      [](const iggy3d::SceneRoomSurfacePatchItem& patch) {
        return patch.corners[0].y != patch.corners[1].y ||
               patch.corners[1].y != patch.corners[2].y ||
               patch.corners[2].y != patch.corners[3].y;
      });
  bool ok = expect(refreshed && !cache.terrainSurfacePatches.empty() &&
                       cache.terrainCollisionPatches.size() ==
                           cache.terrainSurfacePatches.size() &&
                       cache.preview.scene.room.surfacePatches.size() ==
                           cache.terrainSurfacePatches.size(),
                   "scene cache attaches derived terrain patches") &&
            expect(hasBentPatch, "unequal rods bend at least one rendered tile") &&
            expect(first.ready && first.roomFloorDrawCount == 1U &&
                       first.vertices.size() ==
                           cache.terrainSurfacePatches.size() * 5U &&
                       first.indices.size() ==
                           cache.terrainSurfacePatches.size() * 24U,
                   "renderer emits one batched terrain draw without stepped duplicates");

  const cr::CreativeTerrainControlEdit raised{
      cr::CreativeTerrainEditKind::Upsert, {{2, 0}, 9U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&raised, 1U}));
  const bool refreshedAfterRaise = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  const iggy3d::vulkan::RoomMeshCpuGeometry second =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(cache.preview.scene.room);
  return expect(refreshedAfterRaise && cache.terrainSurfaceBuildCount == 2U &&
                    second.ready && second.sourceRoomGeometrySignature !=
                                        first.sourceRoomGeometrySignature,
                "accepted height change rebuilds and re-signatures bent geometry once") &&
         ok;
}

bool smoothTerrainCollisionMatchesRenderedTriangle() {
  cr::CreativeAppState appState;
  installDocument(appState, 412U);
  const std::array edits{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 0}, 8U, 2U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(edits));
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  if (!expect(refreshCreativeEditorSceneCache(
                  cache, appState.facade.document(), grid),
              "smooth collision scene refreshes")) {
    return false;
  }

  const cr::CreativeTerrainSurfacePatch* selectedPatch = nullptr;
  std::size_t selectedCorner = 0U;
  for (const cr::CreativeTerrainSurfacePatch& patch :
       cache.terrainCollisionPatches) {
    for (std::size_t corner = 0U; corner < patch.corners.size(); ++corner) {
      const cr::CreativeVec3 first = patch.corners[corner];
      const cr::CreativeVec3 second =
          patch.corners[(corner + 1U) % patch.corners.size()];
      if (patch.center.y != first.y || patch.center.y != second.y) {
        selectedPatch = &patch;
        selectedCorner = corner;
        break;
      }
    }
    if (selectedPatch != nullptr) {
      break;
    }
  }
  if (!expect(selectedPatch != nullptr,
              "unequal rods expose a bent collision triangle")) {
    return false;
  }

  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(selectedPatch->center);
  const cr::CreativeCoreVec3Conversion first =
      cr::creativeVec3ToCoreChecked(selectedPatch->corners[selectedCorner]);
  const cr::CreativeCoreVec3Conversion second =
      cr::creativeVec3ToCoreChecked(
          selectedPatch->corners[(selectedCorner + 1U) % 4U]);
  if (!expect(center.converted && first.converted && second.converted,
              "selected terrain triangle converts to runtime coordinates")) {
    return false;
  }
  const iggy3d::Vec3 samplePoint =
      (center.value + first.value + second.value) / 3.0F;
  iggy3d::Vec3 expectedNormal;
  if (!expect(iggy3d::tryNormalize(
                  iggy3d::cross(first.value - center.value,
                                second.value - center.value),
                  expectedNormal),
              "selected terrain triangle has a finite normal")) {
    return false;
  }
  if (expectedNormal.y < 0.0F) {
    expectedNormal = expectedNormal * -1.0F;
  }

  const iggy3d::SpatialSurfaceSet collisionSurfaces =
      iggy3d::buildSpatialSurfaceSet(cache.preview.roomBake.room);
  const iggy3d::CollisionQueryResult sampled = iggy3d::sampleSurfaceHeight(
      collisionSurfaces, {samplePoint.x, 0.0F, samplePoint.z}, 0.0F);
  const iggy3d::CollisionQueryResult verticalHit = iggy3d::querySegment(
      collisionSurfaces, samplePoint + iggy3d::Vec3{0.0F, 2.0F, 0.0F},
      samplePoint - iggy3d::Vec3{0.0F, 2.0F, 0.0F},
      iggy3d::CollisionQueryKind::Walkable);
  const iggy3d::CollisionQueryResult projectileHit = iggy3d::querySegment(
      collisionSurfaces, samplePoint + iggy3d::Vec3{0.0F, 2.0F, 0.0F},
      samplePoint - iggy3d::Vec3{0.0F, 2.0F, 0.0F},
      iggy3d::CollisionQueryKind::Projectile);
  const cr::CreativeRoomBakeReceipt& receipt = cache.preview.roomBake.receipt;
  return expect(receipt.usedSmoothTerrainCollision &&
                    receipt.bakedTerrainSurfacePatchCount ==
                        cache.terrainCollisionPatches.size(),
                "room bake owns every admitted smooth collision patch") &&
         expect(sampled.status == iggy3d::CollisionQueryStatus::Hit &&
                    sampled.shape ==
                        iggy3d::CollisionSurfaceShape::HeightPatch &&
                    approx(sampled.heightMeters, samplePoint.y) &&
                    iggy3d::nearlyEqual(sampled.normal, expectedNormal, 0.001F),
                "height sampling matches the rendered triangle height and normal") &&
         expect(verticalHit.status == iggy3d::CollisionQueryStatus::Hit &&
                    verticalHit.shape ==
                        iggy3d::CollisionSurfaceShape::HeightPatch &&
                    approx(verticalHit.pointMeters.y, samplePoint.y) &&
                    iggy3d::nearlyEqual(verticalHit.normal, expectedNormal,
                                        0.001F),
                "vertical collision query hits the same terrain triangle") &&
         expect(projectileHit.status == iggy3d::CollisionQueryStatus::Hit &&
                    projectileHit.shape ==
                        iggy3d::CollisionSurfaceShape::HeightPatch &&
                    approx(projectileHit.pointMeters.y, samplePoint.y),
                "smooth terrain top remains projectile-solid");
}

bool gridChangeRebuildsWorldSpaceTerrainPatches() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Grid Terrain");
  static_cast<void>(document.assignId(406U));
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, {{1, -2}, 6U, 2U}};
  static_cast<void>(document.applyTerrainControlEdits(
      std::span{&edit, 1U}));
  const std::uint64_t terrainRevision = document.terrainField().revision();
  iggy3d::ProductMapMakerGridSnapshot gridSnapshot;
  CreativeEditorSceneCache cache;
  const bool initialRefresh = refreshCreativeEditorSceneCache(
      cache, document, gridSnapshot);
  if (!expect(initialRefresh && !cache.terrainSurfacePatches.empty() &&
                  cache.terrainSurfaceBuildCount == 1U,
              "initial grid builds world-space terrain patches")) {
    return false;
  }
  const iggy3d::Vec3 initialCenter = cache.terrainSurfacePatches.front().center;

  cr::CreativeGridSettings grid = document.gridSettings();
  grid.origin = {10.0, 3.0, -4.0};
  grid.cellSizeMeters = 2.0;
  const bool gridChanged = document.setGridSettings(grid);
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, document, gridSnapshot);
  const cr::CreativeTerrainRenderPlan expected =
      cr::buildCreativeTerrainRenderPlan(document.terrainField(), grid.origin,
                                         grid.cellSizeMeters);
  const cr::CreativeCoreVec3Conversion expectedCenter =
      expected.patches.empty()
          ? cr::CreativeCoreVec3Conversion{}
          : cr::creativeVec3ToCoreChecked(expected.patches.front().center);
  const iggy3d::Vec3 actualCenter = cache.terrainSurfacePatches.empty()
                                        ? iggy3d::Vec3{}
                                        : cache.terrainSurfacePatches.front().center;
  return expect(gridChanged && refreshed &&
                    document.terrainField().revision() == terrainRevision &&
                    cache.terrainSurfaceBuildCount == 2U,
                "grid-only change rebuilds terrain without a terrain edit") &&
         expect(expected.accepted && expectedCenter.converted &&
                    actualCenter.x == expectedCenter.value.x &&
                    actualCenter.y == expectedCenter.value.y &&
                    actualCenter.z == expectedCenter.value.z &&
                    (actualCenter.x != initialCenter.x ||
                     actualCenter.y != initialCenter.y ||
                     actualCenter.z != initialCenter.z),
                "rebuilt patch uses the current grid origin and cell size");
}

bool oversizedBentSurfaceFallsBackToTerrainPlanes() {
  cr::CreativeAppState appState;
  installDocument(appState, 405U);
  std::vector<cr::CreativeTerrainControlEdit> edits;
  for (std::int32_t index = 0; index < 11; ++index) {
    edits.push_back({cr::CreativeTerrainEditKind::Upsert,
                     {{index * 40, 0}, 4U,
                      cr::kCreativeTerrainMaximumRadiusCells}});
  }
  static_cast<void>(appState.facade.applyTerrainControlEdits(edits));
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document(), grid);
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(cache.preview.scene.room);
  const bool hasHeightPatch = std::any_of(
      cache.preview.roomBake.room.spatialSurfaces.begin(),
      cache.preview.roomBake.room.spatialSurfaces.end(),
      [](const iggy3d::RoomSpatialSurface& surface) {
        return surface.shape ==
               iggy3d::RoomSpatialSurfaceShape::HeightPatch;
      });
  return expect(refreshed && cache.terrainSurfacePatches.empty() &&
                    cache.terrainCollisionPatches.empty() &&
                    !cache.terrainCuboids.empty(),
                "over-budget bent plan retains stepped fallback data") &&
         expect(!cache.preview.roomBake.receipt.usedSmoothTerrainCollision &&
                    cache.preview.roomBake.receipt
                            .bakedTerrainSurfacePatchCount == 0U &&
                    !hasHeightPatch,
                "over-budget plan keeps the column collision fallback") &&
         expect(geometry.ready && geometry.roomFloorDrawCount > 1U,
                "renderer draws terrain fallback planes instead of dropping it");
}

bool invalidSmoothPatchInputFallsBackAtomically() {
  cr::CreativeAppState appState;
  installDocument(appState, 413U);
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&edit, 1U}));
  const cr::CreativeTerrainSurfacePlan terrain =
      cr::buildCreativeTerrainSurfacePlan(
          appState.facade.document().terrainField());
  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  cr::CreativeTerrainRenderPlan render = cr::buildCreativeTerrainRenderPlan(
      terrain, grid.origin, grid.cellSizeMeters);
  if (!expect(terrain.accepted && render.accepted && !render.patches.empty(),
              "fallback test starts from a valid terrain plan")) {
    return false;
  }
  render.patches.front().corners[0] = render.patches.front().center;

  cr::CreativeRoomBakeRequest request;
  request.document = &appState.facade.document();
  request.validateReachability = false;
  request.usePrecomputedVoxelCuboids = true;
  request.precomputedVoxelCuboids = terrain.cuboids;
  request.usePrecomputedTerrainSurfacePatches = true;
  request.precomputedTerrainSurfacePatches = render.patches;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(request);
  const bool hasHeightPatch = std::any_of(
      baked.room.spatialSurfaces.begin(), baked.room.spatialSurfaces.end(),
      [](const iggy3d::RoomSpatialSurface& surface) {
        return surface.shape ==
               iggy3d::RoomSpatialSurfaceShape::HeightPatch;
      });
  const bool hasTerrainPlane = std::any_of(
      baked.room.spatialSurfaces.begin(), baked.room.spatialSurfaces.end(),
      [](const iggy3d::RoomSpatialSurface& surface) {
        return surface.shape == iggy3d::RoomSpatialSurfaceShape::Plane &&
               surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable;
      });
  return expect(baked.receipt.accepted &&
                    !baked.receipt.usedSmoothTerrainCollision &&
                    baked.receipt.bakedTerrainSurfacePatchCount == 0U,
                "invalid smooth plan is rejected as one unit") &&
         expect(!hasHeightPatch && hasTerrainPlane,
                "invalid smooth plan preserves stepped collision fallback");
}

bool denseRoomGeometryFallsBackBeforePatchVertexOverflow() {
  iggy3d::SceneRoomProjection room;
  constexpr std::size_t floorCount = 800U;
  room.meshes.reserve(floorCount + 1U);
  for (std::size_t index = 0U; index < floorCount; ++index) {
    iggy3d::SceneRoomMeshItem floor;
    floor.id = "terrain_budget_floor_" + std::to_string(index);
    floor.role = "floor";
    floor.position = {static_cast<float>(index * 2U), 0.0F, 0.0F};
    floor.size = {1.0F, 0.1F, 1.0F};
    room.meshes.push_back(std::move(floor));
  }
  iggy3d::SceneRoomMeshItem fallback;
  fallback.id = "terrain_budget_fallback";
  fallback.role = "terrain";
  fallback.position = {0.0F, 2.0F, 2.0F};
  fallback.size = {1.0F, 4.0F, 1.0F};
  room.meshes.push_back(std::move(fallback));

  const iggy3d::SceneRoomSurfacePatchItem patch{
      "terrain",
      {0.5F, 4.0F, 0.5F},
      {{{0.0F, 4.0F, 0.0F},
        {1.0F, 4.0F, 0.0F},
        {1.0F, 4.0F, 1.0F},
        {0.0F, 4.0F, 1.0F}}}};
  room.surfacePatches.assign(cr::kCreativeTerrainRenderPatchCapacity, patch);

  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(geometry.ready &&
                    geometry.vertices.size() < room.surfacePatches.size() * 5U,
                "dense room falls back instead of partially appending patches") &&
         expect(geometry.roomFloorDrawCount == floorCount + 1U,
                "fallback preserves the room floors and terrain plane");
}

bool bentSurfacePatchesParticipateInFrustumCulling() {
  iggy3d::SceneProjectionResult scene;
  scene.room.loaded = true;
  scene.room.surfacePatches = {
      {"terrain",
       {0.0F, 0.0F, 0.5F},
       {{{-0.25F, 0.0F, 0.25F},
         {0.25F, 0.0F, 0.25F},
         {0.25F, 0.0F, 0.75F},
         {-0.25F, 0.0F, 0.75F}}}},
      {"terrain",
       {10.5F, 0.0F, 0.5F},
       {{{10.0F, 0.0F, 0.25F},
         {11.0F, 0.0F, 0.25F},
         {11.0F, 0.0F, 0.75F},
         {10.0F, 0.0F, 0.75F}}}},
  };
  const StandaloneFrustumCullResult culled =
      cullStandaloneSceneRoomMeshesByFrustum(scene, iggy3d::identityMat4());
  return expect(culled.receipt.inputSurfacePatchCount == 2U &&
                    culled.receipt.keptSurfacePatchCount == 1U &&
                    culled.receipt.culledSurfacePatchCount == 1U,
                "frustum pass accounts for bent terrain patches") &&
         expect(culled.scene.room.surfacePatches.size() == 1U &&
                    culled.scene.room.loaded &&
                    culled.scene.room.floorVisible,
                "visible terrain patch keeps room loaded and floor-visible");
}

}  // namespace

int main() {
  return quickEditOwnsHeightAndRadiusWithoutNewBindings() &&
                 editSampleRemoveAndUndoUseDocumentTruth() &&
                 derivedSurfaceAndGuidesUseRevisionCaching() &&
                 semanticActionsRouteSelectionCommitCancelAndRemoval() &&
                 terrainPaintStrokeRepeatsDeduplicatesCachesAndGroupsUndo() &&
                 terrainSeedPreviewsStampsClearsAndGroupsHistory() &&
                 terrainCancelGestureCannotFallThroughIntoEraseStroke() &&
                 terrainStrokeCapacityStopsFurtherMutation() &&
                 terrainStrokeInterruptionFinalizesChangedAndEmptyGestures() &&
                 terrainGradeAnchorsPreviewsAppliesAndUndoesOneBatch() &&
                 terrainGradeCancelAndCapacityFailureDoNotCreateHistory() &&
                 terrainGradeRoutesSquareXAndCircleThroughWorldActions() &&
                 terrainSculptSamplesFlattensAndUndoesOneBatch() &&
                 terrainSculptHoldRepeatsAndCommitsOneUndo() &&
                 terrainSculptPreviewCachesAndUsesRuntimeSlopeBands() &&
                 terrainProfilePreviewApplyBaseLockAndUndoStayInParity() &&
                 terrainProfileWorldActionsArePressOnlyAndControllerNative() &&
                 terrainProfileRejectionIsVisibleAtomicAndModalSafe() &&
                 bentSurfacePatchesReachRendererAndRefreshWithHeight() &&
                 smoothTerrainCollisionMatchesRenderedTriangle() &&
                 gridChangeRebuildsWorldSpaceTerrainPatches() &&
                 oversizedBentSurfaceFallsBackToTerrainPlanes() &&
                 invalidSmoothPatchInputFallsBackAtomically() &&
                 denseRoomGeometryFallsBackBeforePatchVertexOverflow() &&
                 bentSurfacePatchesParticipateInFrustumCulling()
             ? 0
             : 1;
}
