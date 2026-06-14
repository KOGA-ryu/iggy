#include "scene/npc/NpcActorPathReport2D.hpp"

#include "servers/navigation/NavigationGridPathfinder.hpp"

namespace iggy {

bool NpcActorPathReport2D::hasPath() const
{
	return status == NpcActorPathReport2DStatus::PathFound;
}

NpcActorPathReport2D NpcActorPathReporter2D::findPath(
	const NpcActorNavigationRequest2D &navigation,
	const LevelTileMap &map) const
{
	NpcActorPathReport2D result;
	result.navigation = navigation;

	if (!navigation.ready() || !navigation.requestsPath) {
		result.status = NpcActorPathReport2DStatus::NoNavigationRequest;
		return result;
	}

	result.path = navigation::NavigationGridPathfinder {}.findPath(
		map,
		navigation.route.startPosition,
		navigation.request);
	if (!result.path.found()) {
		result.status = NpcActorPathReport2DStatus::PathNotFound;
		return result;
	}

	result.status = NpcActorPathReport2DStatus::PathFound;
	result.requestsStep = true;
	return result;
}

} // namespace iggy
