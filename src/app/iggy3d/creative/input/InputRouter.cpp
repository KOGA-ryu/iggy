#include "app/iggy3d/creative/input/InputRouter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace iggy3d::creative {
namespace {

constexpr std::uint16_t kCommandPriority = 300;
constexpr std::uint16_t kDestructivePriority = 250;
constexpr std::uint16_t kTransformPriority = 220;
constexpr std::uint16_t kToolPriority = 200;
constexpr std::uint16_t kFilePriority = 180;

constexpr CreativeInputModifierMask kCommandModifiers =
    kCreativeInputModifierControl | kCreativeInputModifierCommand;
constexpr CreativeInputModifierMask kAllModifiers =
    kCreativeInputModifierShift | kCreativeInputModifierControl |
    kCreativeInputModifierAlt | kCreativeInputModifierCommand;

constexpr CreativeInputBinding hotbarBinding(CreativeInputActionId action,
                                              CreativeInputKey key,
                                              CreativeInputContext context =
                                                  CreativeInputContext::
                                                      EditorViewport) noexcept {
  return {action,
          key,
          context,
          kCreativeInputModifierNone,
          kCreativeInputModifierNone,
          kAllModifiers,
          kToolPriority,
          CreativeInputConsumePolicy::ConsumeTrigger};
}

constexpr CreativeInputBinding catalogBinding(
    CreativeInputActionId action,
    CreativeInputKey key,
    CreativeInputContext context) noexcept {
  return {action,
          key,
          context,
          kCreativeInputModifierNone,
          kCreativeInputModifierNone,
          kAllModifiers,
          kToolPriority,
          CreativeInputConsumePolicy::ConsumeTrigger};
}

constexpr CreativeInputBinding toolWheelToggleBinding(
    CreativeInputKey key,
    CreativeInputContext context,
    CreativeInputModifierMask allowedModifiers) noexcept {
  return {CreativeInputActionId::ToggleToolWheel,
          key,
          context,
          kCreativeInputModifierNone,
          kCreativeInputModifierNone,
          allowedModifiers,
          kToolPriority,
          CreativeInputConsumePolicy::ConsumeTrigger};
}

constexpr std::array kDefaultBindings{
    CreativeInputBinding{CreativeInputActionId::Undo,
                         CreativeInputKey::Z,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kCommandPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::Redo,
                         CreativeInputKey::Z,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierShift,
                         kCommandModifiers,
                         static_cast<CreativeInputModifierMask>(
                             kCreativeInputModifierShift | kCommandModifiers),
                         kCommandPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::CopySelection,
                         CreativeInputKey::C,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kCommandPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::CutSelection,
                         CreativeInputKey::X,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kCommandPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::PasteClipboard,
                         CreativeInputKey::V,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kCommandPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::DuplicateSelection,
                         CreativeInputKey::D,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kCommandPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::DeleteSelection,
                         CreativeInputKey::Delete,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kDestructivePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::DeleteSelection,
                         CreativeInputKey::Backspace,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kDestructivePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::RotateYawNegative,
                         CreativeInputKey::LeftBracket,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierShift,
                         kTransformPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::RotateYawPositive,
                         CreativeInputKey::RightBracket,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierShift,
                         kTransformPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::ScaleDown,
                         CreativeInputKey::Minus,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierShift,
                         kTransformPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::ScaleUp,
                         CreativeInputKey::Equals,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierShift,
                         kTransformPriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::ConfirmActiveTool,
                         CreativeInputKey::Enter,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kDestructivePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::CancelActiveTool,
                         CreativeInputKey::Escape,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kDestructivePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    catalogBinding(CreativeInputActionId::ConfirmActiveTool,
                   CreativeInputKey::Enter,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::ConfirmActiveTool,
                   CreativeInputKey::GamepadConfirm,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::CancelActiveTool,
                   CreativeInputKey::Escape,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::CancelActiveTool,
                   CreativeInputKey::GamepadCancel,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::CancelActiveTool,
                   CreativeInputKey::Delete,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::CancelActiveTool,
                   CreativeInputKey::Backspace,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::ToggleTransformControls,
                   CreativeInputKey::R,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::ToggleTransformControls,
                   CreativeInputKey::GamepadDpadRight,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::TransformControlPrevious,
                   CreativeInputKey::ArrowLeft,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::TransformControlPrevious,
                   CreativeInputKey::GamepadDpadUp,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::TransformControlNext,
                   CreativeInputKey::ArrowRight,
                   CreativeInputContext::TransformPreview),
    catalogBinding(CreativeInputActionId::TransformControlNext,
                   CreativeInputKey::GamepadDpadDown,
                   CreativeInputContext::TransformPreview),
    hotbarBinding(CreativeInputActionId::HotbarSlot1,
                  CreativeInputKey::Digit1),
    hotbarBinding(CreativeInputActionId::HotbarSlot2,
                  CreativeInputKey::Digit2),
    hotbarBinding(CreativeInputActionId::HotbarSlot3,
                  CreativeInputKey::Digit3),
    hotbarBinding(CreativeInputActionId::HotbarSlot4,
                  CreativeInputKey::Digit4),
    hotbarBinding(CreativeInputActionId::HotbarSlot5,
                  CreativeInputKey::Digit5),
    hotbarBinding(CreativeInputActionId::HotbarSlot6,
                  CreativeInputKey::Digit6),
    hotbarBinding(CreativeInputActionId::HotbarSlot7,
                  CreativeInputKey::Digit7),
    hotbarBinding(CreativeInputActionId::HotbarSlot8,
                  CreativeInputKey::Digit8),
    hotbarBinding(CreativeInputActionId::HotbarSlot9,
                  CreativeInputKey::Digit9),
    CreativeInputBinding{CreativeInputActionId::Save,
                         CreativeInputKey::S,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kFilePriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::NewDocument,
                         CreativeInputKey::N,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kFilePriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    CreativeInputBinding{CreativeInputActionId::Load,
                         CreativeInputKey::O,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCommandModifiers,
                         kCommandModifiers,
                         kFilePriority,
                         CreativeInputConsumePolicy::ConsumeChord},
    catalogBinding(CreativeInputActionId::ToggleCatalog, CreativeInputKey::E,
                   CreativeInputContext::EditorViewport),
    catalogBinding(CreativeInputActionId::ToggleCatalog, CreativeInputKey::E,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::ToggleCatalog,
                   CreativeInputKey::GamepadInventory,
                   CreativeInputContext::EditorViewport),
    catalogBinding(CreativeInputActionId::ToggleCatalog,
                   CreativeInputKey::GamepadInventory,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogPrevious,
                   CreativeInputKey::ArrowUp,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogPrevious,
                   CreativeInputKey::GamepadDpadUp,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogNext,
                   CreativeInputKey::ArrowDown,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogNext,
                   CreativeInputKey::GamepadDpadDown,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogPreviousVariant,
                   CreativeInputKey::ArrowLeft,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogPreviousVariant,
                   CreativeInputKey::GamepadDpadLeft,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogNextVariant,
                   CreativeInputKey::ArrowRight,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogNextVariant,
                   CreativeInputKey::GamepadDpadRight,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogPreviousPage,
                   CreativeInputKey::LeftBracket,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogPreviousPage,
                   CreativeInputKey::GamepadLeftShoulder,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogNextPage,
                   CreativeInputKey::RightBracket,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogNextPage,
                   CreativeInputKey::GamepadRightShoulder,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogConfirm,
                   CreativeInputKey::Enter,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogConfirm,
                   CreativeInputKey::GamepadConfirm,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogClose,
                   CreativeInputKey::Escape,
                   CreativeInputContext::Catalog),
    catalogBinding(CreativeInputActionId::CatalogClose,
                   CreativeInputKey::GamepadCancel,
                   CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot1,
                  CreativeInputKey::Digit1, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot2,
                  CreativeInputKey::Digit2, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot3,
                  CreativeInputKey::Digit3, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot4,
                  CreativeInputKey::Digit4, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot5,
                  CreativeInputKey::Digit5, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot6,
                  CreativeInputKey::Digit6, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot7,
                  CreativeInputKey::Digit7, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot8,
                  CreativeInputKey::Digit8, CreativeInputContext::Catalog),
    hotbarBinding(CreativeInputActionId::HotbarSlot9,
                  CreativeInputKey::Digit9, CreativeInputContext::Catalog),
    toolWheelToggleBinding(CreativeInputKey::R,
                           CreativeInputContext::EditorViewport,
                           kCreativeInputModifierShift),
    toolWheelToggleBinding(CreativeInputKey::R,
                           CreativeInputContext::ToolWheel,
                           kCreativeInputModifierShift),
    toolWheelToggleBinding(CreativeInputKey::GamepadDpadRight,
                           CreativeInputContext::EditorViewport,
                           kAllModifiers),
    toolWheelToggleBinding(CreativeInputKey::GamepadDpadRight,
                           CreativeInputContext::ToolWheel,
                           kAllModifiers),
    catalogBinding(CreativeInputActionId::ToolWheelPrevious,
                   CreativeInputKey::ArrowLeft,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelPrevious,
                   CreativeInputKey::GamepadDpadUp,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelNext,
                   CreativeInputKey::ArrowRight,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelNext,
                   CreativeInputKey::GamepadDpadDown,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelConfirm,
                   CreativeInputKey::Enter,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelConfirm,
                   CreativeInputKey::GamepadConfirm,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelClose,
                   CreativeInputKey::Escape,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolWheelClose,
                   CreativeInputKey::GamepadCancel,
                   CreativeInputContext::ToolWheel),
    catalogBinding(CreativeInputActionId::ToolOptionsPrevious,
                   CreativeInputKey::ArrowUp,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsPrevious,
                   CreativeInputKey::GamepadDpadUp,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsNext,
                   CreativeInputKey::ArrowDown,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsNext,
                   CreativeInputKey::GamepadDpadDown,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsDecrease,
                   CreativeInputKey::ArrowLeft,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsDecrease,
                   CreativeInputKey::GamepadDpadLeft,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsIncrease,
                   CreativeInputKey::ArrowRight,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsIncrease,
                   CreativeInputKey::GamepadDpadRight,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsConfirm,
                   CreativeInputKey::Enter,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsConfirm,
                   CreativeInputKey::GamepadConfirm,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsClose,
                   CreativeInputKey::Escape,
                   CreativeInputContext::ToolOptions),
    catalogBinding(CreativeInputActionId::ToolOptionsClose,
                   CreativeInputKey::GamepadCancel,
                   CreativeInputContext::ToolOptions),
};

static_assert(kDefaultBindings.size() <= kCreativeInputBindingCapacity);

[[nodiscard]] std::size_t keyIndex(CreativeInputKey key) noexcept {
  return static_cast<std::size_t>(key);
}

[[nodiscard]] std::size_t controllerAxisIndex(
    CreativeControllerAxis axis) noexcept {
  return static_cast<std::size_t>(axis);
}

[[nodiscard]] std::size_t controllerButtonIndex(
    CreativeControllerButton button) noexcept {
  return static_cast<std::size_t>(button);
}

[[nodiscard]] float finiteClamped(float value,
                                  float minimum,
                                  float maximum) noexcept {
  return std::isfinite(value) ? std::clamp(value, minimum, maximum) : 0.0F;
}

[[nodiscard]] float applyControllerStickDeadzone(float value) noexcept {
  value = finiteClamped(value, -1.0F, 1.0F);
  const float magnitude = std::fabs(value);
  if (magnitude <= kCreativeControllerStickDeadzone) {
    return 0.0F;
  }
  const float scaled = (magnitude - kCreativeControllerStickDeadzone) /
                       (1.0F - kCreativeControllerStickDeadzone);
  return std::copysign(std::clamp(scaled, 0.0F, 1.0F), value);
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
  return creativeInputKeyDown(frame, binding.trigger) &&
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
    case CreativeInputActionId::CatalogClose: return "CatalogClose";
    case CreativeInputActionId::ToggleToolWheel: return "ToggleToolWheel";
    case CreativeInputActionId::ToolWheelPrevious: return "ToolWheelPrevious";
    case CreativeInputActionId::ToolWheelNext: return "ToolWheelNext";
    case CreativeInputActionId::ToolWheelConfirm: return "ToolWheelConfirm";
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
    case CreativeInputActionId::ToggleTransformControls:
      return "ToggleTransformControls";
    case CreativeInputActionId::TransformControlPrevious:
      return "TransformControlPrevious";
    case CreativeInputActionId::TransformControlNext:
      return "TransformControlNext";
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

std::span<const CreativeInputBinding> defaultCreativeInputBindings() noexcept {
  return kDefaultBindings;
}

void setCreativeControllerAxis(CreativeControllerSample& sample,
                               CreativeControllerAxis axis,
                               float value) noexcept {
  if (axis != CreativeControllerAxis::Count) {
    sample.axes[controllerAxisIndex(axis)] = value;
  }
}

void setCreativeControllerButton(CreativeControllerSample& sample,
                                 CreativeControllerButton button,
                                 bool down) noexcept {
  if (button != CreativeControllerButton::Count) {
    sample.buttonsDown[controllerButtonIndex(button)] = down;
  }
}

float creativeControllerAxis(const CreativeControllerFrame& frame,
                             CreativeControllerAxis axis) noexcept {
  return axis == CreativeControllerAxis::Count
             ? 0.0F
             : frame.axes[controllerAxisIndex(axis)];
}

bool creativeControllerButtonDown(const CreativeControllerFrame& frame,
                                  CreativeControllerButton button) noexcept {
  return button != CreativeControllerButton::Count &&
         frame.next.buttonsDown[controllerButtonIndex(button)];
}

bool creativeControllerButtonPressed(const CreativeControllerFrame& frame,
                                     CreativeControllerButton button) noexcept {
  return button != CreativeControllerButton::Count &&
         frame.pressed[controllerButtonIndex(button)];
}

bool creativeControllerButtonReleased(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept {
  return button != CreativeControllerButton::Count &&
         frame.released[controllerButtonIndex(button)];
}

CreativeControllerFrame stepCreativeControllerInput(
    CreativeControllerState previous,
    const CreativeControllerSample& current) noexcept {
  CreativeControllerFrame frame;
  frame.next.connected = current.connected;
  if (current.connected) {
    constexpr std::array stickAxes{
        CreativeControllerAxis::MoveX,
        CreativeControllerAxis::MoveY,
        CreativeControllerAxis::LookX,
        CreativeControllerAxis::LookY,
    };
    for (CreativeControllerAxis axis : stickAxes) {
      const std::size_t index = controllerAxisIndex(axis);
      frame.axes[index] = applyControllerStickDeadzone(current.axes[index]);
    }

    constexpr std::array triggerAxes{
        CreativeControllerAxis::LeftTrigger,
        CreativeControllerAxis::RightTrigger,
    };
    for (CreativeControllerAxis axis : triggerAxes) {
      const std::size_t index = controllerAxisIndex(axis);
      frame.axes[index] = finiteClamped(current.axes[index], 0.0F, 1.0F);
    }
    frame.next.buttonsDown = current.buttonsDown;
    frame.next.buttonsDown[controllerButtonIndex(
        CreativeControllerButton::LeftTrigger)] =
        creativeControllerAxis(frame, CreativeControllerAxis::LeftTrigger) >=
        kCreativeControllerTriggerThreshold;
    frame.next.buttonsDown[controllerButtonIndex(
        CreativeControllerButton::RightTrigger)] =
        creativeControllerAxis(frame, CreativeControllerAxis::RightTrigger) >=
        kCreativeControllerTriggerThreshold;
  }

  for (std::size_t index = 0; index < kCreativeControllerButtonCount; ++index) {
    frame.pressed[index] =
        frame.next.buttonsDown[index] && !previous.buttonsDown[index];
    frame.released[index] =
        !frame.next.buttonsDown[index] && previous.buttonsDown[index];
  }
  return frame;
}

void setCreativeInputKey(CreativeInputFrame& frame,
                         CreativeInputKey key,
                         bool down) noexcept {
  if (key != CreativeInputKey::Count) {
    frame.keysDown[keyIndex(key)] = down;
  }
}

bool creativeInputKeyDown(const CreativeInputFrame& frame,
                          CreativeInputKey key) noexcept {
  return key != CreativeInputKey::Count && frame.keysDown[keyIndex(key)];
}

bool creativeInputKeyConsumed(const CreativeInputRouteResult& result,
                              CreativeInputKey key) noexcept {
  return key != CreativeInputKey::Count && result.consumedKeys[keyIndex(key)];
}

CreativeInputRouteResult routeCreativeInput(
    CreativeInputRouterState& state,
    const CreativeInputFrame& frame,
    std::span<const CreativeInputBinding> bindings) {
  CreativeInputRouteResult result;
  result.context = frame.context;
  result.bindingCapacityExceeded =
      bindings.size() > kCreativeInputBindingCapacity;

  const std::size_t bindingCount =
      std::min(bindings.size(), kCreativeInputBindingCapacity);
  std::array<std::size_t, kCreativeInputBindingCapacity> activeBindings{};
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
    if (!state.bindingActive[index] &&
        !actionAlreadyEmitted(result, binding.action)) {
      result.actions[result.actionCount++] = {binding.action, binding.trigger};
    }
  }

  for (std::size_t index = 0; index < bindingCount; ++index) {
    state.bindingActive[index] = physicalChordActive(frame, bindings[index]);
  }
  for (std::size_t index = bindingCount;
       index < state.bindingActive.size(); ++index) {
    state.bindingActive[index] = false;
  }
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
      if (first.context != second.context || first.trigger != second.trigger) {
        continue;
      }

      CreativeInputModifierMask overlappingModifiers =
          kCreativeInputModifierNone;
      if (!modifierDomainsOverlap(first, second, overlappingModifiers)) {
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
      if (first.action == second.action) {
        conflict.kind = CreativeInputBindingConflictKind::DuplicateAction;
      } else if (first.priority != second.priority) {
        conflict.kind = CreativeInputBindingConflictKind::PriorityShadow;
      } else {
        conflict.kind = CreativeInputBindingConflictKind::AmbiguousPriority;
      }
      conflict.winningBindingIndex =
          second.priority > first.priority ? secondIndex : firstIndex;
      result.conflicts[result.conflictCount++] = conflict;
    }
  }
  return result;
}

}  // namespace iggy3d::creative
