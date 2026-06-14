#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorEscapeTarget2D.hpp"
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
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Run,
	const char *npcId = "npc:guard")
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

bool SameMap(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()
		|| actual.entitySpawns.size() != expected.entitySpawns.size()
		|| actual.playerStart.x != expected.playerStart.x
		|| actual.playerStart.y != expected.playerStart.y) {
		return false;
	}

	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable) {
			return false;
		}
	}

	return true;
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

void ExpectNoEscape(
	const iggy::NpcActorEscapeTarget2D &result,
	iggy::NpcActorEscapeTarget2DStatus status,
	const char *message)
{
	Expect(result.status == status, message);
	Expect(!result.hasEscapeTarget, "non-ready escape result should not have target");
	Expect(!result.ready(), "non-ready escape result should not be ready");
}

void TestNonReadyIntentReturnsNoMovementIntent()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D intent = Intent(
		Actor("npc:idle"),
		Control(iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:idle"));

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	ExpectNoEscape(result, iggy::NpcActorEscapeTarget2DStatus::NoMovementIntent, "non-ready intent should return NoMovementIntent");
	Expect(SameIntent(result.intent, intent), "non-ready intent should be copied");
	Expect(result.npcId == Id("npc:idle"), "non-ready intent should preserve npc id");
	Expect(NearVec(result.startPosition, { 1.5F, 1.5F }), "non-ready intent should preserve start position");
}

void TestReadyMoveToIntentReturnsNotMoveAwayFrom()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D intent = Intent(
		Actor("npc:seek"),
		Control(iggy::seekingNpcBehaviorState({ 2.5F, 1.5F }), iggy::NpcMoveMode::Walk, "npc:seek"));

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	ExpectNoEscape(result, iggy::NpcActorEscapeTarget2DStatus::NotMoveAwayFrom, "ready MoveTo intent should not produce escape target");
	Expect(result.npcId == Id("npc:seek"), "MoveTo rejection should preserve npc id");
}

void TestThreatAtActorPositionReturnsThreatAtActorPosition()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:panic", { 1.5F, 1.5F }, { 1.5005F, 1.5F });

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	ExpectNoEscape(result, iggy::NpcActorEscapeTarget2DStatus::ThreatAtActorPosition, "threat at actor position should report undefined escape direction");
	Expect(NearVec(result.threatPosition, { 1.5005F, 1.5F }), "threat-at-actor should preserve threat position");
}

void TestInvalidMapDimensionsOrCellCountReturnInvalidMapQuery()
{
	iggy::LevelTileMap zeroWidth = Map(0, 3);
	iggy::LevelTileMap badCellCount = Map(3, 3);
	badCellCount.tiles.pop_back();
	const iggy::NpcActorMovementIntent2D intent = FleeIntent();

	const iggy::NpcActorEscapeTarget2D zeroWidthResult =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, zeroWidth);
	const iggy::NpcActorEscapeTarget2D badCellCountResult =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, badCellCount);

	ExpectNoEscape(zeroWidthResult, iggy::NpcActorEscapeTarget2DStatus::InvalidMapQuery, "zero-width map should be invalid");
	ExpectNoEscape(badCellCountResult, iggy::NpcActorEscapeTarget2DStatus::InvalidMapQuery, "mismatched tile count should be invalid");
}

void TestOpenMapChoosesNeighborFartherFromThreat()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:flee", { 1.5F, 1.5F }, { 0.5F, 1.5F });

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	Expect(result.status == iggy::NpcActorEscapeTarget2DStatus::Ready, "open map should find escape target");
	Expect(result.ready() && result.hasEscapeTarget, "open map should mark escape target ready");
	Expect(result.startTile == iggy::TileCoord { 1, 1 }, "open map should preserve start tile");
	Expect(result.escapeTile == iggy::TileCoord { 2, 0 }, "open map should select farthest deterministic neighbor from left-side threat");
	Expect(NearVec(result.escapePosition, { 2.5F, 0.5F }), "escape position should be selected tile center");
	Expect(result.selectedCandidateIndex < result.candidates.size(), "selected candidate index should point into candidates");
	Expect(result.candidates[result.selectedCandidateIndex].tile == result.escapeTile, "selected candidate should match escape tile");
}

void TestOffMapAndBlockedCandidatesAreIgnored()
{
	const iggy::LevelTileMap map = Map(2, 2, {
		true, false,
		true, true,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:edge", { 0.5F, 0.5F }, { -0.5F, 0.5F });

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	Expect(result.status == iggy::NpcActorEscapeTarget2DStatus::Ready, "edge actor should still find valid unblocked escape");
	Expect(result.escapeTile == iggy::TileCoord { 1, 1 }, "blocked and off-map candidates should be ignored");
	Expect(result.candidates.size() == 2, "only valid walkable neighbor candidates should be reported");
	for (const iggy::NpcActorEscapeTargetCandidate2D &candidate : result.candidates) {
		Expect(candidate.tile != iggy::TileCoord { 1, 0 }, "blocked candidate should not be reported");
		Expect(candidate.tile.x >= 0 && candidate.tile.y >= 0, "off-map candidates should not be reported");
	}
}

void TestNoValidWalkableCandidatesReturnsNoCandidates()
{
	const iggy::LevelTileMap map = Map(3, 3, {
		false, false, false,
		false, true, false,
		false, false, false,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent();

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	ExpectNoEscape(result, iggy::NpcActorEscapeTarget2DStatus::NoCandidates, "trapped actor should report no candidates");
	Expect(result.candidates.empty(), "trapped actor should report no walkable candidates");
}

void TestNoBetterCandidatePreservesCandidates()
{
	const iggy::LevelTileMap map = Map(2, 3, {
		false, false,
		true, true,
		false, false,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:cornered", { 1.5F, 1.5F }, { 0.5F, 1.5F });

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	ExpectNoEscape(result, iggy::NpcActorEscapeTarget2DStatus::NoBetterCandidate, "closer-only candidates should not be accepted by default");
	Expect(result.candidates.size() == 1, "no-better result should preserve valid walkable candidates for audit");
	Expect(result.candidates[0].tile == iggy::TileCoord { 0, 1 }, "no-better candidate should preserve candidate tile");
}

void TestCanDisableBetterThanCurrentRequirement()
{
	const iggy::LevelTileMap map = Map(2, 3, {
		false, false,
		true, true,
		false, false,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:desperate", { 1.5F, 1.5F }, { 0.5F, 1.5F });
	iggy::NpcActorEscapeTarget2DConfig config;
	config.requireBetterThanCurrent = false;

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map, config);

	Expect(result.status == iggy::NpcActorEscapeTarget2DStatus::Ready, "disabled better-than-current policy should allow best available candidate");
	Expect(result.escapeTile == iggy::TileCoord { 0, 1 }, "disabled better-than-current policy should select best available candidate");
}

void TestDeterministicTieBreakUsesEarlierScanOrderAfterEqualScores()
{
	const iggy::LevelTileMap map = Map(3, 3, {
		false, false, false,
		false, true, false,
		true, false, true,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:tie", { 1.5F, 1.5F }, { 1.5F, 0.5F });

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	Expect(result.status == iggy::NpcActorEscapeTarget2DStatus::Ready, "tie setup should find candidates");
	Expect(result.candidates.size() == 2, "tie setup should preserve two candidates");
	Expect(result.candidates[0].tile == iggy::TileCoord { 0, 2 }, "tie setup first candidate should follow scan order");
	Expect(result.candidates[1].tile == iggy::TileCoord { 2, 2 }, "tie setup second candidate should follow scan order");
	Expect(result.escapeTile == iggy::TileCoord { 0, 2 }, "exact tie should select earlier scan-order candidate");
}

void TestRadiusTwoCanFindCandidateRadiusOneCannot()
{
	const iggy::LevelTileMap map = Map(5, 3, {
		false, false, false, false, false,
		false, false, true, false, true,
		false, false, false, false, false,
	});
	const iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:radius", { 2.5F, 1.5F }, { 1.5F, 1.5F });
	iggy::NpcActorEscapeTarget2DConfig radiusOne;
	radiusOne.searchRadius = 1;
	iggy::NpcActorEscapeTarget2DConfig radiusTwo;
	radiusTwo.searchRadius = 2;

	const iggy::NpcActorEscapeTarget2D nearResult =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map, radiusOne);
	const iggy::NpcActorEscapeTarget2D farResult =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map, radiusTwo);

	Expect(nearResult.status == iggy::NpcActorEscapeTarget2DStatus::NoCandidates, "radius one should not find distant candidate");
	Expect(farResult.status == iggy::NpcActorEscapeTarget2DStatus::Ready, "radius two should find distant candidate");
	Expect(farResult.escapeTile == iggy::TileCoord { 4, 1 }, "radius two should select distant walkable candidate");
}

void TestNamespacedAndUnqualifiedNpcIdsArePreservedExactly()
{
	const iggy::LevelTileMap map = Map(3, 3);
	const iggy::NpcActorMovementIntent2D namespaced = FleeIntent("npc:guard", { 1.5F, 1.5F }, { 0.5F, 1.5F });
	const iggy::NpcActorMovementIntent2D unqualified = FleeIntent("guard", { 1.5F, 1.5F }, { 0.5F, 1.5F });

	const iggy::NpcActorEscapeTarget2D namespacedResult =
		iggy::NpcActorEscapeTargetProjector2D {}.project(namespaced, map);
	const iggy::NpcActorEscapeTarget2D unqualifiedResult =
		iggy::NpcActorEscapeTargetProjector2D {}.project(unqualified, map);

	Expect(namespacedResult.npcId == Id("npc:guard"), "namespaced npc id should be preserved exactly");
	Expect(unqualifiedResult.npcId == Id("guard"), "unqualified npc id should be preserved exactly");
	Expect(namespacedResult.npcId != unqualifiedResult.npcId, "namespaced and unqualified npc ids should remain distinct");
}

void TestInputsAreNotMutated()
{
	iggy::LevelTileMap map = Map(3, 3);
	iggy::NpcActorMovementIntent2D intent = FleeIntent("npc:stable", { 1.5F, 1.5F }, { 0.5F, 1.5F });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorMovementIntent2D intentBefore = intent;

	const iggy::NpcActorEscapeTarget2D result =
		iggy::NpcActorEscapeTargetProjector2D {}.project(intent, map);

	Expect(result.ready(), "immutability setup should find escape target");
	Expect(SameMap(map, mapBefore), "escape target projection should not mutate map");
	Expect(SameIntent(intent, intentBefore), "escape target projection should not mutate intent");
}

} // namespace

int main()
{
	TestNonReadyIntentReturnsNoMovementIntent();
	TestReadyMoveToIntentReturnsNotMoveAwayFrom();
	TestThreatAtActorPositionReturnsThreatAtActorPosition();
	TestInvalidMapDimensionsOrCellCountReturnInvalidMapQuery();
	TestOpenMapChoosesNeighborFartherFromThreat();
	TestOffMapAndBlockedCandidatesAreIgnored();
	TestNoValidWalkableCandidatesReturnsNoCandidates();
	TestNoBetterCandidatePreservesCandidates();
	TestCanDisableBetterThanCurrentRequirement();
	TestDeterministicTieBreakUsesEarlierScanOrderAfterEqualScores();
	TestRadiusTwoCanFindCandidateRadiusOneCannot();
	TestNamespacedAndUnqualifiedNpcIdsArePreservedExactly();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
