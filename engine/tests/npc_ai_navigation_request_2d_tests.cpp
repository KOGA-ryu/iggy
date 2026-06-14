#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/ai/NpcAiNavigationRequest2D.hpp"
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

iggy::NpcAiRouteRequest2DResult Route(
	iggy::NpcAiRouteRequest2DStatus status,
	iggy::Vec2 start = { 0.5F, 0.5F },
	iggy::Vec2 target = { 1.5F, 0.5F },
	iggy::NpcAiBehaviorIntent2DType intent = iggy::NpcAiBehaviorIntent2DType::Patrol)
{
	iggy::NpcAiRouteRequest2DResult route;
	route.status = status;
	route.startPosition = start;
	route.targetPosition = target;
	route.intentType = intent;
	route.requestsRoute = status == iggy::NpcAiRouteRequest2DStatus::Requested;
	route.decision.status = route.requestsRoute
		? iggy::NpcAiDecision2DStatus::Decided
		: iggy::NpcAiDecision2DStatus::NoIntentTarget;
	route.decision.intent.type = intent;
	route.decision.target.selectedPosition = target;
	route.decision.score.currentStateValidation.state.position = start;
	return route;
}

void ExpectNoRoute(const iggy::NpcAiRouteRequest2DResult &route, const char *message)
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});

	const iggy::NpcAiNavigationRequest2DResult result =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcAiNavigationRequest2DStatus::NoRouteRequest, message);
	Expect(!result.hasNavigationRequest(), message);
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::None, message);
	Expect(result.route.status == route.status, message);
}

void TestNoRouteRequestStatusesMapToNoRouteRequest()
{
	ExpectNoRoute(Route(iggy::NpcAiRouteRequest2DStatus::NoDecision), "NoDecision route should not build navigation request");
	ExpectNoRoute(Route(iggy::NpcAiRouteRequest2DStatus::HoldPosition), "HoldPosition route should not build navigation request");
	ExpectNoRoute(Route(iggy::NpcAiRouteRequest2DStatus::NoMovementNeeded), "NoMovementNeeded route should not build navigation request");
}

void TestRequestedRouteBuildsAcceptedNavigationRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcAiRouteRequest2DResult route =
		Route(iggy::NpcAiRouteRequest2DStatus::Requested, { 0.5F, 0.5F }, { 2.5F, 1.5F });

	const iggy::NpcAiNavigationRequest2DResult result =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcAiNavigationRequest2DStatus::Built, "accepted route should build navigation request");
	Expect(result.hasNavigationRequest(), "accepted route should report navigation request");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::Accepted, "accepted route should preserve navigation accepted status");
	Expect(result.request.accepted(), "accepted route request should report accepted");
	Expect(result.request.destination.has_value(), "accepted route should preserve destination");
	if (result.request.destination.has_value())
		Expect(NearVec(*result.request.destination, route.targetPosition), "accepted navigation request should preserve route target as destination");
	Expect(result.request.destinationTileX == 2 && result.request.destinationTileY == 1, "accepted navigation request should report destination tile");
	Expect(NearVec(result.route.startPosition, route.startPosition), "navigation result should preserve route start position");
	Expect(NearVec(result.route.targetPosition, route.targetPosition), "navigation result should preserve route target position");
}

void TestBlockedDestinationMapsToNavigationRequestInvalid()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::NpcAiRouteRequest2DResult route =
		Route(iggy::NpcAiRouteRequest2DStatus::Requested, { 0.5F, 0.5F }, { 1.5F, 1.5F });

	const iggy::NpcAiNavigationRequest2DResult result =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcAiNavigationRequest2DStatus::NavigationRequestInvalid, "blocked destination should map to invalid navigation request");
	Expect(!result.hasNavigationRequest(), "blocked destination should not report usable navigation request");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "blocked destination should preserve navigation diagnostic");
	Expect(result.request.destination.has_value(), "blocked destination should preserve attempted destination");
	if (result.request.destination.has_value())
		Expect(NearVec(*result.request.destination, route.targetPosition), "blocked destination should preserve route target");
	Expect(result.request.destinationTileX == 1 && result.request.destinationTileY == 1, "blocked destination should preserve destination tile");
}

void TestOutOfBoundsDestinationMapsToNavigationRequestInvalid()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcAiRouteRequest2DResult route =
		Route(iggy::NpcAiRouteRequest2DStatus::Requested, { 0.5F, 0.5F }, { 3.5F, 1.5F });

	const iggy::NpcAiNavigationRequest2DResult result =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcAiNavigationRequest2DStatus::NavigationRequestInvalid, "out-of-bounds destination should map to invalid navigation request");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::DestinationOutOfBounds, "out-of-bounds destination should preserve navigation diagnostic");
	Expect(result.request.destinationTileX == 3 && result.request.destinationTileY == 1, "out-of-bounds request should preserve destination tile");
}

void TestCopiedRouteAndNavigationDiagnosticsArePreserved()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::NpcAiRouteRequest2DResult route =
		Route(iggy::NpcAiRouteRequest2DStatus::Requested, { 0.25F, 0.75F }, { 1.5F, 1.5F }, iggy::NpcAiBehaviorIntent2DType::Investigate);

	const iggy::NpcAiNavigationRequest2DResult result =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcAiNavigationRequest2DStatus::Built, "copied route setup should build request");
	Expect(result.route.intentType == iggy::NpcAiBehaviorIntent2DType::Investigate, "navigation result should preserve route intent type");
	Expect(result.route.decision.intent.type == iggy::NpcAiBehaviorIntent2DType::Investigate, "navigation result should preserve copied decision intent");
	Expect(result.request.status == iggy::navigation::NavigationRequestStatus::Accepted, "navigation result should preserve navigation request status");
	Expect(result.request.destinationTileX == 1 && result.request.destinationTileY == 1, "navigation result should preserve navigation request tile diagnostics");
}

void TestInputRouteIsNotMutated()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	iggy::NpcAiRouteRequest2DResult route =
		Route(iggy::NpcAiRouteRequest2DStatus::Requested, { 0.5F, 0.5F }, { 2.5F, 1.5F });
	const iggy::NpcAiRouteRequest2DStatus statusBefore = route.status;
	const iggy::Vec2 startBefore = route.startPosition;
	const iggy::Vec2 targetBefore = route.targetPosition;
	const iggy::NpcAiBehaviorIntent2DType intentBefore = route.intentType;
	const bool requestsRouteBefore = route.requestsRoute;

	const iggy::NpcAiNavigationRequest2DResult result =
		iggy::NpcAiNavigationRequestBuilder2D {}.build(route, map);

	Expect(result.status == iggy::NpcAiNavigationRequest2DStatus::Built, "immutability setup should build request");
	Expect(route.status == statusBefore, "navigation request builder should not mutate route status");
	Expect(NearVec(route.startPosition, startBefore), "navigation request builder should not mutate route start");
	Expect(NearVec(route.targetPosition, targetBefore), "navigation request builder should not mutate route target");
	Expect(route.intentType == intentBefore, "navigation request builder should not mutate route intent");
	Expect(route.requestsRoute == requestsRouteBefore, "navigation request builder should not mutate route request flag");
}

} // namespace

int main()
{
	TestNoRouteRequestStatusesMapToNoRouteRequest();
	TestRequestedRouteBuildsAcceptedNavigationRequest();
	TestBlockedDestinationMapsToNavigationRequestInvalid();
	TestOutOfBoundsDestinationMapsToNavigationRequestInvalid();
	TestCopiedRouteAndNavigationDiagnosticsArePreserved();
	TestInputRouteIsNotMutated();

	return Failures;
}
