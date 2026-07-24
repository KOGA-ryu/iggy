#include "app/iggy3d/creative/input/WorldActionIntent.hpp"

#include <array>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr std::size_t intentIndex(
    CreativeWorldIntentId intent) noexcept {
  return static_cast<std::size_t>(intent);
}

[[nodiscard]] constexpr std::size_t policyIndex(
    CreativeWorldIntentPolicy policy) noexcept {
  return static_cast<std::size_t>(policy);
}

[[nodiscard]] consteval auto makePolicyDefinitions() {
  std::array<CreativeWorldIntentPolicyDefinition,
             static_cast<std::size_t>(CreativeWorldIntentPolicy::Count)>
      rows{};

  {
    auto& row = rows[policyIndex(CreativeWorldIntentPolicy::Placement)];
    row.policy = CreativeWorldIntentPolicy::Placement;
    row.bindings[intentIndex(CreativeWorldIntentId::Positive)] = {
        CreativeWorldActionId::Secondary,
        CreativeWorldActionId::Accept};
    row.bindings[intentIndex(CreativeWorldIntentId::Negative)] = {
        CreativeWorldActionId::Primary,
        CreativeWorldActionId::Reject};
  }
  {
    auto& row = rows[policyIndex(CreativeWorldIntentPolicy::Manipulation)];
    row.policy = CreativeWorldIntentPolicy::Manipulation;
    row.bindings[intentIndex(CreativeWorldIntentId::Positive)] = {
        CreativeWorldActionId::Primary,
        CreativeWorldActionId::Accept};
    row.bindings[intentIndex(CreativeWorldIntentId::Negative)] = {
        CreativeWorldActionId::Count,
        CreativeWorldActionId::Reject};
    row.bindings[intentIndex(CreativeWorldIntentId::Alternate)] = {
        CreativeWorldActionId::Secondary,
        CreativeWorldActionId::Count};
  }
  return rows;
}

constexpr auto kPolicyDefinitions = makePolicyDefinitions();
constexpr CreativeWorldIntentPolicyDefinition kInvalidPolicyDefinition{};
constexpr std::array kWorldInputActions{
    CreativeInputActionId::PrimaryAction,
    CreativeInputActionId::SecondaryAction,
    CreativeInputActionId::AcceptAction,
    CreativeInputActionId::RejectAction,
    CreativeInputActionId::PickAction,
    CreativeInputActionId::HotbarPrevious,
    CreativeInputActionId::HotbarNext,
};
static_assert(kWorldInputActions.size() == kCreativeWorldActionCount);

[[nodiscard]] consteval bool policyDefinitionsAreDisjoint() {
  for (const CreativeWorldIntentPolicyDefinition& policy :
       kPolicyDefinitions) {
    for (std::size_t first = 0U; first < policy.bindings.size(); ++first) {
      for (std::size_t second = first + 1U;
           second < policy.bindings.size(); ++second) {
        const CreativeWorldIntentBinding& lhs = policy.bindings[first];
        const CreativeWorldIntentBinding& rhs = policy.bindings[second];
        if (lhs.keyboardMouseAction != CreativeWorldActionId::Count &&
            lhs.keyboardMouseAction == rhs.keyboardMouseAction) {
          return false;
        }
        if (lhs.gamepadAction != CreativeWorldActionId::Count &&
            lhs.gamepadAction == rhs.gamepadAction) {
          return false;
        }
      }
    }
  }
  return true;
}

static_assert(policyDefinitionsAreDisjoint());

template <typename Predicate>
[[nodiscard]] bool bindingMatches(
    const CreativeWorldActionFrame& frame,
    const CreativeWorldIntentBinding& binding,
    Predicate predicate) noexcept {
  return (binding.keyboardMouseAction != CreativeWorldActionId::Count &&
          predicate(frame, binding.keyboardMouseAction)) ||
         (binding.gamepadAction != CreativeWorldActionId::Count &&
          predicate(frame, binding.gamepadAction));
}

}  // namespace

const CreativeWorldIntentPolicyDefinition& describeCreativeWorldIntentPolicy(
    CreativeWorldIntentPolicy policy) noexcept {
  const std::size_t index = policyIndex(policy);
  return index < kPolicyDefinitions.size() ? kPolicyDefinitions[index]
                                           : kInvalidPolicyDefinition;
}

CreativeWorldIntentFrame resolveCreativeWorldIntents(
    const CreativeWorldActionFrame& actions,
    CreativeWorldIntentPolicy policy) noexcept {
  CreativeWorldIntentFrame frame;
  const CreativeWorldIntentPolicyDefinition& definition =
      describeCreativeWorldIntentPolicy(policy);
  if (definition.policy == CreativeWorldIntentPolicy::Count) {
    return frame;
  }
  for (std::size_t index = 0U; index < definition.bindings.size(); ++index) {
    const CreativeWorldIntentBinding& binding = definition.bindings[index];
    frame.down[index] =
        bindingMatches(actions, binding, creativeWorldActionDown);
    frame.pressed[index] =
        bindingMatches(actions, binding, creativeWorldActionPressed);
    frame.released[index] =
        bindingMatches(actions, binding, creativeWorldActionReleased);
  }
  return frame;
}

bool creativeWorldIntentDown(const CreativeWorldIntentFrame& frame,
                             CreativeWorldIntentId intent) noexcept {
  const std::size_t index = intentIndex(intent);
  return index < frame.down.size() && frame.down[index];
}

bool creativeWorldIntentPressed(const CreativeWorldIntentFrame& frame,
                                CreativeWorldIntentId intent) noexcept {
  const std::size_t index = intentIndex(intent);
  return index < frame.pressed.size() && frame.pressed[index];
}

bool creativeWorldIntentReleased(const CreativeWorldIntentFrame& frame,
                                 CreativeWorldIntentId intent) noexcept {
  const std::size_t index = intentIndex(intent);
  return index < frame.released.size() && frame.released[index];
}

CreativeInputActionId creativeWorldIntentInputAction(
    CreativeWorldIntentPolicy policy,
    CreativeWorldIntentId intent,
    CreativeControlDevice device) noexcept {
  const CreativeWorldIntentPolicyDefinition& definition =
      describeCreativeWorldIntentPolicy(policy);
  const std::size_t index = intentIndex(intent);
  if (index >= definition.bindings.size()) {
    return CreativeInputActionId::Count;
  }
  const CreativeWorldIntentBinding& binding = definition.bindings[index];
  CreativeWorldActionId action = CreativeWorldActionId::Count;
  switch (device) {
    case CreativeControlDevice::KeyboardMouse:
      action = binding.keyboardMouseAction;
      break;
    case CreativeControlDevice::Gamepad:
      action = binding.gamepadAction;
      break;
    case CreativeControlDevice::Count:
      break;
  }
  return creativeWorldInputAction(action);
}

CreativeInputActionId creativeWorldInputAction(
    CreativeWorldActionId action) noexcept {
  const std::size_t index = static_cast<std::size_t>(action);
  return index < kWorldInputActions.size() ? kWorldInputActions[index]
                                           : CreativeInputActionId::Count;
}

CreativeWorldInputSample sampleCreativeWorldInput(
    const CreativeInputRouteResult& routedInput,
    bool enabled,
    std::int32_t hotbarWheelSteps) noexcept {
  CreativeWorldInputSample sample;
  if (!enabled) {
    return sample;
  }
  for (std::size_t index = 0U; index < kCreativeWorldActionCount; ++index) {
    const CreativeWorldActionId worldAction =
        static_cast<CreativeWorldActionId>(index);
    setCreativeWorldAction(
        sample, worldAction,
        creativeInputActionDown(routedInput,
                                creativeWorldInputAction(worldAction)));
  }
  sample.hotbarWheelSteps = hotbarWheelSteps;
  return sample;
}

}  // namespace iggy3d::creative
