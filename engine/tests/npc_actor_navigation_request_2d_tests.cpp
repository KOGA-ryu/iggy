#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"
#include "scene/npc/NpcActorNavigationRequest2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

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

iggy::NpcActorMovementIntent2D ReadyMoveToIntent(
	const char *npcId = "npc:runner",
	iggy::Vec2 start = { 0.5F, 0.5F },
	iggy::Vec2 target = { 1.5F, 0.5F })
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
	return intent;
}

iggy::NpcActorMovementIntent2D ReadyMoveAwayFromIntent(
	const char *npcId = "npc:fleeing",
	iggy::Vec2 start = { 1.5F, 1.5F },
	iggy::Vec2 threat = { 0.5F, 1.5F })
{
	iggy::NpcActorMovementIntent2D intent;
	intent.status = iggy::NpcActorMovementIntent2DStatus::Ready;
	intent.type = iggy::NpcActorMovementIntent2DType::MoveAwayFrom;
	intent.npcId = Id(npcId);
	intent.startPosition = start;
	intent.targetPosition = threat;
	intent.moveMode = iggy::NpcMoveMode::Run;
	intent.speedMultiplier = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Run);
	intent.requestsMovement = true;
	return intent;
}

iggy::NpcActorRouteTarget2D ReadyRoute(
	const char *npcId = "npc:runner",
	iggy::Vec2 start = { 0.5F, 0.5F },
	iggy::Vec2 target = { 1.5F, 0.5F },
	iggy::NpcActorRouteTarget2DType type = iggy::NpcActorRouteTarget2DType::MoveTo)
{
	iggy::NpcActorRouteTarget2D route;
	route.intent = ReadyMoveToIntent(npcId, start, target);
	route.status = iggy::NpcActorRouteTarget2DStatus::Ready;
	route.type = type;
	route.npcId = Id(npcId);
	route.startPosition = start;
	route.targetPosition = target;
	route.moveMode = route.intent.moveMode;
	route.speedMultiplier = route.intent.speedMultiplier;
	route.requestsRoute = true;
	return route;
}

void TestNonReadyRouteTargetReturnsNoRouteTarget()
{
	const iggy::LevelTileMap map = MapFromRows({ "..." });
	iggy::NpcActorRouteTarget2D route = ReadyRoute();
	route.status = iggy::NpcActorRouteTarget2DStatus::AlreadyAtTarget;
	route.requestsRoute = false;

	const iggy::NpcActorNavigationRequest2D result =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcActorNavigationRequest2DStatus::NoRouteTarget, "non-ready route should return NoRouteTarget");
	Expect(!result.ready(), "non-ready route should not produce ready navigation request");
	Expect(!result.requestsPath, "non-ready route should not request path");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::None, "non-ready route should not call navigation validation");
	Expect(result.route.status == iggy::NpcActorRouteTarget2DStatus::AlreadyAtTarget, "non-ready result should preserve copied route status");
}

void TestReadyWalkableRouteBuildsAcceptedNavigationRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcActorRouteTarget2D route =
		ReadyRoute("npc:runner", { 0.5F, 0.5F }, { 2.5F, 1.5F });

	const iggy::NpcActorNavigationRequest2D result =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "walkable route should build ready navigation request");
	Expect(result.ready(), "walkable route should report ready helper");
	Expect(result.requestsPath, "walkable route should request path");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::Accepted, "walkable route should preserve accepted navigation status");
	Expect(result.request.accepted(), "walkable route should preserve accepted request helper");
	Expect(result.request.destination.has_value(), "walkable route should preserve destination");
	if (result.request.destination.has_value()) {
		Expect(NearVec(*result.request.destination, route.targetPosition), "accepted request should preserve route target destination");
	}
	Expect(result.request.destinationTileX == 2 && result.request.destinationTileY == 1, "accepted request should preserve destination tile diagnostics");
	Expect(result.route.npcId == Id("npc:runner"), "navigation result should preserve route npc id");
	Expect(NearVec(result.route.startPosition, route.startPosition), "navigation result should preserve route start");
	Expect(NearVec(result.route.targetPosition, route.targetPosition), "navigation result should preserve route target");
	Expect(result.route.moveMode == route.moveMode, "navigation result should preserve move mode");
}

void TestBlockedDestinationReturnsNavigationRejected()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::NpcActorRouteTarget2D route =
		ReadyRoute("npc:runner", { 0.5F, 0.5F }, { 1.5F, 1.5F });

	const iggy::NpcActorNavigationRequest2D result =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcActorNavigationRequest2DStatus::NavigationRejected, "blocked route should reject navigation request");
	Expect(!result.ready(), "blocked route should not be ready");
	Expect(!result.requestsPath, "blocked route should not request path");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "blocked route should preserve blocked navigation diagnostic");
	Expect(result.request.destination.has_value(), "blocked route should preserve attempted destination");
	if (result.request.destination.has_value()) {
		Expect(NearVec(*result.request.destination, route.targetPosition), "blocked request should preserve route target");
	}
	Expect(result.request.destinationTileX == 1 && result.request.destinationTileY == 1, "blocked route should preserve destination tile diagnostics");
}

void TestOutOfBoundsDestinationReturnsNavigationRejected()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcActorRouteTarget2D route =
		ReadyRoute("npc:runner", { 0.5F, 0.5F }, { 3.5F, 1.5F });

	const iggy::NpcActorNavigationRequest2D result =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcActorNavigationRequest2DStatus::NavigationRejected, "out-of-bounds route should reject navigation request");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::DestinationOutOfBounds, "out-of-bounds route should preserve diagnostic");
	Expect(result.request.destinationTileX == 3 && result.request.destinationTileY == 1, "out-of-bounds route should preserve destination tile diagnostics");
}

void TestNestedEscapeRouteCanBuildNavigationRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
		"...",
	});
	const iggy::NpcActorMovementIntent2D intent =
		ReadyMoveAwayFromIntent("npc:fleeing", { 1.5F, 1.5F }, { 0.5F, 1.5F });
	const iggy::NpcActorEscapeRouteTarget2D escapeRoute =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, map);

	const iggy::NpcActorNavigationRequest2D result =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(escapeRoute.route, map);

	Expect(escapeRoute.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "escape route setup should produce ready route");
	Expect(result.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "ready nested escape route should build navigation request");
	Expect(result.requestsPath, "ready nested escape route should request path");
	Expect(result.route.type == iggy::NpcActorRouteTarget2DType::MoveAwayFrom, "nested route should preserve MoveAwayFrom route type");
	Expect(result.route.npcId == Id("npc:fleeing"), "nested route should preserve npc id");
	Expect(result.request.destination.has_value(), "nested route request should preserve destination");
	if (result.request.destination.has_value()) {
		Expect(NearVec(*result.request.destination, escapeRoute.route.targetPosition), "nested route request should use selected escape destination");
	}
}

void TestCopiedRouteAndMapInputsAreNotMutated()
{
	iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelTileMap mapBefore = map;
	iggy::NpcActorRouteTarget2D route =
		ReadyRoute("npc:runner", { 0.5F, 0.5F }, { 2.5F, 1.5F });
	const iggy::NpcActorRouteTarget2DStatus routeStatusBefore = route.status;
	const iggy::NpcActorRouteTarget2DType routeTypeBefore = route.type;
	const iggy::ResourceId npcIdBefore = route.npcId;
	const iggy::Vec2 startBefore = route.startPosition;
	const iggy::Vec2 targetBefore = route.targetPosition;
	const iggy::NpcMoveMode moveModeBefore = route.moveMode;
	const bool requestsRouteBefore = route.requestsRoute;

	const iggy::NpcActorNavigationRequest2D result =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "immutability setup should build ready request");
	Expect(route.status == routeStatusBefore, "navigation builder should not mutate route status");
	Expect(route.type == routeTypeBefore, "navigation builder should not mutate route type");
	Expect(route.npcId == npcIdBefore, "navigation builder should not mutate route npc id");
	Expect(NearVec(route.startPosition, startBefore), "navigation builder should not mutate route start");
	Expect(NearVec(route.targetPosition, targetBefore), "navigation builder should not mutate route target");
	Expect(route.moveMode == moveModeBefore, "navigation builder should not mutate route move mode");
	Expect(route.requestsRoute == requestsRouteBefore, "navigation builder should not mutate route request flag");
	Expect(SameMap(map, mapBefore), "navigation builder should not mutate map");
}

void TestNamespacedAndUnqualifiedNpcIdsRemainDistinct()
{
	const iggy::LevelTileMap map = MapFromRows({ "..." });
	const iggy::NpcActorRouteTarget2D namespaced =
		ReadyRoute("npc:runner", { 0.5F, 0.5F }, { 1.5F, 0.5F });
	const iggy::NpcActorRouteTarget2D unqualified =
		ReadyRoute("runner", { 0.5F, 0.5F }, { 2.5F, 0.5F });

	const iggy::NpcActorNavigationRequest2D namespacedResult =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(namespaced, map);
	const iggy::NpcActorNavigationRequest2D unqualifiedResult =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(unqualified, map);

	Expect(namespacedResult.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "namespaced route should build");
	Expect(unqualifiedResult.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "unqualified route should build");
	Expect(namespacedResult.route.npcId == Id("npc:runner"), "namespaced npc id should be preserved");
	Expect(unqualifiedResult.route.npcId == Id("runner"), "unqualified npc id should be preserved");
	Expect(namespacedResult.route.npcId != unqualifiedResult.route.npcId, "namespaced and unqualified npc ids should remain distinct");
}

} // namespace

int main()
{
	TestNonReadyRouteTargetReturnsNoRouteTarget();
	TestReadyWalkableRouteBuildsAcceptedNavigationRequest();
	TestBlockedDestinationReturnsNavigationRejected();
	TestOutOfBoundsDestinationReturnsNavigationRejected();
	TestNestedEscapeRouteCanBuildNavigationRequest();
	TestCopiedRouteAndMapInputsAreNotMutated();
	TestNamespacedAndUnqualifiedNpcIdsRemainDistinct();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
