#include "app/iggy3d/creative/input/ControlProfile.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const cr::CreativeControlBindingRow* findRow(
    const cr::CreativeControlBindingList& list,
    cr::CreativeInputActionId action,
    cr::CreativeControlDevice device,
    std::uint16_t ordinal = 0) {
  const auto found = std::find_if(
      list.items().begin(), list.items().end(),
      [=](const cr::CreativeControlBindingRow& row) {
        return row.action == action && row.device == device &&
               row.ordinal == ordinal;
      });
  return found == list.items().end() ? nullptr : &*found;
}

bool defaultsAreBoundedConflictFreeAndMinecraftShaped() {
  const cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(profile);
  const cr::CreativeControlBindingRow* forward =
      findRow(rows, cr::CreativeInputActionId::MoveForward,
              cr::CreativeControlDevice::KeyboardMouse);
  const cr::CreativeControlBindingRow* primaryMouse =
      findRow(rows, cr::CreativeInputActionId::PrimaryAction,
              cr::CreativeControlDevice::KeyboardMouse);
  const cr::CreativeControlBindingRow* primaryPad =
      findRow(rows, cr::CreativeInputActionId::PrimaryAction,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* secondaryPad =
      findRow(rows, cr::CreativeInputActionId::SecondaryAction,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* acceptPad =
      findRow(rows, cr::CreativeInputActionId::AcceptAction,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* rejectPad =
      findRow(rows, cr::CreativeInputActionId::RejectAction,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* flyUpPad =
      findRow(rows, cr::CreativeInputActionId::FlyUp,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* flyDownPad =
      findRow(rows, cr::CreativeInputActionId::FlyDown,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* toolWheelPad =
      findRow(rows, cr::CreativeInputActionId::ToggleToolWheel,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* pickPad =
      findRow(rows, cr::CreativeInputActionId::PickAction,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* cycleSettingPad =
      findRow(rows, cr::CreativeInputActionId::QuickEditNext,
              cr::CreativeControlDevice::Gamepad, 1U);
  const cr::CreativeControlBindingRow* inventory =
      findRow(rows, cr::CreativeInputActionId::ToggleCatalog,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* frameContext =
      findRow(rows, cr::CreativeInputActionId::FrameContext3D,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* runtimeAttackMouse =
      findRow(rows, cr::CreativeInputActionId::RuntimeAttack,
              cr::CreativeControlDevice::KeyboardMouse);
  const cr::CreativeControlBindingRow* runtimeAttackPad =
      findRow(rows, cr::CreativeInputActionId::RuntimeAttack,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* runtimeInteractMouse =
      findRow(rows, cr::CreativeInputActionId::RuntimeInteract,
              cr::CreativeControlDevice::KeyboardMouse);
  const cr::CreativeControlBindingRow* runtimeInteractPad =
      findRow(rows, cr::CreativeInputActionId::RuntimeInteract,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeInputBindingAuditResult audit =
      cr::auditCreativeInputBindings(profile.bindingSpan());
  if (audit.conflictCount != 0U) {
    for (const cr::CreativeInputBindingConflict& conflict :
         audit.conflictItems()) {
      std::cerr << "CONFLICT " << conflict.firstBindingIndex << ' '
                << cr::toString(conflict.firstAction) << " / "
                << conflict.secondBindingIndex << ' '
                << cr::toString(conflict.secondAction) << " key="
                << cr::toString(conflict.trigger) << " context="
                << cr::toString(conflict.context) << '\n';
    }
  }

  return expect(profile.bindingCount <= cr::kCreativeInputBindingCapacity &&
                    profile.groupCount <= profile.bindingCount,
                "default profile stays inside fixed capacity") &&
         expect(cr::isValidCreativeControlProfile(profile),
                "default profile is conflict free") &&
         expect(!rows.capacityExceeded && rows.count > 40U,
                "configurable groups are deduplicated and bounded") &&
         expect(forward != nullptr && forward->trigger == cr::CreativeInputKey::W,
                "W remains move forward") &&
         expect(primaryMouse != nullptr &&
                    primaryMouse->trigger == cr::CreativeInputKey::MousePrimary,
                "left mouse remains primary action") &&
         expect(primaryPad == nullptr && secondaryPad == nullptr,
                "gamepad does not inherit mouse primary secondary jargon") &&
         expect(acceptPad != nullptr &&
                    acceptPad->trigger ==
                        cr::CreativeInputKey::GamepadConfirm &&
                    rejectPad != nullptr &&
                    rejectPad->trigger ==
                        cr::CreativeInputKey::GamepadCancel,
                "X accepts and Circle rejects in the viewport") &&
         expect(flyUpPad != nullptr &&
                    flyUpPad->trigger ==
                        cr::CreativeInputKey::GamepadRightTrigger &&
                    flyDownPad != nullptr &&
                    flyDownPad->trigger ==
                        cr::CreativeInputKey::GamepadLeftTrigger,
                "R2 raises and L2 lowers flight") &&
         expect(toolWheelPad != nullptr &&
                    toolWheelPad->trigger ==
                        cr::CreativeInputKey::GamepadRightStick &&
                    pickPad != nullptr &&
                    pickPad->trigger ==
                        cr::CreativeInputKey::GamepadTouchpad &&
                    cycleSettingPad != nullptr &&
                    cycleSettingPad->trigger ==
                        cr::CreativeInputKey::GamepadWest,
                "R3 opens tools, Square cycles settings, and touchpad picks") &&
         expect(inventory != nullptr &&
                    inventory->trigger ==
                        cr::CreativeInputKey::GamepadInventory,
                "Triangle remains inventory") &&
         expect(frameContext != nullptr &&
                    frameContext->trigger ==
                        cr::CreativeInputKey::GamepadBack,
                "PS5 Create frames selection or the full scene") &&
         expect(runtimeAttackMouse != nullptr &&
                    runtimeAttackMouse->trigger ==
                        cr::CreativeInputKey::MousePrimary &&
                    runtimeAttackPad != nullptr &&
                    runtimeAttackPad->trigger ==
                        cr::CreativeInputKey::GamepadRightTrigger &&
                    runtimeInteractMouse != nullptr &&
                    runtimeInteractMouse->trigger ==
                        cr::CreativeInputKey::MouseSecondary &&
                    runtimeInteractPad != nullptr &&
                    runtimeInteractPad->trigger ==
                        cr::CreativeInputKey::GamepadLeftTrigger,
                "Play maps mouse buttons and PS5 triggers by semantic action") &&
         expect(cr::creativeControlKeyDisplayLabel(
                    cr::CreativeInputKey::GamepadConfirm) == "X" &&
                    cr::creativeControlKeyDisplayLabel(
                        cr::CreativeInputKey::GamepadTouchpad) == "Touchpad" &&
                    cr::creativeControlKeyDisplayLabel(
                        cr::CreativeInputKey::GamepadStart) == "Options",
                "PS5 labels describe physical controls");
}

bool continuousBindingsAreQueryableWithoutEdgeEvents() {
  const cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  cr::CreativeInputFrame frame;
  frame.context = cr::CreativeInputContext::EditorViewport;
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::W, true);
  cr::CreativeInputRouterState router;
  const cr::CreativeInputRouteResult routed =
      cr::routeCreativeInput(router, frame, profile.bindingSpan());

  return expect(cr::creativeInputActionDown(
                    frame, cr::CreativeInputActionId::MoveForward,
                    profile.bindingSpan()),
                "continuous movement action is down") &&
         expect(routed.actionCount == 0U,
                "continuous action does not emit command edge") &&
         expect(!cr::creativeInputKeyConsumed(routed, cr::CreativeInputKey::W),
                "continuous action leaves physical key available");
}

bool requiredPs5ActionsRemainReachable() {
  const cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const std::span<const cr::CreativeControlReachabilityRequirement>
      requirements = cr::defaultCreativeGamepadReachabilityRequirements();
  const cr::CreativeControlReachabilityAuditResult defaults =
      cr::auditCreativeControlReachability(profile.bindingSpan(), requirements);

  std::vector<cr::CreativeInputBinding> withoutAccept(
      profile.bindingSpan().begin(), profile.bindingSpan().end());
  const auto accept = std::find_if(
      withoutAccept.begin(), withoutAccept.end(),
      [](const cr::CreativeInputBinding& binding) {
        return binding.action == cr::CreativeInputActionId::AcceptAction &&
               binding.context == cr::CreativeInputContext::EditorViewport &&
               binding.trigger == cr::CreativeInputKey::GamepadConfirm;
      });
  if (accept == withoutAccept.end()) {
    return expect(false, "PS5 viewport accept binding exists");
  }
  const std::size_t acceptIndex =
      static_cast<std::size_t>(std::distance(withoutAccept.begin(), accept));
  withoutAccept.erase(accept);
  const cr::CreativeControlReachabilityAuditResult removed =
      cr::auditCreativeControlReachability(withoutAccept, requirements);

  std::vector<cr::CreativeInputBinding> wrongDevice(
      profile.bindingSpan().begin(), profile.bindingSpan().end());
  wrongDevice[acceptIndex].trigger = cr::CreativeInputKey::Enter;
  const cr::CreativeControlReachabilityAuditResult deviceMismatch =
      cr::auditCreativeControlReachability(wrongDevice, requirements);

  std::vector<cr::CreativeInputBinding> wrongActivation(
      profile.bindingSpan().begin(), profile.bindingSpan().end());
  wrongActivation[acceptIndex].activation =
      cr::CreativeInputBindingActivation::Press;
  const cr::CreativeControlReachabilityAuditResult activationMismatch =
      cr::auditCreativeControlReachability(wrongActivation, requirements);

  const std::array invalidRequirements{
      cr::CreativeControlReachabilityRequirement{
          cr::CreativeInputActionId::Count,
          cr::CreativeInputContext::EditorViewport,
          cr::CreativeControlDevice::Gamepad,
          cr::CreativeInputBindingActivation::Press},
  };
  const cr::CreativeControlReachabilityAuditResult invalid =
      cr::auditCreativeControlReachability(profile.bindingSpan(),
                                            invalidRequirements);

  const auto missingAccept = [](const auto& audit) {
    return audit.issueCount == 1U &&
           audit.issues[0].kind ==
               cr::CreativeControlReachabilityIssueKind::MissingBinding &&
           audit.issues[0].requirement.action ==
               cr::CreativeInputActionId::AcceptAction &&
           audit.issues[0].requirement.context ==
               cr::CreativeInputContext::EditorViewport;
  };

  return expect(requirements.size() > 70U &&
                    requirements.size() <=
                        cr::kCreativeControlReachabilityRequirementCapacity,
                "PS5 release matrix is substantial and fixed-capacity") &&
         expect(defaults.issueCount == 0U &&
                    !defaults.bindingCapacityExceeded &&
                    !defaults.requirementCapacityExceeded,
                "default profile reaches every required PS5 action") &&
         expect(missingAccept(removed),
                "removed X binding reports one unreachable action") &&
         expect(missingAccept(deviceMismatch),
                "keyboard replacement does not satisfy PS5 reachability") &&
         expect(missingAccept(activationMismatch),
                "press binding does not satisfy continuous held action") &&
         expect(invalid.issueCount == 1U &&
                    invalid.issues[0].kind ==
                        cr::CreativeControlReachabilityIssueKind::
                            InvalidRequirement,
                "invalid release requirement fails closed");
}

bool conflictPoliciesRejectReplaceAndSwapDeterministically() {
  const cr::CreativeControlProfile defaults =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(defaults);
  const cr::CreativeControlBindingRow* copy =
      findRow(rows, cr::CreativeInputActionId::CopySelection,
              cr::CreativeControlDevice::KeyboardMouse);
  const cr::CreativeControlBindingRow* paste =
      findRow(rows, cr::CreativeInputActionId::PasteClipboard,
              cr::CreativeControlDevice::KeyboardMouse);
  if (copy == nullptr || paste == nullptr) {
    return expect(false, "copy and paste rows exist");
  }

  cr::CreativeControlProfile rejected = defaults;
  const cr::CreativeControlRebindReceipt reject = cr::rebindCreativeControl(
      rejected,
      {paste->group, cr::CreativeInputKey::C,
       cr::kCreativeInputModifierCommand,
       cr::CreativeControlConflictPolicy::Reject});

  cr::CreativeControlProfile replaced = defaults;
  const cr::CreativeControlRebindReceipt replace = cr::rebindCreativeControl(
      replaced,
      {paste->group, cr::CreativeInputKey::C,
       cr::kCreativeInputModifierCommand,
       cr::CreativeControlConflictPolicy::Replace});
  const cr::CreativeInputBinding* replacedCopy =
      cr::creativeControlGroupBinding(replaced, copy->group);

  cr::CreativeControlProfile swapped = defaults;
  const cr::CreativeControlRebindReceipt swap = cr::rebindCreativeControl(
      swapped,
      {paste->group, cr::CreativeInputKey::C,
       cr::kCreativeInputModifierCommand,
       cr::CreativeControlConflictPolicy::Swap});
  const cr::CreativeInputBinding* swappedCopy =
      cr::creativeControlGroupBinding(swapped, copy->group);
  const cr::CreativeInputBinding* swappedPaste =
      cr::creativeControlGroupBinding(swapped, paste->group);

  return expect(reject.status == cr::CreativeControlRebindStatus::Conflict &&
                    !reject.changed && reject.conflictCount == 1U &&
                    cr::isValidCreativeControlProfile(rejected),
                "reject reports conflict without mutation") &&
         expect(replace.status == cr::CreativeControlRebindStatus::Applied &&
                    replace.changed && replacedCopy != nullptr &&
                    replacedCopy->trigger == cr::CreativeInputKey::Unbound &&
                    cr::isValidCreativeControlProfile(replaced),
                "replace unbinds the displaced group") &&
         expect(swap.status == cr::CreativeControlRebindStatus::Applied &&
                    swappedCopy != nullptr && swappedPaste != nullptr &&
                    swappedCopy->trigger == cr::CreativeInputKey::V &&
                    swappedPaste->trigger == cr::CreativeInputKey::C &&
                    cr::isValidCreativeControlProfile(swapped),
                "swap exchanges complete chords");
}

bool deviceAndReservedBoundariesFailClosed() {
  cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(profile);
  const cr::CreativeControlBindingRow* copy =
      findRow(rows, cr::CreativeInputActionId::CopySelection,
              cr::CreativeControlDevice::KeyboardMouse);
  if (copy == nullptr) {
    return expect(false, "copy row exists");
  }
  const cr::CreativeControlRebindReceipt mismatch = cr::rebindCreativeControl(
      profile,
      {copy->group, cr::CreativeInputKey::GamepadWest,
       cr::kCreativeInputModifierNone,
       cr::CreativeControlConflictPolicy::Reject});

  const auto reserved = std::find_if(
      profile.groupActions.begin(),
      profile.groupActions.begin() + profile.groupCount,
      [](cr::CreativeInputActionId action) {
        return action == cr::CreativeInputActionId::ToggleControls;
      });
  if (reserved == profile.groupActions.begin() + profile.groupCount) {
    return expect(false, "reserved controls group exists");
  }
  const std::uint16_t reservedGroup = static_cast<std::uint16_t>(
      std::distance(profile.groupActions.begin(), reserved));
  const cr::CreativeControlRebindReceipt reservedReceipt =
      cr::rebindCreativeControl(
          profile,
          {reservedGroup, cr::CreativeInputKey::E,
           cr::kCreativeInputModifierNone,
           cr::CreativeControlConflictPolicy::Replace});

  return expect(mismatch.status ==
                    cr::CreativeControlRebindStatus::DeviceMismatch,
                "keyboard group rejects gamepad key") &&
         expect(reservedReceipt.status ==
                    cr::CreativeControlRebindStatus::ReservedAction,
                "controls escape hatch cannot be rebound");
}

bool standaloneModifierKeysRemainBindable() {
  cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(profile);
  const cr::CreativeControlBindingRow* copy =
      findRow(rows, cr::CreativeInputActionId::CopySelection,
              cr::CreativeControlDevice::KeyboardMouse);
  if (copy == nullptr) {
    return expect(false, "copy row exists for modifier-key rebind");
  }
  const cr::CreativeControlRebindReceipt receipt = cr::rebindCreativeControl(
      profile,
      {copy->group, cr::CreativeInputKey::LeftAlt,
       cr::kCreativeInputModifierAlt,
       cr::CreativeControlConflictPolicy::Reject});
  const cr::CreativeInputBinding* rebound =
      cr::creativeControlGroupBinding(profile, copy->group);

  cr::CreativeInputFrame frame;
  frame.context = cr::CreativeInputContext::EditorViewport;
  frame.modifiers = cr::kCreativeInputModifierAlt;
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::LeftAlt, true);
  cr::CreativeInputRouterState router;
  const cr::CreativeInputRouteResult routed =
      cr::routeCreativeInput(router, frame, profile.bindingSpan());
  const bool copyEmitted = std::any_of(
      routed.actionEvents().begin(), routed.actionEvents().end(),
      [](const cr::CreativeInputActionEvent& event) {
        return event.action == cr::CreativeInputActionId::CopySelection;
      });

  return expect(receipt.status == cr::CreativeControlRebindStatus::Applied &&
                    rebound != nullptr &&
                    rebound->trigger == cr::CreativeInputKey::LeftAlt &&
                    rebound->requiredAllModifiers ==
                        cr::kCreativeInputModifierNone &&
                    rebound->allowedModifiers ==
                        cr::kCreativeInputModifierAlt,
                "standalone modifier rebind has a routable chord") &&
         expect(copyEmitted, "standalone modifier emits its semantic action") &&
         expect(cr::isValidCreativeControlProfile(profile),
                "modifier-key profile remains valid");
}

bool reservedBindingsAndConsumedHeldActionsStayIsolated() {
  cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(profile);
  const cr::CreativeControlBindingRow* catalog =
      findRow(rows, cr::CreativeInputActionId::ToggleCatalog,
              cr::CreativeControlDevice::KeyboardMouse);
  if (catalog == nullptr) {
    return expect(false, "catalog row exists for conflict checks");
  }

  const cr::CreativeControlRebindReceipt reserved = cr::rebindCreativeControl(
      profile,
      {catalog->group, cr::CreativeInputKey::Escape,
       cr::kCreativeInputModifierNone,
       cr::CreativeControlConflictPolicy::Replace});
  const auto controlsGroup = std::find(
      profile.groupActions.begin(),
      profile.groupActions.begin() + profile.groupCount,
      cr::CreativeInputActionId::ToggleControls);
  const cr::CreativeInputBinding* controlsBinding =
      controlsGroup == profile.groupActions.begin() + profile.groupCount
          ? nullptr
          : cr::creativeControlGroupBinding(
                profile,
                static_cast<std::uint16_t>(std::distance(
                    profile.groupActions.begin(), controlsGroup)));

  cr::CreativeInputFrame frame;
  frame.context = cr::CreativeInputContext::EditorViewport;
  frame.modifiers = cr::kCreativeInputModifierCommand;
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::S, true);
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::LeftCommand, true);
  cr::CreativeInputRouterState router;
  const cr::CreativeInputRouteResult routed =
      cr::routeCreativeInput(router, frame, profile.bindingSpan());

  return expect(reserved.status ==
                    cr::CreativeControlRebindStatus::ReservedAction &&
                    controlsBinding != nullptr &&
                    controlsBinding->trigger == cr::CreativeInputKey::Escape,
                "replace cannot steal the Controls escape hatch") &&
         expect(!cr::creativeInputActionDown(
                    frame, cr::CreativeInputActionId::MoveBackward,
                    profile.bindingSpan(), &routed),
                "consumed command chord suppresses its held movement action") &&
         expect(cr::isValidCreativeControlProfile(profile),
                "rejected conflicts leave the profile valid");
}

bool settingsStayBoundedAndIndependentFromBindings() {
  cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const std::size_t bindingCount = profile.bindingCount;
  for (int index = 0; index < 100; ++index) {
    static_cast<void>(cr::adjustCreativeControlSetting(
        profile, cr::CreativeControlSettingId::LookDeadzone, 1));
    static_cast<void>(cr::adjustCreativeControlSetting(
        profile, cr::CreativeControlSettingId::MouseLookSensitivity, -1));
  }
  static_cast<void>(cr::adjustCreativeControlSetting(
      profile, cr::CreativeControlSettingId::InvertLookY, 1));

  return expect(profile.lookStick.deadzone == 0.75F &&
                    profile.mouseLookSensitivity == 0.02F,
                "numeric settings clamp to documented limits") &&
         expect(profile.lookStick.invertY,
                "binary look inversion toggles") &&
         expect(profile.bindingCount == bindingCount &&
                    cr::isValidCreativeControlProfile(profile),
                "tuning does not alter binding ownership");
}

}  // namespace

int main() {
  bool ok = true;
  ok = defaultsAreBoundedConflictFreeAndMinecraftShaped() && ok;
  ok = continuousBindingsAreQueryableWithoutEdgeEvents() && ok;
  ok = requiredPs5ActionsRemainReachable() && ok;
  ok = conflictPoliciesRejectReplaceAndSwapDeterministically() && ok;
  ok = deviceAndReservedBoundariesFailClosed() && ok;
  ok = standaloneModifierKeysRemainBindable() && ok;
  ok = reservedBindingsAndConsumedHeldActionsStayIsolated() && ok;
  ok = settingsStayBoundedAndIndependentFromBindings() && ok;
  return ok ? 0 : 1;
}
