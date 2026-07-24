#include "app/iggy3d/creative/input/InputRouter.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace iggy3d::creative {
namespace {

constexpr CreativeInputModifierMask kAllModifiers =
    kCreativeInputModifierShift | kCreativeInputModifierControl |
    kCreativeInputModifierAlt | kCreativeInputModifierCommand;

constexpr std::array kControllerCommandChords{
    CreativeControllerCommandChord{CreativeInputKey::GamepadDpadLeft,
                                   CreativeInputActionId::Undo},
    CreativeControllerCommandChord{CreativeInputKey::GamepadDpadRight,
                                   CreativeInputActionId::Redo},
    CreativeControllerCommandChord{CreativeInputKey::GamepadDpadUp,
                                   CreativeInputActionId::Save},
};
static_assert(kControllerCommandChords.size() ==
              kCreativeControllerCommandChordCapacity);

[[nodiscard]] std::size_t keyIndex(CreativeInputKey key) noexcept {
  return static_cast<std::size_t>(key);
}

[[nodiscard]] std::size_t actionIndex(
    CreativeInputActionId action) noexcept {
  return static_cast<std::size_t>(action);
}

[[nodiscard]] bool modifiersMatch(
    CreativeInputModifierMask modifiers,
    const CreativeInputBinding& binding) noexcept {
  if ((modifiers & binding.requiredAllModifiers) !=
      binding.requiredAllModifiers) {
    return false;
  }
  if (binding.requiredAnyModifiers != kCreativeInputModifierNone &&
      (modifiers & binding.requiredAnyModifiers) == 0U) {
    return false;
  }
  return (modifiers & ~binding.allowedModifiers) == 0U;
}

[[nodiscard]] bool physicalChordActive(
    const CreativeInputFrame& frame,
    const CreativeInputBinding& binding) noexcept {
  return binding.trigger != CreativeInputKey::Unbound &&
         creativeInputKeyDown(frame, binding.trigger) &&
         modifiersMatch(frame.modifiers, binding);
}

void consumeKey(CreativeInputRouteResult& result,
                CreativeInputKey key) noexcept {
  result.consumedKeys[keyIndex(key)] = true;
}

void consumeModifierKeys(CreativeInputRouteResult& result,
                         const CreativeInputFrame& frame) noexcept {
  constexpr std::array modifierKeys{
      CreativeInputKey::LeftControl,
      CreativeInputKey::RightControl,
      CreativeInputKey::LeftShift,
      CreativeInputKey::RightShift,
      CreativeInputKey::LeftAlt,
      CreativeInputKey::RightAlt,
      CreativeInputKey::LeftCommand,
      CreativeInputKey::RightCommand,
  };
  for (CreativeInputKey key : modifierKeys) {
    if (creativeInputKeyDown(frame, key)) {
      consumeKey(result, key);
    }
  }
}

[[nodiscard]] bool actionAlreadyEmitted(
    const CreativeInputRouteResult& result,
    CreativeInputActionId action) noexcept {
  return std::any_of(result.actionEvents().begin(), result.actionEvents().end(),
                     [action](const CreativeInputActionEvent& event) {
                       return event.action == action;
                     });
}

[[nodiscard]] bool keyWasDown(const CreativeInputRouterState& state,
                              CreativeInputKey key) noexcept {
  return key != CreativeInputKey::Unbound && key != CreativeInputKey::Count &&
         state.previousKeysDown[keyIndex(key)];
}

void appendCommandAction(CreativeInputRouteResult& result,
                         CreativeInputActionId action,
                         CreativeInputKey trigger) noexcept {
  if (actionAlreadyEmitted(result, action)) {
    return;
  }
  if (result.actionCount >= result.actions.size()) {
    result.bindingCapacityExceeded = true;
    return;
  }
  result.actions[result.actionCount++] = {action, trigger};
}

void routeControllerCommandLayer(
    CreativeInputRouterState& state,
    const CreativeInputFrame& frame,
    CreativeInputRouteResult& result,
    std::array<bool, kCreativeInputActionCount>& actionActivationEdge,
    std::span<const CreativeControllerCommandChord> controllerCommands) {
  const bool modifierDown =
      creativeInputKeyDown(frame, kCreativeControllerCommandModifier);
  const bool modifierWasDown =
      keyWasDown(state, kCreativeControllerCommandModifier);

  if (modifierDown && !modifierWasDown) {
    state.controllerCommandGestureEligible =
        frame.context == CreativeInputContext::EditorViewport;
    state.controllerCommandGestureUsed = false;
  }
  if (modifierDown && state.controllerCommandGestureEligible &&
      frame.context != CreativeInputContext::EditorViewport) {
    state.controllerCommandGestureEligible = false;
    state.controllerCommandGestureUsed = true;
  }

  result.controllerCommandLayerActive =
      modifierDown && state.controllerCommandGestureEligible &&
      frame.context == CreativeInputContext::EditorViewport;
  if (result.controllerCommandLayerActive) {
    // This layer is exclusive: a document shortcut cannot also move, place,
    // rotate, or navigate a menu.
    for (std::size_t index = 0U; index < kCreativeInputKeyCount; ++index) {
      const CreativeInputKey key = static_cast<CreativeInputKey>(index);
      if (!creativeInputKeyIsGamepad(key)) {
        continue;
      }
      consumeKey(result, key);
      if (key != kCreativeControllerCommandModifier &&
          creativeInputKeyDown(frame, key)) {
        state.controllerCommandGestureUsed = true;
      }
    }

    controllerCommands = controllerCommands.first(std::min(
        controllerCommands.size(), kCreativeControllerCommandChordCapacity));
    for (const CreativeControllerCommandChord& chord : controllerCommands) {
      if (chord.trigger == CreativeInputKey::Unbound ||
          chord.trigger == CreativeInputKey::Count ||
          chord.trigger == kCreativeControllerCommandModifier ||
          !creativeInputKeyIsGamepad(chord.trigger) ||
          actionIndex(chord.action) >= kCreativeInputActionCount) {
        continue;
      }
      const bool triggerDown = creativeInputKeyDown(frame, chord.trigger);
      const bool triggerWasDown = keyWasDown(state, chord.trigger);
      const std::size_t action = actionIndex(chord.action);
      const bool activated = triggerDown && !triggerWasDown;
      if (action < result.down.size()) {
        result.down[action] =
            triggerDown && (state.actionDown[action] || activated);
        actionActivationEdge[action] = actionActivationEdge[action] || activated;
      }
      if (activated) {
        appendCommandAction(result, chord.action, chord.trigger);
      }
    }
  }

  if (!modifierDown && modifierWasDown) {
    if (state.controllerCommandGestureEligible &&
        !state.controllerCommandGestureUsed &&
        frame.context == CreativeInputContext::EditorViewport) {
      appendCommandAction(result, CreativeInputActionId::ToggleControls,
                          kCreativeControllerCommandModifier);
    }
    state.controllerCommandGestureEligible = false;
    state.controllerCommandGestureUsed = false;
  }
}

void appendNamedModifier(std::string& label,
                         CreativeInputModifierMask modifiers,
                         CreativeInputModifierMask modifier,
                         std::string_view name) {
  if ((modifiers & modifier) != 0U) {
    label.append(name);
    label.push_back('+');
  }
}

[[nodiscard]] CreativeInputModifierMask firstModifier(
    CreativeInputModifierMask modifiers) noexcept {
  constexpr std::array candidates{
      kCreativeInputModifierShift,
      kCreativeInputModifierControl,
      kCreativeInputModifierAlt,
      kCreativeInputModifierCommand,
  };
  for (CreativeInputModifierMask candidate : candidates) {
    if ((modifiers & candidate) != 0U) {
      return candidate;
    }
  }
  return kCreativeInputModifierNone;
}

[[nodiscard]] bool modifierDomainsOverlap(
    const CreativeInputBinding& first,
    const CreativeInputBinding& second,
    CreativeInputModifierMask& overlappingModifiers) noexcept {
  for (CreativeInputModifierMask modifiers = kCreativeInputModifierNone;
       modifiers <= kAllModifiers; ++modifiers) {
    if (modifiersMatch(modifiers, first) && modifiersMatch(modifiers, second)) {
      overlappingModifiers = modifiers;
      return true;
    }
  }
  return false;
}

}  // namespace

std::string_view toString(CreativeInputActionId action) noexcept {
  switch (action) {
    case CreativeInputActionId::HotbarSlot1: return "HotbarSlot1";
    case CreativeInputActionId::HotbarSlot2: return "HotbarSlot2";
    case CreativeInputActionId::HotbarSlot3: return "HotbarSlot3";
    case CreativeInputActionId::HotbarSlot4: return "HotbarSlot4";
    case CreativeInputActionId::HotbarSlot5: return "HotbarSlot5";
    case CreativeInputActionId::HotbarSlot6: return "HotbarSlot6";
    case CreativeInputActionId::HotbarSlot7: return "HotbarSlot7";
    case CreativeInputActionId::HotbarSlot8: return "HotbarSlot8";
    case CreativeInputActionId::HotbarSlot9: return "HotbarSlot9";
    case CreativeInputActionId::ToggleCatalog: return "ToggleCatalog";
    case CreativeInputActionId::CatalogPrevious: return "CatalogPrevious";
    case CreativeInputActionId::CatalogNext: return "CatalogNext";
    case CreativeInputActionId::CatalogPreviousVariant:
      return "CatalogPreviousVariant";
    case CreativeInputActionId::CatalogNextVariant:
      return "CatalogNextVariant";
    case CreativeInputActionId::CatalogPreviousPage:
      return "CatalogPreviousPage";
    case CreativeInputActionId::CatalogNextPage: return "CatalogNextPage";
    case CreativeInputActionId::CatalogConfirm: return "CatalogConfirm";
    case CreativeInputActionId::CatalogContextAction:
      return "CatalogContextAction";
    case CreativeInputActionId::CatalogClose: return "CatalogClose";
    case CreativeInputActionId::ToggleToolWheel: return "ToggleToolWheel";
    case CreativeInputActionId::ToolWheelPrevious: return "ToolWheelPrevious";
    case CreativeInputActionId::ToolWheelNext: return "ToolWheelNext";
    case CreativeInputActionId::ToolWheelConfirm: return "ToolWheelConfirm";
    case CreativeInputActionId::ToolWheelOptions: return "ToolWheelOptions";
    case CreativeInputActionId::ToolWheelClose: return "ToolWheelClose";
    case CreativeInputActionId::ToolOptionsPrevious:
      return "ToolOptionsPrevious";
    case CreativeInputActionId::ToolOptionsNext: return "ToolOptionsNext";
    case CreativeInputActionId::ToolOptionsDecrease:
      return "ToolOptionsDecrease";
    case CreativeInputActionId::ToolOptionsIncrease:
      return "ToolOptionsIncrease";
    case CreativeInputActionId::ToolOptionsConfirm:
      return "ToolOptionsConfirm";
    case CreativeInputActionId::ToolOptionsClose: return "ToolOptionsClose";
    case CreativeInputActionId::QuickEditPrevious:
      return "QuickEditPrevious";
    case CreativeInputActionId::QuickEditNext: return "QuickEditNext";
    case CreativeInputActionId::QuickEditDecrease:
      return "QuickEditDecrease";
    case CreativeInputActionId::QuickEditIncrease:
      return "QuickEditIncrease";
    case CreativeInputActionId::ToggleTransformControls:
      return "ToggleTransformControls";
    case CreativeInputActionId::TransformControlPrevious:
      return "TransformControlPrevious";
    case CreativeInputActionId::TransformControlNext:
      return "TransformControlNext";
    case CreativeInputActionId::TransformConstraintX:
      return "TransformConstraintX";
    case CreativeInputActionId::TransformConstraintY:
      return "TransformConstraintY";
    case CreativeInputActionId::TransformConstraintZ:
      return "TransformConstraintZ";
    case CreativeInputActionId::TransformNudgeNegative:
      return "TransformNudgeNegative";
    case CreativeInputActionId::TransformNudgePositive:
      return "TransformNudgePositive";
    case CreativeInputActionId::ConfirmActiveTool: return "ConfirmActiveTool";
    case CreativeInputActionId::CancelActiveTool: return "CancelActiveTool";
    case CreativeInputActionId::DeleteSelection: return "DeleteSelection";
    case CreativeInputActionId::Undo: return "Undo";
    case CreativeInputActionId::Redo: return "Redo";
    case CreativeInputActionId::CopySelection: return "CopySelection";
    case CreativeInputActionId::CutSelection: return "CutSelection";
    case CreativeInputActionId::PasteClipboard: return "PasteClipboard";
    case CreativeInputActionId::DuplicateSelection: return "DuplicateSelection";
    case CreativeInputActionId::RotateYawNegative: return "RotateYawNegative";
    case CreativeInputActionId::RotateYawPositive: return "RotateYawPositive";
    case CreativeInputActionId::ScaleDown: return "ScaleDown";
    case CreativeInputActionId::ScaleUp: return "ScaleUp";
    case CreativeInputActionId::Save: return "Save";
    case CreativeInputActionId::NewDocument: return "NewDocument";
    case CreativeInputActionId::Load: return "Load";
    case CreativeInputActionId::MoveForward: return "MoveForward";
    case CreativeInputActionId::MoveBackward: return "MoveBackward";
    case CreativeInputActionId::MoveLeft: return "MoveLeft";
    case CreativeInputActionId::MoveRight: return "MoveRight";
    case CreativeInputActionId::FlyUp: return "FlyUp";
    case CreativeInputActionId::FlyDown: return "FlyDown";
    case CreativeInputActionId::Sprint: return "Sprint";
    case CreativeInputActionId::RuntimeAttack: return "RuntimeAttack";
    case CreativeInputActionId::RuntimeInteract: return "RuntimeInteract";
    case CreativeInputActionId::PrimaryAction: return "PrimaryAction";
    case CreativeInputActionId::SecondaryAction: return "SecondaryAction";
    case CreativeInputActionId::AcceptAction: return "AcceptAction";
    case CreativeInputActionId::RejectAction: return "RejectAction";
    case CreativeInputActionId::PickAction: return "PickAction";
    case CreativeInputActionId::HotbarPrevious: return "HotbarPrevious";
    case CreativeInputActionId::HotbarNext: return "HotbarNext";
    case CreativeInputActionId::ToggleControls: return "ToggleControls";
    case CreativeInputActionId::ControlsPrevious: return "ControlsPrevious";
    case CreativeInputActionId::ControlsNext: return "ControlsNext";
    case CreativeInputActionId::ControlsDecrease: return "ControlsDecrease";
    case CreativeInputActionId::ControlsIncrease: return "ControlsIncrease";
    case CreativeInputActionId::ControlsActivate: return "ControlsActivate";
    case CreativeInputActionId::ControlsClose: return "ControlsClose";
    case CreativeInputActionId::ControlsResetDefaults:
      return "ControlsResetDefaults";
    case CreativeInputActionId::FrameContext3D: return "FrameContext3D";
    case CreativeInputActionId::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeInputContext context) noexcept {
  switch (context) {
    case CreativeInputContext::EditorViewport: return "EditorViewport";
    case CreativeInputContext::RuntimePlay: return "RuntimePlay";
    case CreativeInputContext::Catalog: return "Catalog";
    case CreativeInputContext::ToolWheel: return "ToolWheel";
    case CreativeInputContext::ToolOptions: return "ToolOptions";
    case CreativeInputContext::AssetReplacementPreview:
      return "AssetReplacementPreview";
    case CreativeInputContext::AssetLibrary: return "AssetLibrary";
    case CreativeInputContext::AuthoredAssetEditMenu:
      return "AuthoredAssetEditMenu";
    case CreativeInputContext::TransformPreview: return "TransformPreview";
    case CreativeInputContext::TransformControls: return "TransformControls";
    case CreativeInputContext::Controls: return "Controls";
    case CreativeInputContext::TextEntry: return "TextEntry";
    case CreativeInputContext::Modal: return "Modal";
    case CreativeInputContext::Capture: return "Capture";
    case CreativeInputContext::DesktopUi: return "DesktopUi";
    case CreativeInputContext::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeInputKey key) noexcept {
  switch (key) {
    case CreativeInputKey::Digit1: return "1";
    case CreativeInputKey::Digit2: return "2";
    case CreativeInputKey::Digit3: return "3";
    case CreativeInputKey::Digit4: return "4";
    case CreativeInputKey::Digit5: return "5";
    case CreativeInputKey::Digit6: return "6";
    case CreativeInputKey::Digit7: return "7";
    case CreativeInputKey::Digit8: return "8";
    case CreativeInputKey::Digit9: return "9";
    case CreativeInputKey::C: return "C";
    case CreativeInputKey::D: return "D";
    case CreativeInputKey::E: return "E";
    case CreativeInputKey::N: return "N";
    case CreativeInputKey::O: return "O";
    case CreativeInputKey::R: return "R";
    case CreativeInputKey::S: return "S";
    case CreativeInputKey::V: return "V";
    case CreativeInputKey::X: return "X";
    case CreativeInputKey::Y: return "Y";
    case CreativeInputKey::Z: return "Z";
    case CreativeInputKey::LeftBracket: return "LeftBracket";
    case CreativeInputKey::RightBracket: return "RightBracket";
    case CreativeInputKey::Minus: return "Minus";
    case CreativeInputKey::Equals: return "Equals";
    case CreativeInputKey::Delete: return "Delete";
    case CreativeInputKey::Backspace: return "Backspace";
    case CreativeInputKey::Enter: return "Enter";
    case CreativeInputKey::Escape: return "Escape";
    case CreativeInputKey::ArrowUp: return "ArrowUp";
    case CreativeInputKey::ArrowDown: return "ArrowDown";
    case CreativeInputKey::ArrowLeft: return "ArrowLeft";
    case CreativeInputKey::ArrowRight: return "ArrowRight";
    case CreativeInputKey::W: return "W";
    case CreativeInputKey::A: return "A";
    case CreativeInputKey::Space: return "Space";
    case CreativeInputKey::LeftControl: return "LeftControl";
    case CreativeInputKey::RightControl: return "RightControl";
    case CreativeInputKey::LeftShift: return "LeftShift";
    case CreativeInputKey::RightShift: return "RightShift";
    case CreativeInputKey::LeftAlt: return "LeftAlt";
    case CreativeInputKey::RightAlt: return "RightAlt";
    case CreativeInputKey::LeftCommand: return "LeftCommand";
    case CreativeInputKey::RightCommand: return "RightCommand";
    case CreativeInputKey::GamepadInventory: return "GamepadInventory";
    case CreativeInputKey::GamepadDpadUp: return "GamepadDpadUp";
    case CreativeInputKey::GamepadDpadDown: return "GamepadDpadDown";
    case CreativeInputKey::GamepadConfirm: return "GamepadConfirm";
    case CreativeInputKey::GamepadCancel: return "GamepadCancel";
    case CreativeInputKey::GamepadDpadLeft: return "GamepadDpadLeft";
    case CreativeInputKey::GamepadDpadRight: return "GamepadDpadRight";
    case CreativeInputKey::GamepadLeftShoulder: return "GamepadLeftShoulder";
    case CreativeInputKey::GamepadRightShoulder:
      return "GamepadRightShoulder";
    case CreativeInputKey::MousePrimary: return "MousePrimary";
    case CreativeInputKey::MouseSecondary: return "MouseSecondary";
    case CreativeInputKey::MouseMiddle: return "MouseMiddle";
    case CreativeInputKey::GamepadWest: return "GamepadWest";
    case CreativeInputKey::GamepadBack: return "GamepadBack";
    case CreativeInputKey::GamepadStart: return "GamepadStart";
    case CreativeInputKey::GamepadLeftStick: return "GamepadLeftStick";
    case CreativeInputKey::GamepadRightStick: return "GamepadRightStick";
    case CreativeInputKey::GamepadLeftTrigger: return "GamepadLeftTrigger";
    case CreativeInputKey::GamepadRightTrigger: return "GamepadRightTrigger";
    case CreativeInputKey::GamepadTouchpad: return "GamepadTouchpad";
    case CreativeInputKey::Unbound: return "Unbound";
    case CreativeInputKey::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeInputBindingConflictKind kind) noexcept {
  switch (kind) {
    case CreativeInputBindingConflictKind::DuplicateAction:
      return "DuplicateAction";
    case CreativeInputBindingConflictKind::PriorityShadow:
      return "PriorityShadow";
    case CreativeInputBindingConflictKind::AmbiguousPriority:
      return "AmbiguousPriority";
    case CreativeInputBindingConflictKind::ActivationShadow:
      return "ActivationShadow";
  }
  return "Unknown";
}

CreativeInputModifierMask preferredCommandModifier(
    CreativeInputPlatform platform) noexcept {
  return platform == CreativeInputPlatform::MacOS
             ? kCreativeInputModifierCommand
             : kCreativeInputModifierControl;
}

std::string creativeInputBindingLabel(const CreativeInputBinding& binding) {
  return binding.configurableLabel.empty()
             ? std::string(toString(binding.action))
             : std::string(binding.configurableLabel);
}

std::string creativeInputChordLabel(const CreativeInputBinding& binding,
                                    CreativeInputPlatform platform) {
  CreativeInputModifierMask displayedModifiers = binding.requiredAllModifiers;
  if (binding.requiredAnyModifiers != kCreativeInputModifierNone) {
    const CreativeInputModifierMask preferred =
        preferredCommandModifier(platform);
    displayedModifiers |= (binding.requiredAnyModifiers & preferred) != 0U
                              ? preferred
                              : firstModifier(binding.requiredAnyModifiers);
  }

  std::string label;
  appendNamedModifier(label, displayedModifiers, kCreativeInputModifierControl,
                      "Ctrl");
  appendNamedModifier(label, displayedModifiers, kCreativeInputModifierCommand,
                      "Cmd");
  appendNamedModifier(label, displayedModifiers, kCreativeInputModifierAlt,
                      "Alt");
  appendNamedModifier(label, displayedModifiers, kCreativeInputModifierShift,
                      "Shift");
  label.append(toString(binding.trigger));
  return label;
}

std::span<const CreativeControllerCommandChord>
defaultCreativeControllerCommandChords() noexcept {
  return kControllerCommandChords;
}

bool parseCreativeInputActionId(std::string_view value,
                                CreativeInputActionId& out) noexcept {
  // Keep pre-inspection control profiles readable after the semantic rename.
  if (value == "CatalogAssignToolWheel") {
    out = CreativeInputActionId::CatalogContextAction;
    return true;
  }
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(CreativeInputActionId::Count);
       ++index) {
    const CreativeInputActionId candidate =
        static_cast<CreativeInputActionId>(index);
    if (toString(candidate) == value) {
      out = candidate;
      return true;
    }
  }
  return false;
}

bool parseCreativeInputContext(std::string_view value,
                               CreativeInputContext& out) noexcept {
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(CreativeInputContext::Count);
       ++index) {
    const CreativeInputContext candidate =
        static_cast<CreativeInputContext>(index);
    if (toString(candidate) == value) {
      out = candidate;
      return true;
    }
  }
  return false;
}

bool parseCreativeInputKey(std::string_view value,
                           CreativeInputKey& out) noexcept {
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(CreativeInputKey::Count);
       ++index) {
    const CreativeInputKey candidate = static_cast<CreativeInputKey>(index);
    if (toString(candidate) == value) {
      out = candidate;
      return true;
    }
  }
  return false;
}

bool creativeInputKeyIsGamepad(CreativeInputKey key) noexcept {
  switch (key) {
    case CreativeInputKey::GamepadInventory:
    case CreativeInputKey::GamepadDpadUp:
    case CreativeInputKey::GamepadDpadDown:
    case CreativeInputKey::GamepadConfirm:
    case CreativeInputKey::GamepadCancel:
    case CreativeInputKey::GamepadDpadLeft:
    case CreativeInputKey::GamepadDpadRight:
    case CreativeInputKey::GamepadLeftShoulder:
    case CreativeInputKey::GamepadRightShoulder:
    case CreativeInputKey::GamepadWest:
    case CreativeInputKey::GamepadBack:
    case CreativeInputKey::GamepadStart:
    case CreativeInputKey::GamepadLeftStick:
    case CreativeInputKey::GamepadRightStick:
    case CreativeInputKey::GamepadLeftTrigger:
    case CreativeInputKey::GamepadRightTrigger:
    case CreativeInputKey::GamepadTouchpad:
      return true;
    default:
      return false;
  }
}

bool creativeInputKeyIsModifier(CreativeInputKey key) noexcept {
  return key == CreativeInputKey::LeftControl ||
         key == CreativeInputKey::RightControl ||
         key == CreativeInputKey::LeftShift ||
         key == CreativeInputKey::RightShift ||
         key == CreativeInputKey::LeftAlt ||
         key == CreativeInputKey::RightAlt ||
         key == CreativeInputKey::LeftCommand ||
         key == CreativeInputKey::RightCommand;
}

void setCreativeInputKey(CreativeInputFrame& frame,
                         CreativeInputKey key,
                         bool down) noexcept {
  if (key != CreativeInputKey::Unbound && key != CreativeInputKey::Count) {
    frame.keysDown[keyIndex(key)] = down;
  }
}

bool creativeInputKeyDown(const CreativeInputFrame& frame,
                          CreativeInputKey key) noexcept {
  return key != CreativeInputKey::Unbound && key != CreativeInputKey::Count &&
         frame.keysDown[keyIndex(key)];
}

bool creativeInputKeyConsumed(const CreativeInputRouteResult& result,
                              CreativeInputKey key) noexcept {
  return key != CreativeInputKey::Unbound && key != CreativeInputKey::Count &&
         result.consumedKeys[keyIndex(key)];
}

bool creativeInputRouteContains(const CreativeInputRouteResult& route,
                                CreativeInputActionId action) noexcept {
  for (std::size_t index = 0; index < route.actionCount; ++index) {
    if (route.actions[index].action == action) {
      return true;
    }
  }
  return false;
}

bool creativeInputActionDown(const CreativeInputRouteResult& route,
                             CreativeInputActionId action) noexcept {
  const std::size_t index = actionIndex(action);
  return index < route.down.size() && route.down[index];
}

bool creativeInputActionPressed(const CreativeInputRouteResult& route,
                                CreativeInputActionId action) noexcept {
  const std::size_t index = actionIndex(action);
  return index < route.pressed.size() && route.pressed[index];
}

bool creativeInputActionReleased(const CreativeInputRouteResult& route,
                                 CreativeInputActionId action) noexcept {
  const std::size_t index = actionIndex(action);
  return index < route.released.size() && route.released[index];
}

void creativeInputRouteRemove(CreativeInputRouteResult& route,
                              CreativeInputActionId action) noexcept {
  std::size_t write = 0;
  for (std::size_t read = 0; read < route.actionCount; ++read) {
    if (route.actions[read].action != action) {
      if (write != read) {
        route.actions[write] = route.actions[read];
      }
      ++write;
    }
  }
  route.actionCount = write;
  const std::size_t index = actionIndex(action);
  if (index < route.down.size()) {
    route.down[index] = false;
    route.pressed[index] = false;
    route.released[index] = false;
  }
}

CreativeInputRouteResult routeCreativeInput(
    CreativeInputRouterState& state,
    const CreativeInputFrame& frame,
    std::span<const CreativeInputBinding> bindings,
    std::span<const CreativeControllerCommandChord> controllerCommands) {
  CreativeInputRouteResult result;
  result.context = frame.context;
  result.bindingCapacityExceeded =
      bindings.size() > kCreativeInputBindingCapacity;

  const std::size_t bindingCount =
      std::min(bindings.size(), kCreativeInputBindingCapacity);
  std::array<std::size_t, kCreativeInputBindingCapacity> activeBindings{};
  std::array<bool, kCreativeInputActionCount> actionActivationEdge{};
  routeControllerCommandLayer(state, frame, result, actionActivationEdge,
                              controllerCommands);
  std::size_t activeBindingCount = 0;
  for (std::size_t index = 0; index < bindingCount; ++index) {
    if (bindings[index].context == frame.context &&
        physicalChordActive(frame, bindings[index])) {
      activeBindings[activeBindingCount++] = index;
    }
  }
  std::stable_sort(activeBindings.begin(),
                   activeBindings.begin() + activeBindingCount,
                   [bindings](std::size_t lhs, std::size_t rhs) {
                     return bindings[lhs].priority > bindings[rhs].priority;
                   });

  for (std::size_t activeIndex = 0; activeIndex < activeBindingCount;
       ++activeIndex) {
    const std::size_t index = activeBindings[activeIndex];
    const CreativeInputBinding& binding = bindings[index];
    if (creativeInputKeyConsumed(result, binding.trigger)) {
      continue;
    }
    if (binding.consumePolicy != CreativeInputConsumePolicy::PassThrough) {
      consumeKey(result, binding.trigger);
    }
    if (binding.consumePolicy == CreativeInputConsumePolicy::ConsumeChord) {
      consumeModifierKeys(result, frame);
    }
    const std::size_t action = actionIndex(binding.action);
    if (action < result.down.size()) {
      result.down[action] = true;
      actionActivationEdge[action] =
          actionActivationEdge[action] || !state.bindingActive[index];
    }
    if (binding.activation == CreativeInputBindingActivation::Press &&
        !state.bindingActive[index] &&
        !actionAlreadyEmitted(result, binding.action)) {
      result.actions[result.actionCount++] = {binding.action, binding.trigger};
    }
  }

  for (std::size_t index = 0; index < result.down.size(); ++index) {
    result.pressed[index] = result.down[index] && !state.actionDown[index] &&
                            actionActivationEdge[index];
    result.released[index] = !result.down[index] && state.actionDown[index];
  }
  state.actionDown = result.down;
  for (std::size_t index = 0; index < bindingCount; ++index) {
    state.bindingActive[index] = physicalChordActive(frame, bindings[index]);
  }
  for (std::size_t index = bindingCount;
       index < state.bindingActive.size(); ++index) {
    state.bindingActive[index] = false;
  }
  state.previousKeysDown = frame.keysDown;
  return result;
}

CreativeInputBindingAuditResult auditCreativeInputBindings(
    std::span<const CreativeInputBinding> bindings) noexcept {
  CreativeInputBindingAuditResult result;
  result.bindingCapacityExceeded =
      bindings.size() > kCreativeInputBindingCapacity;
  const std::size_t bindingCount =
      std::min(bindings.size(), kCreativeInputBindingCapacity);

  for (std::size_t firstIndex = 0; firstIndex < bindingCount; ++firstIndex) {
    const CreativeInputBinding& first = bindings[firstIndex];
    for (std::size_t secondIndex = firstIndex + 1U;
         secondIndex < bindingCount; ++secondIndex) {
      const CreativeInputBinding& second = bindings[secondIndex];
      if (first.trigger == CreativeInputKey::Unbound ||
          second.trigger == CreativeInputKey::Unbound ||
          first.context != second.context || first.trigger != second.trigger) {
        continue;
      }

      CreativeInputModifierMask overlappingModifiers =
          kCreativeInputModifierNone;
      if (!modifierDomainsOverlap(first, second, overlappingModifiers)) {
        continue;
      }
      const bool activationShadow =
          first.activation != second.activation &&
          overlappingModifiers == kCreativeInputModifierNone &&
          (first.consumePolicy != CreativeInputConsumePolicy::PassThrough ||
           second.consumePolicy != CreativeInputConsumePolicy::PassThrough);
      if (first.activation != second.activation && !activationShadow) {
        continue;
      }

      CreativeInputBindingConflict conflict;
      conflict.firstBindingIndex = firstIndex;
      conflict.secondBindingIndex = secondIndex;
      conflict.firstAction = first.action;
      conflict.secondAction = second.action;
      conflict.context = first.context;
      conflict.trigger = first.trigger;
      conflict.overlappingModifiers = overlappingModifiers;
      if (activationShadow) {
        conflict.kind = CreativeInputBindingConflictKind::ActivationShadow;
      } else if (first.action == second.action) {
        conflict.kind = CreativeInputBindingConflictKind::DuplicateAction;
      } else if (first.priority != second.priority) {
        conflict.kind = CreativeInputBindingConflictKind::PriorityShadow;
      } else {
        conflict.kind = CreativeInputBindingConflictKind::AmbiguousPriority;
      }
      if (second.priority > first.priority ||
          (second.priority == first.priority &&
           first.consumePolicy == CreativeInputConsumePolicy::PassThrough &&
           second.consumePolicy != CreativeInputConsumePolicy::PassThrough)) {
        conflict.winningBindingIndex = secondIndex;
      } else {
        conflict.winningBindingIndex = firstIndex;
      }
      result.conflicts[result.conflictCount++] = conflict;
    }
  }
  return result;
}

}  // namespace iggy3d::creative
