#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"

#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputBinding2D.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using FrameStatus =
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextStatus;
using QueryStatus =
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus;
using TargetContextStatus =
	iggy::runtime::RuntimeGameplayProductInputTargetContextStatus;

const iggy::ResourceId PlayerId { "player:frame-target-context" };

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::Vec2 position,
	float radius,
	bool enabled = true)
{
	return {
		Id(id),
		iggy::InteractionTarget2DKind::Usable,
		position,
		radius,
		enabled,
	};
}

iggy::InteractionTarget2DRegistry Registry(
	std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built,
		"input frame target context registry setup should build");
	return result.registry;
}

iggy::runtime::RuntimeGameplayProductPlayModeState PlayState(
	iggy::InteractionTarget2DRegistry targets = {},
	bool hasPlayer = true,
	iggy::Vec2 playerPosition = { 0.5F, 0.5F })
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state;
	state.loop.loaded = true;
	state.loop.currentState.interaction.targets = targets;
	state.loop.currentState.session.hasPlayer = hasPlayer;
	if (hasPlayer) {
		state.loop.currentState.session.player =
			iggy::test::PlayerAgent(PlayerId, playerPosition);
	}
	return state;
}

iggy::PlayerInputBindingContext2D BaseContext()
{
	iggy::PlayerInputBindingContext2D context;
	context.input.playerControlEnabled = false;
	context.input.worldInputEnabled = true;
	context.input.interactionEnabled = false;
	context.input.cancelEnabled = true;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 4, -1 };
	context.hasSelectedTargetId = true;
	context.selectedTargetId = Id("target:selected");
	context.hasHoveredTargetId = true;
	context.hoveredTargetId = Id("target:hovered-before");
	return context;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D Event(
	iggy::runtime::RuntimeGameplayProductInputControl2D control,
	iggy::runtime::RuntimeGameplayProductInputEventKind kind =
		iggy::runtime::RuntimeGameplayProductInputEventKind::Pressed)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	event.kind = kind;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D PrimaryTile(
	iggy::TileCoord tile,
	iggy::runtime::RuntimeGameplayProductInputEventKind kind =
		iggy::runtime::RuntimeGameplayProductInputEventKind::Pressed)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event =
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::PrimaryTile,
			kind);
	event.hasTile = true;
	event.tile = tile;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D PrimaryTileMissingPayload()
{
	return Event(
		iggy::runtime::RuntimeGameplayProductInputControl2D::PrimaryTile);
}

iggy::runtime::RuntimeGameplayProductInputFrame2D Frame(
	std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> events,
	iggy::PlayerInputBindingContext2D context = BaseContext())
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext = context;
	frame.events = events;
	return frame;
}

iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult Enrich(
	iggy::runtime::RuntimeGameplayProductPlayModeState state,
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame,
	iggy::InteractionTargetSpatialQuery2DConfig spatialConfig = {},
	iggy::InteractionReach2DConfig reachConfig = {})
{
	return iggy::runtime::RuntimeGameplayProductInputFrameTargetContext {}.enrich({
		state,
		frame,
		spatialConfig,
		reachConfig,
	});
}

void ExpectContextEquals(
	const iggy::PlayerInputBindingContext2D &actual,
	const iggy::PlayerInputBindingContext2D &expected,
	const char *message)
{
	Expect(actual.input.playerControlEnabled ==
			expected.input.playerControlEnabled,
		message);
	Expect(actual.input.worldInputEnabled == expected.input.worldInputEnabled,
		message);
	Expect(actual.input.interactionEnabled ==
			expected.input.interactionEnabled,
		message);
	Expect(actual.input.cancelEnabled == expected.input.cancelEnabled,
		message);
	Expect(actual.hasCurrentPlayerTile == expected.hasCurrentPlayerTile,
		message);
	Expect(actual.currentPlayerTile == expected.currentPlayerTile, message);
	Expect(actual.hasSelectedTargetId == expected.hasSelectedTargetId, message);
	Expect(actual.selectedTargetId == expected.selectedTargetId, message);
	Expect(actual.hasHoveredTargetId == expected.hasHoveredTargetId, message);
	Expect(actual.hoveredTargetId == expected.hoveredTargetId, message);
}

void ExpectEventEquals(
	const iggy::runtime::RuntimeGameplayProductInputEvent2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputEvent2D &expected,
	const char *message)
{
	Expect(actual.control == expected.control, message);
	Expect(actual.kind == expected.kind, message);
	Expect(actual.hasTile == expected.hasTile, message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.hasWorldPoint == expected.hasWorldPoint, message);
	Expect(actual.worldPoint.x == expected.worldPoint.x &&
			actual.worldPoint.y == expected.worldPoint.y,
		message);
	Expect(actual.hasTargetId == expected.hasTargetId, message);
	Expect(actual.targetId == expected.targetId, message);
}

void ExpectEventsEqual(
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> &actual,
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	if (actual.size() != expected.size())
		return;

	for (std::size_t index = 0; index < actual.size(); ++index)
		ExpectEventEquals(actual[index], expected[index], message);
}

void TestNoPrimaryTileReturnsNoEligibleAndPreservesFrame()
{
	const auto frame = Frame({
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::MoveEast),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Interact),
	});

	const auto result = Enrich(PlayState(Registry({})), frame);

	Expect(result.status == FrameStatus::NoEligiblePrimaryTile,
		"frame without PrimaryTile should report no eligible primary tile");
	Expect(!result.hasPrimaryTileEvent,
		"frame without PrimaryTile should not report selected primary tile");
	ExpectContextEquals(result.frame.bindingContext, frame.bindingContext,
		"frame without PrimaryTile should preserve binding context");
	ExpectEventsEqual(result.frame.events, frame.events,
		"frame without PrimaryTile should preserve events");
	Expect(result.target.status == QueryStatus::NotLoaded,
		"frame without PrimaryTile should not query interaction target");
}

void TestPrimaryTileMissingPayloadAndReleaseAreIneligible()
{
	const auto missingPayload = Frame({ PrimaryTileMissingPayload() });
	const auto release = Frame({
		PrimaryTile({ 1, 2 },
			iggy::runtime::RuntimeGameplayProductInputEventKind::Released),
	});

	const auto missingResult =
		Enrich(PlayState(Registry({})), missingPayload);
	const auto releaseResult = Enrich(PlayState(Registry({})), release);

	Expect(missingResult.status == FrameStatus::NoEligiblePrimaryTile,
		"PrimaryTile without tile payload should not be eligible");
	Expect(releaseResult.status == FrameStatus::NoEligiblePrimaryTile,
		"PrimaryTile release should not be eligible");
	ExpectEventsEqual(missingResult.frame.events, missingPayload.events,
		"missing-payload PrimaryTile should be preserved for adapter diagnostics");
	ExpectEventsEqual(releaseResult.frame.events, release.events,
		"PrimaryTile release should be preserved");
}

void TestPressedPrimaryTileOnTargetProjectsHoveredTarget()
{
	const iggy::InteractionTarget2D target =
		Target("target:door", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });

	const auto result = Enrich(PlayState(Registry({ target })), frame);

	Expect(result.status == FrameStatus::TargetProjected,
		"PrimaryTile over target should project hover");
	Expect(result.hasPrimaryTileEvent,
		"PrimaryTile over target should report selected primary tile event");
	Expect(result.primaryTileEventIndex == 0,
		"PrimaryTile over target should report event index");
	const iggy::TileCoord expectedTile { 2, 3 };
	Expect(result.primaryTile == expectedTile,
		"PrimaryTile over target should report tile");
	Expect(result.target.status == QueryStatus::TargetFound,
		"PrimaryTile over target should preserve target query result");
	Expect(result.target.targetId == target.id,
		"PrimaryTile over target should preserve target id");
	Expect(result.targetContext.status == TargetContextStatus::TargetProjected,
		"PrimaryTile over target should preserve target context result");
	Expect(result.frame.bindingContext.hasHoveredTargetId,
		"PrimaryTile over target should set hovered target flag");
	Expect(result.frame.bindingContext.hoveredTargetId == target.id,
		"PrimaryTile over target should set hovered target id");
	ExpectEventsEqual(result.frame.events, frame.events,
		"PrimaryTile over target should preserve events");
}

void TestPressedPrimaryTileMissPreservesContextWithDiagnostics()
{
	iggy::InteractionTargetSpatialQuery2DConfig spatialConfig;
	spatialConfig.extraRadius = 0.0F;
	const iggy::InteractionTarget2D far =
		Target("target:far", { 10.5F, 10.5F }, 0.25F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });

	const auto result =
		Enrich(PlayState(Registry({ far })), frame, spatialConfig);

	Expect(result.status == FrameStatus::Unchanged,
		"PrimaryTile miss should leave context unchanged");
	Expect(result.hasPrimaryTileEvent,
		"PrimaryTile miss should still report chosen event");
	Expect(result.primaryTileEventIndex == 0,
		"PrimaryTile miss should report chosen event index");
	const iggy::TileCoord expectedTile { 2, 3 };
	Expect(result.primaryTile == expectedTile,
		"PrimaryTile miss should report chosen tile");
	Expect(result.target.status == QueryStatus::TargetNotFound,
		"PrimaryTile miss should preserve target-not-found diagnostics");
	Expect(result.targetContext.status == TargetContextStatus::Unchanged,
		"PrimaryTile miss should preserve unchanged target context diagnostics");
	ExpectContextEquals(result.frame.bindingContext, frame.bindingContext,
		"PrimaryTile miss should preserve binding context");
	ExpectEventsEqual(result.frame.events, frame.events,
		"PrimaryTile miss should preserve events");
}

void TestDisabledAndOutOfRangeTargetsDoNotProjectHover()
{
	const iggy::InteractionTarget2D disabled =
		Target("target:disabled", { 2.5F, 3.5F }, 10.0F, false);
	const iggy::InteractionTarget2D outOfRange =
		Target("target:out-of-range", { 2.75F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });

	auto disabledResult = Enrich(PlayState(Registry({ disabled })), frame);
	iggy::InteractionTargetSpatialQuery2DConfig negativeExtra;
	negativeExtra.extraRadius = -1.0F;
	auto outOfRangeResult =
		Enrich(PlayState(Registry({ outOfRange })), frame, negativeExtra);

	Expect(disabledResult.status == FrameStatus::Unchanged,
		"disabled target should not project hover");
	Expect(outOfRangeResult.status == FrameStatus::Unchanged,
		"out-of-range target should not project hover");
	Expect(disabledResult.target.status == QueryStatus::TargetNotFound,
		"disabled target should report target-not-found");
	Expect(outOfRangeResult.target.status == QueryStatus::TargetNotFound,
		"out-of-range target should report target-not-found");
}

void TestMultiplePrimaryTileEventsUseLatestEligible()
{
	const iggy::InteractionTarget2D first =
		Target("target:first", { 1.5F, 1.5F }, 0.0F);
	const iggy::InteractionTarget2D second =
		Target("target:second", { 4.5F, 4.5F }, 0.0F);
	const auto events = std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> {
		PrimaryTile({ 1, 1 }),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Wait),
		PrimaryTile({ 4, 4 }),
	};
	const auto frame = Frame(events);

	const auto result =
		Enrich(PlayState(Registry({ first, second })), frame);

	Expect(result.status == FrameStatus::TargetProjected,
		"multiple PrimaryTile events should project target from latest eligible tile");
	Expect(result.primaryTileEventIndex == 2,
		"multiple PrimaryTile events should report latest eligible index");
	const iggy::TileCoord expectedTile { 4, 4 };
	Expect(result.primaryTile == expectedTile,
		"multiple PrimaryTile events should report latest eligible tile");
	Expect(result.target.targetId == second.id,
		"multiple PrimaryTile events should query latest eligible tile");
	Expect(result.frame.bindingContext.hoveredTargetId == second.id,
		"multiple PrimaryTile events should project latest target hover");
	ExpectEventsEqual(result.frame.events, events,
		"multiple PrimaryTile events should preserve all events in order");
}

void TestSelectedTargetStillWinsOverProjectedHover()
{
	iggy::PlayerInputBindingContext2D context = BaseContext();
	context.hasSelectedTargetId = true;
	context.selectedTargetId = Id("target:selected");
	const iggy::InteractionTarget2D hovered =
		Target("target:hovered", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) }, context);

	const auto result = Enrich(PlayState(Registry({ hovered })), frame);
	iggy::PlayerInputBindingAction2D action;
	action.type = iggy::PlayerInputBindingAction2DType::Interact;
	const iggy::PlayerInputBinding2DResult binding =
		iggy::PlayerInputBinding2D {}.bind(
			result.frame.bindingContext,
			{ action });

	Expect(result.status == FrameStatus::TargetProjected,
		"selected target test should project hovered target");
	Expect(result.frame.bindingContext.hasSelectedTargetId,
		"target enrichment should preserve selected target flag");
	Expect(result.frame.bindingContext.selectedTargetId == Id("target:selected"),
		"target enrichment should preserve selected target id");
	Expect(binding.intents.size() == 1,
		"targetless interact should bind when selected target exists");
	if (binding.intents.size() == 1) {
		Expect(binding.intents[0].targetId == Id("target:selected"),
			"PlayerInputBinding2D should prefer selected target over projected hover");
	}
}

void TestHoverPreservationAndReplacement()
{
	iggy::PlayerInputBindingContext2D context = BaseContext();
	context.hoveredTargetId = Id("target:existing-hover");
	const iggy::InteractionTarget2D target =
		Target("target:new-hover", { 2.5F, 3.5F }, 0.0F);
	const auto missFrame = Frame({ PrimaryTile({ 8, 8 }) }, context);
	const auto hitFrame = Frame({ PrimaryTile({ 2, 3 }) }, context);

	const auto miss = Enrich(PlayState(Registry({ target })), missFrame);
	const auto hit = Enrich(PlayState(Registry({ target })), hitFrame);

	Expect(miss.status == FrameStatus::Unchanged,
		"miss should leave existing hover unchanged");
	Expect(miss.frame.bindingContext.hoveredTargetId ==
			Id("target:existing-hover"),
		"miss should preserve existing hover id");
	Expect(hit.status == FrameStatus::TargetProjected,
		"hit should project replacement hover");
	Expect(hit.frame.bindingContext.hoveredTargetId == Id("target:new-hover"),
		"hit should replace existing hover id");
}

void TestReachabilityDoesNotGateProjection()
{
	const iggy::InteractionTarget2D target =
		Target("target:reach-report", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	auto reachableState = PlayState(Registry({ target }), true, { 2.5F, 3.5F });
	auto farState = PlayState(Registry({ target }), true, { -20.0F, -20.0F });

	const auto reachable = Enrich(reachableState, frame);
	const auto outOfRange = Enrich(farState, frame);

	Expect(reachable.status == FrameStatus::TargetProjected,
		"reachable target should project hover");
	Expect(outOfRange.status == FrameStatus::TargetProjected,
		"out-of-range target should still project hover");
	Expect(reachable.target.hasReach && reachable.target.reachable,
		"reachable target should preserve reach annotation");
	Expect(outOfRange.target.hasReach && !outOfRange.target.reachable,
		"out-of-range target should preserve reach annotation");
	Expect(outOfRange.frame.bindingContext.hoveredTargetId == target.id,
		"out-of-range target should still set hovered target id");
}

void TestEventsAreCopiedUnchangedAndInOrder()
{
	auto interact = Event(
		iggy::runtime::RuntimeGameplayProductInputControl2D::Interact);
	interact.hasTargetId = true;
	interact.targetId = Id("target:explicit");
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> events {
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::MoveNorth),
		PrimaryTile({ 2, 3 }),
		interact,
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Wait),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Cancel),
	};
	const iggy::InteractionTarget2D target =
		Target("target:copy-check", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame(events);

	const auto result = Enrich(PlayState(Registry({ target })), frame);

	Expect(result.status == FrameStatus::TargetProjected,
		"copy check setup should project hover");
	ExpectEventsEqual(result.frame.events, events,
		"target context enrichment should preserve event order and payloads");
}

void TestInputFrameAndPlayStateAreNotMutated()
{
	const iggy::InteractionTarget2D target =
		Target("target:immutable", { 2.5F, 3.5F }, 0.0F);
	const auto registry = Registry({ target });
	auto state = PlayState(registry, true, { 2.5F, 3.5F });
	auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	const auto originalFrame = frame;

	const auto result = Enrich(state, frame);

	Expect(result.status == FrameStatus::TargetProjected,
		"immutability setup should project hover");
	ExpectContextEquals(frame.bindingContext, originalFrame.bindingContext,
		"enrichment should not mutate input frame context");
	ExpectEventsEqual(frame.events, originalFrame.events,
		"enrichment should not mutate input frame events");
	Expect(state.loop.loaded,
		"enrichment should not mutate play state loaded flag");
	Expect(state.loop.currentState.interaction.targets.targets().size() == 1,
		"enrichment should not mutate play state target registry");
	if (state.loop.currentState.interaction.targets.targets().size() == 1) {
		Expect(state.loop.currentState.interaction.targets.targets()[0].id ==
				target.id,
			"enrichment should not mutate play state target payload");
	}
}

void TestEnrichedFrameAllowsTargetlessInteractToUseHoveredFallback()
{
	iggy::PlayerInputBindingContext2D context = BaseContext();
	context.hasSelectedTargetId = false;
	context.selectedTargetId = {};
	const iggy::InteractionTarget2D target =
		Target("target:hovered-fallback", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({
		PrimaryTile({ 2, 3 }),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Interact),
	}, context);

	const auto enriched = Enrich(PlayState(Registry({ target })), frame);
	const auto adapter =
		iggy::runtime::RuntimeGameplayProductInputAdapter {}.map(
			enriched.frame);
	const auto binding = iggy::PlayerInputBinding2D {}.bind(
		adapter.bindingContext,
		adapter.actions);

	Expect(enriched.status == FrameStatus::TargetProjected,
		"hover fallback setup should project target context");
	Expect(adapter.actions.size() == 2,
		"hover fallback setup should preserve adapter action behavior");
	Expect(binding.intents.size() == 2,
		"enriched frame should allow targetless interact to bind via hover fallback");
	if (binding.intents.size() == 2) {
		Expect(binding.intents[1].type == iggy::PlayerInputIntent2DType::Interact,
			"second bound intent should be interact");
		Expect(binding.intents[1].targetId == target.id,
			"targetless interact should use projected hovered target fallback");
	}
}

} // namespace

int main()
{
	TestNoPrimaryTileReturnsNoEligibleAndPreservesFrame();
	TestPrimaryTileMissingPayloadAndReleaseAreIneligible();
	TestPressedPrimaryTileOnTargetProjectsHoveredTarget();
	TestPressedPrimaryTileMissPreservesContextWithDiagnostics();
	TestDisabledAndOutOfRangeTargetsDoNotProjectHover();
	TestMultiplePrimaryTileEventsUseLatestEligible();
	TestSelectedTargetStillWinsOverProjectedHover();
	TestHoverPreservationAndReplacement();
	TestReachabilityDoesNotGateProjection();
	TestEventsAreCopiedUnchangedAndInOrder();
	TestInputFrameAndPlayStateAreNotMutated();
	TestEnrichedFrameAllowsTargetlessInteractToUseHoveredFallback();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
