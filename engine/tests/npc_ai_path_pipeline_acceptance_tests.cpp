#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/ai/NpcAiPathReport2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position,
	float radius,
	float patrolWeight,
	float coverWeight,
	float dangerWeight,
	float interestWeight,
	bool enabled = true)
{
	return {
		Id(id),
		position,
		radius,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		{},
		{},
		enabled,
	};
}

iggy::AiMap2D AiMap(std::vector<iggy::AiMapNode2D> nodes)
{
	return { nodes };
}

iggy::LevelTileMap Grid(std::vector<std::string_view> rows)
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

iggy::NpcAiProfile2D Profile(
	const char *profileId = "ai-profile:path",
	float aggression = 0.0F,
	float bravery = 0.0F,
	float alertness = 1.0F)
{
	return {
		Id(profileId),
		Id("faction:town"),
		aggression,
		bravery,
		alertness,
		4.0F,
		{},
	};
}

iggy::NpcAiCurrentState2D State(
	iggy::Vec2 position = { 0.5F, 0.5F },
	bool enabled = true,
	const char *npcId = "npc:path")
{
	return {
		Id(npcId),
		position,
		Id("goal:path"),
		enabled,
	};
}

struct PipelineResult {
	iggy::NpcAiDecision2DResult decision;
	iggy::NpcAiRouteRequest2DResult route;
	iggy::NpcAiNavigationRequest2DResult navigation;
	iggy::NpcAiPathReport2DResult path;
};

PipelineResult RunPipeline(
	const iggy::AiMap2D &aiMap,
	const iggy::NpcAiProfile2D &profile,
	const iggy::NpcAiCurrentState2D &state,
	const iggy::LevelTileMap &grid,
	const iggy::NpcAiDecision2DConfig &decisionConfig = {})
{
	PipelineResult result;
	result.decision = iggy::NpcAiDecisionMaker2D {}.decide(aiMap, profile, state, decisionConfig);
	result.route = iggy::NpcAiRouteRequestBuilder2D {}.build(result.decision);
	result.navigation = iggy::NpcAiNavigationRequestBuilder2D {}.build(result.route, grid);
	result.path = iggy::NpcAiPathReporter2D {}.findPath(result.navigation, grid);
	return result;
}

bool SameTile(iggy::TileCoord tile, int x, int y)
{
	return tile.x == x && tile.y == y;
}

bool SameNodes(const std::vector<iggy::AiMapNode2D> &actual, const std::vector<iggy::AiMapNode2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| !NearVec(actual[index].position, expected[index].position)
			|| actual[index].radius != expected[index].radius
			|| actual[index].patrolWeight != expected[index].patrolWeight
			|| actual[index].coverWeight != expected[index].coverWeight
			|| actual[index].dangerWeight != expected[index].dangerWeight
			|| actual[index].interestWeight != expected[index].interestWeight
			|| actual[index].tags != expected[index].tags
			|| actual[index].links != expected[index].links
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

bool SameProfile(const iggy::NpcAiProfile2D &actual, const iggy::NpcAiProfile2D &expected)
{
	return actual.profileId == expected.profileId
		&& actual.factionId == expected.factionId
		&& actual.aggression == expected.aggression
		&& actual.bravery == expected.bravery
		&& actual.alertness == expected.alertness
		&& actual.preferredRange == expected.preferredRange
		&& actual.behaviorTags == expected.behaviorTags;
}

bool SameState(const iggy::NpcAiCurrentState2D &actual, const iggy::NpcAiCurrentState2D &expected)
{
	return actual.npcId == expected.npcId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.enabled == expected.enabled;
}

bool SameGrid(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable)
			return false;
	}
	return true;
}

void ExpectNoRouteOrPath(const PipelineResult &result, const char *message)
{
	Expect(!result.route.hasRouteRequest(), message);
	Expect(result.navigation.status == iggy::NpcAiNavigationRequest2DStatus::NoRouteRequest, message);
	Expect(!result.navigation.hasNavigationRequest(), message);
	Expect(result.path.status == iggy::NpcAiPathReport2DStatus::NoNavigationRequest, message);
	Expect(!result.path.hasPath(), message);
}

void TestPatrolInterestPipelineFindsPath()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 1.5F, 0.5F }, 4.0F, 2.0F, 0.0F, 0.0F, 0.0F),
		Node("ai:interest", { 3.5F, 0.5F }, 4.0F, 0.0F, 0.0F, 0.0F, 5.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		"....",
		"....",
	});

	const PipelineResult result = RunPipeline(aiMap, Profile("ai-profile:interest"), State(), grid);

	Expect(result.decision.status == iggy::NpcAiDecision2DStatus::Decided, "patrol/interest context should produce decision");
	Expect(result.decision.intent.type == iggy::NpcAiBehaviorIntent2DType::Investigate, "interest score should classify investigate");
	Expect(result.decision.target.selectedNodeId == Id("ai:interest"), "interest decision should select interest target");
	Expect(result.route.status == iggy::NpcAiRouteRequest2DStatus::Requested, "selected target should produce route request");
	Expect(result.navigation.status == iggy::NpcAiNavigationRequest2DStatus::Built, "route should validate into navigation request");
	Expect(result.navigation.request.status == iggy::navigation::NavigationRequestStatus::Accepted, "navigation request should be accepted");
	Expect(result.path.status == iggy::NpcAiPathReport2DStatus::PathFound, "accepted navigation request should find path");
	Expect(result.path.path.tiles.size() == 4, "simple path should preserve tile route");
	if (result.path.path.tiles.size() == 4) {
		Expect(SameTile(result.path.path.tiles.front(), 0, 0), "path should start at NPC tile");
		Expect(SameTile(result.path.path.tiles.back(), 3, 0), "path should end at selected target tile");
	}
}

void TestCoverDangerMapWeightsChangeTargetAndPathWithoutProfileChange()
{
	const iggy::NpcAiProfile2D profile = Profile("ai-profile:stable", 0.0F, 0.0F, 1.0F);
	const iggy::NpcAiCurrentState2D state = State();
	const iggy::LevelTileMap grid = Grid({
		"....",
		"....",
		"....",
	});
	const iggy::AiMap2D coverMap = AiMap({
		Node("ai:cover", { 0.5F, 2.5F }, 5.0F, 0.0F, 5.0F, 0.0F, 0.0F),
	});
	const iggy::AiMap2D dangerMap = AiMap({
		Node("ai:danger-high", { 0.5F, 2.5F }, 5.0F, 0.0F, 0.0F, 5.0F, 0.0F),
		Node("ai:danger-low", { 3.5F, 0.5F }, 5.0F, 0.0F, 0.0F, 1.0F, 0.0F),
	});

	const PipelineResult cover = RunPipeline(coverMap, profile, state, grid);
	const PipelineResult danger = RunPipeline(dangerMap, profile, state, grid);

	Expect(cover.decision.intent.type == iggy::NpcAiBehaviorIntent2DType::TakeCover, "cover map weights should classify cover with unchanged profile");
	Expect(cover.decision.target.selectedNodeId == Id("ai:cover"), "cover map should select cover target");
	Expect(danger.decision.intent.type == iggy::NpcAiBehaviorIntent2DType::AvoidDanger, "danger map weights should classify avoid danger with unchanged profile");
	Expect(danger.decision.target.selectedNodeId == Id("ai:danger-low"), "danger map should select safest target");
	Expect(cover.path.status == iggy::NpcAiPathReport2DStatus::PathFound, "cover map should find path");
	Expect(danger.path.status == iggy::NpcAiPathReport2DStatus::PathFound, "danger map should find path");
	Expect(!NearVec(cover.route.targetPosition, danger.route.targetPosition), "changed map weights should change route target without changing profile");
	if (!cover.path.path.tiles.empty() && !danger.path.path.tiles.empty()) {
		Expect(!SameTile(cover.path.path.tiles.back(), danger.path.path.tiles.back().x, danger.path.path.tiles.back().y), "changed map weights should change path destination tile");
	}
}

void TestDisabledNpcProducesNoRouteNavigationOrPath()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 2.5F, 0.5F }, 4.0F, 4.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		"...",
		"...",
	});

	const PipelineResult result = RunPipeline(aiMap, Profile("ai-profile:disabled"), State({ 0.5F, 0.5F }, false), grid);

	Expect(result.decision.status == iggy::NpcAiDecision2DStatus::DisabledNpc, "disabled NPC should stop at decision layer");
	ExpectNoRouteOrPath(result, "disabled NPC should not produce route, navigation request, or path");
}

void TestNoMapContextProducesNoRouteOrPath()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:far", { 20.5F, 20.5F }, 1.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		"...",
		"...",
	});

	const PipelineResult result = RunPipeline(aiMap, Profile("ai-profile:no-context"), State(), grid);

	Expect(result.decision.status == iggy::NpcAiDecision2DStatus::NoMapContext, "no map context should stop at decision layer");
	ExpectNoRouteOrPath(result, "no map context should not produce route, navigation request, or path");
}

void TestHoldPositionProducesNoRouteOrPath()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:low", { 1.5F, 0.5F }, 4.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		"...",
		"...",
	});
	const iggy::NpcAiDecision2DConfig config { { 100.0F } };

	const PipelineResult result = RunPipeline(aiMap, Profile("ai-profile:hold"), State(), grid, config);

	Expect(result.decision.status == iggy::NpcAiDecision2DStatus::Decided, "below-threshold scorable context should still decide");
	Expect(result.decision.intent.type == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "below-threshold context should classify HoldPosition");
	Expect(result.route.status == iggy::NpcAiRouteRequest2DStatus::HoldPosition, "HoldPosition should stop at route layer");
	ExpectNoRouteOrPath(result, "HoldPosition should not produce navigation request or path");
}

void TestInvalidNavigationDestinationProducesNoPath()
{
	const iggy::AiMap2D blockedAiMap = AiMap({
		Node("ai:blocked", { 1.5F, 1.5F }, 4.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap blockedGrid = Grid({
		"...",
		".#.",
	});

	const PipelineResult blocked = RunPipeline(blockedAiMap, Profile("ai-profile:blocked"), State(), blockedGrid);
	Expect(blocked.route.status == iggy::NpcAiRouteRequest2DStatus::Requested, "blocked destination setup should still produce route");
	Expect(blocked.navigation.status == iggy::NpcAiNavigationRequest2DStatus::NavigationRequestInvalid, "blocked destination should invalidate navigation request");
	Expect(blocked.navigation.request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "blocked destination should preserve navigation diagnostic");
	Expect(blocked.path.status == iggy::NpcAiPathReport2DStatus::NoNavigationRequest, "invalid navigation should not run pathfinder");

	const iggy::AiMap2D outOfBoundsAiMap = AiMap({
		Node("ai:out-of-bounds", { 3.5F, 1.5F }, 5.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap smallGrid = Grid({
		"...",
		"...",
	});

	const PipelineResult outOfBounds = RunPipeline(outOfBoundsAiMap, Profile("ai-profile:oob"), State(), smallGrid);
	Expect(outOfBounds.navigation.status == iggy::NpcAiNavigationRequest2DStatus::NavigationRequestInvalid, "out-of-bounds destination should invalidate navigation request");
	Expect(outOfBounds.navigation.request.status == iggy::navigation::NavigationRequestStatus::DestinationOutOfBounds, "out-of-bounds destination should preserve navigation diagnostic");
	Expect(outOfBounds.path.status == iggy::NpcAiPathReport2DStatus::NoNavigationRequest, "out-of-bounds navigation should not run pathfinder");
}

void TestValidNavigationWithBlockedPathReportsPathNotFound()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:across-wall", { 4.5F, 1.5F }, 10.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		"..#..",
		"..#..",
		"..#..",
	});

	const PipelineResult result = RunPipeline(aiMap, Profile("ai-profile:no-path"), State({ 0.5F, 1.5F }), grid);

	Expect(result.navigation.status == iggy::NpcAiNavigationRequest2DStatus::Built, "blocked path setup should have valid navigation request");
	Expect(result.navigation.request.status == iggy::navigation::NavigationRequestStatus::Accepted, "blocked path destination should be accepted");
	Expect(result.path.status == iggy::NpcAiPathReport2DStatus::PathNotFound, "blocked path should map to PathNotFound");
	Expect(result.path.path.status == iggy::navigation::NavigationPathStatus::NoPath, "blocked path should preserve pathfinder diagnostic");
	Expect(result.path.path.tiles.empty(), "blocked path should not produce path tiles");
}

void TestInputsAreNotMutated()
{
	iggy::AiMap2D aiMap = AiMap({
		Node("ai:immutable", { 2.5F, 0.5F }, 4.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	iggy::NpcAiProfile2D profile = Profile("ai-profile:immutable");
	iggy::NpcAiCurrentState2D state = State({ 0.5F, 0.5F }, true, "npc:immutable");
	iggy::LevelTileMap grid = Grid({
		"...",
		"...",
	});
	const std::vector<iggy::AiMapNode2D> nodesBefore = aiMap.nodes;
	const iggy::NpcAiProfile2D profileBefore = profile;
	const iggy::NpcAiCurrentState2D stateBefore = state;
	const iggy::LevelTileMap gridBefore = grid;

	const PipelineResult result = RunPipeline(aiMap, profile, state, grid);

	Expect(result.path.status == iggy::NpcAiPathReport2DStatus::PathFound, "immutability setup should find path");
	Expect(SameNodes(aiMap.nodes, nodesBefore), "AI path pipeline should not mutate AI map");
	Expect(SameProfile(profile, profileBefore), "AI path pipeline should not mutate NPC profile");
	Expect(SameState(state, stateBefore), "AI path pipeline should not mutate current state");
	Expect(SameGrid(grid, gridBefore), "AI path pipeline should not mutate navigation grid");
}

} // namespace

int main()
{
	TestPatrolInterestPipelineFindsPath();
	TestCoverDangerMapWeightsChangeTargetAndPathWithoutProfileChange();
	TestDisabledNpcProducesNoRouteNavigationOrPath();
	TestNoMapContextProducesNoRouteOrPath();
	TestHoldPositionProducesNoRouteOrPath();
	TestInvalidNavigationDestinationProducesNoPath();
	TestValidNavigationWithBlockedPathReportsPathNotFound();
	TestInputsAreNotMutated();

	return Failures;
}
