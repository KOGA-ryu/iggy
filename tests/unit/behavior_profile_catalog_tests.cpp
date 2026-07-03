// a9s1 — the NPC behavior-profile catalog is DATA, built ONCE. Loader round-trip of a custom
// profile, fail-closed validation, the tick-hoist proof (the stored catalog is read, not rebuilt),
// and the load-carry (catalog survives replaceStateFromLoad; a bare Session heals to built-ins).

#include "content/FixtureScenarioLoader.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/session/Session.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const char* kWardenScenario =
    "[scenario]\n"
    "id = \"warden_test\"\n"
    "[defaults]\n"
    "fixed_tick_rate_hz = 20\n"
    "interaction_range_meters = 1.500\n"
    "movement_distance_meters = 3.000\n"
    "slow_time_scale = 0.250\n"
    "initial_clock = \"Normal\"\n"
    "default_realtime_camera = \"ThirdPerson\"\n"
    "default_tactical_camera = \"TacticalOverhead\"\n"
    "[[players]]\n"
    "slot = 0\n"
    "kind = \"Local\"\n"
    "actor = \"player\"\n"
    "[[entities]]\n"
    "stable_name = \"player\"\n"
    "kind = \"Player\"\n"
    "active = true\n"
    "persistent = true\n"
    "position = [1.000, 0.000, 1.000]\n"
    "bounds_min = [-0.250, 0.000, -0.250]\n"
    "bounds_max = [0.250, 1.800, 0.250]\n"
    "targetable = false\n"
    "target_actions = []\n"
    "combatant = true\n"
    "faction_id = 1\n"
    "hit_points = 10\n"
    "max_hit_points = 10\n"
    "[[entities]]\n"
    "stable_name = \"guard\"\n"
    "kind = \"Npc\"\n"
    "active = true\n"
    "persistent = true\n"
    "position = [9.000, 0.000, 9.000]\n"
    "bounds_min = [-0.250, 0.000, -0.250]\n"
    "bounds_max = [0.250, 1.200, 0.250]\n"
    "targetable = true\n"
    "target_actions = [\"Attack\", \"Inspect\"]\n"
    "combatant = true\n"
    "faction_id = 2\n"
    "hit_points = 3\n"
    "max_hit_points = 3\n"
    "[[ai_actors]]\n"
    "actor = \"guard\"\n"
    "behavior_profile_id = \"warden\"\n"
    "[[behavior_profiles]]\n"
    "id = \"warden\"\n"
    "engagement_policy = \"hostile\"\n"
    "perception_radius_meters = 7.500\n"
    "alert_observant_norm = 0.070\n"
    "alert_suspicious_norm = 0.270\n"
    "alert_searching_norm = 0.440\n"
    "alert_agitated_norm = 0.790\n"
    "alert_combat_norm = 1.000\n"
    "alert_rise_rate_per_tick = 0.055\n"
    "alert_drain_ticks = [111, 222, 333, 444]\n"
    "alert_dead_time_ticks = 7\n"
    "sound_hearing_threshold_db = 21.500\n"
    "sound_falloff_coeff = 8.500\n"
    "weight_suspicion = 1.500\n"
    "weight_ally = 2.000\n"
    "[[objectives]]\n"
    "id = \"guard_the_room\"\n"
    "initial_status = \"Active\"\n"
    "condition = \"InventoryContains\"\n"
    "player_slot = 0\n"
    "item_id = \"phantom_key\"\n"
    "item_count = 1\n"
    "complete_status = \"Complete\"\n";

iggy3d::SessionCreateRequest requestFor(const iggy3d::FixtureScenarioSeed& seed) {
  iggy3d::SessionCreateRequest request;
  request.packageId = "iggy3d.warden_test";
  request.config = seed.config;
  request.seed = seed;
  return request;
}

iggy3d::NpcBehaviorProfile profileWithId(const std::string& id) {
  iggy3d::NpcBehaviorProfile p;
  p.id.value = id;
  return p;
}

bool loaderResolvesCustomProfile() {
  const iggy3d::ScenarioLoadResult parsed = iggy3d::parseScenarioText(kWardenScenario);
  bool ok = expect(parsed.status == iggy3d::ScenarioLoadStatus::Ok, "warden scenario parses Ok");
  ok = ok && expect(parsed.seed.behaviorProfiles.size() == 1U, "one authored profile parsed");
  if (!ok) {
    return false;
  }
  const iggy3d::NpcBehaviorProfile& authored = parsed.seed.behaviorProfiles.front();
  ok = ok && expect(authored.id.value == "warden" && authored.alertProfile.drainTicks[0] == 111U &&
                        authored.alertProfile.deadTimeTicks == 7U &&
                        authored.soundConfig.falloffCoeff == 8.5F &&
                        authored.personalityWeights.allyWeight == 2.0F,
                    "authored profile fields round-trip through the parser");

  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(requestFor(parsed.seed));
  ok = ok && expect(created.status == iggy3d::ResultStatus::Ok, "warden session creates Ok");
  if (created.status != iggy3d::ResultStatus::Ok) {
    return false;
  }
  const iggy3d::Session& session = created.value;
  const iggy3d::NpcBehaviorProfileResolveResult resolved = iggy3d::resolveNpcBehaviorProfile(
      {&session.state().behaviorProfileCatalog, "warden"});
  return ok && expect(resolved.ok && resolved.profile.alertProfile.drainTicks[0] == 111U &&
                          resolved.profile.soundConfig.falloffCoeff == 8.5F &&
                          resolved.profile.personalityWeights.allyWeight == 2.0F,
                      "the session catalog resolves the authored custom profile");
}

bool createFailsClosed(std::vector<iggy3d::NpcBehaviorProfile> customs, std::string_view expectedCode) {
  iggy3d::FixtureScenarioSeed seed = iggy3d::parseScenarioText(kWardenScenario).seed;
  seed.behaviorProfiles = std::move(customs);
  const iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(requestFor(seed));
  return expect(created.status != iggy3d::ResultStatus::Ok &&
                    created.error.code == std::string(expectedCode),
                std::string("fail-closed: ") + std::string(expectedCode));
}

bool failClosedCases() {
  bool ok = createFailsClosed({profileWithId("default")}, "session.behavior_profile_builtin_collision");
  ok = ok && createFailsClosed({profileWithId("dup"), profileWithId("dup")},
                               "session.behavior_profile_duplicate_id");
  ok = ok && createFailsClosed({profileWithId("Bad Id")}, "session.behavior_profile_invalid_id");
  iggy3d::NpcBehaviorProfile badAlert = profileWithId("weird");
  badAlert.alertProfile.observantNorm = 0.5F;   // observant > suspicious -> non-ascending -> invalid
  badAlert.alertProfile.suspiciousNorm = 0.1F;
  ok = ok && createFailsClosed({badAlert}, "session.behavior_profile_invalid_config");
  return ok;
}

bool tickReadsStoredCatalogNotRebuild() {
  iggy3d::Result<iggy3d::Session> created =
      iggy3d::Session::create(requestFor(iggy3d::parseScenarioText(kWardenScenario).seed));
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "hoist session creates")) {
    return false;
  }
  iggy3d::Session session = std::move(created.value);

  // Mutate the STORED catalog: append a marker profile. If the tick rebuilt the catalog it would be
  // gone; if it reads the stored one (the hoist), it survives.
  session.mutableStateForOwnedSystems().behaviorProfileCatalog.profiles.push_back(
      profileWithId("injected_marker"));
  const std::size_t sizeBefore = session.state().behaviorProfileCatalog.profiles.size();

  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  const iggy3d::EntityId playerId = player != nullptr ? player->id : iggy3d::EntityId{};
  bool ok = true;
  for (int i = 0; i < 5; ++i) {
    iggy3d::CommandRecord wait;
    wait.playerSlot = 0;
    wait.actor = playerId;
    wait.kind = iggy3d::CommandKind::Wait;
    wait.source = iggy3d::CommandSource::LocalPlayer;
    (void)session.submitCommand(wait);
    ok = ok && expect(session.tick(nullptr).status == iggy3d::ResultStatus::Ok, "hoist tick ok");
  }
  const bool markerSurvives = iggy3d::resolveNpcBehaviorProfile(
                                  {&session.state().behaviorProfileCatalog, "injected_marker"})
                                  .ok;
  return ok &&
         expect(markerSurvives, "the tick READS the stored catalog (mutation survives -- no rebuild)") &&
         expect(session.state().behaviorProfileCatalog.profiles.size() == sizeBefore,
                "the catalog is byte-stable across ticks (no churn)");
}

bool loadCarriesCatalogAndBareHealsToBuiltIns() {
  iggy3d::Result<iggy3d::Session> created =
      iggy3d::Session::create(requestFor(iggy3d::parseScenarioText(kWardenScenario).seed));
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "carry session creates")) {
    return false;
  }
  iggy3d::Session session = std::move(created.value);

  // Model a loaded state whose catalog is EMPTY (the SaveCodec never carried it); replaceStateFromLoad
  // must CARRY the live warden catalog into it.
  iggy3d::SessionState loaded = session.state();
  loaded.behaviorProfileCatalog = {};
  const iggy3d::SessionLoadResult load = session.replaceStateFromLoad(std::move(loaded));
  bool ok = expect(load.status == iggy3d::SessionLoadStatus::Ok, "replaceStateFromLoad ok");
  ok = ok && expect(iggy3d::resolveNpcBehaviorProfile(
                        {&session.state().behaviorProfileCatalog, "warden"})
                        .ok,
                    "the custom catalog is CARRIED across the load swap");

  // A bare Session(state) with an empty catalog heals to built-ins on its first tick.
  iggy3d::SessionState bareState = session.state();
  bareState.behaviorProfileCatalog = {};
  iggy3d::Session bare(std::move(bareState));
  const iggy3d::EntityState* player = bare.state().world.findByStableName("player");
  iggy3d::CommandRecord wait;
  wait.playerSlot = 0;
  wait.actor = player != nullptr ? player->id : iggy3d::EntityId{};
  wait.kind = iggy3d::CommandKind::Wait;
  wait.source = iggy3d::CommandSource::LocalPlayer;
  (void)bare.submitCommand(wait);
  ok = ok && expect(bare.tick(nullptr).status == iggy3d::ResultStatus::Ok, "bare session ticks");
  ok = ok && expect(iggy3d::resolveNpcBehaviorProfile(
                        {&bare.state().behaviorProfileCatalog, "default"})
                        .ok,
                    "a bare Session heals its empty catalog to the built-ins");
  return ok;
}

bool catalogBuildIsDeterministic() {
  const std::vector<iggy3d::NpcBehaviorProfile> customs = {profileWithId("alpha"), profileWithId("beta")};
  const iggy3d::NpcBehaviorProfileCatalogResult a = iggy3d::buildNpcBehaviorProfileCatalog(customs);
  const iggy3d::NpcBehaviorProfileCatalogResult b = iggy3d::buildNpcBehaviorProfileCatalog(customs);
  bool ok = expect(a.status == iggy3d::NpcBehaviorProfileCatalogStatus::Ok &&
                       a.catalog.profiles.size() == 5U,
                   "built-ins (3) + customs (2) = 5 profiles");
  if (!ok) {
    return false;
  }
  for (std::size_t i = 0; i < a.catalog.profiles.size(); ++i) {
    ok = ok && (a.catalog.profiles[i].id.value == b.catalog.profiles[i].id.value);
  }
  return expect(ok && a.catalog.profiles[0].id.value == "default" &&
                    a.catalog.profiles[3].id.value == "alpha",
                "catalog build is deterministic: built-ins first, then customs in order");
}

}  // namespace

int main() {
  const bool ok = loaderResolvesCustomProfile() && failClosedCases() &&
                  tickReadsStoredCatalogNotRebuild() && loadCarriesCatalogAndBareHealsToBuiltIns() &&
                  catalogBuildIsDeterministic();
  return ok ? 0 : 1;
}
