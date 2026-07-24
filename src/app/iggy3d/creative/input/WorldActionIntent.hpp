#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"

namespace iggy3d::creative {

// Policies convert device-shaped world actions into the intent vocabulary used
// by an interaction family. A policy never maps one source action to two
// intents, so mouse and controller input can be coalesced without double edits.
enum class CreativeWorldIntentPolicy : std::uint8_t {
  Placement,
  Manipulation,
  Count,
};

enum class CreativeWorldIntentId : std::uint8_t {
  Positive,
  Negative,
  Alternate,
  Count,
};

inline constexpr std::size_t kCreativeWorldIntentCount =
    static_cast<std::size_t>(CreativeWorldIntentId::Count);

struct CreativeWorldIntentBinding {
  CreativeWorldActionId keyboardMouseAction = CreativeWorldActionId::Count;
  CreativeWorldActionId gamepadAction = CreativeWorldActionId::Count;
};

struct CreativeWorldIntentPolicyDefinition {
  CreativeWorldIntentPolicy policy = CreativeWorldIntentPolicy::Count;
  std::array<CreativeWorldIntentBinding, kCreativeWorldIntentCount> bindings{};
};

struct CreativeWorldIntentFrame {
  std::array<bool, kCreativeWorldIntentCount> down{};
  std::array<bool, kCreativeWorldIntentCount> pressed{};
  std::array<bool, kCreativeWorldIntentCount> released{};
};

static_assert(std::is_trivially_copyable_v<CreativeWorldIntentBinding>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldIntentPolicyDefinition>);
static_assert(std::is_trivially_copyable_v<CreativeWorldIntentFrame>);

[[nodiscard]] const CreativeWorldIntentPolicyDefinition&
describeCreativeWorldIntentPolicy(CreativeWorldIntentPolicy policy) noexcept;
[[nodiscard]] CreativeWorldIntentFrame resolveCreativeWorldIntents(
    const CreativeWorldActionFrame& actions,
    CreativeWorldIntentPolicy policy) noexcept;
[[nodiscard]] bool creativeWorldIntentDown(
    const CreativeWorldIntentFrame& frame,
    CreativeWorldIntentId intent) noexcept;
[[nodiscard]] bool creativeWorldIntentPressed(
    const CreativeWorldIntentFrame& frame,
    CreativeWorldIntentId intent) noexcept;
[[nodiscard]] bool creativeWorldIntentReleased(
    const CreativeWorldIntentFrame& frame,
    CreativeWorldIntentId intent) noexcept;
[[nodiscard]] CreativeInputActionId creativeWorldIntentInputAction(
    CreativeWorldIntentPolicy policy,
    CreativeWorldIntentId intent,
    CreativeControlDevice device) noexcept;
[[nodiscard]] CreativeInputActionId creativeWorldInputAction(
    CreativeWorldActionId action) noexcept;

[[nodiscard]] CreativeWorldActionFrame routeCreativeWorldActions(
    CreativeWorldActionRouterState& state,
    const CreativeInputRouteResult& routedInput,
    bool enabled,
    std::int32_t hotbarWheelSteps) noexcept;

}  // namespace iggy3d::creative
