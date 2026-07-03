#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

// L7 influence maps (A6, deck doc §9): per-channel scalar fields over the reasoning graph's NODES --
// "Go played on the reasoning graph", NOT a heatmap over the floor (the no-navmesh law holds; nodes
// are the only sample points). Pure, deterministic, INERT -- nothing consumes it (a6s2 wires the
// readers). Static channels (escape/objective) depend only on the graph; dynamic channels
// (guard/visibility) depend on the guard samples. Cadence is the CALLER's law; the kernel is math.

// Full deck-§9 channel vocabulary from birth (APPEND-ONLY). v1 computes the first four; `danger` and
// `soundPressure` are RESERVED enum slots (left 0.0).
enum class InfluenceChannel : std::uint8_t {
  guardInfluence,
  visibilityCoverage,
  escapeRoutePressure,
  objectiveControl,
  danger,
  soundPressure,
};
inline constexpr std::size_t kInfluenceChannelCount = 6;  // keep in sync -- sizes the per-node array

std::string_view influenceChannelName(InfluenceChannel channel);

// A guard sample the field is built from -- a plain position (the kernel knows NOTHING of
// AiActorState). Named struct (not a bare Vec3) so the pre-named cone-aware refinement can append a
// `facingDegrees` field WITHOUT churning buildInfluenceMap's signature.
struct InfluenceGuardSample {
  Vec3 positionMeters;
};

// Named tuning (no magic numbers). Reference-seeded; A10 retunes.
struct InfluenceMapConfig {
  float guardInfluenceRadiusMeters = 8.0F;    // linear presence falloff + HARD cutoff
  float visibilityRadiusMeters = 12.0F;       // potential-visibility radius (omnidirectional)
  float escapePressureDecayMeters = 6.0F;     // graph-distance decay scale to the nearest exit
  float objectiveControlDecayMeters = 6.0F;   // graph-distance decay scale from the nearest objective
};

struct InfluenceMap {
  std::size_t nodeCount = 0;
  // Row i corresponds to graph.nodes[i] (parallel order); index a row by InfluenceChannel.
  std::vector<std::array<float, kInfluenceChannelCount>> values;
};

// Build the per-node, per-channel field. Pure; NEVER bakes (colliders are already-baked input).
// Empty graph => empty map (nodeCount 0), never a crash.
//
// Channel v1 semantics:
//  - guardInfluence (DYNAMIC): sum over guards of max(0, 1 - dEuclid/guardInfluenceRadiusMeters)
//    (linear, exactly 0 at/beyond the radius).
//  - visibilityCoverage (DYNAMIC): 1.0 if ANY guard has dEuclid <= visibilityRadiusMeters AND a
//    clear segment to the node, else 0.0. This is POTENTIAL visibility (omnidirectional -- radius +
//    LOS, NO cone: a guard COULD see the node if it faced that way). Cone-aware actual-visibility is
//    a pre-named later refinement (facing churns per tick; potential visibility is stable + honest
//    -- it may only OVER-cover vs a cone, never claim a cone-blocked node).
//  - escapeRoutePressure (STATIC): exp(-dGraph / escapePressureDecayMeters), dGraph = least graph
//    distance to the nearest `exit` node. PEAK 1.0 AT exits; 0.0 where no exit is reachable.
//  - objectiveControl (STATIC): exp(-dGraph / objectiveControlDecayMeters) from the nearest
//    `objective` node. PEAK 1.0 at objectives; 0.0 where none is reachable.
//  - danger, soundPressure: reserved, 0.0.
InfluenceMap buildInfluenceMap(const ReasoningGraph& graph,
                               std::span<const PhysicsAabbCollider> colliders,
                               std::span<const InfluenceGuardSample> guards,
                               const InfluenceMapConfig& config = {});

// Bounds-safe accessor (0.0 out of range).
float influenceValue(const InfluenceMap& map, std::size_t nodeIndex, InfluenceChannel channel);

}  // namespace iggy3d
