#include "runtime/RuntimeGameplayProductInputAdapter.hpp"

#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputBinding2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;
using EventKind = iggy::runtime::RuntimeGameplayProductInputEventKind;
using IssueCode = iggy::runtime::RuntimeGameplayProductInputAdapterIssueCode;
using BindingActionType = iggy::PlayerInputBindingAction2DType;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayProductInputEvent2D Event(Control control)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D Released(Control control)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event = Event(control);
	event.kind = EventKind::Released;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D TargetEvent(
	Control control,
	iggy::ResourceId targetId)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event = Event(control);
	event.hasTargetId = true;
	event.targetId = targetId;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D TileEvent(
	iggy::TileCoord tile)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event =
		Event(Control::PrimaryTile);
	event.hasTile = true;
	event.tile = tile;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D PointEvent(iggy::Vec2 point)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event =
		Event(Control::PrimaryPoint);
	event.hasWorldPoint = true;
	event.worldPoint = point;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputAdapterResult Map(
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &frame)
{
	return iggy::runtime::RuntimeGameplayProductInputAdapter {}.map(frame);
}

bool SameBindingContext(
	const iggy::PlayerInputBindingContext2D &actual,
	const iggy::PlayerInputBindingContext2D &expected)
{
	return actual.input.playerControlEnabled ==
			expected.input.playerControlEnabled
		&& actual.input.worldInputEnabled ==
			expected.input.worldInputEnabled
		&& actual.input.interactionEnabled ==
			expected.input.interactionEnabled
		&& actual.input.cancelEnabled == expected.input.cancelEnabled
		&& actual.hasCurrentPlayerTile == expected.hasCurrentPlayerTile
		&& actual.currentPlayerTile == expected.currentPlayerTile
		&& actual.hasSelectedTargetId == expected.hasSelectedTargetId
		&& actual.selectedTargetId == expected.selectedTargetId
		&& actual.hasHoveredTargetId == expected.hasHoveredTargetId
		&& actual.hoveredTargetId == expected.hoveredTargetId;
}

bool SameEvent(
	const iggy::runtime::RuntimeGameplayProductInputEvent2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputEvent2D &expected)
{
	return actual.control == expected.control && actual.kind == expected.kind
		&& actual.hasTile == expected.hasTile && actual.tile == expected.tile
		&& actual.hasWorldPoint == expected.hasWorldPoint
		&& NearVec(actual.worldPoint, expected.worldPoint)
		&& actual.hasTargetId == expected.hasTargetId
		&& actual.targetId == expected.targetId;
}

bool SameEvents(
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> &actual,
	const std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D>
		&expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEvent(actual[index], expected[index]))
			return false;
	}
	return true;
}

bool SameFrame(
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &actual,
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &expected)
{
	return SameBindingContext(actual.bindingContext, expected.bindingContext)
		&& SameEvents(actual.events, expected.events);
}

void ExpectAction(
	const iggy::PlayerInputBindingAction2D &actual,
	BindingActionType type,
	const char *message)
{
	Expect(actual.type == type, message);
}

void ExpectIntentEquals(
	const iggy::PlayerInputIntent2D &actual,
	const iggy::PlayerInputIntent2D &expected,
	const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void TestCardinalPressedEventsProduceOrderedTileDeltas()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		Event(Control::MoveNorth),
		Event(Control::MoveSouth),
		Event(Control::MoveWest),
		Event(Control::MoveEast),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.eventCount == 4,
		"cardinal mapping should preserve event count");
	Expect(result.emittedActionCount == 4 && result.actions.size() == 4,
		"cardinal mapping should emit four actions");
	Expect(!result.hasIssues(), "cardinal mapping should not report issues");
	if (result.actions.size() == 4) {
		ExpectAction(
			result.actions[0],
			BindingActionType::MoveByTileDelta,
			"MoveNorth should emit MoveByTileDelta");
		Expect(result.actions[0].tileDelta == iggy::TileCoord { 0, -1 },
			"MoveNorth should target north tile delta");
		Expect(result.actions[1].tileDelta == iggy::TileCoord { 0, 1 },
			"MoveSouth should target south tile delta");
		Expect(result.actions[2].tileDelta == iggy::TileCoord { -1, 0 },
			"MoveWest should target west tile delta");
		Expect(result.actions[3].tileDelta == iggy::TileCoord { 1, 0 },
			"MoveEast should target east tile delta");
	}
}

void TestInteractInspectWaitAndCancelPressedEvents()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		TargetEvent(Control::Interact, Id("target:door")),
		TargetEvent(Control::Inspect, Id("target:sign")),
		Event(Control::Wait),
		Event(Control::Cancel),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.actions.size() == 4,
		"interact inspect wait cancel should emit four actions");
	Expect(!result.hasIssues(),
		"interact inspect wait cancel should not report issues");
	if (result.actions.size() == 4) {
		Expect(result.actions[0].type == BindingActionType::Interact &&
				result.actions[0].targetId == Id("target:door"),
			"Interact should copy supplied target id");
		Expect(result.actions[1].type == BindingActionType::Inspect &&
				result.actions[1].targetId == Id("target:sign"),
			"Inspect should copy supplied target id");
		ExpectAction(
			result.actions[2],
			BindingActionType::Wait,
			"Wait should emit binding wait action");
		ExpectAction(
			result.actions[3],
			BindingActionType::Cancel,
			"Cancel should emit binding cancel action");
	}
}

void TestInteractInspectWithoutTargetsRemainAdapterLevelActions()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		Event(Control::Interact),
		Event(Control::Inspect),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.actions.size() == 2,
		"targetless interact inspect should still emit binding actions");
	Expect(!result.hasIssues(),
		"targetless interact inspect should not be adapter issues");
	if (result.actions.size() == 2) {
		Expect(result.actions[0].type == BindingActionType::Interact &&
				result.actions[0].targetId.empty(),
			"targetless Interact should leave target empty");
		Expect(result.actions[1].type == BindingActionType::Inspect &&
				result.actions[1].targetId.empty(),
			"targetless Inspect should leave target empty");
	}
}

void TestReleasedEventsAreIgnored()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		Released(Control::MoveNorth),
		Released(Control::Interact),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.eventCount == 2,
		"released events should preserve event count");
	Expect(result.ignoredReleaseCount == 2,
		"released events should increment ignored release count");
	Expect(result.actions.empty() && result.emittedActionCount == 0,
		"released events should emit no actions");
	Expect(!result.hasIssues(), "released events should not report issues");
}

void TestUnknownControlReportsUnsupportedControl()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		Event(Control::MoveNorth),
		Event(static_cast<Control>(999)),
		Event(Control::MoveSouth),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.actions.size() == 2,
		"unsupported control should not emit an action");
	Expect(result.hasIssues() && result.issueCount == 1,
		"unsupported control should report one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].eventIndex == 1,
			"unsupported control should preserve event index");
		Expect(result.issues[0].code == IssueCode::UnsupportedControl,
			"unsupported control should use UnsupportedControl issue code");
	}
}

void TestPrimaryTileAndPointPayloads()
{
	const iggy::Vec2 point { -1.5F, 7.25F };
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		TileEvent({ 4, -3 }),
		Event(Control::PrimaryTile),
		PointEvent(point),
		Event(Control::PrimaryPoint),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.actions.size() == 2,
		"valid primary tile and point events should emit two actions");
	Expect(result.hasIssues() && result.issueCount == 2,
		"missing primary payload flags should report two issues");
	if (result.actions.size() == 2) {
		Expect(result.actions[0].type == BindingActionType::MoveToTile &&
				result.actions[0].tile == iggy::TileCoord { 4, -3 },
			"PrimaryTile with payload should emit MoveToTile");
		Expect(result.actions[1].type == BindingActionType::MoveToPoint &&
				NearVec(result.actions[1].worldPoint, point),
			"PrimaryPoint with payload should emit MoveToPoint");
	}
	if (result.issues.size() == 2) {
		Expect(result.issues[0].eventIndex == 1 &&
				result.issues[0].code == IssueCode::MissingTile,
			"PrimaryTile without payload should report MissingTile");
		Expect(result.issues[1].eventIndex == 3 &&
				result.issues[1].code == IssueCode::MissingWorldPoint,
			"PrimaryPoint without payload should report MissingWorldPoint");
	}
}

void TestCopiesBindingContextUnchanged()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext.input.playerControlEnabled = false;
	frame.bindingContext.input.worldInputEnabled = false;
	frame.bindingContext.input.interactionEnabled = false;
	frame.bindingContext.input.cancelEnabled = false;
	frame.bindingContext.hasCurrentPlayerTile = true;
	frame.bindingContext.currentPlayerTile = { 2, 3 };
	frame.bindingContext.hasSelectedTargetId = true;
	frame.bindingContext.selectedTargetId = Id("target:selected");
	frame.bindingContext.hasHoveredTargetId = true;
	frame.bindingContext.hoveredTargetId = Id("target:hovered");
	frame.events = { Event(Control::Wait) };

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(SameBindingContext(result.bindingContext, frame.bindingContext),
		"adapter should copy binding context unchanged");
}

void TestMultipleEventsPreserveActionOrderWhileSkipping()
{
	const iggy::Vec2 point { 6.0F, -2.0F };
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.events = {
		Event(Control::MoveNorth),
		Released(Control::MoveSouth),
		Event(Control::None),
		Event(Control::PrimaryTile),
		Event(Control::Wait),
		PointEvent(point),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.eventCount == 6,
		"mixed event mapping should preserve event count");
	Expect(result.ignoredReleaseCount == 1,
		"mixed event mapping should count release skips");
	Expect(result.issueCount == 1,
		"mixed event mapping should count payload issue");
	Expect(result.actions.size() == 3,
		"mixed event mapping should emit actions excluding release no-op issue");
	if (result.actions.size() == 3) {
		Expect(result.actions[0].type == BindingActionType::MoveByTileDelta &&
				result.actions[0].tileDelta == iggy::TileCoord { 0, -1 },
			"first emitted mixed action should be north movement");
		Expect(result.actions[1].type == BindingActionType::Wait,
			"second emitted mixed action should be wait");
		Expect(result.actions[2].type == BindingActionType::MoveToPoint &&
				NearVec(result.actions[2].worldPoint, point),
			"third emitted mixed action should be point movement");
	}
}

void TestAdapterDoesNotMutateInputFrame()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext.input.worldInputEnabled = false;
	frame.bindingContext.hasCurrentPlayerTile = true;
	frame.bindingContext.currentPlayerTile = { 8, 9 };
	frame.bindingContext.hasSelectedTargetId = true;
	frame.bindingContext.selectedTargetId = Id("target:selected");
	frame.events = {
		Event(Control::MoveEast),
		TargetEvent(Control::Interact, Id("target:explicit")),
		TileEvent({ 1, 2 }),
	};
	const iggy::runtime::RuntimeGameplayProductInputFrame2D before = frame;

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult result =
		Map(frame);

	Expect(result.actions.size() == 3,
		"immutability setup should emit three actions");
	Expect(SameFrame(frame, before),
		"adapter should not mutate input frame events or context");
}

void TestAdapterActionsBindToPlayerInputIntents()
{
	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext.hasCurrentPlayerTile = true;
	frame.bindingContext.currentPlayerTile = { 2, 3 };
	frame.bindingContext.hasSelectedTargetId = true;
	frame.bindingContext.selectedTargetId = Id("target:selected");
	frame.events = {
		Event(Control::MoveEast),
		Event(Control::Interact),
	};

	const iggy::runtime::RuntimeGameplayProductInputAdapterResult adapter =
		Map(frame);
	const iggy::PlayerInputBinding2DResult binding =
		iggy::PlayerInputBinding2D {}.bind(
			adapter.bindingContext,
			adapter.actions);

	Expect(adapter.actions.size() == 2,
		"compatibility setup should emit two binding actions");
	Expect(binding.intents.size() == 2,
		"binding should convert adapter actions into two intents");
	Expect(!binding.hasIssues(),
		"selected target fallback should satisfy binding target resolution");
	if (binding.intents.size() == 2) {
		ExpectIntentEquals(
			binding.intents[0],
			iggy::playerMoveToTileIntent({ 3, 3 }),
			"cardinal adapter action should bind relative to current tile");
		ExpectIntentEquals(
			binding.intents[1],
			iggy::playerInteractIntent(Id("target:selected")),
			"targetless adapter interact should use binding selected fallback");
	}
}

} // namespace

int main()
{
	TestCardinalPressedEventsProduceOrderedTileDeltas();
	TestInteractInspectWaitAndCancelPressedEvents();
	TestInteractInspectWithoutTargetsRemainAdapterLevelActions();
	TestReleasedEventsAreIgnored();
	TestUnknownControlReportsUnsupportedControl();
	TestPrimaryTileAndPointPayloads();
	TestCopiesBindingContextUnchanged();
	TestMultipleEventsPreserveActionOrderWhileSkipping();
	TestAdapterDoesNotMutateInputFrame();
	TestAdapterActionsBindToPlayerInputIntents();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
