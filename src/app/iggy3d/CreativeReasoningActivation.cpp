#include "app/iggy3d/CreativeReasoningActivation.hpp"

#include "app/iggy3d/PatrolRouteWaypoints.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

void activateCreativeReasoningGraph(Session& session, const RoomAsset& room,
                                    const creative::CreativeDocument& document) {
  // The baked room's anchors seed exit / objective / reference nodes; authored PatrolRoute path
  // points become the guard's patrolPost nodes. Both feed the same deterministic build-once graph.
  const PatrolRouteWaypoints patrol = patrolRouteWaypointsFromDocument(document);
  session.setReasoningGraph(buildReasoningGraph(room, patrol.waypoints));
}

}  // namespace iggy3d
