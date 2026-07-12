#include "EditorActionHints.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"

#include <array>
#include <iostream>
#include <string_view>
#include <type_traits>
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

const cr::CreativeActionHint* findHint(
    const cr::CreativeActionHintFrame& frame,
    cr::CreativeInputActionId action) {
  for (const cr::CreativeActionHint& hint : frame.items()) {
    for (std::size_t index = 0U; index < hint.actionCount; ++index) {
      if (hint.actions[index] == action) {
        return &hint;
      }
    }
  }
  return nullptr;
}

bool hintBindingsAreLive(const cr::CreativeActionHintFrame& frame,
                         const cr::CreativeControlProfile& profile,
                         cr::CreativeInputContext context) {
  for (const cr::CreativeActionHint& hint : frame.items()) {
    for (std::size_t actionIndex = 0U; actionIndex < hint.actionCount;
         ++actionIndex) {
      bool found = false;
      for (const cr::CreativeInputBinding& binding : profile.bindingSpan()) {
        found = found ||
                (binding.action == hint.actions[actionIndex] &&
                 binding.trigger == hint.triggers[actionIndex] &&
                 binding.context == context);
      }
      if (!found) {
        return false;
      }
    }
  }
  return true;
}

bool widgetTextFits(const cr::CreativeUiWidgetFrame& frame) {
  constexpr float kGlyphWidth = 10.0F;
  constexpr float kGlyphAdvance = 12.0F;
  for (const cr::CreativeUiWidgetVisual& visual : frame.visualItems()) {
    if (visual.kind != cr::CreativeUiWidgetVisualKind::Text ||
        visual.text.length == 0U) {
      continue;
    }
    const float required =
        kGlyphWidth +
        static_cast<float>(visual.text.length - 1U) * kGlyphAdvance;
    if (required > visual.rect.width + 0.01F) {
      return false;
    }
  }
  return true;
}

void setHeld(CreativeEditorState& editor,
             cr::CreativeHeldItemKind kind,
             cr::CreativeObjectKind objectKind =
                 cr::CreativeObjectKind::Unknown) {
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {kind, objectKind};
  editor.quickEdit.targetEntry = {kind, objectKind};
  editor.quickEdit.options =
      creativeEditorToolOptionsForEntry(editor.quickEdit.targetEntry,
                                        editor.toolSettings);
  editor.quickEdit.selectedIndex = 0U;
}

std::uint16_t groupFor(const cr::CreativeControlProfile& profile,
                       cr::CreativeInputActionId action,
                       cr::CreativeControlDevice device) {
  for (std::size_t group = 0U; group < profile.groupCount; ++group) {
    if (profile.groupActions[group] == action &&
        profile.groupDevices[group] == device) {
      return static_cast<std::uint16_t>(group);
    }
  }
  return static_cast<std::uint16_t>(profile.groupCount);
}

bool resolverUsesLiveBindingsAndBoundedPairs() {
  cr::CreativeControlProfile profile = cr::makeDefaultCreativeControlProfile();
  constexpr std::array specs{
      cr::CreativeActionHintSpec{
          {{cr::CreativeInputActionId::AcceptAction,
            cr::CreativeInputActionId::Count}},
          1U, "Place"},
      cr::CreativeActionHintSpec{
          {{cr::CreativeInputActionId::QuickEditPrevious,
            cr::CreativeInputActionId::QuickEditNext}},
          2U, "Setting"},
  };
  cr::CreativeActionHintFrame frame = cr::resolveCreativeActionHints(
      profile, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, cr::CreativeInputPlatform::MacOS,
      specs);
  const cr::CreativeActionHint* place =
      findHint(frame, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* setting =
      findHint(frame, cr::CreativeInputActionId::QuickEditPrevious);
  bool ok = expect(frame.count == 2U && frame.unresolvedCount == 0U &&
                       !frame.capacityExceeded && !frame.invalidInput,
                   "valid semantic hint request resolves without degradation") &&
            expect(place != nullptr && place->chord.view() == "X" &&
                       place->label.view() == "Place",
                   "PS5 confirm label comes from the live control profile") &&
            expect(setting != nullptr &&
                       setting->chord.view() == "D-pad U/D" &&
                       setting->label.view() == "Setting",
                   "paired semantic actions remain one bounded hint") &&
            expect(std::is_trivially_copyable_v<cr::CreativeActionHintFrame>,
                   "per-frame hints remain fixed-layout and allocation-free");

  cr::CreativeControlRebindRequest rebind;
  rebind.group = groupFor(profile, cr::CreativeInputActionId::AcceptAction,
                          cr::CreativeControlDevice::Gamepad);
  rebind.trigger = cr::CreativeInputKey::GamepadBack;
  const cr::CreativeControlRebindReceipt rebound =
      cr::rebindCreativeControl(profile, rebind);
  frame = cr::resolveCreativeActionHints(
      profile, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, cr::CreativeInputPlatform::MacOS,
      specs);
  place = findHint(frame, cr::CreativeInputActionId::AcceptAction);
  return expect(rebound.changed &&
                    rebound.status == cr::CreativeControlRebindStatus::Applied,
                "test remap changes the semantic confirm group") &&
         expect(place != nullptr && place->chord.view() == "Create",
                "ribbon reflects remapped control without copied labels") &&
         ok;
}

bool resolverSkipsMissingBindingsAndFailsClosedAtCapacity() {
  const cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  constexpr std::array missing{
      cr::CreativeActionHintSpec{
          {{cr::CreativeInputActionId::CatalogConfirm,
            cr::CreativeInputActionId::Count}},
          1U, "Not in viewport"},
  };
  const cr::CreativeActionHintFrame unresolved =
      cr::resolveCreativeActionHints(
          profile, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad,
          cr::CreativeInputPlatform::MacOS, missing);
  std::array<cr::CreativeActionHintSpec,
             cr::kCreativeActionHintCapacity + 1U>
      oversized{};
  for (cr::CreativeActionHintSpec& spec : oversized) {
    spec.actions[0] = cr::CreativeInputActionId::AcceptAction;
    spec.actionCount = 1U;
    spec.label = "Too many";
  }
  const cr::CreativeActionHintFrame overflow = cr::resolveCreativeActionHints(
      profile, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, cr::CreativeInputPlatform::MacOS,
      oversized);
  return expect(unresolved.count == 0U && unresolved.unresolvedCount == 1U,
                "actions without a live context binding are not displayed") &&
         expect(overflow.count == 0U && overflow.capacityExceeded,
                "oversized hint requests fail closed");
}

bool activeDeviceUsesUnambiguousPhysicalActivity() {
  cr::CreativeInputFrame keyboardFrame;
  cr::setCreativeInputKey(keyboardFrame, cr::CreativeInputKey::W, true);
  const cr::CreativeControlDeviceActivity keyboard =
      cr::measureCreativeControlDeviceActivity(keyboardFrame, false, false);
  cr::CreativeInputFrame gamepadFrame;
  cr::setCreativeInputKey(gamepadFrame,
                          cr::CreativeInputKey::GamepadConfirm, true);
  const cr::CreativeControlDeviceActivity gamepad =
      cr::measureCreativeControlDeviceActivity(gamepadFrame, false, true);
  cr::CreativeControlDeviceActivity simultaneous;
  simultaneous.keyboardMouse = true;
  simultaneous.gamepad = true;
  return expect(keyboard.keyboardMouse && !keyboard.gamepad,
                "keyboard activity is classified once") &&
         expect(gamepad.gamepad && !gamepad.keyboardMouse,
                "buttons and analog controller activity classify gamepad") &&
         expect(cr::resolveCreativeActiveControlDevice(
                    cr::CreativeControlDevice::KeyboardMouse, gamepad) ==
                    cr::CreativeControlDevice::Gamepad,
                "unambiguous controller activity switches the ribbon") &&
         expect(cr::resolveCreativeActiveControlDevice(
                    cr::CreativeControlDevice::Gamepad, simultaneous) ==
                    cr::CreativeControlDevice::Gamepad,
                "simultaneous activity preserves the prior device") &&
         expect(cr::resolveCreativeActiveControlDevice(
                    cr::CreativeControlDevice::Count, {}) ==
                    cr::CreativeControlDevice::KeyboardMouse,
                "invalid idle prior state fails closed to keyboard and mouse");
}

bool editorHintsMatchToolsContextsAndPs5Language() {
  CreativeEditorState editor;
  setHeld(editor, cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::Wall);
  editor.quickEdit.options = {};
  const cr::CreativeActionHintFrame material =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* place =
      findHint(material, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* remove =
      findHint(material, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* hotbar =
      findHint(material, cr::CreativeInputActionId::HotbarPrevious);
  const cr::CreativeActionHintFrame keyboardMaterial =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::KeyboardMouse, false);
  const cr::CreativeActionHint* keyboardPlace =
      findHint(keyboardMaterial, cr::CreativeInputActionId::SecondaryAction);
  bool ok = expect(material.count == 6U && place != nullptr &&
                       place->chord.view() == "X" &&
                       place->label.view() == "Place" && remove != nullptr &&
                       remove->chord.view() == "Circle" &&
                       remove->label.view() == "Remove",
                   "material ribbon uses PS5 confirm and cancel grammar") &&
            expect(hotbar != nullptr && hotbar->chord.view() == "L1/R1",
                   "material ribbon exposes paired hotbar shoulders") &&
            expect(keyboardPlace != nullptr &&
                       keyboardPlace->chord.view() == "Mouse R" &&
                       keyboardPlace->label.view() == "Place",
                   "keyboard and mouse receive compact device-native hints");

  setHeld(editor, cr::CreativeHeldItemKind::MaterialBrush,
          cr::CreativeObjectKind::Wall);
  const cr::CreativeActionHintFrame brush = resolveCreativeEditorActionHints(
      editor, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* paint =
      findHint(brush, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* erase =
      findHint(brush, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* brushSetting =
      findHint(brush, cr::CreativeInputActionId::QuickEditPrevious);
  ok = expect(paint != nullptr && paint->chord.view() == "X" &&
                  paint->label.view() == "Paint" && erase != nullptr &&
                  erase->chord.view() == "Circle" &&
                  erase->label.view() == "Erase" &&
                  brushSetting != nullptr &&
                  brushSetting->chord.view() == "D-pad U/D",
              "material brush advertises controller-native paint erase and settings") &&
       ok;

  setHeld(editor, cr::CreativeHeldItemKind::ConnectedFill,
          cr::CreativeObjectKind::Floor);
  const cr::CreativeActionHintFrame connectedFill =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* fillRegion =
      findHint(connectedFill, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* eraseRegion =
      findHint(connectedFill, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* fillLimit =
      findHint(connectedFill,
               cr::CreativeInputActionId::QuickEditPrevious);
  ok = expect(fillRegion != nullptr && fillRegion->chord.view() == "X" &&
                  fillRegion->label.view() == "Fill region" &&
                  eraseRegion != nullptr &&
                  eraseRegion->chord.view() == "Circle" &&
                  eraseRegion->label.view() == "Erase region" &&
                  fillLimit != nullptr &&
                  fillLimit->chord.view() == "D-pad U/D",
              "connected fill exposes paint erase and bounded-limit controls") &&
       ok;

  setHeld(editor, cr::CreativeHeldItemKind::SurfaceExtrude,
          cr::CreativeObjectKind::Wall);
  const cr::CreativeActionHintFrame surfaceExtrude =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* extrude =
      findHint(surfaceExtrude, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* removeLayer =
      findHint(surfaceExtrude, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* surfaceSetting =
      findHint(surfaceExtrude,
               cr::CreativeInputActionId::QuickEditPrevious);
  ok = expect(extrude != nullptr && extrude->chord.view() == "X" &&
                  extrude->label.view() == "Extrude" &&
                  removeLayer != nullptr &&
                  removeLayer->chord.view() == "Circle" &&
                  removeLayer->label.view() == "Remove layer" &&
                  surfaceSetting != nullptr &&
                  surfaceSetting->chord.view() == "D-pad U/D",
              "surface extrude exposes pull remove and bounded settings") &&
       ok;

  setHeld(editor, cr::CreativeHeldItemKind::TerrainControl);
  const cr::CreativeActionHintFrame terrain = resolveCreativeEditorActionHints(
      editor, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* terrainApply =
      findHint(terrain, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* terrainReject =
      findHint(terrain, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* terrainSelect =
      findHint(terrain, cr::CreativeInputActionId::PickAction);
  const cr::CreativeActionHint* terrainHeight =
      findHint(terrain, cr::CreativeInputActionId::QuickEditPrevious);
  const cr::CreativeActionHint* terrainRadius =
      findHint(terrain, cr::CreativeInputActionId::QuickEditDecrease);
  ok = expect(terrainApply != nullptr &&
                  terrainApply->label.view() == "Paint rods" &&
                  terrainReject != nullptr &&
                  terrainReject->label.view() == "Erase rods" &&
                  terrainSelect != nullptr &&
                  terrainSelect->label.view() == "Select rod" &&
                  terrainHeight != nullptr &&
                  terrainHeight->chord.view() == "D-pad U/D" &&
                  terrainHeight->label.view() == "Height" &&
                  terrainRadius != nullptr &&
                  terrainRadius->chord.view() == "D-pad L/R" &&
                  terrainRadius->label.view() == "Radius",
              "terrain hints expose direct height radius and edit actions") &&
       ok;
  editor.terrain.selectionValid = true;
  const cr::CreativeActionHintFrame terrainEdit =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* terrainCommit =
      findHint(terrainEdit, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* terrainCancel =
      findHint(terrainEdit, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHintFrame keyboardTerrainEdit =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::KeyboardMouse, false);
  const cr::CreativeActionHint* keyboardTerrainCommit = findHint(
      keyboardTerrainEdit, cr::CreativeInputActionId::SecondaryAction);
  const cr::CreativeActionHint* keyboardTerrainCancel = findHint(
      keyboardTerrainEdit, cr::CreativeInputActionId::PrimaryAction);
  const cr::CreativeActionHint* keyboardTerrainHeight = findHint(
      keyboardTerrainEdit, cr::CreativeInputActionId::QuickEditPrevious);
  const cr::CreativeActionHint* keyboardTerrainRadius = findHint(
      keyboardTerrainEdit, cr::CreativeInputActionId::QuickEditDecrease);
  ok = expect(terrainCommit != nullptr && terrainCommit->chord.view() == "X" &&
                  terrainCommit->label.view() == "Apply edit" &&
                  terrainCancel != nullptr &&
                  terrainCancel->chord.view() == "Circle" &&
                  terrainCancel->label.view() == "Cancel edit",
              "selected terrain draft advertises PS5 commit and cancel") &&
       expect(keyboardTerrainCommit != nullptr &&
                  keyboardTerrainCommit->label.view() == "Apply edit" &&
                  keyboardTerrainCancel != nullptr &&
                  keyboardTerrainCancel->label.view() == "Cancel edit" &&
                  keyboardTerrainHeight != nullptr &&
                  keyboardTerrainHeight->chord.view() == "Up/Down" &&
                  keyboardTerrainRadius != nullptr &&
                  keyboardTerrainRadius->chord.view() == "Left/Right",
              "selected terrain draft advertises mouse and arrow controls") &&
       ok;
  editor.terrain.selectionValid = false;

  setHeld(editor, cr::CreativeHeldItemKind::TerrainGrade);
  editor.terrain.grade.anchorValid = true;
  const cr::CreativeActionHintFrame terrainGrade =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* gradeApply =
      findHint(terrainGrade, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* gradeCancel =
      findHint(terrainGrade, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* gradeAnchor =
      findHint(terrainGrade, cr::CreativeInputActionId::PickAction);
  const cr::CreativeActionHint* gradeHeight =
      findHint(terrainGrade, cr::CreativeInputActionId::QuickEditPrevious);
  const cr::CreativeActionHint* gradeWidth =
      findHint(terrainGrade, cr::CreativeInputActionId::QuickEditDecrease);
  ok = expect(gradeApply != nullptr && gradeApply->chord.view() == "X" &&
                  gradeApply->label.view() == "Apply grade" &&
                  gradeCancel != nullptr &&
                  gradeCancel->chord.view() == "Circle" &&
                  gradeCancel->label.view() == "Cancel grade" &&
                  gradeAnchor != nullptr &&
                  gradeAnchor->chord.view() == "Square" &&
                  gradeAnchor->label.view() == "Set start rod" &&
                  gradeHeight != nullptr &&
                  gradeHeight->label.view() == "End height" &&
                  gradeWidth != nullptr && gradeWidth->label.view() == "Width",
              "terrain grade advertises anchor apply cancel height and width") &&
       ok;

  setHeld(editor, cr::CreativeHeldItemKind::TerrainSculpt);
  const cr::CreativeActionHintFrame terrainSculpt =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* sculptApply =
      findHint(terrainSculpt, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* sculptCancel =
      findHint(terrainSculpt, cr::CreativeInputActionId::RejectAction);
  const cr::CreativeActionHint* sculptSample =
      findHint(terrainSculpt, cr::CreativeInputActionId::PickAction);
  const cr::CreativeActionHint* sculptStrength =
      findHint(terrainSculpt, cr::CreativeInputActionId::QuickEditPrevious);
  const cr::CreativeActionHint* sculptRadius =
      findHint(terrainSculpt, cr::CreativeInputActionId::QuickEditDecrease);
  ok = expect(sculptApply != nullptr && sculptApply->chord.view() == "X" &&
                  sculptApply->label.view() == "Sculpt" &&
                  sculptCancel != nullptr &&
                  sculptCancel->chord.view() == "Circle" &&
                  sculptCancel->label.view() == "Cancel sculpt" &&
                  sculptSample != nullptr &&
                  sculptSample->chord.view() == "Square" &&
                  sculptSample->label.view() == "Sample height" &&
                  sculptStrength != nullptr &&
                  sculptStrength->label.view() == "Strength" &&
                  sculptRadius != nullptr &&
                  sculptRadius->label.view() == "Radius",
              "terrain sculpt advertises apply cancel sample strength and radius") &&
       ok;

  setHeld(editor, cr::CreativeHeldItemKind::LinearArray);
  const cr::CreativeActionHintFrame array = resolveCreativeEditorActionHints(
      editor, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* arrayApply =
      findHint(array, cr::CreativeInputActionId::AcceptAction);
  const cr::CreativeActionHint* setting =
      findHint(array, cr::CreativeInputActionId::QuickEditPrevious);
  ok = expect(array.count == 6U && arrayApply != nullptr &&
                  arrayApply->label.view() == "Select/Use" &&
                  setting != nullptr &&
                  setting->chord.view() == "D-pad U/D",
              "array ribbon prioritizes apply and quick-edit controls") &&
       ok;

  const cr::CreativeActionHintFrame transform =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::TransformPreview,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* transformControls =
      findHint(transform,
               cr::CreativeInputActionId::ToggleTransformControls);
  ok = expect(transformControls != nullptr &&
                  transformControls->chord.view() == "R3" &&
                  transformControls->label.view() == "Controls",
              "transform preview advertises its contextual R3 controls") &&
       ok;

  const cr::CreativeActionHintFrame catalog = resolveCreativeEditorActionHints(
      editor, cr::CreativeInputContext::Catalog,
      cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* equip =
      findHint(catalog, cr::CreativeInputActionId::CatalogConfirm);
  const cr::CreativeActionHint* assignWheel =
      findHint(catalog,
               cr::CreativeInputActionId::CatalogAssignToolWheel);
  editor.catalog.toolWheelAssignmentCatalogEntryIndex = 0U;
  const cr::CreativeActionHintFrame wheelAssignment =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::ToolWheel,
          cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeActionHint* assignmentConfirm =
      findHint(wheelAssignment,
               cr::CreativeInputActionId::ToolWheelConfirm);
  const cr::CreativeActionHint* assignmentOptions =
      findHint(wheelAssignment,
               cr::CreativeInputActionId::ToolWheelOptions);
  const cr::CreativeActionHintFrame capture =
      resolveCreativeEditorActionHints(
          editor, cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad, true);
  return expect(catalog.count == 6U && equip != nullptr &&
                    equip->chord.view() == "X" &&
                    equip->label.view() == "Equip",
                "modal catalog context receives its own semantic actions") &&
         expect(assignWheel != nullptr &&
                    assignWheel->chord.view() == "Square" &&
                    assignWheel->label.view() == "Assign wheel",
                "catalog advertises the contextual PS5 wheel assignment") &&
         expect(wheelAssignment.count == 3U &&
                    assignmentConfirm != nullptr &&
                    assignmentConfirm->label.view() == "Assign" &&
                    assignmentOptions == nullptr,
                "assignment wheel replaces equip and settings with assign") &&
         expect(capture.count == 0U,
                "capture mode emits no interactive action ribbon") &&
         ok;
}

bool everyInteractiveContextResolvesOnlyLiveBindings() {
  CreativeEditorState editor;
  setHeld(editor, cr::CreativeHeldItemKind::LinearArray);
  constexpr std::array contexts{
      cr::CreativeInputContext::Catalog,
      cr::CreativeInputContext::ToolWheel,
      cr::CreativeInputContext::ToolOptions,
      cr::CreativeInputContext::TransformPreview,
      cr::CreativeInputContext::TransformControls,
      cr::CreativeInputContext::Controls,
  };
  constexpr std::array devices{
      cr::CreativeControlDevice::KeyboardMouse,
      cr::CreativeControlDevice::Gamepad,
  };
  bool ok = true;
  for (cr::CreativeControlDevice device : devices) {
    for (std::size_t heldIndex = 0U;
         heldIndex < static_cast<std::size_t>(cr::CreativeHeldItemKind::Count);
         ++heldIndex) {
      setHeld(editor, static_cast<cr::CreativeHeldItemKind>(heldIndex),
              cr::CreativeObjectKind::Crate);
      const cr::CreativeActionHintFrame viewport =
          resolveCreativeEditorActionHints(
              editor, cr::CreativeInputContext::EditorViewport, device,
              false);
      ok = expect(viewport.count > 0U && !viewport.invalidInput &&
                      !viewport.capacityExceeded,
                  "every held tool has a bounded viewport hint set") &&
           expect(hintBindingsAreLive(
                      viewport, editor.controlProfile,
                      cr::CreativeInputContext::EditorViewport),
                  "every held-tool hint names a live viewport binding") &&
           ok;
    }
    for (cr::CreativeInputContext context : contexts) {
      const cr::CreativeActionHintFrame frame =
          resolveCreativeEditorActionHints(editor, context, device, false);
      ok = expect(frame.count > 0U && !frame.invalidInput &&
                      !frame.capacityExceeded,
                  "every interactive context has a bounded hint set") &&
           expect(hintBindingsAreLive(frame, editor.controlProfile, context),
                  "every context hint names a live profile binding") &&
           ok;
    }
  }
  return ok;
}

bool widgetProjectionIsResponsiveAndUsesTheStandardFrame() {
  CreativeEditorState editor;
  setHeld(editor, cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::Wall);
  editor.quickEdit.options = {};
  const cr::CreativeActionHintFrame hints = resolveCreativeEditorActionHints(
      editor, cr::CreativeInputContext::EditorViewport,
      cr::CreativeControlDevice::Gamepad, false);
  const cr::CreativeUiWidgetFrame wide =
      buildCreativeEditorActionHintWidgetFrame(hints, 1280U, 720U);
  const cr::CreativeUiWidgetFrame compact =
      buildCreativeEditorActionHintWidgetFrame(hints, 480U, 320U);
  const cr::CreativeUiWidgetFrame tooNarrow =
      buildCreativeEditorActionHintWidgetFrame(hints, 120U, 240U);
  std::vector<iggy3d::RenderUiRect> rects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  const iggy3d::CreativeUiWidgetOverlayAppendReceipt projected =
      iggy3d::appendCreativeUiWidgetOverlay(wide, 1280U, 720U, rects, glyphs);
  return expect(!wide.invalidInput && !wide.capacityExceeded &&
                    wide.widgetCount == 0U && wide.visualCount == 24U,
                "six passive hints use standard panel and label visuals") &&
         expect(!compact.invalidInput && compact.visualCount == 12U,
                "compact viewport keeps the three highest-priority hints") &&
         expect(!tooNarrow.invalidInput && tooNarrow.visualCount == 0U,
                "undersized viewport omits the ribbon instead of overlapping") &&
         expect(widgetTextFits(wide) && widgetTextFits(compact),
                "tile widths contain every rendered chord and label") &&
         expect(projected.ready && projected.rectCount == 12U &&
                    projected.textGlyphCount > 0U && !rects.empty() &&
                    !glyphs.empty(),
                "standard widget overlay is the sole ribbon draw path");
}

}  // namespace

int main() {
  bool ok = true;
  ok = resolverUsesLiveBindingsAndBoundedPairs() && ok;
  ok = resolverSkipsMissingBindingsAndFailsClosedAtCapacity() && ok;
  ok = activeDeviceUsesUnambiguousPhysicalActivity() && ok;
  ok = editorHintsMatchToolsContextsAndPs5Language() && ok;
  ok = everyInteractiveContextResolvesOnlyLiveBindings() && ok;
  ok = widgetProjectionIsResponsiveAndUsesTheStandardFrame() && ok;
  return ok ? 0 : 1;
}
