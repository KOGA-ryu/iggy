// Pure playtest lifecycle decisions: no processes, no SDL calls.

#include "EditorPlaytestProcess.hpp"

#include "EditorPlaytestNames.hpp"

#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>

namespace {

namespace app = iggy3d_creative_app;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool launchDecision() {
  return expect(app::decidePlaytestLaunchAction(false) ==
                    app::PlaytestLaunchAction::SpawnFresh,
                "no child -> spawn fresh") &&
         expect(app::decidePlaytestLaunchAction(true) ==
                    app::PlaytestLaunchAction::ReplaceRunning,
                "child running -> kill-then-spawn");
}

bool shutdownDecision() {
  return expect(app::decidePlaytestShutdownAction(false) ==
                    app::PlaytestShutdownAction::None,
                "no child -> shutdown does nothing") &&
         expect(app::decidePlaytestShutdownAction(true) ==
                    app::PlaytestShutdownAction::KillAndReap,
                "child running -> shutdown kills and reaps");
}

bool exitMessages() {
  return expect(app::playtestExitStatusMessage(0) == "playtest ended",
                "clean exit message") &&
         expect(app::playtestExitStatusMessage(3) ==
                    "playtest crashed (exit 3)",
                "nonzero exit message") &&
         expect(app::playtestExitStatusMessage(-15) ==
                    "playtest crashed (signal 15)",
                "signal exit message") &&
         expect(app::playtestExitStatusMessage(-255) ==
                    "playtest crashed (signal 255)",
                "abnormal exit reports as the SDL sentinel signal") &&
         expect(app::playtestRunningStatusMessage() == "playtest running",
                "running status text");
}

bool stallDecision() {
  using app::decidePlaytestStalled;
  constexpr std::uint64_t kT = app::kPlaytestStallThresholdMs;
  return expect(!decidePlaytestStalled(true, kT - 1U, kT),
                "alive + fresh heartbeat -> ok") &&
         expect(decidePlaytestStalled(true, kT + 1U, kT),
                "alive + stale -> stalled") &&
         expect(!decidePlaytestStalled(false, kT * 10U, kT),
                "dead is never stalled (exit reporting owns it)") &&
         // Suspended-state heartbeats refresh liveness upstream, so their
         // age stays low -- semantically: suspended-but-heartbeating -> ok.
         expect(!decidePlaytestStalled(true, 0U, kT),
                "suspended-but-heartbeating (age refreshed) -> ok") &&
         expect(app::playtestStalledStatusMessage(7400U) ==
                    "playtest stalled (7s) -- Play to replace",
                "stalled message names the age and the remedy");
}

bool monitorRowFormatting() {
  app::PlaytestEvent runtime;
  runtime.kind = std::string(app::kPlaytestEventKindRuntimeEvent);
  runtime.fields = {{"kind", "interacted"}, {"tick", "140"},
                    {"actor", "1"}, {"target", "12"}};
  app::PlaytestEvent heartbeat;
  heartbeat.kind = std::string(app::kPlaytestEventKindHeartbeat);
  heartbeat.fields = {{"tick", "60"}, {"state", "suspended"}};
  app::PlaytestEvent ended;
  ended.kind = std::string(app::kPlaytestEventKindSessionEnded);
  ended.fields = {{"reason", "frame_limit_reached"}, {"frames", "240"}};
  app::PlaytestEvent unknown;
  unknown.kind = "teleport_used";
  unknown.fields = {{"from", "a"}};
  return expect(app::formatPlaytestMonitorRow(runtime) ==
                    "interacted tick=140 actor=1 target=12",
                "runtime row reads as a story line with raw ids") &&
         expect(app::formatPlaytestMonitorRow(heartbeat) ==
                    "heartbeat tick=60 (suspended)",
                "heartbeat row carries the sim state") &&
         expect(app::formatPlaytestMonitorRow(ended) ==
                    "session ended: frame_limit_reached (240 frames)",
                "ended row names reason and frames") &&
         expect(app::formatPlaytestMonitorRow(unknown) ==
                    "IGGY3DP1 teleport_used from=a",
                "unknown kind falls back to the raw wire line");
}

iggy3d::creative::CreativePlayActivationPayload payloadWithNpcAnchors(
    std::initializer_list<iggy3d::creative::CreativeObjectId> objectIds) {
  namespace cr = iggy3d::creative;
  iggy3d::creative::CreativePlayActivationPayload payload;
  for (cr::CreativeObjectId objectId : objectIds) {
    const std::string stableName =
        "creative_object_" + std::to_string(objectId);
    iggy3d::RoomAnchorAsset anchor;
    anchor.id = stableName + "_anchor";
    anchor.kind = "npc";
    anchor.runtimeStableName = stableName;
    payload.room.anchors.push_back(anchor);
    cr::CreativeNpcSpawnPlan actor;
    actor.objectId = objectId;
    actor.objectKind = cr::CreativeObjectKind::NpcSpawn;
    actor.anchor = anchor;
    actor.facingDirection = {0.0F, 0.0F, -1.0F};
    payload.npcSpawns.push_back(std::move(actor));
  }
  return payload;
}

bool nameMapBuildAndStaleness() {
  namespace cr = iggy3d::creative;
  cr::CreativeDocument document = cr::CreativeDocument::create("Names");
  static_cast<void>(document.assignId(1));
  cr::CreativeDocumentCreateRequest guard;
  guard.kind = cr::CreativeObjectKind::NpcSpawn;
  guard.name = "Aisle One Guard";
  guard.hasTransformOverride = true;
  const cr::CreativeObjectId guardId = document.createObject(guard).objectId;
  cr::CreativeDocumentCreateRequest unnamed;
  unnamed.kind = cr::CreativeObjectKind::NpcSpawn;
  unnamed.hasTransformOverride = true;
  const cr::CreativeObjectId unnamedId =
      document.createObject(unnamed).objectId;

  const auto payload = payloadWithNpcAnchors({guardId, unnamedId, 9999U});
  app::PlaytestEntityNameMap names =
      app::buildPlaytestNameMap(payload, document);

  const bool buildOk =
      expect(names.at(1U) == "player", "entity 1 is the player") &&
      expect(names.at(2U) == "Aisle One Guard",
             "named object resolves to its name") &&
      // createObject auto-fills empty names with the descriptor default, so
      // the kind+id fallback is defensive-only; assert the real contract.
      expect(names.at(3U) == document.findObject(unnamedId)->name &&
                 !names.at(3U).empty(),
             "unnamed create resolves to the descriptor default name") &&
      expect(names.find(4U) == names.end(),
             "missing document object stays unmapped (raw)");

  // THE STALENESS PIN: rename after capture -- the captured map must keep
  // the snapshot-time label; only a re-capture picks up the new name.
  cr::CreativeObject* live = document.findObject(guardId);
  live->name = "Renamed After Spawn";
  const bool staleOk =
      expect(names.at(2U) == "Aisle One Guard",
             "captured map keeps the snapshot-time label after a rename") &&
      expect(app::buildPlaytestNameMap(payload, document).at(2U) ==
                 "Renamed After Spawn",
             "the NEXT capture (next Play) picks up the new name");

  // Row rendering: resolved actor/target vs raw fallback for unmapped ids.
  app::PlaytestEvent moved;
  moved.kind = std::string(app::kPlaytestEventKindRuntimeEvent);
  moved.fields = {{"kind", "moved"}, {"tick", "9"}, {"actor", "2"}};
  app::PlaytestEvent interacted;
  interacted.kind = std::string(app::kPlaytestEventKindRuntimeEvent);
  interacted.fields = {{"kind", "interacted"}, {"actor", "1"},
                       {"target", "4"}};
  return buildOk && staleOk &&
         expect(app::formatPlaytestMonitorRow(moved, &names) ==
                    "moved Aisle One Guard tick=9",
                "resolved actor replaces the raw id") &&
         expect(app::formatPlaytestMonitorRow(interacted, &names) ==
                    "interacted player target=4",
                "player resolves; unmapped target stays raw") &&
         expect(app::formatPlaytestMonitorRow(moved, nullptr) ==
                    "moved tick=9 actor=2",
                "no map: everything stays raw");
}

}  // namespace

int main() {
  const bool ok = launchDecision() && shutdownDecision() && exitMessages() &&
                  stallDecision() && monitorRowFormatting() &&
                  nameMapBuildAndStaleness();
  if (ok) {
    std::cout << "playtest_lifecycle_tests passed\n";
  }
  return ok ? 0 : 1;
}
