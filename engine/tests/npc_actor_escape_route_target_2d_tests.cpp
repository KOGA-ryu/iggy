#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap Map(int width, int height, std::vector<bool> walkable = {})
{
	iggy::LevelTileMap map;
	map.id = Id("level:test");
	map.width = width;
	map.height = height;
	const std::size_t tileCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
	map.tiles.resize(tileCount);
	for (std::size_t index = 0; index < tileCount; ++index) {
		map.tiles[index].walkable = walkable.empty() ? true : walkable[index];
	}
	return map;
}

iggy::NpcActorState2D Actor(
	const char *npcId = "npc:guard",
	iggy::Vec2 position = { 1.5F, 1.5F },
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:guard"),
		Id("faction:town"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D Control(
	iggy::NpcBehaviorState behavior,
	iggy::NpcMoveMode moveMode,
	const char *npcId)
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		behavior,
		moveMode,
	};
}

iggy::NpcActorMovementIntent2D Intent(
	const iggy::NpcActorState2D &actor,
	const iggy::NpcActorControlState2D &control,
	bool hasControl = true)
{
	return iggy::NpcActorMovementIntentProjector2D {}.project({
		actor,
		control,
		hasControl,
	});
}

iggy::NpcActorMovementIntent2D FleeIntent(
	const char *npcId = "npc:guard",
	iggy::Vec2 start = { 1.5F, 1.5F },
	iggy::Vec2 threat = { 0.5F, 1.5F })
{
	return Intent(
		Actor(npcId, start),
		Control(iggy::fleeingNpcBehaviorState(threat), iggy::NpcMoveMode::Run, npcId));
}

bool SameIntent(const iggy::NpcActorMovementIntent2D &actual, const iggy::NpcActorMovementIntent2D &expected)
{
	return actual.status == expected.status
		&& actual.type == expected.type
		&& actual.npcId == expected.npcId
		&& NearVec(actual.startPosition, expected.startPosition)
		&& NearVec(actual.targetPosition, expected.targetPosition)
		&& actual.moveMode == expected.moveMode
		&& actual.speedMultiplier == expected.speedMultiplier
		&& actual.requestsMovement == expected.requestsMovement;
}

bool SameMap(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()) {
		return false;
	}

	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable) {
			return false;
		}
	}
	return true;
}

void ExpectNoRoute(
	const iggy::NpcActorEscapeRouteTarget2D &result,
	iggy::NpcActorEscapeRouteTarget2DStatus status,
	const char *message)
{
	Expect(result.status == status, message);
	Expect(!result.requestsRoute, "non-ready escape route target should not request route");
	Expect(!result.ready(), "non-ready escape route target should not be ready");
}

void TestNonReadyIntentReturnsNoMovementIntent()
{
	const iggy::LevelTileMap invalidMap;
	const iggy::NpcActorMovementIntent2D intent = Intent(
		Actor("npc:idle"),
		Control(iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:idle"));

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, invalidMap);

	ExpectNoRoute(result, iggy::NpcActorEscapeRouteTarget2DStatus::NoMovementIntent, "non-ready intent should return NoMovementIntent");
	Expect(SameIntent(result.intent, intent), "non-ready adapter result should preserve copied intent");
	Expect(result.escape.status == iggy::NpcActorEscapeTarget2DStatus::NoMovementIntent, "non-ready adapter should not run escape target");
	Expect(result.route.status == iggy::NpcActorRouteTarget2DStatus::NoMovementIntent, "non-ready adapter should not run route target");
}

void TestReadyMoveToReturnsNotMoveAwayFrom()
{
	const iggy::LevelTileMap invalidMap;
	const iggy::NpcActorMovementIntent2D intent = Intent(
		Actor("npc:seek"),
		Control(iggy::seekingNpcBehaviorState({ 2.5F, 1.5F }), iggy::NpcMoveMode::Walk, "npc:seek"));

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, invalidMap);

	ExpectNoRoute(result, iggy::NpcActorEscapeRouteTarget2DStatus::NotMoveAwayFrom, "ready MoveTo should not enter flee escape flow");
	Expect(SameIntent(result.intent, intent), "MoveTo adapter result should preserve copied intent");
	Expect(result.escape.status == iggy::NpcActorEscapeTarget2DStatus::NoMovementIntent, "MoveTo adapter should not run escape target");
	Expect(result.route.status == iggy::NpcActorRouteTarget2DStatus::NoMovementIntent, "MoveTo adapter should not run route target");
}

void TestMoveAwayFromWithoutCandidateReturnsEscapeTargetFailed()
{
	const iggy::LevelTileMap map = Map(3, 3, {
		false, false, false,
		false, true, false,
		false, false, false,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent();

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, map);

	ExpectNoRoute(result, iggy::NpcActorEscapeRouteTarget2DStatus::EscapeTargetFailed, "flee intent without escape candidate should fail at escape stage");
	Expect(result.escape.status == iggy::NpcActorEscapeTarget2DStatus::NoCandidates, "escape failure should preserve escape status");
	Expect(result.escape.candidates.empty(), "escape failure should preserve candidate diagnostics");
	Expect(result.route.status == iggy::NpcActorRouteTarget2DStatus::NoMovementIntent, "escape failure should not run route target");
}

void TestMoveAwayFromWithEscapeCandidateReturnsReadyRoute()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:flee", { 1.5F, 1.5F }, { 0.5F, 1.5F });

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, map);

	Expect(result.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "escape route adapter should be ready when escape and route are ready");
	Expect(result.ready() && result.requestsRoute, "ready escape route adapter should request route");
	Expect(result.escape.ready(), "ready escape route adapter should preserve ready escape target");
	Expect(result.route.ready(), "ready escape route adapter should preserve ready route target");
	Expect(result.route.type == iggy::NpcActorRouteTarget2DType::MoveAwayFrom, "route target should preserve MoveAwayFrom route type");
	Expect(NearVec(result.route.targetPosition, result.escape.escapePosition), "route target should use selected escape position");
	Expect(NearVec(result.route.sourcePosition, intent.targetPosition), "route target should preserve threat/source position");
	Expect(result.route.npcId == Id("npc:flee"), "route target should preserve npc id");
}

void TestRouteArrivalToleranceCanStillFailRouteTarget()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:tolerance", { 1.5F, 1.5F }, { 0.5F, 1.5F });
	iggy::NpcActorEscapeRouteTarget2DConfig config;
	config.route.arrivalTolerance = 10.0F;

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, map, config);

	ExpectNoRoute(result, iggy::NpcActorEscapeRouteTarget2DStatus::RouteTargetFailed, "route arrival tolerance should still be able to reject route request");
	Expect(result.escape.ready(), "route failure should preserve ready escape diagnostics");
	Expect(result.route.status == iggy::NpcActorRouteTarget2DStatus::AlreadyAtTarget, "route failure should preserve route target diagnostics");
	Expect(NearVec(result.route.targetPosition, result.escape.escapePosition), "route failure should preserve route destination from escape");
}

void TestNestedDiagnosticsArePreserved()
{
	const iggy::LevelTileMap map = Map(2, 3, {
		false, false,
		true, true,
		false, false,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:cornered", { 1.5F, 1.5F }, { 0.5F, 1.5F });

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, map);

	Expect(result.status == iggy::NpcActorEscapeRouteTarget2DStatus::EscapeTargetFailed, "no-better escape should fail at escape stage");
	Expect(result.escape.status == iggy::NpcActorEscapeTarget2DStatus::NoBetterCandidate, "nested escape status should be preserved");
	Expect(result.escape.candidates.size() == 1, "nested escape candidates should be preserved");
	Expect(result.escape.candidates[0].tile == iggy::TileCoord { 0, 1 }, "nested escape candidate tile should be preserved");
	Expect(SameIntent(result.intent, intent), "nested diagnostics should preserve copied input intent");
}

void TestInputsAreNotMutated()
{
	iggy::LevelTileMap map = Map(3, 3);
	iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:stable", { 1.5F, 1.5F }, { 0.5F, 1.5F });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorMovementIntent2D intentBefore = intent;

	const iggy::NpcActorEscapeRouteTarget2D result =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intent, map);

	Expect(result.ready(), "immutability setup should produce ready route");
	Expect(SameMap(map, mapBefore), "escape route adapter should not mutate map");
	Expect(SameIntent(intent, intentBefore), "escape route adapter should not mutate intent");
}

} // namespace

int main()
{
	TestNonReadyIntentReturnsNoMovementIntent();
	TestReadyMoveToReturnsNotMoveAwayFrom();
	TestMoveAwayFromWithoutCandidateReturnsEscapeTargetFailed();
	TestMoveAwayFromWithEscapeCandidateReturnsReadyRoute();
	TestRouteArrivalToleranceCanStillFailRouteTarget();
	TestNestedDiagnosticsArePreserved();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
