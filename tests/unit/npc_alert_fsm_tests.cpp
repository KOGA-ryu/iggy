#include "runtime/ai/NpcAlertSystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

#include "runtime/ai/AiState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float tol) {
  return std::fabs(lhs - rhs) <= tol;
}

bool profileValidationRejectsBadValues() {
  iggy3d::AlertProfile profile;
  bool ok = expect(iggy3d::isValidAlertProfile(profile), "default alert profile valid");

  profile = {};
  profile.suspiciousNorm = profile.observantNorm;  // not strictly ascending
  ok = ok && expect(!iggy3d::isValidAlertProfile(profile),
                    "non-ascending band boundaries rejected");
  profile = {};
  profile.riseRatePerTick = 0.0F;
  ok = ok && expect(!iggy3d::isValidAlertProfile(profile), "zero rise rate rejected");
  profile = {};
  profile.drainTicks[2] = 0U;
  ok = ok && expect(!iggy3d::isValidAlertProfile(profile), "zero drain ticks rejected");
  profile = {};
  profile.graceFrac = 0.5F;
  ok = ok && expect(!iggy3d::isValidAlertProfile(profile), "grace frac below 1 rejected");
  return ok;
}

bool alertLadderMapsLevelsToBands() {
  const iggy3d::AlertProfile p;
  return expect(iggy3d::alertBehaviorForLevel(0.0F, p) == iggy3d::AiBehaviorKind::Idle,
                "zero level is idle") &&
         expect(iggy3d::alertBehaviorForLevel(p.observantNorm - 0.001F, p) ==
                    iggy3d::AiBehaviorKind::Idle,
                "just below observant stays idle") &&
         expect(iggy3d::alertBehaviorForLevel(p.observantNorm, p) ==
                    iggy3d::AiBehaviorKind::Observant,
                "observant boundary is observant") &&
         expect(iggy3d::alertBehaviorForLevel(p.suspiciousNorm, p) ==
                    iggy3d::AiBehaviorKind::Suspicious,
                "suspicious boundary is suspicious") &&
         expect(iggy3d::alertBehaviorForLevel(p.searchingNorm, p) ==
                    iggy3d::AiBehaviorKind::Searching,
                "searching boundary is searching") &&
         expect(iggy3d::alertBehaviorForLevel(p.agitatedNorm, p) ==
                    iggy3d::AiBehaviorKind::Alert,
                "agitated boundary is alert") &&
         expect(iggy3d::alertBehaviorForLevel(p.combatNorm, p) ==
                    iggy3d::AiBehaviorKind::Chasing,
                "combat boundary is chasing") &&
         expect(iggy3d::alertBandIndex(p.combatNorm, p) == 5U, "combat band index 5");
}

bool noTargetRuleCapsBelowCombat() {
  const iggy3d::AlertProfile p;
  // A hard rise with NO valid target must top out just below combat (agitated).
  iggy3d::AiActorState noTarget;
  iggy3d::npcRaiseAlert(noTarget, p, 5.0F, 10, /*hasValidTarget=*/false,
                        /*visualConfirmed=*/true);
  bool ok = expect(noTarget.alertLevel < p.combatNorm, "no-target level capped below combat") &&
            expect(iggy3d::alertBehaviorForLevel(noTarget.alertLevel, p) ==
                       iggy3d::AiBehaviorKind::Alert,
                   "no-target tops out at agitated, never chasing");

  // The identical rise WITH a valid target reaches combat.
  iggy3d::AiActorState withTarget;
  iggy3d::npcRaiseAlert(withTarget, p, 5.0F, 10, /*hasValidTarget=*/true,
                        /*visualConfirmed=*/true);
  return ok &&
         expect(withTarget.alertLevel >= p.combatNorm, "with target reaches combat") &&
         expect(iggy3d::alertBehaviorForLevel(withTarget.alertLevel, p) ==
                    iggy3d::AiBehaviorKind::Chasing,
                "with target enters chasing");
}

bool decayIsLinearPerBandAndDeadTimeGated() {
  const iggy3d::AlertProfile p;
  // Within a single band, decay is linear at width/drainTicks per tick.
  iggy3d::AiActorState a;
  a.alertLevel = p.searchingNorm - 0.001F;  // inside the suspicious band
  a.lastRiseTick = 0;                        // dead-time already elapsed
  const float ratePerTick = (p.searchingNorm - p.suspiciousNorm) /
                            static_cast<float>(p.drainTicks[1]);
  const float before = a.alertLevel;
  for (int i = 0; i < 10; ++i) {
    iggy3d::npcDecayAlert(a, p, p.deadTimeTicks + static_cast<std::uint64_t>(i));
  }
  bool ok = expect(near(before - a.alertLevel, 10.0F * ratePerTick, 0.0005F),
                   "suspicious band decays linearly at width/drainTicks");

  // Dead-time holds the level flat for deadTimeTicks after a rise, then decays.
  iggy3d::AiActorState b;
  b.alertLevel = 0.5F;
  b.lastRiseTick = 100;
  const float held = b.alertLevel;
  iggy3d::npcDecayAlert(b, p, 100 + p.deadTimeTicks - 1);
  ok = ok && expect(near(b.alertLevel, held, 0.0001F), "dead-time holds level flat");
  iggy3d::npcDecayAlert(b, p, 100 + p.deadTimeTicks);
  ok = ok && expect(b.alertLevel < held, "level decays once dead-time elapses");
  return ok;
}

bool graceWindowSwallowsSmallRisesButVisualBypasses() {
  const iggy3d::AlertProfile p;
  iggy3d::AiActorState a;
  // A band-crossing rise into observant opens a grace window.
  iggy3d::npcRaiseAlert(a, p, p.observantNorm + 0.001F, 100, /*target=*/true,
                        /*visual=*/false);
  const float opened = a.alertLevel;
  bool ok = expect(a.graceUntilTick > 100, "band cross opens a grace window");

  // Small non-visual rises inside the window are swallowed up to the limit.
  for (std::uint32_t i = 0; i < p.graceCountLimit; ++i) {
    iggy3d::npcRaiseAlert(a, p, 0.001F, 101 + i, true, /*visual=*/false);
  }
  ok = ok && expect(near(a.alertLevel, opened, 0.00001F),
                    "small non-visual rises swallowed within grace") &&
       expect(a.graceCount == p.graceCountLimit, "grace count reached its limit");

  // The next non-visual rise breaks through once the count limit is spent.
  iggy3d::npcRaiseAlert(a, p, 0.001F, 120, true, /*visual=*/false);
  ok = ok && expect(a.alertLevel > opened, "rise breaks through after the count limit");

  // A LOS-confirmed sighting always bypasses grace.
  iggy3d::AiActorState b;
  iggy3d::npcRaiseAlert(b, p, p.observantNorm + 0.001F, 100, true, /*visual=*/false);
  const float bOpened = b.alertLevel;
  iggy3d::npcRaiseAlert(b, p, 0.001F, 101, true, /*visual=*/true);
  return ok && expect(b.alertLevel > bOpened, "visual-confirmed rise bypasses grace");
}

bool alertLadderWalksUpThenDownDeterministically() {
  const iggy3d::AlertProfile p;
  iggy3d::AiActorState a;
  const bool startsIdle = a.behavior == iggy3d::AiBehaviorKind::Idle;

  iggy3d::NpcAlertStimulus seen;
  seen.targetPerceived = true;
  seen.proximity01 = 1.0F;
  seen.hasValidTarget = true;
  seen.visualConfirmed = true;

  bool sawObservant = false, sawSuspicious = false, sawSearching = false, sawAlert = false;
  std::uint64_t tick = 0;
  for (; tick < 400; ++tick) {
    iggy3d::npcStepAlert(a, seen, p, tick);
    if (a.behavior == iggy3d::AiBehaviorKind::Observant) sawObservant = true;
    if (a.behavior == iggy3d::AiBehaviorKind::Suspicious) sawSuspicious = true;
    if (a.behavior == iggy3d::AiBehaviorKind::Searching) sawSearching = true;
    if (a.behavior == iggy3d::AiBehaviorKind::Alert) sawAlert = true;
    if (a.behavior == iggy3d::AiBehaviorKind::Chasing) break;
  }
  bool ok = expect(startsIdle, "actor starts idle") &&
            expect(sawObservant && sawSuspicious && sawSearching && sawAlert,
                   "climbs through every intermediate band") &&
            expect(a.behavior == iggy3d::AiBehaviorKind::Chasing,
                   "reaches combat with a valid target in sight");

  // Lose the target: the behavior decays back to idle, then the level fully
  // drains to zero (idle is the band level < observant; zero is the true floor).
  iggy3d::NpcAlertStimulus lost;  // targetPerceived defaults false
  bool calmedToIdle = false;
  for (std::uint64_t t = tick + 1; t < tick + 8000; ++t) {
    iggy3d::npcStepAlert(a, lost, p, t);
    if (!calmedToIdle && a.behavior == iggy3d::AiBehaviorKind::Idle) {
      calmedToIdle = true;
    }
    if (a.alertLevel == 0.0F) {
      break;
    }
  }
  return ok && expect(calmedToIdle, "behavior calms back to idle") &&
         expect(a.alertLevel == 0.0F, "alert level fully drains to zero") &&
         expect(a.maxAlertIndexThisEngagement == 0U, "engagement high-water cleared when fully calm");
}

bool alertFsmIsDeterministic() {
  const iggy3d::AlertProfile p;
  const auto runTrace = [&](std::vector<float>& out) {
    iggy3d::AiActorState a;
    iggy3d::NpcAlertStimulus stim;
    stim.targetPerceived = true;
    stim.proximity01 = 0.7F;
    stim.hasValidTarget = true;
    stim.visualConfirmed = true;
    for (std::uint64_t t = 0; t < 60; ++t) {
      iggy3d::npcStepAlert(a, stim, p, t);
      out.push_back(a.alertLevel);
    }
  };
  std::vector<float> first;
  std::vector<float> second;
  runTrace(first);
  runTrace(second);
  return expect(first == second, "identical inputs yield a byte-identical alert trace");
}

bool ineffectiveHeardSoundFallsThroughToDecay() {
  const iggy3d::AlertProfile p;
  iggy3d::AiActorState a;
  a.alertLevel = p.searchingNorm;
  a.lastRiseTick = 0;
  iggy3d::NpcAlertStimulus stim;
  stim.heard = true;
  stim.alertUnits = 0.0F;

  const float before = a.alertLevel;
  iggy3d::npcStepAlert(a, stim, p, p.deadTimeTicks);

  return expect(a.alertLevel < before,
                "zero heard alert units fall through to decay") &&
         expect(a.lastRiseTick == 0U,
                "zero heard alert units do not re-anchor dead-time");
}

bool graceSwallowedHeardSoundFallsThroughToDecay() {
  const iggy3d::AlertProfile p;
  iggy3d::AiActorState a;
  a.alertLevel = p.observantNorm + 0.001F;
  a.lastRiseTick = 0;
  a.graceUntilTick = 100U;
  a.graceThreshold = a.alertLevel;
  iggy3d::NpcAlertStimulus stim;
  stim.heard = true;
  stim.alertUnits = p.soundAlertUnitsRef * 0.01F;

  const float before = a.alertLevel;
  iggy3d::npcStepAlert(a, stim, p, p.deadTimeTicks);

  return expect(a.graceCount == 1U, "heard sound was grace-swallowed") &&
         expect(a.alertLevel < before,
                "grace-swallowed heard sound falls through to decay") &&
         expect(a.lastRiseTick == 0U,
                "grace-swallowed heard sound does not re-anchor dead-time");
}

bool cappedHeardSoundDoesNotReanchorDecayForever() {
  const iggy3d::AlertProfile p;
  const float noTargetCap = std::nextafter(p.combatNorm, 0.0F);
  iggy3d::AiActorState a;
  a.alertLevel = noTargetCap;
  a.lastRiseTick = 10U;
  iggy3d::NpcAlertStimulus stim;
  stim.heard = true;
  stim.alertUnits = p.soundAlertUnitsRef;

  iggy3d::npcStepAlert(a, stim, p, 10U + p.deadTimeTicks);

  return expect(a.lastRiseTick == 10U,
                "capped heard sound does not move lastRiseTick") &&
         expect(a.alertLevel < noTargetCap,
                "capped heard sound falls through to decay") &&
         expect(a.alertLevel < p.combatNorm,
                "capped heard sound remains below combat");
}

bool visualAtCapKeepsDeadTimeFreshWithoutDecay() {
  const iggy3d::AlertProfile p;
  iggy3d::AiActorState a;
  a.alertLevel = p.combatNorm;
  a.lastRiseTick = 10U;
  iggy3d::NpcAlertStimulus stim;
  stim.targetPerceived = true;
  stim.proximity01 = 1.0F;
  stim.hasValidTarget = true;
  stim.visualConfirmed = true;

  iggy3d::npcStepAlert(a, stim, p, 100U);

  return expect(near(a.alertLevel, p.combatNorm, 0.00001F),
                "visual at cap does not decay") &&
         expect(a.lastRiseTick == 100U,
                "visual at cap keeps dead-time fresh") &&
         expect(a.behavior == iggy3d::AiBehaviorKind::Chasing,
                "visual at cap remains chasing");
}

// a1s2 (L1): sustained heard-but-unseen noise drives alert up but the no-target
// combat cap (hasValidTarget=false, wired by the heard branch) holds it at band 4 --
// pure noise NEVER reaches band 5 / Chasing. Feeds npcStepAlert directly (no session)
// with a maxed sound stimulus and targetPerceived=false, spamming many ticks.
bool soundOnlyNoiseCapsBelowChasing() {
  const iggy3d::AlertProfile p;
  iggy3d::AiActorState a;
  iggy3d::NpcAlertStimulus stim;
  stim.targetPerceived = false;  // never visually seen
  stim.hasValidTarget = false;   // no resolved target -- pure noise
  stim.visualConfirmed = false;
  stim.heard = true;
  stim.alertUnits = p.soundAlertUnitsRef;  // full-strength (clamp01(units/ref) == 1)
  stim.soundInvestigatePos = iggy3d::Vec3{3.0F, 0.0F, 3.0F};

  std::uint8_t maxBand = 0U;
  float maxLevel = 0.0F;
  for (std::uint64_t t = 0; t < 600; ++t) {
    iggy3d::npcStepAlert(a, stim, p, t);
    maxBand = std::max(maxBand, iggy3d::alertBandIndex(a.alertLevel, p));
    maxLevel = std::max(maxLevel, a.alertLevel);
  }
  const float noTargetCap = std::nextafter(p.combatNorm, 0.0F);
  return expect(a.alertLevel > p.searchingNorm,
                "sustained noise climbs into the search/alert bands") &&
         expect(a.alertLevel < p.combatNorm, "noise-only level capped below combat") &&
         expect(maxLevel <= noTargetCap, "noise-only never exceeds no-target cap") &&
         expect(near(maxLevel, noTargetCap, 0.0005F),
                "sustained noise reaches the no-target cap") &&
         expect(iggy3d::alertBandIndex(a.alertLevel, p) == 4U,
                "noise-only tops out at band 4 (agitated)") &&
         expect(maxBand < 5U, "noise-only never reaches band 5 / chasing");
}

}  // namespace

int main() {
  const bool ok = profileValidationRejectsBadValues() &&
                  alertLadderMapsLevelsToBands() &&
                  noTargetRuleCapsBelowCombat() &&
                  decayIsLinearPerBandAndDeadTimeGated() &&
                  graceWindowSwallowsSmallRisesButVisualBypasses() &&
                  alertLadderWalksUpThenDownDeterministically() &&
                  alertFsmIsDeterministic() &&
                  ineffectiveHeardSoundFallsThroughToDecay() &&
                  graceSwallowedHeardSoundFallsThroughToDecay() &&
                  cappedHeardSoundDoesNotReanchorDecayForever() &&
                  visualAtCapKeepsDeadTimeFreshWithoutDecay() &&
                  soundOnlyNoiseCapsBelowChasing();
  return ok ? 0 : 1;
}
