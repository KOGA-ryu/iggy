#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeInputContext : std::uint8_t {
  EditorViewport,
  Catalog,
  ToolWheel,
  ToolOptions,
  ClipboardPreview,
  TextEntry,
  Modal,
  Capture,
};

enum class CreativeInputPlatform : std::uint8_t {
  WindowsLinux,
  MacOS,
};

enum class CreativeControllerAxis : std::uint8_t {
  MoveX,
  MoveY,
  LookX,
  LookY,
  LeftTrigger,
  RightTrigger,
  Count,
};

enum class CreativeControllerButton : std::uint8_t {
  South,
  East,
  West,
  North,
  Back,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  LeftTrigger,
  RightTrigger,
  Count,
};

inline constexpr std::size_t kCreativeControllerAxisCount =
    static_cast<std::size_t>(CreativeControllerAxis::Count);
inline constexpr std::size_t kCreativeControllerButtonCount =
    static_cast<std::size_t>(CreativeControllerButton::Count);
inline constexpr float kCreativeControllerStickDeadzone = 0.18F;
inline constexpr float kCreativeControllerTriggerThreshold = 0.55F;

// Hardware adapters populate this platform-neutral snapshot. Trigger button
// state is derived from the corresponding axes by stepCreativeControllerInput.
struct CreativeControllerSample {
  bool connected = false;
  std::array<float, kCreativeControllerAxisCount> axes{};
  std::array<bool, kCreativeControllerButtonCount> buttonsDown{};
};

struct CreativeControllerState {
  bool connected = false;
  std::array<bool, kCreativeControllerButtonCount> buttonsDown{};
};

struct CreativeControllerFrame {
  CreativeControllerState next{};
  std::array<float, kCreativeControllerAxisCount> axes{};
  std::array<bool, kCreativeControllerButtonCount> pressed{};
  std::array<bool, kCreativeControllerButtonCount> released{};
};

enum class CreativeInputActionId : std::uint8_t {
  HotbarSlot1,
  HotbarSlot2,
  HotbarSlot3,
  HotbarSlot4,
  HotbarSlot5,
  HotbarSlot6,
  HotbarSlot7,
  HotbarSlot8,
  HotbarSlot9,
  ToggleCatalog,
  CatalogPrevious,
  CatalogNext,
  CatalogPreviousVariant,
  CatalogNextVariant,
  CatalogPreviousPage,
  CatalogNextPage,
  CatalogConfirm,
  CatalogClose,
  ToggleToolWheel,
  ToolWheelPrevious,
  ToolWheelNext,
  ToolWheelConfirm,
  ToolWheelClose,
  ToolOptionsPrevious,
  ToolOptionsNext,
  ToolOptionsDecrease,
  ToolOptionsIncrease,
  ToolOptionsConfirm,
  ToolOptionsClose,
  ConfirmActiveTool,
  CancelActiveTool,
  DeleteSelection,
  Undo,
  Redo,
  CopySelection,
  CutSelection,
  PasteClipboard,
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
  Digit4,
  Digit5,
  Digit6,
  Digit7,
  Digit8,
  Digit9,
  C,
  D,
  E,
  N,
  O,
  R,
  S,
  V,
  X,
  Z,
  LeftBracket,
  RightBracket,
  Minus,
  Equals,
  Delete,
  Backspace,
  Enter,
  Escape,
  ArrowUp,
  ArrowDown,
  ArrowLeft,
  ArrowRight,
  W,
  A,
  Space,
  LeftControl,
  RightControl,
  LeftShift,
  RightShift,
  LeftAlt,
  RightAlt,
  LeftCommand,
  RightCommand,
  GamepadInventory,
  GamepadDpadUp,
  GamepadDpadDown,
  GamepadConfirm,
  GamepadCancel,
  GamepadDpadLeft,
  GamepadDpadRight,
  GamepadLeftShoulder,
  GamepadRightShoulder,
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
  CreativeInputActionId action = CreativeInputActionId::HotbarSlot1;
  CreativeInputKey trigger = CreativeInputKey::Digit1;
  CreativeInputContext context = CreativeInputContext::EditorViewport;
  CreativeInputModifierMask requiredAllModifiers = kCreativeInputModifierNone;
  CreativeInputModifierMask requiredAnyModifiers = kCreativeInputModifierNone;
  CreativeInputModifierMask allowedModifiers = kCreativeInputModifierNone;
  std::uint16_t priority = 0;
  CreativeInputConsumePolicy consumePolicy =
      CreativeInputConsumePolicy::ConsumeTrigger;
  std::string_view configurableLabel;

  constexpr CreativeInputBinding() noexcept = default;
  constexpr CreativeInputBinding(
      CreativeInputActionId actionValue,
      CreativeInputKey triggerValue,
      CreativeInputContext contextValue,
      CreativeInputModifierMask requiredAllValue,
      CreativeInputModifierMask requiredAnyValue,
      CreativeInputModifierMask allowedValue,
      std::uint16_t priorityValue,
      CreativeInputConsumePolicy consumeValue,
      std::string_view labelValue = {}) noexcept
      : action(actionValue),
        trigger(triggerValue),
        context(contextValue),
        requiredAllModifiers(requiredAllValue),
        requiredAnyModifiers(requiredAnyValue),
        allowedModifiers(allowedValue),
        priority(priorityValue),
        consumePolicy(consumeValue),
        configurableLabel(labelValue) {}
};

inline constexpr std::size_t kCreativeInputKeyCount =
    static_cast<std::size_t>(CreativeInputKey::Count);
inline constexpr std::size_t kCreativeInputBindingCapacity = 96;
inline constexpr std::size_t kCreativeInputConflictCapacity =
    kCreativeInputBindingCapacity * (kCreativeInputBindingCapacity - 1U) / 2U;

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
  CreativeInputActionId action = CreativeInputActionId::HotbarSlot1;
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

enum class CreativeInputBindingConflictKind : std::uint8_t {
  DuplicateAction,
  PriorityShadow,
  AmbiguousPriority,
};

struct CreativeInputBindingConflict {
  CreativeInputBindingConflictKind kind =
      CreativeInputBindingConflictKind::AmbiguousPriority;
  std::size_t firstBindingIndex = 0;
  std::size_t secondBindingIndex = 0;
  std::size_t winningBindingIndex = 0;
  CreativeInputActionId firstAction = CreativeInputActionId::HotbarSlot1;
  CreativeInputActionId secondAction = CreativeInputActionId::HotbarSlot1;
  CreativeInputContext context = CreativeInputContext::EditorViewport;
  CreativeInputKey trigger = CreativeInputKey::Digit1;
  CreativeInputModifierMask overlappingModifiers =
      kCreativeInputModifierNone;
};

struct CreativeInputBindingAuditResult {
  std::array<CreativeInputBindingConflict, kCreativeInputConflictCapacity>
      conflicts{};
  std::size_t conflictCount = 0;
  bool bindingCapacityExceeded = false;

  [[nodiscard]] std::span<const CreativeInputBindingConflict>
  conflictItems() const noexcept {
    return {conflicts.data(), conflictCount};
  }
};

[[nodiscard]] std::string_view toString(
    CreativeInputActionId action) noexcept;
[[nodiscard]] std::string_view toString(CreativeInputKey key) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeInputBindingConflictKind kind) noexcept;

[[nodiscard]] CreativeInputModifierMask preferredCommandModifier(
    CreativeInputPlatform platform) noexcept;
[[nodiscard]] std::string creativeInputBindingLabel(
    const CreativeInputBinding& binding);
[[nodiscard]] std::string creativeInputChordLabel(
    const CreativeInputBinding& binding,
    CreativeInputPlatform platform);

[[nodiscard]] std::span<const CreativeInputBinding>
defaultCreativeInputBindings() noexcept;

void setCreativeControllerAxis(CreativeControllerSample& sample,
                               CreativeControllerAxis axis,
                               float value) noexcept;
void setCreativeControllerButton(CreativeControllerSample& sample,
                                 CreativeControllerButton button,
                                 bool down) noexcept;
[[nodiscard]] float creativeControllerAxis(
    const CreativeControllerFrame& frame,
    CreativeControllerAxis axis) noexcept;
[[nodiscard]] bool creativeControllerButtonDown(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept;
[[nodiscard]] bool creativeControllerButtonPressed(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept;
[[nodiscard]] bool creativeControllerButtonReleased(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept;
[[nodiscard]] CreativeControllerFrame stepCreativeControllerInput(
    CreativeControllerState previous,
    const CreativeControllerSample& current) noexcept;

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

// O(n^2 * 16) over a registry bounded by kCreativeInputBindingCapacity. The
// modifier domain is four bits, so every possible overlap is checked exactly.
[[nodiscard]] CreativeInputBindingAuditResult auditCreativeInputBindings(
    std::span<const CreativeInputBinding> bindings =
        defaultCreativeInputBindings()) noexcept;

}  // namespace iggy3d::creative
