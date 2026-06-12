#include "servers/navigation/NavigationGridPathfinder.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <queue>
#include <vector>

namespace iggy::navigation {

namespace {

NavigationPath Path(NavigationPathStatus status)
{
	NavigationPath path;
	path.status = status;
	return path;
}

NavigationPathTile TileForPoint(Vec2 point)
{
	return { static_cast<int>(std::floor(point.x)), static_cast<int>(std::floor(point.y)) };
}

Vec2 TileCenter(NavigationPathTile tile)
{
	return { static_cast<float>(tile.x) + 0.5F, static_cast<float>(tile.y) + 0.5F };
}

bool SameTile(NavigationPathTile left, NavigationPathTile right)
{
	return left.x == right.x && left.y == right.y;
}

std::size_t TileIndex(const LevelTileMap &map, int x, int y)
{
	return static_cast<std::size_t>(y) * static_cast<std::size_t>(map.width) + static_cast<std::size_t>(x);
}

bool Walkable(const LevelTileMap &map, int x, int y)
{
	const LevelTile *tile = map.tileAt(x, y);
	return tile != nullptr && tile->walkable;
}

NavigationPath BuildPath(const LevelTileMap &map, NavigationPathTile start, NavigationPathTile destination, const std::vector<int> &previous)
{
	NavigationPath path;
	path.status = NavigationPathStatus::Found;

	for (NavigationPathTile cursor = destination;;) {
		path.tiles.push_back(cursor);
		if (SameTile(cursor, start))
			break;

		const int previousIndex = previous[TileIndex(map, cursor.x, cursor.y)];
		cursor = { previousIndex % map.width, previousIndex / map.width };
	}

	std::reverse(path.tiles.begin(), path.tiles.end());
	path.waypoints.reserve(path.tiles.size());
	for (NavigationPathTile tile : path.tiles)
		path.waypoints.push_back(TileCenter(tile));
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

	const NavigationPathTile startTile = TileForPoint(start);
	if (!map.contains(startTile.x, startTile.y))
		return Path(NavigationPathStatus::StartOutOfBounds);
	if (!Walkable(map, startTile.x, startTile.y))
		return Path(NavigationPathStatus::StartBlocked);

	const NavigationPathTile destination { request.destinationTileX, request.destinationTileY };
	if (SameTile(startTile, destination)) {
		NavigationPath path;
		path.status = NavigationPathStatus::Found;
		path.tiles.push_back(startTile);
		path.waypoints.push_back(TileCenter(startTile));
		return path;
	}

	const std::size_t tileCount = static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height);
	std::vector<bool> visited(tileCount, false);
	std::vector<int> previous(tileCount, -1);
	std::queue<NavigationPathTile> frontier;

	frontier.push(startTile);
	visited[TileIndex(map, startTile.x, startTile.y)] = true;

	const NavigationPathTile neighbors[] = {
		{ 1, 0 },
		{ 0, 1 },
		{ -1, 0 },
		{ 0, -1 },
	};

	while (!frontier.empty()) {
		const NavigationPathTile current = frontier.front();
		frontier.pop();

		for (NavigationPathTile neighborOffset : neighbors) {
			const NavigationPathTile neighbor { current.x + neighborOffset.x, current.y + neighborOffset.y };
			if (!map.contains(neighbor.x, neighbor.y) || !Walkable(map, neighbor.x, neighbor.y))
				continue;

			const std::size_t neighborIndex = TileIndex(map, neighbor.x, neighbor.y);
			if (visited[neighborIndex])
				continue;

			visited[neighborIndex] = true;
			previous[neighborIndex] = static_cast<int>(TileIndex(map, current.x, current.y));
			if (SameTile(neighbor, destination))
				return BuildPath(map, startTile, destination, previous);

			frontier.push(neighbor);
		}
	}

	return Path(NavigationPathStatus::NoPath);
}

} // namespace iggy::navigation
