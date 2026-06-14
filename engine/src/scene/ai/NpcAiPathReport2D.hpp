#pragma once

#include "scene/ai/NpcAiNavigationRequest2D.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationPath.hpp"

namespace iggy {

enum class NpcAiPathReport2DStatus {
	PathFound,
	NoNavigationRequest,
	PathNotFound,
};

struct NpcAiPathReport2DResult {
	NpcAiPathReport2DStatus status = NpcAiPathReport2DStatus::NoNavigationRequest;
	NpcAiNavigationRequest2DResult navigation;
	navigation::NavigationPath path;

	[[nodiscard]] bool hasPath() const;
};

class NpcAiPathReporter2D {
public:
	[[nodiscard]] NpcAiPathReport2DResult findPath(
		const NpcAiNavigationRequest2DResult &navigation,
		const LevelTileMap &map) const;
};

} // namespace iggy
