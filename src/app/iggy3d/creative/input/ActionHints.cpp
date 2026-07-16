#include "app/iggy3d/creative/input/ActionHints.hpp"

#include <type_traits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validContext(CreativeInputContext context) noexcept {
  return static_cast<std::size_t>(context) <
         static_cast<std::size_t>(CreativeInputContext::Count);
}

[[nodiscard]] bool validDevice(CreativeControlDevice device) noexcept {
  return static_cast<std::size_t>(device) <
         static_cast<std::size_t>(CreativeControlDevice::Count);
}

[[nodiscard]] bool appendText(CreativeActionHintFixedText& output,
                              std::string_view text) noexcept {
  const std::size_t available = output.bytes.size() - output.length;
  const std::size_t copied = text.size() < available ? text.size() : available;
  for (std::size_t index = 0U; index < copied; ++index) {
    output.bytes[output.length + index] = text[index];
  }
  output.length = static_cast<std::uint8_t>(output.length + copied);
  return copied == text.size();
}

[[nodiscard]] const CreativeInputBinding* findBinding(
    const CreativeControlProfile& profile,
    CreativeInputActionId action,
    CreativeInputContext context,
    CreativeControlDevice device,
    std::uint16_t preferredGroupOrdinal) noexcept {
  const CreativeInputBinding* fallback = nullptr;
  for (std::size_t index = 0U; index < profile.bindingCount; ++index) {
    const CreativeInputBinding& binding = profile.bindings[index];
    if (binding.action != action || binding.context != context ||
        binding.trigger == CreativeInputKey::Unbound) {
      continue;
    }
    const bool gamepad = creativeInputKeyIsGamepad(binding.trigger);
    if ((device == CreativeControlDevice::Gamepad) == gamepad) {
      fallback = fallback == nullptr ? &binding : fallback;
      const std::uint16_t group = profile.bindingGroups[index];
      if (preferredGroupOrdinal != kCreativeActionHintAnyGroupOrdinal &&
          group < profile.groupCount &&
          profile.groupOrdinals[group] == preferredGroupOrdinal) {
        return &binding;
      }
      if (preferredGroupOrdinal == kCreativeActionHintAnyGroupOrdinal &&
          action == CreativeInputActionId::QuickEditNext && gamepad &&
          group < profile.groupCount &&
          profile.groupOrdinals[group] == 1U) {
        return &binding;
      }
    }
  }
  return preferredGroupOrdinal == kCreativeActionHintAnyGroupOrdinal
             ? fallback
             : nullptr;
}

[[nodiscard]] bool appendModifierChord(
    CreativeActionHintFixedText& output,
    CreativeInputModifierMask modifiers,
    CreativeInputModifierMask flag,
    std::string_view label) noexcept {
  if ((modifiers & flag) == 0U) {
    return true;
  }
  return appendText(output, label) && appendText(output, "+");
}

[[nodiscard]] CreativeInputModifierMask resolvedModifiers(
    const CreativeInputBinding& binding,
    CreativeInputPlatform platform) noexcept {
  CreativeInputModifierMask modifiers = binding.requiredAllModifiers;
  if (binding.requiredAnyModifiers != kCreativeInputModifierNone) {
    const CreativeInputModifierMask preferred =
        preferredCommandModifier(platform);
    modifiers |= (binding.requiredAnyModifiers & preferred) != 0U
                     ? preferred
                     : binding.requiredAnyModifiers;
  }
  return modifiers;
}

[[nodiscard]] bool appendModifiers(CreativeActionHintFixedText& output,
                                   CreativeInputModifierMask modifiers) noexcept {
  return appendModifierChord(output, modifiers, kCreativeInputModifierControl,
                             "Ctrl") &&
         appendModifierChord(output, modifiers, kCreativeInputModifierCommand,
                             "Cmd") &&
         appendModifierChord(output, modifiers, kCreativeInputModifierAlt,
                             "Alt") &&
         appendModifierChord(output, modifiers, kCreativeInputModifierShift,
                             "Shift");
}

[[nodiscard]] std::string_view compactPairLabel(
    CreativeInputKey first,
    CreativeInputKey second) noexcept {
  struct PairLabel {
    CreativeInputKey first;
    CreativeInputKey second;
    std::string_view label;
  };
  constexpr std::array rows{
      PairLabel{CreativeInputKey::GamepadDpadUp,
                CreativeInputKey::GamepadDpadDown, "D-pad U/D"},
      PairLabel{CreativeInputKey::GamepadDpadLeft,
                CreativeInputKey::GamepadDpadRight, "D-pad L/R"},
      PairLabel{CreativeInputKey::ArrowUp, CreativeInputKey::ArrowDown,
                "Up/Down"},
      PairLabel{CreativeInputKey::ArrowLeft, CreativeInputKey::ArrowRight,
                "Left/Right"},
      PairLabel{CreativeInputKey::GamepadLeftShoulder,
                CreativeInputKey::GamepadRightShoulder, "L1/R1"},
      PairLabel{CreativeInputKey::GamepadLeftTrigger,
                CreativeInputKey::GamepadRightTrigger, "L2/R2"},
      PairLabel{CreativeInputKey::LeftShift, CreativeInputKey::Space,
                "Shift/Space"},
      PairLabel{CreativeInputKey::RightShift, CreativeInputKey::Space,
                "Shift/Space"},
      PairLabel{CreativeInputKey::LeftBracket,
                CreativeInputKey::RightBracket, "[/]"},
  };
  for (const PairLabel& row : rows) {
    if (row.first == first && row.second == second) {
      return row.label;
    }
  }
  return {};
}

[[nodiscard]] std::string_view compactKeyLabel(CreativeInputKey key) noexcept {
  switch (key) {
    case CreativeInputKey::MousePrimary: return "Mouse L";
    case CreativeInputKey::MouseSecondary: return "Mouse R";
    case CreativeInputKey::MouseMiddle: return "Mouse M";
    case CreativeInputKey::ArrowUp: return "Up";
    case CreativeInputKey::ArrowDown: return "Down";
    case CreativeInputKey::ArrowLeft: return "Left";
    case CreativeInputKey::ArrowRight: return "Right";
    default: return creativeControlKeyDisplayLabelView(key);
  }
}

[[nodiscard]] bool appendBindingChord(
    CreativeActionHintFixedText& output,
    const CreativeInputBinding& binding,
    CreativeInputPlatform platform) noexcept {
  return appendModifiers(output, resolvedModifiers(binding, platform)) &&
         appendText(output, compactKeyLabel(binding.trigger));
}

}  // namespace

static_assert(std::is_trivially_copyable_v<CreativeActionHintFixedText>);
static_assert(std::is_trivially_copyable_v<CreativeActionHint>);
static_assert(std::is_trivially_copyable_v<CreativeActionHintFrame>);

CreativeControlDeviceActivity measureCreativeControlDeviceActivity(
    const CreativeInputFrame& frame,
    bool pointerActivity,
    bool gamepadAnalogActivity) noexcept {
  CreativeControlDeviceActivity activity;
  activity.keyboardMouse = pointerActivity;
  activity.gamepad = gamepadAnalogActivity;
  for (std::size_t index = 0U; index < frame.keysDown.size(); ++index) {
    if (!frame.keysDown[index]) {
      continue;
    }
    const CreativeInputKey key = static_cast<CreativeInputKey>(index);
    if (creativeInputKeyIsGamepad(key)) {
      activity.gamepad = true;
    } else if (key != CreativeInputKey::Unbound &&
               key != CreativeInputKey::Count) {
      activity.keyboardMouse = true;
    }
  }
  return activity;
}

CreativeControlDevice resolveCreativeActiveControlDevice(
    CreativeControlDevice previous,
    CreativeControlDeviceActivity activity) noexcept {
  if (activity.keyboardMouse != activity.gamepad) {
    return activity.gamepad ? CreativeControlDevice::Gamepad
                            : CreativeControlDevice::KeyboardMouse;
  }
  return validDevice(previous) ? previous
                               : CreativeControlDevice::KeyboardMouse;
}

CreativeActionHintFrame resolveCreativeActionHints(
    const CreativeControlProfile& profile,
    CreativeInputContext context,
    CreativeControlDevice device,
    CreativeInputPlatform platform,
    std::span<const CreativeActionHintSpec> specs) noexcept {
  CreativeActionHintFrame frame;
  if (!validContext(context) || !validDevice(device) ||
      profile.bindingCount > profile.bindings.size()) {
    frame.invalidInput = true;
    return frame;
  }
  if (specs.size() > frame.hints.size()) {
    frame.capacityExceeded = true;
    return frame;
  }

  for (const CreativeActionHintSpec& spec : specs) {
    if (spec.actionCount == 0U || spec.actionCount > spec.actions.size() ||
        spec.label.empty()) {
      frame.invalidInput = true;
      frame.count = 0U;
      return frame;
    }
    std::array<const CreativeInputBinding*, kCreativeActionHintActionCapacity>
        bindings{};
    bool resolved = true;
    for (std::size_t actionIndex = 0U; actionIndex < spec.actionCount;
         ++actionIndex) {
      if (spec.actions[actionIndex] == CreativeInputActionId::Count) {
        frame.invalidInput = true;
        frame.count = 0U;
        return frame;
      }
      bindings[actionIndex] = findBinding(
          profile, spec.actions[actionIndex], context, device,
          spec.preferredGroupOrdinals[actionIndex]);
      resolved = resolved && bindings[actionIndex] != nullptr;
    }
    if (!resolved) {
      ++frame.unresolvedCount;
      continue;
    }

    CreativeActionHint& hint = frame.hints[frame.count++];
    hint.actionCount = spec.actionCount;
    frame.textTruncated = !appendText(hint.label, spec.label) ||
                          frame.textTruncated;
    for (std::size_t actionIndex = 0U; actionIndex < spec.actionCount;
         ++actionIndex) {
      hint.actions[actionIndex] = spec.actions[actionIndex];
      hint.triggers[actionIndex] = bindings[actionIndex]->trigger;
    }
    const std::string_view pairLabel =
        spec.actionCount == 2U
            ? compactPairLabel(bindings[0]->trigger, bindings[1]->trigger)
            : std::string_view{};
    const bool compactPair =
        !pairLabel.empty() &&
        resolvedModifiers(*bindings[0], platform) ==
            resolvedModifiers(*bindings[1], platform);
    if (compactPair) {
      frame.textTruncated =
          !appendModifiers(hint.chord,
                           resolvedModifiers(*bindings[0], platform)) ||
          !appendText(hint.chord, pairLabel) || frame.textTruncated;
      continue;
    }
    for (std::size_t actionIndex = 0U; actionIndex < spec.actionCount;
         ++actionIndex) {
      if (actionIndex > 0U) {
        frame.textTruncated = !appendText(hint.chord, " / ") ||
                              frame.textTruncated;
      }
      frame.textTruncated =
          !appendBindingChord(hint.chord, *bindings[actionIndex], platform) ||
          frame.textTruncated;
    }
  }
  return frame;
}

}  // namespace iggy3d::creative
