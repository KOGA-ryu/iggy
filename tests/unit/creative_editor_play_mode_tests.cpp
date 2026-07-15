#include "EditorDesktopCommands.hpp"
#include "EditorPlayMode.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "render/FrameInput.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool addObject(cr::CreativeDocument& document,
               cr::CreativeObjectKind kind,
               std::string name,
               cr::CreativeVec3 position,
               std::optional<cr::CreativeBounds> bounds = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  if (bounds.has_value()) {
    request.bounds = *bounds;
    request.hasBoundsOverride = true;
  }
  return document.createObject(request).accepted;
}

cr::CreativeDocument playableDocument(bool includeActors = true,
                                      cr::CreativeDocumentId id = 901U) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Editor Play");
  static_cast<void>(document.assignId(id));
  static_cast<void>(addObject(
      document, cr::CreativeObjectKind::Floor, "Play Floor", {0.0, 0.0, 0.0},
      cr::CreativeBounds{{-8.0, 0.0, -8.0}, {8.0, 0.25, 8.0}}));
  static_cast<void>(addObject(document, cr::CreativeObjectKind::SpawnPoint,
                              "Player Spawn", {0.0, 0.25, 0.0}));
  if (includeActors) {
    static_cast<void>(addObject(document, cr::CreativeObjectKind::NpcSpawn,
                                "Friendly NPC", {3.0, 0.25, 0.0}));
    static_cast<void>(addObject(document, cr::CreativeObjectKind::EnemySpawn,
                                "Hostile Monster", {-3.0, 0.25, 0.0}));
  }
  return document;
}

app::CreativeEditorPlayStartReceipt start(
    app::CreativeEditorPlayMode& mode,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& catalog) {
  app::CreativeEditorPlayStartRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  return app::startCreativeEditorPlayMode(mode, std::move(request));
}

const iggy3d::CombatantState* findCombatant(
    const app::CreativeEditorPlayMode& mode,
    std::uint32_t factionId) {
  if (!mode.sandbox.has_value()) {
    return nullptr;
  }
  const auto& combatants = mode.sandbox->session.state().combat.combatants;
  const auto found = std::find_if(
      combatants.begin(), combatants.end(),
      [factionId](const iggy3d::CombatantState& combatant) {
        return combatant.factionId == factionId &&
               combatant.entity != iggy3d::EntityId{1U};
      });
  return found == combatants.end() ? nullptr : &*found;
}

cr::CreativeDocument aimedActorDocument(cr::CreativeObjectKind actorKind,
                                        float actorZ,
                                        bool addBlockingWall,
                                        cr::CreativeDocumentId id) {
  cr::CreativeDocument document = playableDocument(false, id);
  static_cast<void>(addObject(document, actorKind, "Aimed Actor",
                              {0.0, 0.25, actorZ}));
  if (addBlockingWall) {
    static_cast<void>(addObject(
        document, cr::CreativeObjectKind::Wall, "Blocking Wall",
        {0.0, 0.0, 0.0},
        cr::CreativeBounds{{-1.0, 0.0, -0.72}, {1.0, 2.5, -0.52}}));
  }
  return document;
}

app::CreativeEditorPlayTickReceipt tickAt(
    app::CreativeEditorPlayMode& mode,
    const cr::CreativeDocument& document,
    std::uint64_t timeNanoseconds,
    app::CreativeEditorPlayActionSample actions = {}) {
  app::CreativeEditorPlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.input.actions = actions;
  tick.monotonicTimeNanoseconds = timeNanoseconds;
  return app::tickCreativeEditorPlayMode(mode, tick);
}

bool runtimeBindingsAndActionEdgesAreDeterministic() {
  const cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeInputBindingAuditResult audit =
      cr::auditCreativeInputBindings(profile.bindingSpan());

  cr::CreativeInputFrame mouse;
  mouse.context = cr::CreativeInputContext::RuntimePlay;
  cr::setCreativeInputKey(mouse, cr::CreativeInputKey::MousePrimary, true);
  cr::CreativeInputRouteResult mouseRoute;
  mouseRoute.context = mouse.context;
  const app::CreativeEditorPlayActionSample mouseSample =
      app::sampleCreativeEditorPlayActions(
          mouse, mouseRoute, profile.bindingSpan());

  cr::CreativeInputFrame gamepad;
  gamepad.context = cr::CreativeInputContext::RuntimePlay;
  cr::setCreativeInputKey(
      gamepad, cr::CreativeInputKey::GamepadRightTrigger, true);
  cr::setCreativeInputKey(
      gamepad, cr::CreativeInputKey::GamepadLeftTrigger, true);
  cr::CreativeInputRouteResult gamepadRoute;
  gamepadRoute.context = gamepad.context;
  const app::CreativeEditorPlayActionSample gamepadSample =
      app::sampleCreativeEditorPlayActions(
          gamepad, gamepadRoute, profile.bindingSpan());

  app::CreativeEditorPlayActionRouterState router;
  const app::CreativeEditorPlayAction first =
      app::routeCreativeEditorPlayAction(router, gamepadSample);
  const app::CreativeEditorPlayAction held =
      app::routeCreativeEditorPlayAction(router, gamepadSample);
  static_cast<void>(app::routeCreativeEditorPlayAction(router, {}));
  const app::CreativeEditorPlayAction interact =
      app::routeCreativeEditorPlayAction(router, {false, true});
  app::CreativeEditorPlayActionRouterState contextRouter;
  const app::CreativeEditorPlayAction hiddenPress =
      app::routeCreativeEditorPlayAction(
          contextRouter, {true, false, false});
  const app::CreativeEditorPlayAction heldOnReturn =
      app::routeCreativeEditorPlayAction(
          contextRouter, {true, false, true});
  static_cast<void>(app::routeCreativeEditorPlayAction(
      contextRouter, {false, false, true}));
  const app::CreativeEditorPlayAction pressedAfterRelease =
      app::routeCreativeEditorPlayAction(
          contextRouter, {true, false, true});

  return expect(profile.bindingCount <= cr::kCreativeInputBindingCapacity &&
                    audit.conflictCount == 0U,
                "runtime bindings remain bounded and conflict free") &&
         expect(mouseSample.attackDown && !mouseSample.interactDown,
                "left mouse maps to runtime attack") &&
         expect(gamepadSample.attackDown && gamepadSample.interactDown,
                "R2 and L2 map to attack and interact in Play") &&
         expect(first == app::CreativeEditorPlayAction::Attack &&
                    held == app::CreativeEditorPlayAction::None &&
                    interact == app::CreativeEditorPlayAction::Interact,
                "attack wins simultaneous press and held actions do not repeat") &&
         expect(hiddenPress == app::CreativeEditorPlayAction::None &&
                    heldOnReturn == app::CreativeEditorPlayAction::None &&
                    pressedAfterRelease ==
                        app::CreativeEditorPlayAction::Attack,
                "UI context changes cannot turn a held trigger into an edge") &&
         expect(cr::toString(cr::CreativeInputContext::RuntimePlay) ==
                        "RuntimePlay" &&
                    cr::toString(cr::CreativeInputActionId::RuntimeAttack) ==
                        "RuntimeAttack" &&
                    cr::toString(
                        cr::CreativeInputActionId::RuntimeInteract) ==
                        "RuntimeInteract",
                "runtime input identities persist by stable semantic names");
}

bool targetResolverClassifiesRuntimeTruth() {
  iggy3d::StaticMeshAssetCatalog catalog;

  cr::CreativeDocument hostile = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -1.0F, false, 910U);
  app::CreativeEditorPlayMode hostileMode;
  if (!start(hostileMode, hostile, catalog).accepted) {
    return expect(false, "hostile target setup starts");
  }
  static_cast<void>(tickAt(hostileMode, hostile, 1U));
  const app::CreativeEditorPlayTarget hostileTarget = hostileMode.target;

  cr::CreativeDocument friendly = aimedActorDocument(
      cr::CreativeObjectKind::NpcSpawn, -1.0F, false, 911U);
  app::CreativeEditorPlayMode friendlyMode;
  if (!start(friendlyMode, friendly, catalog).accepted) {
    return expect(false, "friendly target setup starts");
  }
  static_cast<void>(tickAt(friendlyMode, friendly, 1U));
  const app::CreativeEditorPlayTarget friendlyTarget = friendlyMode.target;

  cr::CreativeDocument distant = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -3.0F, false, 912U);
  app::CreativeEditorPlayMode distantMode;
  if (!start(distantMode, distant, catalog).accepted) {
    return expect(false, "distant target setup starts");
  }
  static_cast<void>(tickAt(distantMode, distant, 1U));
  const app::CreativeEditorPlayTarget distantTarget = distantMode.target;

  cr::CreativeDocument blocked = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -1.2F, true, 913U);
  app::CreativeEditorPlayMode blockedMode;
  if (!start(blockedMode, blocked, catalog).accepted) {
    return expect(false, "blocked target setup starts");
  }
  static_cast<void>(tickAt(blockedMode, blocked, 1U));
  const app::CreativeEditorPlayTarget blockedTarget = blockedMode.target;

  return expect(hostileTarget.status ==
                        app::CreativeEditorPlayTargetStatus::Valid &&
                    hostileTarget.supportsAttack &&
                    hostileTarget.supportsInteract &&
                    app::creativeEditorPlayTargetAcceptsAction(
                        hostileTarget, app::CreativeEditorPlayAction::Attack),
                "clear hostile in reach is actionable") &&
         expect(friendlyTarget.status ==
                        app::CreativeEditorPlayTargetStatus::Friendly &&
                    friendlyTarget.friendly &&
                    !app::creativeEditorPlayTargetAcceptsAction(
                        friendlyTarget, app::CreativeEditorPlayAction::Attack) &&
                    app::creativeEditorPlayTargetAcceptsAction(
                        friendlyTarget,
                        app::CreativeEditorPlayAction::Interact),
                "friendly target blocks attack but allows interaction") &&
         expect(distantTarget.status ==
                    app::CreativeEditorPlayTargetStatus::OutOfRange,
                "reach classification matches command admission") &&
         expect(blockedTarget.status ==
                    app::CreativeEditorPlayTargetStatus::Blocked,
                "room collider blocks center-ray target") &&
         expect(app::toString(blockedTarget.status) == "blocked",
                "target status has stable HUD text");
}

bool attackAndInteractSubmitOncePerPress() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument hostile = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -1.0F, false, 914U);
  app::CreativeEditorPlayMode attackMode;
  if (!start(attackMode, hostile, catalog).accepted) {
    return expect(false, "attack setup starts");
  }
  const std::uint64_t hostileRevision = hostile.revision();
  const iggy3d::CombatantState* initialMonster =
      findCombatant(attackMode, 2U);
  if (initialMonster == nullptr) {
    return expect(false, "attack setup has monster combatant");
  }
  const std::int32_t initialHitPoints = initialMonster->hitPoints;
  const app::CreativeEditorPlayTickReceipt attackPressed = tickAt(
      attackMode, hostile, 1U, {true, false});
  const app::CreativeEditorPlayTickReceipt attackHeld = tickAt(
      attackMode, hostile, 50'000'001U, {true, false});
  const iggy3d::CombatantState* damagedMonster =
      findCombatant(attackMode, 2U);

  cr::CreativeDocument friendly = aimedActorDocument(
      cr::CreativeObjectKind::NpcSpawn, -1.0F, false, 915U);
  app::CreativeEditorPlayMode interactMode;
  if (!start(interactMode, friendly, catalog).accepted) {
    return expect(false, "interaction setup starts");
  }
  const app::CreativeEditorPlayTickReceipt interactPressed = tickAt(
      interactMode, friendly, 1U, {false, true});
  const app::CreativeEditorPlayTickReceipt interactExecuted = tickAt(
      interactMode, friendly, 50'000'001U, {false, true});

  return expect(attackPressed.actionSubmitted &&
                    attackPressed.attackCommandsSubmitted == 1U &&
                    attackPressed.status ==
                        app::CreativeEditorPlayTickStatus::ClockPrimed,
                "attack press queues one admitted runtime command") &&
         expect(!attackHeld.actionSubmitted &&
                    attackHeld.attackCommandsSubmitted == 0U &&
                    damagedMonster != nullptr &&
                    damagedMonster->hitPoints == initialHitPoints - 1,
                "held attack executes once without repeat") &&
         expect(hostile.revision() == hostileRevision,
                "runtime combat does not mutate authored document") &&
         expect(interactPressed.actionSubmitted &&
                    interactPressed.interactionCommandsSubmitted == 1U &&
                    !interactExecuted.actionSubmitted &&
                    interactMode.sandbox->session.state()
                            .transient.metrics.interactionExecutions == 1U,
                "friendly interaction submits and executes once");
}

bool playHudIsBoundedAndUsesTargetState() {
  cr::CreativeDocument document = aimedActorDocument(
      cr::CreativeObjectKind::NpcSpawn, -1.0F, false, 916U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted) {
    return expect(false, "HUD setup starts");
  }
  static_cast<void>(tickAt(mode, document, 1U));
  const app::CreativeEditorPlayScene scene =
      app::buildCreativeEditorPlayScene(mode);
  const iggy3d::DebugProjectionResult debug;
  iggy3d::FrameInput frame = iggy3d::makeCreativeVulkanFrame(
      scene.scene, debug, 1U, 1280U, 720U, mode.cameraYawDegrees,
      mode.cameraPitchDegrees, true, scene.cameraAnchorMeters,
      {240, 60, 800U, 600U});
  const app::CreativeEditorPlayHudFrame hud =
      app::buildCreativeEditorPlayHud(mode, frame);
  app::attachCreativeEditorPlayHud(hud, frame);
  const iggy3d::RenderUiRect& centerDot = hud.rects[hud.rectCount - 1U];

  return expect(!hud.capacityExceeded && hud.rectCount == 8U &&
                    hud.glyphQuadCount > 0U && hud.textGlyphCount > 0U,
                "play HUD stays inside fixed buffers") &&
         expect(centerDot.x == 639 && centerDot.y == 359 &&
                    centerDot.r > 0.9F && centerDot.g > 0.7F,
                "friendly reticle is yellow and centered in content viewport") &&
         expect(frame.ui.visible && frame.ui.rects == hud.rects.data() &&
                    frame.ui.textGlyphQuads == hud.glyphQuads.data() &&
                    iggy3d::validateFrameInput(frame) ==
                        iggy3d::FrameInputStatus::Valid,
                "bounded HUD attaches to a valid render frame");
}

bool startStopAndProjectionPreserveAuthoredDocument() {
  cr::CreativeDocument document = playableDocument();
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  const std::uint64_t revision = document.revision();
  const std::size_t objectCount = document.objectCount();

  const app::CreativeEditorPlayStartReceipt started =
      start(mode, document, catalog);
  const app::CreativeEditorPlayScene projected =
      app::buildCreativeEditorPlayScene(mode);
  const std::size_t npcCount = static_cast<std::size_t>(std::count_if(
      projected.scene.items.begin(), projected.scene.items.end(),
      [](const iggy3d::SceneItem& item) {
        return item.kind == iggy3d::SceneItemKind::Npc;
      }));
  const iggy3d::DebugProjectionResult debug;
  const iggy3d::FrameInput frame = iggy3d::makeCreativeVulkanFrame(
      projected.scene, debug, 1U, 1280U, 720U, mode.cameraYawDegrees,
      mode.cameraPitchDegrees, true, projected.cameraAnchorMeters);
  const cr::CreativeRuntimeSandboxStopReceipt stopped =
      app::stopCreativeEditorPlayMode(mode);

  return expect(started.accepted &&
                    started.status == app::CreativeEditorPlayStartStatus::Started &&
                    app::toString(started.status) == "started",
                "valid authored map starts play") &&
         expect(projected.available && projected.scene.room.loaded &&
                    projected.scene.playerCount == 1U && npcCount == 2U,
                "play scene projects room, player, npc, and monster") &&
         expect(iggy3d::validateFrameInput(frame) ==
                        iggy3d::FrameInputStatus::Valid &&
                    iggy3d::nearlyEqual(
                        frame.camera.worldEye,
                        projected.cameraAnchorMeters +
                            iggy3d::Vec3{0.0F, 1.7F, 0.0F}),
                "runtime projection produces a valid player-follow frame") &&
         expect(document.revision() == revision &&
                    document.objectCount() == objectCount,
                "start and projection do not mutate authored content") &&
         expect(stopped.stopped && !app::creativeEditorPlayModeActive(mode),
                "stop destroys only the runtime sandbox");
}

bool fixedTickMovementAndCatchUpAreBounded() {
  cr::CreativeDocument document = playableDocument(false, 902U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  const app::CreativeEditorPlayStartReceipt started =
      start(mode, document, catalog);
  if (!started.accepted || !mode.sandbox.has_value()) {
    return expect(false, "movement setup starts play");
  }
  const std::uint64_t revision = document.revision();
  const iggy3d::EntityState* before =
      mode.sandbox->session.state().world.findById({1U});
  if (before == nullptr) {
    return expect(false, "movement setup has local player");
  }
  const iggy3d::Vec3 startPosition = before->transform.position;

  app::CreativeEditorPlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.input.moveForward = 1.0F;
  tick.monotonicTimeNanoseconds = 1U;
  const app::CreativeEditorPlayTickReceipt primed =
      app::tickCreativeEditorPlayMode(mode, tick);
  tick.monotonicTimeNanoseconds = 50'000'000U;
  const app::CreativeEditorPlayTickReceipt early =
      app::tickCreativeEditorPlayMode(mode, tick);
  tick.monotonicTimeNanoseconds = 50'000'001U;
  const app::CreativeEditorPlayTickReceipt first =
      app::tickCreativeEditorPlayMode(mode, tick);
  const iggy3d::EntityState* afterFirst =
      mode.sandbox->session.state().world.findById({1U});
  const bool movedForward =
      afterFirst != nullptr && afterFirst->transform.position.z < startPosition.z;

  tick.monotonicTimeNanoseconds = 2'050'000'001U;
  const app::CreativeEditorPlayTickReceipt catchUp =
      app::tickCreativeEditorPlayMode(mode, tick);

  return expect(primed.status == app::CreativeEditorPlayTickStatus::ClockPrimed &&
                    primed.ticksAdvanced == 0U,
                "first play frame only primes the fixed-step clock") &&
         expect(early.status == app::CreativeEditorPlayTickStatus::NoTickDue &&
                    early.ticksAdvanced == 0U,
                "49,999,999 ns does not advance a 20 Hz tick") &&
         expect(first.status == app::CreativeEditorPlayTickStatus::Advanced &&
                    first.ticksAdvanced == 1U &&
                    first.movementCommandsSubmitted == 1U && movedForward,
                "50 ms submits one collision-backed move") &&
         expect(catchUp.status == app::CreativeEditorPlayTickStatus::Advanced &&
                    catchUp.ticksAdvanced == mode.tuning.maximumCatchUpTicks,
                "long frame is capped by the catch-up bound") &&
         expect(document.revision() == revision,
                "runtime movement leaves authored revision unchanged");
}

bool idleTicksAdvanceAndStaleDocumentsStop() {
  cr::CreativeDocument document = playableDocument(false, 903U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted) {
    return expect(false, "idle setup starts play");
  }

  app::CreativeEditorPlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.monotonicTimeNanoseconds = 10U;
  static_cast<void>(app::tickCreativeEditorPlayMode(mode, tick));
  tick.monotonicTimeNanoseconds = 50'000'010U;
  const app::CreativeEditorPlayTickReceipt idle =
      app::tickCreativeEditorPlayMode(mode, tick);

  static_cast<void>(addObject(document, cr::CreativeObjectKind::Prop,
                              "Authored During Play", {1.0, 0.25, 1.0}));
  tick.monotonicTimeNanoseconds = 100'000'010U;
  const app::CreativeEditorPlayTickReceipt stale =
      app::tickCreativeEditorPlayMode(mode, tick);

  return expect(idle.status == app::CreativeEditorPlayTickStatus::Advanced &&
                    idle.commandsSubmitted == 1U &&
                    idle.movementCommandsSubmitted == 0U &&
                    idle.sourceTick > 0U,
                "idle frame submits Wait so runtime systems advance") &&
         expect(stale.status ==
                        app::CreativeEditorPlayTickStatus::SourceDocumentChanged &&
                    !stale.active && !app::creativeEditorPlayModeActive(mode),
                "changed source document invalidates and stops sandbox");
}

bool desktopPlayTogglesAndBlocksEditing() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = playableDocument(false, 904U);
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const std::uint64_t revision = appState.facade.document().revision();
  app::CreativeEditorState editor;
  app::CreativeEditorPlayMode mode;
  iggy3d::StaticMeshAssetCatalog catalog;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId, &mode, &catalog};

  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::Play);
  const app::CreativeDesktopCommandResult started =
      app::dispatchCreativeDesktopCommands(frame, context);
  const bool activeAfterStart = app::creativeEditorPlayModeActive(mode);
  frame.clear();
  frame.push(app::CreativeDesktopCommandId::NewDocument);
  const app::CreativeDesktopCommandResult blocked =
      app::dispatchCreativeDesktopCommands(frame, context);
  frame.clear();
  frame.push(app::CreativeDesktopCommandId::Play);
  const app::CreativeDesktopCommandResult stopped =
      app::dispatchCreativeDesktopCommands(frame, context);
  const bool inactiveAfterStop = !app::creativeEditorPlayModeActive(mode);

  return expect(started.accepted && activeAfterStart,
                "desktop Play starts the runtime owner") &&
         expect(!blocked.accepted && !blocked.documentReplaced &&
                    appState.facade.document().revision() == revision,
                "desktop mutations are rejected while play owns the frame") &&
         expect(stopped.accepted && inactiveAfterStop,
                "desktop Play toggles to Stop");
}

bool invalidMapAndTuningFailClosed() {
  cr::CreativeDocument invalid = cr::CreativeDocument::create("Invalid Play");
  static_cast<void>(invalid.assignId(905U));
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  const app::CreativeEditorPlayStartReceipt missingSpawn =
      start(mode, invalid, catalog);

  cr::CreativeDocument valid = playableDocument(false, 906U);
  app::CreativeEditorPlayStartRequest badTuning;
  badTuning.document = &valid;
  badTuning.staticMeshAssetCatalog = &catalog;
  badTuning.tuning.maximumCatchUpTicks = 0U;
  const app::CreativeEditorPlayStartReceipt invalidTuning =
      app::startCreativeEditorPlayMode(mode, std::move(badTuning));

  return expect(!missingSpawn.accepted &&
                    missingSpawn.status ==
                        app::CreativeEditorPlayStartStatus::PreparationRejected &&
                    !app::creativeEditorPlayModeActive(mode),
                "invalid map never creates a runtime sandbox") &&
         expect(!invalidTuning.accepted &&
                    invalidTuning.status ==
                        app::CreativeEditorPlayStartStatus::InvalidTuning &&
                    !app::creativeEditorPlayModeActive(mode),
                "invalid fixed-step tuning fails before preparation");
}

}  // namespace

int main() {
  const bool ok = runtimeBindingsAndActionEdgesAreDeterministic() &&
                  targetResolverClassifiesRuntimeTruth() &&
                  attackAndInteractSubmitOncePerPress() &&
                  playHudIsBoundedAndUsesTargetState() &&
                  startStopAndProjectionPreserveAuthoredDocument() &&
                  fixedTickMovementAndCatchUpAreBounded() &&
                  idleTicksAdvanceAndStaleDocumentsStop() &&
                  desktopPlayTogglesAndBlocksEditing() &&
                  invalidMapAndTuningFailClosed();
  if (!ok) {
    return 1;
  }
  std::cout << "creative_editor_play_mode_tests: PASS\n";
  return 0;
}
