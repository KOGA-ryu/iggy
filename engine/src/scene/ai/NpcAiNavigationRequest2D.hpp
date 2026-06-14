#pragma once

#include "scene/ai/NpcAiRouteRequest2D.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationRequest.hpp"

namespace iggy {

enum class NpcAiNavigationRequest2DStatus {
	Built,
	NoRouteRequest,
	NavigationRequestInvalid,
};

struct NpcAiNavigationRequest2DResult {
	NpcAiNavigationRequest2DStatus status = NpcAiNavigationRequest2DStatus::NoRouteRequest;
	NpcAiRouteRequest2DResult route;
	navigation::NavigationRequest request;

	[[nodiscard]] bool hasNavigationRequest() const;
};

class NpcAiNavigationRequestBuilder2D {
public:
	[[nodiscard]] NpcAiNavigationRequest2DResult build(
		const NpcAiRouteRequest2DResult &route,
		const LevelTileMap &map) const;
};

} // namespace iggy
