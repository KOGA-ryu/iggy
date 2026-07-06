#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"  // RoomAsset + creative::CreativeDocument

namespace iggy3d {

class Session;

// The production activation hook the reasoning graph was missing (game_master_plan: the #1 slice
// unblocker). Build the L4 reasoning graph from the freshly baked room and install it on the
// session, so the shipped stealth guard reasons over the AUTHORED room instead of the empty-graph
// fallback. buildReasoningGraph is pure + deterministic. Matches ProductCreativeBakedRoomActivationHook.
//
// Patrol waypoints are empty for now: the room's baked anchors already seed spawn/npc/objective/
// cover/... nodes. Patrol-post nodes arrive when patrolRouteWaypointsFromDocument extracts authored
// PatrolRoute waypoints (blocked on Line-endpoint storage).
void activateCreativeReasoningGraph(Session& session, const RoomAsset& room,
                                    const creative::CreativeDocument& document);

}  // namespace iggy3d
