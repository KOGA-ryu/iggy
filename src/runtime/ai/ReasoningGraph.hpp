#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

// L4 reasoning graph (A3, deck_driven_tactical_ai.md §12): "the physical map moves bodies; the
// reasoning map moves thoughts." A SPARSE graph of meaningful positions, derived as a PURE
// function of explicit inputs -- no session, no tick, no RNG. v1 derives anchor + waypoint nodes
// and walkable edges only; geometry-derived nodes (doorways/chokepoints) are the a3s1b follow-up.
// Determinism law: identical inputs always yield a bitwise-identical graph (stable-ordered ids).
//
// Two-readers law: menu/Notebook.hpp will INCLUDE this header later, never the reverse -- keep
// this module free of app/menu/session deps. Including content/assets + runtime/physics from
// runtime/ai is legal (runtime/collision precedent) and slice-authorized.

// Full §12 vocabulary from birth -- APPEND-ONLY (it serializes later and the notebook reads it),
// even though v1 emits only exit / objective / patrolPost / reference.
enum class ReasoningNodeKind : std::uint8_t {
  doorway,
  stair,
  chokepoint,
  hidingSpot,
  coverCluster,
  objective,
  window,
  ladder,
  exit,
  patrolPost,
  highGround,
  soundSource,
  lastKnownPosition,
  // 14th, appended: an entity reference point (spawn/npc). §12 has no fit for an entity anchor
  // and the garden pin needs it; append-only order is preserved. Flagged to the planner.
  reference,
};

// Full edge vocabulary from birth -- APPEND-ONLY; v1 emits `walkable` only.
enum class ReasoningEdgeKind : std::uint8_t {
  walkable,
  hidden,
  climb,
  locked,
  noisy,
  dangerous,
  guarded,
};

// Count of ReasoningNodeKind values (append-only enum: doorway..reference). Keep in sync when a
// kind is appended -- it sizes the per-kind summary array.
inline constexpr std::size_t kReasoningNodeKindCount = 14;

// Count of ReasoningEdgeKind values (walkable..guarded). Sizes L5's per-edge-kind cost table.
inline constexpr std::size_t kReasoningEdgeKindCount = 7;

inline constexpr float kReasoningDefaultOcclusionEyeHeightMeters = 1.6F;
inline constexpr float kReasoningDefaultOcclusionMarginMeters = 0.05F;

std::string_view reasoningNodeKindName(ReasoningNodeKind kind);
std::string_view reasoningEdgeKindName(ReasoningEdgeKind kind);

// NAMED tuning (no magic numbers). v1 seed; scenario-overridable / A10-tunable later.
struct ReasoningGraphConfig {
  float maxLinkDistanceMeters = 20.0F;
  float occlusionEyeHeightMeters = kReasoningDefaultOcclusionEyeHeightMeters;
  float occlusionMarginMeters = kReasoningDefaultOcclusionMarginMeters;
};

// Actor-blocking test between two world points, lifted to the reasoning occlusion eye height. This
// is the ONE segment discipline shared by a3s1's edge build and L5's route reachability, so what
// routes == what an edge links == what a sense traverses. Empty valid colliders are clear (R3.1);
// invalid/unknown occlusion is blocked for reasoning. Takes ALREADY-BAKED colliders (never bakes).
bool reasoningSegmentBlocked(
    std::span<const PhysicsAabbCollider> colliders,
    Vec3 from,
    Vec3 to,
    float eyeHeightMeters = kReasoningDefaultOcclusionEyeHeightMeters,
    float marginMeters = kReasoningDefaultOcclusionMarginMeters);

struct ReasoningNode {
  std::uint32_t id = 0U;
  ReasoningNodeKind kind = ReasoningNodeKind::reference;
  Vec3 positionMeters;
  // Provenance: the origin anchor kind string ("exit"/"treasure"/"spawn"/"npc"/"waypoint"/...).
  // For debug + shape pins; not hashed or serialized in this slice.
  std::string sourceLabel;
};

struct ReasoningEdge {
  std::uint32_t from = 0U;
  std::uint32_t to = 0U;
  ReasoningEdgeKind kind = ReasoningEdgeKind::walkable;
  float lengthMeters = 0.0F;
};

struct ReasoningGraph {
  std::vector<ReasoningNode> nodes;
  std::vector<ReasoningEdge> edges;
};

// Observability facts about a built graph -- a SIBLING to the graph (kept OFF NpcBehaviorDebugSnapshot
// so the graph facts don't collide with the pending HUD string-mirror cut). Indexed by
// static_cast<std::size_t>(ReasoningNodeKind).
struct ReasoningGraphSummary {
  std::size_t nodeCount = 0;
  std::size_t edgeCount = 0;
  std::array<std::size_t, kReasoningNodeKindCount> perKindCounts{};
};

ReasoningGraphSummary summarizeReasoningGraph(const ReasoningGraph& graph);

// Pure builder. NODES: each derivable `room.anchors` entry (exit / treasure|key|pickup ->
// objective / spawn|npc -> reference) plus one `patrolPost` per waypoint. ORDER: stable-sorted by
// (kind, x, z, y), id = sorted index (bitwise-reproducible). EDGES: `walkable` between node pairs
// within `maxLinkDistanceMeters` whose eye-height segment crosses no actor-blocking surface (the
// SAME colliders vision/hearing use). All-pairs over the sparse set -- BUILD-TIME only, never a
// per-tick navmesh. No session access; deterministic.
ReasoningGraph buildReasoningGraph(const RoomAsset& room,
                                   std::span<const Vec3> patrolWaypoints,
                                   const ReasoningGraphConfig& config = {});

}  // namespace iggy3d
