#include "servers/navigation/NavigationGridPathfinder.hpp"

#include <algorithm>
#include <cstddef>
#include <queue>
#include <vector>

#include "scene/level/LevelGridQuery.hpp"

namespace iggy::navigation {

namespace {

NavigationPath Path(NavigationPathStatus status)
{
	NavigationPath path;
	path.status = status;
	return path;
}

NavigationPath BuildPath(const LevelTileMap &map, TileCoord start, TileCoord destination, const std::vector<int> &previous)
{
	NavigationPath path;
	path.status = NavigationPathStatus::Found;

	for (TileCoord cursor = destination;;) {
		path.tiles.push_back(cursor);
		if (sameTile(cursor, start))
			break;

		const int previousIndex = previous[tileIndex(map, cursor)];
		cursor = { previousIndex % map.width, previousIndex / map.width };
	}

	std::reverse(path.tiles.begin(), path.tiles.end());
	path.waypoints.reserve(path.tiles.size());
	for (TileCoord tile : path.tiles)
		path.waypoints.push_back(tileCenter(tile));
	return path;
}

} // namespace

bool NavigationPath::found() const
{
	return status == NavigationPathStatus::Found;
}

NavigationPath NavigationGridPathfinder::findPath(const LevelTileMap &map, Vec2 start, const NavigationRequest &request) const
{
	if (!request.accepted())
		return Path(NavigationPathStatus::DestinationRejected);

	const TileCoord startTile = tileForPoint(start);
	if (!containsTile(map, startTile))
		return Path(NavigationPathStatus::StartOutOfBounds);
	if (!isWalkable(map, startTile))
		return Path(NavigationPathStatus::StartBlocked);

	const TileCoord destination { request.destinationTileX, request.destinationTileY };
	if (sameTile(startTile, destination)) {
		NavigationPath path;
		path.status = NavigationPathStatus::Found;
		path.tiles.push_back(startTile);
		path.waypoints.push_back(tileCenter(startTile));
		return path;
	}

	const std::size_t tileCount = static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height);
	std::vector<bool> visited(tileCount, false);
	std::vector<int> previous(tileCount, -1);
	std::queue<TileCoord> frontier;

	frontier.push(startTile);
	visited[tileIndex(map, startTile)] = true;

	const TileCoord neighbors[] = {
		{ 1, 0 },
		{ 0, 1 },
		{ -1, 0 },
		{ 0, -1 },
	};

	while (!frontier.empty()) {
		const TileCoord current = frontier.front();
		frontier.pop();

		for (TileCoord neighborOffset : neighbors) {
			const TileCoord neighbor { current.x + neighborOffset.x, current.y + neighborOffset.y };
			if (!containsTile(map, neighbor) || !isWalkable(map, neighbor))
				continue;

			const std::size_t neighborIndex = tileIndex(map, neighbor);
			if (visited[neighborIndex])
				continue;

			visited[neighborIndex] = true;
			previous[neighborIndex] = static_cast<int>(tileIndex(map, current));
			if (sameTile(neighbor, destination))
				return BuildPath(map, startTile, destination, previous);

			frontier.push(neighbor);
		}
	}

	return Path(NavigationPathStatus::NoPath);
}

} // namespace iggy::navigation
