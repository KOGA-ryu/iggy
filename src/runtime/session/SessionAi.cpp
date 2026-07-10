#include "runtime/session/SessionInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/ai/GuardDecision.hpp"
#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/NpcBehaviorSystem.hpp"
#include "runtime/ai/NpcInvestigateSystem.hpp"
#include "runtime/ai/NpcPatrolSystem.hpp"
#include "runtime/ai/NpcPersonalityWeights.hpp"
#include "runtime/ai/NpcSoundPerception.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/ai/ReasoningRoute.hpp"
#include "runtime/ai/SegmentOcclusion.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace iggy3d {

namespace session_detail {

// branch-gate-relocation: BG-1240 from=src/runtime/session/Session.cpp
namespace {

bool isActiveNpc(const EntityState& entity) {
  return entity.active && entity.kind == EntityKind::Npc;
}

AiActorState* findAiActorState(AiState& ai, EntityId actor) {
  for (AiActorState& actorState : ai.actors) {
    if (actorState.actor == actor) {
      return &actorState;
    }
  }
  return nullptr;
}

void ensureAiActorsForActiveNpcs(SessionState& state) {
  std::vector<EntityId> activeNpcs;
  for (const EntityState& entity : state.world.entities()) {
    if (isActiveNpc(entity)) {
      activeNpcs.push_back(entity.id);
    }
  }
  std::sort(activeNpcs.begin(), activeNpcs.end());
  for (EntityId actor : activeNpcs) {
    if (findAiActorState(state.ai, actor) == nullptr) {
      AiActorState actorState;
      actorState.actor = actor;
      if (const EntityState* actorEntity = state.world.findById(actor)) {
        actorState.facingDirection =
            initialNpcFacing(state.world, actorEntity->transform.position);
      }
      state.ai.actors.push_back(actorState);
    }
  }
}

bool shouldBuildCommandForDecision(const NpcBehaviorDecision& decision) {
  return decision.status == NpcBehaviorDecisionStatus::Decided ||
         decision.status == NpcBehaviorDecisionStatus::OnCooldown;
}

// Local clamp to [0,1] matching NpcAlertSystem.cpp's private clamp01 (NaN -> 0).
float clamp01(float value) {
  if (!(value > 0.0F)) {
    return 0.0F;
  }
  if (value > 1.0F) {
    return 1.0F;
  }
  return value;
}

// A resolved, alive target entity exists (regardless of whether it is currently
// perceived). This is the honest source for the FSM's no-target combat cap: the
// four "target known but not visible right now" statuses still have a live
// target, while defeat/inactive/invalid statuses do not.
bool perceptionHasLiveTarget(NpcPerceptionStatus status) {
  switch (status) {
    case NpcPerceptionStatus::Ready:
    case NpcPerceptionStatus::TargetOutOfRange:
    case NpcPerceptionStatus::TargetOutOfCone:
    case NpcPerceptionStatus::TargetOccluded:
      return true;
    case NpcPerceptionStatus::InvalidWorld:
    case NpcPerceptionStatus::InvalidCombat:
    case NpcPerceptionStatus::InvalidActor:
    case NpcPerceptionStatus::InvalidTarget:
    case NpcPerceptionStatus::InvalidConfig:
    case NpcPerceptionStatus::ActorInactive:
    case NpcPerceptionStatus::TargetInactive:
    case NpcPerceptionStatus::ActorDefeated:
    case NpcPerceptionStatus::TargetDefeated:
      return false;
  }
  return false;
}

// Overlay the graded-alert band onto the fully-alert decision: only the hostile
// combat outcomes (Chasing/Attacking, or OnCooldown) are gated. Below the combat
// band they are downgraded to the alert rung's behavior with a passive Wait
// intent (like a non-engaged NPC). Everything else — leash/return, passive Alert,
// no-target Idle, defeat, disabled, waiting, invalid — passes through unchanged so
// those authoritative outcomes always win over the alert overlay.
NpcBehaviorDecision reconcileAlertBand(const NpcBehaviorDecision& engaged,
                                       const AiActorState& actor,
                                       const AlertProfile& profile) {
  const bool hostileCombat =
      (engaged.status == NpcBehaviorDecisionStatus::Decided &&
       (engaged.behavior == AiBehaviorKind::Chasing ||
        engaged.behavior == AiBehaviorKind::Attacking)) ||
      engaged.status == NpcBehaviorDecisionStatus::OnCooldown;
  if (!hostileCombat) {
    return engaged;
  }

  const std::uint8_t band = alertBandIndex(actor.alertLevel, profile);
  if (band >= 5U) {
    return engaged;  // combat band: keep the range split / cooldown / movement
  }

  // Sub-combat: hold at the alert rung, no attack/move command this tick.
  NpcBehaviorDecision downgraded = engaged;
  downgraded.behavior = alertBehaviorForLevel(actor.alertLevel, profile);
  downgraded.intent = AiIntentKind::Wait;
  downgraded.cooldownTicksRemaining = 0;
  return downgraded;
}

// A resolved perception with a usable actor position (mirrors the NpcBehaviorSystem
// predicate; patrol needs the live position to measure waypoint arrival).
bool perceptionHasActorPosition(const NpcPerceptionResult& perception) {
  return isValid(perception.actor) && isFinite(perception.actorPosition);
}

// Overlay last-known-position investigation onto the reconciled decision (slice 7). Sits in
// precedence BETWEEN combat (kept by reconcileAlertBand at band 5) and patrol (band <=1): when
// the guard is standing/aware (intent==Wait) with memory of a target it can no longer see and
// alert is still in the Searching/Alert band, walk to the remembered spot and look around.
// Gating on intent==Wait leaves combat move/attack, leash ReturnToAnchor, and defeat/disabled
// None untouched. Bands are disjoint from patrol's, so the two overlays never fight.
NpcBehaviorDecision maybeApplyInvestigate(const NpcBehaviorDecision& decision,
                                          AiActorState& actor,
                                          const NpcPerceptionResult& perception,
                                          const AlertProfile& profile,
                                          std::uint64_t tick) {
  static_cast<void>(tick);
  if (decision.intent != AiIntentKind::Wait || !perceptionHasActorPosition(perception)) {
    return decision;
  }

  const std::uint8_t band = alertBandIndex(actor.alertLevel, profile);
  const NpcInvestigateStep step =
      npcStepInvestigate(actor, perception.actorPosition, band, perception.perceived,
                         kPatrolArriveEpsilonMeters, kInvestigateDwellTicks);
  if (!step.active) {
    return decision;
  }

  // Behavior stays derived from the alert level (Searching/Alert); only the intent changes.
  NpcBehaviorDecision investigate = decision;
  investigate.status = NpcBehaviorDecisionStatus::Decided;
  investigate.behavior = alertBehaviorForLevel(actor.alertLevel, profile);
  investigate.cooldownTicksRemaining = 0;
  if (step.dwelling) {
    investigate.intent = AiIntentKind::Wait;  // look around at the spot, no move
  } else {
    investigate.intent = AiIntentKind::Investigate;
    investigate.homePosition = step.destination;
    investigate.returnStopDistanceMeters = kPatrolMoveStopMeters;
  }
  return investigate;
}

// Overlay low-alert patrol onto the reconciled decision (slice 6). Patrol drives
// movement ONLY when the NPC is at rest in the Idle/Observant band; every engaged /
// returning / passive / defeated / cooldown outcome is left untouched (none is an
// Idle/Observant + Wait resting state), so s5 and guard/leash behavior never regress.
NpcBehaviorDecision maybeApplyPatrol(const NpcBehaviorDecision& decision,
                                     AiActorState& actor,
                                     const NpcPerceptionResult& perception,
                                     const AlertProfile& profile) {
  const bool resting =
      decision.intent == AiIntentKind::Wait &&
      (decision.behavior == AiBehaviorKind::Idle ||
       decision.behavior == AiBehaviorKind::Observant);
  if (actor.patrolWaypoints.empty() || !resting ||
      alertBandIndex(actor.alertLevel, profile) > 1U ||
      !perceptionHasActorPosition(perception)) {
    return decision;
  }

  const NpcPatrolStep step =
      npcStepPatrol(actor, perception.actorPosition, kPatrolArriveEpsilonMeters);
  if (!step.active) {
    return decision;
  }

  // Keep the alert-derived behavior + target; switch to a point-move toward the
  // waypoint. status=Decided so shouldBuildCommandForDecision emits the Move (a resting
  // decision is NoTarget, which would emit nothing).
  NpcBehaviorDecision patrol = decision;
  patrol.status = NpcBehaviorDecisionStatus::Decided;
  patrol.intent = AiIntentKind::Patrol;
  patrol.homePosition = step.destination;
  // Rest strictly inside the arrival ring so a cornered approach always registers arrival
  // next tick (see kPatrolMoveStopMeters); arrival precision itself stays at the epsilon.
  patrol.returnStopDistanceMeters = kPatrolMoveStopMeters;
  patrol.cooldownTicksRemaining = 0;
  return patrol;
}

// Scored-search rung (A5 slice 2). A guard still hot (Searching/Alert) whose investigate memory is
// SPENT no longer Waits in place until decay -- it moves between the top-scored reasoning nodes
// (a5s1 chooseSearchNode), steered by the COLD memory sample, riding maybeFollowRoute's flanking.
// Precedence: combat > investigate > SEARCH > patrol, all intent==Wait gated. ZERO new AiIntentKind
// (reuses Investigate -- it IS investigating likely spots; the receipt distinguishes scored-search
// from memory-investigate). NEVER re-scores per tick: choose on entry, re-choose ONLY on arrival
// (alternation via excludedNodeId). Not-hot / has-memory / not-Waiting / EMPTY graph / no candidate
// => decision UNCHANGED (graphless sessions byte-identical). Search state = the actor's TRANSIENT
// fields (never hashed/saved).
NpcBehaviorDecision maybeApplySearch(NpcBehaviorDecision decision, AiActorState& actor, Vec3 guardPos,
                                     std::span<const PhysicsAabbCollider> colliders,
                                     const ReasoningGraph& graph, const AlertProfile& alertProfile,
                                     const NpcPersonalityWeights& weights, std::uint64_t tick) {
  const std::uint8_t band = alertBandIndex(actor.alertLevel, alertProfile);
  if (band < 3U || band > 4U) {
    actor.hasSearchChoice = false;  // cooled out of the search bands -> abandon
    return decision;
  }
  // Trigger keyed to the MEMORY FACT (not the outcome): a DWELLING guard and a VISUALLY-STARING
  // guard both HOLD memory (recording runs before the chain), so hasLastKnownTarget==false excludes
  // both; only a hot guard with SPENT memory searches. intent!=Wait means investigate/patrol/chase
  // already own the tick; empty graph => today's Wait-until-decay.
  if (decision.intent != AiIntentKind::Wait || actor.hasLastKnownTarget || graph.nodes.empty()) {
    return decision;
  }

  const auto nodePos = [&graph](std::uint32_t id) {
    return id < graph.nodes.size() ? graph.nodes[id].positionMeters : Vec3{};
  };
  const auto distanceMeters = [](Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(a - b)); };

  // COLD memory sample: the surviving stale position/tick (flag down) so suspicion still steers the
  // search toward where the target vanished.
  GuardMemorySample memory;
  memory.lastKnownPosition = actor.lastKnownTargetPosition;
  memory.lastKnownTick = actor.lastKnownTargetTick;
  memory.hasMemorySample = actor.hasLastKnownTarget || actor.lastKnownTargetTick != 0;

  // Choose on entry; re-choose ONLY on arrival at the current node, excluding it so the choice
  // ALTERNATES among top nodes (v1 honesty: alternation, not a full circuit).
  bool reChoose = !actor.hasSearchChoice;
  std::optional<std::uint32_t> excluded;
  if (actor.hasSearchChoice &&
      distanceMeters(guardPos, nodePos(actor.searchChosenNodeId)) <= kPatrolArriveEpsilonMeters) {
    reChoose = true;
    excluded = actor.searchChosenNodeId;
  }
  if (reChoose) {
    const GuardDecision chosen = chooseSearchNode(graph, colliders, guardPos, memory, tick,
                                                  actor.actor, weights, GuardDecisionConfig{}, excluded);
    actor.searchLastReceipt = chosen.receipt;
    if (!chosen.nodeId.has_value()) {
      actor.hasSearchChoice = false;
      return decision;  // no candidate -> pass through (today's Wait-until-decay)
    }
    actor.searchChosenNodeId = *chosen.nodeId;
    actor.hasSearchChoice = true;
  }

  // Reuse the Investigate intent toward the chosen node; maybeFollowRoute flanks it if blocked.
  decision.status = NpcBehaviorDecisionStatus::Decided;
  decision.behavior = alertBehaviorForLevel(actor.alertLevel, alertProfile);
  decision.intent = AiIntentKind::Investigate;
  decision.homePosition = nodePos(actor.searchChosenNodeId);
  decision.returnStopDistanceMeters = kPatrolMoveStopMeters;
  decision.cooldownTicksRemaining = 0;
  return decision;
}

// Route-follow POST-STEP (A4 slice 2). After the overlay chain has set a point-move DESTINATION,
// if that destination's straight segment is blocked, steer the guard along a planned graph route
// instead of stalling into the wall. Rewrites ONLY the interim destination + stop distance -- adds
// or reorders NO rung, introduces NO new AiIntentKind. Eligible: Investigate / ReturnToAnchor /
// Patrol (Patrol only actually routes when its waypoint is blocked, which the trigger enforces);
// Chasing/Attacking (band 5) stay DIRECT. EMPTY graph / no path / clear shot => decision untouched
// (today's behavior), so graphless sessions are byte-identical. Route state = the actor's TRANSIENT
// route fields (never hashed/saved). NEVER re-plans per tick: it invalidates on a cheap key
// mismatch and plans only when the final destination is straight-blocked.
NpcBehaviorDecision maybeFollowRoute(NpcBehaviorDecision decision, AiActorState& actor, Vec3 guardPos,
                                     std::span<const PhysicsAabbCollider> colliders,
                                     const ReasoningGraph& graph) {
  const auto clearRoute = [&actor]() {
    actor.hasRoute = false;
    actor.routeNodeIds.clear();
    actor.routeCursor = 0;
  };
  const auto nodePos = [&graph](std::uint32_t id) {
    return id < graph.nodes.size() ? graph.nodes[id].positionMeters : Vec3{};
  };
  const auto distanceMeters = [](Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(a - b)); };

  const bool routable = decision.status == NpcBehaviorDecisionStatus::Decided &&
                        (decision.intent == AiIntentKind::Investigate ||
                         decision.intent == AiIntentKind::ReturnToAnchor ||
                         decision.intent == AiIntentKind::Patrol);
  if (!routable) {
    return decision;  // Chasing/Attacking/Wait/None: never routed; any held route stays dormant.
  }

  const Vec3 finalDestination = decision.homePosition;  // the true target, captured BEFORE rewrite

  // (1) Invalidate a stale route -- cheap equality only, no ray, no Dijkstra. Investigate origins
  // move when fresh noise overwrites the memory; intents flip Return<->Chase<->Return.
  if (actor.hasRoute && (actor.routeIntent != decision.intent ||
                         !nearlyEqual(actor.routePlannedForDestination, finalDestination, 0.05F))) {
    clearRoute();
  }

  // (2) Plan ONLY when the final destination is straight-blocked (one query on the already-baked
  // per-tick occlusion colliders; NO new bake). A clear shot or an empty/no-path result => direct.
  bool justPlanned = false;
  if (!actor.hasRoute) {
    if (!reasoningSegmentBlocked(colliders, guardPos, finalDestination)) {
      return decision;  // clear shot -> today's direct behavior
    }
    const PlannedRoute planned = planRoute(graph, colliders, guardPos, finalDestination, {});
    if (planned.nodeIds.empty()) {
      return decision;  // no path / empty graph / unreachable -> direct (never worse than status quo)
    }
    actor.routeNodeIds = planned.nodeIds;
    actor.routeCursor = 0;
    actor.routeIntent = decision.intent;
    actor.routePlannedForDestination = finalDestination;
    actor.hasRoute = true;
    actor.routeLastPositionMeters = guardPos;
    justPlanned = true;
  }

  // (3) Advance the cursor through every route node already reached.
  const std::uint32_t cursorBefore = actor.routeCursor;
  const std::uint32_t routeSize = static_cast<std::uint32_t>(actor.routeNodeIds.size());
  while (actor.routeCursor < routeSize &&
         distanceMeters(guardPos, nodePos(actor.routeNodeIds[actor.routeCursor])) <=
             kPatrolArriveEpsilonMeters) {
    ++actor.routeCursor;
  }
  if (actor.routeCursor >= routeSize) {
    // Every route node reached -> the wall is flanked; the final leg goes DIRECT to the true target
    // with the intent's OWN stop (already on `decision`). Retire the route.
    clearRoute();
    return decision;
  }

  // (4) No-progress guard (deterministic, per-guard -- NEVER transient.lastMovementResult): if we
  // neither advanced a node nor moved since the previous follow tick, the leg is stuck. A re-plan
  // would reproduce the same blocked node, so DROP to direct rather than loop.
  const bool advancedNode = actor.routeCursor > cursorBefore;
  const bool moved = distanceMeters(guardPos, actor.routeLastPositionMeters) >= kPatrolArriveEpsilonMeters;
  if (!justPlanned && !advancedNode && !moved) {
    clearRoute();
    return decision;
  }
  actor.routeLastPositionMeters = guardPos;

  // (5) Steer to the current interim node with the PATROL stop pair. The intent's own stop (e.g.
  // ReturnToAnchor's larger returnStopDistanceMeters) would freeze the guard AT an interim node; it
  // applies only to the final leg handled in (3).
  decision.homePosition = nodePos(actor.routeNodeIds[actor.routeCursor]);
  decision.returnStopDistanceMeters = kPatrolMoveStopMeters;
  return decision;
}

void applyNpcBehaviorDecision(AiActorState& actorState,
                              const NpcBehaviorDecision& decision) {
  actorState.behavior = decision.behavior;
  actorState.lastIntent = decision.intent;
  actorState.target = decision.target;
  actorState.nextDecisionTick = decision.nextDecisionTick;
  actorState.cooldownTicksRemaining = decision.cooldownTicksRemaining;
}

Vec3 horizontalDirectionOrForward(Vec3 from, Vec3 to) {
  const Vec3 delta{to.x - from.x, 0.0F, to.z - from.z};
  const float lengthSq = lengthSquared(delta);
  if (!std::isfinite(lengthSq) || lengthSq < 1.0e-8F) {
    return Vec3{0.0F, 0.0F, 1.0F};
  }
  const float invLength = 1.0F / std::sqrt(lengthSq);
  return Vec3{delta.x * invLength, 0.0F, delta.z * invLength};
}

}  // namespace

// Provisional spawn facing until authored orientation exists: point the NPC at
// the first player entity so seeded guards start oriented toward the threat.
Vec3 initialNpcFacing(const WorldState& world, Vec3 actorPosition) {
  for (const EntityState& entity : world.entities()) {
    if (entity.kind == EntityKind::Player) {
      return horizontalDirectionOrForward(actorPosition, entity.transform.position);
    }
  }
  return Vec3{0.0F, 0.0F, 1.0F};
}

namespace {

void updateNpcFacing(AiActorState& actorState,
                     const NpcBehaviorDecision& decision,
                     const NpcPerceptionResult& perception) {
  switch (decision.intent) {
    case AiIntentKind::MoveTowardTarget:
    case AiIntentKind::AttackTarget:
      actorState.facingDirection = horizontalDirectionOrForward(
          perception.actorPosition, perception.targetPosition);
      break;
    case AiIntentKind::ReturnToAnchor:
    case AiIntentKind::Patrol:
    case AiIntentKind::Investigate:
      // Point-moves face decision.homePosition (anchor / waypoint / last-known sighting).
      actorState.facingDirection = horizontalDirectionOrForward(
          perception.actorPosition, decision.homePosition);
      break;
    case AiIntentKind::None:
    case AiIntentKind::Wait:
      break;  // hold current gaze
  }
}

NpcPerceptionResult::Los losFromSegmentOcclusion(SegmentOcclusionVerdict verdict) {
  switch (verdict) {
    case SegmentOcclusionVerdict::Clear:
      return NpcPerceptionResult::Los::Clear;
    case SegmentOcclusionVerdict::Blocked:
      return NpcPerceptionResult::Los::Blocked;
    case SegmentOcclusionVerdict::Unknown:
      return NpcPerceptionResult::Los::Unknown;
  }
  return NpcPerceptionResult::Los::Unknown;
}

AiPerceptionLos aiLosFromPerceptionLos(NpcPerceptionResult::Los los) {
  switch (los) {
    case NpcPerceptionResult::Los::Clear:
      return AiPerceptionLos::Clear;
    case NpcPerceptionResult::Los::Blocked:
      return AiPerceptionLos::Blocked;
    case NpcPerceptionResult::Los::Unknown:
      return AiPerceptionLos::Unknown;
  }
  return AiPerceptionLos::Unknown;
}

// Cast an eye-to-eye segment from actor to target against this tick's baked world colliders.
NpcPerceptionResult::Los actorLineOfSightToTarget(
    const PhysicsSpatialSurfaceColliderBakeResult& bake,
    const EntityState* actor,
    const EntityState* target,
    const NpcBehaviorConfig& config) {
  if (!bake.ok || actor == nullptr || target == nullptr) {
    return NpcPerceptionResult::Los::Unknown;
  }
  Vec3 origin = actor->transform.position;
  origin.y += config.guardEyeHeightMeters;
  Vec3 targetEye = target->transform.position;
  // TODO(P2 follow-up): switch to the sneak eye height when stance is exposed here.
  targetEye.y += config.targetStandEyeHeightMeters;
  return losFromSegmentOcclusion(
      segmentOcclusion(bake.colliders, origin, targetEye, config.occlusionMarginMeters));
}

// Point-to-point occlusion for the sound path (a1s2, L1): same lifted shape as before on raw
// positions, returning APPLY-ONCE whether a wall or unknown bake sits between.
bool soundHasBlockerBetween(const PhysicsSpatialSurfaceColliderBakeResult& bake,
                            Vec3 from,
                            Vec3 to,
                            const NpcBehaviorConfig& config) {
  if (!bake.ok) {
    return true;
  }
  Vec3 origin = from;
  origin.y += config.guardEyeHeightMeters;
  Vec3 destEye = to;
  destEye.y += config.guardEyeHeightMeters;
  const SegmentOcclusionVerdict verdict =
      segmentOcclusion(bake.colliders, origin, destEye, config.occlusionMarginMeters);
  return verdict != SegmentOcclusionVerdict::Clear;
}

}  // namespace

void enqueueNpcBehaviorCommands(
    Session& session,
    SessionState& state,
    const PhysicsSpatialSurfaceColliderBakeResult& occlusionBake) {
  if (state.lifecycle != SessionLifecycle::Playing || state.clock.mode == ClockMode::Paused) {
    return;
  }

  ensureAiActorsForActiveNpcs(state);
  const PlayerSlot* playerZero = state.players.findSlot(0);
  const EntityId target = playerZero == nullptr ? EntityId{} : playerZero->actor;
  const NpcBehaviorProfileCatalog profileCatalog = makeBuiltInNpcBehaviorProfileCatalog();

  std::vector<std::size_t> actorIndexes;
  actorIndexes.reserve(state.ai.actors.size());
  for (std::size_t index = 0; index < state.ai.actors.size(); ++index) {
    const EntityState* actor = state.world.findById(state.ai.actors[index].actor);
    if (actor != nullptr && actor->kind == EntityKind::Npc) {
      actorIndexes.push_back(index);
    }
  }
  std::sort(actorIndexes.begin(), actorIndexes.end(), [&](std::size_t lhs, std::size_t rhs) {
    return state.ai.actors[lhs].actor < state.ai.actors[rhs].actor;
  });

  for (std::size_t index : actorIndexes) {
    AiActorState& actorState = state.ai.actors[index];
    const NpcBehaviorProfileResolveResult resolvedProfile =
        resolveNpcBehaviorProfile({&profileCatalog, actorState.behaviorProfileId});
    if (!resolvedProfile.ok) {
      continue;
    }
    const NpcBehaviorConfig config = resolvedProfile.config;
    const AlertProfile alertProfile = resolvedProfile.profile.alertProfile;

    const EntityState* actorEntity = state.world.findById(actorState.actor);
    const EntityState* targetEntity = state.world.findById(target);
    const NpcPerceptionResult::Los targetLos =
        actorLineOfSightToTarget(occlusionBake, actorEntity, targetEntity, config);

    NpcPerceptionRequest perceptionRequest;
    perceptionRequest.world = &state.world;
    perceptionRequest.combat = &state.combat;
    perceptionRequest.actor = actorState.actor;
    perceptionRequest.target = target;
    perceptionRequest.config = config;
    perceptionRequest.actorFacingDirection = actorState.facingDirection;
    perceptionRequest.targetLos = targetLos;
    const NpcPerceptionResult perception = queryNpcPerception(perceptionRequest);

    // Step the graded-alert FSM from the perception already computed. It writes
    // actor.alertLevel (durable) and actor.behavior; the final behavior is set
    // authoritatively by applyNpcBehaviorDecision below, so the intermediate
    // behavior write here is harmless.
    NpcAlertStimulus stimulus;
    stimulus.targetPerceived = perception.perceived;
    stimulus.proximity01 =
        config.perceptionRadiusMeters > 0.0F
            ? clamp01(1.0F - perception.distanceMeters / config.perceptionRadiusMeters)
            : 0.0F;
    stimulus.hasValidTarget = perceptionHasLiveTarget(perception.status);
    stimulus.visualConfirmed = perception.perceived;

    // PERCEIVE (a1s2, L1): resolve THIS tick's sound bus at the guard. Self-hearing
    // skip -- a guard never hears an event it emitted (v1 guards are silent, but the
    // guard is defensively excluded so patrol/idle can't self-alert). blockers[i] is
    // the APPLY-ONCE wall/unknown test between guard and each origin, reusing the tick bake.
    // guardPos comes from actorEntity; if it's missing the guard simply hears nothing.
    if (actorEntity != nullptr && !state.transient.soundEvents.empty()) {
      const Vec3 guardPos = actorEntity->transform.position;
      std::vector<SoundEvent> audibleEvents;
      audibleEvents.reserve(state.transient.soundEvents.size());
      // Parallel blocker buffer. std::vector<bool> is bit-packed and cannot back a
      // std::span<const bool>, so use a plain heap bool[] the span can view.
      auto blockers = std::make_unique<bool[]>(state.transient.soundEvents.size());
      std::size_t heardCount = 0;
      for (const SoundEvent& event : state.transient.soundEvents) {
        if (event.source == actorState.actor) {
          continue;  // never hear yourself
        }
        blockers[heardCount] =
            soundHasBlockerBetween(occlusionBake, guardPos, event.originMeters, config);
        audibleEvents.push_back(event);
        ++heardCount;
      }
      const SoundPerceptionResult snd = resolveLoudestSound(
          audibleEvents, guardPos, resolvedProfile.profile.soundConfig,
          std::span<const bool>(blockers.get(), heardCount));
      stimulus.heard = snd.heard;
      stimulus.audibilityDb = snd.audibilityDb;
      stimulus.alertUnits = snd.alertUnits;
      stimulus.soundInvestigatePos = snd.investigatePos;
    }

    const std::uint8_t alertBandBefore = alertBandIndex(actorState.alertLevel, alertProfile);
    npcStepAlert(actorState, stimulus, alertProfile, state.clock.tickIndex);
    const std::uint8_t alertBandAfter = alertBandIndex(actorState.alertLevel, alertProfile);
    const bool alertBandIncreased = alertBandAfter > alertBandBefore;
    // Remember where the target is while it is actually seen (slice 7). visualConfirmed implies
    // Ready, so perception.targetPosition is the live sighting. MEMORY (a1s2/P5b): if the target
    // was heard but never seen this tick, share the SAME last-known memory but only refresh it
    // for a relocated sound or strict alert-band rise. Visual always wins when both are present.
    if (stimulus.visualConfirmed) {
      npcRecordSighting(actorState, perception.targetPosition, state.clock.tickIndex);
    } else if (stimulus.heard) {
      (void)npcRecordNonvisualInvestigationMemory(
          actorState, stimulus.soundInvestigatePos, state.clock.tickIndex,
          alertBandIncreased);
    }

    const NpcBehaviorDecision engaged =
        chooseNpcBehaviorIntent(NpcBehaviorDecisionRequest{&actorState, perception, config,
                                                           state.clock.tickIndex});
    NpcBehaviorDecision decision =
        reconcileAlertBand(engaged, actorState, alertProfile);
    // Precedence: combat (kept above) > investigate last-known (band 3-4) > SEARCH (a5s2) > patrol.
    decision =
        maybeApplyInvestigate(decision, actorState, perception, alertProfile, state.clock.tickIndex);
    // SEARCH rung (a5s2): a hot guard with SPENT memory checks scored nodes instead of Waiting.
    if (actorEntity != nullptr && occlusionBake.ok) {
      decision = maybeApplySearch(decision, actorState, actorEntity->transform.position,
                                  occlusionBake.colliders, state.reasoningGraph, alertProfile,
                                  resolvedProfile.profile.personalityWeights, state.clock.tickIndex);
    }
    decision = maybeApplyPatrol(decision, actorState, perception, alertProfile);
    // Route-follow POST-STEP (A4 s2): flank blocked destinations via the carried reasoning graph.
    // Rewrites only the interim destination inside the rung above; empty graph -> unchanged.
    if (actorEntity != nullptr && occlusionBake.ok) {
      decision = maybeFollowRoute(decision, actorState, actorEntity->transform.position,
                                  occlusionBake.colliders, state.reasoningGraph);
    }
    applyNpcBehaviorDecision(actorState, decision);
    updateNpcFacing(actorState, decision, perception);
    actorState.lastTargetInRadius = perception.targetInPerceptionRadius;
    actorState.lastTargetInVisionCone = perception.targetInVisionCone;
    actorState.lastTargetHasLineOfSight = perception.hasLineOfSight;
    actorState.lastSightRangeMeters = config.perceptionRadiusMeters;
    actorState.lastHorizontalAngleDeg = perception.horizontalAngleDeg;
    actorState.lastVerticalAngleDeg = perception.verticalAngleDeg;
    actorState.lastInVerticalCone = perception.inVerticalCone;
    actorState.lastPerceived = perception.perceived;
    actorState.lastLos = aiLosFromPerceptionLos(perception.los);
    actorState.lastGuardEyeHeightMeters = config.guardEyeHeightMeters;
    actorState.lastVerticalHalfAngleDegrees = config.verticalHalfAngleDegrees;

    if (!shouldBuildCommandForDecision(decision)) {
      continue;
    }

    const NpcBehaviorCommandResult command =
        buildNpcBehaviorCommand(NpcBehaviorCommandRequest{decision, perception, config});
    if (command.hasCommand) {
      (void)appendCommandThroughAdmission(session, state, command.command);
    }
  }
}

}  // namespace session_detail

}  // namespace iggy3d

