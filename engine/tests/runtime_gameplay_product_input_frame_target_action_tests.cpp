#include "runtime/RuntimeGameplayProductInputFrameTargetAction.hpp"

#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputBinding2D.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using ActionStatus =
	iggy::runtime::RuntimeGameplayProductInputFrameTargetActionStatus;
using ContextStatus =
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextStatus;
using QueryStatus =
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus;

const iggy::ResourceId PlayerId { "player:frame-target-action" };

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
		"input frame target action registry setup should build");
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
	context.input.worldInputEnabled = true;
	context.input.interactionEnabled = true;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 4, -1 };
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
	iggy::InteractionReach2DConfig reachConfig = {})
{
	return iggy::runtime::RuntimeGameplayProductInputFrameTargetContext {}.enrich({
		state,
		frame,
		{},
		reachConfig,
	});
}

iggy::runtime::RuntimeGameplayProductInputFrameTargetActionResult Synthesize(
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult targetContext)
{
	return iggy::runtime::RuntimeGameplayProductInputFrameTargetAction {}.synthesize(
		{ targetContext });
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

void TestNoEligiblePrimaryTileLeavesFrameUnchanged()
{
	const auto frame = Frame({
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::MoveEast),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Wait),
	});
	const auto targetContext = Enrich(PlayState(Registry({})), frame);

	const auto result = Synthesize(targetContext);

	Expect(targetContext.status == ContextStatus::NoEligiblePrimaryTile,
		"no eligible primary tile setup should report no eligible tile");
	Expect(result.status == ActionStatus::Unchanged,
		"no eligible primary tile should not synthesize interact");
	Expect(!result.hasActionEvent,
		"no eligible primary tile should not report action event");
	ExpectEventsEqual(result.frame.events, frame.events,
		"no eligible primary tile should preserve events");
}

void TestTargetFoundReplacesPrimaryTileWithExplicitInteract()
{
	const iggy::InteractionTarget2D target =
		Target("target:door", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	const auto targetContext = Enrich(PlayState(Registry({ target })), frame);

	const auto result = Synthesize(targetContext);

	Expect(targetContext.status == ContextStatus::TargetProjected,
		"target-found action setup should project target context");
	Expect(result.status == ActionStatus::InteractSynthesized,
		"target-found click should synthesize interact");
	Expect(result.hasActionEvent && result.actionEventIndex == 0,
		"target-found click should report replaced event index");
	Expect(result.targetId == target.id,
		"target-found click should report target id");
	Expect(result.frame.events.size() == 1,
		"target-found click should preserve event count");
	if (result.frame.events.size() == 1) {
		const auto &event = result.frame.events[0];
		Expect(event.control ==
				iggy::runtime::RuntimeGameplayProductInputControl2D::Interact,
			"target-found click should replace PrimaryTile with Interact");
		Expect(event.kind ==
				iggy::runtime::RuntimeGameplayProductInputEventKind::Pressed,
			"target-found click should synthesize pressed Interact");
		Expect(event.hasTargetId && event.targetId == target.id,
			"target-found click should synthesize explicit target id");
		Expect(!event.hasTile,
			"synthesized Interact should not preserve PrimaryTile tile payload");
	}
}

void TestMissAndInvalidFoundLeaveFrameUnchanged()
{
	const iggy::InteractionTarget2D target =
		Target("target:far", { 20.5F, 20.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	const auto missContext = Enrich(PlayState(Registry({ target })), frame);
	auto invalidFound = missContext;
	invalidFound.status = ContextStatus::TargetProjected;
	invalidFound.target.status = QueryStatus::TargetFound;
	invalidFound.target.hasTarget = false;
	invalidFound.target.targetId = {};

	const auto miss = Synthesize(missContext);
	const auto invalid = Synthesize(invalidFound);

	Expect(missContext.status == ContextStatus::Unchanged,
		"miss setup should not project target context");
	Expect(miss.status == ActionStatus::Unchanged,
		"target miss should not synthesize interact");
	Expect(invalid.status == ActionStatus::Unchanged,
		"invalid found target should not synthesize interact");
	ExpectEventsEqual(miss.frame.events, frame.events,
		"target miss should preserve events");
	ExpectEventsEqual(invalid.frame.events, frame.events,
		"invalid found target should preserve events");
}

void TestLatestPrimaryTileEventIsReplacedAndOrderIsPreserved()
{
	const iggy::InteractionTarget2D first =
		Target("target:first", { 1.5F, 1.5F }, 0.0F);
	const iggy::InteractionTarget2D second =
		Target("target:second", { 4.5F, 4.5F }, 0.0F);
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> events {
		PrimaryTile({ 1, 1 }),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Wait),
		PrimaryTile({ 4, 4 }),
		Event(iggy::runtime::RuntimeGameplayProductInputControl2D::Cancel),
	};
	const auto frame = Frame(events);
	const auto targetContext =
		Enrich(PlayState(Registry({ first, second })), frame);

	const auto result = Synthesize(targetContext);

	Expect(targetContext.primaryTileEventIndex == 2,
		"latest primary tile setup should choose latest eligible event");
	Expect(result.status == ActionStatus::InteractSynthesized,
		"latest primary tile should synthesize interact");
	Expect(result.actionEventIndex == 2,
		"latest primary tile should report replaced event index");
	Expect(result.frame.events.size() == events.size(),
		"latest primary tile should preserve event count");
	if (result.frame.events.size() == events.size()) {
		ExpectEventEquals(result.frame.events[0], events[0],
			"latest primary tile should preserve earlier PrimaryTile event");
		ExpectEventEquals(result.frame.events[1], events[1],
			"latest primary tile should preserve preceding event");
		Expect(result.frame.events[2].control ==
				iggy::runtime::RuntimeGameplayProductInputControl2D::Interact &&
				result.frame.events[2].targetId == second.id,
			"latest primary tile should replace chosen event with target Interact");
		ExpectEventEquals(result.frame.events[3], events[3],
			"latest primary tile should preserve following event");
	}
}

void TestReachabilityDoesNotGateInteractSynthesis()
{
	const iggy::InteractionTarget2D target =
		Target("target:out-of-reach", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	const auto targetContext = Enrich(
		PlayState(Registry({ target }), true, { -20.0F, -20.0F }),
		frame);

	const auto result = Synthesize(targetContext);

	Expect(targetContext.target.hasReach && !targetContext.target.reachable,
		"out-of-reach setup should preserve reach diagnostic");
	Expect(result.status == ActionStatus::InteractSynthesized,
		"out-of-reach target should still synthesize interact");
	Expect(result.targetId == target.id,
		"out-of-reach target should preserve target id");
}

void TestSynthesizedFrameMapsAndBindsExplicitInteractOnly()
{
	const iggy::InteractionTarget2D target =
		Target("target:actionable", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	const auto targetContext = Enrich(PlayState(Registry({ target })), frame);

	const auto result = Synthesize(targetContext);
	const auto adapter =
		iggy::runtime::RuntimeGameplayProductInputAdapter {}.map(result.frame);
	const auto binding = iggy::PlayerInputBinding2D {}.bind(
		adapter.bindingContext,
		adapter.actions);

	Expect(result.status == ActionStatus::InteractSynthesized,
		"explicit interact setup should synthesize target action");
	Expect(adapter.eventCount == 1 && adapter.emittedActionCount == 1,
		"synthesized frame should map exactly one event/action");
	Expect(adapter.actions.size() == 1 &&
			adapter.actions[0].type ==
				iggy::PlayerInputBindingAction2DType::Interact &&
			adapter.actions[0].targetId == target.id,
		"synthesized frame should map explicit target Interact action");
	Expect(binding.intents.size() == 1 &&
			binding.intents[0].type == iggy::PlayerInputIntent2DType::Interact &&
			binding.intents[0].targetId == target.id,
		"synthesized frame should bind directly to target Interact intent");
}

void TestInputTargetContextIsNotMutated()
{
	const iggy::InteractionTarget2D target =
		Target("target:immutable", { 2.5F, 3.5F }, 0.0F);
	const auto frame = Frame({ PrimaryTile({ 2, 3 }) });
	const auto targetContext = Enrich(PlayState(Registry({ target })), frame);
	const auto originalEvents = targetContext.frame.events;

	const auto result = Synthesize(targetContext);

	Expect(result.status == ActionStatus::InteractSynthesized,
		"immutability setup should synthesize target action");
	ExpectEventsEqual(targetContext.frame.events, originalEvents,
		"target action synthesis should not mutate input target-context result");
}

} // namespace

int main()
{
	TestNoEligiblePrimaryTileLeavesFrameUnchanged();
	TestTargetFoundReplacesPrimaryTileWithExplicitInteract();
	TestMissAndInvalidFoundLeaveFrameUnchanged();
	TestLatestPrimaryTileEventIsReplacedAndOrderIsPreserved();
	TestReachabilityDoesNotGateInteractSynthesis();
	TestSynthesizedFrameMapsAndBindsExplicitInteractOnly();
	TestInputTargetContextIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
