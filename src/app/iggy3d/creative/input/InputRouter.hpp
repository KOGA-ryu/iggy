#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/input/ControllerInput.hpp"

namespace iggy3d::creative {

enum class CreativeInputContext : std::uint8_t {
  EditorViewport,
  RuntimePlay,
  Catalog,
  ToolWheel,
  ToolOptions,
  AssetReplacementPreview,
  AssetLibrary,
  AuthoredAssetEditMenu,
  TransformPreview,
  TransformControls,
  Controls,
  TextEntry,
  Modal,
  Capture,
  DesktopUi,
  Count,
};

enum class CreativeInputPlatform : std::uint8_t {
  WindowsLinux,
  MacOS,
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
  // Secondary catalog action: assign a tool to the wheel or cycle an asset
  // between Equip and Replace/Manage.
  CatalogContextAction,
  CatalogClose,
  ToggleToolWheel,
  ToolWheelPrevious,
  ToolWheelNext,
  ToolWheelConfirm,
  ToolWheelOptions,
  ToolWheelClose,
  ToolOptionsPrevious,
  ToolOptionsNext,
  ToolOptionsDecrease,
  ToolOptionsIncrease,
  ToolOptionsConfirm,
  ToolOptionsClose,
  QuickEditPrevious,
  QuickEditNext,
  QuickEditDecrease,
  QuickEditIncrease,
  ToggleTransformControls,
  TransformControlPrevious,
  TransformControlNext,
  TransformConstraintX,
  TransformConstraintY,
  TransformConstraintZ,
  TransformNudgeNegative,
  TransformNudgePositive,
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
  MoveForward,
  MoveBackward,
  MoveLeft,
  MoveRight,
  FlyUp,
  FlyDown,
  Sprint,
  RuntimeAttack,
  RuntimeInteract,
  PrimaryAction,
  SecondaryAction,
  AcceptAction,
  RejectAction,
  PickAction,
  HotbarPrevious,
  HotbarNext,
  ToggleControls,
  ControlsPrevious,
  ControlsNext,
  ControlsDecrease,
  ControlsIncrease,
  ControlsActivate,
  ControlsClose,
  ControlsResetDefaults,
  FrameContext3D,
  Count,
};

inline constexpr std::size_t kCreativeInputActionCount =
    static_cast<std::size_t>(CreativeInputActionId::Count);

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
  Y,
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
  MousePrimary,
  MouseSecondary,
  MouseMiddle,
  GamepadWest,
  GamepadBack,
  GamepadStart,
  GamepadLeftStick,
  GamepadRightStick,
  GamepadLeftTrigger,
  GamepadRightTrigger,
  GamepadTouchpad,
  Unbound,
  Count,
};

// System-command layer for gamepads. Holding the reserved modifier temporarily
// replaces ordinary gamepad actions with profile-owned semantic commands; the
// router consumes the physical chord so tool, flight, and menu bindings cannot
// also fire. These commands remain device-independent after routing.
struct CreativeControllerCommandChord {
  CreativeInputKey trigger = CreativeInputKey::Unbound;
  CreativeInputActionId action = CreativeInputActionId::Undo;
};

inline constexpr std::size_t kCreativeControllerCommandChordCapacity = 3U;
inline constexpr CreativeInputKey kCreativeControllerCommandModifier =
    CreativeInputKey::GamepadStart;

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

enum class CreativeInputBindingActivation : std::uint8_t {
  Press,
  Continuous,
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
  CreativeInputBindingActivation activation =
      CreativeInputBindingActivation::Press;

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
      std::string_view labelValue = {},
      CreativeInputBindingActivation activationValue =
          CreativeInputBindingActivation::Press) noexcept
      : action(actionValue),
        trigger(triggerValue),
        context(contextValue),
        requiredAllModifiers(requiredAllValue),
        requiredAnyModifiers(requiredAnyValue),
        allowedModifiers(allowedValue),
        priority(priorityValue),
        consumePolicy(consumeValue),
        configurableLabel(labelValue),
        activation(activationValue) {}
};

inline constexpr std::size_t kCreativeInputKeyCount =
    static_cast<std::size_t>(CreativeInputKey::Count);
inline constexpr std::size_t kCreativeInputBindingCapacity = 240;
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
  std::array<bool, kCreativeInputActionCount> actionDown{};
  std::array<bool, kCreativeInputKeyCount> previousKeysDown{};
  bool controllerCommandGestureEligible = false;
  bool controllerCommandGestureUsed = false;
};

struct CreativeInputActionEvent {
  CreativeInputActionId action = CreativeInputActionId::HotbarSlot1;
  CreativeInputKey trigger = CreativeInputKey::Digit1;
};

struct CreativeInputRouteResult {
  CreativeInputContext context = CreativeInputContext::EditorViewport;
  std::array<CreativeInputActionEvent, kCreativeInputBindingCapacity> actions{};
  std::size_t actionCount = 0;
  std::array<bool, kCreativeInputActionCount> down{};
  std::array<bool, kCreativeInputActionCount> pressed{};
  std::array<bool, kCreativeInputActionCount> released{};
  std::array<bool, kCreativeInputKeyCount> consumedKeys{};
  bool controllerCommandLayerActive = false;
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
  ActivationShadow,
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
[[nodiscard]] std::string_view toString(
    CreativeInputContext context) noexcept;
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
[[nodiscard]] std::span<const CreativeControllerCommandChord>
defaultCreativeControllerCommandChords() noexcept;

[[nodiscard]] bool parseCreativeInputActionId(
    std::string_view value,
    CreativeInputActionId& out) noexcept;
[[nodiscard]] bool parseCreativeInputContext(
    std::string_view value,
    CreativeInputContext& out) noexcept;
[[nodiscard]] bool parseCreativeInputKey(
    std::string_view value,
    CreativeInputKey& out) noexcept;
[[nodiscard]] bool creativeInputKeyIsGamepad(
    CreativeInputKey key) noexcept;
[[nodiscard]] bool creativeInputKeyIsModifier(
    CreativeInputKey key) noexcept;

void setCreativeInputKey(CreativeInputFrame& frame,
                         CreativeInputKey key,
                         bool down) noexcept;
[[nodiscard]] bool creativeInputKeyDown(const CreativeInputFrame& frame,
                                        CreativeInputKey key) noexcept;
[[nodiscard]] bool creativeInputKeyConsumed(
    const CreativeInputRouteResult& result,
    CreativeInputKey key) noexcept;
[[nodiscard]] bool creativeInputActionDown(
    const CreativeInputFrame& frame,
    CreativeInputActionId action,
    std::span<const CreativeInputBinding> bindings =
        defaultCreativeInputBindings(),
    const CreativeInputRouteResult* routedInput = nullptr) noexcept;

[[nodiscard]] CreativeInputRouteResult routeCreativeInput(
    CreativeInputRouterState& state,
    const CreativeInputFrame& frame,
    std::span<const CreativeInputBinding> bindings =
        defaultCreativeInputBindings(),
    std::span<const CreativeControllerCommandChord> controllerCommands =
        defaultCreativeControllerCommandChords());

// Whether a routed action is present this frame.
[[nodiscard]] bool creativeInputRouteContains(
    const CreativeInputRouteResult& route,
    CreativeInputActionId action) noexcept;
[[nodiscard]] bool creativeInputActionDown(
    const CreativeInputRouteResult& route,
    CreativeInputActionId action) noexcept;
[[nodiscard]] bool creativeInputActionPressed(
    const CreativeInputRouteResult& route,
    CreativeInputActionId action) noexcept;
[[nodiscard]] bool creativeInputActionReleased(
    const CreativeInputRouteResult& route,
    CreativeInputActionId action) noexcept;

// Removes every occurrence of an action from the route (stable compaction), so
// a consumer downstream never sees it. Used to make captured Esc release-only:
// the pointer is freed without the ToggleControls action also opening Controls.
void creativeInputRouteRemove(CreativeInputRouteResult& route,
                              CreativeInputActionId action) noexcept;

// O(n^2 * 16) over a registry bounded by kCreativeInputBindingCapacity. The
// modifier domain is four bits, so every possible overlap is checked exactly.
[[nodiscard]] CreativeInputBindingAuditResult auditCreativeInputBindings(
    std::span<const CreativeInputBinding> bindings =
        defaultCreativeInputBindings()) noexcept;

}  // namespace iggy3d::creative
