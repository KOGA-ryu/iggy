#include "EditorDesktopCommands.hpp"
#include "app/iggy3d/creative/play/PlaySession.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "render/FrameInput.hpp"
#include "runtime/inventory/InventorySystem.hpp"

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

cr::CreativeDocumentCreateReceipt createObject(
    cr::CreativeDocument& document,
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
  return document.createObject(request);
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

app::CreativePlayStartReceipt start(
    app::CreativePlaySession& mode,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& catalog) {
  app::CreativePlayStartRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  return app::startCreativePlaySession(mode, std::move(request));
}

const iggy3d::CombatantState* findCombatant(
    const app::CreativePlaySession& mode,
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

app::CreativePlayTickReceipt tickAt(
    app::CreativePlaySession& mode,
    const cr::CreativeDocument& document,
    std::uint64_t timeNanoseconds,
    app::CreativePlayActionSample actions = {}) {
  app::CreativePlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.input.actions = actions;
  tick.monotonicTimeNanoseconds = timeNanoseconds;
  return app::tickCreativePlaySession(mode, tick);
}

bool moveRuntimeEntity(app::CreativePlaySession& mode,
                       iggy3d::EntityId entityId,
                       iggy3d::Vec3 position) {
  if (!mode.sandbox.has_value()) {
    return false;
  }
  const iggy3d::EntityState* entity =
      mode.sandbox->session.state().world.findById(entityId);
  if (entity == nullptr) {
    return false;
  }
  iggy3d::Transform3 transform = entity->transform;
  transform.position = position;
  return mode.sandbox->session.mutableStateForOwnedSystems()
             .world.updateTransform(entityId, transform)
             .status == iggy3d::WorldStatus::Ok;
}

bool runtimeBindingsAndActionEdgesAreDeterministic() {
  const cr::CreativeControlProfile profile =
      cr::makeDefaultCreativeControlProfile();
  const cr::CreativeInputBindingAuditResult audit =
      cr::auditCreativeInputBindings(profile.bindingSpan());

  cr::CreativeInputFrame mouse;
  mouse.context = cr::CreativeInputContext::RuntimePlay;
  cr::setCreativeInputKey(mouse, cr::CreativeInputKey::MousePrimary, true);
  cr::CreativeInputRouterState mouseInputRouter;
  const cr::CreativeInputRouteResult mouseRoute =
      cr::routeCreativeInput(mouseInputRouter, mouse, profile.bindingSpan());
  const app::CreativePlayActionSample mouseSample =
      app::sampleCreativePlayActions(mouseRoute);

  cr::CreativeInputFrame gamepad;
  gamepad.context = cr::CreativeInputContext::RuntimePlay;
  cr::setCreativeInputKey(
      gamepad, cr::CreativeInputKey::GamepadRightTrigger, true);
  cr::setCreativeInputKey(
      gamepad, cr::CreativeInputKey::GamepadLeftTrigger, true);
  cr::CreativeInputRouterState gamepadInputRouter;
  const cr::CreativeInputRouteResult gamepadRoute =
      cr::routeCreativeInput(gamepadInputRouter, gamepad,
                             profile.bindingSpan());
  const app::CreativePlayActionSample gamepadSample =
      app::sampleCreativePlayActions(gamepadRoute);

  app::CreativePlayActionRouterState router;
  const app::CreativePlayAction first =
      app::routeCreativePlayAction(router, gamepadSample);
  const app::CreativePlayAction held =
      app::routeCreativePlayAction(router, gamepadSample);
  static_cast<void>(app::routeCreativePlayAction(router, {}));
  const app::CreativePlayAction interact =
      app::routeCreativePlayAction(router, {false, true});

  cr::CreativeInputRouterState contextInputRouter;
  cr::CreativeInputFrame contextFrame;
  contextFrame.context = cr::CreativeInputContext::DesktopUi;
  cr::setCreativeInputKey(
      contextFrame, cr::CreativeInputKey::GamepadRightTrigger, true);
  const cr::CreativeInputRouteResult hiddenRoute =
      cr::routeCreativeInput(contextInputRouter, contextFrame,
                             profile.bindingSpan());
  const app::CreativePlayActionSample hiddenSample =
      app::sampleCreativePlayActions(hiddenRoute);
  app::CreativePlayActionRouterState contextRouter;
  const app::CreativePlayAction hiddenPress =
      app::routeCreativePlayAction(contextRouter, hiddenSample);

  contextFrame.context = cr::CreativeInputContext::RuntimePlay;
  const cr::CreativeInputRouteResult heldOnReturnRoute =
      cr::routeCreativeInput(contextInputRouter, contextFrame,
                             profile.bindingSpan());
  const app::CreativePlayAction heldOnReturn =
      app::routeCreativePlayAction(
          contextRouter,
          app::sampleCreativePlayActions(heldOnReturnRoute));

  cr::setCreativeInputKey(
      contextFrame, cr::CreativeInputKey::GamepadRightTrigger, false);
  const cr::CreativeInputRouteResult releaseRoute =
      cr::routeCreativeInput(contextInputRouter, contextFrame,
                             profile.bindingSpan());
  const app::CreativePlayAction release =
      app::routeCreativePlayAction(
          contextRouter, app::sampleCreativePlayActions(releaseRoute));

  cr::setCreativeInputKey(
      contextFrame, cr::CreativeInputKey::GamepadRightTrigger, true);
  const cr::CreativeInputRouteResult freshPressRoute =
      cr::routeCreativeInput(contextInputRouter, contextFrame,
                             profile.bindingSpan());
  const app::CreativePlayAction pressedAfterRelease =
      app::routeCreativePlayAction(
          contextRouter,
          app::sampleCreativePlayActions(freshPressRoute));

  return expect(profile.bindingCount <= cr::kCreativeInputBindingCapacity &&
                    audit.conflictCount == 0U,
                "runtime bindings remain bounded and conflict free") &&
         expect(mouseSample.attackDown && !mouseSample.interactDown,
                "left mouse maps to runtime attack") &&
         expect(gamepadSample.attackDown && gamepadSample.interactDown,
                "R2 and L2 map to attack and interact in Play") &&
         expect(first == app::CreativePlayAction::Attack &&
                    held == app::CreativePlayAction::None &&
                    interact == app::CreativePlayAction::Interact,
                "attack wins simultaneous press and held actions do not repeat") &&
         expect(hiddenPress == app::CreativePlayAction::None &&
                    !hiddenSample.enabled && !hiddenSample.attackDown &&
                    heldOnReturn == app::CreativePlayAction::None &&
                    release == app::CreativePlayAction::None &&
                    pressedAfterRelease ==
                        app::CreativePlayAction::Attack,
                "UI context requires release before a held trigger can fire") &&
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
  app::CreativePlaySession hostileMode;
  if (!start(hostileMode, hostile, catalog).accepted) {
    return expect(false, "hostile target setup starts");
  }
  static_cast<void>(tickAt(hostileMode, hostile, 1U));
  const app::CreativePlayTarget hostileTarget = hostileMode.target;

  cr::CreativeDocument friendly = aimedActorDocument(
      cr::CreativeObjectKind::NpcSpawn, -1.0F, false, 911U);
  app::CreativePlaySession friendlyMode;
  if (!start(friendlyMode, friendly, catalog).accepted) {
    return expect(false, "friendly target setup starts");
  }
  static_cast<void>(tickAt(friendlyMode, friendly, 1U));
  const app::CreativePlayTarget friendlyTarget = friendlyMode.target;

  cr::CreativeDocument distant = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -3.0F, false, 912U);
  app::CreativePlaySession distantMode;
  if (!start(distantMode, distant, catalog).accepted) {
    return expect(false, "distant target setup starts");
  }
  static_cast<void>(tickAt(distantMode, distant, 1U));
  const app::CreativePlayTarget distantTarget = distantMode.target;

  cr::CreativeDocument blocked = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -1.2F, true, 913U);
  app::CreativePlaySession blockedMode;
  if (!start(blockedMode, blocked, catalog).accepted) {
    return expect(false, "blocked target setup starts");
  }
  static_cast<void>(tickAt(blockedMode, blocked, 1U));
  const app::CreativePlayTarget blockedTarget = blockedMode.target;

  return expect(hostileTarget.status ==
                        app::CreativePlayTargetStatus::Valid &&
                    hostileTarget.supportsAttack &&
                    hostileTarget.supportsInteract &&
                    app::creativeEditorPlayTargetAcceptsAction(
                        hostileTarget, app::CreativePlayAction::Attack),
                "clear hostile in reach is actionable") &&
         expect(friendlyTarget.status ==
                        app::CreativePlayTargetStatus::Friendly &&
                    friendlyTarget.friendly &&
                    !app::creativeEditorPlayTargetAcceptsAction(
                        friendlyTarget, app::CreativePlayAction::Attack) &&
                    app::creativeEditorPlayTargetAcceptsAction(
                        friendlyTarget,
                        app::CreativePlayAction::Interact),
                "friendly target blocks attack but allows interaction") &&
         expect(distantTarget.status ==
                    app::CreativePlayTargetStatus::OutOfRange,
                "reach classification matches command admission") &&
         expect(blockedTarget.status ==
                    app::CreativePlayTargetStatus::Blocked,
                "room collider blocks center-ray target") &&
         expect(app::toString(blockedTarget.status) == "blocked",
                "target status has stable HUD text");
}

bool attackAndInteractSubmitOncePerPress() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument hostile = aimedActorDocument(
      cr::CreativeObjectKind::EnemySpawn, -1.0F, false, 914U);
  app::CreativePlaySession attackMode;
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
  const app::CreativePlayTickReceipt attackPressed = tickAt(
      attackMode, hostile, 1U, {true, false});
  const app::CreativePlayTickReceipt attackHeld = tickAt(
      attackMode, hostile, 50'000'001U, {true, false});
  const iggy3d::CombatantState* damagedMonster =
      findCombatant(attackMode, 2U);

  cr::CreativeDocument friendly = aimedActorDocument(
      cr::CreativeObjectKind::NpcSpawn, -1.0F, false, 915U);
  app::CreativePlaySession interactMode;
  if (!start(interactMode, friendly, catalog).accepted) {
    return expect(false, "interaction setup starts");
  }
  const app::CreativePlayTickReceipt interactPressed = tickAt(
      interactMode, friendly, 1U, {false, true});
  const app::CreativePlayTickReceipt interactExecuted = tickAt(
      interactMode, friendly, 50'000'001U, {false, true});

  return expect(attackPressed.actionSubmitted &&
                    attackPressed.attackCommandsSubmitted == 1U &&
                    attackPressed.status ==
                        app::CreativePlayTickStatus::ClockPrimed,
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
  app::CreativePlaySession mode;
  if (!start(mode, document, catalog).accepted) {
    return expect(false, "HUD setup starts");
  }
  static_cast<void>(tickAt(mode, document, 1U));
  const app::CreativePlayScene scene =
      app::buildCreativePlaySessionScene(mode);
  const iggy3d::DebugProjectionResult debug;
  iggy3d::FrameInput frame = iggy3d::makeCreativeVulkanFrame(
      scene.scene, debug, 1U, 1280U, 720U, mode.cameraYawDegrees,
      mode.cameraPitchDegrees, true, scene.cameraAnchorMeters,
      {240, 60, 800U, 600U});
  const app::CreativePlayHudFrame hud =
      app::buildCreativePlayHud(mode, frame);
  app::attachCreativePlayHud(hud, frame);
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

bool automaticLogicRunsInPlayAndProjectsOccupancy() {
  cr::CreativeDocument document = playableDocument(false, 919U);
  const cr::CreativeDocumentCreateReceipt trigger = createObject(
      document, cr::CreativeObjectKind::TriggerZone, "Play Trigger",
      {-1.0, 0.25, -1.0},
      cr::CreativeBounds{{-1.0, 0.25, -1.0}, {1.0, 2.25, 1.0}});
  const cr::CreativeDocumentCreateReceipt door = createObject(
      document, cr::CreativeObjectKind::Door, "Triggered Door",
      {3.0, 0.25, -0.25},
      cr::CreativeBounds{{3.0, 0.25, -0.25}, {4.0, 2.5, 0.25}});
  if (!trigger.accepted || !door.accepted ||
      !document
           .setLogicLink({trigger.objectId, door.objectId,
                          cr::CreativeLogicLinkAction::Toggle})
           .accepted) {
    return expect(false, "play automatic logic setup creates link");
  }

  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "play automatic logic setup starts");
  }
  const app::CreativePlayScene inactiveScene =
      app::buildCreativePlaySessionScene(mode);
  const app::CreativePlayScene selectedInactiveScene =
      app::buildCreativePlaySessionScene(mode, trigger.objectId);
  static_cast<void>(tickAt(mode, document, 1U));
  const app::CreativePlayTickReceipt entered =
      tickAt(mode, document, 50'000'001U);
  const app::CreativePlayScene activeScene =
      app::buildCreativePlaySessionScene(mode);
  const app::CreativePlayScene selectedActiveScene =
      app::buildCreativePlaySessionScene(mode, trigger.objectId);

  const auto doorState = std::find_if(
      mode.sandbox->interactables.begin(), mode.sandbox->interactables.end(),
      [&door](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == door.objectId;
      });
  if (doorState == mode.sandbox->interactables.end() ||
      inactiveScene.logicOverlay.lines.empty() ||
      activeScene.logicOverlay.lines.empty()) {
    return expect(false, "play automatic logic states project");
  }

  const iggy3d::DebugProjectionResult debug;
  iggy3d::FrameInput frame = iggy3d::makeCreativeVulkanFrame(
      activeScene.scene, debug, 2U, 1280U, 720U, mode.cameraYawDegrees,
      mode.cameraPitchDegrees, true, activeScene.cameraAnchorMeters);
  frame.creativeWireframeDebug.available = true;
  frame.creativeWireframeDebug.visible = true;
  frame.creativeWireframeDebug.lines = activeScene.logicOverlay.lines.data();
  frame.creativeWireframeDebug.lineCount =
      activeScene.logicOverlay.lines.size();

  return expect(inactiveScene.logicOverlay.lines.size() == 12U &&
                    inactiveScene.logicOverlay.sourceEdgeCount == 12U &&
                    inactiveScene.logicOverlay.lines.front().style == 0U &&
                    inactiveScene.logicOverlay.lines.front().color.g < 0.7F &&
                    inactiveScene.logicOverlay.lines.front().thickness < 0.1F,
                "empty trigger projects a restrained inactive box") &&
         expect(selectedInactiveScene.logicOverlay.lines.size() == 27U &&
                    selectedInactiveScene.logicOverlay.linkShaftCount == 1U &&
                    selectedInactiveScene.logicOverlay.linkArrowEdgeCount ==
                        2U &&
                    selectedInactiveScene.logicOverlay.targetEdgeCount == 12U &&
                    selectedInactiveScene.logicOverlay.lines[12].objectId ==
                        trigger.objectId &&
                    selectedInactiveScene.logicOverlay.lines[12].color.b >
                        0.9F,
                "selected idle circuit adds a cyan arrow and target box") &&
         expect(entered.status == app::CreativePlayTickStatus::Advanced &&
                    entered.automaticSourceTransitions == 1U &&
                    entered.automaticEffectsApplied == 1U &&
                    entered.automaticLogic ==
                        cr::CreativeRuntimeAutomaticLogicStatus::Applied &&
                    doorState->targetActive,
                "play tick applies first-entry trigger effect") &&
         expect(activeScene.logicOverlay.lines.size() == 12U &&
                    activeScene.logicOverlay.lines.front().style == 1U &&
                    activeScene.logicOverlay.lines.front().color.g > 0.9F &&
                    activeScene.logicOverlay.lines.front().objectId ==
                        trigger.objectId,
                "occupied trigger projects a green active box") &&
         expect(selectedActiveScene.logicOverlay.lines.size() == 27U &&
                    selectedActiveScene.logicOverlay.linkShaftCount == 1U &&
                    selectedActiveScene.logicOverlay.lines[12].color.g > 0.9F &&
                    selectedActiveScene.logicOverlay.lines[15].objectId ==
                        door.objectId &&
                    selectedActiveScene.logicOverlay.lines[15].color.g > 0.9F,
                "selected active circuit and opened target turn green") &&
         expect(iggy3d::validateFrameInput(frame) ==
                    iggy3d::FrameInputStatus::Valid,
                "automatic source overlay is valid bounded frame data");
}

bool manualAndInvalidLogicLinksRemainInspectableInPlay() {
  cr::CreativeDocument document = playableDocument(false, 920U);
  const cr::CreativeDocumentCreateReceipt source = createObject(
      document, cr::CreativeObjectKind::Switch, "Manual Switch",
      {-2.0F, 0.25F, -1.0F});
  const cr::CreativeDocumentCreateReceipt door = createObject(
      document, cr::CreativeObjectKind::Door, "Manual Door",
      {2.0F, 0.25F, -0.25F},
      cr::CreativeBounds{{2.0F, 0.25F, -0.25F},
                         {3.0F, 2.5F, 0.25F}});
  if (!source.accepted || !door.accepted ||
      !document
           .setLogicLink({source.objectId, door.objectId,
                          cr::CreativeLogicLinkAction::Open})
           .accepted) {
    return expect(false, "manual logic overlay setup creates link");
  }

  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "manual logic overlay setup starts");
  }
  const app::CreativePlayScene valid =
      app::buildCreativePlaySessionScene(mode, source.objectId);
  if (mode.sandbox->logicLinks.empty()) {
    return expect(false, "manual logic overlay has runtime link");
  }
  mode.sandbox->logicLinks.front().targetObjectId = cr::kInvalidObjectId;
  const app::CreativePlayScene invalid =
      app::buildCreativePlaySessionScene(mode, source.objectId);

  return expect(valid.logicOverlay.lines.size() == 27U &&
                    valid.logicOverlay.sourceEdgeCount == 12U &&
                    valid.logicOverlay.linkShaftCount == 1U &&
                    valid.logicOverlay.linkArrowEdgeCount == 2U &&
                    valid.logicOverlay.targetEdgeCount == 12U &&
                    valid.logicOverlay.invalidLinkCount == 0U &&
                    valid.logicOverlay.lines[12].color.g > 0.9F &&
                    valid.logicOverlay.lines[15].color.r > 0.9F &&
                    valid.logicOverlay.lines[15].color.g > 0.5F,
                "manual source projects its Open arrow and closed door state") &&
         expect(invalid.logicOverlay.lines.size() == 15U &&
                    invalid.logicOverlay.sourceEdgeCount == 12U &&
                    invalid.logicOverlay.linkShaftCount == 1U &&
                    invalid.logicOverlay.linkArrowEdgeCount == 2U &&
                    invalid.logicOverlay.targetEdgeCount == 0U &&
                    invalid.logicOverlay.invalidLinkCount == 1U &&
                    invalid.logicOverlay.lines[12].color.r > 0.9F &&
                    invalid.logicOverlay.lines[12].color.g < 0.3F,
                "missing runtime target remains visible as a red arrow stub");
}

bool platformLogicTargetProjectsEnabledAndRetractedState() {
  cr::CreativeDocument document = playableDocument(false, 921U);
  const cr::CreativeDocumentCreateReceipt source = createObject(
      document, cr::CreativeObjectKind::Switch, "Platform Switch",
      {-2.0F, 1.0F, 0.0F});
  const cr::CreativeDocumentCreateReceipt platform = createObject(
      document, cr::CreativeObjectKind::Platform, "Retractable Platform",
      {3.0F, 1.0F, 0.0F},
      cr::CreativeBounds{{2.0F, 1.0F, -1.0F}, {4.0F, 1.35F, 1.0F}});
  if (!source.accepted || !platform.accepted ||
      !document
           .setLogicLink({source.objectId, platform.objectId,
                          cr::CreativeLogicLinkAction::Toggle})
           .accepted) {
    return expect(false, "platform overlay setup creates logic target");
  }

  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "platform overlay setup starts");
  }
  const cr::CreativeRuntimeInteractableState* runtimeSource =
      cr::findCreativeRuntimeInteractableByObjectId(*mode.sandbox,
                                                    source.objectId);
  if (runtimeSource == nullptr) {
    return expect(false, "platform overlay source exists at runtime");
  }
  const app::CreativePlayScene enabled =
      app::buildCreativePlaySessionScene(mode, source.objectId);
  const cr::CreativeRuntimeInteractionEffectReceipt retracted =
      cr::applyCreativeRuntimeInteractionEffect(*mode.sandbox,
                                                runtimeSource->entity);
  const app::CreativePlayScene disabled =
      app::buildCreativePlaySessionScene(mode, source.objectId);

  return expect(enabled.logicOverlay.lines.size() == 27U &&
                    enabled.logicOverlay.targetEdgeCount == 12U &&
                    enabled.logicOverlay.invalidLinkCount == 0U &&
                    enabled.logicOverlay.lines[15].objectId ==
                        platform.objectId &&
                    enabled.logicOverlay.lines[15].color.g > 0.9F,
                "enabled platform projects as a valid green logic target") &&
         expect(retracted.accepted && retracted.changed &&
                    retracted.affectedTargetCount == 1U &&
                    retracted.affectedPlatformCount == 1U &&
                    retracted.affectedDoorCount == 0U &&
                    retracted.geometryRevision == 1U,
                "platform switch publishes one generic target transition") &&
         expect(disabled.logicOverlay.lines.size() == 27U &&
                    disabled.logicOverlay.targetEdgeCount == 12U &&
                    disabled.logicOverlay.lines[15].color.r > 0.9F &&
                    disabled.logicOverlay.lines[15].color.g > 0.5F &&
                    disabled.logicOverlay.lines[15].color.g < 0.8F,
                "retracted platform remains inspectable as an amber target");
}

bool startStopAndProjectionPreserveAuthoredDocument() {
  cr::CreativeDocument document = playableDocument();
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  const std::uint64_t revision = document.revision();
  const std::size_t objectCount = document.objectCount();

  const app::CreativePlayStartReceipt started =
      start(mode, document, catalog);
  const app::CreativePlayScene projected =
      app::buildCreativePlaySessionScene(mode);
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
      app::stopCreativePlaySession(mode);

  return expect(started.accepted &&
                    started.status == app::CreativePlayStartStatus::Started &&
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
         expect(stopped.stopped && !app::creativePlaySessionActive(mode),
                "stop destroys only the runtime sandbox");
}

bool fixedTickMovementAndCatchUpAreBounded() {
  cr::CreativeDocument document = playableDocument(false, 902U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  const app::CreativePlayStartReceipt started =
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

  app::CreativePlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.input.moveForward = 1.0F;
  tick.monotonicTimeNanoseconds = 1U;
  const app::CreativePlayTickReceipt primed =
      app::tickCreativePlaySession(mode, tick);
  tick.monotonicTimeNanoseconds = 50'000'000U;
  const app::CreativePlayTickReceipt early =
      app::tickCreativePlaySession(mode, tick);
  tick.monotonicTimeNanoseconds = 50'000'001U;
  const app::CreativePlayTickReceipt first =
      app::tickCreativePlaySession(mode, tick);
  const iggy3d::EntityState* afterFirst =
      mode.sandbox->session.state().world.findById({1U});
  const bool movedForward =
      afterFirst != nullptr && afterFirst->transform.position.z < startPosition.z;

  tick.monotonicTimeNanoseconds = 2'050'000'001U;
  const app::CreativePlayTickReceipt catchUp =
      app::tickCreativePlaySession(mode, tick);

  return expect(primed.status == app::CreativePlayTickStatus::ClockPrimed &&
                    primed.ticksAdvanced == 0U,
                "first play frame only primes the fixed-step clock") &&
         expect(early.status == app::CreativePlayTickStatus::NoTickDue &&
                    early.ticksAdvanced == 0U,
                "49,999,999 ns does not advance a 20 Hz tick") &&
         expect(first.status == app::CreativePlayTickStatus::Advanced &&
                    first.ticksAdvanced == 1U &&
                    first.movementCommandsSubmitted == 1U && movedForward,
                "50 ms submits one collision-backed move") &&
         expect(catchUp.status == app::CreativePlayTickStatus::Advanced &&
                    catchUp.ticksAdvanced == mode.tuning.maximumCatchUpTicks,
                "long frame is capped by the catch-up bound") &&
         expect(document.revision() == revision,
                "runtime movement leaves authored revision unchanged");
}

bool idleTicksAdvanceAndStaleDocumentsStop() {
  cr::CreativeDocument document = playableDocument(false, 903U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  if (!start(mode, document, catalog).accepted) {
    return expect(false, "idle setup starts play");
  }

  app::CreativePlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.monotonicTimeNanoseconds = 10U;
  static_cast<void>(app::tickCreativePlaySession(mode, tick));
  tick.monotonicTimeNanoseconds = 50'000'010U;
  const app::CreativePlayTickReceipt idle =
      app::tickCreativePlaySession(mode, tick);

  static_cast<void>(addObject(document, cr::CreativeObjectKind::Prop,
                              "Authored During Play", {1.0, 0.25, 1.0}));
  tick.monotonicTimeNanoseconds = 100'000'010U;
  const app::CreativePlayTickReceipt stale =
      app::tickCreativePlaySession(mode, tick);

  return expect(idle.status == app::CreativePlayTickStatus::Advanced &&
                    idle.commandsSubmitted == 1U &&
                    idle.movementCommandsSubmitted == 0U &&
                    idle.sourceTick > 0U,
                "idle frame submits Wait so runtime systems advance") &&
         expect(stale.status ==
                        app::CreativePlayTickStatus::SourceDocumentChanged &&
                    !stale.active && !app::creativePlaySessionActive(mode),
                "changed source document invalidates and stops sandbox");
}

bool desktopPlayLaunchesWithoutOwningSession() {
  // C3 contract: the Play command validates + snapshots + spawns i3dp; it
  // NEVER starts the embedded session, and the editor stays editable. With
  // an empty save root the snapshot step refuses, so no process is spawned
  // in tests -- the accepted path (plan + snapshot) is pinned separately in
  // creative_playtest_launch_tests via preparePlaytestLaunch.
  cr::CreativeAppState appState;
  cr::CreativeDocument document = playableDocument(false, 904U);
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  iggy3d::StaticMeshAssetCatalog catalog;
  std::string saveId = "unused";
  // The Creative command context has no embedded-session owner. With no save
  // root, Play must refuse before fallback spawning and leave editing live.
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId, &catalog};

  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::Play);
  const app::CreativeDesktopCommandResult played =
      app::dispatchCreativeDesktopCommands(frame, context);
  frame.clear();
  frame.push(app::CreativeDesktopCommandId::NewDocument);
  const app::CreativeDesktopCommandResult newDocument =
      app::dispatchCreativeDesktopCommands(frame, context);

  return expect(!played.accepted &&
                    played.message.starts_with("playtest refused:"),
                "Desktop Play refuses an invalid launch environment") &&
         expect(newDocument.accepted && newDocument.documentReplaced,
                "editing stays live after Play (no lockout engages)");
}

bool invalidMapAndTuningFailClosed() {
  cr::CreativeDocument invalid = cr::CreativeDocument::create("Invalid Play");
  static_cast<void>(invalid.assignId(905U));
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativePlaySession mode;
  const app::CreativePlayStartReceipt missingSpawn =
      start(mode, invalid, catalog);

  cr::CreativeDocument valid = playableDocument(false, 906U);
  app::CreativePlayStartRequest badTuning;
  badTuning.document = &valid;
  badTuning.staticMeshAssetCatalog = &catalog;
  badTuning.tuning.maximumCatchUpTicks = 0U;
  const app::CreativePlayStartReceipt invalidTuning =
      app::startCreativePlaySession(mode, std::move(badTuning));

  return expect(!missingSpawn.accepted &&
                    missingSpawn.status ==
                        app::CreativePlayStartStatus::PreparationRejected &&
                    !app::creativePlaySessionActive(mode),
                "invalid map never creates a runtime sandbox") &&
         expect(!invalidTuning.accepted &&
                    invalidTuning.status ==
                        app::CreativePlayStartStatus::InvalidTuning &&
                    !app::creativePlaySessionActive(mode),
                "invalid fixed-step tuning fails before preparation");
}

bool authoredDoorAndPickupCompleteTheRuntimeInteractionLoop() {
  iggy3d::StaticMeshAssetCatalog catalog;

  cr::CreativeDocument doorDocument = playableDocument(false, 917U);
  static_cast<void>(addObject(
      doorDocument, cr::CreativeObjectKind::Door, "Play Door",
      {0.0, 0.25, -0.8},
      cr::CreativeBounds{{-0.5, 0.25, -0.9}, {0.5, 2.5, -0.7}}));
  const std::uint64_t doorRevision = doorDocument.revision();
  const std::size_t doorObjectCount = doorDocument.objectCount();
  app::CreativePlaySession doorMode;
  if (!start(doorMode, doorDocument, catalog).accepted ||
      !doorMode.sandbox.has_value()) {
    return expect(false, "door interaction setup starts");
  }
  const std::size_t closedMeshCount =
      doorMode.sandbox->room.staticMeshes.size();
  const std::size_t closedColliderCount =
      doorMode.sandbox->collisionSurfaces.size();
  const app::CreativePlayTickReceipt doorPressed = tickAt(
      doorMode, doorDocument, 1U, {false, true});
  const std::string openPrompt = doorMode.target.actionPrompt;
  const app::CreativePlayTargetStatus openTargetStatus =
      doorMode.target.status;
  const app::CreativePlayTickReceipt doorOpened = tickAt(
      doorMode, doorDocument, 50'000'001U, {false, true});
  const bool doorMeshRetained =
      doorMode.sandbox->room.staticMeshes.size() == closedMeshCount;
  const std::size_t openColliderCount =
      doorMode.sandbox->collisionSurfaces.size();
  static_cast<void>(tickAt(doorMode, doorDocument, 100'000'001U, {}));
  const std::string closePrompt = doorMode.target.actionPrompt;
  const app::CreativePlayTickReceipt doorClosed = tickAt(
      doorMode, doorDocument, 150'000'001U, {false, true});

  cr::CreativeDocument pickupDocument = playableDocument(false, 918U);
  static_cast<void>(addObject(pickupDocument, cr::CreativeObjectKind::LootPoint,
                              "Play Key", {0.0, 1.65, -0.4}));
  const std::uint64_t pickupRevision = pickupDocument.revision();
  app::CreativePlaySession pickupMode;
  if (!start(pickupMode, pickupDocument, catalog).accepted ||
      !pickupMode.sandbox.has_value()) {
    return expect(false, "pickup interaction setup starts");
  }
  const app::CreativePlayTickReceipt pickupPressed = tickAt(
      pickupMode, pickupDocument, 1U, {false, true});
  const iggy3d::EntityId pickupEntityId = pickupMode.target.entity;
  const std::string pickupItemId =
      pickupMode.sandbox->interactables.front().definition.itemId;
  const app::CreativePlayTickReceipt pickupExecuted = tickAt(
      pickupMode, pickupDocument, 50'000'001U, {false, true});
  const iggy3d::EntityState* pickupEntity =
      pickupMode.sandbox->session.state().world.findById(pickupEntityId);
  const iggy3d::PlayerInventory* inventory = iggy3d::findInventory(
      pickupMode.sandbox->session.state().inventory, 0U);
  const bool inventoryContainsPickup =
      inventory != nullptr &&
      std::any_of(inventory->stacks.begin(), inventory->stacks.end(),
                  [&pickupItemId](const iggy3d::InventoryStack& stack) {
                    return stack.itemId == pickupItemId && stack.count == 1U;
                  });

  return expect(openPrompt == "OPEN",
                "closed door advertises its semantic open action") &&
         expect(openTargetStatus == app::CreativePlayTargetStatus::Valid,
                std::string("closed door target is actionable, got ") +
                    std::string(app::toString(openTargetStatus))) &&
         expect(doorPressed.actionSubmitted,
                "door press is admitted through the semantic action path") &&
         expect(doorOpened.interactionEffectsApplied == 1U &&
                    doorOpened.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorOpening &&
                    doorOpened.runtimeGeometryRevision == 0U &&
                    doorMeshRetained && openColliderCount == closedColliderCount,
                "executed door interaction starts motion without deleting geometry") &&
         expect(closePrompt == "CLOSE",
                "moving door advertises the reverse action") &&
         expect(doorClosed.actionSubmitted,
                "reverse interaction is admitted while the door moves") &&
         expect(doorClosed.interactionEffectsApplied == 1U,
                "reverse interaction reaches the runtime effect owner") &&
         expect(doorClosed.interactionEffect ==
                    cr::CreativeRuntimeInteractionEffectStatus::DoorClosing,
                "moving door remains targetable for reversal") &&
         expect(doorMode.targetingGeometryRevision == 2U,
                "moving door republishes targeting geometry per fixed tick") &&
         expect(doorMode.sandbox->room.staticMeshes.size() == closedMeshCount &&
                    doorMode.sandbox->collisionSurfaces.size() ==
                        closedColliderCount,
                "moving door preserves grouped geometry cardinality") &&
         expect(doorDocument.revision() == doorRevision &&
                    doorDocument.objectCount() == doorObjectCount,
                "door runtime state never leaks into authored content") &&
         expect(pickupPressed.actionSubmitted &&
                    pickupExecuted.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::PickupAcquired &&
                    pickupExecuted.interactionEffectsApplied == 1U &&
                    pickupEntity != nullptr && !pickupEntity->active &&
                    inventoryContainsPickup,
                "pickup enters inventory and disappears after one interaction") &&
         expect(pickupMode.lastInteractionEffect.displayName == "Play Key" &&
                    pickupDocument.revision() == pickupRevision,
                "pickup feedback is authored-name aware and sandbox isolated");
}

bool persistentAuthoredLootCanBeCollectedOnSeparatePresses() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(false, 924U);

  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::LootPoint;
  request.name = "Arrow Bundle";
  request.transform.position = {0.0, 1.65, -0.4};
  request.hasTransformOverride = true;
  request.hasLootPointSettingsOverride = true;
  request.lootPoint.itemId = "arrow_bundle";
  request.lootPoint.itemCount = 1U;
  request.lootPoint.deactivateOnCollect = false;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  const std::uint64_t authoredRevision = document.revision();

  app::CreativePlaySession mode;
  if (!created.accepted || !start(mode, document, catalog).accepted ||
      !mode.sandbox.has_value()) {
    return expect(false, "persistent pickup setup starts");
  }

  const app::CreativePlayTickReceipt firstPressed =
      tickAt(mode, document, 1U, {false, true});
  const app::CreativePlayTickReceipt firstExecuted =
      tickAt(mode, document, 50'000'001U, {false, true});
  static_cast<void>(tickAt(mode, document, 100'000'001U, {}));
  const app::CreativePlayTickReceipt secondPressed =
      tickAt(mode, document, 150'000'001U, {false, true});
  const app::CreativePlayTickReceipt secondExecuted =
      tickAt(mode, document, 200'000'001U, {false, true});

  const cr::CreativeRuntimeInteractableState* state =
      cr::findCreativeRuntimeInteractableByObjectId(
          *mode.sandbox, created.objectId);
  const iggy3d::EntityState* entity =
      state == nullptr
          ? nullptr
          : mode.sandbox->session.state().world.findById(state->entity);
  const iggy3d::PlayerInventory* inventory = iggy3d::findInventory(
      mode.sandbox->session.state().inventory, 0U);
  const bool collectedTwice =
      inventory != nullptr &&
      std::any_of(
          inventory->stacks.begin(), inventory->stacks.end(),
          [](const iggy3d::InventoryStack& stack) {
            return stack.itemId == "arrow_bundle" && stack.count == 2U;
          });
  return expect(firstPressed.actionSubmitted &&
                    firstExecuted.interactionEffectsApplied == 1U &&
                    firstExecuted.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            PickupAcquired,
                "persistent pickup accepts the first press") &&
         expect(secondPressed.actionSubmitted &&
                    secondPressed.interactionEffectsApplied == 1U &&
                    secondPressed.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            PickupAcquired &&
                    !secondExecuted.actionSubmitted &&
                    secondExecuted.interactionEffectsApplied == 0U,
                "persistent pickup accepts a later distinct press") &&
         expect(state != nullptr && !state->pickupConsumed &&
                    entity != nullptr && entity->active && collectedTwice,
                "persistent pickup remains active and grants each quantity") &&
         expect(document.revision() == authoredRevision,
                "persistent pickup runtime state stays sandbox-only");
}

bool authoredLootAndExitCompleteAPlayableObjectiveLoop() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(false, 923U);

  cr::CreativeDocumentCreateRequest lootRequest;
  lootRequest.kind = cr::CreativeObjectKind::LootPoint;
  lootRequest.name = "Estate Key Ring";
  lootRequest.transform.position = {0.0, 1.65, -0.4};
  lootRequest.hasTransformOverride = true;
  lootRequest.hasLootPointSettingsOverride = true;
  lootRequest.lootPoint.itemId = "estate_key";
  lootRequest.lootPoint.itemCount = 2U;
  const cr::CreativeDocumentCreateReceipt loot =
      document.createObject(lootRequest);

  cr::CreativeDocumentCreateRequest exitRequest;
  exitRequest.kind = cr::CreativeObjectKind::ExitPoint;
  exitRequest.name = "Estate Gate";
  exitRequest.transform.position = {0.0, 1.65, -0.4};
  exitRequest.hasTransformOverride = true;
  exitRequest.hasExitPointSettingsOverride = true;
  exitRequest.exitPoint.requiredItemId = "estate_key";
  exitRequest.exitPoint.requiredItemCount = 2U;
  const cr::CreativeDocumentCreateReceipt exit =
      document.createObject(exitRequest);
  const std::uint64_t authoredRevision = document.revision();

  app::CreativePlaySession mode;
  if (!loot.accepted || !exit.accepted ||
      !start(mode, document, catalog).accepted ||
      !mode.sandbox.has_value()) {
    return expect(false, "authored objective loop starts");
  }
  const auto lootState = std::find_if(
      mode.sandbox->interactables.begin(), mode.sandbox->interactables.end(),
      [&loot](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == loot.objectId;
      });
  const auto exitState = std::find_if(
      mode.sandbox->interactables.begin(), mode.sandbox->interactables.end(),
      [&exit](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == exit.objectId;
      });
  if (lootState == mode.sandbox->interactables.end() ||
      exitState == mode.sandbox->interactables.end()) {
    return expect(false, "authored objective loop has runtime owners");
  }

  const app::CreativePlayTickReceipt pickupPressed =
      tickAt(mode, document, 1U, {false, true});
  const app::CreativePlayTickReceipt pickupExecuted =
      tickAt(mode, document, 50'000'001U, {false, true});
  const app::CreativePlayTickReceipt released =
      tickAt(mode, document, 100'000'001U, {});
  const std::string exitPrompt = mode.target.actionPrompt;
  const app::CreativePlayTargetStatus releasedTargetStatus =
      mode.target.status;
  const std::string releasedTargetStableName = mode.target.stableName;
  const app::CreativePlayTickReceipt exitPressed =
      tickAt(mode, document, 150'000'001U, {false, true});
  const app::CreativePlayTickReceipt exitExecuted =
      tickAt(mode, document, 200'000'001U, {false, true});

  const iggy3d::PlayerInventory* inventory = iggy3d::findInventory(
      mode.sandbox->session.state().inventory, 0U);
  const bool inventoryContainsRequirement =
      inventory != nullptr &&
      std::any_of(
          inventory->stacks.begin(), inventory->stacks.end(),
          [](const iggy3d::InventoryStack& stack) {
            return stack.itemId == "estate_key" && stack.count == 2U;
          });
  const iggy3d::EntityState* lootEntity =
      mode.sandbox->session.state().world.findById(lootState->entity);
  const iggy3d::EntityState* exitEntity =
      mode.sandbox->session.state().world.findById(exitState->entity);
  return expect(lootState->definition.itemCount == 2U &&
                    lootState->definition.itemId == "estate_key" &&
                    exitState->definition.requiredItemId == "estate_key" &&
                    exitState->definition.requiredItemCount == 2U &&
                    exitState->definition.objectiveId ==
                        cr::makeCreativeExitObjectiveId(exit.objectId),
                "authored quantities and deterministic objective id reach Play") &&
         expect(pickupPressed.actionSubmitted &&
                    pickupExecuted.interactionEffectsApplied == 1U &&
                    pickupExecuted.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            PickupAcquired &&
                    inventoryContainsRequirement && lootEntity != nullptr &&
                    !lootEntity->active,
                "one authored pickup interaction grants the full quantity") &&
         expect(released.active,
                "Play remains active after collecting objective loot") &&
         expect(exitPrompt == "EXIT",
                std::string("completed pickup reveals EXIT prompt, got ") +
                    exitPrompt) &&
         expect(releasedTargetStableName ==
                    exitState->definition.stableName,
                "exit target carries a valid stable runtime identity") &&
         expect(exitPressed.actionSubmitted,
                std::string("completed pickup admits an exit interaction, "
                            "target status ") +
                    std::string(app::toString(releasedTargetStatus))) &&
         expect(exitPressed.interactionEffectsApplied == 1U,
                "satisfied exit publishes one interaction effect") &&
         expect(exitPressed.interactionEffect ==
                    cr::CreativeRuntimeInteractionEffectStatus::
                        ObjectiveCompleted,
                std::string("satisfied exit reports objective completion, "
                            "got ") +
                    std::string(cr::toString(
                        exitPressed.interactionEffect))) &&
         expect(exitEntity != nullptr && !exitEntity->active,
                "satisfied exit deactivates its runtime target") &&
         expect(exitState->objectiveCompleted,
                "satisfied exit marks its runtime owner complete") &&
         expect(!exitExecuted.actionSubmitted &&
                    exitExecuted.interactionEffectsApplied == 0U,
                "held exit input cannot complete the objective twice") &&
         expect(mode.sandbox->session.state().outcome ==
                    iggy3d::SessionOutcome::Victory &&
                    mode.lastInteractionEffect.displayName == "Estate Gate",
                "authored exit objective resolves to Victory feedback") &&
         expect(document.revision() == authoredRevision,
                "objective runtime state never mutates authored content");
}

bool movingPlatformAdvancesThroughEditorPlayTick() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(false, 922U);
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::MovingPlatform;
  request.name = "Play Lift";
  request.transform.position = {3.0, 0.375, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{2.0, 0.25, -1.0}, {4.0, 0.5, 1.0}};
  request.hasBoundsOverride = true;
  request.pathPoints = {{{3.0, 0.375, 0.0}}, {{3.0, 1.375, 0.0}}};
  request.hasPathOverride = true;
  request.movingPlatform.speedMetersPerSecond = 1.0;
  request.hasMovingPlatformSettingsOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  const std::uint64_t authoredRevision = document.revision();

  app::CreativePlaySession mode;
  if (!created.accepted || !start(mode, document, catalog).accepted ||
      !mode.sandbox.has_value()) {
    return expect(false, "moving platform Play fixture starts");
  }
  const auto platform = std::find_if(
      mode.sandbox->interactables.begin(), mode.sandbox->interactables.end(),
      [&created](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == created.objectId;
      });
  if (platform == mode.sandbox->interactables.end()) {
    return expect(false, "moving platform Play state exists");
  }
  const float initialY = platform->movingPlatform.positionMeters.y;
  const std::string meshId = platform->definition.roomMeshId;

  const app::CreativePlayTickReceipt primed =
      tickAt(mode, document, 1U);
  const app::CreativePlayTickReceipt advanced =
      tickAt(mode, document, 50'000'001U);
  const app::CreativePlayScene scene =
      app::buildCreativePlaySessionScene(mode);
  const auto movedMesh = std::find_if(
      mode.sandbox->room.staticMeshes.begin(),
      mode.sandbox->room.staticMeshes.end(),
      [&meshId](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.id == meshId;
      });
  const cr::CreativeObject* authored = document.findObject(created.objectId);

  return expect(primed.status ==
                    app::CreativePlayTickStatus::ClockPrimed,
                "moving platform Play clock primes") &&
         expect(advanced.status ==
                        app::CreativePlayTickStatus::Advanced &&
                    advanced.movingPlatformsAdvanced == 1U &&
                    advanced.movingPlatformsBlocked == 0U &&
                    advanced.runtimeGeometryRevision == 1U,
                "editor Play tick advances one moving platform") &&
         expect(platform->movingPlatform.positionMeters.y > initialY &&
                    movedMesh != mode.sandbox->room.staticMeshes.end() &&
                    movedMesh->positionMeters.y > initialY,
                "Play publishes the moved runtime marker and room mesh") &&
         expect(scene.available && mode.targetingGeometryRevision == 1U,
                "moved platform projects and refreshes targeting") &&
         expect(document.revision() == authoredRevision &&
                    authored != nullptr && authored->pathPoints.size() == 2U,
                "runtime movement leaves authored platform unchanged");
}

bool occupiedPlatformRestoreIsHandledInsidePlay() {
  iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativeDocument document = playableDocument(false, 921U);
  const cr::CreativeDocumentCreateReceipt button = createObject(
      document, cr::CreativeObjectKind::Button, "Lift Button",
      {0.0, 1.7, -0.3});
  const cr::CreativeDocumentCreateReceipt platform = createObject(
      document, cr::CreativeObjectKind::Platform, "Spawn Platform",
      {0.0, 0.25, 0.0},
      cr::CreativeBounds{{-1.0, 0.25, -1.0}, {1.0, 0.6, 1.0}});
  if (!button.accepted || !platform.accepted ||
      !document
           .setLogicLink({button.objectId, platform.objectId,
                          cr::CreativeLogicLinkAction::Toggle})
           .accepted) {
    return expect(false, "occupied platform Play fixture is authored");
  }

  app::CreativePlaySession mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "occupied platform Play fixture starts");
  }
  mode.cameraPitchDegrees = -63.435F;
  const auto platformState = std::find_if(
      mode.sandbox->interactables.begin(), mode.sandbox->interactables.end(),
      [&platform](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == platform.objectId;
      });
  if (platformState == mode.sandbox->interactables.end()) {
    return expect(false, "occupied platform state exists");
  }

  const app::CreativePlayTickReceipt firstPress =
      tickAt(mode, document, 1U, {false, true});
  const app::CreativePlayTickReceipt retracted =
      tickAt(mode, document, 50'000'001U, {false, true});
  if (!moveRuntimeEntity(mode, {1U}, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player moves into retracted platform volume");
  }
  mode.cameraPitchDegrees = -39.806F;
  static_cast<void>(tickAt(mode, document, 100'000'001U, {}));
  const app::CreativePlayTickReceipt blocked =
      tickAt(mode, document, 150'000'001U, {false, true});
  const bool stayedRetracted =
      !platformState->targetActive && mode.sandbox->geometryRevision == 1U;
  const app::CreativePlayTickReceipt continued =
      tickAt(mode, document, 200'000'001U, {});

  return expect(firstPress.actionSubmitted &&
                    retracted.status ==
                        app::CreativePlayTickStatus::Advanced &&
                    retracted.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::LinksApplied,
                "first button gesture retracts the platform") &&
         expect(blocked.status ==
                        app::CreativePlayTickStatus::Advanced &&
                    blocked.active && blocked.interactionEffectsApplied == 1U &&
                    blocked.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            TargetOccupied &&
                    mode.lastInteractionEffect.reasonCode ==
                        "creative_runtime_platform_enable_occupied" &&
                    stayedRetracted,
                "occupied restore is consumed without stopping Play") &&
         expect(continued.status ==
                        app::CreativePlayTickStatus::Advanced &&
                    continued.active &&
                    app::creativePlaySessionActive(mode),
                "Play continues after an occupied restore refusal");
}

}  // namespace

int main() {
  const bool ok = runtimeBindingsAndActionEdgesAreDeterministic() &&
                  targetResolverClassifiesRuntimeTruth() &&
                  attackAndInteractSubmitOncePerPress() &&
                  playHudIsBoundedAndUsesTargetState() &&
                  automaticLogicRunsInPlayAndProjectsOccupancy() &&
                  manualAndInvalidLogicLinksRemainInspectableInPlay() &&
                  platformLogicTargetProjectsEnabledAndRetractedState() &&
                  startStopAndProjectionPreserveAuthoredDocument() &&
                  fixedTickMovementAndCatchUpAreBounded() &&
                  idleTicksAdvanceAndStaleDocumentsStop() &&
                  desktopPlayLaunchesWithoutOwningSession() &&
                  invalidMapAndTuningFailClosed() &&
                  authoredDoorAndPickupCompleteTheRuntimeInteractionLoop() &&
                  persistentAuthoredLootCanBeCollectedOnSeparatePresses() &&
                  authoredLootAndExitCompleteAPlayableObjectiveLoop() &&
                  movingPlatformAdvancesThroughEditorPlayTick() &&
                  occupiedPlatformRestoreIsHandledInsidePlay();
  if (!ok) {
    return 1;
  }
  std::cout << "creative_editor_play_mode_tests: PASS\n";
  return 0;
}
