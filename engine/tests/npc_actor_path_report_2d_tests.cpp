#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorPathReport2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameTile;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	return iggy::test::MapFromRows(rows);
}

bool SameMap(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	return actual.id == expected.id
		&& actual.width == expected.width
		&& actual.height == expected.height
		&& actual.tiles.size() == expected.tiles.size()
		&& actual.entitySpawns.size() == expected.entitySpawns.size();
}

iggy::NpcActorRouteTarget2D ReadyRoute(
	const char *npcId,
	iggy::Vec2 start,
	iggy::Vec2 target)
{
	iggy::NpcActorMovementIntent2D intent;
	intent.status = iggy::NpcActorMovementIntent2DStatus::Ready;
	intent.type = iggy::NpcActorMovementIntent2DType::MoveTo;
	intent.npcId = Id(npcId);
	intent.startPosition = start;
	intent.targetPosition = target;
	intent.moveMode = iggy::NpcMoveMode::Walk;
	intent.speedMultiplier = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Walk);
	intent.requestsMovement = true;

	iggy::NpcActorRouteTarget2D route;
	route.intent = intent;
	route.status = iggy::NpcActorRouteTarget2DStatus::Ready;
	route.type = iggy::NpcActorRouteTarget2DType::MoveTo;
	route.npcId = intent.npcId;
	route.startPosition = start;
	route.targetPosition = target;
	route.moveMode = intent.moveMode;
	route.speedMultiplier = intent.speedMultiplier;
	route.requestsRoute = true;
	return route;
}

iggy::NpcActorNavigationRequest2D NavigationFor(
	const iggy::LevelTileMap &map,
	iggy::Vec2 start,
	iggy::Vec2 target,
	const char *npcId = "npc:runner")
{
	return iggy::NpcActorNavigationRequestBuilder2D {}.build(
		ReadyRoute(npcId, start, target),
		map);
}

void TestNoNavigationRequestDoesNotRequestStep()
{
	const iggy::LevelTileMap map = MapFromRows({ "..." });
	iggy::NpcActorNavigationRequest2D navigation;
	navigation.status = iggy::NpcActorNavigationRequest2DStatus::NoRouteTarget;
	navigation.requestsPath = false;

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::NoNavigationRequest, "no navigation request should map to NoNavigationRequest");
	Expect(!result.hasPath(), "no navigation request should not have path");
	Expect(!result.requestsStep, "no navigation request should not request step");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::NoPath, "no navigation request should keep default path diagnostic");
	Expect(result.navigation.status == navigation.status, "path report should preserve copied navigation status");
}

void TestNavigationRejectedMapsToNoNavigationRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 1.5F, 1.5F });

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(navigation.status == iggy::NpcActorNavigationRequest2DStatus::NavigationRejected, "navigation setup should reject blocked destination");
	Expect(result.status == iggy::NpcActorPathReport2DStatus::NoNavigationRequest, "rejected navigation should not run pathfinder");
	Expect(!result.requestsStep, "rejected navigation should not request step");
	Expect(result.navigation.request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "path report should preserve rejected navigation diagnostics");
}

void TestAcceptedWalkableRouteFindsPathAndPreservesWaypoints()
{
	const iggy::LevelTileMap map = MapFromRows({
		"....",
		"....",
	});
	const iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { 0.5F, 1.5F }, { 3.5F, 1.5F }, "npc:runner");

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::PathFound, "accepted walkable route should find path");
	Expect(result.hasPath(), "found path should report hasPath");
	Expect(result.requestsStep, "found path should request step");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::Found, "path report should preserve found path status");
	Expect(result.path.tiles.size() == 4, "straight route should preserve path tiles");
	if (result.path.tiles.size() == 4) {
		Expect(SameTile(result.path.tiles.front(), 0, 1), "path report should preserve start tile");
		Expect(SameTile(result.path.tiles.back(), 3, 1), "path report should preserve destination tile");
	}
	Expect(result.path.waypoints.size() == result.path.tiles.size(), "path report should preserve waypoints");
	if (!result.path.waypoints.empty()) {
		Expect(NearVec(result.path.waypoints.front(), { 0.5F, 1.5F }), "path waypoint should preserve start tile center");
		Expect(NearVec(result.path.waypoints.back(), { 3.5F, 1.5F }), "path waypoint should preserve destination tile center");
	}
	Expect(result.navigation.route.npcId == Id("npc:runner"), "path report should preserve copied route npc id");
}

void TestNoPathMapsToPathNotFound()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#..",
		"..#..",
		"..#..",
	});
	const iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { 0.5F, 1.5F }, { 4.5F, 1.5F });

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::PathNotFound, "unreachable route should map to PathNotFound");
	Expect(!result.hasPath(), "unreachable route should not have path");
	Expect(!result.requestsStep, "unreachable route should not request step");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::NoPath, "unreachable route should preserve NoPath diagnostic");
	Expect(result.path.tiles.empty(), "unreachable route should preserve empty path tiles");
}

void TestStartBlockedMapsToPathNotFound()
{
	const iggy::LevelTileMap map = MapFromRows({
		"#..",
		"...",
	});
	const iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 2.5F, 0.5F });

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::PathNotFound, "blocked start should map to PathNotFound");
	Expect(!result.requestsStep, "blocked start should not request step");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::StartBlocked, "blocked start should preserve path diagnostic");
}

void TestStartOutOfBoundsMapsToPathNotFound()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { -0.5F, 0.5F }, { 2.5F, 0.5F });

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::PathNotFound, "out-of-bounds start should map to PathNotFound");
	Expect(!result.requestsStep, "out-of-bounds start should not request step");
	Expect(result.path.status == iggy::navigation::NavigationPathStatus::StartOutOfBounds, "out-of-bounds start should preserve path diagnostic");
}

void TestCopiedNavigationRouteFactsArePreserved()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 2.5F, 1.5F }, "runner");

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::PathFound, "copied navigation setup should find path");
	Expect(result.navigation.status == navigation.status, "path report should preserve navigation status");
	Expect(result.navigation.request.status == navigation.request.status, "path report should preserve navigation request status");
	Expect(result.navigation.request.destinationTileX == navigation.request.destinationTileX, "path report should preserve destination tile x");
	Expect(result.navigation.request.destinationTileY == navigation.request.destinationTileY, "path report should preserve destination tile y");
	Expect(result.navigation.route.npcId == Id("runner"), "path report should preserve unqualified route npc id");
	Expect(result.navigation.route.moveMode == iggy::NpcMoveMode::Walk, "path report should preserve route move mode");
	Expect(NearVec(result.navigation.route.startPosition, navigation.route.startPosition), "path report should preserve route start");
	Expect(NearVec(result.navigation.route.targetPosition, navigation.route.targetPosition), "path report should preserve route target");
}

void TestInputNavigationAndMapAreNotMutated()
{
	iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelTileMap mapBefore = map;
	iggy::NpcActorNavigationRequest2D navigation =
		NavigationFor(map, { 0.5F, 0.5F }, { 2.5F, 1.5F });
	const iggy::NpcActorNavigationRequest2DStatus statusBefore = navigation.status;
	const iggy::navigation::NavigationRequestStatus requestStatusBefore = navigation.request.status;
	const int tileXBefore = navigation.request.destinationTileX;
	const int tileYBefore = navigation.request.destinationTileY;
	const iggy::Vec2 startBefore = navigation.route.startPosition;
	const iggy::Vec2 targetBefore = navigation.route.targetPosition;
	const bool requestsPathBefore = navigation.requestsPath;

	const iggy::NpcActorPathReport2D result =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);

	Expect(result.status == iggy::NpcActorPathReport2DStatus::PathFound, "immutability setup should find path");
	Expect(navigation.status == statusBefore, "path reporter should not mutate navigation status");
	Expect(navigation.request.status == requestStatusBefore, "path reporter should not mutate request status");
	Expect(navigation.request.destinationTileX == tileXBefore, "path reporter should not mutate destination tile x");
	Expect(navigation.request.destinationTileY == tileYBefore, "path reporter should not mutate destination tile y");
	Expect(NearVec(navigation.route.startPosition, startBefore), "path reporter should not mutate route start");
	Expect(NearVec(navigation.route.targetPosition, targetBefore), "path reporter should not mutate route target");
	Expect(navigation.requestsPath == requestsPathBefore, "path reporter should not mutate requestsPath");
	Expect(SameMap(map, mapBefore), "path reporter should not mutate map");
}

} // namespace

int main()
{
	TestNoNavigationRequestDoesNotRequestStep();
	TestNavigationRejectedMapsToNoNavigationRequest();
	TestAcceptedWalkableRouteFindsPathAndPreservesWaypoints();
	TestNoPathMapsToPathNotFound();
	TestStartBlockedMapsToPathNotFound();
	TestStartOutOfBoundsMapsToPathNotFound();
	TestCopiedNavigationRouteFactsArePreserved();
	TestInputNavigationAndMapAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
