#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcMovementPlan.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationPath.hpp"
#include "servers/navigation/NavigationPathFollower.hpp"
#include "servers/navigation/NavigationRequest.hpp"

namespace iggy::npc_ai {

enum class NpcNavigationStatus {
	NoMovement,
	RequestRejected,
	PathNotFound,
	Moving,
	Arrived,
};

struct NpcNavigationResult {
	NpcNavigationStatus status = NpcNavigationStatus::NoMovement;
	Vec2 nextPosition;
	navigation::NavigationRequestStatus requestStatus = navigation::NavigationRequestStatus::None;
	navigation::NavigationPathStatus pathStatus = navigation::NavigationPathStatus::NoPath;
	navigation::NavigationPathFollowState followState;
};

class NpcNavigationController {
public:
	[[nodiscard]] NpcNavigationResult step(const LevelTileMap &map, Vec2 currentPosition, const NpcMovementPlan &plan, navigation::NavigationPathFollowState followState, float maxDistance) const;
};

} // namespace iggy::npc_ai
