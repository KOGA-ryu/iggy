#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "app/iggy3d/creative/tools/SelectionTransformCommands.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeToolInputPacket pointerInput(cr::CreativeToolInputKind kind,
                                         double x = 10.0,
                                         double y = 20.0) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.x = x;
  input.pointer.y = y;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = 42;
  return input;
}

bool defaultStateUsesSelect() {
  const cr::CreativeToolState state = cr::makeDefaultCreativeToolState();

  return expect(state.activeTool == cr::Tool::Select,
                "default active tool select") &&
         expect(!state.measurementActive, "default measurement inactive");
}

bool changingActiveToolWorks() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  bool ok = true;
  const cr::Tool tools[] = {cr::Tool::Move,
                            cr::Tool::Measure,
                            cr::Tool::Navigate,
                            cr::Tool::Select};
  for (const cr::Tool tool : tools) {
    ok = expect(cr::setActiveTool(state, tool), "active tool changed") && ok;
    ok = expect(state.activeTool == tool, "active tool stored") && ok;
  }
  return ok;
}

bool sameToolActivationIsNoChange() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool changed = cr::setActiveTool(state, cr::Tool::Select);

  return expect(!changed, "same active tool no change") &&
         expect(state.activeTool == cr::Tool::Select,
                "same active tool remains select");
}

bool pointerMoveEmitsPreviewIntent() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerMove));

  return expect(receipt.accepted, "preview accepted") &&
         expect(receipt.emittedIntentCount == 1U, "preview intent count") &&
         expect(receipt.intents.size() == 1U, "preview intent size") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::PreviewPointer,
                "preview intent kind") &&
         expect(receipt.activeToolBefore == cr::Tool::Select,
                "preview tool before") &&
         expect(receipt.activeToolAfter == cr::Tool::Select,
                "preview tool after");
}

bool selectPressEmitsSelectObjectCandidate() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress));

  return expect(receipt.accepted, "select press accepted") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::PointerPress,
                "select press input kind") &&
         expect(receipt.emittedIntentCount == 1U, "select press count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::SelectObjectCandidate,
                "select press intent") &&
         expect(receipt.intents[0].pointer.target.value == 42U,
                "select press target forwarded");
}

bool movePressSelectsAndBeginsDrag() {
  // TV1-G: a Move-tool press selects (TV1-C) AND begins a drag (TD-6).
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool toolChanged = cr::setActiveTool(state, cr::Tool::Move);

  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress));

  return expect(toolChanged, "move setup changed tool") &&
         expect(receipt.accepted, "move press accepted") &&
         expect(receipt.activeToolBefore == cr::Tool::Move,
                "move press tool before") &&
         expect(receipt.activeToolAfter == cr::Tool::Move,
                "move press tool after") &&
         expect(receipt.emittedIntentCount == 2U, "move press count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::SelectObjectCandidate,
                "move press select intent") &&
         expect(receipt.intents[1].kind ==
                    cr::CreativeToolIntentKind::BeginMove,
                "move press begin-move intent") &&
         expect(receipt.intents[0].pointer.target.value == 42U,
                "move press select target forwarded") &&
         expect(receipt.intents[1].pointer.target.value == 42U,
                "move press begin target forwarded") &&
         expect(state.moveDragActive, "move press activates drag") &&
         expect(state.moveDragTarget.value == 42U, "move press records target") &&
         expect(receipt.message == "move_drag_begin", "move press message");
}

bool moveDragPreviewCommitLifecycle() {
  // Press -> Move (preview) -> Release (commit) drives the drag state machine.
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));
  static_cast<void>(cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerPress)));

  const cr::CreativeToolDispatchReceipt preview = cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerMove, 30.0, 40.0));
  const bool previewOk =
      expect(preview.emittedIntentCount == 1U, "drag preview count") &&
      expect(preview.intents[0].kind ==
                 cr::CreativeToolIntentKind::PreviewMove,
             "drag preview intent") &&
      expect(preview.message == "move_preview", "drag preview message") &&
      expect(state.moveDragActive, "drag still active during preview");

  const cr::CreativeToolDispatchReceipt commit = cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerRelease, 30.0, 40.0));
  const bool commitOk =
      expect(commit.emittedIntentCount == 1U, "drag commit count") &&
      expect(commit.intents[0].kind ==
                 cr::CreativeToolIntentKind::CommitMove,
             "drag commit intent") &&
      expect(commit.message == "move_drag_commit", "drag commit message") &&
      expect(!state.moveDragActive, "drag cleared after commit") &&
      expect(state.moveDragTarget.value == cr::kInvalidId,
             "drag target cleared after commit");
  return previewOk && commitOk;
}

bool releaseWithoutDragIsNoOp() {
  // TV1-F entry req (ii): a Release with no active drag is a harmless no-op.
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));

  const cr::CreativeToolDispatchReceipt receipt = cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerRelease));

  return expect(receipt.accepted, "orphan release accepted") &&
         expect(receipt.emittedIntentCount == 0U, "orphan release no intent") &&
         expect(receipt.message == "no_intent", "orphan release message") &&
         expect(!state.moveDragActive, "orphan release leaves drag inactive");
}

bool cancelMidDragDiscardsWithoutMutation() {
  // TD-6: Esc/Cancel mid-drag discards the drag, emitting CancelMove.
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));
  static_cast<void>(cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerPress)));

  cr::CreativeToolInputPacket cancel;
  cancel.kind = cr::CreativeToolInputKind::Cancel;
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state, cancel);

  return expect(receipt.emittedIntentCount == 1U, "cancel drag count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::CancelMove,
                "cancel drag intent") &&
         expect(receipt.message == "move_drag_cancel", "cancel drag message") &&
         expect(!state.moveDragActive, "cancel clears drag");
}

bool toolSwitchAbandonsDrag() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));
  static_cast<void>(cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerPress)));

  const bool switched = cr::setActiveTool(state, cr::Tool::Select);

  return expect(switched, "tool switched") &&
         expect(!state.moveDragActive, "tool switch abandons drag") &&
         expect(state.moveDragTarget.value == cr::kInvalidId,
                "tool switch clears drag target");
}

bool moveToolPointerMoveKeepsGhostPreview() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));

  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerMove));

  return expect(receipt.accepted, "move preview accepted") &&
         expect(receipt.emittedIntentCount == 1U, "move preview count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::PreviewPointer,
                "move preview intent") &&
         expect(receipt.message == "preview_pointer",
                "move preview message");
}

bool navigatePointerInputIsInert() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Navigate));

  bool ok = true;
  const cr::CreativeToolInputKind kinds[] = {
      cr::CreativeToolInputKind::PointerPress,
      cr::CreativeToolInputKind::PointerMove,
      cr::CreativeToolInputKind::PointerRelease,
  };
  for (const cr::CreativeToolInputKind kind : kinds) {
    const cr::CreativeToolDispatchReceipt receipt =
        cr::dispatchToolInput(state, pointerInput(kind));
    ok = expect(receipt.accepted, "navigate input accepted") && ok;
    ok = expect(receipt.emittedIntentCount == 0U,
                "navigate input no intents") && ok;
    ok = expect(receipt.intents.empty(), "navigate input intents empty") && ok;
    ok = expect(!receipt.changedState, "navigate input unchanged") && ok;
    ok = expect(receipt.message == "navigate_pointer_inert",
                "navigate input message") && ok;
  }
  ok = expect(state.pointer.target.value == cr::kInvalidId,
              "navigate pointer target untouched") && ok;
  return ok;
}

bool measureClicksPreviewAndCompletePaths() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool toolChanged = cr::setActiveTool(state, cr::Tool::Measure);

  const cr::CreativeToolDispatchReceipt begin =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress,
                                         1.0,
                                         2.0));
  const bool began =
      expect(toolChanged, "measure setup changed tool") &&
      expect(begin.emittedIntentCount == 1U, "measure begin count") &&
      expect(begin.intents[0].kind ==
                 cr::CreativeToolIntentKind::AppendMeasurementPoint,
             "measure append intent") &&
      expect(state.measurementActive, "measure active after begin");

  const cr::CreativeToolDispatchReceipt update =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerMove,
                                         3.0,
                                         4.0));
  const bool updated =
      expect(update.emittedIntentCount == 1U, "measure update count") &&
      expect(update.intents[0].kind ==
                 cr::CreativeToolIntentKind::UpdateMeasurement,
             "measure update intent") &&
      expect(state.measurementActive, "measure active after update");

  const cr::CreativeToolDispatchReceipt release =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerRelease,
                                         5.0,
                                         6.0));
  const bool activeAfterRelease = state.measurementActive;
  cr::CreativeToolInputPacket completeInput =
      pointerInput(cr::CreativeToolInputKind::PointerPress, 5.0, 6.0);
  completeInput.pointer.button = cr::CreativeToolPointerButton::Secondary;
  const cr::CreativeToolDispatchReceipt complete =
      cr::dispatchToolInput(state, completeInput);
  const bool ended =
      expect(release.emittedIntentCount == 0U, "measure release is inert") &&
      expect(activeAfterRelease, "measure remains active after release") &&
      expect(complete.emittedIntentCount == 1U, "measure complete count") &&
      expect(complete.intents[0].kind ==
                 cr::CreativeToolIntentKind::CompleteMeasurement,
             "measure complete intent") &&
      expect(!state.measurementActive, "measure inactive after end");

  return began && updated && ended;
}

bool unknownInputEmitsNoIntent() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state, {});

  return expect(!receipt.accepted, "unknown input not accepted") &&
         expect(!receipt.changedState, "unknown input unchanged") &&
         expect(receipt.emittedIntentCount == 0U, "unknown input count") &&
         expect(receipt.intents.empty(), "unknown input no intents") &&
         expect(receipt.message == "unsupported_input",
                "unknown input message") &&
         expect(state.activeTool == cr::Tool::Select,
                "unknown input keeps active tool");
}

bool optionDescriptorsAreContextualAndBounded() {
  const std::span<const cr::CreativeToolOptionDescriptor> descriptors =
      cr::creativeToolOptionDescriptors();
  const cr::CreativeToolOptionList material =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::Material);
  const cr::CreativeToolOptionList materialBrush =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::MaterialBrush);
  cr::CreativeToolSettings cylinderSettings =
      cr::makeDefaultCreativeToolSettings();
  cylinderSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cylinder;
  const cr::CreativeToolOptionList cylinderBrush =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::MaterialBrush, cylinderSettings);
  cr::CreativeToolSettings replaceBrushSettings =
      cr::makeDefaultCreativeToolSettings();
  replaceBrushSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::Replace;
  const cr::CreativeToolOptionList replaceBrush =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::MaterialBrush, replaceBrushSettings);
  replaceBrushSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cylinder;
  const cr::CreativeToolOptionList cylinderReplaceBrush =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::MaterialBrush, replaceBrushSettings);
  const cr::CreativeToolOptionList move =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::ObjectMove);
  const cr::CreativeToolOptionList room =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::BuildingRoom);
  const cr::CreativeToolOptionList replace =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::VolumeReplace);
  const cr::CreativeToolOptionList erase =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::VolumeErase);
  const cr::CreativeToolOptionList fill =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::VolumeFill);
  const cr::CreativeToolOptionList hollow =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::VolumeHollow);
  const cr::CreativeToolOptionList clone =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::VolumeClone);
  const cr::CreativeToolOptionList connectedFill =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::ConnectedFill);
  const cr::CreativeToolOptionList surfaceExtrude =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::SurfaceExtrude);
  const cr::CreativeToolOptionList terrainSculpt =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainSculpt);
  cr::CreativeToolSettings terrainRaiseSettings =
      cr::makeDefaultCreativeToolSettings();
  terrainRaiseSettings.terrainSculptMode = cr::CreativeTerrainSculptMode::Raise;
  const cr::CreativeToolOptionList terrainRaise =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainSculpt, terrainRaiseSettings);
  const cr::CreativeToolOptionList terrainRodSingle =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainControl);
  cr::CreativeToolSettings terrainSeedSettings =
      cr::makeDefaultCreativeToolSettings();
  terrainSeedSettings.terrainRodStampMode =
      cr::CreativeTerrainRodStampMode::Seed;
  const cr::CreativeToolOptionList terrainSeed =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainControl, terrainSeedSettings);
  const cr::CreativeToolOptionList terrainProfile =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainProfile);
  const cr::CreativeToolOptionList terrainPath =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainPath);
  const cr::CreativeToolOptionList terrainRegion =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainRegion);
  cr::CreativeToolSettings flattenRegionSettings =
      cr::makeDefaultCreativeToolSettings();
  flattenRegionSettings.terrainRegionRecipe.mode =
      cr::CreativeTerrainRegionMode::Flatten;
  const cr::CreativeToolOptionList flattenRegion =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainRegion, flattenRegionSettings);
  cr::CreativeToolSettings noiseRegionSettings =
      cr::makeDefaultCreativeToolSettings();
  noiseRegionSettings.terrainRegionRecipe.mode =
      cr::CreativeTerrainRegionMode::Noise;
  const cr::CreativeToolOptionList noiseRegion =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainRegion, noiseRegionSettings);
  cr::CreativeToolSettings ridgeSettings =
      cr::makeDefaultCreativeToolSettings();
  ridgeSettings.terrainProfileKind = cr::CreativeTerrainProfileKind::Ridge;
  const cr::CreativeToolOptionList terrainRidge =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainProfile, ridgeSettings);
  cr::CreativeToolSettings waveSettings =
      cr::makeDefaultCreativeToolSettings();
  waveSettings.terrainProfileKind = cr::CreativeTerrainProfileKind::Wave;
  const cr::CreativeToolOptionList terrainWave =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainProfile, waveSettings);
  waveSettings.terrainProfileRodPolicy =
      cr::CreativeTerrainProfileRodPolicy::Existing;
  const cr::CreativeToolOptionList terrainWaveExisting =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainProfile, waveSettings);
  const cr::CreativeToolOptionList array =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::LinearArray);
  cr::CreativeToolSettings radialSettings =
      cr::makeDefaultCreativeToolSettings();
  radialSettings.arrayMode = cr::CreativeArrayMode::Radial;
  const cr::CreativeToolOptionList radialArray =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::LinearArray,
                                         radialSettings);

  return expect(descriptors.size() == cr::kCreativeToolOptionDescriptorCount,
                "global descriptor table has one row per option id") &&
         expect(material.count == 6U &&
                    material.ids[0] ==
                        cr::CreativeToolOptionId::PlacementYaw &&
                    material.ids[1] == cr::CreativeToolOptionId::SnapIncrement &&
                    material.ids[2] ==
                        cr::CreativeToolOptionId::PlacementGridDots &&
                    material.ids[3] ==
                        cr::CreativeToolOptionId::PlacementPlane &&
                    material.ids[4] ==
                        cr::CreativeToolOptionId::PlacementAnchor &&
                    material.ids[5] ==
                        cr::CreativeToolOptionId::PlacementDepth,
                "material exposes orientation grid dots plane anchor and depth") &&
         expect(room.count == 4U &&
                    room.ids[0] == cr::CreativeToolOptionId::SnapIncrement &&
                    room.ids[1] == cr::CreativeToolOptionId::RoomWallHeight &&
                    room.ids[2] ==
                        cr::CreativeToolOptionId::RoomWallThickness &&
                    room.ids[3] ==
                        cr::CreativeToolOptionId::RoomFloorThickness &&
                    !room.capacityExceeded,
                "room exposes grid height wall and floor dimensions") &&
         expect(materialBrush.count == 6U &&
                    materialBrush.ids[0] ==
                        cr::CreativeToolOptionId::MaterialBrushShape &&
                    materialBrush.ids[1] ==
                        cr::CreativeToolOptionId::MaterialBrushSize &&
                    materialBrush.ids[2] ==
                        cr::CreativeToolOptionId::MaterialBrushFill &&
                    materialBrush.ids[3] ==
                        cr::CreativeToolOptionId::MaterialBrushGuide &&
                    materialBrush.ids[4] ==
                        cr::CreativeToolOptionId::MaterialBrushSymmetry &&
                    materialBrush.ids[5] ==
                        cr::CreativeToolOptionId::MaterialBrushMask,
                "material brush exposes shape size body guide symmetry and mask") &&
         expect(cylinderBrush.count == 7U &&
                    cylinderBrush.ids[0] ==
                        cr::CreativeToolOptionId::MaterialBrushShape &&
                    cylinderBrush.ids[1] ==
                        cr::CreativeToolOptionId::MaterialBrushAxis &&
                    cylinderBrush.ids[2] ==
                        cr::CreativeToolOptionId::MaterialBrushSize &&
                    cylinderBrush.ids[3] ==
                        cr::CreativeToolOptionId::MaterialBrushFill &&
                    cylinderBrush.ids[4] ==
                        cr::CreativeToolOptionId::MaterialBrushGuide &&
                    cylinderBrush.ids[5] ==
                        cr::CreativeToolOptionId::MaterialBrushSymmetry &&
                    cylinderBrush.ids[6] ==
                        cr::CreativeToolOptionId::MaterialBrushMask,
                "cylinder brush exposes its contextual extrusion axis") &&
         expect(replaceBrush.count == 7U &&
                    replaceBrush.ids[6] ==
                        cr::CreativeToolOptionId::MaterialBrushReplaceSource,
                "replace brush exposes its contextual source filter") &&
         expect(cylinderReplaceBrush.count == 8U &&
                    cylinderReplaceBrush.ids[1] ==
                        cr::CreativeToolOptionId::MaterialBrushAxis &&
                    cylinderReplaceBrush.ids[7] ==
                        cr::CreativeToolOptionId::MaterialBrushReplaceSource,
                "cylinder replace exposes all eight contextual options") &&
         expect(move.count == 3U &&
                    move.ids[0] ==
                        cr::CreativeToolOptionId::MoveConstraint &&
                    move.ids[1] == cr::CreativeToolOptionId::RotationStep &&
                    move.ids[2] == cr::CreativeToolOptionId::SnapIncrement,
                "move options retain descriptor order") &&
         expect(fill.count == 4U && hollow.count == 7U &&
                    fill.ids[0] == cr::CreativeToolOptionId::SnapIncrement &&
                    fill.ids[1] == cr::CreativeToolOptionId::ShapeBrushKind &&
                    fill.ids[2] == cr::CreativeToolOptionId::ShapeBrushAxis &&
                    fill.ids[3] ==
                        cr::CreativeToolOptionId::VolumeFillOverlapPolicy &&
                    hollow.ids[1] ==
                        cr::CreativeToolOptionId::ShapeBrushKind &&
                    hollow.ids[3] ==
                        cr::CreativeToolOptionId::VolumeHollowThickness &&
                    hollow.ids[4] ==
                        cr::CreativeToolOptionId::VolumeHollowAlignment &&
                    hollow.ids[5] ==
                        cr::CreativeToolOptionId::VolumeHollowOpening &&
                    hollow.ids[6] ==
                        cr::CreativeToolOptionId::VolumeHollowCornerRule,
                "fill and hollow expose their distinct volume contracts") &&
         expect(replace.count == 3U &&
                    replace.ids[1] ==
                        cr::CreativeToolOptionId::ReplaceSource &&
                    replace.ids[2] ==
                        cr::CreativeToolOptionId::ReplaceMemberMask,
                "replace exposes source and member filters") &&
         expect(erase.count == 3U &&
                    erase.ids[1] == cr::CreativeToolOptionId::EraseSource &&
                    erase.ids[2] ==
                        cr::CreativeToolOptionId::EraseMemberMask,
                "erase exposes source and member filters") &&
         expect(clone.count == 7U &&
                    clone.ids[1] ==
                        cr::CreativeToolOptionId::CloneOffsetAxis &&
                    clone.ids[2] ==
                        cr::CreativeToolOptionId::CloneOffsetDistance &&
                    clone.ids[3] ==
                        cr::CreativeToolOptionId::CloneRotation &&
                    clone.ids[4] == cr::CreativeToolOptionId::CloneMirror &&
                    clone.ids[5] ==
                        cr::CreativeToolOptionId::CloneMemberMask &&
                    clone.ids[6] ==
                        cr::CreativeToolOptionId::CloneVoxelOverlapPolicy,
                "clone exposes transform member and voxel overlap controls") &&
         expect(connectedFill.count == 1U &&
                    connectedFill.ids[0] ==
                        cr::CreativeToolOptionId::ConnectedFillLimit,
                "connected fill exposes only its bounded region limit") &&
         expect(surfaceExtrude.count == 2U &&
                    surfaceExtrude.ids[0] ==
                        cr::CreativeToolOptionId::SurfaceExtrudeDepth &&
                    surfaceExtrude.ids[1] ==
                        cr::CreativeToolOptionId::SurfaceExtrudeLimit,
                "surface extrude exposes depth and affected-cell limit") &&
         expect(terrainSculpt.count == 6U &&
                    terrainSculpt.ids[0] ==
                        cr::CreativeToolOptionId::TerrainSculptMode &&
                    terrainSculpt.ids[1] ==
                        cr::CreativeToolOptionId::TerrainSculptRadius &&
                    terrainSculpt.ids[2] ==
                        cr::CreativeToolOptionId::TerrainSculptStrength &&
                    terrainSculpt.ids[3] ==
                        cr::CreativeToolOptionId::TerrainSculptTargetHeight &&
                    terrainSculpt.ids[4] ==
                        cr::CreativeToolOptionId::TerrainSculptFalloff &&
                    terrainSculpt.ids[5] ==
                        cr::CreativeToolOptionId::TerrainSculptMask,
                "flatten sculpt exposes mode radius strength target falloff and mask") &&
         expect(terrainRaise.count == 5U &&
                    terrainRaise.ids[0] ==
                        cr::CreativeToolOptionId::TerrainSculptMode &&
                    terrainRaise.ids[1] ==
                        cr::CreativeToolOptionId::TerrainSculptRadius &&
                    terrainRaise.ids[2] ==
                        cr::CreativeToolOptionId::TerrainSculptStrength &&
                    terrainRaise.ids[3] ==
                        cr::CreativeToolOptionId::TerrainSculptFalloff &&
                    terrainRaise.ids[4] ==
                        cr::CreativeToolOptionId::TerrainSculptMask,
                "non-flatten sculpt hides the irrelevant target height") &&
         expect(terrainRodSingle.count == 1U &&
                    terrainRodSingle.ids[0] ==
                        cr::CreativeToolOptionId::TerrainRodStampMode,
                "single terrain rod mode hides irrelevant seed settings") &&
         expect(terrainSeed.count == 3U &&
                    terrainSeed.ids[0] ==
                        cr::CreativeToolOptionId::TerrainRodStampMode &&
                    terrainSeed.ids[1] ==
                        cr::CreativeToolOptionId::TerrainSeedRadius &&
                    terrainSeed.ids[2] ==
                        cr::CreativeToolOptionId::TerrainSeedSpacing,
                "terrain seed exposes stamp mode radius and spacing") &&
         expect(terrainProfile.count == 6U &&
                    terrainProfile.ids[0] ==
                        cr::CreativeToolOptionId::TerrainProfileKind &&
                    terrainProfile.ids[5] ==
                        cr::CreativeToolOptionId::TerrainProfileSpacing,
                "hill profile exposes six relevant settings") &&
         expect(terrainRidge.count == 7U &&
                    terrainRidge.ids[6] ==
                        cr::CreativeToolOptionId::TerrainProfileDirection,
                "ridge adds direction without frequency") &&
         expect(terrainWave.count == 9U &&
                    terrainWave.ids[6] ==
                        cr::CreativeToolOptionId::TerrainProfileDirection &&
                    terrainWave.ids[7] ==
                        cr::CreativeToolOptionId::TerrainProfileFrequency &&
                    terrainWave.ids[8] ==
                        cr::CreativeToolOptionId::TerrainProfileSeed &&
                    !terrainWave.capacityExceeded,
                "wave exposes frequency and deterministic seed") &&
         expect(terrainWaveExisting.count == 8U &&
                    std::find(terrainWaveExisting.items().begin(),
                              terrainWaveExisting.items().end(),
                              cr::CreativeToolOptionId::TerrainProfileSpacing) ==
                        terrainWaveExisting.items().end(),
                "existing-only profile hides lattice spacing") &&
         expect(terrainPath.count == 4U &&
                    terrainPath.ids[0] ==
                        cr::CreativeToolOptionId::TerrainPathKind &&
                    terrainPath.ids[1] ==
                        cr::CreativeToolOptionId::TerrainPathElevation &&
                    terrainPath.ids[2] ==
                        cr::CreativeToolOptionId::TerrainPathWidth &&
                    terrainPath.ids[3] ==
                        cr::CreativeToolOptionId::TerrainPathAmplitude,
                "terrain path exposes type elevation width and rise depth") &&
         expect(terrainRegion.count == 6U &&
                    terrainRegion.ids[0] ==
                        cr::CreativeToolOptionId::TerrainRegionOperation &&
                    terrainRegion.ids[1] ==
                        cr::CreativeToolOptionId::TerrainRegionMask &&
                    terrainRegion.ids[2] ==
                        cr::CreativeToolOptionId::TerrainRegionAmount &&
                    terrainRegion.ids[3] ==
                        cr::CreativeToolOptionId::TerrainRegionFeather &&
                    terrainRegion.ids[4] ==
                        cr::CreativeToolOptionId::TerrainStampMode &&
                    terrainRegion.ids[5] ==
                        cr::CreativeToolOptionId::TerrainStampElevation &&
                    flattenRegion.count == 6U &&
                    flattenRegion.ids[0] ==
                        cr::CreativeToolOptionId::TerrainRegionOperation &&
                    flattenRegion.ids[1] ==
                        cr::CreativeToolOptionId::TerrainRegionMask &&
                    flattenRegion.ids[2] ==
                        cr::CreativeToolOptionId::TerrainRegionTargetHeight &&
                    flattenRegion.ids[3] ==
                        cr::CreativeToolOptionId::TerrainRegionFeather &&
                    flattenRegion.ids[4] ==
                        cr::CreativeToolOptionId::TerrainStampMode &&
                    flattenRegion.ids[5] ==
                        cr::CreativeToolOptionId::TerrainStampElevation &&
                    noiseRegion.count == 9U &&
                    noiseRegion.ids[2] ==
                        cr::CreativeToolOptionId::TerrainRegionTargetHeight &&
                    noiseRegion.ids[3] ==
                        cr::CreativeToolOptionId::TerrainRegionNoiseRelief &&
                    noiseRegion.ids[4] ==
                        cr::CreativeToolOptionId::TerrainRegionNoiseScale &&
                    noiseRegion.ids[5] ==
                        cr::CreativeToolOptionId::TerrainRegionSeed &&
                    noiseRegion.ids[6] ==
                        cr::CreativeToolOptionId::TerrainRegionFeather,
                "terrain region exposes only the canonical mode parameters") &&
         expect(array.count == 4U &&
                    array.ids[0] ==
                        cr::CreativeToolOptionId::ArrayMode &&
                    array.ids[1] ==
                        cr::CreativeToolOptionId::ArrayDirection &&
                    array.ids[2] ==
                        cr::CreativeToolOptionId::ArrayCopyCount &&
                    array.ids[3] ==
                        cr::CreativeToolOptionId::ArraySpacing,
                "linear array exposes mode, direction, copies, and spacing") &&
         expect(radialArray.count == 4U &&
                    radialArray.ids[0] ==
                        cr::CreativeToolOptionId::ArrayMode &&
                    radialArray.ids[1] ==
                        cr::CreativeToolOptionId::RadialArrayAxis &&
                    radialArray.ids[2] ==
                        cr::CreativeToolOptionId::RadialArrayInstanceCount &&
                    radialArray.ids[3] ==
                        cr::CreativeToolOptionId::RadialArraySweep,
                "radial array hides irrelevant linear settings") &&
         expect(!material.capacityExceeded &&
                    !materialBrush.capacityExceeded &&
                    !cylinderBrush.capacityExceeded && !move.capacityExceeded &&
                    !replaceBrush.capacityExceeded &&
                    !cylinderReplaceBrush.capacityExceeded &&
                    !fill.capacityExceeded && !hollow.capacityExceeded &&
                    !replace.capacityExceeded && !clone.capacityExceeded &&
                    !connectedFill.capacityExceeded &&
                    !surfaceExtrude.capacityExceeded &&
                    !terrainSculpt.capacityExceeded &&
                    !terrainRodSingle.capacityExceeded &&
                    !terrainSeed.capacityExceeded &&
                    !terrainProfile.capacityExceeded &&
                    !terrainPath.capacityExceeded &&
                    !terrainRegion.capacityExceeded &&
                    !flattenRegion.capacityExceeded &&
                    !noiseRegion.capacityExceeded &&
                    !terrainRidge.capacityExceeded &&
                    !terrainWaveExisting.capacityExceeded &&
                    !array.capacityExceeded && !radialArray.capacityExceeded &&
                    array.count <= cr::kCreativeToolOptionCapacity,
                "default option lists fit bounded storage") &&
         expect(cr::creativeToolOptionDescriptor(
                    cr::CreativeToolOptionId::Count) == nullptr &&
                    !cr::creativeToolOptionAppliesToHeldItem(
                        cr::CreativeToolOptionId::MoveConstraint,
                        cr::CreativeHeldItemKind::Material) &&
                    sizeof(cr::CreativeHeldItemMask) == sizeof(std::uint32_t) &&
                    cr::creativeToolOptionAppliesToHeldItem(
                        cr::CreativeToolOptionId::TerrainProfileKind,
                        cr::CreativeHeldItemKind::TerrainProfile) &&
                    cr::creativeToolOptionAppliesToHeldItem(
                        cr::CreativeToolOptionId::TerrainRegionOperation,
                        cr::CreativeHeldItemKind::TerrainRegion),
                "invalid and inapplicable options are rejected");
}

bool optionAdjustmentIsDeterministicAndAtomic() {
  cr::CreativeToolSettings settings = cr::makeDefaultCreativeToolSettings();
  cr::CreativeToolSettings invalidMask = settings;
  invalidMask.materialBrushMask = cr::CreativeMaterialBrushMask::Count;
  cr::CreativeToolSettings invalidGridDots = settings;
  invalidGridDots.placementGridDots = cr::CreativePlacementGridDots::Count;
  cr::CreativeToolSettings invalidPlacementPlane = settings;
  invalidPlacementPlane.placementPlane = cr::CreativePlacementPlane::Count;
  cr::CreativeToolSettings invalidPlacementAnchor = settings;
  invalidPlacementAnchor.placementAnchor =
      cr::CreativePlacementAnchor::Count;
  cr::CreativeToolSettings invalidPlacementDepth = settings;
  invalidPlacementDepth.placementDepth = cr::CreativePlacementDepth::Count;
  cr::CreativeToolSettings invalidVolumeFillOverlap = settings;
  invalidVolumeFillOverlap.volumeFillOverlapPolicy =
      cr::CreativeVolumeFillOverlapPolicy::Count;
  cr::CreativeToolSettings invalidVolumeHollow = settings;
  invalidVolumeHollow.volumeHollowCornerRule =
      cr::CreativeVolumeHollowCornerRule::Count;
  cr::CreativeToolSettings invalidVolumeReplaceMask = settings;
  invalidVolumeReplaceMask.volumeReplaceMemberMask =
      cr::CreativeVolumeMemberMask::Count;
  cr::CreativeToolSettings invalidVolumeEraseMask = settings;
  invalidVolumeEraseMask.volumeEraseMemberMask =
      cr::CreativeVolumeMemberMask::Count;
  cr::CreativeToolSettings invalidVolumeEraseSource = settings;
  invalidVolumeEraseSource.eraseSourceKind = cr::CreativeObjectKind::Count;
  cr::CreativeToolSettings invalidCloneRotation = settings;
  invalidCloneRotation.cloneRotation = cr::CreativeCloneRotation::Count;
  cr::CreativeToolSettings invalidCloneMirror = settings;
  invalidCloneMirror.cloneMirror = cr::CreativeCloneMirror::Count;
  cr::CreativeToolSettings invalidCloneMemberMask = settings;
  invalidCloneMemberMask.volumeCloneMemberMask =
      cr::CreativeVolumeMemberMask::Count;
  cr::CreativeToolSettings invalidCloneVoxelOverlap = settings;
  invalidCloneVoxelOverlap.cloneVoxelOverlapPolicy =
      cr::CreativeVolumeCloneVoxelOverlapPolicy::Count;
  cr::CreativeToolSettings invalidBrushAxis = settings;
  invalidBrushAxis.materialBrushAxis = cr::CreativeAxis3::Count;
  cr::CreativeToolSettings invalidBrushFill = settings;
  invalidBrushFill.materialBrushFill = cr::CreativeMaterialBrushFill::Count;
  cr::CreativeToolSettings invalidBrushGuide = settings;
  invalidBrushGuide.materialBrushGuide =
      cr::CreativeMaterialBrushGuide::Count;
  cr::CreativeToolSettings invalidBrushSymmetry = settings;
  invalidBrushSymmetry.materialBrushSymmetry =
      cr::CreativeMaterialBrushSymmetry::Count;
  cr::CreativeToolSettings invalidBrushSource = settings;
  invalidBrushSource.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Count;
  cr::CreativeToolSettings invalidBrushPropSource = settings;
  invalidBrushPropSource.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Crate;
  cr::CreativeToolSettings invalidConnectedFillLimit = settings;
  invalidConnectedFillLimit.connectedFillLimit =
      cr::CreativeConnectedFillLimit::Count;
  cr::CreativeToolSettings invalidSurfaceDepth = settings;
  invalidSurfaceDepth.surfaceExtrudeDepth =
      cr::CreativeSurfaceExtrudeDepth::Count;
  cr::CreativeToolSettings invalidSurfaceLimit = settings;
  invalidSurfaceLimit.surfaceExtrudeLimit =
      cr::CreativeConnectedFillLimit::Count;
  cr::CreativeToolSettings invalidSculpt = settings;
  invalidSculpt.terrainSculptMode = cr::CreativeTerrainSculptMode::Count;
  cr::CreativeToolSettings invalidSculptFalloff = settings;
  invalidSculptFalloff.terrainSculptFalloff =
      cr::CreativeTerrainSculptFalloff::Count;
  cr::CreativeToolSettings invalidSculptMask = settings;
  invalidSculptMask.terrainSculptMask =
      cr::CreativeTerrainSculptMask::Count;
  cr::CreativeToolSettings invalidSculptTarget = settings;
  invalidSculptTarget.terrainSculptTargetHeightCells = 0U;
  cr::CreativeToolSettings invalidSeed = settings;
  invalidSeed.terrainSeedSpacing = cr::CreativeTerrainSeedSpacing::Count;
  cr::CreativeToolSettings invalidProfile = settings;
  invalidProfile.terrainProfileKind = cr::CreativeTerrainProfileKind::Count;
  cr::CreativeToolSettings invalidProfileRadius = settings;
  invalidProfileRadius.terrainProfileRadiusCells = 0U;
  cr::CreativeToolSettings invalidPath = settings;
  invalidPath.terrainPathElevation =
      cr::CreativeTerrainPathElevation::Count;
  cr::CreativeToolSettings invalidRegionOperation = settings;
  invalidRegionOperation.terrainRegionRecipe.mode =
      cr::CreativeTerrainRegionMode::Count;
  cr::CreativeToolSettings invalidRegionAmount = settings;
  invalidRegionAmount.terrainRegionRecipe.amountCells = 0U;
  cr::CreativeToolSettings invalidStampMode = settings;
  invalidStampMode.terrainStampMode = cr::CreativeTerrainStampMode::Count;
  cr::CreativeToolSettings invalidStampElevation = settings;
  invalidStampElevation.terrainStampElevationMode =
      cr::CreativeTerrainStampElevationMode::Count;
  bool ok = expect(cr::isValidCreativeToolSettings(settings),
                   "default settings valid") &&
            expect(!cr::isValidCreativeToolSettings(invalidMask),
                   "invalid material brush mask fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidGridDots) &&
                       !cr::isValidCreativeToolSettings(invalidPlacementPlane) &&
                       !cr::isValidCreativeToolSettings(invalidPlacementAnchor) &&
                       !cr::isValidCreativeToolSettings(invalidPlacementDepth),
                   "invalid placement grid settings fail validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidVolumeFillOverlap),
                   "invalid volume fill overlap policy fails validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidVolumeHollow),
                   "invalid volume hollow setting fails validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidVolumeReplaceMask),
                   "invalid volume replace member mask fails validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidVolumeEraseMask) &&
                       !cr::isValidCreativeToolSettings(
                           invalidVolumeEraseSource),
                   "invalid volume erase filters fail validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidCloneRotation) &&
                       !cr::isValidCreativeToolSettings(invalidCloneMirror) &&
                       !cr::isValidCreativeToolSettings(
                           invalidCloneMemberMask) &&
                       !cr::isValidCreativeToolSettings(
                           invalidCloneVoxelOverlap),
                   "invalid volume clone settings fail validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushAxis),
                   "invalid material brush axis fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushFill),
                   "invalid material brush fill fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushGuide),
                   "invalid material brush guide fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushSymmetry),
                   "invalid material brush symmetry fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushSource),
                   "invalid material brush source fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushPropSource),
                   "non-voxel brush source fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(
                       invalidConnectedFillLimit),
                   "invalid connected fill limit fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidSurfaceDepth) &&
                       !cr::isValidCreativeToolSettings(invalidSurfaceLimit),
                   "invalid surface depth or limit fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidSculpt) &&
                       !cr::isValidCreativeToolSettings(invalidSculptFalloff) &&
                       !cr::isValidCreativeToolSettings(invalidSculptMask) &&
                       !cr::isValidCreativeToolSettings(invalidSculptTarget),
                   "invalid terrain sculpt option fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidSeed),
                   "invalid terrain seed option fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidProfile) &&
                       !cr::isValidCreativeToolSettings(invalidProfileRadius),
                   "invalid terrain profile option fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidPath),
                   "invalid terrain path option fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidRegionOperation) &&
                       !cr::isValidCreativeToolSettings(invalidRegionAmount) &&
                       !cr::isValidCreativeToolSettings(invalidStampMode) &&
                       !cr::isValidCreativeToolSettings(invalidStampElevation),
                   "invalid terrain region options fail settings validation") &&
            expect(cr::creativeToolOptionValueLabel(
                       settings,
                       cr::CreativeToolOptionId::MoveConstraint) == "FREE" &&
                       cr::creativeRotationStepDegrees(settings.rotationStep) ==
                           15.0 &&
                       cr::creativePlacementYawRadians(settings.placementYaw) ==
                           0.0 &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::PlacementYaw) == "0 DEG" &&
                       cr::creativeSnapIncrementMeters(settings.snapIncrement) ==
                           1.0 &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::PlacementGridDots) ==
                           "OFF" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::PlacementPlane) ==
                           "AUTO" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::PlacementAnchor) ==
                           "CENTER" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::PlacementDepth) ==
                           "0 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::RoomWallHeight) ==
                           "3 M" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::RoomWallThickness) ==
                           "0.25 M" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::RoomFloorThickness) ==
                           "0.05 M" &&
                       cr::creativeRoomWallHeightMeters(
                           settings.roomWallHeight) == 3.0 &&
                       cr::creativeRoomWallThicknessMeters(
                           settings.roomWallThickness) == 0.25 &&
                       cr::creativeRoomFloorThicknessMeters(
                           settings.roomFloorThickness) == 0.05 &&
                       cr::creativePlacementDepthSteps(
                           settings.placementDepth) == 0U &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::ShapeBrushKind) == "BOX" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::ShapeBrushAxis) == "Y" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::VolumeFillOverlapPolicy) ==
                           "PRESERVE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::VolumeHollowThickness) ==
                           "1 CELL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::VolumeHollowAlignment) ==
                           "INWARD" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::VolumeHollowOpening) ==
                           "CLOSED" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::VolumeHollowCornerRule) ==
                           "KEEP EDGES" &&
                       cr::creativeVolumeHollowThicknessCells(
                           settings.volumeHollowThickness) == 1U &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushShape) ==
                           "SPHERE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushAxis) ==
                           "Y" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushSize) ==
                           "3 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushFill) ==
                           "SOLID" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushGuide) ==
                           "FREE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushSymmetry) ==
                           "OFF" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushMask) ==
                           "OVERWRITE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::
                               MaterialBrushReplaceSource) == "ANY" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::ReplaceMemberMask) ==
                           "BOTH" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::EraseSource) == "ANY" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::EraseMemberMask) ==
                           "BOTH" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::ConnectedFillLimit) ==
                           "256 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::SurfaceExtrudeDepth) ==
                           "1 CELL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::SurfaceExtrudeLimit) ==
                           "256 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSculptMode) ==
                           "FLATTEN" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSculptRadius) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSculptStrength) ==
                           "1 CELL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSculptTargetHeight) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSculptFalloff) ==
                           "UNIFORM" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSculptMask) ==
                           "CIRCLE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRodStampMode) ==
                           "SINGLE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSeedRadius) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainSeedSpacing) ==
                           "2 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileKind) ==
                           "HILL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileBlend) ==
                           "SET" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileRodPolicy) ==
                           "FILL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileRadius) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileAmplitude) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileSpacing) ==
                           "1 CELL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileDirection) ==
                           "+X" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileFrequency) ==
                           "1 CYCLE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainProfileSeed) ==
                           "0" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainPathKind) ==
                           "ROAD" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainPathElevation) ==
                           "FOLLOW" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainPathWidth) ==
                           "3 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainPathAmplitude) ==
                           "1 CELL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionOperation) ==
                           "Raise" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionMask) ==
                           "Rectangle" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionAmount) ==
                           "1 CELL" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionTargetHeight) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionNoiseRelief) ==
                           "4 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionNoiseScale) ==
                           "12 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionSeed) == "1" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainRegionFeather) ==
                           "0 CELLS" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainStampMode) ==
                           "MERGE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::TerrainStampElevation) ==
                           "SURFACE",
                   "default labels and scalar conversions stable");

  const auto adjust = [&settings](cr::CreativeToolOptionId option,
                                  std::int32_t direction) {
    return cr::adjustCreativeToolOption(settings, option, direction);
  };
  const cr::CreativeToolOptionAdjustReceipt surfaceClamp =
      adjust(cr::CreativeToolOptionId::PlacementDepth, -1);
  ok = expect(surfaceClamp.accepted && !surfaceClamp.changed &&
                  surfaceClamp.status ==
                      cr::CreativeToolOptionAdjustStatus::NoChange &&
                  settings.placementDepth ==
                      cr::CreativePlacementDepth::ZeroCells,
              "placement depth clamps at the hit surface") &&
       ok;
  ok = expect(adjust(cr::CreativeToolOptionId::MoveConstraint, 1).changed &&
                  settings.moveConstraint == cr::CreativeMoveConstraint::X,
              "move constraint cycles to X") &&
       expect(adjust(cr::CreativeToolOptionId::MoveConstraint, 1).changed &&
                  settings.moveConstraint == cr::CreativeMoveConstraint::Z,
              "move constraint cycles to Z") &&
       expect(adjust(cr::CreativeToolOptionId::MoveConstraint, 1).changed &&
                  settings.moveConstraint == cr::CreativeMoveConstraint::Free,
              "move constraint wraps") &&
       expect(adjust(cr::CreativeToolOptionId::RotationStep, -1).changed &&
                  settings.rotationStep == cr::CreativeRotationStep::Degrees90,
              "rotation cycles backward") &&
       expect(adjust(cr::CreativeToolOptionId::PlacementYaw, 1).changed &&
                  settings.placementYaw == cr::CreativePlacementYaw::Degrees90 &&
                  cr::creativePlacementYawRadians(settings.placementYaw) > 1.57 &&
                  cr::creativePlacementYawRadians(settings.placementYaw) < 1.58,
              "placement orientation advances by a quarter turn") &&
       expect(adjust(cr::CreativeToolOptionId::SnapIncrement, 1).changed &&
                  settings.snapIncrement == cr::CreativeSnapIncrement::TwoMeters,
              "grid increment cycles") &&
       expect(adjust(cr::CreativeToolOptionId::PlacementGridDots, 1).changed &&
                  settings.placementGridDots ==
                      cr::CreativePlacementGridDots::NearestLayer,
              "grid dots toggle to the nearest interaction layer") &&
       expect(adjust(cr::CreativeToolOptionId::PlacementPlane, 1).changed &&
                  settings.placementPlane == cr::CreativePlacementPlane::X,
              "placement plane cycles from automatic to X") &&
       expect(adjust(cr::CreativeToolOptionId::PlacementAnchor, 1).changed &&
                  settings.placementAnchor ==
                      cr::CreativePlacementAnchor::Face,
              "placement anchor cycles from center to face") &&
       expect(adjust(cr::CreativeToolOptionId::PlacementDepth, 1).changed &&
                  settings.placementDepth ==
                      cr::CreativePlacementDepth::OneCell &&
                  cr::creativePlacementDepthSteps(settings.placementDepth) == 1U,
              "placement depth advances one cell away") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushShape, 1).changed &&
                  settings.materialBrushShape ==
                      cr::CreativeMaterialBrushShape::Cylinder,
              "material brush shape cycles") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushAxis, 1).changed &&
                  settings.materialBrushAxis == cr::CreativeAxis3::Z,
              "material brush cylinder axis cycles") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushSize, 1).changed &&
                  settings.materialBrushSize ==
                      cr::CreativeMaterialBrushSize::FiveCells,
              "material brush size cycles within its fixed budget") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushFill, 1).changed &&
                  settings.materialBrushFill ==
                      cr::CreativeMaterialBrushFill::Shell,
              "material brush body cycles from solid to shell") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushGuide, 1).changed &&
                  settings.materialBrushGuide ==
                      cr::CreativeMaterialBrushGuide::LineX,
              "material brush guide cycles from free to line X") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushSymmetry, 1)
                  .changed &&
                  settings.materialBrushSymmetry ==
                      cr::CreativeMaterialBrushSymmetry::MirrorX,
              "material brush symmetry cycles from off to mirror X") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushMask, 1).changed &&
                  settings.materialBrushMask ==
                      cr::CreativeMaterialBrushMask::AddOnly,
              "material brush mask cycles from overwrite to add-only") &&
       expect(adjust(cr::CreativeToolOptionId::ConnectedFillLimit, 1).changed &&
                  settings.connectedFillLimit ==
                      cr::CreativeConnectedFillLimit::Cells512,
              "connected fill limit cycles within fixed storage") &&
       expect(adjust(cr::CreativeToolOptionId::SurfaceExtrudeDepth, 1).changed &&
                  settings.surfaceExtrudeDepth ==
                      cr::CreativeSurfaceExtrudeDepth::TwoCells,
              "surface extrusion depth cycles within fixed choices") &&
       expect(adjust(cr::CreativeToolOptionId::SurfaceExtrudeLimit, 1).changed &&
                  settings.surfaceExtrudeLimit ==
                      cr::CreativeConnectedFillLimit::Cells512,
              "surface affected-cell limit cycles within fixed storage") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptMode, 1).changed &&
                  settings.terrainSculptMode ==
                      cr::CreativeTerrainSculptMode::Smooth,
              "terrain sculpt mode cycles to smooth") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptMode, 1).changed &&
                  settings.terrainSculptMode ==
                      cr::CreativeTerrainSculptMode::Raise,
              "terrain sculpt mode wraps from smooth to raise") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptRadius, 1).changed &&
                  settings.terrainSculptRadius ==
                      cr::CreativeTerrainSculptRadius::EightCells,
              "terrain sculpt radius cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptStrength, 1)
                  .changed &&
                  settings.terrainSculptStrength ==
                      cr::CreativeTerrainSculptStrength::TwoCells,
              "terrain sculpt strength cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptTargetHeight, 1)
                  .changed &&
                  settings.terrainSculptTargetHeightCells == 5U,
              "terrain sculpt target height adjusts numerically") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptFalloff, 1)
                  .changed &&
                  settings.terrainSculptFalloff ==
                      cr::CreativeTerrainSculptFalloff::Linear,
              "terrain sculpt falloff cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSculptMask, 1).changed &&
                  settings.terrainSculptMask ==
                      cr::CreativeTerrainSculptMask::Square,
              "terrain sculpt mask cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRodStampMode, 1).changed &&
                  settings.terrainRodStampMode ==
                      cr::CreativeTerrainRodStampMode::Seed,
              "terrain rod stamp mode cycles to seed") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSeedRadius, 1).changed &&
                  settings.terrainSeedRadius ==
                      cr::CreativeTerrainSeedRadius::EightCells,
              "terrain seed radius cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainSeedSpacing, 1).changed &&
                  settings.terrainSeedSpacing ==
                      cr::CreativeTerrainSeedSpacing::FourCells,
              "terrain seed spacing cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileKind, 1).changed &&
                  settings.terrainProfileKind ==
                      cr::CreativeTerrainProfileKind::Basin,
              "terrain profile kind cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileBlend, 1).changed &&
                  settings.terrainProfileBlend ==
                      cr::CreativeTerrainProfileBlend::Add,
              "terrain profile blend cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileRodPolicy, 1)
                  .changed &&
                  settings.terrainProfileRodPolicy ==
                      cr::CreativeTerrainProfileRodPolicy::Existing,
              "terrain profile rod policy cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileRadius, 1).changed &&
                  settings.terrainProfileRadiusCells == 5U,
              "terrain profile radius adjusts one cell") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileAmplitude, 1)
                  .changed &&
                  settings.terrainProfileAmplitudeCells == 5U,
              "terrain profile amplitude adjusts one cell") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileSpacing, 1)
                  .changed &&
                  settings.terrainProfileSpacingCells == 2U,
              "terrain profile spacing adjusts one cell") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileDirection, 1)
                  .changed &&
                  settings.terrainProfileDirection ==
                      cr::CreativeTerrainProfileDirection::PositiveXPositiveZ,
              "terrain profile direction cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileFrequency, 1)
                  .changed &&
                  settings.terrainProfileFrequencyCycles == 2U,
              "terrain profile frequency adjusts one cycle") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainProfileSeed, 1).changed &&
                  settings.terrainProfileSeed == 1U,
              "terrain profile seed adjusts deterministically") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainPathKind, 1).changed &&
                  settings.terrainPathKind ==
                      cr::CreativeTerrainPathKind::River,
              "terrain path kind cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainPathElevation, 1)
                  .changed &&
                  settings.terrainPathElevation ==
                      cr::CreativeTerrainPathElevation::Level,
              "terrain path elevation cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainPathWidth, 1).changed &&
                  settings.terrainPathWidth ==
                      cr::CreativeTerrainPathWidth::FiveCells,
              "terrain path width cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainPathAmplitude, 1)
                  .changed &&
                  settings.terrainPathAmplitude ==
                      cr::CreativeTerrainPathAmplitude::TwoCells,
              "terrain path rise depth cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionOperation, 1)
                  .changed &&
                  settings.terrainRegionRecipe.mode ==
                      cr::CreativeTerrainRegionMode::Lower,
              "terrain region operation cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionMask, 1).changed &&
                  settings.terrainRegionRecipe.mask ==
                      cr::CreativeTerrainCompositionMask::Ellipse,
              "terrain region mask cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionAmount, 1).changed &&
                  settings.terrainRegionRecipe.amountCells == 2U,
              "terrain region amount steps exactly") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionTargetHeight, 1)
                  .changed &&
                  settings.terrainRegionRecipe.targetHeightCells == 5U,
              "terrain region target steps exactly") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionNoiseRelief, 1)
                  .changed &&
                  settings.terrainRegionRecipe.noiseReliefCells == 5U,
              "terrain region relief steps exactly") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionNoiseScale, 1)
                  .changed &&
                  settings.terrainRegionRecipe.noiseScaleCells == 13.0,
              "terrain region noise scale steps exactly") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionSeed, 1).changed &&
                  settings.terrainRegionRecipe.seed == 2U,
              "terrain region seed steps deterministically") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainRegionFeather, 1).changed &&
                  settings.terrainRegionRecipe.featherCells == 1U,
              "terrain region feather steps exactly") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainStampMode, 1).changed &&
                  settings.terrainStampMode ==
                      cr::CreativeTerrainStampMode::Replace,
              "terrain stamp mode cycles") &&
       expect(adjust(cr::CreativeToolOptionId::TerrainStampElevation, 1)
                  .changed &&
                  settings.terrainStampElevationMode ==
                      cr::CreativeTerrainStampElevationMode::Absolute,
              "terrain stamp elevation cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ShapeBrushKind, 1).changed &&
                  settings.shapeBrushKind == cr::CreativeShapeBrushKind::Line,
              "shape kind cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ShapeBrushAxis, 1).changed &&
                  settings.shapeBrushAxis == cr::CreativeShapeBrushAxis::Z,
              "shape axis cycles") &&
       expect(adjust(cr::CreativeToolOptionId::VolumeFillOverlapPolicy, 1)
                  .changed &&
                  settings.volumeFillOverlapPolicy ==
                      cr::CreativeVolumeFillOverlapPolicy::ReplaceExisting,
              "volume fill overlap policy cycles") &&
       expect(adjust(cr::CreativeToolOptionId::VolumeHollowThickness, 1)
                  .changed &&
                  settings.volumeHollowThickness ==
                      cr::CreativeVolumeHollowThickness::TwoCells,
              "volume hollow thickness cycles") &&
       expect(adjust(cr::CreativeToolOptionId::VolumeHollowAlignment, 1)
                  .changed &&
                  settings.volumeHollowAlignment ==
                      cr::CreativeVolumeHollowAlignment::Outward,
              "volume hollow alignment cycles") &&
       expect(adjust(cr::CreativeToolOptionId::VolumeHollowOpening, 1)
                  .changed &&
                  settings.volumeHollowOpening ==
                      cr::CreativeVolumeHollowOpening::NegativeEnd,
              "volume hollow opening cycles") &&
       expect(adjust(cr::CreativeToolOptionId::VolumeHollowCornerRule, 1)
                  .changed &&
                  settings.volumeHollowCornerRule ==
                      cr::CreativeVolumeHollowCornerRule::CutThrough,
              "volume hollow corner rule cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneOffsetAxis, 1).changed &&
                  settings.cloneOffsetAxis == cr::CreativeCloneOffsetAxis::Y,
              "clone axis cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneOffsetDistance, 1).changed &&
                  settings.cloneOffsetDistance ==
                      cr::CreativeCloneOffsetDistance::TwoCells,
              "clone distance cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneRotation, 1).changed &&
                  settings.cloneRotation == cr::CreativeCloneRotation::Degrees90,
              "clone rotation cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneMirror, 1).changed &&
                  settings.cloneMirror == cr::CreativeCloneMirror::X,
              "clone mirror cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneMemberMask, 1).changed &&
                  settings.volumeCloneMemberMask ==
                      cr::CreativeVolumeMemberMask::VoxelCells,
              "clone member mask cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneVoxelOverlapPolicy, 1)
                  .changed &&
                  settings.cloneVoxelOverlapPolicy ==
                      cr::CreativeVolumeCloneVoxelOverlapPolicy::PreserveExisting,
              "clone voxel overlap cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ArrayDirection, 1).changed &&
                  settings.arrayDirection ==
                      cr::CreativeLinearArrayDirection::NegativeX,
              "array direction cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ArrayCopyCount, 1).changed &&
                  settings.arrayCopyCount ==
                      cr::CreativeLinearArrayCopyCount::Eight,
              "array copy count cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ArraySpacing, 1).changed &&
                  settings.arraySpacing ==
                      cr::CreativeLinearArraySpacing::TwoCells,
              "array spacing cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ArrayMode, 1).changed &&
                  settings.arrayMode == cr::CreativeArrayMode::Radial,
              "array mode cycles to radial") &&
       expect(adjust(cr::CreativeToolOptionId::RadialArrayAxis, 1).changed &&
                  settings.radialArrayAxis == cr::CreativeAxis3::Z,
              "radial axis cycles") &&
       expect(adjust(cr::CreativeToolOptionId::RadialArrayInstanceCount, 1)
                  .changed &&
                  settings.radialArrayInstanceCount ==
                      cr::CreativeRadialArrayInstanceCount::Sixteen,
              "radial instance count cycles") &&
       expect(adjust(cr::CreativeToolOptionId::RadialArraySweep, 1).changed &&
                  settings.radialArraySweep ==
                      cr::CreativeRadialArraySweep::Degrees90,
              "radial sweep wraps") &&
       ok;

  settings.placementDepth = cr::CreativePlacementDepth::EightCells;
  const cr::CreativeToolOptionAdjustReceipt depthMaximum =
      adjust(cr::CreativeToolOptionId::PlacementDepth, 1);
  const cr::CreativeToolOptionAdjustReceipt depthBack =
      adjust(cr::CreativeToolOptionId::PlacementDepth, -1);
  ok = expect(depthMaximum.accepted && !depthMaximum.changed &&
                  depthMaximum.status ==
                      cr::CreativeToolOptionAdjustStatus::NoChange &&
                  depthBack.changed &&
                  settings.placementDepth ==
                      cr::CreativePlacementDepth::SevenCells,
              "placement depth clamps far away and can move back") &&
       ok;

  const cr::CreativeToolSettings beforeZero = settings;
  const cr::CreativeToolOptionAdjustReceipt zero = adjust(
      cr::CreativeToolOptionId::MoveConstraint, 0);
  const cr::CreativeToolOptionAdjustReceipt invalid = adjust(
      cr::CreativeToolOptionId::Count, 1);
  settings.moveConstraint = cr::CreativeMoveConstraint::Count;
  const cr::CreativeToolSettings beforeInvalid = settings;
  const cr::CreativeToolOptionAdjustReceipt invalidSettings = adjust(
      cr::CreativeToolOptionId::RotationStep, 1);
  return expect(zero.accepted && !zero.changed &&
                    zero.status == cr::CreativeToolOptionAdjustStatus::NoChange &&
                    beforeZero.moveConstraint ==
                        cr::CreativeMoveConstraint::Free,
                "zero direction is accepted no-change") &&
         expect(!invalid.accepted && !invalid.changed &&
                    invalid.status ==
                        cr::CreativeToolOptionAdjustStatus::InvalidOption,
                "invalid option rejected") &&
         expect(!invalidSettings.accepted && !invalidSettings.changed &&
                    invalidSettings.status ==
                        cr::CreativeToolOptionAdjustStatus::InvalidSettings &&
                    settings.moveConstraint == beforeInvalid.moveConstraint,
                "invalid settings are not normalized") &&
         ok;
}

bool replaceFilterAndCloneOffsetUseExplicitInputs() {
  cr::CreativeToolSettings settings = cr::makeDefaultCreativeToolSettings();
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  const auto adjustSource = [&settings, &palette](std::int32_t direction) {
    return cr::adjustCreativeToolOption(
        settings, cr::CreativeToolOptionId::ReplaceSource, direction, palette);
  };
  const auto adjustBrushSource = [&settings, &palette](
                                     std::int32_t direction) {
    return cr::adjustCreativeToolOption(
        settings, cr::CreativeToolOptionId::MaterialBrushReplaceSource,
        direction, palette);
  };
  const auto adjustMemberMask = [&settings, &palette](
                                    std::int32_t direction) {
    return cr::adjustCreativeToolOption(
        settings, cr::CreativeToolOptionId::ReplaceMemberMask, direction,
        palette);
  };
  const auto adjustEraseSource = [&settings, &palette](
                                     std::int32_t direction) {
    return cr::adjustCreativeToolOption(
        settings, cr::CreativeToolOptionId::EraseSource, direction, palette);
  };
  const auto adjustEraseMemberMask = [&settings, &palette](
                                         std::int32_t direction) {
    return cr::adjustCreativeToolOption(
        settings, cr::CreativeToolOptionId::EraseMemberMask, direction,
        palette);
  };

  bool ok = expect(adjustSource(1).changed &&
                       settings.replaceSourceKind ==
                           cr::CreativeObjectKind::Wall,
                   "replace source advances from Any to first material") &&
            expect(adjustSource(1).changed &&
                       settings.replaceSourceKind ==
                           cr::CreativeObjectKind::Crate,
                   "replace source advances through palette") &&
            expect(adjustSource(1).changed &&
                       settings.replaceSourceKind ==
                           cr::CreativeObjectKind::Unknown,
                   "replace source wraps to Any") &&
            expect(adjustSource(-1).changed &&
                       settings.replaceSourceKind ==
                           cr::CreativeObjectKind::Crate,
                   "replace source cycles backward") &&
            expect(adjustBrushSource(1).changed &&
                       settings.materialBrushReplaceSourceKind ==
                           cr::CreativeObjectKind::Wall,
                   "material brush source cycles independently") &&
            expect(adjustBrushSource(1).changed &&
                       settings.materialBrushReplaceSourceKind ==
                           cr::CreativeObjectKind::Unknown,
                   "material brush source skips non-voxel palette entries") &&
            expect(adjustMemberMask(1).changed &&
                       settings.volumeReplaceMemberMask ==
                           cr::CreativeVolumeMemberMask::VoxelCells &&
                       cr::creativeVolumeMemberMaskIncludesVoxels(
                           settings.volumeReplaceMemberMask) &&
                       !cr::creativeVolumeMemberMaskIncludesObjects(
                           settings.volumeReplaceMemberMask),
                   "replace members wrap from both to voxels") &&
            expect(adjustMemberMask(1).changed &&
                       settings.volumeReplaceMemberMask ==
                           cr::CreativeVolumeMemberMask::DocumentObjects &&
                       !cr::creativeVolumeMemberMaskIncludesVoxels(
                           settings.volumeReplaceMemberMask) &&
                       cr::creativeVolumeMemberMaskIncludesObjects(
                           settings.volumeReplaceMemberMask),
                   "replace members advance to document objects") &&
            expect(adjustMemberMask(1).changed &&
                       settings.volumeReplaceMemberMask ==
                           cr::CreativeVolumeMemberMask::Both &&
                       cr::creativeVolumeMemberMaskIncludesVoxels(
                           settings.volumeReplaceMemberMask) &&
                       cr::creativeVolumeMemberMaskIncludesObjects(
                           settings.volumeReplaceMemberMask),
                   "replace members advance to both domains") &&
            expect(adjustEraseSource(1).changed &&
                       settings.eraseSourceKind ==
                           cr::CreativeObjectKind::Wall,
                   "erase source cycles independently") &&
            expect(adjustEraseMemberMask(1).changed &&
                       settings.volumeEraseMemberMask ==
                           cr::CreativeVolumeMemberMask::VoxelCells,
                   "erase member mask cycles independently") &&
            expect(cr::creativeMaterialBrushPaintAllows(
                       cr::CreativeMaterialBrushMask::Replace,
                       cr::CreativeObjectKind::Wall,
                       cr::CreativeObjectKind::Wall) &&
                       !cr::creativeMaterialBrushPaintAllows(
                           cr::CreativeMaterialBrushMask::Replace,
                           cr::CreativeObjectKind::Floor,
                           cr::CreativeObjectKind::Wall) &&
                       cr::creativeMaterialBrushPaintAllows(
                           cr::CreativeMaterialBrushMask::Replace,
                           cr::CreativeObjectKind::Floor,
                           cr::CreativeObjectKind::Unknown),
                   "material-aware replace admits exact source or Any") &&
            expect(cr::creativeMaterialBrushPaintAllows(
                       cr::CreativeMaterialBrushMask::AddOnly,
                       cr::CreativeObjectKind::Unknown,
                       cr::CreativeObjectKind::Wall) &&
                       cr::creativeMaterialBrushPaintAllows(
                           cr::CreativeMaterialBrushMask::Overwrite,
                           cr::CreativeObjectKind::Floor,
                           cr::CreativeObjectKind::Wall) &&
                       !cr::creativeMaterialBrushPaintAllows(
                           cr::CreativeMaterialBrushMask::Count,
                           cr::CreativeObjectKind::Floor,
                           cr::CreativeObjectKind::Wall),
                   "non-replace masks ignore source and invalid mask closes");

  settings.cloneOffsetAxis = cr::CreativeCloneOffsetAxis::Z;
  settings.cloneOffsetDistance =
      cr::CreativeCloneOffsetDistance::FourCells;
  cr::CreativeToolWorldPoint offset;
  ok = expect(cr::tryCreativeCloneOffset(settings, 0.5, offset) &&
                  offset.x == 0.0 && offset.y == 0.0 && offset.z == 2.0,
              "clone offset combines axis, distance, and cell size") &&
       expect(!cr::tryCreativeCloneOffset(settings, 0.0, offset) &&
                  offset.x == 0.0 && offset.y == 0.0 && offset.z == 0.0,
              "invalid clone cell size fails closed") &&
       ok;

  settings.cloneOffsetAxis = cr::CreativeCloneOffsetAxis::NegativeX;
  settings.cloneOffsetDistance = cr::CreativeCloneOffsetDistance::TwoCells;
  ok = expect(cr::tryCreativeCloneOffset(settings, 0.5, offset) &&
                  offset.x == -1.0 && offset.y == 0.0 && offset.z == 0.0,
              "clone offset supports negative directions") &&
       ok;

  cr::CreativeToolSettings noPalette = cr::makeDefaultCreativeToolSettings();
  const cr::CreativeToolOptionAdjustReceipt unavailable =
      cr::adjustCreativeToolOption(
          noPalette, cr::CreativeToolOptionId::ReplaceSource, 1);
  const cr::CreativeToolOptionAdjustReceipt brushUnavailable =
      cr::adjustCreativeToolOption(
          noPalette, cr::CreativeToolOptionId::MaterialBrushReplaceSource, 1);
  const cr::CreativeToolOptionAdjustReceipt eraseUnavailable =
      cr::adjustCreativeToolOption(
          noPalette, cr::CreativeToolOptionId::EraseSource, 1);
  return expect(!unavailable.accepted && !unavailable.changed &&
                    unavailable.status ==
                        cr::CreativeToolOptionAdjustStatus::NoAvailableValue &&
                    noPalette.replaceSourceKind ==
                        cr::CreativeObjectKind::Unknown,
                "missing material palette leaves filter unchanged") &&
         expect(!brushUnavailable.accepted && !brushUnavailable.changed &&
                    noPalette.materialBrushReplaceSourceKind ==
                        cr::CreativeObjectKind::Unknown,
                "missing palette also leaves brush filter unchanged") &&
         expect(!eraseUnavailable.accepted && !eraseUnavailable.changed &&
                    noPalette.eraseSourceKind ==
                        cr::CreativeObjectKind::Unknown,
                "missing palette leaves erase filter unchanged") &&
         ok;
}

bool transformCommandsStoreRadiansAndResolveLiveGeometry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Transform");
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Wall;
  create.name = "Rotated Wall";
  create.transform.position = {0.0, 1.0, 0.0};
  create.hasTransformOverride = true;
  create.bounds = {{-2.0, 0.0, -0.125}, {2.0, 2.0, 0.125}};
  create.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt created = document.createObject(create);
  const std::array objectIds{created.objectId};

  cr::CreativeTransformCommandRequest rotate;
  rotate.kind = cr::CreativeTransformCommandKind::RotateYaw;
  rotate.yawDegrees = 90.0;
  const cr::CreativeTransformCommandReceipt rotated =
      cr::transformDocumentObjectsAtomically(document, objectIds, rotate);
  const cr::CreativeObject* object = document.findObject(created.objectId);
  const cr::CreativeTransformedBounds rotatedBounds =
      object != nullptr ? cr::resolveCreativeObjectBounds(*object)
                        : cr::CreativeTransformedBounds{};

  const auto near = [](double actual, double expected) {
    return std::fabs(actual - expected) <= 1.0e-9;
  };
  bool ok = expect(created.accepted && rotated.accepted && rotated.changed &&
                       object != nullptr,
                   "yaw transform applies to a bounds-backed object") &&
            expect(object != nullptr &&
                       near(object->transform.rotationEulerRadians.y,
                            std::numbers::pi * 0.5),
                   "degree command stores radians exactly once") &&
            expect(rotatedBounds.valid &&
                       near(rotatedBounds.worldBounds.max.x -
                                rotatedBounds.worldBounds.min.x,
                            0.25) &&
                       near(rotatedBounds.worldBounds.max.z -
                                rotatedBounds.worldBounds.min.z,
                            4.0),
                   "resolved world bounds honor stored yaw");

  cr::CreativeTransformCommandRequest scale;
  scale.kind = cr::CreativeTransformCommandKind::Scale;
  scale.scaleFactor = {2.0, 1.0, 0.5};
  const cr::CreativeTransformCommandReceipt scaled =
      cr::transformDocumentObjectsAtomically(document, objectIds, scale);
  object = document.findObject(created.objectId);
  const cr::CreativeTransformedBounds scaledBounds =
      object != nullptr ? cr::resolveCreativeObjectBounds(*object)
                        : cr::CreativeTransformedBounds{};
  return expect(scaled.accepted && scaled.changed && scaledBounds.valid,
                "scale transform applies after rotation") &&
         expect(near(scaledBounds.worldBounds.max.x -
                         scaledBounds.worldBounds.min.x,
                     0.125) &&
                    near(scaledBounds.worldBounds.max.z -
                         scaledBounds.worldBounds.min.z,
                     8.0),
                "resolved world geometry applies scale before rotation") &&
         ok;
}

bool immediateTransformUsesSelectionPlacementKernel() {
  cr::CreativeDocument commandDocument =
      cr::CreativeDocument::create("Immediate Transform");
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "First";
  create.transform.position = {2.0, 0.0, 0.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt first =
      commandDocument.createObject(create);
  create.name = "Second";
  create.transform.position = {0.0, 0.0, 4.0};
  const cr::CreativeDocumentCreateReceipt second =
      commandDocument.createObject(create);
  cr::CreativeDocument placementDocument = commandDocument;
  const std::array objectIds{first.objectId, second.objectId};
  const cr::CreativeVec3 pivot{1.0, 0.0, 2.0};

  cr::CreativeTransformCommandRequest command;
  command.kind = cr::CreativeTransformCommandKind::RotateYaw;
  command.yawDegrees = 45.0;
  const cr::CreativeTransformCommandReceipt commandReceipt =
      cr::transformDocumentObjectsAtomically(commandDocument, objectIds,
                                             command);

  cr::CreativeSelectionPlacementRequest placement;
  placement.mode = cr::CreativeSelectionPlacementMode::Move;
  placement.sourceAnchor = pivot;
  placement.targetAnchor = pivot;
  placement.hasAxisAngleRotation = true;
  placement.rotationAxis = cr::CreativeAxis3::Y;
  placement.rotationRadians = std::numbers::pi * 0.25;
  const cr::CreativeSelectionPlacementReceipt placementReceipt =
      cr::placeDocumentObjectsAtomically(placementDocument, objectIds,
                                         placement);

  bool objectsMatch = true;
  for (cr::CreativeObjectId objectId : objectIds) {
    const cr::CreativeObject* commanded = commandDocument.findObject(objectId);
    const cr::CreativeObject* placed = placementDocument.findObject(objectId);
    objectsMatch = objectsMatch && commanded != nullptr && placed != nullptr &&
                   cr::creativeVec3ExactlyEqual(
                       commanded->transform.position,
                       placed->transform.position) &&
                   cr::creativeVec3ExactlyEqual(
                       commanded->transform.rotationEulerRadians,
                       placed->transform.rotationEulerRadians) &&
                   cr::creativeVec3ExactlyEqual(commanded->transform.scale,
                                                placed->transform.scale);
  }
  return expect(commandReceipt.accepted && commandReceipt.changed &&
                    placementReceipt.accepted && placementReceipt.changed,
                "immediate and previewable transform paths both apply") &&
         expect(cr::creativeVec3ExactlyEqual(commandReceipt.pivot, pivot),
                "immediate transform preserves the root-average pivot") &&
         expect(objectsMatch,
                "immediate transform delegates exact geometry to placement");
}

bool resetTransformPreservesPositionAndAppliesAtomically() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Reset Transform");
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Wall;
  create.name = "Transformed Wall";
  create.transform.position = {3.0, 2.0, -4.0};
  create.transform.rotationEulerRadians = {0.25, 0.5, -0.75};
  create.transform.scale = {2.0, 0.5, 3.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt created = document.createObject(create);
  const std::array objectIds{created.objectId};

  cr::CreativeTransformCommandRequest reset;
  reset.kind = cr::CreativeTransformCommandKind::ResetRotationScale;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeTransformCommandReceipt receipt =
      cr::transformDocumentObjectsAtomically(document, objectIds, reset);
  const cr::CreativeObject* object = document.findObject(created.objectId);
  bool ok = expect(receipt.accepted && receipt.changed && object != nullptr,
                   "reset transform applies") &&
            expect(object != nullptr &&
                       cr::creativeVec3ExactlyEqual(
                           object->transform.position, {3.0, 2.0, -4.0}) &&
                       cr::creativeVec3ExactlyEqual(
                           object->transform.rotationEulerRadians,
                           {0.0, 0.0, 0.0}) &&
                       cr::creativeVec3ExactlyEqual(
                           object->transform.scale, {1.0, 1.0, 1.0}),
                   "reset preserves position and restores rotation and scale") &&
            expect(document.revision() == revisionBefore + 1U &&
                       receipt.mutationReceipt.attemptedCount == 2U,
                   "reset commits both fields in one document revision");

  const std::uint64_t settledRevision = document.revision();
  const cr::CreativeTransformCommandReceipt noChange =
      cr::transformDocumentObjectsAtomically(document, objectIds, reset);
  ok = expect(noChange.accepted && !noChange.changed &&
                  document.revision() == settledRevision,
              "repeated reset is an accepted no-op") &&
       ok;

  cr::CreativeDocument unsupported =
      cr::CreativeDocument::create("Unsupported Reset");
  create = {};
  create.kind = cr::CreativeObjectKind::SoundEmitter;
  const cr::CreativeDocumentCreateReceipt sound = unsupported.createObject(create);
  const std::array unsupportedIds{sound.objectId};
  const std::uint64_t unsupportedRevision = unsupported.revision();
  const cr::CreativeTransformCommandReceipt rejected =
      cr::transformDocumentObjectsAtomically(unsupported, unsupportedIds, reset);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeTransformCommandStatus::UnsupportedObject &&
                    unsupported.revision() == unsupportedRevision,
                "reset rejects objects without rotate and scale capability") &&
         ok;
}

bool selectionPlacementPlanOwnsPreviewAndCommitGeometry() {
  cr::CreativeObject wall;
  wall.id = 1U;
  wall.kind = cr::CreativeObjectKind::Wall;
  wall.name = "Wall";
  wall.transform.position = {1.0, 0.5, 2.0};
  wall.bounds = {{0.5, 0.0, 1.5}, {1.5, 1.0, 2.5}};

  cr::CreativeObject route;
  route.id = 2U;
  route.kind = cr::CreativeObjectKind::PatrolRoute;
  route.name = "Route";
  route.pathPoints = {{{0.0, 0.0, 1.0}}, {{0.0, 0.0, 3.0}}};
  const std::array sources{wall, route};

  cr::CreativeSelectionPlacementRequest request;
  request.mode = cr::CreativeSelectionPlacementMode::Copy;
  request.targetAnchor = {10.0, 0.0, 20.0};
  request.quarterTurns = 1U;
  request.mirrorX = true;
  const cr::CreativeSelectionPlacementPlan plan =
      cr::planCreativeSelectionPlacement(sources, request);
  const auto near = [](double actual, double expected) {
    return std::fabs(actual - expected) <= 1.0e-9;
  };

  return expect(plan.accepted &&
                    plan.status == cr::CreativeSelectionPlacementStatus::Planned &&
                    plan.objects.size() == 2U,
                "selection placement plan accepted") &&
         expect(near(plan.objects[0].transform.position.x, 12.0) &&
                    near(plan.objects[0].transform.position.z, 21.0) &&
                    near(plan.objects[0].transform.rotationEulerRadians.y,
                         std::numbers::pi * 0.5),
                "quarter turn and mirror transform object pivot and yaw") &&
         expect(near(plan.objects[1].pathPoints[0].position.x, 11.0) &&
                    near(plan.objects[1].pathPoints[0].position.z, 20.0) &&
                    near(plan.objects[1].pathPoints[1].position.x, 13.0),
                "path points use the shared selection transform") &&
         expect(wall.transform.position.x == 1.0 &&
                    route.pathPoints[0].position.z == 1.0,
                "planning leaves source objects unchanged") &&
         expect(plan.hasAggregateBounds &&
                    plan.aggregateBounds.max.x >= 13.0,
                "plan publishes aggregate preview bounds");
}

bool selectionPlacementPrecisionIsExactAndFailClosed() {
  const auto near = [](double actual, double expected) {
    return std::fabs(actual - expected) <= 1.0e-12;
  };
  cr::CreativeSelectionPlacementTargetRequest target;
  target.sourceAnchor = {0.1, -2.0, 0.3};
  target.aimedAnchor = {-1.16, 3.12, -4.74};
  target.nudgeOffset = {0.25, 8.0, -6.0};
  target.axis = cr::CreativeSelectionPlacementAxis::X;
  target.snapStepMeters = 0.5;
  const cr::CreativeSelectionPlacementTargetResult constrained =
      cr::resolveCreativeSelectionPlacementTarget(target);

  bool ok = expect(constrained.accepted &&
                       constrained.status ==
                           cr::CreativeSelectionPlacementTargetStatus::Resolved,
                   "precision target resolves") &&
            expect(near(constrained.displacement.x, -1.25) &&
                       near(constrained.displacement.y, 0.0) &&
                       near(constrained.displacement.z, 0.0) &&
                       near(constrained.targetAnchor.x, -1.15) &&
                       near(constrained.targetAnchor.y, -2.0) &&
                       near(constrained.targetAnchor.z, 0.3),
                   "negative constrained displacement snaps from source anchor");

  target.axis = cr::CreativeSelectionPlacementAxis::Free;
  const cr::CreativeSelectionPlacementTargetResult free =
      cr::resolveCreativeSelectionPlacementTarget(target);
  ok = expect(free.accepted && near(free.targetAnchor.x, -0.91) &&
                  near(free.targetAnchor.y, 11.12) &&
                  near(free.targetAnchor.z, -10.74),
              "free target preserves aim and accumulated axis offsets") &&
       ok;

  cr::CreativeSelectionPlacementNudgeRequest nudge;
  nudge.axis = cr::CreativeSelectionPlacementAxis::X;
  nudge.snapStepMeters = 1.0;
  nudge.steps = -2;
  nudge.fine = true;
  const cr::CreativeSelectionPlacementNudgeReceipt fine =
      cr::nudgeCreativeSelectionPlacementOffset(nudge);
  ok = expect(fine.accepted && fine.changed &&
                  fine.status ==
                      cr::CreativeSelectionPlacementNudgeStatus::Applied &&
                  near(fine.appliedStepMeters, 0.25) &&
                  near(fine.offset.x, -0.5),
              "fine nudge applies quarter-step exactly") &&
       ok;

  nudge.offset = fine.offset;
  nudge.axis = cr::CreativeSelectionPlacementAxis::Y;
  nudge.snapStepMeters = 0.5;
  nudge.steps = 3;
  nudge.fine = false;
  const cr::CreativeSelectionPlacementNudgeReceipt vertical =
      cr::nudgeCreativeSelectionPlacementOffset(nudge);
  ok = expect(vertical.accepted && near(vertical.offset.x, -0.5) &&
                  near(vertical.offset.y, 1.5),
              "nudge retains other axis offsets") &&
       ok;

  nudge.axis = cr::CreativeSelectionPlacementAxis::Free;
  const cr::CreativeSelectionPlacementNudgeReceipt axisRequired =
      cr::nudgeCreativeSelectionPlacementOffset(nudge);
  ok = expect(!axisRequired.accepted && !axisRequired.changed &&
                  axisRequired.status ==
                      cr::CreativeSelectionPlacementNudgeStatus::AxisRequired &&
                  near(axisRequired.offset.x, nudge.offset.x),
              "free-axis nudge fails without changing offset") &&
       ok;

  cr::CreativeSelectionPlacementTargetRequest localTarget;
  localTarget.aimedAnchor = {2.0, 0.0, -3.0};
  localTarget.axis = cr::CreativeSelectionPlacementAxis::X;
  localTarget.coordinateSpace =
      cr::CreativeSelectionPlacementCoordinateSpace::Local;
  localTarget.coordinateBasisEulerRadians =
      {0.0, std::numbers::pi * 0.5, 0.0};
  localTarget.snapStepMeters = 0.5;
  const cr::CreativeSelectionPlacementTargetResult localConstrained =
      cr::resolveCreativeSelectionPlacementTarget(localTarget);
  cr::CreativeSelectionPlacementNudgeRequest localNudge;
  localNudge.axis = cr::CreativeSelectionPlacementAxis::X;
  localNudge.coordinateSpace = localTarget.coordinateSpace;
  localNudge.coordinateBasisEulerRadians =
      localTarget.coordinateBasisEulerRadians;
  localNudge.snapStepMeters = 0.5;
  localNudge.steps = 2;
  const cr::CreativeSelectionPlacementNudgeReceipt localNudged =
      cr::nudgeCreativeSelectionPlacementOffset(localNudge);
  localTarget.nudgeOffset = localNudged.offset;
  const cr::CreativeSelectionPlacementTargetResult localWithNudge =
      cr::resolveCreativeSelectionPlacementTarget(localTarget);
  ok = expect(localConstrained.accepted &&
                  near(localConstrained.targetAnchor.x, 0.0) &&
                  near(localConstrained.targetAnchor.y, 0.0) &&
                  near(localConstrained.targetAnchor.z, -3.0),
              "local X constraint projects onto the frozen rotated basis") &&
       expect(localNudged.accepted &&
                  near(localNudged.offset.x, 0.0) &&
                  near(localNudged.offset.z, -1.0) &&
                  localWithNudge.accepted &&
                  near(localWithNudge.targetAnchor.z, -4.0),
              "local nudge advances on the same basis used by targeting") &&
       ok;

  target.snapStepMeters = std::numeric_limits<double>::infinity();
  const cr::CreativeSelectionPlacementTargetResult invalidTarget =
      cr::resolveCreativeSelectionPlacementTarget(target);
  target.snapStepMeters = 1.0;
  target.axis = static_cast<cr::CreativeSelectionPlacementAxis>(255U);
  const cr::CreativeSelectionPlacementTargetResult invalidAxis =
      cr::resolveCreativeSelectionPlacementTarget(target);
  nudge.axis = cr::CreativeSelectionPlacementAxis::Z;
  nudge.snapStepMeters = std::numeric_limits<double>::max();
  nudge.steps = 2;
  const cr::CreativeSelectionPlacementNudgeReceipt overflow =
      cr::nudgeCreativeSelectionPlacementOffset(nudge);
  localTarget.coordinateSpace =
      static_cast<cr::CreativeSelectionPlacementCoordinateSpace>(255U);
  const cr::CreativeSelectionPlacementTargetResult invalidSpace =
      cr::resolveCreativeSelectionPlacementTarget(localTarget);
  return expect(!invalidTarget.accepted && !invalidAxis.accepted &&
                    !invalidSpace.accepted && !overflow.accepted &&
                    !overflow.changed,
                "invalid target, axis, and overflowing nudge fail closed") &&
         ok;
}

bool selectionPlacementScalePlanMatchesAtomicCommit() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Scale Plan");
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Left";
  create.transform.position = {0.0, 0.5, 0.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt left = document.createObject(create);
  create.name = "Right";
  create.transform.position = {2.0, 0.5, 0.0};
  const cr::CreativeDocumentCreateReceipt right = document.createObject(create);
  const std::array ids{left.objectId, right.objectId};

  cr::CreativeSelectionPlacementRequest request;
  request.mode = cr::CreativeSelectionPlacementMode::Move;
  request.sourceAnchor = {1.0, 0.5, 0.0};
  request.targetAnchor = request.sourceAnchor;
  request.scaleFactor = {1.5, 2.0, 0.5};
  const cr::CreativeSelectionPlacementPlan plan =
      cr::planCreativeSelectionPlacement(document.objects(), request);
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeSelectionPlacementReceipt applied =
      cr::placeDocumentObjectsAtomically(document, ids, request);
  const cr::CreativeObject* scaledLeft = document.findObject(left.objectId);
  const cr::CreativeObject* scaledRight = document.findObject(right.objectId);

  bool ok = expect(plan.accepted && plan.objects.size() == 2U &&
                       applied.accepted && applied.changed,
                   "three-axis scale plans and commits") &&
            expect(scaledLeft != nullptr && scaledRight != nullptr &&
                       cr::creativeVec3ExactlyEqual(
                           scaledLeft->transform.position,
                           plan.objects[0].transform.position) &&
                       cr::creativeVec3ExactlyEqual(
                           scaledRight->transform.position,
                           plan.objects[1].transform.position) &&
                       cr::creativeVec3ExactlyEqual(
                           scaledLeft->transform.scale,
                           plan.objects[0].transform.scale) &&
                       scaledLeft->transform.position.x == -0.5 &&
                       scaledRight->transform.position.x == 2.5 &&
                       cr::creativeVec3ExactlyEqual(
                           scaledLeft->transform.scale, {1.5, 2.0, 0.5}),
                   "commit publishes the exact planned pivot-relative scale") &&
            expect(document.revision() == revisionBefore + 1U,
                   "scaled batch advances document revision once");

  cr::CreativeObject route;
  route.id = 99U;
  route.kind = cr::CreativeObjectKind::PatrolRoute;
  route.pathPoints = {{{0.0, 0.0, 2.0}}, {{2.0, 1.0, 4.0}}};
  const std::array routeSource{route};
  const cr::CreativeSelectionPlacementPlan routePlan =
      cr::planCreativeSelectionPlacement(routeSource, request);
  ok = expect(routePlan.accepted &&
                  routePlan.objects[0].pathPoints[0].position.x == -0.5 &&
                  routePlan.objects[0].pathPoints[0].position.y == -0.5 &&
                  routePlan.objects[0].pathPoints[0].position.z == 1.0 &&
                  routePlan.objects[0].pathPoints[1].position.x == 2.5 &&
                  routePlan.objects[0].pathPoints[1].position.y == 1.5 &&
                  routePlan.objects[0].pathPoints[1].position.z == 2.0,
              "path points use the same three-axis pivot scale") &&
       ok;
  cr::CreativeSelectionPlacementRequest scaledCopy = request;
  scaledCopy.mode = cr::CreativeSelectionPlacementMode::Copy;
  const cr::CreativeSelectionPlacementPlan copyScale =
      cr::planCreativeSelectionPlacement(routeSource, scaledCopy);
  ok = expect(copyScale.accepted && copyScale.objects.size() == 1U &&
                  copyScale.objects[0].pathPoints[0].position.x == -0.5 &&
                  copyScale.objects[0].pathPoints[1].position.z == 2.0,
              "scaled Copy uses the same placement transform as Move") &&
       ok;

  cr::CreativeDocument invalidDocument = document;
  request.scaleFactor = {0.0, 1.0, 1.0};
  const std::uint64_t invalidRevision = invalidDocument.revision();
  const cr::CreativeSelectionPlacementReceipt rejected =
      cr::placeDocumentObjectsAtomically(invalidDocument, ids, request);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeSelectionPlacementStatus::InvalidRequest &&
                    invalidDocument.revision() == invalidRevision,
                "non-positive scale fails before mutation") &&
         ok;
}

bool selectionPlacementPivotModesSharePreviewAndCommitGeometry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Pivot Modes");
  static_cast<void>(document.assignId(901U));
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Left";
  create.transform.position = {0.0, 0.5, 0.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt left = document.createObject(create);
  create.name = "Right";
  create.transform.position = {4.0, 0.5, 0.0};
  const cr::CreativeDocumentCreateReceipt right = document.createObject(create);
  const std::array ids{left.objectId, right.objectId};

  cr::CreativeSelectionPlacementRequest individual;
  individual.mode = cr::CreativeSelectionPlacementMode::Copy;
  individual.sourceAnchor = {2.0, 0.5, 0.0};
  individual.targetAnchor = {12.0, 0.5, 0.0};
  individual.pivotMode =
      cr::CreativeSelectionPlacementPivotMode::IndividualOrigins;
  individual.coordinateSpace =
      cr::CreativeSelectionPlacementCoordinateSpace::Local;
  individual.coordinateBasisEulerRadians = {0.0, 0.0,
                                             std::numbers::pi * 0.5};
  individual.scaleFactor = {2.0, 1.0, 0.5};
  individual.hasAxisAngleRotation = true;
  individual.rotationAxis = cr::CreativeAxis3::Y;
  individual.rotationRadians = std::numbers::pi * 0.5;

  const cr::CreativeSelectionPlacementPlan individualPlan =
      cr::planCreativeSelectionPlacement(document.objects(), individual);
  cr::CreativeSelectionPlacementRequest shared = individual;
  shared.pivotMode = cr::CreativeSelectionPlacementPivotMode::SharedAnchor;
  const cr::CreativeSelectionPlacementPlan sharedPlan =
      cr::planCreativeSelectionPlacement(document.objects(), shared);

  cr::CreativeVec3 leftOrigin{};
  cr::CreativeObject boundsOnly;
  boundsOnly.id = 99U;
  boundsOnly.kind = cr::CreativeObjectKind::Room;
  boundsOnly.bounds = {{2.0, 1.0, 4.0}, {6.0, 5.0, 10.0}};
  cr::CreativeVec3 boundsOrigin{};
  bool ok = expect(individualPlan.accepted && sharedPlan.accepted &&
                       individualPlan.objects.size() == 2U &&
                       sharedPlan.objects.size() == 2U,
                   "shared and individual pivot plans are accepted") &&
            expect(cr::creativeVec3ExactlyEqual(
                       individualPlan.objects[0].transform.position,
                       {10.0, 0.5, 0.0}) &&
                       cr::creativeVec3ExactlyEqual(
                           individualPlan.objects[1].transform.position,
                           {14.0, 0.5, 0.0}) &&
                       !cr::creativeVec3ExactlyEqual(
                           sharedPlan.objects[0].transform.position,
                           individualPlan.objects[0].transform.position),
                   "individual origins preserve spacing while shared pivot "
                   "rotates and scales the assembly") &&
            expect(cr::resolveCreativeSelectionPlacementObjectOrigin(
                       document.objects().front(), leftOrigin) &&
                       cr::creativeVec3ExactlyEqual(leftOrigin,
                                                    {0.0, 0.5, 0.0}) &&
                       cr::resolveCreativeSelectionPlacementObjectOrigin(
                           boundsOnly, boundsOrigin) &&
                       cr::creativeVec3ExactlyEqual(boundsOrigin,
                                                    {4.0, 3.0, 7.0}),
                   "active origins resolve from authored transforms or "
                   "world extents");

  cr::CreativeDocument movedDocument = document;
  cr::CreativeSelectionPlacementRequest move = individual;
  move.mode = cr::CreativeSelectionPlacementMode::Move;
  const cr::CreativeSelectionPlacementReceipt moved =
      cr::placeDocumentObjectsAtomically(movedDocument, ids, move);
  const cr::CreativeObject* movedLeft = movedDocument.findObject(left.objectId);
  const cr::CreativeObject* movedRight =
      movedDocument.findObject(right.objectId);
  ok = expect(moved.accepted && moved.changed && movedLeft != nullptr &&
                  movedRight != nullptr &&
                  cr::creativeVec3ExactlyEqual(
                      movedLeft->transform.position,
                      individualPlan.objects[0].transform.position) &&
                  cr::creativeVec3ExactlyEqual(
                      movedRight->transform.position,
                      individualPlan.objects[1].transform.position) &&
                  cr::creativeVec3ExactlyEqual(
                      movedLeft->transform.rotationEulerRadians,
                      individualPlan.objects[0].transform.rotationEulerRadians) &&
                  cr::creativeVec3ExactlyEqual(
                      movedLeft->transform.scale,
                      individualPlan.objects[0].transform.scale),
              "individual-origin move commit exactly matches preview") &&
       ok;

  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(document, ids, clipboard);
  cr::CreativeClipboardPasteRequest paste;
  paste.offset = {10.0, 0.0, 0.0};
  paste.pivotMode =
      cr::CreativeSelectionPlacementPivotMode::IndividualOrigins;
  paste.coordinateSpace = individual.coordinateSpace;
  paste.coordinateBasisEulerRadians =
      individual.coordinateBasisEulerRadians;
  paste.scaleFactor = individual.scaleFactor;
  paste.hasTransformAnchor = true;
  paste.transformAnchor = individual.sourceAnchor;
  paste.hasAxisAngleRotation = true;
  paste.rotationAxis = individual.rotationAxis;
  paste.rotationRadians = individual.rotationRadians;
  const cr::CreativeClipboardPasteReceipt pasted =
      cr::pasteCreativeClipboardAtomically(document, clipboard, paste);
  const cr::CreativeObject* pastedLeft =
      pasted.pastedObjectIds.empty()
          ? nullptr
          : document.findObject(pasted.pastedObjectIds.front());
  const cr::CreativeObject* pastedRight =
      pasted.pastedObjectIds.size() < 2U
          ? nullptr
          : document.findObject(pasted.pastedObjectIds[1]);
  ok = expect(copied.accepted && pasted.accepted && pasted.changed &&
                  pastedLeft != nullptr && pastedRight != nullptr,
              "individual-origin duplicate commits both objects") &&
       expect(pastedLeft != nullptr && pastedRight != nullptr &&
                  cr::creativeVec3ExactlyEqual(
                      pastedLeft->transform.position,
                      individualPlan.objects[0].transform.position) &&
                  cr::creativeVec3ExactlyEqual(
                      pastedRight->transform.position,
                      individualPlan.objects[1].transform.position),
              "individual-origin duplicate positions match preview") &&
       expect(pastedLeft != nullptr &&
                  cr::creativeVec3ExactlyEqual(
                      pastedLeft->transform.scale,
                      individualPlan.objects[0].transform.scale) &&
                  cr::creativeVec3ExactlyEqual(
                      pastedLeft->transform.rotationEulerRadians,
                      individualPlan.objects[0].transform.rotationEulerRadians),
              "individual-origin duplicate rotation and scale match preview") &&
       ok;

  individual.pivotMode =
      static_cast<cr::CreativeSelectionPlacementPivotMode>(255U);
  const cr::CreativeSelectionPlacementPlan invalid =
      cr::planCreativeSelectionPlacement(document.objects(), individual);
  return expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeSelectionPlacementStatus::InvalidRequest,
                "invalid pivot mode fails closed") &&
         ok;
}

bool selectionPlacementLocalSpaceUsesFrozenBasis() {
  const auto near = [](double actual, double expected) {
    return std::fabs(actual - expected) <= 1.0e-9;
  };
  cr::CreativeObject object;
  object.id = 1U;
  object.kind = cr::CreativeObjectKind::Crate;
  object.name = "Local Basis";
  object.transform.position = {0.0, 0.5, -2.0};
  object.transform.rotationEulerRadians.y = std::numbers::pi * 0.5;
  object.bounds = {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
  const std::array source{object};

  cr::CreativeSelectionPlacementRequest local;
  local.coordinateSpace =
      cr::CreativeSelectionPlacementCoordinateSpace::Local;
  local.coordinateBasisEulerRadians = object.transform.rotationEulerRadians;
  local.scaleFactor = {2.0, 1.0, 1.0};
  const cr::CreativeSelectionPlacementPlan localPlan =
      cr::planCreativeSelectionPlacement(source, local);
  cr::CreativeSelectionPlacementRequest world = local;
  world.coordinateSpace =
      cr::CreativeSelectionPlacementCoordinateSpace::World;
  world.coordinateBasisEulerRadians = {};
  const cr::CreativeSelectionPlacementPlan worldPlan =
      cr::planCreativeSelectionPlacement(source, world);

  const cr::CreativeVec3 aroundY = cr::rotateCreativeVectorAroundAxis(
      {1.0, 0.0, 0.0}, {0.0, 2.0, 0.0}, std::numbers::pi * 0.5);
  const cr::CreativeVec3 invalidAxis = cr::rotateCreativeVectorAroundAxis(
      {1.0, 0.0, 0.0}, {}, std::numbers::pi * 0.5);
  return expect(localPlan.accepted && worldPlan.accepted,
                "world and local coordinate plans are accepted") &&
         expect(near(localPlan.objects[0].transform.position.x, 0.0) &&
                    near(localPlan.objects[0].transform.position.z, -4.0) &&
                    near(worldPlan.objects[0].transform.position.x, 0.0) &&
                    near(worldPlan.objects[0].transform.position.z, -2.0),
                "local scaling follows the frozen rotated X basis") &&
         expect(near(aroundY.x, 0.0) && near(aroundY.y, 0.0) &&
                    near(aroundY.z, -1.0) &&
                    !cr::isFiniteCreativeVec3(invalidAxis),
                "arbitrary world-axis rotation normalizes and fails closed");
}

bool selectionPlacementCapabilitiesAreExplicitAndFailClosed() {
  cr::CreativeObject crate;
  crate.id = 1U;
  crate.kind = cr::CreativeObjectKind::Crate;
  crate.bounds = {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
  cr::CreativeObject room;
  room.id = 2U;
  room.kind = cr::CreativeObjectKind::Room;
  room.bounds = {{0.0, 0.0, 0.0}, {4.0, 3.0, 6.0}};
  cr::CreativeObject route;
  route.id = 3U;
  route.kind = cr::CreativeObjectKind::PatrolRoute;
  route.pathPoints = {{{0.0, 0.0, 0.0}}, {{2.0, 0.0, 0.0}}};

  const std::array crateSelection{crate};
  const std::array roomSelection{room};
  const std::array routeSelection{route};
  const std::array mixedSelection{crate, room, route};
  const cr::CreativeSelectionPlacementCapabilities crateCapabilities =
      cr::resolveCreativeSelectionPlacementCapabilities(crateSelection);
  const cr::CreativeSelectionPlacementCapabilities roomCapabilities =
      cr::resolveCreativeSelectionPlacementCapabilities(roomSelection);
  const cr::CreativeSelectionPlacementCapabilities routeCapabilities =
      cr::resolveCreativeSelectionPlacementCapabilities(routeSelection);
  const cr::CreativeSelectionPlacementCapabilities mixedCapabilities =
      cr::resolveCreativeSelectionPlacementCapabilities(mixedSelection);

  bool ok = expect(
      crateCapabilities.resolved && crateCapabilities.translate &&
          crateCapabilities.rotation ==
              cr::CreativeObjectRotationSupport::Arbitrary &&
          crateCapabilities.scale ==
              cr::CreativeObjectScaleSupport::NonUniform &&
          crateCapabilities.mirror,
      "transform-backed object exposes arbitrary rotation and nonuniform scale");
  ok = expect(
           roomCapabilities.resolved && roomCapabilities.translate &&
               roomCapabilities.rotation ==
                   cr::CreativeObjectRotationSupport::QuarterTurns &&
               roomCapabilities.scale ==
                   cr::CreativeObjectScaleSupport::NonUniform,
           "bounds-only room explicitly exposes quarter-turn rotation") &&
       expect(routeCapabilities.resolved && routeCapabilities.translate &&
                  routeCapabilities.rotation ==
                      cr::CreativeObjectRotationSupport::Arbitrary &&
                  routeCapabilities.scale ==
                      cr::CreativeObjectScaleSupport::NonUniform,
              "path storage exposes exact point transforms") &&
       expect(mixedCapabilities.resolved && mixedCapabilities.translate &&
                  mixedCapabilities.rotation ==
                      cr::CreativeObjectRotationSupport::QuarterTurns &&
                  mixedCapabilities.scale ==
                      cr::CreativeObjectScaleSupport::NonUniform,
              "mixed selection intersects every object's capabilities") &&
       ok;

  cr::CreativeSelectionPlacementRequest arbitraryRoom;
  arbitraryRoom.mode = cr::CreativeSelectionPlacementMode::Copy;
  arbitraryRoom.hasAxisAngleRotation = true;
  arbitraryRoom.rotationAxis = cr::CreativeAxis3::Y;
  arbitraryRoom.rotationRadians = 37.5 * std::numbers::pi / 180.0;
  const cr::CreativeSelectionPlacementPlan rejectedArbitrary =
      cr::planCreativeSelectionPlacement(roomSelection, arbitraryRoom);
  arbitraryRoom.rotationRadians = std::numbers::pi * 0.5;
  const cr::CreativeSelectionPlacementPlan acceptedQuarterTurn =
      cr::planCreativeSelectionPlacement(roomSelection, arbitraryRoom);
  arbitraryRoom.coordinateSpace =
      cr::CreativeSelectionPlacementCoordinateSpace::Local;
  arbitraryRoom.coordinateBasisEulerRadians =
      {0.0, 37.5 * std::numbers::pi / 180.0, 0.0};
  arbitraryRoom.hasAxisAngleRotation = false;
  arbitraryRoom.rotationRadians = 0.0;
  arbitraryRoom.scaleFactor = {2.0, 1.0, 0.5};
  const cr::CreativeSelectionPlacementPlan rejectedLocalBounds =
      cr::planCreativeSelectionPlacement(roomSelection, arbitraryRoom);
  return expect(!rejectedArbitrary.accepted &&
                    rejectedArbitrary.status ==
                        cr::CreativeSelectionPlacementStatus::UnsupportedObject &&
                    rejectedArbitrary.reasonCode ==
                        "selection_placement_rotation_not_representable",
                "bounds-only arbitrary-angle Copy fails before envelope drift") &&
         expect(acceptedQuarterTurn.accepted,
                "bounds-only world quarter turn remains representable") &&
         expect(!rejectedLocalBounds.accepted &&
                    rejectedLocalBounds.reasonCode ==
                        "selection_placement_local_bounds_not_representable",
                "bounds-only nonuniform local scale rejects a rotated basis") &&
         ok;
}

bool selectionPlacementPreservesExternalAttachments() {
  cr::CreativeObject parent;
  parent.id = 10U;
  parent.kind = cr::CreativeObjectKind::Group;
  parent.transform.position = {0.0, 0.0, 0.0};
  cr::CreativeObject child;
  child.id = 11U;
  child.kind = cr::CreativeObjectKind::Wall;
  child.transform.position = {1.0, 1.5, 0.0};
  child.bounds = {{0.5, 0.0, -0.1}, {1.5, 3.0, 0.1}};
  child.parentId = parent.id;
  child.attachmentSocket = "wall_socket";

  cr::CreativeSelectionPlacementRequest move;
  move.mode = cr::CreativeSelectionPlacementMode::Move;
  move.targetAnchor = {5.0, 0.0, 0.0};
  const std::array childOnly{child};
  const cr::CreativeSelectionPlacementPlan rejected =
      cr::planCreativeSelectionPlacement(childOnly, move);
  const std::array assembly{parent, child};
  const cr::CreativeSelectionPlacementPlan accepted =
      cr::planCreativeSelectionPlacement(assembly, move);

  return expect(!rejected.accepted &&
                    rejected.failedObjectId == child.id &&
                    rejected.reasonCode ==
                        "selection_placement_external_parent",
                "moving an attached child without its parent fails closed") &&
         expect(accepted.accepted && accepted.objects.size() == 2U &&
                    accepted.objects[1].parentId == child.parentId &&
                    accepted.objects[1].attachmentSocket ==
                        child.attachmentSocket,
                "moving a complete assembly preserves hierarchy and socket");
}

bool selectionPlacementMoveIsAtomicAndFailClosed() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Placement");
  cr::CreativeDocumentCreateRequest wallRequest;
  wallRequest.kind = cr::CreativeObjectKind::Wall;
  wallRequest.name = "Wall";
  wallRequest.transform.position = {0.0, 0.5, 1.0};
  wallRequest.hasTransformOverride = true;
  wallRequest.bounds = {{-0.5, 0.0, 0.5}, {0.5, 1.0, 1.5}};
  wallRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt wall =
      document.createObject(wallRequest);

  cr::CreativeDocumentCreateRequest roomRequest;
  roomRequest.kind = cr::CreativeObjectKind::Room;
  roomRequest.name = "Room";
  roomRequest.bounds = {{-1.0, 0.0, -2.0}, {1.0, 2.0, 2.0}};
  roomRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt room =
      document.createObject(roomRequest);
  const std::array ids{wall.objectId, room.objectId};

  cr::CreativeSelectionPlacementRequest request;
  request.mode = cr::CreativeSelectionPlacementMode::Move;
  request.targetAnchor = {4.0, 0.0, 6.0};
  request.quarterTurns = 1U;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeSelectionPlacementReceipt moved =
      cr::placeDocumentObjectsAtomically(document, ids, request);
  const cr::CreativeObject* movedWall = document.findObject(wall.objectId);
  const cr::CreativeObject* movedRoom = document.findObject(room.objectId);

  bool ok = expect(moved.accepted && moved.changed &&
                       moved.status ==
                           cr::CreativeSelectionPlacementStatus::Applied,
                   "selection placement move applies") &&
            expect(document.revision() == revisionBefore + 1U,
                   "selection placement advances revision once") &&
            expect(movedWall != nullptr && movedRoom != nullptr &&
                       movedWall->transform.position.x == 5.0 &&
                       movedWall->transform.position.z == 6.0 &&
                       movedRoom->bounds.min.x == 2.0 &&
                       movedRoom->bounds.max.x == 6.0 &&
                       movedRoom->bounds.min.z == 5.0 &&
                       movedRoom->bounds.max.z == 7.0,
                   "transform and bounds-only objects share placement geometry");

  cr::CreativeDocument lockedDocument = document;
  cr::CreativeObject* locked = lockedDocument.findObject(wall.objectId);
  if (locked != nullptr) {
    locked->locked = true;
  }
  const std::uint64_t lockedRevision = lockedDocument.revision();
  const cr::CreativeSelectionPlacementReceipt rejected =
      cr::placeDocumentObjectsAtomically(lockedDocument, ids, request);
  ok = expect(!rejected.accepted && !rejected.changed &&
                  rejected.status ==
                      cr::CreativeSelectionPlacementStatus::LockedObject &&
                  lockedDocument.revision() == lockedRevision,
              "locked move rejects without partial mutation") &&
       ok;

  request.quarterTurns = 4U;
  const cr::CreativeSelectionPlacementPlan invalid =
      cr::planCreativeSelectionPlacement(document.objects(), request);
  return expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeSelectionPlacementStatus::InvalidRequest,
                "out-of-domain quarter turn fails closed") &&
         ok;
}

}  // namespace

int main() {
  const bool ok = defaultStateUsesSelect() &&
                  changingActiveToolWorks() &&
                  sameToolActivationIsNoChange() &&
                  pointerMoveEmitsPreviewIntent() &&
                  selectPressEmitsSelectObjectCandidate() &&
                  movePressSelectsAndBeginsDrag() &&
                  moveDragPreviewCommitLifecycle() &&
                  releaseWithoutDragIsNoOp() &&
                  cancelMidDragDiscardsWithoutMutation() &&
                  toolSwitchAbandonsDrag() &&
                  moveToolPointerMoveKeepsGhostPreview() &&
                  navigatePointerInputIsInert() &&
                  measureClicksPreviewAndCompletePaths() &&
                  unknownInputEmitsNoIntent() &&
                  optionDescriptorsAreContextualAndBounded() &&
                  optionAdjustmentIsDeterministicAndAtomic() &&
                  replaceFilterAndCloneOffsetUseExplicitInputs() &&
                  transformCommandsStoreRadiansAndResolveLiveGeometry() &&
                  immediateTransformUsesSelectionPlacementKernel() &&
                  resetTransformPreservesPositionAndAppliesAtomically() &&
                  selectionPlacementPlanOwnsPreviewAndCommitGeometry() &&
                  selectionPlacementScalePlanMatchesAtomicCommit() &&
                  selectionPlacementPivotModesSharePreviewAndCommitGeometry() &&
                  selectionPlacementLocalSpaceUsesFrozenBasis() &&
                  selectionPlacementCapabilitiesAreExplicitAndFailClosed() &&
                  selectionPlacementPreservesExternalAttachments() &&
                  selectionPlacementPrecisionIsExactAndFailClosed() &&
                  selectionPlacementMoveIsAtomicAndFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
