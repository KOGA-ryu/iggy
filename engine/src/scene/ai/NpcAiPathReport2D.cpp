#include "scene/ai/NpcAiPathReport2D.hpp"

#include "servers/navigation/NavigationGridPathfinder.hpp"

namespace iggy {

bool NpcAiPathReport2DResult::hasPath() const
{
	return status == NpcAiPathReport2DStatus::PathFound;
}

NpcAiPathReport2DResult NpcAiPathReporter2D::findPath(
	const NpcAiNavigationRequest2DResult &navigation,
	const LevelTileMap &map) const
{
	NpcAiPathReport2DResult result;
	result.navigation = navigation;

	if (!navigation.hasNavigationRequest()) {
		result.status = NpcAiPathReport2DStatus::NoNavigationRequest;
		return result;
	}

	result.path = navigation::NavigationGridPathfinder {}.findPath(
		map,
		navigation.route.startPosition,
		navigation.request);
	result.status = result.path.found()
		? NpcAiPathReport2DStatus::PathFound
		: NpcAiPathReport2DStatus::PathNotFound;
	return result;
}

} // namespace iggy
