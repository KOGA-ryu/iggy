// s8 — stealth tuning readout. MEASUREMENT ONLY: this test changes no constants and no
// behavior. It drives the EXACT shipped alert/investigate functions (npcStepAlert /
// npcStepInvestigate) on scratch state, seeded from the garden guard's RESOLVED melee_training
// AlertProfile and the REAL tick rate parsed from the garden scenario, and prints a legible
// ticks+seconds dashboard (escalation / decay / dwell) plus the tunable knobs. The sanity
// asserts are invariants of the current profile (not magic numbers), so the test stays green
// and also catches an accidental future profile change.
//
// Retrieve headless:  ctest --test-dir build -R stealth_tuning_readout -V

#include "content/FixtureScenarioLoader.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/NpcInvestigateSystem.hpp"
#include "runtime/ai/NpcPatrolSystem.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

constexpr const char* kScenarioPath =
    "fixtures/rooms/ascii/stealth_garden.scenario.iggy3d.toml";
constexpr float kProximity01 = 0.5F;  // player squarely mid-cone (~half the perception radius)

const std::array<const char*, 6> kBandName = {"Idle",      "Observant", "Suspicious",
                                              "Searching", "Alert",     "Chasing"};

std::string readFile(const char* path) {
  std::ifstream file(path);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

double secs(std::uint64_t ticks, std::uint32_t rateHz) {
  return static_cast<double>(ticks) / static_cast<double>(rateHz);
}

void printBandRow(std::string_view band, std::uint64_t ticks, std::uint32_t rateHz) {
  std::cout << "  " << std::left << std::setw(11) << band << std::right << std::setw(7) << ticks
            << std::setw(11) << std::fixed << std::setprecision(2) << secs(ticks, rateHz) << '\n';
}

}  // namespace

int main() {
  // --- Resolve the REAL profile + tick rate (do not hardcode) ------------------------------
  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  const iggy3d::NpcBehaviorProfileResolveResult resolved =
      iggy3d::resolveNpcBehaviorProfile({&catalog, "melee_training"});
  bool ok = expect(resolved.ok, "melee_training profile resolves");
  const iggy3d::AlertProfile profile = resolved.profile.alertProfile;

  const iggy3d::ScenarioLoadResult scenario =
      iggy3d::parseScenarioText(readFile(kScenarioPath));
  ok = ok && expect(scenario.status == iggy3d::ScenarioLoadStatus::Ok, "garden scenario parses");
  const std::uint32_t rateHz = scenario.seed.config.fixedTickRateHz;
  ok = ok && expect(rateHz > 0U, "tick rate is positive") &&
       expect(iggy3d::isValidAlertProfile(profile), "shipped alert profile is valid");
  if (!ok) {
    return 1;
  }

  const float risePerTick = profile.riseRatePerTick * kProximity01;

  std::cout << "\n================ STEALTH TUNING READOUT (measurement only) ================\n";
  std::cout << "profile        : melee_training (garden guard)\n";
  std::cout << "tick rate      : " << rateHz << " Hz  (dt = " << std::fixed << std::setprecision(4)
            << secs(1, rateHz) << " s/tick)\n";
  std::cout << "fixed proximity: proximity01 = " << std::setprecision(2) << kProximity01
            << "  -> rise/tick = " << std::setprecision(5) << risePerTick << " alert/tick\n";

  // --- 1) Escalation: player held in cone from first sight, clean monotonic rise ----------
  std::array<std::uint64_t, 6> escTick = {0, 0, 0, 0, 0, 0};
  std::array<bool, 6> escReached = {true, false, false, false, false, false};  // Idle at t=0
  {
    iggy3d::AiActorState actor;
    actor.alertLevel = 0.0F;
    iggy3d::NpcAlertStimulus stim;
    stim.targetPerceived = true;
    stim.proximity01 = kProximity01;
    stim.hasValidTarget = true;
    stim.visualConfirmed = true;  // visual sighting bypasses grace -> clean rise
    std::uint8_t lastBand = 0;
    for (std::uint64_t tick = 0; tick < 600U; ++tick) {
      iggy3d::npcStepAlert(actor, stim, profile, tick);
      const std::uint8_t band = iggy3d::alertBandIndex(actor.alertLevel, profile);
      for (std::uint8_t b = static_cast<std::uint8_t>(lastBand + 1); b <= band; ++b) {
        escTick[b] = tick + 1U;  // ticks of perception elapsed to first reach band b
        escReached[b] = true;
      }
      lastBand = band > lastBand ? band : lastBand;
      if (band >= 5U) {
        break;
      }
    }
  }
  std::cout << "\n-- Escalation (ticks of perception to first reach each band) --\n";
  std::cout << "  " << std::left << std::setw(11) << "band" << std::right << std::setw(7) << "ticks"
            << std::setw(11) << "seconds" << '\n';
  for (std::uint8_t b = 1; b <= 5; ++b) {
    printBandRow(kBandName[b], escTick[b], rateHz);
  }

  // --- 2) Decay: from combat, contact broken; ticks to fall back through each band --------
  std::array<std::uint64_t, 6> decTick = {0, 0, 0, 0, 0, 0};
  std::array<bool, 6> decReached = {false, false, false, false, false, true};  // Chasing at t=0
  {
    iggy3d::AiActorState actor;
    actor.alertLevel = profile.combatNorm;
    actor.lastRiseTick = 0;
    actor.maxAlertIndexThisEngagement = 5;
    iggy3d::NpcAlertStimulus stim;  // targetPerceived = false -> decay
    std::uint8_t lastBand = 5;
    for (std::uint64_t tick = 1; tick < 2600U; ++tick) {
      iggy3d::npcStepAlert(actor, stim, profile, tick);
      const std::uint8_t band = iggy3d::alertBandIndex(actor.alertLevel, profile);
      for (int b = lastBand - 1; b >= band; --b) {
        decTick[static_cast<std::size_t>(b)] = tick;  // ticks since contact broke
        decReached[static_cast<std::size_t>(b)] = true;
      }
      lastBand = band < lastBand ? band : lastBand;
      if (band == 0U) {
        break;
      }
    }
  }
  std::cout << "\n-- Decay from combat (ticks since contact broke to fall to each band) --\n";
  std::cout << "  " << std::left << std::setw(11) << "band" << std::right << std::setw(7) << "ticks"
            << std::setw(11) << "seconds" << '\n';
  for (int b = 4; b >= 0; --b) {
    printBandRow(kBandName[static_cast<std::size_t>(b)], decTick[static_cast<std::size_t>(b)],
                 rateHz);
  }

  // --- 3) Investigate dwell: ticks looking around at last-known before giving up ----------
  std::uint64_t dwellTicks = 0;
  {
    iggy3d::AiActorState actor;
    actor.hasLastKnownTarget = true;
    actor.lastKnownTargetPosition = {0.0F, 0.0F, 0.0F};
    actor.alertLevel = profile.searchingNorm;  // Searching band
    const iggy3d::Vec3 atSpot = actor.lastKnownTargetPosition;  // dist == 0 -> dwell
    for (std::uint64_t tick = 0; tick < 600U; ++tick) {
      const iggy3d::NpcInvestigateStep step = iggy3d::npcStepInvestigate(
          actor, atSpot, /*alertBand=*/3U, /*visualConfirmed=*/false,
          iggy3d::kPatrolArriveEpsilonMeters, iggy3d::kInvestigateDwellTicks);
      ++dwellTicks;
      if (!step.active) {
        break;  // gave up (memory cleared)
      }
    }
  }
  std::cout << "\n-- Investigate dwell (look-around at last-known before giving up) --\n";
  printBandRow("dwell", dwellTicks, rateHz);

  // --- Knobs: current values + what each controls -----------------------------------------
  std::cout << "\n-- Knobs (AlertProfile melee_training + investigate + clock) --\n";
  const auto knobF = [](std::string_view name, float value, std::string_view what) {
    std::cout << "  " << std::left << std::setw(20) << name << std::right << std::setw(9)
              << std::fixed << std::setprecision(4) << value << "   " << what << '\n';
  };
  const auto knobTicks = [rateHz](std::string_view name, std::uint32_t ticks,
                                  std::string_view what) {
    std::cout << "  " << std::left << std::setw(20) << name << std::right << std::setw(9) << ticks
              << "   (" << std::fixed << std::setprecision(2) << secs(ticks, rateHz) << " s) "
              << what << '\n';
  };
  knobF("riseRatePerTick", profile.riseRatePerTick,
        "alert/tick at full proximity while seen (x proximity01)");
  knobF("observantNorm", profile.observantNorm, "threshold where Observant begins");
  knobF("suspiciousNorm", profile.suspiciousNorm, "threshold where Suspicious begins");
  knobF("searchingNorm", profile.searchingNorm, "threshold where Searching begins");
  knobF("agitatedNorm", profile.agitatedNorm, "threshold where Alert(agitated) begins");
  knobF("combatNorm", profile.combatNorm, "threshold where Chasing/combat begins");
  knobTicks("drainTicks[0]", profile.drainTicks[0], "ticks to drain one Observant band width");
  knobTicks("drainTicks[1]", profile.drainTicks[1], "ticks to drain one Suspicious band width");
  knobTicks("drainTicks[2]", profile.drainTicks[2], "ticks to drain one Searching band width");
  knobTicks("drainTicks[3]", profile.drainTicks[3], "ticks to drain one Alert(agitated) band width");
  knobTicks("deadTimeTicks", profile.deadTimeTicks, "hold flat after last rise before decay resumes");
  knobF("graceFrac", profile.graceFrac, "up-hysteresis: cap on swallowed non-visual rises");
  knobTicks("graceWindowTicks", profile.graceWindowTicks, "anti-spam window for non-visual rises");
  std::cout << "  " << std::left << std::setw(20) << "graceCountLimit" << std::right << std::setw(9)
            << profile.graceCountLimit << "   max non-visual rises swallowed per window (count)\n";
  knobTicks("kInvestigateDwellTicks", iggy3d::kInvestigateDwellTicks,
            "look-around at last-known before giving up");
  std::cout << "  " << std::left << std::setw(20) << "fixedTickRateHz" << std::right << std::setw(9)
            << rateHz << "   clock rate used for the ticks->seconds conversion\n";
  std::cout << "===========================================================================\n\n";

  // --- Sanity invariants (not magic numbers) ----------------------------------------------
  for (std::uint8_t b = 1; b <= 5; ++b) {
    ok = ok && expect(escReached[b], std::string("escalation reaches ") + kBandName[b]);
    if (b > 1) {
      ok = ok && expect(escTick[b] > escTick[b - 1], "escalation ticks strictly ascending");
    }
  }
  for (int b = 4; b >= 0; --b) {
    ok = ok && expect(decReached[static_cast<std::size_t>(b)],
                      std::string("decay reaches ") + kBandName[static_cast<std::size_t>(b)]);
    if (b < 4) {
      ok = ok && expect(decTick[static_cast<std::size_t>(b)] > decTick[static_cast<std::size_t>(b + 1)],
                        "decay ticks strictly ascending");
    }
  }
  ok = ok && expect(dwellTicks == iggy3d::kInvestigateDwellTicks,
                    "dwell equals kInvestigateDwellTicks");
  return ok ? 0 : 1;
}
