#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeInputContext : std::uint8_t {
  EditorViewport,
  TextEntry,
  Modal,
  Capture,
};

enum class CreativeInputActionId : std::uint8_t {
  SelectTool,
  MoveTool,
  EnterPlaceMode,
  CycleBrush,
  DeleteSelection,
  Undo,
  DuplicateSelection,
  RotateYawNegative,
  RotateYawPositive,
  ScaleDown,
  ScaleUp,
  Save,
  NewDocument,
  Load,
};

enum class CreativeInputKey : std::uint8_t {
  Digit1,
  Digit2,
  Digit3,
  B,
  D,
  Z,
  LeftBracket,
  RightBracket,
  Minus,
  Equals,
  Delete,
  Backspace,
  F5,
  F6,
  F9,
  W,
  A,
  S,
  Space,
  LeftControl,
  RightControl,
  LeftShift,
  RightShift,
  LeftAlt,
  RightAlt,
  LeftCommand,
  RightCommand,
  Count,
};

using CreativeInputModifierMask = std::uint8_t;

inline constexpr CreativeInputModifierMask kCreativeInputModifierNone = 0;
inline constexpr CreativeInputModifierMask kCreativeInputModifierShift = 1U << 0;
inline constexpr CreativeInputModifierMask kCreativeInputModifierControl = 1U << 1;
inline constexpr CreativeInputModifierMask kCreativeInputModifierAlt = 1U << 2;
inline constexpr CreativeInputModifierMask kCreativeInputModifierCommand = 1U << 3;

enum class CreativeInputConsumePolicy : std::uint8_t {
  PassThrough,
  ConsumeTrigger,
  ConsumeChord,
};

struct CreativeInputBinding {
  CreativeInputActionId action = CreativeInputActionId::SelectTool;
  CreativeInputKey trigger = CreativeInputKey::Digit1;
  CreativeInputContext context = CreativeInputContext::EditorViewport;
  CreativeInputModifierMask requiredAllModifiers = kCreativeInputModifierNone;
  CreativeInputModifierMask requiredAnyModifiers = kCreativeInputModifierNone;
  CreativeInputModifierMask allowedModifiers = kCreativeInputModifierNone;
  std::uint16_t priority = 0;
  CreativeInputConsumePolicy consumePolicy =
      CreativeInputConsumePolicy::ConsumeTrigger;
};

inline constexpr std::size_t kCreativeInputKeyCount =
    static_cast<std::size_t>(CreativeInputKey::Count);
inline constexpr std::size_t kCreativeInputBindingCapacity = 32;

struct CreativeInputFrame {
  CreativeInputContext context = CreativeInputContext::EditorViewport;
  CreativeInputModifierMask modifiers = kCreativeInputModifierNone;
  std::array<bool, kCreativeInputKeyCount> keysDown{};
};

struct CreativeInputRouterState {
  // Physical chord activity is tracked even outside the binding's context. A
  // held shortcut therefore cannot fire merely because a text field or modal
  // closes and returns focus to the viewport.
  std::array<bool, kCreativeInputBindingCapacity> bindingActive{};
};

struct CreativeInputActionEvent {
  CreativeInputActionId action = CreativeInputActionId::SelectTool;
  CreativeInputKey trigger = CreativeInputKey::Digit1;
};

struct CreativeInputRouteResult {
  CreativeInputContext context = CreativeInputContext::EditorViewport;
  std::array<CreativeInputActionEvent, kCreativeInputBindingCapacity> actions{};
  std::size_t actionCount = 0;
  std::array<bool, kCreativeInputKeyCount> consumedKeys{};
  bool bindingCapacityExceeded = false;

  [[nodiscard]] std::span<const CreativeInputActionEvent>
  actionEvents() const noexcept {
    return std::span<const CreativeInputActionEvent>{actions.data(), actionCount};
  }
};

[[nodiscard]] std::string_view toString(
    CreativeInputActionId action) noexcept;
[[nodiscard]] std::string_view toString(CreativeInputKey key) noexcept;

[[nodiscard]] std::span<const CreativeInputBinding>
defaultCreativeInputBindings() noexcept;

void setCreativeInputKey(CreativeInputFrame& frame,
                         CreativeInputKey key,
                         bool down) noexcept;
[[nodiscard]] bool creativeInputKeyDown(const CreativeInputFrame& frame,
                                        CreativeInputKey key) noexcept;
[[nodiscard]] bool creativeInputKeyConsumed(
    const CreativeInputRouteResult& result,
    CreativeInputKey key) noexcept;

[[nodiscard]] CreativeInputRouteResult routeCreativeInput(
    CreativeInputRouterState& state,
    const CreativeInputFrame& frame,
    std::span<const CreativeInputBinding> bindings =
        defaultCreativeInputBindings());

}  // namespace iggy3d::creative
