#include "app/iggy3d/creative/play/PlaySession.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

#include "render/debug/DebugHudText.hpp"
#include "runtime/ai/SegmentOcclusion.hpp"
#include "runtime/collision/EntityHitQuery.hpp"
#include "runtime/combat/CombatSystem.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"

namespace iggy3d_creative_app {
namespace {

constexpr iggy3d::EntityId kLocalPlayerEntity{1U};
constexpr float kPlayerEyeHeightMeters = 1.7F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kDirectionEpsilon = 0.000001F;

[[nodiscard]] bool finitePositive(float value) noexcept {
  return std::isfinite(value) && value > 0.0F;
}

[[nodiscard]] bool finiteNonnegative(float value) noexcept {
  return std::isfinite(value) && value >= 0.0F;
}

[[nodiscard]] iggy3d::Vec3 normalizedOrZero(iggy3d::Vec3 value) noexcept {
  const float lengthSquared = iggy3d::lengthSquared(value);
  if (!std::isfinite(lengthSquared) || lengthSquared <= kDirectionEpsilon) {
    return {};
  }
  return value / std::sqrt(lengthSquared);
}

[[nodiscard]] std::pair<std::int32_t, std::int32_t> playerHealth(
    const CreativePlaySession& mode) noexcept {
  if (!mode.sandbox.has_value()) {
    return {0, 0};
  }
  for (const iggy3d::CombatantState& combatant :
       mode.sandbox->session.state().combat.combatants) {
    if (combatant.entity == kLocalPlayerEntity) {
      return {combatant.hitPoints, combatant.maxHitPoints};
    }
  }
  return {0, 0};
}

struct HudColor {
  float r = 1.0F;
  float g = 1.0F;
  float b = 1.0F;
  float a = 1.0F;
};

struct FixedHudText {
  std::array<char, 128U> chars{};

  [[nodiscard]] std::string_view view() const noexcept {
    return chars.data();
  }
};

[[nodiscard]] HudColor targetColor(
    CreativePlayTargetStatus status) noexcept {
  switch (status) {
    case CreativePlayTargetStatus::Valid:
      return {0.28F, 0.95F, 0.43F, 1.0F};
    case CreativePlayTargetStatus::Blocked:
    case CreativePlayTargetStatus::Invalid:
      return {0.95F, 0.25F, 0.23F, 1.0F};
    case CreativePlayTargetStatus::Friendly:
      return {0.98F, 0.84F, 0.24F, 1.0F};
    case CreativePlayTargetStatus::OutOfRange:
      return {1.0F, 0.53F, 0.18F, 1.0F};
    case CreativePlayTargetStatus::None:
    case CreativePlayTargetStatus::Unsupported:
    case CreativePlayTargetStatus::Defeated:
      return {0.72F, 0.76F, 0.79F, 1.0F};
  }
  return {0.72F, 0.76F, 0.79F, 1.0F};
}

void appendRect(CreativePlayHudFrame& hud,
                iggy3d::RenderUiRect rect) noexcept {
  if (rect.width == 0U || rect.height == 0U) {
    return;
  }
  if (hud.rectCount >= hud.rects.size()) {
    hud.capacityExceeded = true;
    return;
  }
  hud.rects[hud.rectCount++] = rect;
}

void appendText(CreativePlayHudFrame& hud,
                std::string_view text,
                std::int32_t x,
                std::int32_t y,
                std::uint32_t viewportWidth,
                std::uint32_t viewportHeight,
                HudColor color) {
  std::span<iggy3d::DebugHudGlyphQuad> remaining{
      hud.glyphQuads.data() + hud.glyphQuadCount,
      hud.glyphQuads.size() - hud.glyphQuadCount};
  const iggy3d::DebugHudFixedLayoutResult layout =
      iggy3d::layoutDebugHudTextAtInto(
          text, x, y, viewportWidth, viewportHeight, remaining);
  if (layout.capacityExceeded) {
    hud.capacityExceeded = true;
    return;
  }
  for (std::size_t index = 0U; index < layout.quadCount; ++index) {
    iggy3d::DebugHudGlyphQuad& quad = remaining[index];
    quad.r = color.r;
    quad.g = color.g;
    quad.b = color.b;
    quad.a = color.a;
  }
  hud.glyphQuadCount += layout.quadCount;
  hud.textGlyphCount += layout.glyphCount;
}

[[nodiscard]] FixedHudText targetHudText(
    const CreativePlayTarget& target,
    std::size_t maximumCharacterCount) {
  FixedHudText result;
  if (target.status == CreativePlayTargetStatus::None) {
    std::snprintf(result.chars.data(), result.chars.size(), "NO TARGET");
    return result;
  }
  constexpr std::size_t kMaximumNameLength = 22U;
  const std::string& label =
      target.displayName.empty() ? target.stableName : target.displayName;
  const int nameLength = static_cast<int>(
      std::min(label.size(), kMaximumNameLength));
  const std::string_view status = toString(target.status);
  const bool actionable =
      (target.status == CreativePlayTargetStatus::Valid ||
       target.status == CreativePlayTargetStatus::Friendly) &&
      !target.actionPrompt.empty();
  if (std::isfinite(target.reachDistanceMeters) &&
      target.reachDistanceMeters > 0.0F) {
    if (actionable) {
      std::snprintf(result.chars.data(), result.chars.size(),
                    "%s %.*s %.1fM", target.actionPrompt.c_str(), nameLength,
                    label.c_str(), target.reachDistanceMeters);
    } else {
      std::snprintf(result.chars.data(), result.chars.size(),
                    "%.*s %.*s %.1fM", nameLength, label.c_str(),
                    static_cast<int>(status.size()), status.data(),
                    target.reachDistanceMeters);
    }
  } else {
    if (actionable) {
      std::snprintf(result.chars.data(), result.chars.size(), "%s %.*s",
                    target.actionPrompt.c_str(), nameLength, label.c_str());
    } else {
      std::snprintf(result.chars.data(), result.chars.size(), "%.*s %.*s",
                    nameLength, label.c_str(), static_cast<int>(status.size()),
                    status.data());
    }
  }
  result.chars[std::min(maximumCharacterCount,
                        result.chars.size() - 1U)] = '\0';
  return result;
}

[[nodiscard]] FixedHudText interactionEffectHudText(
    const iggy3d::creative::CreativeRuntimeInteractionEffectReceipt& effect) {
  FixedHudText result;
  using Status =
      iggy3d::creative::CreativeRuntimeInteractionEffectStatus;
  switch (effect.status) {
    case Status::DoorOpening:
      std::snprintf(result.chars.data(), result.chars.size(), "OPENING %.79s",
                    effect.displayName.c_str());
      break;
    case Status::DoorClosing:
      std::snprintf(result.chars.data(), result.chars.size(), "CLOSING %.79s",
                    effect.displayName.c_str());
      break;
    case Status::CircuitOpened:
      std::snprintf(result.chars.data(), result.chars.size(),
                    "OPENED %zu LINKED DOOR%s", effect.affectedDoorCount,
                    effect.affectedDoorCount == 1U ? "" : "S");
      break;
    case Status::CircuitClosed:
      std::snprintf(result.chars.data(), result.chars.size(),
                    "CLOSED %zu LINKED DOOR%s", effect.affectedDoorCount,
                    effect.affectedDoorCount == 1U ? "" : "S");
      break;
    case Status::LinksApplied:
      std::snprintf(result.chars.data(), result.chars.size(),
                    "ACTIVATED %zu LINKED TARGET%s",
                    effect.affectedTargetCount,
                    effect.affectedTargetCount == 1U ? "" : "S");
      break;
    case Status::LinksNoChange:
      std::snprintf(result.chars.data(), result.chars.size(),
                    "LINKED TARGETS ALREADY SET");
      break;
    case Status::PickupAcquired:
      std::snprintf(result.chars.data(), result.chars.size(), "PICKED UP %.78s",
                    effect.displayName.c_str());
      break;
    case Status::NoLinkedTarget:
      std::snprintf(result.chars.data(), result.chars.size(),
                    "NO LINKED TARGET");
      break;
    case Status::TargetOccupied:
      std::snprintf(result.chars.data(), result.chars.size(),
                    "TARGET BLOCKED: OCCUPIED");
      break;
    case Status::DoorLocked:
      std::snprintf(result.chars.data(), result.chars.size(), "DOOR LOCKED");
      break;
    case Status::NotRequested:
    case Status::TargetMissing:
    case Status::UnsupportedTarget:
    case Status::GeometryRejected:
      break;
  }
  return result;
}

}  // namespace

CreativePlayActionSample sampleCreativePlayActions(
    const iggy3d::creative::CreativeInputFrame& inputFrame,
    const iggy3d::creative::CreativeInputRouteResult& routedInput,
    std::span<const iggy3d::creative::CreativeInputBinding> bindings) noexcept {
  const bool enabled = inputFrame.context ==
                       iggy3d::creative::CreativeInputContext::RuntimePlay;
  iggy3d::creative::CreativeInputFrame playFrame = inputFrame;
  playFrame.context = iggy3d::creative::CreativeInputContext::RuntimePlay;
  const auto actionDown = [&](iggy3d::creative::CreativeInputActionId action) {
    return iggy3d::creative::creativeInputActionDown(
        playFrame, action, bindings, enabled ? &routedInput : nullptr);
  };
  return {actionDown(iggy3d::creative::CreativeInputActionId::RuntimeAttack),
          actionDown(
              iggy3d::creative::CreativeInputActionId::RuntimeInteract),
          enabled};
}

CreativePlayAction routeCreativePlayAction(
    CreativePlayActionRouterState& state,
    CreativePlayActionSample sample) noexcept {
  const bool attackPressed = sample.attackDown && !state.attackDown;
  const bool interactPressed = sample.interactDown && !state.interactDown;
  state.attackDown = sample.attackDown;
  state.interactDown = sample.interactDown;
  if (state.rearmRequired) {
    state.rearmRequired = sample.attackDown || sample.interactDown;
    return CreativePlayAction::None;
  }
  if (!sample.enabled) {
    return CreativePlayAction::None;
  }
  if (attackPressed) {
    return CreativePlayAction::Attack;
  }
  if (interactPressed) {
    return CreativePlayAction::Interact;
  }
  return CreativePlayAction::None;
}

std::string_view toString(CreativePlayTargetStatus status) noexcept {
  switch (status) {
    case CreativePlayTargetStatus::None:
      return "none";
    case CreativePlayTargetStatus::Valid:
      return "valid";
    case CreativePlayTargetStatus::Blocked:
      return "blocked";
    case CreativePlayTargetStatus::Friendly:
      return "friendly";
    case CreativePlayTargetStatus::OutOfRange:
      return "out_of_range";
    case CreativePlayTargetStatus::Unsupported:
      return "unsupported";
    case CreativePlayTargetStatus::Defeated:
      return "defeated";
    case CreativePlayTargetStatus::Invalid:
      return "invalid";
  }
  return "invalid";
}

CreativePlayTarget resolveCreativePlayTarget(
    const CreativePlayTargetRequest& request) {
  CreativePlayTarget result;
  const iggy3d::Vec3 forward = normalizedOrZero(request.forward);
  if (request.world == nullptr || !iggy3d::isValid(request.actor) ||
      !iggy3d::isFinite(request.eyeMeters) ||
      iggy3d::lengthSquared(forward) <= kDirectionEpsilon ||
      !finitePositive(request.probeDistanceMeters) ||
      !finiteNonnegative(request.targetRadiusMeters) ||
      !finitePositive(request.interactionRangeMeters) ||
      !finiteNonnegative(request.occlusionMarginMeters) ||
      request.attackDamage <= 0) {
    result.status = CreativePlayTargetStatus::Invalid;
    return result;
  }

  iggy3d::EntityHitQueryRequest hitRequest;
  hitRequest.world = request.world;
  hitRequest.startMeters = request.eyeMeters;
  hitRequest.endMeters =
      request.eyeMeters + forward * request.probeDistanceMeters;
  hitRequest.ignoredEntity = request.actor;
  hitRequest.radiusMeters = request.targetRadiusMeters;
  hitRequest.requireAttackTarget = false;
  const iggy3d::EntityHitQueryResult hit =
      iggy3d::queryFirstEntityHit(hitRequest);
  if (hit.status == iggy3d::EntityHitStatus::NoHit) {
    return result;
  }
  if (hit.status != iggy3d::EntityHitStatus::Hit) {
    result.status = CreativePlayTargetStatus::Invalid;
    return result;
  }

  result.entity = hit.entity;
  result.stableName = hit.stableName;
  result.hitPointMeters = hit.pointMeters;
  result.hitDistanceMeters = hit.distanceMeters;
  const iggy3d::EntityState* entity = request.world->findById(hit.entity);
  if (entity == nullptr) {
    result.status = CreativePlayTargetStatus::Invalid;
    return result;
  }
  result.supportsAttack =
      iggy3d::targetSupportsCommandKind(*entity, iggy3d::CommandKind::Attack);
  result.supportsInteract = iggy3d::targetSupportsCommandKind(
      *entity, iggy3d::CommandKind::Interact);

  const iggy3d::SegmentOcclusionVerdict occlusion = iggy3d::segmentOcclusion(
      request.colliders, request.eyeMeters, hit.pointMeters,
      request.occlusionMarginMeters);
  if (occlusion == iggy3d::SegmentOcclusionVerdict::Unknown) {
    result.status = CreativePlayTargetStatus::Invalid;
    return result;
  }
  if (occlusion == iggy3d::SegmentOcclusionVerdict::Blocked) {
    result.status = CreativePlayTargetStatus::Blocked;
    return result;
  }

  const iggy3d::ReachQueryResult reach = iggy3d::queryReach(
      {request.world, request.actor, hit.entity, false, {},
       request.interactionRangeMeters, true});
  result.reachDistanceMeters = reach.distanceMeters;
  if (reach.status == iggy3d::ReachQueryStatus::OutOfRange) {
    result.status = CreativePlayTargetStatus::OutOfRange;
    return result;
  }
  if (reach.status != iggy3d::ReachQueryStatus::Reachable) {
    result.status = CreativePlayTargetStatus::Invalid;
    return result;
  }

  if (result.supportsAttack) {
    if (request.combat == nullptr) {
      result.status = CreativePlayTargetStatus::Invalid;
      return result;
    }
    const iggy3d::CombatAttackResult attack = iggy3d::previewAttack(
        *request.combat,
        {request.actor, hit.entity, request.attackDamage});
    if (attack.status == iggy3d::CombatStatus::TargetDefeated) {
      result.defeated = true;
      result.status = CreativePlayTargetStatus::Defeated;
      return result;
    }
    if (attack.status == iggy3d::CombatStatus::FriendlyFireBlocked) {
      result.friendly = true;
      result.status = CreativePlayTargetStatus::Friendly;
      return result;
    }
    if (attack.status != iggy3d::CombatStatus::Succeeded) {
      result.status = CreativePlayTargetStatus::Invalid;
      return result;
    }
  }

  result.status = result.supportsAttack || result.supportsInteract
                      ? CreativePlayTargetStatus::Valid
                      : CreativePlayTargetStatus::Unsupported;
  return result;
}

bool creativeEditorPlayTargetAcceptsAction(
    const CreativePlayTarget& target,
    CreativePlayAction action) noexcept {
  switch (action) {
    case CreativePlayAction::Attack:
      return target.status == CreativePlayTargetStatus::Valid &&
             target.supportsAttack;
    case CreativePlayAction::Interact:
      return (target.status == CreativePlayTargetStatus::Valid ||
              target.status == CreativePlayTargetStatus::Friendly) &&
             target.supportsInteract;
    case CreativePlayAction::None:
      return false;
  }
  return false;
}

CreativePlayView buildCreativePlayView(
    const CreativePlaySession& mode) noexcept {
  CreativePlayView result;
  if (!mode.sandbox.has_value()) {
    return result;
  }
  const iggy3d::EntityState* player =
      mode.sandbox->session.state().world.findById(kLocalPlayerEntity);
  if (player == nullptr || !iggy3d::isFinite(player->transform.position)) {
    return result;
  }
  const float yaw = mode.cameraYawDegrees * kPi / 180.0F;
  const float pitch = mode.cameraPitchDegrees * kPi / 180.0F;
  const float cosPitch = std::cos(pitch);
  result.eyeMeters = player->transform.position +
                     iggy3d::Vec3{0.0F, kPlayerEyeHeightMeters, 0.0F};
  result.forward = {std::sin(yaw) * cosPitch, std::sin(pitch),
                    -std::cos(yaw) * cosPitch};
  result.available = iggy3d::isFinite(result.eyeMeters) &&
                     iggy3d::isFinite(result.forward);
  return result;
}

CreativePlayHudFrame buildCreativePlayHud(
    const CreativePlaySession& mode,
    const iggy3d::FrameInput& frame) {
  CreativePlayHudFrame hud;
  if (!mode.sandbox.has_value() || frame.viewport.width == 0U ||
      frame.viewport.height == 0U) {
    return hud;
  }
  const iggy3d::RenderContentViewport content =
      iggy3d::effectiveContentViewport(frame);
  if (content.width < 160U || content.height < 100U) {
    return hud;
  }

  const auto [health, maximumHealth] = playerHealth(mode);
  const std::int32_t panelX = content.x + 12;
  const std::int32_t panelY = content.y + 12;
  const std::uint32_t panelWidth = std::min<std::uint32_t>(
      420U, content.width > 24U ? content.width - 24U : content.width);
  appendRect(hud, {panelX, panelY, panelWidth, 96U,
                   0.04F, 0.055F, 0.065F, 0.88F});

  FixedHudText healthText;
  std::snprintf(healthText.chars.data(), healthText.chars.size(),
                "PLAY HP %d/%d", health, maximumHealth);
  appendText(hud, healthText.view(), panelX + 10, panelY + 9,
             frame.viewport.width, frame.viewport.height,
             {0.86F, 0.92F, 0.95F, 1.0F});
  constexpr std::size_t kGlyphAdvancePixels = 12U;
  const std::size_t targetCharacterCapacity =
      (panelWidth - 20U) / kGlyphAdvancePixels;
  const FixedHudText targetText =
      targetHudText(mode.target, targetCharacterCapacity);
  appendText(hud, targetText.view(), panelX + 10, panelY + 29,
             frame.viewport.width, frame.viewport.height,
             targetColor(mode.target.status));

  const FixedHudText effectText =
      interactionEffectHudText(mode.lastInteractionEffect);
  if (!effectText.view().empty()) {
    using EffectStatus =
        iggy3d::creative::CreativeRuntimeInteractionEffectStatus;
    const auto effectStatus = mode.lastInteractionEffect.status;
    const bool warning = effectStatus == EffectStatus::NoLinkedTarget ||
                         effectStatus == EffectStatus::TargetOccupied ||
                         effectStatus == EffectStatus::DoorLocked;
    appendText(hud, effectText.view(), panelX + 10, panelY + 49,
               frame.viewport.width, frame.viewport.height,
               warning ? HudColor{0.98F, 0.84F, 0.24F, 1.0F}
                       : HudColor{0.30F, 0.95F, 0.48F, 1.0F});
  }

  const std::int32_t healthBarX = panelX + 10;
  const std::int32_t healthBarY = panelY + 75;
  const std::uint32_t healthBarWidth = panelWidth > 20U ? panelWidth - 20U : 0U;
  appendRect(hud, {healthBarX, healthBarY, healthBarWidth, 8U,
                   0.18F, 0.20F, 0.22F, 1.0F});
  const float healthRatio = maximumHealth > 0
                                ? std::clamp(
                                      static_cast<float>(health) /
                                          static_cast<float>(maximumHealth),
                                      0.0F, 1.0F)
                                : 0.0F;
  appendRect(hud, {healthBarX, healthBarY,
                   static_cast<std::uint32_t>(
                       static_cast<float>(healthBarWidth) * healthRatio),
                   8U, 0.24F, 0.84F, 0.40F, 1.0F});

  const std::int32_t centerX =
      content.x + static_cast<std::int32_t>(content.width / 2U);
  const std::int32_t centerY =
      content.y + static_cast<std::int32_t>(content.height / 2U);
  const HudColor color = targetColor(mode.target.status);
  constexpr std::int32_t kGap = 4;
  constexpr std::uint32_t kArmLength = 8U;
  constexpr std::uint32_t kThickness = 2U;
  appendRect(hud, {centerX - kGap - static_cast<std::int32_t>(kArmLength),
                   centerY - 1, kArmLength, kThickness,
                   color.r, color.g, color.b, color.a});
  appendRect(hud, {centerX + kGap, centerY - 1,
                   kArmLength, kThickness,
                   color.r, color.g, color.b, color.a});
  appendRect(hud, {centerX - 1,
                   centerY - kGap - static_cast<std::int32_t>(kArmLength),
                   kThickness, kArmLength,
                   color.r, color.g, color.b, color.a});
  appendRect(hud, {centerX - 1, centerY + kGap,
                   kThickness, kArmLength,
                   color.r, color.g, color.b, color.a});
  appendRect(hud, {centerX - 1, centerY - 1, 2U, 2U,
                   color.r, color.g, color.b, color.a});
  return hud;
}

void attachCreativePlayHud(const CreativePlayHudFrame& hud,
                                 iggy3d::FrameInput& frame) noexcept {
  frame.ui.visible = hud.rectCount > 0U || hud.glyphQuadCount > 0U;
  frame.ui.rects = hud.rects.data();
  frame.ui.rectCount = hud.rectCount;
  frame.ui.textGlyphQuads = hud.glyphQuads.data();
  frame.ui.textGlyphQuadCount = hud.glyphQuadCount;
  frame.ui.textGlyphCount = hud.textGlyphCount;
  frame.ui.primitiveCount = hud.rectCount + hud.glyphQuadCount;
}

}  // namespace iggy3d_creative_app
