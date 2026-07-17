#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorGizmo.hpp"
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
#include <filesystem>
#include <iostream>
#include <limits>
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

CreativeEditorState terrainPaintEditor() {
  CreativeEditorState editor;
  editor.frameIndex = 8U;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] =
      {cr::CreativeHeldItemKind::TerrainPaint,
       cr::CreativeObjectKind::Unknown};
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

CreativeEditorState terrainPathEditor(std::int32_t x, std::int32_t z) {
  CreativeEditorState editor = terrainEditor(x, z);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::TerrainPath;
  return editor;
}

CreativeEditorState terrainRegionEditor(std::int32_t x, std::int32_t z) {
  CreativeEditorState editor = terrainEditor(x, z);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::TerrainRegion;
  return editor;
}

void setTerrainRegionSelection(CreativeEditorState& editor,
                               cr::CreativeGridCoord3 first,
                               cr::CreativeGridCoord3 second) {
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First, first));
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::Second, second));
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
  CreativeEditorSceneCache cache;
  bool ok = expect(refreshCreativeEditorSceneCache(
                       cache, appState.facade.document()) &&
                       cache.terrainSurfaceBuildCount == 1U,
                   "initial access builds empty terrain plan");
  for (std::int32_t x = 0; x < 8; ++x) {
    editor.interaction.target.grid.targetCell.x = x;
    std::vector<iggy3d::RenderCreativeWireframeDebugLine> guides;
    appendCreativeEditorTerrainOverlay(appState.facade.document(), editor,
                                       0.04F, guides);
    ok = expect(!guides.empty() &&
                    !refreshCreativeEditorSceneCache(
                        cache, appState.facade.document()) &&
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
      cache, appState.facade.document());
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
                    cache, appState.facade.document()) &&
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
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
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

bool terrainSurfacePaintRoutesGesturesHistorySamplingAndRendering() {
  cr::CreativeAppState appState;
  installDocument(appState, 430U);
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&control, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainPaintEditor();
  editor.toolSettings.terrainPaintMaterial =
      cr::CreativeTerrainMaterial::Stone;
  editor.toolSettings.terrainPaintRadius =
      cr::CreativeTerrainPaintRadius::OneCell;
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
  };
  CreativeEditorSceneCache cache;
  static_cast<void>(refreshCreativeEditorSceneCache(
      cache, appState.facade.document()));

  process(strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  process(strokeAction(cr::CreativeWorldActionId::Accept, true),
          cr::kCreativeMaterialStrokeRepeatNanoseconds - 1U);
  process(strokeAction(cr::CreativeWorldActionId::Accept, false, false, true),
          cr::kCreativeMaterialStrokeRepeatNanoseconds);
  bool ok = expect(
      appState.facade.document().terrainMaterialField().overrideCount() == 5U &&
          appState.facade.document().terrainMaterialField().materialAt({0, 0}) ==
              cr::CreativeTerrainMaterial::Stone &&
          cr::creativeUndoDepth(appState.history) == 1U,
      "X paints one radius-one surface gesture and records one undo");

  editor.toolSettings.terrainPaintMaterial = cr::CreativeTerrainMaterial::Sand;
  process(strokeAction(cr::CreativeWorldActionId::Pick, true, true),
          300'000'000ULL);
  ok = expect(editor.toolSettings.terrainPaintMaterial ==
                  cr::CreativeTerrainMaterial::Stone,
              "Square samples the aimed terrain surface material") &&
       ok;

  const bool materialRefreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document());
  const bool hasStone = std::any_of(
      cache.preview.scene.room.surfacePatches.begin(),
      cache.preview.scene.room.surfacePatches.end(),
      [](const iggy3d::SceneRoomSurfacePatchItem& patch) {
        return patch.role == "terrain_stone";
      });
  ok = expect(materialRefreshed && hasStone &&
                  cache.terrainSurfaceBuildCount == 1U &&
                  cache.terrainMaterialBuildCount == 2U &&
                  cache.terrainMaterialRevision ==
                      appState.facade.document()
                          .terrainMaterialField()
                          .revision(),
              "material refresh reuses geometry and joins the painted role") &&
       ok;

  process(strokeAction(cr::CreativeWorldActionId::Reject, true, true),
          400'000'000ULL);
  process(strokeAction(cr::CreativeWorldActionId::Reject, false, false, true),
          400'000'001ULL);
  return expect(
             appState.facade.document().terrainMaterialField().overrideCount() ==
                     0U &&
                 cr::creativeUndoDepth(appState.history) == 2U,
             "Circle restores grass and commits a second grouped gesture") &&
         ok;
}

bool terrainConnectedAndRegionModesUsePressBasedAtomicGestures() {
  cr::CreativeAppState appState;
  installDocument(appState, 431U);
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&control, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainPaintEditor();
  editor.toolSettings.terrainPaintMode =
      cr::CreativeTerrainPaintMode::Connected;
  editor.toolSettings.terrainPaintMaterial = cr::CreativeTerrainMaterial::Dirt;
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
  };
  const auto press = [&](cr::CreativeWorldActionId action,
                         std::uint64_t now) {
    process(strokeAction(action, true, true), now);
    process(strokeAction(action, false, false, true), now + 1U);
  };

  press(cr::CreativeWorldActionId::Accept, 0U);
  const std::uint64_t connectedPreviewBuilds =
      editor.terrainPaint.preview.buildCount;
  const std::size_t connectedPreviewEdges =
      editor.terrainPaint.preview.edges.size();
  process({}, 1U);
  bool ok = expect(
      appState.facade.document().terrainMaterialField().overrideCount() == 13U &&
          cr::creativeUndoDepth(appState.history) == 1U &&
          connectedPreviewBuilds == 1U &&
          connectedPreviewEdges > 0U &&
          editor.terrainPaint.preview.buildCount == connectedPreviewBuilds &&
          editor.terrainPaint.preview.edges.size() == connectedPreviewEdges,
      "connected X fills once and idle frames reuse plan and boundary geometry");
  press(cr::CreativeWorldActionId::Reject, 2U);
  ok = expect(
           appState.facade.document().terrainMaterialField().overrideCount() ==
                   0U &&
               cr::creativeUndoDepth(appState.history) == 2U,
           "connected Circle restores the component in one undoable batch") &&
       ok;

  const std::array stoneEdits{
      cr::CreativeTerrainMaterialEdit{cr::CreativeTerrainMaterialEditKind::Set,
                                      {0, 0},
                                      cr::CreativeTerrainMaterial::Stone},
      cr::CreativeTerrainMaterialEdit{cr::CreativeTerrainMaterialEditKind::Set,
                                      {1, 0},
                                      cr::CreativeTerrainMaterial::Stone},
  };
  static_cast<void>(appState.facade.applyTerrainMaterialEdits(stoneEdits));
  appState.history = {};
  editor.toolSettings.terrainPaintMode = cr::CreativeTerrainPaintMode::Region;
  editor.toolSettings.terrainPaintMaterial = cr::CreativeTerrainMaterial::Sand;
  editor.toolSettings.terrainPaintSource =
      cr::CreativeTerrainPaintSource::Stone;
  camera.worldEye.x = -0.5F;
  press(cr::CreativeWorldActionId::Accept, 10U);
  camera.worldEye.x = 1.5F;
  press(cr::CreativeWorldActionId::Accept, 12U);
  ok = expect(editor.terrainPaint.regionPhase ==
                  CreativeEditorTerrainPaintRegionPhase::Complete &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "region stores two corners without mutating history") &&
       ok;
  camera.worldForward = {1.0F, 0.0F, 0.0F};
  press(cr::CreativeWorldActionId::Accept, 14U);
  ok = expect(
           appState.facade.document().terrainMaterialField().materialAt({0, 0}) ==
                   cr::CreativeTerrainMaterial::Sand &&
               appState.facade.document().terrainMaterialField().materialAt(
                   {1, 0}) == cr::CreativeTerrainMaterial::Sand &&
               appState.facade.document().terrainMaterialField().materialAt(
                   {-1, 0}) == cr::CreativeTerrainMaterial::Grass &&
               cr::creativeUndoDepth(appState.history) == 1U &&
               editor.terrainPaint.regionPhase ==
                   CreativeEditorTerrainPaintRegionPhase::Empty,
           "third X replaces only the selected source material atomically") &&
       ok;

  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldEye.x = -0.5F;
  press(cr::CreativeWorldActionId::Accept, 20U);
  camera.worldEye.x = 1.5F;
  press(cr::CreativeWorldActionId::Accept, 22U);
  press(cr::CreativeWorldActionId::Reject, 24U);
  const bool backedToSecond =
      editor.terrainPaint.regionPhase ==
      CreativeEditorTerrainPaintRegionPhase::FirstCorner;
  press(cr::CreativeWorldActionId::Reject, 26U);
  return expect(backedToSecond &&
                    editor.terrainPaint.regionPhase ==
                        CreativeEditorTerrainPaintRegionPhase::Empty &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "Circle backs through region corners without mutation") &&
         ok;
}

bool terrainPaintStrokeRepeatsDeduplicatesCachesAndGroupsUndo() {
  cr::CreativeAppState appState;
  installDocument(appState, 407U);
  CreativeEditorState editor = terrainEditor(0, 0);
  editor.terrain.heightCells = 6U;
  editor.terrain.radiusCells = 3U;
  CreativeEditorSceneCache cache;
  bool ok = expect(refreshCreativeEditorSceneCache(
                       cache, appState.facade.document()) &&
                       cache.terrainSurfaceBuildCount == 1U,
                   "stroke test begins from one empty terrain bake");

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true, true), 0U);
  const bool firstRefresh = refreshCreativeEditorSceneCache(
      cache, appState.facade.document());
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
                      cache, appState.facade.document()),
              "held X does not repeat before 200 ms") &&
       ok;

  processCreativeTerrainStrokeFrame(
      appState, editor,
      strokeAction(cr::CreativeWorldActionId::Accept, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  const bool secondRefresh = refreshCreativeEditorSceneCache(
      cache, appState.facade.document());
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
                      cache, appState.facade.document()),
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
  CreativeEditorSceneCache cache;
  static_cast<void>(refreshCreativeEditorSceneCache(
      cache, appState.facade.document()));
  const std::uint64_t buildsBeforePreview = cache.terrainSurfaceBuildCount;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> preview;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.04F,
                                     preview);
  ok = expect(!preview.empty() &&
                  !refreshCreativeEditorSceneCache(
                      cache, appState.facade.document()) &&
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
      cache, appState.facade.document());
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
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
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
  CreativeEditorSceneCache sceneCache;
  const bool sceneBuilt = refreshCreativeEditorSceneCache(
      sceneCache, appState.facade.document());
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
      sceneCache, appState.facade.document());
  const bool sceneReused = refreshCreativeEditorSceneCache(
      sceneCache, appState.facade.document());
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
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
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

bool terrainPathLocksBendsPreviewsCommitsAndUndoesOneBatch() {
  cr::CreativeAppState appState;
  installDocument(appState, 421U);
  CreativeEditorState editor = terrainPathEditor(0, 0);
  const CreativeEditorTerrainPathReceipt start =
      addCreativeEditorTerrainPathPoint(appState.facade.document(), editor);
  setTerrainStrokeTarget(editor, 2, 2);
  const CreativeEditorTerrainPathReceipt bend =
      addCreativeEditorTerrainPathPoint(appState.facade.document(), editor);
  const CreativeEditorTerrainPathReceipt backed =
      removeCreativeEditorTerrainPathPoint(editor);
  setTerrainStrokeTarget(editor, 4, 0);

  const std::uint64_t revisionBeforePreview =
      appState.facade.document().revision();
  const bool previewBuilt = refreshCreativeEditorTerrainPathPreview(
      editor.terrain, appState.facade.document(), editor);
  const CreativeTerrainPathPreviewCache preview = editor.terrain.path.preview;
  const bool previewReused = !refreshCreativeEditorTerrainPathPreview(
      editor.terrain, appState.facade.document(), editor);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     lines);
  editor.toolOptions.open = true;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> hiddenLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     hiddenLines);
  editor.toolOptions.open = false;
  bool ok = expect(start.accepted && start.changed && bend.accepted &&
                       bend.changed && backed.accepted && backed.changed &&
                       editor.terrain.path.pointCount == 1U,
                   "Square locks points and Circle removes only the latest bend") &&
            expect(previewBuilt && previewReused && preview.valid &&
                       preview.pointCount == 2U &&
                       preview.lockedPointCount == 1U &&
                       preview.plan.accepted && preview.renderAccepted &&
                       preview.buildCount == 1U && !lines.empty() &&
                       hiddenLines.empty() &&
                       appState.facade.document().revision() ==
                           revisionBeforePreview,
                   "locked start plus live aim previews exact path without mutation");

  const CreativeEditorTerrainPathReceipt applied =
      applyCreativeEditorTerrainPathWithHistory(
          appState, editor, "test_terrain_path_apply");
  bool parity = applied.plan.finalControls().size() ==
                preview.plan.finalControls().size();
  for (const cr::CreativeTerrainControlPoint& control :
       preview.plan.finalControls()) {
    const cr::CreativeTerrainControlPoint* stored =
        appState.facade.document().terrainField().controlAt(control.coord);
    parity = parity && stored != nullptr && *stored == control;
  }
  ok = expect(applied.accepted && applied.changed && parity &&
                  editor.terrain.path.pointCount == 0U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "X commits the visible plan as one terrain batch and one undo") &&
       ok;

  const bool undone = undoLastEdit(appState, "test_terrain_path_undo");
  CreativeEditorState tuning = terrainPathEditor(0, 0);
  const bool widened = processCreativeEditorQuickEditAction(
      tuning, cr::CreativeInputActionId::QuickEditIncrease);
  const bool raised = processCreativeEditorQuickEditAction(
      tuning, cr::CreativeInputActionId::QuickEditPrevious);
  return expect(undone &&
                    appState.facade.document().terrainField().controlCount() ==
                        0U,
                "one undo removes the complete path") &&
         expect(widened && raised &&
                    tuning.toolSettings.terrainPathWidth ==
                        cr::CreativeTerrainPathWidth::FiveCells &&
                    tuning.toolSettings.terrainPathAmplitude ==
                        cr::CreativeTerrainPathAmplitude::TwoCells &&
                    creativeEditorTerrainPathQuickEditLabel(tuning) ==
                        "WIDTH 5 | RISE 2",
                "D-pad adjusts path width and rise through semantic quick edits") &&
         ok;
}

bool terrainPathRoutesSquareXAndCircleThroughWorldActions() {
  cr::CreativeAppState appState;
  installDocument(appState, 422U);
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&initial, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainPathEditor(0, 0);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
  };
  const auto press = [&](cr::CreativeWorldActionId action,
                         std::uint64_t now) {
    process(strokeAction(action, true, true), now);
    process(strokeAction(action, false, false, true), now + 1U);
  };

  press(cr::CreativeWorldActionId::Pick, 0U);
  camera.worldEye.x = 2.5F;
  press(cr::CreativeWorldActionId::Pick, 2U);
  press(cr::CreativeWorldActionId::Reject, 4U);
  bool ok = expect(editor.terrain.path.pointCount == 1U &&
                       editor.terrain.path.points[0].coord ==
                           cr::CreativeTerrainCoord2{0, 0},
                   "PS5 Square locks route points and Circle backs up one point");

  camera.worldEye.x = 4.5F;
  press(cr::CreativeWorldActionId::Accept, 6U);
  ok = expect(editor.terrain.path.pointCount == 0U &&
                  appState.facade.document().terrainField().controlAt({4, 0}) !=
                      nullptr &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "PS5 X commits the locked start plus live endpoint once") &&
       ok;
  return expect(undoLastEdit(appState, "test_terrain_path_world_undo") &&
                    appState.facade.document().terrainField().controlCount() ==
                        1U,
                "controller path commit remains one undo record") &&
         ok;
}

bool terrainRegionPreviewApplyAndUndoStayAtomic() {
  cr::CreativeAppState appState;
  installDocument(appState, 423U);
  const std::array initial{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{1, 0}, 6U, 3U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{3, 0}, 10U, 1U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(initial));
  appState.history = {};
  CreativeEditorState editor = terrainRegionEditor(0, 0);
  setTerrainRegionSelection(editor, {0, 0, 0}, {1, 0, 0});
  editor.toolSettings.terrainRegionAmount =
      cr::CreativeTerrainRegionAmount::TwoCells;

  const std::uint64_t revisionBeforePreview =
      appState.facade.document().revision();
  const bool previewBuilt = refreshCreativeEditorTerrainRegionPreview(
      editor.terrain, appState.facade.document(), editor);
  const CreativeTerrainRegionPreviewCache preview = editor.terrain.region.preview;
  const bool previewReused = !refreshCreativeEditorTerrainRegionPreview(
      editor.terrain, appState.facade.document(), editor);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> previewLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     previewLines);
  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  const float expectedRaisedTop = static_cast<float>(
      grid.origin.y + 8.0 * grid.cellSizeMeters);
  const bool plannedHeightVisible = std::any_of(
      previewLines.begin(), previewLines.end(),
      [expectedRaisedTop](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return approx(line.color.r, 0.20F) && approx(line.color.g, 1.0F) &&
               approx(line.color.b, 0.35F) &&
               (approx(line.start.y, expectedRaisedTop) ||
                approx(line.end.y, expectedRaisedTop));
      });
  editor.volume.active = true;
  const CreativeEditorSelectionFrame selection;
  const CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame visibleOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      visibleOverlay);
  editor.toolOptions.open = true;
  CreativeEditorOverlayFrame modalOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      modalOverlay);
  editor.toolOptions.open = false;
  bool ok = expect(previewBuilt && previewReused && preview.valid &&
                       preview.renderAccepted && preview.plan.accepted &&
                       preview.plan.affectedControlCount == 2U &&
                       preview.plan.editCount == 2U &&
                       preview.buildCount == 1U && plannedHeightVisible &&
                       visibleOverlay.volumeEdgeCount > 0U &&
                       visibleOverlay.terrainEdgeCount > 0U &&
                       modalOverlay.volumeEdgeCount == 0U &&
                       modalOverlay.terrainEdgeCount == 0U &&
                       appState.facade.document().revision() ==
                           revisionBeforePreview,
                   "complete region draws and caches its exact non-mutating preview");

  const CreativeEditorTerrainRegionReceipt applied =
      applyCreativeEditorTerrainRegionWithHistory(
          appState, editor, "test_terrain_region_apply");
  const cr::CreativeTerrainControlPoint* first =
      appState.facade.document().terrainField().controlAt({0, 0});
  const cr::CreativeTerrainControlPoint* second =
      appState.facade.document().terrainField().controlAt({1, 0});
  const cr::CreativeTerrainControlPoint* outside =
      appState.facade.document().terrainField().controlAt({3, 0});
  ok = expect(applied.accepted && applied.changed &&
                  applied.plan.editCount == preview.plan.editCount &&
                  first != nullptr && first->heightCells == 4U &&
                  first->radiusCells == 2U && second != nullptr &&
                  second->heightCells == 8U && second->radiusCells == 3U &&
                  outside != nullptr && outside->heightCells == 10U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "region commits previewed rods once and preserves outside truth") &&
       ok;

  const bool undone = undoLastEdit(appState, "test_terrain_region_undo");
  first = appState.facade.document().terrainField().controlAt({0, 0});
  second = appState.facade.document().terrainField().controlAt({1, 0});
  ok = expect(undone && first != nullptr && first->heightCells == 2U &&
                  second != nullptr && second->heightCells == 6U &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "one undo restores the complete region batch") &&
       ok;

  editor.toolSettings.terrainRegionOperation =
      cr::CreativeTerrainRegionOperation::Erase;
  const bool erasePreviewBuilt = refreshCreativeEditorTerrainRegionPreview(
      editor.terrain, appState.facade.document(), editor);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> eraseLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     eraseLines);
  const bool eraseColorVisible = std::any_of(
      eraseLines.begin(), eraseLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return approx(line.color.r, 1.0F) && approx(line.color.g, 0.46F) &&
               approx(line.color.b, 0.12F);
      });
  const CreativeEditorTerrainRegionReceipt erased =
      applyCreativeEditorTerrainRegionWithHistory(
          appState, editor, "test_terrain_region_erase");
  ok = expect(erasePreviewBuilt && eraseColorVisible && erased.accepted &&
                  erased.changed &&
                  appState.facade.document().terrainField().controlAt({0, 0}) ==
                      nullptr &&
                  appState.facade.document().terrainField().controlAt({1, 0}) ==
                      nullptr &&
                  appState.facade.document().terrainField().controlAt({3, 0}) !=
                      nullptr &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "orange Erase preview removes only selected rods in one batch") &&
       ok;
  return expect(undoLastEdit(appState, "test_terrain_region_erase_undo") &&
                    appState.facade.document().terrainField().controlCount() ==
                        3U,
                "one undo restores an erased terrain region") &&
         ok;
}

bool terrainRegionRoutesCornersSampleCancelAndQuickEdit() {
  cr::CreativeAppState appState;
  installDocument(appState, 424U);
  const std::array initial{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 3U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 0}, 7U, 2U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(initial));
  appState.history = {};
  CreativeEditorState editor = terrainRegionEditor(0, 0);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const auto process = [&](const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t now) {
    processCreativeEditorWorldInteractionFrame(
        {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
         pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, now,
         false});
  };
  const auto press = [&](cr::CreativeWorldActionId action,
                         std::uint64_t now) {
    process(strokeAction(action, true, true), now);
    process(strokeAction(action, false, false, true), now + 1U);
  };

  editor.toolSettings.terrainRegionOperation =
      cr::CreativeTerrainRegionOperation::Flatten;
  camera.worldEye.x = 2.5F;
  press(cr::CreativeWorldActionId::Pick, 0U);
  bool ok = expect(editor.terrain.region.targetHeightCells == 7U &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "PS5 Square samples flatten height without document history");

  editor.toolSettings.terrainRegionOperation =
      cr::CreativeTerrainRegionOperation::Raise;
  camera.worldEye.x = 0.5F;
  press(cr::CreativeWorldActionId::Accept, 2U);
  camera.worldEye.x = 2.5F;
  press(cr::CreativeWorldActionId::Accept, 4U);
  const bool tunedAmount = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditPrevious);
  press(cr::CreativeWorldActionId::Accept, 6U);
  const cr::CreativeTerrainControlPoint* first =
      appState.facade.document().terrainField().controlAt({0, 0});
  const cr::CreativeTerrainControlPoint* second =
      appState.facade.document().terrainField().controlAt({2, 0});
  ok = expect(tunedAmount &&
                  editor.toolSettings.terrainRegionAmount ==
                      cr::CreativeTerrainRegionAmount::TwoCells &&
                  first != nullptr && first->heightCells == 5U &&
                  second != nullptr && second->heightCells == 9U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "PS5 X selects two corners and applies the tuned region once") &&
       ok;

  press(cr::CreativeWorldActionId::Primary, 8U);
  const bool mouseCancelled = editor.volume.selection.phase ==
                              cr::CreativeVolumeSelectionPhase::Empty;
  press(cr::CreativeWorldActionId::Accept, 10U);
  press(cr::CreativeWorldActionId::Reject, 12U);
  const bool cycledOperation = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  return expect(mouseCancelled && editor.volume.selection.phase ==
                        cr::CreativeVolumeSelectionPhase::Empty &&
                    cycledOperation &&
                    editor.toolSettings.terrainRegionOperation ==
                        cr::CreativeTerrainRegionOperation::Lower &&
                    creativeEditorTerrainRegionQuickEditLabel(editor) ==
                        "LOWER | AMOUNT 2",
                "left mouse and Circle clear while D-pad cycles persistent operation") &&
         ok;
}

bool terrainStampCopiesPreviewsTransformsCommitsAndRepeats() {
  cr::CreativeAppState appState;
  installDocument(appState, 425U);
  const std::array initial{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 0}, 6U, 3U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{10, 0}, 9U, 1U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{11, 0}, 8U, 1U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{20, 20}, 7U, 2U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(initial));
  appState.history = {};
  CreativeEditorState editor = terrainRegionEditor(0, 0);
  editor.toolSettings.terrainStampElevationMode =
      cr::CreativeTerrainStampElevationMode::Absolute;
  setTerrainRegionSelection(editor, {0, 0, 0}, {2, 0, 0});

  const std::uint64_t revisionBeforeCopy =
      appState.facade.document().revision();
  const CreativeEditorTerrainStampReceipt copied =
      copyCreativeEditorTerrainRegionToStamp(appState, editor);
  const CreativeEditorTerrainStampReceipt began =
      beginCreativeEditorTerrainStampPreview(appState, editor);
  setTerrainStrokeTarget(editor, 10, 0);
  const bool previewBuilt = refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp);
  const CreativeTerrainStampPreviewCache mergePreview =
      editor.terrain.region.stamp.preview;
  const bool previewReused = !refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> mergeLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     mergeLines);
  const bool mergeGreen = std::any_of(
      mergeLines.begin(), mergeLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return approx(line.color.r, 0.20F) && approx(line.color.g, 1.0F) &&
               approx(line.color.b, 0.35F);
      });
  bool ok = expect(copied.accepted && copied.copy.copiedControlCount == 2U &&
                       cr::isValidCreativeTerrainStamp(appState.terrainStamp) &&
                       began.accepted && editor.terrain.region.stamp.active &&
                       appState.facade.document().revision() ==
                           revisionBeforeCopy &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "terrain region copy starts a transient non-mutating stamp") &&
            expect(previewBuilt && previewReused && mergePreview.valid &&
                       mergePreview.renderAccepted &&
                       mergePreview.plan.accepted &&
                       mergePreview.plan.transformedWidthCells == 3U &&
                       mergePreview.plan.transformedDepthCells == 1U &&
                       mergePreview.plan.insertedControlCount == 1U &&
                       mergePreview.plan.updatedControlCount == 1U &&
                       mergePreview.plan.removedControlCount == 0U &&
                       mergePreview.buildCount == 1U && mergeGreen,
                   "merge preview is cached green and preserves footprint extras");

  const bool rotated = processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  const bool selectedMirror = processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditNext);
  const bool mirrored = processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  ok = expect(rotated && selectedMirror && mirrored &&
                  editor.terrain.region.stamp.quarterTurns == 1U &&
                  editor.terrain.region.stamp.mirrorX &&
                  creativeEditorTerrainStampQuickEditLabel(editor) ==
                      "STAMP MERGE | ABSOLUTE | MIRROR X | ROT 90 | MX ON | MZ OFF | Y +0",
              "D-pad controls select and apply exact stamp transforms") &&
       ok;

  editor.terrain.region.stamp.quarterTurns = 0U;
  editor.terrain.region.stamp.mirrorX = false;
  editor.terrain.region.stamp.selectedControl =
      CreativeTerrainStampTransformControl::Rotation;
  editor.toolSettings.terrainStampMode =
      cr::CreativeTerrainStampMode::Replace;
  const bool replacePreviewBuilt = refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp);
  const CreativeTerrainStampPreviewCache replacePreview =
      editor.terrain.region.stamp.preview;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> replaceLines;
  appendCreativeEditorTerrainOverlay(appState.facade.document(), editor, 0.1F,
                                     replaceLines);
  const bool removalOrange = std::any_of(
      replaceLines.begin(), replaceLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return approx(line.color.r, 1.0F) && approx(line.color.g, 0.46F) &&
               approx(line.color.b, 0.12F);
      });
  editor.volume.active = true;
  const CreativeEditorSelectionFrame selection;
  const CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame visibleOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      visibleOverlay);
  editor.toolOptions.open = true;
  CreativeEditorOverlayFrame modalOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      modalOverlay);
  editor.toolOptions.open = false;
  ok = expect(replacePreviewBuilt && replacePreview.plan.accepted &&
                  replacePreview.plan.removedControlCount == 1U &&
                  removalOrange && visibleOverlay.volumeEdgeCount > 0U &&
                  visibleOverlay.terrainEdgeCount > 0U &&
                  modalOverlay.volumeEdgeCount == 0U &&
                  modalOverlay.terrainEdgeCount == 0U,
              "replace preview marks removals orange and hides under modals") &&
       ok;

  const CreativeEditorTerrainStampReceipt firstStamp =
      applyCreativeEditorTerrainStampWithHistory(
          appState, editor, "test_terrain_stamp_first");
  const cr::CreativeTerrainControlPoint* first =
      appState.facade.document().terrainField().controlAt({10, 0});
  const cr::CreativeTerrainControlPoint* second =
      appState.facade.document().terrainField().controlAt({12, 0});
  const cr::CreativeTerrainControlPoint* outside =
      appState.facade.document().terrainField().controlAt({20, 20});
  const bool rebuiltAfterMutation = refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp);
  ok = expect(firstStamp.accepted && firstStamp.changed && first != nullptr &&
                  first->heightCells == 2U && second != nullptr &&
                  second->heightCells == 6U &&
                  appState.facade.document().terrainField().controlAt({11, 0}) ==
                      nullptr &&
                  outside != nullptr && outside->heightCells == 7U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  editor.terrain.region.stamp.active,
              "one stamp commits one Facade batch and stays active for repeats") &&
       expect(rebuiltAfterMutation &&
                  editor.terrain.region.stamp.preview.buildCount ==
                      replacePreview.buildCount + 1U &&
                  editor.terrain.region.stamp.preview.plan.status ==
                      cr::CreativeTerrainStampPlanStatus::NoChange,
              "accepted mutation invalidates and rebuilds the stamp preview once") &&
       ok;

  setTerrainStrokeTarget(editor, 30, 0);
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  const CreativeEditorTerrainStampReceipt secondStamp =
      applyCreativeEditorTerrainStampWithHistory(
          appState, editor, "test_terrain_stamp_second");
  const CreativeEditorTerrainStampReceipt cancelled =
      cancelCreativeEditorTerrainStamp(editor);
  ok = expect(secondStamp.accepted && secondStamp.changed &&
                  appState.facade.document().terrainField().controlAt({30, 0}) !=
                      nullptr &&
                  appState.facade.document().terrainField().controlAt({32, 0}) !=
                      nullptr &&
                  cr::creativeUndoDepth(appState.history) == 2U &&
                  cancelled.accepted && cancelled.changed &&
                  !editor.terrain.region.stamp.active &&
                  cr::creativeVolumeSelectionComplete(editor.volume.selection),
              "repositioned stamp repeats as a second undoable batch and cancels cleanly") &&
       ok;

  static_cast<void>(beginCreativeEditorTerrainStampPreview(appState, editor));
  setTerrainStrokeTarget(editor, std::numeric_limits<std::int32_t>::max(), 0);
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  CreativeEditorOverlayFrame overflowOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      overflowOverlay);
  ok = expect(editor.terrain.region.stamp.preview.plan.status ==
                  cr::CreativeTerrainStampPlanStatus::CoordinateOverflow &&
                  overflowOverlay.volumeEdgeCount == 0U,
              "non-positionable overflow rejects without constructing a bogus footprint") &&
       ok;
  static_cast<void>(cancelCreativeEditorTerrainStamp(editor));

  const bool secondUndone =
      undoLastEdit(appState, "test_terrain_stamp_second_undo");
  const bool firstUndone =
      undoLastEdit(appState, "test_terrain_stamp_first_undo");
  return expect(secondUndone && firstUndone &&
                    appState.facade.document().terrainField().controlAt({30, 0}) ==
                        nullptr &&
                    appState.facade.document().terrainField().controlAt({12, 0}) ==
                        nullptr &&
                    appState.facade.document().terrainField().controlAt({11, 0}) !=
                        nullptr &&
                    appState.facade.document().terrainField().controlCount() ==
                        initial.size() &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "two undos restore each repeated stamp in reverse order") &&
         ok;
}

bool terrainStampSurfaceAlignmentAndHeightOffsetStayPreviewExact() {
  cr::CreativeAppState appState;
  installDocument(appState, 427U);
  const std::array initial{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 2U, 2U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 0}, 6U, 3U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{10, 0}, 9U, 1U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{20, 0}, 64U, 1U}},
  };
  static_cast<void>(appState.facade.applyTerrainControlEdits(initial));
  appState.history = {};
  CreativeEditorState editor = terrainRegionEditor(0, 0);
  editor.volume.active = true;
  setTerrainRegionSelection(editor, {0, 0, 0}, {2, 0, 0});
  static_cast<void>(copyCreativeEditorTerrainRegionToStamp(appState, editor));
  static_cast<void>(beginCreativeEditorTerrainStampPreview(appState, editor));
  setTerrainStrokeTarget(editor, 10, 0);
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  const cr::CreativeTerrainStampPlan aligned =
      editor.terrain.region.stamp.preview.plan;
  bool ok = expect(editor.toolSettings.terrainStampElevationMode ==
                       cr::CreativeTerrainStampElevationMode::Surface &&
                       aligned.accepted && aligned.targetSurfacePresent &&
                       aligned.targetSurfaceHeightCells == 9U &&
                       aligned.appliedHeightOffsetCells == 7 &&
                       aligned.controls()[0].heightCells == 9U &&
                       aligned.controls()[1].heightCells == 13U,
                   "Surface preview aligns the copied minimum to aimed terrain");

  static_cast<void>(processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditNext));
  static_cast<void>(processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditNext));
  const bool selectedHeight = processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditNext);
  const bool raised = processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  const cr::CreativeTerrainStampPlan raisedPlan =
      editor.terrain.region.stamp.preview.plan;
  ok = expect(selectedHeight && raised &&
                  editor.terrain.region.stamp.selectedControl ==
                      CreativeTerrainStampTransformControl::HeightOffset &&
                  raisedPlan.accepted &&
                  raisedPlan.manualHeightOffsetCells == 1 &&
                  raisedPlan.appliedHeightOffsetCells == 8 &&
                  raisedPlan.controls()[0].heightCells == 10U &&
                  raisedPlan.controls()[1].heightCells == 14U &&
                  creativeEditorTerrainStampQuickEditLabel(editor) ==
                      "STAMP MERGE | SURFACE | HEIGHT | ROT 0 | MX OFF | MZ OFF | Y +1",
              "D-pad height offset applies after automatic surface alignment") &&
       ok;
  const CreativeEditorTerrainStampReceipt applied =
      applyCreativeEditorTerrainStampWithHistory(
          appState, editor, "test_terrain_stamp_surface_apply");
  const cr::CreativeTerrainControlPoint* first =
      appState.facade.document().terrainField().controlAt({10, 0});
  const cr::CreativeTerrainControlPoint* second =
      appState.facade.document().terrainField().controlAt({12, 0});
  ok = expect(applied.accepted && applied.changed && first != nullptr &&
                  first->heightCells == 10U && second != nullptr &&
                  second->heightCells == 14U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "Surface stamp commits the exact elevated preview in one undo") &&
       ok;
  static_cast<void>(cancelCreativeEditorTerrainStamp(editor));
  ok = expect(undoLastEdit(appState, "test_terrain_stamp_surface_undo") &&
                  appState.facade.document().terrainField().controlAt({10, 0}) !=
                      nullptr &&
                  appState.facade.document()
                          .terrainField()
                          .controlAt({10, 0})
                          ->heightCells == 9U &&
                  appState.facade.document().terrainField().controlAt({12, 0}) ==
                      nullptr,
              "one undo restores terrain beneath an aligned stamp") &&
       ok;

  static_cast<void>(beginCreativeEditorTerrainStampPreview(appState, editor));
  setTerrainStrokeTarget(editor, 20, 0);
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  const std::uint64_t revisionBeforeRejectedApply =
      appState.facade.document().revision();
  const cr::CreativeTerrainStampPlan overflow =
      editor.terrain.region.stamp.preview.plan;
  const CreativeEditorSelectionFrame selection;
  const CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame rejectedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      rejectedOverlay);
  const CreativeEditorTerrainStampReceipt rejected =
      applyCreativeEditorTerrainStampWithHistory(
          appState, editor, "test_terrain_stamp_height_rejected");
  ok = expect(!overflow.accepted &&
                  overflow.status ==
                      cr::CreativeTerrainStampPlanStatus::HeightOutOfRange &&
                  overflow.appliedHeightOffsetCells == 62 &&
                  rejectedOverlay.volumeEdgeCount > 0U &&
                  !rejected.accepted && !rejected.changed &&
                  appState.facade.document().revision() ==
                      revisionBeforeRejectedApply &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "out-of-range Surface placement stays visible but cannot mutate") &&
       ok;

  editor.toolSettings.terrainStampElevationMode =
      cr::CreativeTerrainStampElevationMode::Absolute;
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  const cr::CreativeTerrainStampPlan absolute =
      editor.terrain.region.stamp.preview.plan;
  editor.terrain.region.stamp.selectedControl =
      CreativeTerrainStampTransformControl::HeightOffset;
  editor.terrain.region.stamp.heightOffsetCells =
      cr::kCreativeTerrainStampMaximumHeightOffsetCells;
  const bool bounded = !processCreativeEditorTerrainStampQuickEdit(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  return expect(absolute.accepted && absolute.targetSurfacePresent &&
                    absolute.targetSurfaceHeightCells == 64U &&
                    absolute.appliedHeightOffsetCells == 0 &&
                    absolute.controls()[0].heightCells == 2U &&
                    absolute.controls()[1].heightCells == 6U,
                "Absolute mode ignores destination elevation") &&
         expect(bounded &&
                    editor.terrain.region.stamp.heightOffsetCells ==
                        cr::kCreativeTerrainStampMaximumHeightOffsetCells,
                "manual height quick edit stops at its explicit bound") &&
         ok;
}

bool terrainStampCommandsUseTheTerrainClipboardLane() {
  cr::CreativeAppState appState;
  installDocument(appState, 426U);
  const cr::CreativeTerrainControlEdit source{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 5U, 2U}};
  static_cast<void>(appState.facade.applyTerrainControlEdits(
      std::span{&source, 1U}));
  appState.history = {};
  CreativeEditorState editor = terrainRegionEditor(0, 0);
  setTerrainRegionSelection(editor, {0, 0, 0}, {0, 0, 0});
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const auto command = [&](cr::CreativeInputActionId action,
                           cr::CreativeInputKey trigger) {
    cr::CreativeInputRouteResult routed;
    routed.context = cr::CreativeInputContext::EditorViewport;
    routed.actions[0] = {action, trigger};
    routed.actionCount = 1U;
    applyCreativeEditorCommandInput(routed, appState, editor,
                                    std::filesystem::path{}, "terrain_test");
  };

  command(cr::CreativeInputActionId::CopySelection,
          cr::CreativeInputKey::C);
  bool ok = expect(cr::isValidCreativeTerrainStamp(appState.terrainStamp) &&
                       cr::creativeClipboardEmpty(appState.clipboard) &&
                       appState.facade.document().revision() == revisionBefore &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "Copy routes Terrain Region into its separate transient clipboard");
  command(cr::CreativeInputActionId::CutSelection,
          cr::CreativeInputKey::X);
  command(cr::CreativeInputActionId::DeleteSelection,
          cr::CreativeInputKey::Delete);
  ok = expect(appState.facade.document().terrainField().controlAt({0, 0}) !=
                  nullptr &&
                  appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "terrain Cut and Delete fail closed instead of erasing region data") &&
       ok;
  command(cr::CreativeInputActionId::PasteClipboard,
          cr::CreativeInputKey::V);
  ok = expect(editor.terrain.region.stamp.active,
              "Paste starts the terrain stamp preview through semantic routing") &&
       ok;
  static_cast<void>(cancelCreativeEditorTerrainStamp(editor));
  command(cr::CreativeInputActionId::DuplicateSelection,
          cr::CreativeInputKey::D);
  return expect(editor.terrain.region.stamp.active &&
                    cr::isValidCreativeTerrainStamp(appState.terrainStamp) &&
                    cr::creativeClipboardEmpty(appState.clipboard) &&
                    appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "Duplicate refreshes the terrain stamp and begins one preview") &&
         ok;
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
  CreativeEditorSceneCache cache;
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document());
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
      cache, appState.facade.document());
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
  CreativeEditorSceneCache cache;
  if (!expect(refreshCreativeEditorSceneCache(
                  cache, appState.facade.document()),
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
  CreativeEditorSceneCache cache;
  const bool initialRefresh = refreshCreativeEditorSceneCache(
      cache, document);
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
      cache, document);
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
  CreativeEditorSceneCache cache;
  const bool refreshed = refreshCreativeEditorSceneCache(
      cache, appState.facade.document());
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
                 terrainSurfacePaintRoutesGesturesHistorySamplingAndRendering() &&
                 terrainConnectedAndRegionModesUsePressBasedAtomicGestures() &&
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
                 terrainPathLocksBendsPreviewsCommitsAndUndoesOneBatch() &&
                 terrainPathRoutesSquareXAndCircleThroughWorldActions() &&
                 terrainRegionPreviewApplyAndUndoStayAtomic() &&
                 terrainRegionRoutesCornersSampleCancelAndQuickEdit() &&
                 terrainStampCopiesPreviewsTransformsCommitsAndRepeats() &&
                 terrainStampSurfaceAlignmentAndHeightOffsetStayPreviewExact() &&
                 terrainStampCommandsUseTheTerrainClipboardLane() &&
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
