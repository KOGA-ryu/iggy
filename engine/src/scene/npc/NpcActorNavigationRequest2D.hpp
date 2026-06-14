#pragma once

#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorRouteTarget2D.hpp"
#include "servers/navigation/NavigationRequest.hpp"

namespace iggy {

enum class NpcActorNavigationRequest2DStatus {
	Ready,
	NoRouteTarget,
	NavigationRejected,
};

struct NpcActorNavigationRequest2D {
	NpcActorRouteTarget2D route;
	navigation::NavigationRequest request;
	NpcActorNavigationRequest2DStatus status = NpcActorNavigationRequest2DStatus::NoRouteTarget;
	bool requestsPath = false;

	[[nodiscard]] bool ready() const;
};

class NpcActorNavigationRequestBuilder2D {
public:
	[[nodiscard]] NpcActorNavigationRequest2D build(
		const NpcActorRouteTarget2D &route,
		const LevelTileMap &map) const;
};

} // namespace iggy
