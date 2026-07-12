#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/tools/Transform.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"

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

bool measurePressMoveReleaseEmitsMeasurementIntents() {
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
                 cr::CreativeToolIntentKind::BeginMeasurement,
             "measure begin intent") &&
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

  const cr::CreativeToolDispatchReceipt end =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerRelease,
                                         5.0,
                                         6.0));
  const bool ended =
      expect(end.emittedIntentCount == 1U, "measure end count") &&
      expect(end.intents[0].kind ==
                 cr::CreativeToolIntentKind::EndMeasurement,
             "measure end intent") &&
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
  const cr::CreativeToolOptionList replace =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::VolumeReplace);
  const cr::CreativeToolOptionList fill =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::VolumeFill);
  const cr::CreativeToolOptionList hollow =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::VolumeHollow);
  const cr::CreativeToolOptionList clone =
      cr::creativeToolOptionsForHeldItem(cr::CreativeHeldItemKind::VolumeClone);
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
         expect(material.count == 2U &&
                    material.ids[0] ==
                        cr::CreativeToolOptionId::PlacementYaw &&
                    material.ids[1] == cr::CreativeToolOptionId::SnapIncrement,
                "material exposes orientation and grid size") &&
         expect(materialBrush.count == 4U &&
                    materialBrush.ids[0] ==
                        cr::CreativeToolOptionId::MaterialBrushShape &&
                    materialBrush.ids[1] ==
                        cr::CreativeToolOptionId::MaterialBrushSize &&
                    materialBrush.ids[2] ==
                        cr::CreativeToolOptionId::MaterialBrushPlane &&
                    materialBrush.ids[3] ==
                        cr::CreativeToolOptionId::MaterialBrushMask,
                "material brush exposes shape size plane and occupancy mask") &&
         expect(cylinderBrush.count == 5U &&
                    cylinderBrush.ids[0] ==
                        cr::CreativeToolOptionId::MaterialBrushShape &&
                    cylinderBrush.ids[1] ==
                        cr::CreativeToolOptionId::MaterialBrushAxis &&
                    cylinderBrush.ids[2] ==
                        cr::CreativeToolOptionId::MaterialBrushSize &&
                    cylinderBrush.ids[3] ==
                        cr::CreativeToolOptionId::MaterialBrushPlane &&
                    cylinderBrush.ids[4] ==
                        cr::CreativeToolOptionId::MaterialBrushMask,
                "cylinder brush exposes its contextual extrusion axis") &&
         expect(replaceBrush.count == 5U &&
                    replaceBrush.ids[4] ==
                        cr::CreativeToolOptionId::MaterialBrushReplaceSource,
                "replace brush exposes its contextual source filter") &&
         expect(cylinderReplaceBrush.count ==
                        cr::kCreativeToolOptionCapacity &&
                    cylinderReplaceBrush.ids[1] ==
                        cr::CreativeToolOptionId::MaterialBrushAxis &&
                    cylinderReplaceBrush.ids[5] ==
                        cr::CreativeToolOptionId::MaterialBrushReplaceSource,
                "cylinder replace options exactly fit bounded storage") &&
         expect(move.count == 3U &&
                    move.ids[0] ==
                        cr::CreativeToolOptionId::MoveConstraint &&
                    move.ids[1] == cr::CreativeToolOptionId::RotationStep &&
                    move.ids[2] == cr::CreativeToolOptionId::SnapIncrement,
                "move options retain descriptor order") &&
         expect(fill.count == 3U && hollow.count == 3U &&
                    fill.ids[0] == cr::CreativeToolOptionId::SnapIncrement &&
                    fill.ids[1] == cr::CreativeToolOptionId::ShapeBrushKind &&
                    fill.ids[2] == cr::CreativeToolOptionId::ShapeBrushAxis &&
                    hollow.ids[1] ==
                        cr::CreativeToolOptionId::ShapeBrushKind,
                "fill and hollow expose bounded shape controls") &&
         expect(replace.count == 2U &&
                    replace.ids[1] ==
                        cr::CreativeToolOptionId::ReplaceSource,
                "replace exposes source filter") &&
         expect(clone.count == 3U &&
                    clone.ids[1] ==
                        cr::CreativeToolOptionId::CloneOffsetAxis &&
                    clone.ids[2] ==
                        cr::CreativeToolOptionId::CloneOffsetDistance,
                "clone exposes offset axis and distance") &&
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
                    !array.capacityExceeded && !radialArray.capacityExceeded &&
                    array.count <= cr::kCreativeToolOptionCapacity,
                "default option lists fit bounded storage") &&
         expect(cr::creativeToolOptionDescriptor(
                    cr::CreativeToolOptionId::Count) == nullptr &&
                    !cr::creativeToolOptionAppliesToHeldItem(
                        cr::CreativeToolOptionId::MoveConstraint,
                        cr::CreativeHeldItemKind::Material),
                "invalid and inapplicable options are rejected");
}

bool optionAdjustmentIsDeterministicAndAtomic() {
  cr::CreativeToolSettings settings = cr::makeDefaultCreativeToolSettings();
  cr::CreativeToolSettings invalidMask = settings;
  invalidMask.materialBrushMask = cr::CreativeMaterialBrushMask::Count;
  cr::CreativeToolSettings invalidBrushAxis = settings;
  invalidBrushAxis.materialBrushAxis = cr::CreativeAxis3::Count;
  cr::CreativeToolSettings invalidBrushPlane = settings;
  invalidBrushPlane.materialBrushPlane =
      cr::CreativeMaterialBrushPlane::Count;
  cr::CreativeToolSettings invalidBrushSource = settings;
  invalidBrushSource.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Count;
  cr::CreativeToolSettings invalidBrushPropSource = settings;
  invalidBrushPropSource.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Crate;
  bool ok = expect(cr::isValidCreativeToolSettings(settings),
                   "default settings valid") &&
            expect(!cr::isValidCreativeToolSettings(invalidMask),
                   "invalid material brush mask fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushAxis),
                   "invalid material brush axis fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushPlane),
                   "invalid material brush plane fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushSource),
                   "invalid material brush source fails settings validation") &&
            expect(!cr::isValidCreativeToolSettings(invalidBrushPropSource),
                   "non-voxel brush source fails settings validation") &&
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
                           cr::CreativeToolOptionId::ShapeBrushKind) == "BOX" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::ShapeBrushAxis) == "Y" &&
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
                           cr::CreativeToolOptionId::MaterialBrushPlane) ==
                           "FREE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::MaterialBrushMask) ==
                           "OVERWRITE" &&
                       cr::creativeToolOptionValueLabel(
                           settings,
                           cr::CreativeToolOptionId::
                               MaterialBrushReplaceSource) == "ANY",
                   "default labels and scalar conversions stable");

  const auto adjust = [&settings](cr::CreativeToolOptionId option,
                                  std::int32_t direction) {
    return cr::adjustCreativeToolOption(settings, option, direction);
  };
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
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushPlane, 1).changed &&
                  settings.materialBrushPlane ==
                      cr::CreativeMaterialBrushPlane::X,
              "material brush plane cycles from free to X") &&
       expect(adjust(cr::CreativeToolOptionId::MaterialBrushMask, 1).changed &&
                  settings.materialBrushMask ==
                      cr::CreativeMaterialBrushMask::AddOnly,
              "material brush mask cycles from overwrite to add-only") &&
       expect(adjust(cr::CreativeToolOptionId::ShapeBrushKind, 1).changed &&
                  settings.shapeBrushKind == cr::CreativeShapeBrushKind::Line,
              "shape kind cycles") &&
       expect(adjust(cr::CreativeToolOptionId::ShapeBrushAxis, 1).changed &&
                  settings.shapeBrushAxis == cr::CreativeShapeBrushAxis::Z,
              "shape axis cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneOffsetAxis, 1).changed &&
                  settings.cloneOffsetAxis == cr::CreativeCloneOffsetAxis::Y,
              "clone axis cycles") &&
       expect(adjust(cr::CreativeToolOptionId::CloneOffsetDistance, 1).changed &&
                  settings.cloneOffsetDistance ==
                      cr::CreativeCloneOffsetDistance::TwoCells,
              "clone distance cycles") &&
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

  cr::CreativeToolSettings noPalette = cr::makeDefaultCreativeToolSettings();
  const cr::CreativeToolOptionAdjustReceipt unavailable =
      cr::adjustCreativeToolOption(
          noPalette, cr::CreativeToolOptionId::ReplaceSource, 1);
  const cr::CreativeToolOptionAdjustReceipt brushUnavailable =
      cr::adjustCreativeToolOption(
          noPalette, cr::CreativeToolOptionId::MaterialBrushReplaceSource, 1);
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
  return expect(!invalidTarget.accepted && !invalidAxis.accepted &&
                    !overflow.accepted && !overflow.changed,
                "invalid target, axis, and overflowing nudge fail closed") &&
         ok;
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
                  measurePressMoveReleaseEmitsMeasurementIntents() &&
                  unknownInputEmitsNoIntent() &&
                  optionDescriptorsAreContextualAndBounded() &&
                  optionAdjustmentIsDeterministicAndAtomic() &&
                  replaceFilterAndCloneOffsetUseExplicitInputs() &&
                  transformCommandsStoreRadiansAndResolveLiveGeometry() &&
                  selectionPlacementPlanOwnsPreviewAndCommitGeometry() &&
                  selectionPlacementPrecisionIsExactAndFailClosed() &&
                  selectionPlacementMoveIsAtomicAndFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
