#include "app/iggy3d/CreativeReasoningActivation.hpp"

#include <span>

#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

void activateCreativeReasoningGraph(Session& session, const RoomAsset& room,
                                    const creative::CreativeDocument& document) {
  // Patrol waypoints (authored PatrolRoute) arrive with patrolRouteWaypointsFromDocument; the baked
  // room's anchors already seed the reasoning nodes today.
  (void)document;
  session.setReasoningGraph(buildReasoningGraph(room, std::span<const Vec3>{}));
}

}  // namespace iggy3d
