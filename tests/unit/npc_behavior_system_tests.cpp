#include "runtime/ai/NpcBehaviorSystem.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/world/WorldState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::EntityState entity(iggy3d::EntityId id,
                           std::string_view stableName,
                           iggy3d::EntityKind kind,
                           float x,
                           float z,
                           bool active = true) {
  iggy3d::EntityState entity;
  entity.id = id;
  entity.stableName = std::string(stableName);
  entity.kind = kind;
  entity.transform = transformAt(x, 0.0F, z);
  entity.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                         {0.25F, 1.2F, 0.25F});
  entity.active = active;
  entity.persistent = true;
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Attack,
                              iggy3d::TargetAction::Inspect};
  return entity;
}

iggy3d::WorldState worldWithNpcAndPlayer(float targetX = 3.0F,
                                         bool actorActive = true,
                                         bool targetActive = true) {
  iggy3d::WorldState world;
  (void)world.seedEntity(entity({1},
                                "npc",
                                iggy3d::EntityKind::Npc,
                                0.0F,
                                0.0F,
                                actorActive));
  (void)world.seedEntity(entity({2},
                                "player",
                                iggy3d::EntityKind::Player,
                                targetX,
                                0.0F,
                                targetActive));
  return world;
}

iggy3d::WorldState worldWithNpcAtAndPlayer(float actorX,
                                           float targetX,
                                           bool actorActive = true,
                                           bool targetActive = true) {
  iggy3d::WorldState world;
  (void)world.seedEntity(entity({1},
                                "npc",
                                iggy3d::EntityKind::Npc,
                                actorX,
                                0.0F,
                                actorActive));
  (void)world.seedEntity(entity({2},
                                "player",
                                iggy3d::EntityKind::Player,
                                targetX,
                                0.0F,
                                targetActive));
  return world;
}

iggy3d::CombatState combat(bool actorDefeated = false, bool targetDefeated = false) {
  iggy3d::CombatState combat;
  combat.combatants.push_back({{1}, 2, actorDefeated ? 0 : 3, 3, actorDefeated});
  combat.combatants.push_back({{2}, 1, targetDefeated ? 0 : 10, 10, targetDefeated});
  return combat;
}

iggy3d::AiActorState actorState() {
  iggy3d::AiActorState state;
  state.actor = {1};
  state.target = {2};
  state.enabled = true;
  return state;
}

iggy3d::AiActorState guardedActorState(iggy3d::Vec3 homePosition = {0.0F, 0.0F, 0.0F},
                                       float leashRadiusMeters = 6.0F,
                                       float returnRadiusMeters = 1.0F,
                                       float homeToleranceMeters = 0.25F) {
  iggy3d::AiActorState state = actorState();
  state.hasHomePosition = true;
  state.homePosition = homePosition;
  state.homeStableName = "guard_post_alpha";
  state.leashRadiusMeters = leashRadiusMeters;
  state.returnRadiusMeters = returnRadiusMeters;
  state.homeToleranceMeters = homeToleranceMeters;
  return state;
}

iggy3d::NpcPerceptionResult perceptionFor(const iggy3d::WorldState& world,
                                          const iggy3d::CombatState& combat,
                                          iggy3d::NpcBehaviorConfig config = {}) {
  return iggy3d::queryNpcPerception({&world, &combat, {1}, {2}, config});
}

bool configValidationRejectsBadValues() {
  iggy3d::NpcBehaviorConfig config;
  bool ok = expect(iggy3d::isValidNpcBehaviorConfig(config), "default config valid") &&
            expect(config.engagementPolicy == iggy3d::NpcEngagementPolicy::Hostile,
                   "default engagement hostile");

  config.engagementPolicy = static_cast<iggy3d::NpcEngagementPolicy>(255);
  ok = ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                    "invalid engagement policy rejected");
  config = {};

  config.perceptionRadiusMeters = 0.0F;
  ok = ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                    "zero perception rejected");
  config = {};
  config.chaseStopDistanceMeters = -1.0F;
  ok = ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                    "negative stop rejected");
  config = {};
  config.attackRangeMeters = std::numeric_limits<float>::infinity();
  ok = ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                    "infinite attack range rejected");
  config = {};
  config.chaseStepMeters = 0.0F;
  ok = ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                    "zero chase step rejected");
  config = {};
  config.attackDamage = 0;
  ok = ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                    "zero damage rejected");
  config = {};
  config.decisionIntervalTicks = 0;
  return ok && expect(!iggy3d::isValidNpcBehaviorConfig(config),
                      "zero decision interval rejected");
}

bool perceptionReportsDeterministicFailures() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer();
  const iggy3d::CombatState readyCombat = combat();
  bool ok = expect(iggy3d::queryNpcPerception({nullptr, &readyCombat, {1}, {2}, {}})
                       .status == iggy3d::NpcPerceptionStatus::InvalidWorld,
                   "invalid world") &&
            expect(iggy3d::queryNpcPerception({&world, nullptr, {1}, {2}, {}})
                       .status == iggy3d::NpcPerceptionStatus::InvalidCombat,
                   "invalid combat") &&
            expect(iggy3d::queryNpcPerception({&world, &readyCombat, {99}, {2}, {}})
                       .status == iggy3d::NpcPerceptionStatus::InvalidActor,
                   "missing actor") &&
            expect(iggy3d::queryNpcPerception({&world, &readyCombat, {1}, {99}, {}})
                       .status == iggy3d::NpcPerceptionStatus::InvalidTarget,
                   "missing target");

  const iggy3d::WorldState inactiveActor = worldWithNpcAndPlayer(3.0F, false, true);
  const iggy3d::WorldState inactiveTarget = worldWithNpcAndPlayer(3.0F, true, false);
  ok = ok && expect(perceptionFor(inactiveActor, readyCombat).status ==
                        iggy3d::NpcPerceptionStatus::ActorInactive,
                    "inactive actor") &&
       expect(perceptionFor(inactiveTarget, readyCombat).status ==
                  iggy3d::NpcPerceptionStatus::TargetInactive,
              "inactive target");

  return ok;
}

bool defeatedAndOutOfRangePerceptionStatuses() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer();
  bool ok = expect(perceptionFor(world, combat(true, false)).status ==
                       iggy3d::NpcPerceptionStatus::ActorDefeated,
                   "actor defeated") &&
            expect(perceptionFor(world, combat(false, true)).status ==
                       iggy3d::NpcPerceptionStatus::TargetDefeated,
                   "target defeated");

  const iggy3d::WorldState farWorld = worldWithNpcAndPlayer(8.0F);
  const iggy3d::NpcPerceptionResult far = perceptionFor(farWorld, combat());
  return ok && expect(far.status == iggy3d::NpcPerceptionStatus::TargetOutOfRange,
                      "target out of range") &&
         expect(!far.targetInPerceptionRadius, "perception radius false");
}

bool chaseDecisionAndCommandAreDeterministic() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer(4.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);
  const iggy3d::AiActorState state = actorState();
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 10});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(perception.status == iggy3d::NpcPerceptionStatus::Ready,
                "chase perception ready") &&
         expect(!perception.targetInAttackRange, "outside attack range") &&
         expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                "chase decided") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Chasing,
                "chase behavior") &&
         expect(decision.intent == iggy3d::AiIntentKind::MoveTowardTarget,
                "chase intent") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand,
                "chase command built") &&
         expect(command.command.kind == iggy3d::CommandKind::Move,
                "chase move kind") &&
         expect(command.command.source == iggy3d::CommandSource::Ai,
                "chase source ai") &&
         expect(command.command.actor == iggy3d::EntityId{1}, "chase actor") &&
         expect(command.command.payload.target.hasPoint, "chase point") &&
         expect(near(command.command.payload.target.point.x, 1.0F),
                "chase step x") &&
         expect(near(command.command.payload.target.point.y, 0.0F),
                "chase step y") &&
         expect(near(command.command.payload.target.point.z, 0.0F),
                "chase horizontal z");
}

bool attackDecisionAndCommandAreDeterministic() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer(1.0F);
  const iggy3d::CombatState readyCombat = combat();
  iggy3d::NpcBehaviorConfig config;
  config.attackDamage = 4;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat, config);
  const iggy3d::AiActorState state = actorState();
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 12});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(perception.status == iggy3d::NpcPerceptionStatus::Ready,
                "attack perception ready") &&
         expect(perception.targetInAttackRange, "inside attack range") &&
         expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                "attack decided") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Attacking,
                "attack behavior") &&
         expect(decision.intent == iggy3d::AiIntentKind::AttackTarget,
                "attack intent") &&
         expect(decision.cooldownTicksRemaining == config.attackCooldownTicks,
                "attack cooldown reset") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand,
                "attack command built") &&
         expect(command.command.kind == iggy3d::CommandKind::Attack,
                "attack kind") &&
         expect(command.command.source == iggy3d::CommandSource::Ai,
                "attack source") &&
         expect(command.command.actor == iggy3d::EntityId{1}, "attack actor") &&
         expect(command.command.payload.target.hasEntity &&
                    command.command.payload.target.entity == iggy3d::EntityId{2},
                "attack target") &&
         expect(command.command.payload.attackDamage == 4, "attack damage");
}

bool cooldownDisabledAndNoTargetPoliciesAreDeterministic() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer(1.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);

  iggy3d::AiActorState state = actorState();
  state.cooldownTicksRemaining = 2;
  iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 4});
  bool ok = expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::OnCooldown,
                   "cooldown status") &&
            expect(decision.behavior == iggy3d::AiBehaviorKind::Alert,
                   "cooldown behavior") &&
            expect(decision.intent == iggy3d::AiIntentKind::Wait,
                   "cooldown wait") &&
            expect(decision.cooldownTicksRemaining == 1U,
                   "cooldown decrements");

  state = actorState();
  state.enabled = false;
  decision = iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 4});
  const iggy3d::NpcBehaviorCommandResult disabledCommand =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});
  ok = ok && expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Disabled,
                    "disabled status") &&
       expect(decision.intent == iggy3d::AiIntentKind::None, "disabled no intent") &&
       expect(disabledCommand.status == iggy3d::NpcBehaviorCommandStatus::NoCommand &&
                  !disabledCommand.hasCommand,
              "disabled no command");

  const iggy3d::WorldState farWorld = worldWithNpcAndPlayer(8.0F);
  const iggy3d::NpcPerceptionResult far = perceptionFor(farWorld, readyCombat);
  state = actorState();
  decision = iggy3d::chooseNpcBehaviorIntent({&state, far, config, 4});
  return ok && expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::NoTarget,
                      "no target status") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Idle,
                "no target idle") &&
         expect(decision.intent == iggy3d::AiIntentKind::Wait,
                "no target wait") &&
         expect(!iggy3d::isValid(decision.target), "no target clears target");
}

bool passivePolicyWaitsWithoutChasingOrAttacking() {
  iggy3d::NpcBehaviorConfig config;
  config.engagementPolicy = iggy3d::NpcEngagementPolicy::Passive;

  const iggy3d::WorldState closeWorld = worldWithNpcAndPlayer(1.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcPerceptionResult closePerception =
      perceptionFor(closeWorld, readyCombat, config);
  iggy3d::AiActorState state = actorState();
  iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, closePerception, config, 7});
  bool ok = expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                   "passive close decided") &&
            expect(decision.behavior == iggy3d::AiBehaviorKind::Alert,
                   "passive close alert") &&
            expect(decision.intent == iggy3d::AiIntentKind::Wait,
                   "passive close wait") &&
            expect(decision.target == iggy3d::EntityId{2}, "passive close target") &&
            expect(decision.nextDecisionTick == 8U,
                   "passive close next decision tick");

  const iggy3d::WorldState chaseWorld = worldWithNpcAndPlayer(4.0F);
  const iggy3d::NpcPerceptionResult chasePerception =
      perceptionFor(chaseWorld, readyCombat, config);
  state = actorState();
  decision = iggy3d::chooseNpcBehaviorIntent({&state, chasePerception, config, 9});
  return ok &&
         expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                "passive far decided") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Alert,
                "passive far alert") &&
         expect(decision.intent == iggy3d::AiIntentKind::Wait,
                "passive far wait") &&
         expect(decision.intent != iggy3d::AiIntentKind::MoveTowardTarget,
                "passive never chases") &&
         expect(decision.intent != iggy3d::AiIntentKind::AttackTarget,
                "passive never attacks");
}

bool guardedHostileInsideLeashCanAttack() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer(1.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);
  const iggy3d::AiActorState state = guardedActorState();
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 20});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                "guard attack decided") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Attacking,
                "guard attack behavior") &&
         expect(decision.intent == iggy3d::AiIntentKind::AttackTarget,
                "guard attack intent") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand &&
                    command.command.kind == iggy3d::CommandKind::Attack,
                "guard attack command");
}

bool guardedHostileChasesInsideLeash() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer(4.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);
  const iggy3d::AiActorState state = guardedActorState();
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 21});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                "guard chase decided") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Chasing,
                "guard chase behavior") &&
         expect(decision.intent == iggy3d::AiIntentKind::MoveTowardTarget,
                "guard chase intent") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand &&
                    command.command.kind == iggy3d::CommandKind::Move,
                "guard chase command") &&
         expect(near(command.command.payload.target.point.x, 1.0F),
                "guard chase step");
}

bool guardedActorOutsideLeashReturnsHome() {
  const iggy3d::WorldState world = worldWithNpcAtAndPlayer(8.0F, 8.5F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);
  const iggy3d::AiActorState state = guardedActorState();
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 22});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::Decided,
                "guard outside decided") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Returning,
                "guard outside returning") &&
         expect(decision.intent == iggy3d::AiIntentKind::ReturnToAnchor,
                "guard outside return intent") &&
         expect(iggy3d::nearlyEqual(decision.homePosition, {0.0F, 0.0F, 0.0F}),
                "guard outside home copied") &&
         expect(near(decision.returnStopDistanceMeters, 1.0F),
                "guard outside stop distance") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand &&
                    command.command.kind == iggy3d::CommandKind::Move,
                "guard outside move command") &&
         expect(near(command.command.payload.target.point.x, 7.0F),
                "guard outside return step");
}

bool guardedTargetOutsideLeashPoliciesAreDeterministic() {
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;

  const iggy3d::WorldState awayWorld = worldWithNpcAtAndPlayer(2.0F, 4.0F);
  const iggy3d::NpcPerceptionResult awayPerception =
      perceptionFor(awayWorld, readyCombat);
  iggy3d::AiActorState state =
      guardedActorState({0.0F, 0.0F, 0.0F}, 3.0F, 1.0F, 0.25F);
  iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, awayPerception, config, 23});
  bool ok = expect(decision.behavior == iggy3d::AiBehaviorKind::Returning,
                   "target outside away returns") &&
            expect(decision.intent == iggy3d::AiIntentKind::ReturnToAnchor,
                   "target outside away return intent");

  const iggy3d::WorldState homeWorld = worldWithNpcAtAndPlayer(0.5F, 4.0F);
  const iggy3d::NpcPerceptionResult homePerception =
      perceptionFor(homeWorld, readyCombat);
  state = guardedActorState({0.0F, 0.0F, 0.0F}, 3.0F, 1.0F, 0.25F);
  decision = iggy3d::chooseNpcBehaviorIntent({&state, homePerception, config, 24});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, homePerception, config});
  return ok && expect(decision.behavior == iggy3d::AiBehaviorKind::Alert,
                      "target outside home alert") &&
         expect(decision.intent == iggy3d::AiIntentKind::Wait,
                "target outside home wait") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand &&
                    command.command.kind == iggy3d::CommandKind::Wait,
                "target outside home wait command");
}

bool guardedChaseDestinationBeyondLeashReturns() {
  iggy3d::NpcBehaviorConfig config;
  config.attackRangeMeters = 0.05F;
  config.chaseStopDistanceMeters = 0.01F;
  config.chaseStepMeters = 0.1F;

  const iggy3d::WorldState world = worldWithNpcAtAndPlayer(3.2F, 3.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat, config);
  const iggy3d::AiActorState state =
      guardedActorState({0.0F, 0.0F, 0.0F}, 3.0F, 1.0F, 0.5F);
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 25});

  return expect(perception.status == iggy3d::NpcPerceptionStatus::Ready,
                "leash edge perception ready") &&
         expect(decision.behavior == iggy3d::AiBehaviorKind::Returning,
                "leash edge returning") &&
         expect(decision.intent == iggy3d::AiIntentKind::ReturnToAnchor,
                "leash edge return intent");
}

bool guardedReturnCompletionWaitsWithoutMove() {
  const iggy3d::WorldState world = worldWithNpcAtAndPlayer(0.5F, 4.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);
  const iggy3d::AiActorState state =
      guardedActorState({0.0F, 0.0F, 0.0F}, 3.0F, 1.0F, 0.25F);
  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 26});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(decision.behavior == iggy3d::AiBehaviorKind::Alert,
                "return completion alert") &&
         expect(decision.intent == iggy3d::AiIntentKind::Wait,
                "return completion wait") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::Built &&
                    command.hasCommand &&
                    command.command.kind == iggy3d::CommandKind::Wait,
                "return completion no move");
}

bool guardedPassivePolicyReturnsOnlyWhenOutsideHomeArea() {
  iggy3d::NpcBehaviorConfig config;
  config.engagementPolicy = iggy3d::NpcEngagementPolicy::Passive;
  const iggy3d::CombatState readyCombat = combat();

  const iggy3d::WorldState insideWorld = worldWithNpcAndPlayer(4.0F);
  const iggy3d::NpcPerceptionResult insidePerception =
      perceptionFor(insideWorld, readyCombat, config);
  iggy3d::AiActorState state = guardedActorState();
  iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, insidePerception, config, 27});
  bool ok = expect(decision.behavior == iggy3d::AiBehaviorKind::Alert,
                   "guard passive inside alert") &&
            expect(decision.intent == iggy3d::AiIntentKind::Wait,
                   "guard passive inside wait");

  const iggy3d::WorldState outsideWorld = worldWithNpcAtAndPlayer(8.0F, 8.5F);
  const iggy3d::NpcPerceptionResult outsidePerception =
      perceptionFor(outsideWorld, readyCombat, config);
  state = guardedActorState();
  decision = iggy3d::chooseNpcBehaviorIntent({&state, outsidePerception, config, 28});
  return ok && expect(decision.behavior == iggy3d::AiBehaviorKind::Returning,
                      "guard passive outside returning") &&
         expect(decision.intent == iggy3d::AiIntentKind::ReturnToAnchor,
                "guard passive outside return intent");
}

bool invalidGuardStateFailsClosed() {
  const iggy3d::WorldState world = worldWithNpcAndPlayer(1.0F);
  const iggy3d::CombatState readyCombat = combat();
  const iggy3d::NpcBehaviorConfig config;
  const iggy3d::NpcPerceptionResult perception = perceptionFor(world, readyCombat);
  iggy3d::AiActorState state = guardedActorState();
  state.returnRadiusMeters = 7.0F;

  const iggy3d::NpcBehaviorDecision decision =
      iggy3d::chooseNpcBehaviorIntent({&state, perception, config, 29});
  const iggy3d::NpcBehaviorCommandResult command =
      iggy3d::buildNpcBehaviorCommand({decision, perception, config});

  return expect(decision.status == iggy3d::NpcBehaviorDecisionStatus::InvalidGuard,
                "invalid guard status") &&
         expect(decision.intent == iggy3d::AiIntentKind::None,
                "invalid guard no intent") &&
         expect(command.status == iggy3d::NpcBehaviorCommandStatus::NoCommand &&
                    !command.hasCommand,
                "invalid guard no command");
}

bool stableStatusNamesAreLowerSnake() {
  return expect(iggy3d::npcEngagementPolicyName(
                    iggy3d::NpcEngagementPolicy::Hostile) == "hostile",
                "engagement hostile lower snake") &&
         expect(iggy3d::npcEngagementPolicyName(
                    iggy3d::NpcEngagementPolicy::Passive) == "passive",
                "engagement passive lower snake") &&
         expect(iggy3d::npcPerceptionStatusName(
                    iggy3d::NpcPerceptionStatus::TargetOutOfRange) ==
                    "target_out_of_range",
                "perception lower snake") &&
         expect(iggy3d::npcBehaviorDecisionStatusName(
                    iggy3d::NpcBehaviorDecisionStatus::InvalidGuard) ==
                    "invalid_guard",
                "decision invalid guard lower snake") &&
         expect(iggy3d::npcBehaviorDecisionStatusName(
                    iggy3d::NpcBehaviorDecisionStatus::WaitingForDecisionTick) ==
                    "waiting_for_decision_tick",
                "decision lower snake") &&
         expect(iggy3d::npcBehaviorCommandStatusName(
                    iggy3d::NpcBehaviorCommandStatus::InvalidDestination) ==
                    "invalid_destination",
                "command lower snake");
}

}  // namespace

int main() {
  const bool ok = configValidationRejectsBadValues() &&
                  perceptionReportsDeterministicFailures() &&
                  defeatedAndOutOfRangePerceptionStatuses() &&
                  chaseDecisionAndCommandAreDeterministic() &&
                  attackDecisionAndCommandAreDeterministic() &&
                  cooldownDisabledAndNoTargetPoliciesAreDeterministic() &&
                  passivePolicyWaitsWithoutChasingOrAttacking() &&
                  guardedHostileInsideLeashCanAttack() &&
                  guardedHostileChasesInsideLeash() &&
                  guardedActorOutsideLeashReturnsHome() &&
                  guardedTargetOutsideLeashPoliciesAreDeterministic() &&
                  guardedChaseDestinationBeyondLeashReturns() &&
                  guardedReturnCompletionWaitsWithoutMove() &&
                  guardedPassivePolicyReturnsOnlyWhenOutsideHomeArea() &&
                  invalidGuardStateFailsClosed() &&
                  stableStatusNamesAreLowerSnake();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
