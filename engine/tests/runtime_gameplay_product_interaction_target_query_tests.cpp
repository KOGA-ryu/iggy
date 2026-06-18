#include "runtime/RuntimeGameplayProductInteractionTargetQuery.hpp"

#include <cstdlib>
#include <vector>

#include "scene/player/PlayerAgentState.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using Status =
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus;
using Kind = iggy::runtime::RuntimeGameplayProductInteractionTargetQueryKind;

const iggy::ResourceId PlayerId { "player:target-query" };

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::Vec2 position,
	float radius,
	bool enabled = true,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(
	std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built,
		"product interaction target query registry setup should build");
	return result.registry;
}

iggy::runtime::RuntimeGameplayProductPlayModeState PlayState(
	iggy::InteractionTarget2DRegistry targets = {},
	bool loaded = true,
	bool hasPlayer = true,
	iggy::Vec2 playerPosition = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state;
	state.loop.loaded = loaded;
	state.loop.currentState.interaction.targets = targets;
	state.loop.currentState.session.hasPlayer = hasPlayer;
	if (hasPlayer) {
		state.loop.currentState.session.player =
			iggy::test::PlayerAgent(PlayerId, playerPosition);
	}
	return state;
}

iggy::runtime::RuntimeGameplayProductInteractionTargetQueryInput Input(
	iggy::runtime::RuntimeGameplayProductPlayModeState state,
	Kind kind)
{
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryInput input;
	input.state = state;
	input.kind = kind;
	return input;
}

void ExpectTarget(
	const iggy::InteractionTarget2D &actual,
	const iggy::InteractionTarget2D &expected,
	const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

bool DefaultNoTarget(
	const iggy::runtime::RuntimeGameplayProductInteractionTargetQueryResult &result)
{
	return !result.hasTarget && result.targetId.empty() &&
		result.target.id.empty() && !result.hasPlayer && !result.hasReach &&
		!result.reachable;
}

void TestNotLoadedReturnsNotLoadedWithoutQueryingTargets()
{
	const iggy::InteractionTarget2D target =
		Target("target:loaded-only", { 0.5F, 0.5F }, 10.0F);
	auto input = Input(PlayState(Registry({ target }), false), Kind::TileCenter);
	input.tile = { 0, 0 };

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::NotLoaded,
		"not-loaded product target query should report NotLoaded");
	Expect(result.kind == Kind::TileCenter,
		"not-loaded product target query should preserve requested kind");
	Expect(result.spatial.status ==
			iggy::InteractionTargetSpatialQuery2DStatus::NotFound,
		"not-loaded product target query should not run spatial lookup");
	Expect(DefaultNoTarget(result),
		"not-loaded product target query should not expose target or reach");
}

void TestLoadedMissingQueryDoesNotQueryTargets()
{
	const iggy::InteractionTarget2D target =
		Target("target:missing-query", { 0.5F, 0.5F }, 10.0F);
	const auto input = Input(PlayState(Registry({ target })), Kind::None);

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::MissingQuery,
		"kind None should report MissingQuery");
	Expect(result.kind == Kind::None,
		"missing query should preserve kind");
	Expect(result.spatial.status ==
			iggy::InteractionTargetSpatialQuery2DStatus::NotFound,
		"missing query should not run spatial lookup");
	Expect(DefaultNoTarget(result),
		"missing query should not expose target or reach");
}

void TestLoadedTileQueryFindsTargetThroughTileCenter()
{
	const iggy::InteractionTarget2D target =
		Target("target:tile", { 2.5F, 3.5F }, 0.0F,
			true,
			iggy::InteractionTarget2DKind::Door);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:far", { 10.0F, 10.0F }, 1.0F),
		target,
	});
	auto input = Input(PlayState(registry, true, false), Kind::TileCenter);
	input.tile = { 2, 3 };

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::TargetFound,
		"tile-center product target query should find target");
	Expect(result.kind == Kind::TileCenter,
		"tile-center product target query should preserve kind");
	Expect(result.hasTarget,
		"tile-center product target query should expose found target");
	Expect(result.targetId == target.id,
		"tile-center product target query should copy target id");
	ExpectTarget(result.target, target,
		"tile-center product target query should copy target payload");
	Expect(result.spatial.targetIndex == 1,
		"tile-center product target query should preserve spatial index");
	Expect(Near(result.spatial.distance, 0.0F),
		"tile-center product target query should preserve spatial distance");
	Expect(Near(result.spatial.allowedDistance, 0.0F),
		"tile-center product target query should preserve allowed distance");
	Expect(!result.hasPlayer && !result.hasReach && !result.reachable,
		"tile-center product target query without player should not annotate reach");
}

void TestLoadedPointQueryFindsTargetThroughPoint()
{
	const iggy::InteractionTarget2D target =
		Target("target:point", { 1.0F, 1.0F }, 2.0F);
	auto input = Input(PlayState(Registry({ target }), true, false), Kind::Point);
	input.point = { 2.0F, 1.0F };

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::TargetFound,
		"point product target query should find target");
	Expect(result.kind == Kind::Point,
		"point product target query should preserve kind");
	Expect(result.spatial.status ==
			iggy::InteractionTargetSpatialQuery2DStatus::Found,
		"point product target query should preserve spatial found result");
	Expect(result.targetId == target.id,
		"point product target query should copy spatial target id");
	Expect(Near(result.spatial.distance, 1.0F),
		"point product target query should preserve spatial distance");
	Expect(Near(result.spatial.allowedDistance, 2.0F),
		"point product target query should preserve spatial allowed distance");
}

void TestMissingSpatialTargetsReturnTargetNotFound()
{
	const iggy::InteractionTarget2D disabled =
		Target("target:disabled", { 0.5F, 0.5F }, 10.0F, false);
	const iggy::InteractionTarget2D far =
		Target("target:far", { 5.0F, 5.0F }, 0.25F);

	auto disabledInput =
		Input(PlayState(Registry({ disabled })), Kind::TileCenter);
	disabledInput.tile = { 0, 0 };
	auto farInput = Input(PlayState(Registry({ far })), Kind::Point);
	farInput.point = { 0.0F, 0.0F };
	auto emptyInput = Input(PlayState(Registry({})), Kind::Point);
	emptyInput.point = { 0.0F, 0.0F };

	const auto disabledResult =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			disabledInput);
	const auto farResult =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			farInput);
	const auto emptyResult =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			emptyInput);

	Expect(disabledResult.status == Status::TargetNotFound,
		"disabled in-range target should be invisible to product query");
	Expect(farResult.status == Status::TargetNotFound,
		"out-of-range point should report TargetNotFound");
	Expect(emptyResult.status == Status::TargetNotFound,
		"empty interaction registry should report TargetNotFound");
	Expect(disabledResult.spatial.status ==
			iggy::InteractionTargetSpatialQuery2DStatus::NotFound,
		"disabled miss should preserve spatial NotFound");
	Expect(farResult.spatial.status ==
			iggy::InteractionTargetSpatialQuery2DStatus::NotFound,
		"far miss should preserve spatial NotFound");
	Expect(emptyResult.spatial.status ==
			iggy::InteractionTargetSpatialQuery2DStatus::NotFound,
		"empty miss should preserve spatial NotFound");
	Expect(DefaultNoTarget(disabledResult),
		"disabled miss should not expose target or reach");
	Expect(DefaultNoTarget(farResult),
		"far miss should not expose target or reach");
	Expect(DefaultNoTarget(emptyResult),
		"empty miss should not expose target or reach");
}

void TestSpatialConfigExtraRadiusIsForwarded()
{
	const iggy::InteractionTarget2D target =
		Target("target:expanded", { 0.0F, 0.0F }, 1.0F);
	auto input = Input(PlayState(Registry({ target }), true, false), Kind::Point);
	input.point = { 2.0F, 0.0F };
	input.spatialConfig.extraRadius = 1.0F;

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::TargetFound,
		"product target query should forward spatial extra radius");
	Expect(result.targetId == target.id,
		"expanded product target query should return target");
	Expect(Near(result.spatial.distance, 2.0F),
		"expanded product target query should preserve spatial distance");
	Expect(Near(result.spatial.allowedDistance, 2.0F),
		"expanded product target query should preserve expanded allowed distance");
}

void TestFoundTargetWithPlayerAnnotatesReach()
{
	const iggy::InteractionTarget2D target =
		Target("target:reachable", { 2.0F, 0.0F }, 0.5F);
	auto reachableInput =
		Input(PlayState(Registry({ target }), true, true, { 0.0F, 0.0F }),
			Kind::Point);
	reachableInput.point = { 2.0F, 0.0F };
	reachableInput.reachConfig.extraReach = 1.5F;
	auto outOfRangeInput = reachableInput;
	outOfRangeInput.reachConfig.extraReach = 0.0F;

	const auto reachableResult =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			reachableInput);
	const auto outOfRangeResult =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			outOfRangeInput);

	Expect(reachableResult.status == Status::TargetFound,
		"reachable product target query should preserve found status");
	Expect(reachableResult.hasTarget && reachableResult.hasPlayer &&
			reachableResult.hasReach,
		"reachable product target query should expose target/player/reach");
	Expect(reachableResult.reachable,
		"reachable product target query should mark target reachable");
	Expect(reachableResult.targetQuery.status ==
			iggy::InteractionTargetQuery2DStatus::Found,
		"reachable product target query should preserve id query result");
	Expect(reachableResult.reach.status ==
			iggy::InteractionReach2DStatus::Reachable,
		"reachable product target query should preserve reach status");
	Expect(Near(reachableResult.reach.distance, 2.0F),
		"reachable product target query should preserve reach distance");
	Expect(Near(reachableResult.reach.allowedDistance, 2.0F),
		"reachable product target query should preserve reach allowed distance");

	Expect(outOfRangeResult.status == Status::TargetFound,
		"out-of-reach product target query should still preserve found status");
	Expect(outOfRangeResult.hasTarget && outOfRangeResult.hasPlayer &&
			outOfRangeResult.hasReach,
		"out-of-reach product target query should still annotate reach");
	Expect(!outOfRangeResult.reachable,
		"out-of-reach product target query should mark target unreachable");
	Expect(outOfRangeResult.reach.status ==
			iggy::InteractionReach2DStatus::OutOfRange,
		"out-of-reach product target query should preserve reach out-of-range status");
}

void TestFoundTargetWithoutPlayerKeepsVisibilityWithoutReach()
{
	const iggy::InteractionTarget2D target =
		Target("target:no-player", { 0.0F, 0.0F }, 1.0F);
	auto input =
		Input(PlayState(Registry({ target }), true, false), Kind::Point);
	input.point = { 0.0F, 0.0F };

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::TargetFound,
		"missing player should not block target visibility");
	Expect(result.hasTarget,
		"missing player found result should expose target");
	Expect(result.targetId == target.id,
		"missing player found result should copy target id");
	Expect(!result.hasPlayer && !result.hasReach && !result.reachable,
		"missing player found result should not annotate reach");
	Expect(result.targetQuery.status ==
			iggy::InteractionTargetQuery2DStatus::NotFound,
		"missing player found result should leave target query default");
}

void TestQueryDoesNotMutateStateOrRegistry()
{
	const iggy::InteractionTarget2D first =
		Target("target:first", { 1.0F, 0.0F }, 2.0F);
	const iggy::InteractionTarget2D second =
		Target("target:second", { 4.0F, 0.0F }, 1.0F);
	auto input =
		Input(PlayState(Registry({ first, second }), true, true, { 0.0F, 0.0F }),
			Kind::Point);
	input.point = { 1.0F, 0.0F };
	const auto stateBefore = input.state;
	const std::vector<iggy::InteractionTarget2D> targetsBefore =
		input.state.loop.currentState.interaction.targets.targets();

	const auto result =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQuery {}.find(
			input);

	Expect(result.status == Status::TargetFound,
		"immutability setup should find target");
	Expect(input.state.loop.loaded == stateBefore.loop.loaded,
		"product target query should not mutate loaded flag");
	Expect(input.state.hasInputFocus == stateBefore.hasInputFocus,
		"product target query should not mutate focus");
	Expect(NearVec(
			   input.state.loop.currentState.session.player.position,
			   stateBefore.loop.currentState.session.player.position),
		"product target query should not mutate player position");
	Expect(input.state.loop.currentState.interaction.targets.targets().size() ==
			targetsBefore.size(),
		"product target query should not mutate target count");
	if (input.state.loop.currentState.interaction.targets.targets().size() ==
		targetsBefore.size()) {
		ExpectTarget(
			input.state.loop.currentState.interaction.targets.targets()[0],
			targetsBefore[0],
			"product target query should not mutate first target payload");
		ExpectTarget(
			input.state.loop.currentState.interaction.targets.targets()[1],
			targetsBefore[1],
			"product target query should not mutate second target payload");
	}
}

} // namespace

int main()
{
	TestNotLoadedReturnsNotLoadedWithoutQueryingTargets();
	TestLoadedMissingQueryDoesNotQueryTargets();
	TestLoadedTileQueryFindsTargetThroughTileCenter();
	TestLoadedPointQueryFindsTargetThroughPoint();
	TestMissingSpatialTargetsReturnTargetNotFound();
	TestSpatialConfigExtraRadiusIsForwarded();
	TestFoundTargetWithPlayerAnnotatesReach();
	TestFoundTargetWithoutPlayerKeepsVisibilityWithoutReach();
	TestQueryDoesNotMutateStateOrRegistry();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
