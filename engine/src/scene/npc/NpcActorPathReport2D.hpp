#pragma once

#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorNavigationRequest2D.hpp"
#include "servers/navigation/NavigationPath.hpp"

namespace iggy {

enum class NpcActorPathReport2DStatus {
	PathFound,
	NoNavigationRequest,
	PathNotFound,
};

struct NpcActorPathReport2D {
	NpcActorNavigationRequest2D navigation;
	navigation::NavigationPath path;
	NpcActorPathReport2DStatus status = NpcActorPathReport2DStatus::NoNavigationRequest;
	bool requestsStep = false;

	[[nodiscard]] bool hasPath() const;
};

class NpcActorPathReporter2D {
public:
	[[nodiscard]] NpcActorPathReport2D findPath(
		const NpcActorNavigationRequest2D &navigation,
		const LevelTileMap &map) const;
};

} // namespace iggy
