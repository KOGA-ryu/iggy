#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace iggy3d::creative {
namespace {

[[nodiscard]] std::optional<CreativeObjectKind> authoredKindForLogicTarget(
    CreativeRuntimeInteractableKind kind) noexcept {
  switch (kind) {
    case CreativeRuntimeInteractableKind::Door:
      return CreativeObjectKind::Door;
    case CreativeRuntimeInteractableKind::Platform:
      return CreativeObjectKind::Platform;
    case CreativeRuntimeInteractableKind::MovingPlatform:
      return CreativeObjectKind::MovingPlatform;
    case CreativeRuntimeInteractableKind::Control:
    case CreativeRuntimeInteractableKind::Pickup:
    case CreativeRuntimeInteractableKind::Objective:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace

bool creativeRuntimeInteractableIsLogicTarget(
    CreativeRuntimeInteractableKind kind) noexcept {
  return kind == CreativeRuntimeInteractableKind::Door ||
         kind == CreativeRuntimeInteractableKind::Platform ||
         kind == CreativeRuntimeInteractableKind::MovingPlatform;
}

bool creativeRuntimeLogicActionSupported(
    CreativeRuntimeInteractableKind targetKind,
    CreativeLogicLinkAction action) noexcept {
  const std::optional<CreativeObjectKind> authoredKind =
      authoredKindForLogicTarget(targetKind);
  return authoredKind.has_value() &&
         creativeLogicLinkActionSupported(*authoredKind, action);
}

std::string_view toString(
    CreativeRuntimeOccupancyTransition transition) noexcept {
  switch (transition) {
    case CreativeRuntimeOccupancyTransition::None:
      return "none";
    case CreativeRuntimeOccupancyTransition::Entered:
      return "entered";
    case CreativeRuntimeOccupancyTransition::Exited:
      return "exited";
  }
  return "none";
}

CreativeRuntimeLogicActivationPlan planCreativeRuntimeLogicActivation(
    std::span<const CreativeRuntimeLogicLink> links,
    std::span<const CreativeRuntimeLogicTargetStateFact> targets,
    CreativeObjectId sourceObjectId,
    CreativeRuntimeLogicSignal signal) {
  CreativeRuntimeLogicActivationPlan result;
  if (sourceObjectId == kInvalidObjectId ||
      static_cast<std::uint8_t>(signal) >=
          static_cast<std::uint8_t>(CreativeRuntimeLogicSignal::Count)) {
    result.reasonCode = "creative_runtime_logic_plan_request_invalid";
    return result;
  }
  for (std::size_t index = 0U; index < targets.size(); ++index) {
    bool duplicate = false;
    for (std::size_t prior = 0U; prior < index; ++prior) {
      duplicate = duplicate ||
                  targets[prior].objectId == targets[index].objectId;
    }
    if (targets[index].objectId == kInvalidObjectId ||
        !creativeRuntimeInteractableIsLogicTarget(targets[index].kind) ||
        duplicate) {
      result.reasonCode = "creative_runtime_logic_plan_target_facts_invalid";
      return result;
    }
  }

  std::vector<const CreativeRuntimeLogicLink*> sourceLinks;
  sourceLinks.reserve(links.size());
  for (const CreativeRuntimeLogicLink& link : links) {
    if (link.sourceObjectId == sourceObjectId) {
      sourceLinks.push_back(&link);
    }
  }
  if (sourceLinks.empty()) {
    result.ok = true;
    result.reasonCode = "creative_runtime_logic_plan_no_linked_target";
    return result;
  }

  const bool compatibilityFallback =
      sourceLinks.front()->compatibilityFallback;
  if (std::any_of(sourceLinks.begin(), sourceLinks.end(),
                  [compatibilityFallback](const auto* link) {
                    return link->compatibilityFallback !=
                           compatibilityFallback;
                  })) {
    result.reasonCode = "creative_runtime_logic_plan_link_modes_mixed";
    return result;
  }
  result.compatibilityFallback = compatibilityFallback;

  bool compatibilityActive = false;
  if (compatibilityFallback) {
    const bool compatibleDoorLinks = std::all_of(
        sourceLinks.begin(), sourceLinks.end(), [targets](const auto* link) {
          const auto target = std::find_if(
              targets.begin(), targets.end(), [link](const auto& fact) {
                return fact.objectId == link->targetObjectId;
              });
          return link->action == CreativeLogicLinkAction::Toggle &&
                 target != targets.end() &&
                 target->kind == CreativeRuntimeInteractableKind::Door;
        });
    if (!compatibleDoorLinks) {
      result.reasonCode =
          "creative_runtime_logic_plan_compatibility_target_invalid";
      return result;
    }
    if (signal == CreativeRuntimeLogicSignal::Pulse) {
      compatibilityActive = std::any_of(
          sourceLinks.begin(), sourceLinks.end(),
          [targets](const auto* link) {
            const auto target = std::find_if(
                targets.begin(), targets.end(), [link](const auto& fact) {
                  return fact.objectId == link->targetObjectId;
                });
            return target != targets.end() && !target->active;
          });
    } else {
      compatibilityActive = signal == CreativeRuntimeLogicSignal::Activate;
    }
  }

  result.commands.reserve(sourceLinks.size());
  for (const CreativeRuntimeLogicLink* link : sourceLinks) {
    const auto target = std::find_if(
        targets.begin(), targets.end(), [link](const auto& fact) {
          return fact.objectId == link->targetObjectId;
        });
    if (target == targets.end() ||
        std::any_of(result.commands.begin(), result.commands.end(),
                    [link](const auto& command) {
                      return command.objectId == link->targetObjectId;
                    })) {
      result.commands.clear();
      result.reasonCode = "creative_runtime_logic_plan_target_invalid";
      return result;
    }

    bool active = compatibilityActive;
    bool reverse = false;
    if (!compatibilityFallback) {
      if (!creativeRuntimeLogicActionSupported(target->kind, link->action)) {
        result.commands.clear();
        result.reasonCode = "creative_runtime_logic_plan_action_invalid";
        return result;
      }
      switch (link->action) {
        case CreativeLogicLinkAction::Toggle:
          active = !target->active;
          break;
        case CreativeLogicLinkAction::Open:
        case CreativeLogicLinkAction::Enable:
          active = signal != CreativeRuntimeLogicSignal::Deactivate;
          break;
        case CreativeLogicLinkAction::Close:
        case CreativeLogicLinkAction::Disable:
          active = signal == CreativeRuntimeLogicSignal::Deactivate;
          break;
        case CreativeLogicLinkAction::Reverse:
          active = target->active;
          reverse = signal != CreativeRuntimeLogicSignal::Deactivate;
          break;
        case CreativeLogicLinkAction::Count:
          result.commands.clear();
          result.reasonCode = "creative_runtime_logic_plan_action_invalid";
          return result;
      }
    }
    result.commands.push_back(
        {link->targetObjectId, target->kind, active, reverse});
  }

  result.ok = true;
  result.reasonCode = "creative_runtime_logic_plan_built";
  return result;
}

}  // namespace iggy3d::creative
