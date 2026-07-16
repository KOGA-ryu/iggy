#include "EditorDesktopCommands.hpp"
#include "EditorPlayMode.hpp"
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

bool moveRuntimeEntity(app::CreativeEditorPlayMode& mode,
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
  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "play automatic logic setup starts");
  }
  const app::CreativeEditorPlayScene inactiveScene =
      app::buildCreativeEditorPlayScene(mode);
  const app::CreativeEditorPlayScene selectedInactiveScene =
      app::buildCreativeEditorPlayScene(mode, trigger.objectId);
  static_cast<void>(tickAt(mode, document, 1U));
  const app::CreativeEditorPlayTickReceipt entered =
      tickAt(mode, document, 50'000'001U);
  const app::CreativeEditorPlayScene activeScene =
      app::buildCreativeEditorPlayScene(mode);
  const app::CreativeEditorPlayScene selectedActiveScene =
      app::buildCreativeEditorPlayScene(mode, trigger.objectId);

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
         expect(entered.status == app::CreativeEditorPlayTickStatus::Advanced &&
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
  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "manual logic overlay setup starts");
  }
  const app::CreativeEditorPlayScene valid =
      app::buildCreativeEditorPlayScene(mode, source.objectId);
  if (mode.sandbox->logicLinks.empty()) {
    return expect(false, "manual logic overlay has runtime link");
  }
  mode.sandbox->logicLinks.front().targetObjectId = cr::kInvalidObjectId;
  const app::CreativeEditorPlayScene invalid =
      app::buildCreativeEditorPlayScene(mode, source.objectId);

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
  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "platform overlay setup starts");
  }
  const cr::CreativeRuntimeInteractableState* runtimeSource =
      cr::findCreativeRuntimeInteractableByObjectId(*mode.sandbox,
                                                    source.objectId);
  if (runtimeSource == nullptr) {
    return expect(false, "platform overlay source exists at runtime");
  }
  const app::CreativeEditorPlayScene enabled =
      app::buildCreativeEditorPlayScene(mode, source.objectId);
  const cr::CreativeRuntimeInteractionEffectReceipt retracted =
      cr::applyCreativeRuntimeInteractionEffect(*mode.sandbox,
                                                runtimeSource->entity);
  const app::CreativeEditorPlayScene disabled =
      app::buildCreativeEditorPlayScene(mode, source.objectId);

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
  frame.push(app::CreativeDesktopCommandId::SetLogicLink,
             app::CreativeDesktopLogicLinkPayload{
                 1U, 2U, cr::CreativeLogicLinkAction::Toggle});
  const app::CreativeDesktopCommandResult logicBlocked =
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
         expect(!logicBlocked.accepted &&
                    logicBlocked.message == "stop play before editing" &&
                    appState.facade.document().revision() == revision,
                "Inspector logic edits are read-only during Play") &&
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

bool authoredDoorAndPickupCompleteTheRuntimeInteractionLoop() {
  iggy3d::StaticMeshAssetCatalog catalog;

  cr::CreativeDocument doorDocument = playableDocument(false, 917U);
  static_cast<void>(addObject(
      doorDocument, cr::CreativeObjectKind::Door, "Play Door",
      {0.0, 0.25, -0.8},
      cr::CreativeBounds{{-0.5, 0.25, -0.9}, {0.5, 2.5, -0.7}}));
  const std::uint64_t doorRevision = doorDocument.revision();
  const std::size_t doorObjectCount = doorDocument.objectCount();
  app::CreativeEditorPlayMode doorMode;
  if (!start(doorMode, doorDocument, catalog).accepted ||
      !doorMode.sandbox.has_value()) {
    return expect(false, "door interaction setup starts");
  }
  const std::size_t closedMeshCount =
      doorMode.sandbox->room.staticMeshes.size();
  const std::size_t closedColliderCount =
      doorMode.sandbox->collisionSurfaces.size();
  const app::CreativeEditorPlayTickReceipt doorPressed = tickAt(
      doorMode, doorDocument, 1U, {false, true});
  const std::string openPrompt = doorMode.target.actionPrompt;
  const app::CreativeEditorPlayTickReceipt doorOpened = tickAt(
      doorMode, doorDocument, 50'000'001U, {false, true});
  const bool openMeshRemoved =
      doorMode.sandbox->room.staticMeshes.size() + 1U == closedMeshCount;
  const std::size_t openColliderCount =
      doorMode.sandbox->collisionSurfaces.size();
  static_cast<void>(tickAt(doorMode, doorDocument, 100'000'001U, {}));
  const std::string closePrompt = doorMode.target.actionPrompt;
  const app::CreativeEditorPlayTickReceipt doorClosed = tickAt(
      doorMode, doorDocument, 150'000'001U, {false, true});

  cr::CreativeDocument pickupDocument = playableDocument(false, 918U);
  static_cast<void>(addObject(pickupDocument, cr::CreativeObjectKind::LootPoint,
                              "Play Key", {0.0, 1.65, -0.4}));
  const std::uint64_t pickupRevision = pickupDocument.revision();
  app::CreativeEditorPlayMode pickupMode;
  if (!start(pickupMode, pickupDocument, catalog).accepted ||
      !pickupMode.sandbox.has_value()) {
    return expect(false, "pickup interaction setup starts");
  }
  const app::CreativeEditorPlayTickReceipt pickupPressed = tickAt(
      pickupMode, pickupDocument, 1U, {false, true});
  const iggy3d::EntityId pickupEntityId = pickupMode.target.entity;
  const std::string pickupItemId =
      pickupMode.sandbox->interactables.front().definition.itemId;
  const app::CreativeEditorPlayTickReceipt pickupExecuted = tickAt(
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

  return expect(doorPressed.actionSubmitted && openPrompt == "OPEN",
                "door press is admitted through the semantic action path") &&
         expect(doorOpened.interactionEffectsApplied == 1U &&
                    doorOpened.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorOpened &&
                    doorOpened.runtimeGeometryRevision == 1U &&
                    openMeshRemoved && openColliderCount < closedColliderCount,
                "executed door interaction refreshes scene and collision") &&
         expect(closePrompt == "CLOSE" &&
                    doorClosed.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorClosed &&
                    doorMode.targetingGeometryRevision == 2U &&
                    doorMode.sandbox->room.staticMeshes.size() ==
                        closedMeshCount &&
                    doorMode.sandbox->collisionSurfaces.size() ==
                        closedColliderCount,
                "open door stays targetable and closes with restored targeting") &&
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

  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted || !mode.sandbox.has_value()) {
    return expect(false, "occupied platform Play fixture starts");
  }
  const auto platformState = std::find_if(
      mode.sandbox->interactables.begin(), mode.sandbox->interactables.end(),
      [&platform](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == platform.objectId;
      });
  if (platformState == mode.sandbox->interactables.end()) {
    return expect(false, "occupied platform state exists");
  }

  const app::CreativeEditorPlayTickReceipt firstPress =
      tickAt(mode, document, 1U, {false, true});
  const app::CreativeEditorPlayTickReceipt retracted =
      tickAt(mode, document, 50'000'001U, {false, true});
  if (!moveRuntimeEntity(mode, {1U}, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player moves into retracted platform volume");
  }
  static_cast<void>(tickAt(mode, document, 100'000'001U, {}));
  const app::CreativeEditorPlayTickReceipt blocked =
      tickAt(mode, document, 150'000'001U, {false, true});
  const bool stayedRetracted =
      !platformState->targetActive && mode.sandbox->geometryRevision == 1U;
  const app::CreativeEditorPlayTickReceipt continued =
      tickAt(mode, document, 200'000'001U, {});

  return expect(firstPress.actionSubmitted &&
                    retracted.status ==
                        app::CreativeEditorPlayTickStatus::Advanced &&
                    retracted.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::LinksApplied,
                "first button gesture retracts the platform") &&
         expect(blocked.status ==
                        app::CreativeEditorPlayTickStatus::Advanced &&
                    blocked.active && blocked.interactionEffectsApplied == 1U &&
                    blocked.interactionEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            TargetOccupied &&
                    mode.lastInteractionEffect.reasonCode ==
                        "creative_runtime_platform_enable_occupied" &&
                    stayedRetracted,
                "occupied restore is consumed without stopping Play") &&
         expect(continued.status ==
                        app::CreativeEditorPlayTickStatus::Advanced &&
                    continued.active &&
                    app::creativeEditorPlayModeActive(mode),
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
                  desktopPlayTogglesAndBlocksEditing() &&
                  invalidMapAndTuningFailClosed() &&
                  authoredDoorAndPickupCompleteTheRuntimeInteractionLoop() &&
                  occupiedPlatformRestoreIsHandledInsidePlay();
  if (!ok) {
    return 1;
  }
  std::cout << "creative_editor_play_mode_tests: PASS\n";
  return 0;
}
