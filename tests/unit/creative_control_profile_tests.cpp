#include "app/iggy3d/creative/input/ControlProfile.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>

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
  const cr::CreativeControlBindingRow* inventory =
      findRow(rows, cr::CreativeInputActionId::ToggleCatalog,
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
         expect(primaryPad != nullptr &&
                    primaryPad->trigger ==
                        cr::CreativeInputKey::GamepadRightTrigger,
                "R2 remains primary action") &&
         expect(inventory != nullptr &&
                    inventory->trigger ==
                        cr::CreativeInputKey::GamepadInventory,
                "Triangle remains inventory") &&
         expect(cr::creativeControlKeyDisplayLabel(
                    cr::CreativeInputKey::GamepadConfirm) == "Cross" &&
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
  ok = conflictPoliciesRejectReplaceAndSwapDeterministically() && ok;
  ok = deviceAndReservedBoundariesFailClosed() && ok;
  ok = standaloneModifierKeysRemainBindable() && ok;
  ok = reservedBindingsAndConsumedHeldActionsStayIsolated() && ok;
  ok = settingsStayBoundedAndIndependentFromBindings() && ok;
  return ok ? 0 : 1;
}
