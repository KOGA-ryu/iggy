#pragma once

#include <cstdint>
#include <string_view>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/ai/ReasoningGraph.hpp"

namespace iggy3d {

// Recon-projection kernel (game_master_plan L3 -- the notebook chain head). A PURE, deterministic
// projection of ONE guard's durable, scoutable state into an inspectable observation: the intel a
// thief's notebook records about a guard. Reads only durable AiActorState + the guard's world
// position (PASSED IN, never read off an entity) + the session reasoning graph. No session, no tick,
// no RNG, no Date. Generic over ANY guard -- a new guard kind needs no new code here.
struct GuardReconObservation {
  EntityId guard;
  Vec3 position;               // guard world position (the caller resolves it; never fabricated)
  Vec3 facing;                 // horizontal gaze direction the vision cone originates from
  float alertLevel = 0.0F;     // 0..1 (combat == 1.0)
  std::string_view behavior;   // aiBehaviorKindName(state.behavior)

  // Patrol timing.
  bool patrols = false;
  std::uint32_t patrolWaypointCount = 0;
  std::uint32_t patrolTargetIndex = 0;   // waypoint currently walking toward
  PatrolMode patrolMode = PatrolMode::Loop;
  Vec3 patrolTarget;                     // the waypoint being walked to (meaningful iff patrols)

  // Last-known intruder memory.
  bool hasLastKnownTarget = false;
  Vec3 lastKnownTargetPosition;

  // The reasoning node the guard is searching toward, resolved against the graph.
  bool hasWatchedNode = false;
  ReasoningNodeKind watchedNodeKind = ReasoningNodeKind::reference;
  Vec3 watchedNodePosition;
};

// Project one guard. `guardPosition` is supplied by the caller (the pure kernel never reads an
// Entity). The watched node is a lookup by id in `graph`; an unresolved id -- empty graph, or no
// current search choice -- leaves hasWatchedNode false, NEVER a fabricated node. Deterministic:
// identical inputs yield an identical observation.
GuardReconObservation projectGuardRecon(const AiActorState& guard,
                                        Vec3 guardPosition,
                                        const ReasoningGraph& graph);

}  // namespace iggy3d
