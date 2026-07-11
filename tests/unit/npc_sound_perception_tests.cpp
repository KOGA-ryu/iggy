#include "runtime/ai/NpcSoundPerception.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float tol) { return std::fabs(lhs - rhs) <= tol; }

iggy3d::Vec3 at(float x, float z) { return iggy3d::Vec3{x, 0.0F, z}; }

iggy3d::SoundEvent sound(float loudnessDb, iggy3d::Vec3 origin) {
  iggy3d::SoundEvent event;
  event.originMeters = origin;
  event.loudnessDb = loudnessDb;
  return event;
}

// A 50 dB footstep 3 m away in the same room is clearly heard with meaningful alert.
bool sameRoomFootstepIsClearlyHeard() {
  const iggy3d::SoundPerceptionConfig config;  // threshold 20, falloff 9
  const iggy3d::SoundEvent footstep = sound(50.0F, at(3.0F, 0.0F));
  const float audibility = iggy3d::soundAudibilityDb(footstep, at(0.0F, 0.0F), config, false);
  // 50 - 9*log10(3) = 50 - 4.294 = 45.706.
  return expect(near(audibility, 45.706F, 0.01F), "footstep audibility falloff") &&
         expect(iggy3d::hearsSound(audibility, config.hearingThresholdDb), "footstep heard") &&
         expect(iggy3d::soundToAlertUnits(audibility, config.hearingThresholdDb, 1.0F, 30.0F) > 1.0F,
                "footstep alert above baseline");
}

// The hearing gate is a STRICT greater-than: exactly at the threshold is NOT heard.
bool hearingThresholdIsStrict() {
  const float t = 20.0F;
  const float justAbove = std::nextafter(t, 100.0F);
  const float justBelow = std::nextafter(t, 0.0F);
  bool ok = expect(!iggy3d::hearsSound(t, t), "at threshold not heard") &&
            expect(iggy3d::hearsSound(justAbove, t), "one ulp above heard") &&
            expect(!iggy3d::hearsSound(justBelow, t), "one ulp below not heard");
  // At 1 m the falloff is zero (log10(1)=0), so audibility == loudness exactly: a clean edge.
  const iggy3d::SoundPerceptionConfig config;
  ok = ok && expect(iggy3d::soundAudibilityDb(sound(20.0F, at(1.0F, 0.0F)), at(0.0F, 0.0F),
                                              config, false) == 20.0F,
                    "audibility equals loudness at 1 m") &&
       expect(!iggy3d::hearsSound(iggy3d::soundAudibilityDb(sound(20.0F, at(1.0F, 0.0F)),
                                                            at(0.0F, 0.0F), config, false),
                                  config.hearingThresholdDb),
              "20 dB at 1 m sits at threshold, not heard") &&
       expect(iggy3d::hearsSound(iggy3d::soundAudibilityDb(sound(21.0F, at(1.0F, 0.0F)),
                                                           at(0.0F, 0.0F), config, false),
                                 config.hearingThresholdDb),
              "21 dB at 1 m clears the threshold");
  return ok;
}

// Audibility decreases monotonically with distance.
bool distanceFalloffIsMonotonic() {
  const iggy3d::SoundPerceptionConfig config;
  const std::array<float, 5> distances = {1.0F, 2.0F, 4.0F, 8.0F, 16.0F};
  bool ok = true;
  float previous = std::numeric_limits<float>::infinity();
  for (const float d : distances) {
    const float a = iggy3d::soundAudibilityDb(sound(60.0F, at(d, 0.0F)), at(0.0F, 0.0F), config, false);
    ok = ok && expect(a < previous, "audibility strictly decreases with distance");
    previous = a;
  }
  return ok;
}

// A blocker between source and listener costs EXACTLY perWallLossDb (once), and can silence.
bool blockerCostsExactlyOneWallLoss() {
  const iggy3d::SoundPerceptionConfig config;  // perWallLossDb 20
  const iggy3d::SoundEvent event = sound(35.0F, at(1.0F, 0.0F));
  const float clear = iggy3d::soundAudibilityDb(event, at(0.0F, 0.0F), config, false);
  const float blocked = iggy3d::soundAudibilityDb(event, at(0.0F, 0.0F), config, true);
  return expect(clear == 35.0F, "clear audibility at 1 m") &&
         expect(near(clear - blocked, config.perWallLossDb, 1e-4F),
                "blocker costs exactly one wall loss") &&
         expect(iggy3d::hearsSound(clear, config.hearingThresholdDb), "clear is heard") &&
         expect(!iggy3d::hearsSound(blocked, config.hearingThresholdDb),
                "blocker silences the sound");
}

// Alert units saturate at alertMax at point blank; never exceed it.
bool alertUnitsClampToMax() {
  return expect(iggy3d::soundToAlertUnits(100.0F, 20.0F, 1.0F, 30.0F) == 30.0F,
                "loud sound clamps to alertMax") &&
         expect(iggy3d::soundToAlertUnits(19.0F, 20.0F, 1.0F, 30.0F) == 0.0F,
                "sub-threshold clamps to zero floor");
}

// The cheap range-sphere culls a beyond-range event even if the audibility math would pass it.
bool rangeRejectCullsFarSounds() {
  const iggy3d::SoundPerceptionConfig config;
  // 30 dB audible range = 2^0 * 2.2 = 2.2 m. A 30 dB source at 5 m is culled.
  const std::vector<iggy3d::SoundEvent> events = {sound(30.0F, at(5.0F, 0.0F))};
  const std::array<bool, 1> blockers = {false};
  const iggy3d::SoundPerceptionResult far =
      iggy3d::resolveLoudestSound(events, at(0.0F, 0.0F), config, blockers);
  bool ok = expect(near(iggy3d::soundAudibleRangeMeters(30.0F), 2.2F, 1e-4F),
                   "30 dB audible range is 2.2 m") &&
            expect(!far.heard, "beyond-range 30 dB sound is culled");
  // The same source within range is heard.
  const std::vector<iggy3d::SoundEvent> nearEvents = {sound(30.0F, at(2.0F, 0.0F))};
  const iggy3d::SoundPerceptionResult inRange =
      iggy3d::resolveLoudestSound(nearEvents, at(0.0F, 0.0F), config, blockers);
  ok = ok && expect(inRange.heard, "in-range 30 dB sound is heard");
  return ok;
}

// Highest-wins per tick: the loudest heard event supplies the result, including investigatePos.
bool highestWinsPerTick() {
  const iggy3d::SoundPerceptionConfig config;
  const std::vector<iggy3d::SoundEvent> events = {
      sound(45.0F, at(4.0F, 0.0F)), sound(65.0F, at(-3.0F, 0.0F)), sound(50.0F, at(0.0F, 5.0F))};
  const std::array<bool, 3> blockers = {false, false, false};
  const iggy3d::SoundPerceptionResult result =
      iggy3d::resolveLoudestSound(events, at(0.0F, 0.0F), config, blockers);
  const float loudest =
      iggy3d::soundAudibilityDb(events[1], at(0.0F, 0.0F), config, false);
  return expect(result.heard, "at least one heard") &&
         expect(near(result.audibilityDb, loudest, 1e-4F), "loudest event wins audibility") &&
         expect(result.investigatePos.x == -3.0F && result.investigatePos.z == 0.0F,
                "investigate position is the loudest event's true origin");
}

// Bitwise-identical inputs produce identical results (log10/pow are deterministic).
bool resolutionIsDeterministic() {
  const iggy3d::SoundPerceptionConfig config;
  const std::vector<iggy3d::SoundEvent> events = {sound(55.0F, at(2.5F, 1.5F)),
                                                  sound(48.0F, at(-1.0F, 3.0F))};
  const std::array<bool, 2> blockers = {true, false};
  const iggy3d::SoundPerceptionResult a =
      iggy3d::resolveLoudestSound(events, at(0.0F, 0.0F), config, blockers);
  const iggy3d::SoundPerceptionResult b =
      iggy3d::resolveLoudestSound(events, at(0.0F, 0.0F), config, blockers);
  return expect(a.heard == b.heard && a.audibilityDb == b.audibilityDb &&
                    a.alertUnits == b.alertUnits &&
                    a.investigatePos.x == b.investigatePos.x &&
                    a.investigatePos.z == b.investigatePos.z,
                "identical inputs yield identical results");
}

}  // namespace

int main() {
  const bool ok = sameRoomFootstepIsClearlyHeard() && hearingThresholdIsStrict() &&
                  distanceFalloffIsMonotonic() && blockerCostsExactlyOneWallLoss() &&
                  alertUnitsClampToMax() && rangeRejectCullsFarSounds() && highestWinsPerTick() &&
                  resolutionIsDeterministic();
  return ok ? 0 : 1;
}
