#include "app/iggy3d/creative/input/InputRouter.hpp"

#include <algorithm>
#include <array>

namespace iggy3d::creative {
namespace {

constexpr std::uint16_t kCommandPriority = 300;
constexpr std::uint16_t kDestructivePriority = 250;
constexpr std::uint16_t kTransformPriority = 220;
constexpr std::uint16_t kToolPriority = 200;
constexpr std::uint16_t kFilePriority = 180;

constexpr CreativeInputModifierMask kCommandModifiers =
    kCreativeInputModifierControl | kCreativeInputModifierCommand;

constexpr std::array kDefaultBindings{
    CreativeInputBinding{CreativeInputActionId::Undo,
                         CreativeInputKey::Z,
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
    CreativeInputBinding{CreativeInputActionId::SelectTool,
                         CreativeInputKey::Digit1,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kToolPriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::MoveTool,
                         CreativeInputKey::Digit2,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kToolPriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::EnterPlaceMode,
                         CreativeInputKey::Digit3,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kToolPriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::CycleBrush,
                         CreativeInputKey::B,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kToolPriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::Save,
                         CreativeInputKey::F5,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kFilePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::NewDocument,
                         CreativeInputKey::F6,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kFilePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
    CreativeInputBinding{CreativeInputActionId::Load,
                         CreativeInputKey::F9,
                         CreativeInputContext::EditorViewport,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kCreativeInputModifierNone,
                         kFilePriority,
                         CreativeInputConsumePolicy::ConsumeTrigger},
};

static_assert(kDefaultBindings.size() <= kCreativeInputBindingCapacity);

[[nodiscard]] std::size_t keyIndex(CreativeInputKey key) noexcept {
  return static_cast<std::size_t>(key);
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

}  // namespace

std::string_view toString(CreativeInputActionId action) noexcept {
  switch (action) {
    case CreativeInputActionId::SelectTool: return "SelectTool";
    case CreativeInputActionId::MoveTool: return "MoveTool";
    case CreativeInputActionId::EnterPlaceMode: return "EnterPlaceMode";
    case CreativeInputActionId::CycleBrush: return "CycleBrush";
    case CreativeInputActionId::DeleteSelection: return "DeleteSelection";
    case CreativeInputActionId::Undo: return "Undo";
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
    case CreativeInputKey::B: return "B";
    case CreativeInputKey::D: return "D";
    case CreativeInputKey::Z: return "Z";
    case CreativeInputKey::LeftBracket: return "LeftBracket";
    case CreativeInputKey::RightBracket: return "RightBracket";
    case CreativeInputKey::Minus: return "Minus";
    case CreativeInputKey::Equals: return "Equals";
    case CreativeInputKey::Delete: return "Delete";
    case CreativeInputKey::Backspace: return "Backspace";
    case CreativeInputKey::F5: return "F5";
    case CreativeInputKey::F6: return "F6";
    case CreativeInputKey::F9: return "F9";
    case CreativeInputKey::W: return "W";
    case CreativeInputKey::A: return "A";
    case CreativeInputKey::S: return "S";
    case CreativeInputKey::Space: return "Space";
    case CreativeInputKey::LeftControl: return "LeftControl";
    case CreativeInputKey::RightControl: return "RightControl";
    case CreativeInputKey::LeftShift: return "LeftShift";
    case CreativeInputKey::RightShift: return "RightShift";
    case CreativeInputKey::LeftAlt: return "LeftAlt";
    case CreativeInputKey::RightAlt: return "RightAlt";
    case CreativeInputKey::LeftCommand: return "LeftCommand";
    case CreativeInputKey::RightCommand: return "RightCommand";
    case CreativeInputKey::Count: break;
  }
  return "Unknown";
}

std::span<const CreativeInputBinding> defaultCreativeInputBindings() noexcept {
  return kDefaultBindings;
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

}  // namespace iggy3d::creative
