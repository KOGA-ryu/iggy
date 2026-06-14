#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/ai/NpcAiPathReport2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

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

bool SameTile(iggy::TileCoord tile, int x, int y)
{
	return tile.x == x && tile.y == y;
}

iggy::NpcAiRouteRequest2DResult Route(
	iggy::NpcAiRouteRequest2DStatus status,
	iggy::Vec2 start,
	iggy::Vec2 target)
{
	iggy::NpcAiRouteRequest2DResult route;
	route.status = status;
	route.startPosition = start;
	route.targetPosition = target;
	route.intentType = iggy::NpcAiBehaviorIntent2DType::Patrol;
	route.requestsRoute = status == iggy::NpcAiRouteRequest2DStatus::Requested;
	return route;
}

iggy::NpcAiNavigationRequest2DResult NavigationFor(
	const iggy::LevelTileMap &map,
	iggy::Vec2 start,
	iggy::Vec2 target)
{
	return iggy::NpcAiNavigationRequestBuilder2D {}.build(
		Route(iggy::NpcAiRouteRequest2DStatus::Requested, start, target),
		map);
}

void TestNoNavigationRequestProducesNoNavigationRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcAiNavigationRequest2DResult navigation =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(
			Route(iggy::NpcAiRouteRequest2DStatus::HoldPosition, { 0.5F, 0.5F }, { 0.5F, 0.5F }),
			map);

	const iggy::NpcAiPathReport2DResult result =
		iggy::NpcAiPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcAiPathReport2DStatus::NoNavigationRequest, "no navigation request should not run pathfinding");
	Expect(!result.hasPath(), "no navigation request should not have path");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::NoPath, "no navigation request should keep default path diagnostics");
	Expect(result.navigation.status == navigation.status, "path report should preserve copied no-request navigation result");
}

void TestInvalidNavigationRequestProducesNoNavigationRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::NpcAiNavigationRequest2DResult navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 1.5F, 1.5F });

	const iggy::NpcAiPathReport2DResult result =
		iggy::NpcAiPathReporter2D {}.findPath(navigation, map);

	Expect(navigation.status == iggy::NpcAiNavigationRequest2DStatus::NavigationRequestInvalid, "invalid navigation setup should reject request");
	Expect(result.status == iggy::NpcAiPathReport2DStatus::NoNavigationRequest, "invalid navigation request should not run pathfinding");
	Expect(!result.hasPath(), "invalid navigation request should not have path");
	Expect(result.navigation.request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "path report should preserve invalid navigation diagnostic");
}

void TestValidWalkableRouteFindsPathAndPreservesEndpoints()
{
	const iggy::LevelTileMap map = MapFromRows({
		"....",
		"....",
	});
	const iggy::NpcAiNavigationRequest2DResult navigation =
		NavigationFor(map, { 0.5F, 1.5F }, { 3.5F, 1.5F });

	const iggy::NpcAiPathReport2DResult result =
		iggy::NpcAiPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcAiPathReport2DStatus::PathFound, "accepted navigation request should find straight path");
	Expect(result.hasPath(), "found path should report hasPath");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::Found, "path report should preserve found path status");
	Expect(result.path.tiles.size() == 4, "straight path should preserve path tiles");
	if (result.path.tiles.size() == 4) {
		Expect(SameTile(result.path.tiles.front(), 0, 1), "path should preserve start tile");
		Expect(SameTile(result.path.tiles.back(), 3, 1), "path should preserve destination tile");
	}
	Expect(result.path.waypoints.size() == result.path.tiles.size(), "path report should preserve path waypoints");
}

void TestNoPathMapsToPathNotFound()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#..",
		"..#..",
		"..#..",
	});
	const iggy::NpcAiNavigationRequest2DResult navigation =
		NavigationFor(map, { 0.5F, 1.5F }, { 4.5F, 1.5F });

	const iggy::NpcAiPathReport2DResult result =
		iggy::NpcAiPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcAiPathReport2DStatus::PathNotFound, "unreachable destination should map to PathNotFound");
	Expect(!result.hasPath(), "unreachable destination should not have path");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::NoPath, "unreachable destination should preserve NoPath diagnostic");
	Expect(result.path.tiles.empty(), "unreachable destination should preserve empty path tiles");
}

void TestStartFailureMapsToPathNotFound()
{
	const iggy::LevelTileMap blockedStartMap = MapFromRows({
		"#..",
		"...",
	});
	const iggy::NpcAiNavigationRequest2DResult blockedStart =
		NavigationFor(blockedStartMap, { 0.5F, 0.5F }, { 2.5F, 0.5F });
	const iggy::NpcAiPathReport2DResult blockedStartResult =
		iggy::NpcAiPathReporter2D {}.findPath(blockedStart, blockedStartMap);

	const iggy::LevelTileMap openMap = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcAiNavigationRequest2DResult outOfBoundsStart =
		NavigationFor(openMap, { -0.5F, 0.5F }, { 2.5F, 0.5F });
	const iggy::NpcAiPathReport2DResult outOfBoundsStartResult =
		iggy::NpcAiPathReporter2D {}.findPath(outOfBoundsStart, openMap);

	Expect(blockedStartResult.status == iggy::NpcAiPathReport2DStatus::PathNotFound, "blocked start should map to PathNotFound");
	Expect(blockedStartResult.path.status == iggy::navigation::NavigationPathStatus::StartBlocked, "blocked start should preserve path diagnostic");
	Expect(outOfBoundsStartResult.status == iggy::NpcAiPathReport2DStatus::PathNotFound, "out-of-bounds start should map to PathNotFound");
	Expect(outOfBoundsStartResult.path.status == iggy::navigation::NavigationPathStatus::StartOutOfBounds, "out-of-bounds start should preserve path diagnostic");
}

void TestCopiedNavigationResultIsPreserved()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcAiNavigationRequest2DResult navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 2.5F, 1.5F });

	const iggy::NpcAiPathReport2DResult result =
		iggy::NpcAiPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcAiPathReport2DStatus::PathFound, "copied navigation setup should find path");
	Expect(result.navigation.status == navigation.status, "path report should preserve navigation status");
	Expect(result.navigation.request.status == navigation.request.status, "path report should preserve navigation request status");
	Expect(result.navigation.request.destinationTileX == navigation.request.destinationTileX, "path report should preserve destination tile x");
	Expect(result.navigation.request.destinationTileY == navigation.request.destinationTileY, "path report should preserve destination tile y");
	Expect(NearVec(result.navigation.route.startPosition, navigation.route.startPosition), "path report should preserve route start");
	Expect(NearVec(result.navigation.route.targetPosition, navigation.route.targetPosition), "path report should preserve route target");
}

void TestInputNavigationResultIsNotMutated()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	iggy::NpcAiNavigationRequest2DResult navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 2.5F, 1.5F });
	const iggy::NpcAiNavigationRequest2DStatus statusBefore = navigation.status;
	const iggy::navigation::NavigationRequestStatus requestStatusBefore = navigation.request.status;
	const int tileXBefore = navigation.request.destinationTileX;
	const int tileYBefore = navigation.request.destinationTileY;
	const iggy::Vec2 startBefore = navigation.route.startPosition;
	const iggy::Vec2 targetBefore = navigation.route.targetPosition;

	const iggy::NpcAiPathReport2DResult result =
		iggy::NpcAiPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcAiPathReport2DStatus::PathFound, "immutability setup should find path");
	Expect(navigation.status == statusBefore, "path reporter should not mutate navigation status");
	Expect(navigation.request.status == requestStatusBefore, "path reporter should not mutate navigation request status");
	Expect(navigation.request.destinationTileX == tileXBefore, "path reporter should not mutate destination tile x");
	Expect(navigation.request.destinationTileY == tileYBefore, "path reporter should not mutate destination tile y");
	Expect(NearVec(navigation.route.startPosition, startBefore), "path reporter should not mutate route start");
	Expect(NearVec(navigation.route.targetPosition, targetBefore), "path reporter should not mutate route target");
}

} // namespace

int main()
{
	TestNoNavigationRequestProducesNoNavigationRequest();
	TestInvalidNavigationRequestProducesNoNavigationRequest();
	TestValidWalkableRouteFindsPathAndPreservesEndpoints();
	TestNoPathMapsToPathNotFound();
	TestStartFailureMapsToPathNotFound();
	TestCopiedNavigationResultIsPreserved();
	TestInputNavigationResultIsNotMutated();

	return Failures;
}
