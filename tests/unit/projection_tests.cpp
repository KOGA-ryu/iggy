#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::SessionCreateRequest createRequestFromPackage() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return create;
}

iggy3d::Session makeSession() {
  return iggy3d::Session::create(createRequestFromPackage()).value;
}

iggy3d::CommandRecord submittedInteract() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  return command;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord submittedRetry(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::CommandRecord submittedControl(iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord submittedWait() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

bool runQueuedCommand(iggy3d::Session& session) {
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 8, true, true});
  return run.status == iggy3d::SessionRunnerStatus::Advanced && run.ticksAdvanced == 1U;
}

iggy3d::Session makePickedUpSession() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedInteract());
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedRetry(1));
  (void)runQueuedCommand(session);
  return session;
}

iggy3d::Session makeCompletedSession() {
  iggy3d::Session session = makePickedUpSession();
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));
  (void)session.submitCommand(submittedWait());
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  (void)session.finalizeDemoIfComplete();
  return session;
}

const iggy3d::SceneItem* findSceneItem(const iggy3d::SceneProjectionResult& projection,
                                       std::string_view stableName) {
  for (const iggy3d::SceneItem& item : projection.items) {
    if (item.stableName == stableName) {
      return &item;
    }
  }
  return nullptr;
}

bool hasDebugKind(const iggy3d::DebugProjectionResult& projection,
                  iggy3d::DebugProjectionKind kind) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == kind) {
      return true;
    }
  }
  return false;
}

bool hasOutOfRangeRejection(const iggy3d::DebugProjectionResult& projection) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == iggy3d::DebugProjectionKind::CommandRejected &&
        item.commandId == 1U && item.sequence == 1U &&
        item.rejection == iggy3d::CommandRejectionReason::OutOfRange &&
        item.actor == iggy3d::EntityId{1} && item.target == iggy3d::EntityId{2}) {
      return true;
    }
  }
  return false;
}

bool firstRoomProjectionContainsInitialItems() {
  const iggy3d::Session session = makeSession();
  const iggy3d::SceneProjectionResult projection = iggy3d::buildSceneProjection(session.state());
  const iggy3d::SceneItem* player = findSceneItem(projection, "player");
  const iggy3d::SceneItem* key = findSceneItem(projection, "gold_key");
  const iggy3d::SceneItem* marker = findSceneItem(projection, "tactical_marker_alpha");

  return expect(projection.items.size() == 3U, "initial projection item count") &&
         expect(player != nullptr && player->kind == iggy3d::SceneItemKind::Player,
                "player projected") &&
         expect(player != nullptr && player->owningPlayerSlot == 0U, "player owning slot") &&
         expect(key != nullptr && key->kind == iggy3d::SceneItemKind::Pickup, "key projected") &&
         expect(key != nullptr && key->active && key->visible && key->interactable,
                "key active interactable") &&
         expect(key != nullptr && key->itemId == "gold_key", "key item id") &&
         expect(marker != nullptr && marker->kind == iggy3d::SceneItemKind::TacticalMarker,
                "marker projected") &&
         expect(projection.sourceStateHash == session.state().currentStateHash,
                "projection state hash copied") &&
         expect(projection.sourceTick == session.state().clock.tickIndex, "projection tick copied") &&
         expect(projection.cameraMode == iggy3d::CameraMode::ThirdPerson, "projection camera");
}

bool inactivePickupFilteringWorks() {
  const iggy3d::Session session = makePickedUpSession();
  const iggy3d::SceneProjectionResult activeOnly = iggy3d::buildSceneProjection(session.state());
  iggy3d::SceneProjectionConfig includeInactive;
  includeInactive.includeInactive = true;
  const iggy3d::SceneProjectionResult allItems =
      iggy3d::buildSceneProjection(session.state(), includeInactive);
  const iggy3d::SceneItem* hiddenKey = findSceneItem(activeOnly, "gold_key");
  const iggy3d::SceneItem* inactiveKey = findSceneItem(allItems, "gold_key");

  return expect(hiddenKey == nullptr, "inactive key hidden") &&
         expect(inactiveKey != nullptr, "inactive key included") &&
         expect(inactiveKey != nullptr && !inactiveKey->active && !inactiveKey->visible,
                "inactive key marked inactive") &&
         expect(inactiveKey != nullptr && inactiveKey->itemId == "gold_key",
                "inactive key retains item fact");
}

bool projectionDoesNotMutateRuntimeTruth() {
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::StateHashValue hashBefore = session.state().currentStateHash;
  const std::size_t logSizeBefore = session.state().commandLog.size();
  const iggy3d::CommandSequence nextSequenceBefore = session.state().commandLog.nextSequence();
  const iggy3d::CommandTick tickBefore = session.state().clock.tickIndex;

  iggy3d::SceneProjectionConfig sceneConfig;
  sceneConfig.includeInactive = true;
  const iggy3d::SceneProjectionResult scene =
      iggy3d::buildSceneProjection(session.state(), sceneConfig);
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());

  return expect(!scene.items.empty(), "scene projected") &&
         expect(!debug.items.empty(), "debug projected") &&
         expect(session.state().currentStateHash == hashBefore, "projection hash unchanged") &&
         expect(session.state().commandLog.size() == logSizeBefore, "projection log size unchanged") &&
         expect(session.state().commandLog.nextSequence() == nextSequenceBefore,
                "projection log cursor unchanged") &&
         expect(session.state().clock.tickIndex == tickBefore, "projection tick unchanged");
}

bool debugProjectionIncludesProofFacts() {
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());

  bool hasReach = false;
  bool hasObjective = false;
  for (const iggy3d::DebugProjectionItem& item : debug.items) {
    if (item.kind == iggy3d::DebugProjectionKind::ReachRadius &&
        item.actor == iggy3d::EntityId{1} && item.radiusMeters == 1.500F) {
      hasReach = true;
    }
    if (item.kind == iggy3d::DebugProjectionKind::ObjectiveState &&
        item.objectiveId == "collect_gold_key") {
      hasObjective = true;
    }
  }

  return expect(hasOutOfRangeRejection(debug), "debug out of range rejection") &&
         expect(hasReach, "debug reach radius") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::TargetCandidate),
                "debug target candidate") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::ClockMode), "debug clock") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::CameraMode), "debug camera") &&
         expect(hasObjective, "debug objective") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::StateHash), "debug hash") &&
         expect(debug.sourceStateHash == session.state().currentStateHash, "debug source hash");
}

bool saveLoadProjectionIsEquivalent() {
  const iggy3d::SessionCreateRequest create = createRequestFromPackage();
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  iggy3d::Session loaded = iggy3d::Session::create(create).value;
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, session.state().identity.packageId, session.state().identity.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);

  iggy3d::SceneProjectionConfig config;
  config.includeInactive = true;
  const iggy3d::SceneProjectionResult original =
      iggy3d::buildSceneProjection(session.state(), config);
  const iggy3d::SceneProjectionResult restored =
      iggy3d::buildSceneProjection(loaded.state(), config);
  const iggy3d::SceneItem* originalPlayer = findSceneItem(original, "player");
  const iggy3d::SceneItem* restoredPlayer = findSceneItem(restored, "player");
  const iggy3d::SceneItem* originalKey = findSceneItem(original, "gold_key");
  const iggy3d::SceneItem* restoredKey = findSceneItem(restored, "gold_key");

  return expect(saved.status == iggy3d::SaveLoadStatus::Ok, "save ok") &&
         expect(load.status == iggy3d::SaveLoadStatus::Ok, "load ok") &&
         expect(original.items.size() == restored.items.size(), "projection item count roundtrip") &&
         expect(original.sourceStateHash == restored.sourceStateHash, "projection hash roundtrip") &&
         expect(original.sourceTick == restored.sourceTick, "projection tick roundtrip") &&
         expect(originalPlayer != nullptr && restoredPlayer != nullptr, "player roundtrip exists") &&
         expect(originalPlayer != nullptr && restoredPlayer != nullptr &&
                    iggy3d::nearlyEqual(originalPlayer->transform.position,
                                        restoredPlayer->transform.position),
                "player position roundtrip") &&
         expect(originalKey != nullptr && restoredKey != nullptr, "key roundtrip exists") &&
         expect(originalKey != nullptr && restoredKey != nullptr &&
                    originalKey->active == restoredKey->active &&
                    originalKey->itemId == restoredKey->itemId,
                "key inactive fact roundtrip");
}

}  // namespace

int main() {
  bool ok = true;
  ok = firstRoomProjectionContainsInitialItems() && ok;
  ok = inactivePickupFilteringWorks() && ok;
  ok = projectionDoesNotMutateRuntimeTruth() && ok;
  ok = debugProjectionIncludesProofFacts() && ok;
  ok = saveLoadProjectionIsEquivalent() && ok;
  return ok ? 0 : 1;
}
