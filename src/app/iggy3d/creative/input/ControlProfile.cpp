#include "app/iggy3d/creative/input/ControlProfile.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace iggy3d::creative {
namespace {

constexpr CreativeInputModifierMask kAllModifiers =
    kCreativeInputModifierShift | kCreativeInputModifierControl |
    kCreativeInputModifierAlt | kCreativeInputModifierCommand;
constexpr std::uint16_t kControllerCommandGroupBase =
    static_cast<std::uint16_t>(kCreativeInputBindingCapacity);
static_assert(kCreativeInputBindingCapacity +
                  kCreativeControllerCommandChordCapacity <=
              std::numeric_limits<std::uint16_t>::max());

using ReachabilityRequirement = CreativeControlReachabilityRequirement;
using Action = CreativeInputActionId;
using Context = CreativeInputContext;
using Activation = CreativeInputBindingActivation;

constexpr ReachabilityRequirement requireGamepad(
    Action action,
    Context context,
    Activation activation = Activation::Press) noexcept {
  return {action, context, CreativeControlDevice::Gamepad, activation};
}

constexpr std::array kDefaultGamepadReachabilityRequirements{
    requireGamepad(Action::Undo, Context::EditorViewport),
    requireGamepad(Action::Redo, Context::EditorViewport),
    requireGamepad(Action::Save, Context::EditorViewport),
    requireGamepad(Action::ToggleCatalog, Context::EditorViewport),
    requireGamepad(Action::ToggleToolWheel, Context::EditorViewport),
    requireGamepad(Action::QuickEditPrevious, Context::EditorViewport),
    requireGamepad(Action::QuickEditNext, Context::EditorViewport),
    requireGamepad(Action::QuickEditDecrease, Context::EditorViewport),
    requireGamepad(Action::QuickEditIncrease, Context::EditorViewport),
    requireGamepad(Action::AcceptAction, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::RejectAction, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::PickAction, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::HotbarPrevious, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::HotbarNext, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::ToggleControls, Context::EditorViewport),
    requireGamepad(Action::FrameContext3D, Context::EditorViewport),
    requireGamepad(Action::FlyUp, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::FlyDown, Context::EditorViewport,
                   Activation::Continuous),
    requireGamepad(Action::Sprint, Context::EditorViewport,
                   Activation::Continuous),

    requireGamepad(Action::ToggleCatalog, Context::Catalog),
    requireGamepad(Action::CatalogPrevious, Context::Catalog),
    requireGamepad(Action::CatalogNext, Context::Catalog),
    requireGamepad(Action::CatalogPreviousVariant, Context::Catalog),
    requireGamepad(Action::CatalogNextVariant, Context::Catalog),
    requireGamepad(Action::CatalogPreviousPage, Context::Catalog),
    requireGamepad(Action::CatalogNextPage, Context::Catalog),
    requireGamepad(Action::CatalogConfirm, Context::Catalog),
    requireGamepad(Action::CatalogContextAction, Context::Catalog),
    requireGamepad(Action::CatalogClose, Context::Catalog),

    requireGamepad(Action::ToggleToolWheel, Context::ToolWheel),
    requireGamepad(Action::ToolWheelPrevious, Context::ToolWheel),
    requireGamepad(Action::ToolWheelNext, Context::ToolWheel),
    requireGamepad(Action::ToolWheelConfirm, Context::ToolWheel),
    requireGamepad(Action::ToolWheelOptions, Context::ToolWheel),
    requireGamepad(Action::ToolWheelClose, Context::ToolWheel),

    requireGamepad(Action::ToolOptionsPrevious, Context::ToolOptions),
    requireGamepad(Action::ToolOptionsNext, Context::ToolOptions),
    requireGamepad(Action::ToolOptionsDecrease, Context::ToolOptions),
    requireGamepad(Action::ToolOptionsIncrease, Context::ToolOptions),
    requireGamepad(Action::ToolOptionsConfirm, Context::ToolOptions),
    requireGamepad(Action::ToolOptionsClose, Context::ToolOptions),

    requireGamepad(Action::ConfirmActiveTool,
                   Context::AssetReplacementPreview),
    requireGamepad(Action::CancelActiveTool,
                   Context::AssetReplacementPreview),
    requireGamepad(Action::FlyUp, Context::AssetReplacementPreview,
                   Activation::Continuous),
    requireGamepad(Action::FlyDown, Context::AssetReplacementPreview,
                   Activation::Continuous),
    requireGamepad(Action::Sprint, Context::AssetReplacementPreview,
                   Activation::Continuous),

    requireGamepad(Action::CatalogPrevious, Context::AssetLibrary),
    requireGamepad(Action::CatalogNext, Context::AssetLibrary),
    requireGamepad(Action::CatalogPreviousVariant, Context::AssetLibrary),
    requireGamepad(Action::CatalogNextVariant, Context::AssetLibrary),
    requireGamepad(Action::CatalogConfirm, Context::AssetLibrary),
    requireGamepad(Action::CatalogClose, Context::AssetLibrary),

    requireGamepad(Action::ToolOptionsPrevious,
                   Context::AuthoredAssetEditMenu),
    requireGamepad(Action::ToolOptionsNext,
                   Context::AuthoredAssetEditMenu),
    requireGamepad(Action::ConfirmActiveTool,
                   Context::AuthoredAssetEditMenu),
    requireGamepad(Action::CancelActiveTool,
                   Context::AuthoredAssetEditMenu),

    requireGamepad(Action::ConfirmActiveTool, Context::TransformPreview),
    requireGamepad(Action::CancelActiveTool, Context::TransformPreview),
    requireGamepad(Action::ToggleTransformControls,
                   Context::TransformPreview),
    requireGamepad(Action::TransformNudgeNegative, Context::TransformPreview),
    requireGamepad(Action::TransformNudgePositive, Context::TransformPreview),
    requireGamepad(Action::QuickEditNext, Context::TransformPreview),
    requireGamepad(Action::QuickEditDecrease, Context::TransformPreview),
    requireGamepad(Action::QuickEditIncrease, Context::TransformPreview),
    requireGamepad(Action::FlyUp, Context::TransformPreview,
                   Activation::Continuous),
    requireGamepad(Action::FlyDown, Context::TransformPreview,
                   Activation::Continuous),
    requireGamepad(Action::Sprint, Context::TransformPreview,
                   Activation::Continuous),

    requireGamepad(Action::ConfirmActiveTool, Context::TransformControls),
    requireGamepad(Action::CancelActiveTool, Context::TransformControls),
    requireGamepad(Action::ToggleTransformControls,
                   Context::TransformControls),
    requireGamepad(Action::TransformControlPrevious,
                   Context::TransformControls),
    requireGamepad(Action::TransformControlNext, Context::TransformControls),

    requireGamepad(Action::ControlsPrevious, Context::Controls),
    requireGamepad(Action::ControlsNext, Context::Controls),
    requireGamepad(Action::ControlsDecrease, Context::Controls),
    requireGamepad(Action::ControlsIncrease, Context::Controls),
    requireGamepad(Action::ControlsActivate, Context::Controls),
    requireGamepad(Action::ControlsClose, Context::Controls),
    requireGamepad(Action::ControlsResetDefaults, Context::Controls),

    requireGamepad(Action::RuntimeAttack, Context::RuntimePlay,
                   Activation::Continuous),
    requireGamepad(Action::RuntimeInteract, Context::RuntimePlay,
                   Activation::Continuous),
    requireGamepad(Action::Sprint, Context::RuntimePlay,
                   Activation::Continuous),
    requireGamepad(Action::ToggleControls, Context::RuntimePlay),
};

static_assert(kDefaultGamepadReachabilityRequirements.size() <=
              kCreativeControlReachabilityRequirementCapacity);

[[nodiscard]] CreativeControlDevice deviceForKey(
    CreativeInputKey key) noexcept {
  return creativeInputKeyIsGamepad(key) ? CreativeControlDevice::Gamepad
                                        : CreativeControlDevice::KeyboardMouse;
}

[[nodiscard]] bool validReachabilityRequirement(
    const ReachabilityRequirement& requirement) noexcept {
  const auto action = static_cast<std::size_t>(requirement.action);
  const auto context = static_cast<std::size_t>(requirement.context);
  const auto device = static_cast<std::size_t>(requirement.device);
  const bool activationValid =
      requirement.activation == Activation::Press ||
      requirement.activation == Activation::Continuous;
  return action < static_cast<std::size_t>(Action::Count) &&
         context < static_cast<std::size_t>(Context::Count) &&
         device < static_cast<std::size_t>(CreativeControlDevice::Count) &&
         activationValid;
}

[[nodiscard]] bool bindingSatisfies(
    const CreativeInputBinding& binding,
    const ReachabilityRequirement& requirement) noexcept {
  const auto trigger = static_cast<std::size_t>(binding.trigger);
  if (trigger >= static_cast<std::size_t>(CreativeInputKey::Count) ||
      binding.trigger == CreativeInputKey::Unbound) {
    return false;
  }
  return binding.action == requirement.action &&
         binding.context == requirement.context &&
         binding.activation == requirement.activation &&
         deviceForKey(binding.trigger) == requirement.device;
}

[[nodiscard]] bool commandSatisfies(
    const CreativeControllerCommandChord& command,
    const ReachabilityRequirement& requirement) noexcept {
  return command.trigger != CreativeInputKey::Unbound &&
         command.trigger != CreativeInputKey::Count &&
         creativeInputKeyIsGamepad(command.trigger) &&
         command.action == requirement.action &&
         requirement.context == Context::EditorViewport &&
         requirement.device == CreativeControlDevice::Gamepad &&
         requirement.activation == Activation::Press;
}

void appendReachabilityIssue(
    CreativeControlReachabilityAuditResult& result,
    CreativeControlReachabilityIssueKind kind,
    std::size_t requirementIndex,
    const ReachabilityRequirement& requirement) noexcept {
  if (result.issueCount >= result.issues.size()) {
    result.requirementCapacityExceeded = true;
    return;
  }
  result.issues[result.issueCount++] = {kind, requirementIndex, requirement};
}

[[nodiscard]] bool sameDefaultGroup(const CreativeInputBinding& lhs,
                                    const CreativeInputBinding& rhs) noexcept {
  return lhs.action == rhs.action && lhs.trigger == rhs.trigger &&
         lhs.requiredAllModifiers == rhs.requiredAllModifiers &&
         lhs.requiredAnyModifiers == rhs.requiredAnyModifiers &&
         lhs.allowedModifiers == rhs.allowedModifiers &&
         lhs.activation == rhs.activation &&
         deviceForKey(lhs.trigger) == deviceForKey(rhs.trigger);
}

[[nodiscard]] bool validModifiers(CreativeInputModifierMask modifiers) noexcept {
  return (modifiers & ~kAllModifiers) == 0U;
}

[[nodiscard]] bool controllerCommandIndexForGroup(
    std::uint16_t group,
    std::size_t& index) noexcept {
  if (group < kControllerCommandGroupBase) {
    return false;
  }
  index = static_cast<std::size_t>(group - kControllerCommandGroupBase);
  return index < kCreativeControllerCommandChordCapacity;
}

[[nodiscard]] bool validControllerCommands(
    const CreativeControlProfile& profile) noexcept {
  const std::span<const CreativeControllerCommandChord> defaults =
      defaultCreativeControllerCommandChords();
  if (defaults.size() != kCreativeControllerCommandChordCapacity ||
      profile.controllerCommandCount != defaults.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < profile.controllerCommandCount;
       ++index) {
    const CreativeControllerCommandChord& command =
        profile.controllerCommands[index];
    if (command.action != defaults[index].action ||
        command.trigger == CreativeInputKey::Unbound ||
        command.trigger == CreativeInputKey::Count ||
        command.trigger == kCreativeControllerCommandModifier ||
        !creativeInputKeyIsGamepad(command.trigger)) {
      return false;
    }
    for (std::size_t previous = 0U; previous < index; ++previous) {
      if (profile.controllerCommands[previous].trigger == command.trigger) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] CreativeInputModifierMask modifierForKey(
    CreativeInputKey key) noexcept {
  switch (key) {
    case CreativeInputKey::LeftControl:
    case CreativeInputKey::RightControl:
      return kCreativeInputModifierControl;
    case CreativeInputKey::LeftShift:
    case CreativeInputKey::RightShift:
      return kCreativeInputModifierShift;
    case CreativeInputKey::LeftAlt:
    case CreativeInputKey::RightAlt:
      return kCreativeInputModifierAlt;
    case CreativeInputKey::LeftCommand:
    case CreativeInputKey::RightCommand:
      return kCreativeInputModifierCommand;
    default:
      return kCreativeInputModifierNone;
  }
}

[[nodiscard]] bool validStickProfile(
    const CreativeStickProfile& profile) noexcept {
  return std::isfinite(profile.deadzone) && profile.deadzone >= 0.0F &&
         profile.deadzone <= 0.75F &&
         std::isfinite(profile.responseExponent) &&
         profile.responseExponent >= 0.5F &&
         profile.responseExponent <= 3.0F;
}

[[nodiscard]] bool validProfileTuning(
    const CreativeControlProfile& profile) noexcept {
  return validStickProfile(profile.movementStick) &&
         validStickProfile(profile.lookStick) &&
         std::isfinite(profile.mouseLookSensitivity) &&
         profile.mouseLookSensitivity >= 0.02F &&
         profile.mouseLookSensitivity <= 1.0F &&
         std::isfinite(profile.gamepadLookSensitivity) &&
         profile.gamepadLookSensitivity >= 0.2F &&
         profile.gamepadLookSensitivity <= 8.0F &&
         profile.menuRepeatDelayMilliseconds >= 150U &&
         profile.menuRepeatDelayMilliseconds <= 750U &&
         profile.menuRepeatIntervalMilliseconds >= 40U &&
         profile.menuRepeatIntervalMilliseconds <= 300U;
}

void setGroupChord(CreativeControlProfile& profile,
                   std::uint16_t group,
                   CreativeInputKey trigger,
                   CreativeInputModifierMask requiredAll,
                   CreativeInputModifierMask requiredAny,
                   CreativeInputModifierMask allowed) noexcept {
  for (std::size_t index = 0; index < profile.bindingCount; ++index) {
    if (profile.bindingGroups[index] != group) {
      continue;
    }
    CreativeInputBinding& binding = profile.bindings[index];
    binding.trigger = trigger;
    binding.requiredAllModifiers = requiredAll;
    binding.requiredAnyModifiers = requiredAny;
    binding.allowedModifiers = allowed;
  }
}

[[nodiscard]] bool appendUniqueConflict(
    CreativeControlRebindReceipt& receipt,
    std::uint16_t group) noexcept {
  if (std::find(receipt.conflictGroups.begin(),
                receipt.conflictGroups.begin() + receipt.conflictCount,
                group) !=
      receipt.conflictGroups.begin() + receipt.conflictCount) {
    return true;
  }
  if (receipt.conflictCount >= receipt.conflictGroups.size()) {
    return false;
  }
  receipt.conflictGroups[receipt.conflictCount++] = group;
  return true;
}

void appendModifier(std::string& label,
                    CreativeInputModifierMask modifiers,
                    CreativeInputModifierMask modifier,
                    std::string_view name) {
  if ((modifiers & modifier) != 0U) {
    label.append(name);
    label.push_back('+');
  }
}

[[nodiscard]] CreativeControlRebindReceipt rebindControllerCommand(
    CreativeControlProfile& profile,
    const CreativeControlRebindRequest& request,
    std::size_t commandIndex) noexcept {
  CreativeControlRebindReceipt receipt;
  receipt.group = request.group;
  receipt.requestedTrigger = request.trigger;
  const CreativeControllerCommandChord& current =
      profile.controllerCommands[commandIndex];
  receipt.previousTrigger = current.trigger;

  if (request.conflictPolicy == CreativeControlConflictPolicy::Count ||
      request.modifiers != kCreativeInputModifierNone ||
      request.trigger == CreativeInputKey::Count) {
    receipt.status = CreativeControlRebindStatus::InvalidKey;
    return receipt;
  }
  if (request.trigger == CreativeInputKey::Unbound) {
    receipt.status = CreativeControlRebindStatus::RequiredAction;
    return receipt;
  }
  if (!creativeInputKeyIsGamepad(request.trigger)) {
    receipt.status = CreativeControlRebindStatus::DeviceMismatch;
    return receipt;
  }
  if (request.trigger == kCreativeControllerCommandModifier) {
    receipt.status = CreativeControlRebindStatus::ReservedAction;
    return receipt;
  }
  if (request.trigger == current.trigger) {
    receipt.status = CreativeControlRebindStatus::NoChange;
    return receipt;
  }

  std::size_t conflictIndex = profile.controllerCommandCount;
  for (std::size_t index = 0U; index < profile.controllerCommandCount;
       ++index) {
    if (index != commandIndex &&
        profile.controllerCommands[index].trigger == request.trigger) {
      conflictIndex = index;
      break;
    }
  }
  if (conflictIndex < profile.controllerCommandCount) {
    receipt.conflictGroups[0] = static_cast<std::uint16_t>(
        kControllerCommandGroupBase + conflictIndex);
    receipt.conflictCount = 1U;
    if (request.conflictPolicy == CreativeControlConflictPolicy::Reject) {
      receipt.status = CreativeControlRebindStatus::Conflict;
      return receipt;
    }
    if (request.conflictPolicy == CreativeControlConflictPolicy::Replace) {
      receipt.status = CreativeControlRebindStatus::RequiredAction;
      return receipt;
    }
  }

  CreativeControlProfile candidate = profile;
  candidate.controllerCommands[commandIndex].trigger = request.trigger;
  if (conflictIndex < candidate.controllerCommandCount) {
    candidate.controllerCommands[conflictIndex].trigger = current.trigger;
  }
  if (!validControllerCommands(candidate)) {
    receipt.status = CreativeControlRebindStatus::InvalidProfile;
    return receipt;
  }
  profile = candidate;
  receipt.status = CreativeControlRebindStatus::Applied;
  receipt.changed = true;
  return receipt;
}

template <typename Value>
[[nodiscard]] bool adjustClamped(Value& value,
                                 Value step,
                                 Value minimum,
                                 Value maximum,
                                 std::int32_t direction) noexcept {
  if (direction == 0) {
    return false;
  }
  const double nextValue =
      std::clamp(static_cast<double>(value) +
                     static_cast<double>(step) *
                         static_cast<double>(direction),
                 static_cast<double>(minimum),
                 static_cast<double>(maximum));
  const Value next = static_cast<Value>(nextValue);
  if (next == value) {
    return false;
  }
  value = next;
  return true;
}

}  // namespace

std::string_view toString(CreativeControlDevice device) noexcept {
  switch (device) {
    case CreativeControlDevice::KeyboardMouse: return "KeyboardMouse";
    case CreativeControlDevice::Gamepad: return "Gamepad";
    case CreativeControlDevice::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeControlConflictPolicy policy) noexcept {
  switch (policy) {
    case CreativeControlConflictPolicy::Reject: return "Reject";
    case CreativeControlConflictPolicy::Replace: return "Replace";
    case CreativeControlConflictPolicy::Swap: return "Swap";
    case CreativeControlConflictPolicy::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeControlRebindStatus status) noexcept {
  switch (status) {
    case CreativeControlRebindStatus::Applied: return "Applied";
    case CreativeControlRebindStatus::NoChange: return "NoChange";
    case CreativeControlRebindStatus::Conflict: return "Conflict";
    case CreativeControlRebindStatus::MultipleConflicts:
      return "MultipleConflicts";
    case CreativeControlRebindStatus::InvalidGroup: return "InvalidGroup";
    case CreativeControlRebindStatus::ReservedAction: return "ReservedAction";
    case CreativeControlRebindStatus::RequiredAction: return "RequiredAction";
    case CreativeControlRebindStatus::InvalidKey: return "InvalidKey";
    case CreativeControlRebindStatus::DeviceMismatch: return "DeviceMismatch";
    case CreativeControlRebindStatus::InvalidProfile: return "InvalidProfile";
    case CreativeControlRebindStatus::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeControlSettingId setting) noexcept {
  switch (setting) {
    case CreativeControlSettingId::MouseLookSensitivity:
      return "Mouse sensitivity";
    case CreativeControlSettingId::GamepadLookSensitivity:
      return "Look sensitivity";
    case CreativeControlSettingId::MovementDeadzone:
      return "Move deadzone";
    case CreativeControlSettingId::MovementResponse:
      return "Move response";
    case CreativeControlSettingId::LookDeadzone: return "Look deadzone";
    case CreativeControlSettingId::LookResponse: return "Look response";
    case CreativeControlSettingId::InvertLookX: return "Invert look X";
    case CreativeControlSettingId::InvertLookY: return "Invert look Y";
    case CreativeControlSettingId::MenuRepeatDelay:
      return "Menu repeat delay";
    case CreativeControlSettingId::MenuRepeatInterval:
      return "Menu repeat interval";
    case CreativeControlSettingId::Count: break;
  }
  return "Unknown";
}

std::string_view toString(
    CreativeControlReachabilityIssueKind kind) noexcept {
  switch (kind) {
    case CreativeControlReachabilityIssueKind::MissingBinding:
      return "MissingBinding";
    case CreativeControlReachabilityIssueKind::InvalidRequirement:
      return "InvalidRequirement";
    case CreativeControlReachabilityIssueKind::Count:
      break;
  }
  return "Unknown";
}

CreativeControlProfile makeDefaultCreativeControlProfile() {
  CreativeControlProfile profile;
  const std::span<const CreativeInputBinding> defaults =
      defaultCreativeInputBindings();
  profile.bindingCount =
      std::min(defaults.size(), profile.bindings.size());
  std::copy_n(defaults.begin(), profile.bindingCount, profile.bindings.begin());
  const std::span<const CreativeControllerCommandChord> commandDefaults =
      defaultCreativeControllerCommandChords();
  profile.controllerCommandCount =
      std::min(commandDefaults.size(), profile.controllerCommands.size());
  std::copy_n(commandDefaults.begin(), profile.controllerCommandCount,
              profile.controllerCommands.begin());

  for (std::size_t index = 0; index < profile.bindingCount; ++index) {
    std::size_t group = profile.groupCount;
    for (std::size_t previous = 0; previous < index; ++previous) {
      if (sameDefaultGroup(profile.bindings[index],
                           profile.bindings[previous])) {
        group = profile.bindingGroups[previous];
        break;
      }
    }
    if (group == profile.groupCount) {
      const CreativeInputActionId action = profile.bindings[index].action;
      const CreativeControlDevice device =
          deviceForKey(profile.bindings[index].trigger);
      std::uint16_t ordinal = 0;
      for (std::size_t priorGroup = 0; priorGroup < profile.groupCount;
           ++priorGroup) {
        if (profile.groupActions[priorGroup] == action &&
            profile.groupDevices[priorGroup] == device) {
          ++ordinal;
        }
      }
      profile.groupActions[group] = action;
      profile.groupDevices[group] = device;
      profile.groupOrdinals[group] = ordinal;
      ++profile.groupCount;
    }
    profile.bindingGroups[index] = static_cast<std::uint16_t>(group);
  }
  return profile;
}

bool isValidCreativeControlProfile(
    const CreativeControlProfile& profile) noexcept {
  if (profile.bindingCount == 0U ||
      profile.bindingCount > profile.bindings.size() ||
      profile.groupCount == 0U ||
      profile.groupCount > profile.groupActions.size() ||
      !validProfileTuning(profile) || !validControllerCommands(profile)) {
    return false;
  }
  for (std::size_t index = 0; index < profile.bindingCount; ++index) {
    const CreativeInputBinding& binding = profile.bindings[index];
    const std::uint16_t group = profile.bindingGroups[index];
    if (group >= profile.groupCount ||
        binding.action == CreativeInputActionId::Count ||
        binding.context == CreativeInputContext::Count ||
        binding.trigger == CreativeInputKey::Count ||
        !validModifiers(binding.requiredAllModifiers) ||
        !validModifiers(binding.requiredAnyModifiers) ||
        !validModifiers(binding.allowedModifiers) ||
        binding.action != profile.groupActions[group] ||
        (binding.trigger != CreativeInputKey::Unbound &&
         deviceForKey(binding.trigger) != profile.groupDevices[group])) {
      return false;
    }
  }
  const CreativeInputBindingAuditResult audit =
      auditCreativeInputBindings(profile.bindingSpan());
  if (audit.bindingCapacityExceeded || audit.conflictCount != 0U) {
    return false;
  }
  const CreativeControlReachabilityAuditResult reachability =
      auditCreativeControlReachability(
          profile, defaultCreativeGamepadReachabilityRequirements());
  return reachability.issueCount == 0U &&
         !reachability.bindingCapacityExceeded &&
         !reachability.requirementCapacityExceeded;
}

CreativeControlBindingList buildCreativeControlBindingList(
    const CreativeControlProfile& profile) noexcept {
  CreativeControlBindingList result;
  if (profile.bindingCount > profile.bindings.size() ||
      profile.groupCount > profile.groupActions.size() ||
      profile.controllerCommandCount > profile.controllerCommands.size()) {
    result.capacityExceeded = true;
    return result;
  }
  for (std::size_t index = 0U; index < profile.controllerCommandCount;
       ++index) {
    if (result.count >= result.rows.size()) {
      result.capacityExceeded = true;
      return result;
    }
    const CreativeControllerCommandChord& command =
        profile.controllerCommands[index];
    CreativeControlBindingRow& row = result.rows[result.count++];
    row.group =
        static_cast<std::uint16_t>(kControllerCommandGroupBase + index);
    row.action = command.action;
    row.trigger = command.trigger;
    row.device = CreativeControlDevice::Gamepad;
    row.activation = CreativeInputBindingActivation::Press;
    row.controllerCommandLayer = true;
    row.layerModifier = kCreativeControllerCommandModifier;
  }
  for (std::size_t group = 0; group < profile.groupCount; ++group) {
    if (creativeControlActionIsReserved(profile.groupActions[group])) {
      continue;
    }
    const CreativeInputBinding* binding = creativeControlGroupBinding(
        profile, static_cast<std::uint16_t>(group));
    if (binding == nullptr || result.count >= result.rows.size()) {
      result.capacityExceeded = true;
      break;
    }
    result.rows[result.count++] = {
        static_cast<std::uint16_t>(group),
        profile.groupOrdinals[group],
        binding->action,
        binding->trigger,
        binding->requiredAllModifiers,
        binding->requiredAnyModifiers,
        binding->allowedModifiers,
        profile.groupDevices[group],
        binding->activation,
    };
  }
  return result;
}

std::span<const CreativeControlReachabilityRequirement>
defaultCreativeGamepadReachabilityRequirements() noexcept {
  return kDefaultGamepadReachabilityRequirements;
}

CreativeControlReachabilityAuditResult auditCreativeControlReachability(
    std::span<const CreativeInputBinding> bindings,
    std::span<const CreativeControlReachabilityRequirement>
        requirements) noexcept {
  CreativeControlReachabilityAuditResult result;
  result.bindingCapacityExceeded =
      bindings.size() > kCreativeInputBindingCapacity;
  result.requirementCapacityExceeded =
      requirements.size() > kCreativeControlReachabilityRequirementCapacity;
  bindings = bindings.first(
      std::min(bindings.size(), kCreativeInputBindingCapacity));
  requirements = requirements.first(std::min(
      requirements.size(), kCreativeControlReachabilityRequirementCapacity));

  for (std::size_t requirementIndex = 0;
       requirementIndex < requirements.size(); ++requirementIndex) {
    const CreativeControlReachabilityRequirement& requirement =
        requirements[requirementIndex];
    if (!validReachabilityRequirement(requirement)) {
      appendReachabilityIssue(
          result, CreativeControlReachabilityIssueKind::InvalidRequirement,
          requirementIndex, requirement);
      continue;
    }
    const bool found = std::any_of(
        bindings.begin(), bindings.end(),
        [&requirement](const CreativeInputBinding& binding) {
          return bindingSatisfies(binding, requirement);
        });
    if (!found) {
      appendReachabilityIssue(
          result, CreativeControlReachabilityIssueKind::MissingBinding,
          requirementIndex, requirement);
    }
  }
  return result;
}

CreativeControlReachabilityAuditResult auditCreativeControlReachability(
    const CreativeControlProfile& profile,
    std::span<const CreativeControlReachabilityRequirement>
        requirements) noexcept {
  CreativeControlReachabilityAuditResult result;
  result.bindingCapacityExceeded =
      profile.bindingCount > kCreativeInputBindingCapacity ||
      profile.controllerCommandCount >
          kCreativeControllerCommandChordCapacity;
  result.requirementCapacityExceeded =
      requirements.size() > kCreativeControlReachabilityRequirementCapacity;
  const std::span<const CreativeInputBinding> bindings{
      profile.bindings.data(),
      std::min(profile.bindingCount, kCreativeInputBindingCapacity)};
  const std::span<const CreativeControllerCommandChord> commands{
      profile.controllerCommands.data(),
      std::min(profile.controllerCommandCount,
               kCreativeControllerCommandChordCapacity)};
  requirements = requirements.first(std::min(
      requirements.size(), kCreativeControlReachabilityRequirementCapacity));

  for (std::size_t requirementIndex = 0U;
       requirementIndex < requirements.size(); ++requirementIndex) {
    const CreativeControlReachabilityRequirement& requirement =
        requirements[requirementIndex];
    if (!validReachabilityRequirement(requirement)) {
      appendReachabilityIssue(
          result, CreativeControlReachabilityIssueKind::InvalidRequirement,
          requirementIndex, requirement);
      continue;
    }
    const bool bindingFound = std::any_of(
        bindings.begin(), bindings.end(),
        [&requirement](const CreativeInputBinding& binding) {
          return bindingSatisfies(binding, requirement);
        });
    const bool commandFound = std::any_of(
        commands.begin(), commands.end(),
        [&requirement](const CreativeControllerCommandChord& command) {
          return commandSatisfies(command, requirement);
        });
    if (!bindingFound && !commandFound) {
      appendReachabilityIssue(
          result, CreativeControlReachabilityIssueKind::MissingBinding,
          requirementIndex, requirement);
    }
  }
  return result;
}

const CreativeInputBinding* creativeControlGroupBinding(
    const CreativeControlProfile& profile,
    std::uint16_t group) noexcept {
  if (group >= profile.groupCount) {
    return nullptr;
  }
  for (std::size_t index = 0; index < profile.bindingCount; ++index) {
    if (profile.bindingGroups[index] == group) {
      return &profile.bindings[index];
    }
  }
  return nullptr;
}

bool creativeControlActionIsReserved(CreativeInputActionId action) noexcept {
  return action == CreativeInputActionId::ToggleControls ||
         action == CreativeInputActionId::ControlsPrevious ||
         action == CreativeInputActionId::ControlsNext ||
         action == CreativeInputActionId::ControlsDecrease ||
         action == CreativeInputActionId::ControlsIncrease ||
         action == CreativeInputActionId::ControlsActivate ||
         action == CreativeInputActionId::ControlsClose ||
         action == CreativeInputActionId::ControlsResetDefaults;
}

CreativeControlRebindReceipt rebindCreativeControl(
    CreativeControlProfile& profile,
    const CreativeControlRebindRequest& request) noexcept {
  CreativeControlRebindReceipt receipt;
  receipt.group = request.group;
  receipt.requestedTrigger = request.trigger;
  if (!isValidCreativeControlProfile(profile)) {
    return receipt;
  }
  std::size_t controllerCommandIndex = 0U;
  if (controllerCommandIndexForGroup(request.group,
                                     controllerCommandIndex)) {
    return rebindControllerCommand(profile, request,
                                   controllerCommandIndex);
  }
  const CreativeInputBinding* current =
      creativeControlGroupBinding(profile, request.group);
  if (current == nullptr) {
    receipt.status = CreativeControlRebindStatus::InvalidGroup;
    return receipt;
  }
  receipt.previousTrigger = current->trigger;
  if (creativeControlActionIsReserved(current->action)) {
    receipt.status = CreativeControlRebindStatus::ReservedAction;
    return receipt;
  }
  if (request.trigger == CreativeInputKey::Count ||
      !validModifiers(request.modifiers) ||
      request.conflictPolicy == CreativeControlConflictPolicy::Count) {
    receipt.status = CreativeControlRebindStatus::InvalidKey;
    return receipt;
  }
  const CreativeControlDevice device = profile.groupDevices[request.group];
  if (request.trigger != CreativeInputKey::Unbound &&
      deviceForKey(request.trigger) != device) {
    receipt.status = CreativeControlRebindStatus::DeviceMismatch;
    return receipt;
  }

  CreativeInputModifierMask requestedAll = request.modifiers;
  CreativeInputModifierMask requestedAny = kCreativeInputModifierNone;
  CreativeInputModifierMask requestedAllowed = request.modifiers;
  const CreativeInputModifierMask triggerModifier =
      modifierForKey(request.trigger);
  if (triggerModifier != kCreativeInputModifierNone) {
    requestedAll = kCreativeInputModifierNone;
    requestedAllowed = triggerModifier;
  }
  if (current->activation == CreativeInputBindingActivation::Continuous) {
    requestedAll = kCreativeInputModifierNone;
    requestedAllowed = kAllModifiers;
  } else if (device == CreativeControlDevice::Gamepad) {
    requestedAll = kCreativeInputModifierNone;
    requestedAllowed = kCreativeInputModifierNone;
  }
  if (current->trigger == request.trigger &&
      current->requiredAllModifiers == requestedAll &&
      current->requiredAnyModifiers == requestedAny &&
      current->allowedModifiers == requestedAllowed) {
    receipt.status = CreativeControlRebindStatus::NoChange;
    return receipt;
  }

  const CreativeInputKey previousTrigger = current->trigger;
  const CreativeInputModifierMask previousAll = current->requiredAllModifiers;
  const CreativeInputModifierMask previousAny = current->requiredAnyModifiers;
  const CreativeInputModifierMask previousAllowed = current->allowedModifiers;
  CreativeControlProfile candidate = profile;
  setGroupChord(candidate, request.group, request.trigger, requestedAll,
                requestedAny, requestedAllowed);

  const CreativeInputBindingAuditResult firstAudit =
      auditCreativeInputBindings(candidate.bindingSpan());
  for (const CreativeInputBindingConflict& conflict :
       firstAudit.conflictItems()) {
    const std::uint16_t firstGroup =
        candidate.bindingGroups[conflict.firstBindingIndex];
    const std::uint16_t secondGroup =
        candidate.bindingGroups[conflict.secondBindingIndex];
    if (firstGroup == request.group && secondGroup != request.group) {
      static_cast<void>(appendUniqueConflict(receipt, secondGroup));
    } else if (secondGroup == request.group && firstGroup != request.group) {
      static_cast<void>(appendUniqueConflict(receipt, firstGroup));
    }
  }

  if (receipt.conflictCount > 0U) {
    const bool conflictsWithReservedAction = std::any_of(
        receipt.conflictGroups.begin(),
        receipt.conflictGroups.begin() + receipt.conflictCount,
        [&candidate](std::uint16_t group) {
          return group < candidate.groupCount &&
                 creativeControlActionIsReserved(
                     candidate.groupActions[group]);
        });
    if (conflictsWithReservedAction) {
      receipt.status = CreativeControlRebindStatus::ReservedAction;
      return receipt;
    }
    if (request.conflictPolicy == CreativeControlConflictPolicy::Reject) {
      receipt.status = CreativeControlRebindStatus::Conflict;
      return receipt;
    }
    if (request.conflictPolicy == CreativeControlConflictPolicy::Swap) {
      if (receipt.conflictCount != 1U) {
        receipt.status = CreativeControlRebindStatus::MultipleConflicts;
        return receipt;
      }
      setGroupChord(candidate, receipt.conflictGroups[0], previousTrigger,
                    previousAll, previousAny, previousAllowed);
    } else {
      for (std::size_t index = 0; index < receipt.conflictCount; ++index) {
        setGroupChord(candidate, receipt.conflictGroups[index],
                      CreativeInputKey::Unbound,
                      kCreativeInputModifierNone,
                      kCreativeInputModifierNone,
                      kCreativeInputModifierNone);
      }
    }
  }

  const CreativeControlReachabilityAuditResult reachability =
      auditCreativeControlReachability(
          candidate, defaultCreativeGamepadReachabilityRequirements());
  if (reachability.issueCount != 0U) {
    receipt.status = CreativeControlRebindStatus::RequiredAction;
    return receipt;
  }
  if (!isValidCreativeControlProfile(candidate)) {
    receipt.status = CreativeControlRebindStatus::InvalidProfile;
    return receipt;
  }
  profile = candidate;
  receipt.status = CreativeControlRebindStatus::Applied;
  receipt.changed = true;
  return receipt;
}

bool applyStoredCreativeControlChord(
    CreativeControlProfile& profile,
    std::uint16_t group,
    CreativeInputKey trigger,
    CreativeInputModifierMask requiredAllModifiers,
    CreativeInputModifierMask requiredAnyModifiers,
    CreativeInputModifierMask allowedModifiers) noexcept {
  if (group >= profile.groupCount || trigger == CreativeInputKey::Count ||
      creativeControlActionIsReserved(profile.groupActions[group]) ||
      !validModifiers(requiredAllModifiers) ||
      !validModifiers(requiredAnyModifiers) ||
      !validModifiers(allowedModifiers) ||
      (trigger != CreativeInputKey::Unbound &&
       deviceForKey(trigger) != profile.groupDevices[group])) {
    return false;
  }
  setGroupChord(profile, group, trigger, requiredAllModifiers,
                requiredAnyModifiers, allowedModifiers);
  return true;
}

bool applyStoredCreativeControllerCommand(
    CreativeControlProfile& profile,
    CreativeInputActionId action,
    CreativeInputKey trigger) noexcept {
  if (trigger == CreativeInputKey::Unbound ||
      trigger == CreativeInputKey::Count ||
      trigger == kCreativeControllerCommandModifier ||
      !creativeInputKeyIsGamepad(trigger)) {
    return false;
  }
  for (std::size_t index = 0U; index < profile.controllerCommandCount;
       ++index) {
    if (profile.controllerCommands[index].action == action) {
      profile.controllerCommands[index].trigger = trigger;
      return true;
    }
  }
  return false;
}

bool adjustCreativeControlSetting(CreativeControlProfile& profile,
                                  CreativeControlSettingId setting,
                                  std::int32_t direction) noexcept {
  switch (setting) {
    case CreativeControlSettingId::MouseLookSensitivity:
      return adjustClamped(profile.mouseLookSensitivity, 0.02F, 0.02F, 1.0F,
                           direction);
    case CreativeControlSettingId::GamepadLookSensitivity:
      return adjustClamped(profile.gamepadLookSensitivity, 0.2F, 0.2F, 8.0F,
                           direction);
    case CreativeControlSettingId::MovementDeadzone:
      return adjustClamped(profile.movementStick.deadzone, 0.01F, 0.0F, 0.75F,
                           direction);
    case CreativeControlSettingId::MovementResponse:
      return adjustClamped(profile.movementStick.responseExponent, 0.1F, 0.5F,
                           3.0F, direction);
    case CreativeControlSettingId::LookDeadzone:
      return adjustClamped(profile.lookStick.deadzone, 0.01F, 0.0F, 0.75F,
                           direction);
    case CreativeControlSettingId::LookResponse:
      return adjustClamped(profile.lookStick.responseExponent, 0.1F, 0.5F,
                           3.0F, direction);
    case CreativeControlSettingId::InvertLookX:
      if (direction != 0) {
        profile.lookStick.invertX = !profile.lookStick.invertX;
        return true;
      }
      return false;
    case CreativeControlSettingId::InvertLookY:
      if (direction != 0) {
        profile.lookStick.invertY = !profile.lookStick.invertY;
        return true;
      }
      return false;
    case CreativeControlSettingId::MenuRepeatDelay:
      return adjustClamped(profile.menuRepeatDelayMilliseconds,
                           static_cast<std::uint16_t>(25),
                           static_cast<std::uint16_t>(150),
                           static_cast<std::uint16_t>(750), direction);
    case CreativeControlSettingId::MenuRepeatInterval:
      return adjustClamped(profile.menuRepeatIntervalMilliseconds,
                           static_cast<std::uint16_t>(10),
                           static_cast<std::uint16_t>(40),
                           static_cast<std::uint16_t>(300), direction);
    case CreativeControlSettingId::Count:
      return false;
  }
  return false;
}

std::string_view creativeControlKeyDisplayLabelView(
    CreativeInputKey key) noexcept {
  switch (key) {
    case CreativeInputKey::GamepadInventory: return "Triangle";
    case CreativeInputKey::GamepadConfirm: return "X";
    case CreativeInputKey::GamepadCancel: return "Circle";
    case CreativeInputKey::GamepadWest: return "Square";
    case CreativeInputKey::GamepadBack: return "Create";
    case CreativeInputKey::GamepadStart: return "Options";
    case CreativeInputKey::GamepadTouchpad: return "Touchpad";
    case CreativeInputKey::GamepadLeftShoulder: return "L1";
    case CreativeInputKey::GamepadRightShoulder: return "R1";
    case CreativeInputKey::GamepadLeftTrigger: return "L2";
    case CreativeInputKey::GamepadRightTrigger: return "R2";
    case CreativeInputKey::GamepadLeftStick: return "L3";
    case CreativeInputKey::GamepadRightStick: return "R3";
    case CreativeInputKey::GamepadDpadUp: return "D-pad Up";
    case CreativeInputKey::GamepadDpadDown: return "D-pad Down";
    case CreativeInputKey::GamepadDpadLeft: return "D-pad Left";
    case CreativeInputKey::GamepadDpadRight: return "D-pad Right";
    case CreativeInputKey::MousePrimary: return "Mouse Left";
    case CreativeInputKey::MouseSecondary: return "Mouse Right";
    case CreativeInputKey::MouseMiddle: return "Mouse Middle";
    default: return toString(key);
  }
}

std::string creativeControlKeyDisplayLabel(CreativeInputKey key) {
  return std::string(creativeControlKeyDisplayLabelView(key));
}

std::string creativeControlBindingDisplayLabel(
    const CreativeControlBindingRow& row,
    CreativeInputPlatform platform) {
  CreativeInputModifierMask modifiers = row.requiredAllModifiers;
  if (row.requiredAnyModifiers != kCreativeInputModifierNone) {
    const CreativeInputModifierMask preferred =
        preferredCommandModifier(platform);
    modifiers |= (row.requiredAnyModifiers & preferred) != 0U
                     ? preferred
                     : row.requiredAnyModifiers;
  }
  std::string label;
  if (row.controllerCommandLayer &&
      row.layerModifier != CreativeInputKey::Unbound) {
    label.append(creativeControlKeyDisplayLabel(row.layerModifier));
    label.push_back('+');
  }
  appendModifier(label, modifiers, kCreativeInputModifierControl, "Ctrl");
  appendModifier(label, modifiers, kCreativeInputModifierCommand, "Cmd");
  appendModifier(label, modifiers, kCreativeInputModifierAlt, "Alt");
  appendModifier(label, modifiers, kCreativeInputModifierShift, "Shift");
  label.append(creativeControlKeyDisplayLabel(row.trigger));
  return label;
}

}  // namespace iggy3d::creative
