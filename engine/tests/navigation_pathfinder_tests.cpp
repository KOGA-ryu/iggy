#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "servers/navigation/NavigationGridPathfinder.hpp"
#include "servers/navigation/NavigationGridValidator.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

iggy::navigation::NavigationRequest RequestFor(const iggy::LevelTileMap &map, iggy::Vec2 destination)
{
	const iggy::npc_ai::NpcMovementPlan plan { iggy::npc_ai::NpcMovementPlanType::MoveTo, destination };
	return iggy::navigation::NavigationGridValidator {}.validate(map, plan);
}

bool SameTile(iggy::navigation::NavigationPathTile tile, int x, int y)
{
	return tile.x == x && tile.y == y;
}

void TestSameTilePath()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { 1.2F, 0.2F }, RequestFor(map, { 1.8F, 0.8F }));

	Expect(path.status == iggy::navigation::NavigationPathStatus::Found, "same-tile path should be found");
	Expect(path.found(), "same-tile path should report found");
	Expect(path.tiles.size() == 1, "same-tile path should include one tile");
	Expect(SameTile(path.tiles[0], 1, 0), "same-tile path should preserve start and destination tile");
	Expect(path.waypoints.size() == 1 && path.waypoints[0] == iggy::Vec2 { 1.5F, 0.5F }, "same-tile path should use tile center waypoint");
}

void TestStraightPath()
{
	const iggy::LevelTileMap map = MapFromRows({
		"....",
		"....",
	});
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { 0.5F, 1.5F }, RequestFor(map, { 3.5F, 1.5F }));

	Expect(path.status == iggy::navigation::NavigationPathStatus::Found, "straight path should be found");
	Expect(path.tiles.size() == 4, "straight path should include every crossed tile");
	Expect(SameTile(path.tiles.front(), 0, 1), "straight path should start at start tile");
	Expect(SameTile(path.tiles[1], 1, 1), "straight path should move horizontally in deterministic order");
	Expect(SameTile(path.tiles[2], 2, 1), "straight path should continue horizontally");
	Expect(SameTile(path.tiles.back(), 3, 1), "straight path should end at destination tile");
}

void TestBlockedStart()
{
	const iggy::LevelTileMap map = MapFromRows({
		"#..",
		"...",
	});
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { 0.5F, 0.5F }, RequestFor(map, { 2.5F, 0.5F }));

	Expect(path.status == iggy::navigation::NavigationPathStatus::StartBlocked, "blocked start should return StartBlocked");
	Expect(path.tiles.empty(), "blocked start should not produce path tiles");
}

void TestOutOfBoundsStart()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { -0.5F, 0.5F }, RequestFor(map, { 2.5F, 0.5F }));

	Expect(path.status == iggy::navigation::NavigationPathStatus::StartOutOfBounds, "out-of-bounds start should return StartOutOfBounds");
	Expect(path.tiles.empty(), "out-of-bounds start should not produce path tiles");
}

void TestBlockedDestinationDoesNotProducePath()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::navigation::NavigationRequest request = RequestFor(map, { 1.5F, 1.5F });
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { 0.5F, 1.5F }, request);

	Expect(request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "test setup should reject blocked destination");
	Expect(path.status == iggy::navigation::NavigationPathStatus::DestinationRejected, "blocked destination should not produce path");
	Expect(path.tiles.empty(), "blocked destination should not produce path tiles");
}

void TestNoPathThroughFullWall()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#..",
		"..#..",
		"..#..",
	});
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { 0.5F, 1.5F }, RequestFor(map, { 4.5F, 1.5F }));

	Expect(path.status == iggy::navigation::NavigationPathStatus::NoPath, "full wall should prevent path");
	Expect(path.tiles.empty(), "no path should not produce path tiles");
}

void TestFindsRouteAroundSingleBlocker()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	const iggy::navigation::NavigationPath path = iggy::navigation::NavigationGridPathfinder {}.findPath(map, { 0.5F, 1.5F }, RequestFor(map, { 4.5F, 1.5F }));

	Expect(path.status == iggy::navigation::NavigationPathStatus::Found, "pathfinder should route around single blocker");
	Expect(SameTile(path.tiles.front(), 0, 1), "route should preserve start tile");
	Expect(SameTile(path.tiles.back(), 4, 1), "route should preserve destination tile");
	Expect(path.tiles.size() == 7, "route around one blocker should use shortest 4-way route");
	Expect(SameTile(path.tiles[1], 1, 1), "route should begin toward destination");
	Expect(SameTile(path.tiles[2], 1, 2), "route should use deterministic down-first detour after blocked right neighbor");
	Expect(SameTile(path.tiles[3], 2, 2), "route should pass below blocker");
	Expect(SameTile(path.tiles[4], 3, 2), "route should continue below blocker");
	Expect(SameTile(path.tiles[5], 4, 2), "route should continue to destination column before returning to row");
	Expect(SameTile(path.tiles[6], 4, 1), "route should return to destination row");
}

} // namespace

int main()
{
	TestSameTilePath();
	TestStraightPath();
	TestBlockedStart();
	TestOutOfBoundsStart();
	TestBlockedDestinationDoesNotProducePath();
	TestNoPathThroughFullWall();
	TestFindsRouteAroundSingleBlocker();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
