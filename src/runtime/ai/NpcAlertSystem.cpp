#include "runtime/ai/NpcAlertSystem.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {
namespace {

float clamp01(float value) {
  if (!(value > 0.0F)) {
    return 0.0F;
  }
  if (value > 1.0F) {
    return 1.0F;
  }
  return value;
}

// Lower/upper boundary and per-band decay ticks for the band containing `level`.
struct BandSpan {
  float lower = 0.0F;
  float upper = 0.0F;
  std::uint32_t drainTicks = 0;
};

BandSpan bandSpanFor(float level, const AlertProfile& profile) {
  if (level >= profile.agitatedNorm) {
    return {profile.agitatedNorm, profile.combatNorm, profile.drainTicks[3]};
  }
  if (level >= profile.searchingNorm) {
    return {profile.searchingNorm, profile.agitatedNorm, profile.drainTicks[2]};
  }
  if (level >= profile.suspiciousNorm) {
    return {profile.suspiciousNorm, profile.searchingNorm, profile.drainTicks[1]};
  }
  if (level >= profile.observantNorm) {
    return {profile.observantNorm, profile.suspiciousNorm, profile.drainTicks[0]};
  }
  return {0.0F, profile.observantNorm, profile.drainTicks[0]};
}

bool raiseAlertAndReportAcceptedRise(AiActorState& actor,
                                     const AlertProfile& profile,
                                     float increment,
                                     std::uint64_t tick,
                                     bool hasValidTarget,
                                     bool visualConfirmed) {
  if (!(increment > 0.0F)) {
    return false;
  }
  // Grace-window anti-spam: swallow small non-visual rises that don't push
  // meaningfully past the level that opened the window, up to the count limit.
  if (!visualConfirmed && tick < actor.graceUntilTick &&
      actor.graceCount < profile.graceCountLimit &&
      actor.alertLevel + increment < actor.graceThreshold * profile.graceFrac) {
    ++actor.graceCount;
    return false;
  }

  const float levelBefore = actor.alertLevel;
  const std::uint8_t bandBefore = alertBandIndex(actor.alertLevel, profile);
  actor.alertLevel += increment;
  // No-target combat cap: can't cross into combat without a live target.
  if (!hasValidTarget) {
    const float cap = std::nextafter(profile.combatNorm, 0.0F);
    if (actor.alertLevel > cap) {
      actor.alertLevel = cap;
    }
  }
  if (actor.alertLevel > profile.combatNorm) {
    actor.alertLevel = profile.combatNorm;
  }

  const bool acceptedRise = actor.alertLevel > levelBefore;
  if (!acceptedRise) {
    if (visualConfirmed) {
      actor.lastRiseTick = tick;
    }
    return false;
  }

  actor.lastRiseTick = tick;  // dead-time resets on every accepted rise
  const std::uint8_t bandAfter = alertBandIndex(actor.alertLevel, profile);
  if (bandAfter > bandBefore) {
    // Crossed up a band: open a fresh grace window anchored at the new level.
    actor.graceUntilTick = tick + profile.graceWindowTicks;
    actor.graceThreshold = actor.alertLevel;
    actor.graceCount = 0U;
  }
  if (bandAfter > actor.maxAlertIndexThisEngagement) {
    actor.maxAlertIndexThisEngagement = bandAfter;
  }
  return true;
}

}  // namespace

bool isValidAlertProfile(const AlertProfile& profile) {
  const bool ascending = std::isfinite(profile.observantNorm) &&
                         profile.observantNorm > 0.0F &&
                         profile.suspiciousNorm > profile.observantNorm &&
                         profile.searchingNorm > profile.suspiciousNorm &&
                         profile.agitatedNorm > profile.searchingNorm &&
                         profile.combatNorm > profile.agitatedNorm;
  const bool rates = std::isfinite(profile.riseRatePerTick) &&
                     profile.riseRatePerTick > 0.0F &&
                     profile.drainTicks[0] > 0U && profile.drainTicks[1] > 0U &&
                     profile.drainTicks[2] > 0U && profile.drainTicks[3] > 0U;
  const bool grace = std::isfinite(profile.graceFrac) && profile.graceFrac >= 1.0F;
  return ascending && rates && grace;
}

std::uint8_t alertBandIndex(float level, const AlertProfile& profile) {
  if (level >= profile.combatNorm) {
    return 5U;
  }
  if (level >= profile.agitatedNorm) {
    return 4U;
  }
  if (level >= profile.searchingNorm) {
    return 3U;
  }
  if (level >= profile.suspiciousNorm) {
    return 2U;
  }
  if (level >= profile.observantNorm) {
    return 1U;
  }
  return 0U;
}

AiBehaviorKind alertBehaviorForLevel(float level, const AlertProfile& profile) {
  switch (alertBandIndex(level, profile)) {
    case 5U:
      return AiBehaviorKind::Chasing;  // combat; decision layer splits by range
    case 4U:
      return AiBehaviorKind::Alert;    // agitated, weapon-ready
    case 3U:
      return AiBehaviorKind::Searching;
    case 2U:
      return AiBehaviorKind::Suspicious;
    case 1U:
      return AiBehaviorKind::Observant;
    default:
      return AiBehaviorKind::Idle;
  }
}

void npcDecayAlert(AiActorState& actor, const AlertProfile& profile,
                   std::uint64_t tick) {
  if (actor.alertLevel <= 0.0F) {
    actor.alertLevel = 0.0F;
    actor.maxAlertIndexThisEngagement = 0U;
    return;
  }
  // Dead-time: hold flat for a window after the most recent rise.
  if (tick < actor.lastRiseTick + profile.deadTimeTicks) {
    return;
  }
  const BandSpan span = bandSpanFor(actor.alertLevel, profile);
  const float width = span.upper - span.lower;
  const float ratePerTick =
      span.drainTicks > 0U ? width / static_cast<float>(span.drainTicks) : width;
  actor.alertLevel -= ratePerTick;
  if (actor.alertLevel <= 0.0F) {
    actor.alertLevel = 0.0F;
    actor.maxAlertIndexThisEngagement = 0U;
  }
}

void npcRaiseAlert(AiActorState& actor, const AlertProfile& profile,
                   float increment, std::uint64_t tick, bool hasValidTarget,
                   bool visualConfirmed) {
  (void)raiseAlertAndReportAcceptedRise(actor, profile, increment, tick,
                                        hasValidTarget, visualConfirmed);
}

void npcStepAlert(AiActorState& actor, const NpcAlertStimulus& stimulus,
                  const AlertProfile& profile, std::uint64_t tick) {
  if (stimulus.targetPerceived) {
    const float increment = profile.riseRatePerTick * clamp01(stimulus.proximity01);
    npcRaiseAlert(actor, profile, increment, tick, stimulus.hasValidTarget,
                  stimulus.visualConfirmed);
  } else if (stimulus.heard) {
    // Heard-and-unseen: a NON-VISUAL rise (grace applies). hasValidTarget=false is
    // deliberate -- an unnamed threat -- so the no-target combat cap holds the guard
    // just below combatNorm: band 4 max, Searching reachable, NEVER band 5 / Chasing
    // on noise alone (the guard has no true target to run down through walls).
    const float ref = profile.soundAlertUnitsRef > 0.0F ? profile.soundAlertUnitsRef : 1.0F;
    const float increment = profile.soundRiseScale * clamp01(stimulus.alertUnits / ref);
    if (!raiseAlertAndReportAcceptedRise(actor, profile, increment, tick,
                                         /*hasValidTarget=*/false,
                                         /*visualConfirmed=*/false)) {
      npcDecayAlert(actor, profile, tick);
    }
  } else {
    npcDecayAlert(actor, profile, tick);
  }
  actor.behavior = alertBehaviorForLevel(actor.alertLevel, profile);
}

}  // namespace iggy3d
