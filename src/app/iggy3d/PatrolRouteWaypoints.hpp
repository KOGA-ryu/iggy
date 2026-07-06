#pragma once

#include <cstdint>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

namespace creative {
class CreativeDocument;
}

// The authored patrol posts extracted from a CreativeDocument, ready to drop
// straight into buildReasoningGraph's `patrolWaypoints` span. Each PatrolRoute
// object contributes its stored path points (already world-space -- no transform
// applied, matching DocumentWireframe) in document order. A PatrolRoute with no
// path points is still counted as a route but yields no waypoints. Path points
// that are non-finite or exceed float range are dropped (they could never bake
// into a usable patrolPost node) and counted so the drop is never silent.
struct PatrolRouteWaypoints {
  std::vector<Vec3> waypoints;                    // flattened, document order
  std::uint64_t patrolRouteCount = 0;             // PatrolRoute objects seen
  std::uint64_t contributingRouteCount = 0;       // routes that yielded >= 1 waypoint
  std::uint64_t skippedNonFinitePointCount = 0;   // path points dropped
};

// Pure + deterministic. Reads only PatrolRoute-kind objects; ignores everything
// else. Feeds the reasoning-graph fill hook (activateCreativeReasoningGraph),
// turning authored PatrolRoute path points into the guard's patrol-post nodes.
[[nodiscard]] PatrolRouteWaypoints patrolRouteWaypointsFromDocument(
    const creative::CreativeDocument& document);

}  // namespace iggy3d
