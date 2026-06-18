#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputBinding2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
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

bool SameContext(
	const iggy::PlayerInputContext2D &actual,
	const iggy::PlayerInputContext2D &expected)
{
	return actual.playerControlEnabled == expected.playerControlEnabled
		&& actual.worldInputEnabled == expected.worldInputEnabled
		&& actual.interactionEnabled == expected.interactionEnabled
		&& actual.cancelEnabled == expected.cancelEnabled;
}

bool SameAction(
	const iggy::PlayerInputBindingAction2D &actual,
	const iggy::PlayerInputBindingAction2D &expected)
{
	return actual.type == expected.type
		&& NearVec(actual.worldPoint, expected.worldPoint)
		&& actual.tile == expected.tile
		&& actual.tileDelta == expected.tileDelta
		&& actual.targetId == expected.targetId;
}

bool SameActions(
	const std::vector<iggy::PlayerInputBindingAction2D> &actual,
	const std::vector<iggy::PlayerInputBindingAction2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameAction(actual[index], expected[index]))
			return false;
	}
	return true;
}

iggy::PlayerInputBinding2DResult Bind(
	const iggy::PlayerInputBindingContext2D &context,
	const std::vector<iggy::PlayerInputBindingAction2D> &actions)
{
	return iggy::PlayerInputBinding2D {}.bind(context, actions);
}

iggy::PlayerInputBindingAction2D Action(
	iggy::PlayerInputBindingAction2DType type)
{
	iggy::PlayerInputBindingAction2D action;
	action.type = type;
	return action;
}

iggy::PlayerInputBindingAction2D MovePoint(iggy::Vec2 point)
{
	iggy::PlayerInputBindingAction2D action =
		Action(iggy::PlayerInputBindingAction2DType::MoveToPoint);
	action.worldPoint = point;
	return action;
}

iggy::PlayerInputBindingAction2D MoveTile(iggy::TileCoord tile)
{
	iggy::PlayerInputBindingAction2D action =
		Action(iggy::PlayerInputBindingAction2DType::MoveToTile);
	action.tile = tile;
	return action;
}

iggy::PlayerInputBindingAction2D MoveDelta(iggy::TileCoord delta)
{
	iggy::PlayerInputBindingAction2D action =
		Action(iggy::PlayerInputBindingAction2DType::MoveByTileDelta);
	action.tileDelta = delta;
	return action;
}

iggy::PlayerInputBindingAction2D TargetAction(
	iggy::PlayerInputBindingAction2DType type,
	iggy::ResourceId targetId)
{
	iggy::PlayerInputBindingAction2D action = Action(type);
	action.targetId = targetId;
	return action;
}

void TestMoveToTileMapsToIntent()
{
	const iggy::PlayerInputBinding2DResult result =
		Bind({}, { MoveTile({ -4, 7 }) });

	Expect(result.actionCount == 1, "MoveToTile action count should be preserved");
	Expect(result.emittedIntentCount == 1 && result.intents.size() == 1,
		"MoveToTile should emit one intent");
	Expect(!result.hasIssues(), "MoveToTile should not report issues");
	if (!result.intents.empty()) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerMoveToTileIntent({ -4, 7 }),
			"MoveToTile should map to playerMoveToTileIntent");
	}
}

void TestMoveToPointMapsToIntent()
{
	const iggy::Vec2 point { -3.5F, 9.25F };
	const iggy::PlayerInputBinding2DResult result =
		Bind({}, { MovePoint(point) });

	Expect(result.emittedIntentCount == 1 && result.intents.size() == 1,
		"MoveToPoint should emit one intent");
	if (!result.intents.empty()) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerMoveToPointIntent(point),
			"MoveToPoint should map to playerMoveToPointIntent");
	}
}

void TestMoveByTileDeltaUsesCurrentTileIncludingNegativeDelta()
{
	iggy::PlayerInputBindingContext2D context;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 3, 4 };

	const iggy::PlayerInputBinding2DResult result =
		Bind(context, { MoveDelta({ -2, 5 }) });

	Expect(result.emittedIntentCount == 1 && result.intents.size() == 1,
		"MoveByTileDelta with current tile should emit one intent");
	Expect(!result.hasIssues(),
		"MoveByTileDelta with current tile should not report issues");
	if (!result.intents.empty()) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerMoveToTileIntent({ 1, 9 }),
			"MoveByTileDelta should target current tile plus delta");
	}
}

void TestMoveByTileDeltaWithoutCurrentTileReportsIssue()
{
	const iggy::PlayerInputBinding2DResult result =
		Bind({}, { MoveDelta({ 1, 0 }) });

	Expect(result.emittedIntentCount == 0 && result.intents.empty(),
		"MoveByTileDelta without current tile should emit no intents");
	Expect(result.hasIssues() && result.issueCount == 1,
		"MoveByTileDelta without current tile should report one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].actionIndex == 0,
			"MoveByTileDelta missing-current-tile issue should preserve action index");
		Expect(result.issues[0].code ==
				iggy::PlayerInputBindingIssueCode::MissingCurrentPlayerTile,
			"MoveByTileDelta should report MissingCurrentPlayerTile");
	}
}

void TestInteractAndInspectUseExplicitTarget()
{
	const iggy::PlayerInputBinding2DResult result = Bind(
		{},
		{
			TargetAction(
				iggy::PlayerInputBindingAction2DType::Interact,
				Id("target:door")),
			TargetAction(
				iggy::PlayerInputBindingAction2DType::Inspect,
				Id("target:sign")),
		});

	Expect(result.emittedIntentCount == 2 && result.intents.size() == 2,
		"explicit interact and inspect should emit two intents");
	Expect(!result.hasIssues(),
		"explicit interact and inspect targets should not report issues");
	if (result.intents.size() == 2) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerInteractIntent(Id("target:door")),
			"Interact should preserve explicit target");
		ExpectIntentEquals(
			result.intents[1],
			iggy::playerInspectIntent(Id("target:sign")),
			"Inspect should preserve explicit target");
	}
}

void TestInteractAndInspectFallbackToSelectedThenHoveredTarget()
{
	iggy::PlayerInputBindingContext2D selected;
	selected.hasSelectedTargetId = true;
	selected.selectedTargetId = Id("target:selected");
	selected.hasHoveredTargetId = true;
	selected.hoveredTargetId = Id("target:hovered");

	const iggy::PlayerInputBinding2DResult selectedResult = Bind(
		selected,
		{
			Action(iggy::PlayerInputBindingAction2DType::Interact),
			Action(iggy::PlayerInputBindingAction2DType::Inspect),
		});

	Expect(selectedResult.intents.size() == 2,
		"selected target fallback should emit two intents");
	if (selectedResult.intents.size() == 2) {
		ExpectIntentEquals(
			selectedResult.intents[0],
			iggy::playerInteractIntent(Id("target:selected")),
			"Interact should prefer selected target over hovered target");
		ExpectIntentEquals(
			selectedResult.intents[1],
			iggy::playerInspectIntent(Id("target:selected")),
			"Inspect should prefer selected target over hovered target");
	}

	iggy::PlayerInputBindingContext2D hovered;
	hovered.hasHoveredTargetId = true;
	hovered.hoveredTargetId = Id("target:hovered");

	const iggy::PlayerInputBinding2DResult hoveredResult = Bind(
		hovered,
		{
			Action(iggy::PlayerInputBindingAction2DType::Interact),
			Action(iggy::PlayerInputBindingAction2DType::Inspect),
		});

	Expect(hoveredResult.intents.size() == 2,
		"hovered target fallback should emit two intents");
	if (hoveredResult.intents.size() == 2) {
		ExpectIntentEquals(
			hoveredResult.intents[0],
			iggy::playerInteractIntent(Id("target:hovered")),
			"Interact should use hovered target when selected target is absent");
		ExpectIntentEquals(
			hoveredResult.intents[1],
			iggy::playerInspectIntent(Id("target:hovered")),
			"Inspect should use hovered target when selected target is absent");
	}
}

void TestInteractAndInspectWithoutTargetReportMissingTarget()
{
	const iggy::PlayerInputBinding2DResult result = Bind(
		{},
		{
			Action(iggy::PlayerInputBindingAction2DType::Interact),
			Action(iggy::PlayerInputBindingAction2DType::Inspect),
		});

	Expect(result.intents.empty() && result.emittedIntentCount == 0,
		"missing target actions should emit no intents");
	Expect(result.hasIssues() && result.issueCount == 2,
		"missing target actions should report two issues");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].actionIndex == 0 &&
				result.issues[0].code ==
					iggy::PlayerInputBindingIssueCode::MissingTarget,
			"Interact without target should report MissingTarget");
		Expect(result.issues[1].actionIndex == 1 &&
				result.issues[1].code ==
					iggy::PlayerInputBindingIssueCode::MissingTarget,
			"Inspect without target should report MissingTarget");
	}
}

void TestWaitCancelAndNone()
{
	const iggy::PlayerInputBinding2DResult result = Bind(
		{},
		{
			Action(iggy::PlayerInputBindingAction2DType::None),
			Action(iggy::PlayerInputBindingAction2DType::Wait),
			Action(iggy::PlayerInputBindingAction2DType::Cancel),
		});

	Expect(result.actionCount == 3,
		"None/Wait/Cancel should preserve action count");
	Expect(result.emittedIntentCount == 2 && result.intents.size() == 2,
		"None should be a no-op while Wait and Cancel emit intents");
	Expect(!result.hasIssues(), "None should not report an issue");
	if (result.intents.size() == 2) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerWaitIntent(),
			"Wait should map to playerWaitIntent");
		ExpectIntentEquals(
			result.intents[1],
			iggy::playerCancelIntent(),
			"Cancel should map to playerCancelIntent");
	}
}

void TestBindingPreservesIntentOrderExcludingNoOpsAndIssues()
{
	iggy::PlayerInputBindingContext2D context;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 5, 5 };
	context.hasHoveredTargetId = true;
	context.hoveredTargetId = Id("target:hovered");

	const iggy::PlayerInputBinding2DResult result = Bind(
		context,
		{
			MoveTile({ 1, 1 }),
			Action(iggy::PlayerInputBindingAction2DType::None),
			MoveDelta({ -1, 2 }),
			TargetAction(
				iggy::PlayerInputBindingAction2DType::Interact,
				{}),
			MoveDelta({ 1, -1 }),
		});

	Expect(result.intents.size() == 4,
		"binding should preserve emitted intent order and skip no-op only");
	if (result.intents.size() == 4) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerMoveToTileIntent({ 1, 1 }),
			"first emitted intent should be direct tile move");
		ExpectIntentEquals(
			result.intents[1],
			iggy::playerMoveToTileIntent({ 4, 7 }),
			"second emitted intent should be tile delta move");
		ExpectIntentEquals(
			result.intents[2],
			iggy::playerInteractIntent(Id("target:hovered")),
			"third emitted intent should use fallback interact target");
		ExpectIntentEquals(
			result.intents[3],
			iggy::playerMoveToTileIntent({ 6, 4 }),
			"fourth emitted intent should be final tile delta move");
	}
}

void TestBindingCarriesContextButDoesNotGate()
{
	iggy::PlayerInputBindingContext2D context;
	context.input.playerControlEnabled = false;
	context.input.worldInputEnabled = false;
	context.input.interactionEnabled = false;
	context.input.cancelEnabled = false;

	const iggy::PlayerInputBinding2DResult result =
		Bind(context, { MoveTile({ 3, 4 }) });

	Expect(SameContext(result.inputContext, context.input),
		"binding should copy input context unchanged");
	Expect(result.intents.size() == 1,
		"binding should emit movement intent even when world input is disabled");
	if (!result.intents.empty()) {
		ExpectIntentEquals(
			result.intents[0],
			iggy::playerMoveToTileIntent({ 3, 4 }),
			"binding should not apply gate rules");
	}
}

void TestBindingDoesNotMutateInputs()
{
	iggy::PlayerInputBindingContext2D context;
	context.input.worldInputEnabled = false;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 1, 2 };
	context.hasSelectedTargetId = true;
	context.selectedTargetId = Id("target:selected");
	const iggy::PlayerInputBindingContext2D contextBefore = context;
	std::vector<iggy::PlayerInputBindingAction2D> actions {
		MoveDelta({ 2, 3 }),
		Action(iggy::PlayerInputBindingAction2DType::Interact),
	};
	const std::vector<iggy::PlayerInputBindingAction2D> actionsBefore =
		actions;

	const iggy::PlayerInputBinding2DResult result = Bind(context, actions);

	Expect(result.intents.size() == 2,
		"immutability setup should emit two intents");
	Expect(SameContext(context.input, contextBefore.input) &&
			context.hasCurrentPlayerTile == contextBefore.hasCurrentPlayerTile &&
			context.currentPlayerTile == contextBefore.currentPlayerTile &&
			context.hasSelectedTargetId == contextBefore.hasSelectedTargetId &&
			context.selectedTargetId == contextBefore.selectedTargetId,
		"binding should not mutate context");
	Expect(SameActions(actions, actionsBefore),
		"binding should not mutate action vector");
}

} // namespace

int main()
{
	TestMoveToTileMapsToIntent();
	TestMoveToPointMapsToIntent();
	TestMoveByTileDeltaUsesCurrentTileIncludingNegativeDelta();
	TestMoveByTileDeltaWithoutCurrentTileReportsIssue();
	TestInteractAndInspectUseExplicitTarget();
	TestInteractAndInspectFallbackToSelectedThenHoveredTarget();
	TestInteractAndInspectWithoutTargetReportMissingTarget();
	TestWaitCancelAndNone();
	TestBindingPreservesIntentOrderExcludingNoOpsAndIssues();
	TestBindingCarriesContextButDoesNotGate();
	TestBindingDoesNotMutateInputs();

	if (Failures != 0)
		return 1;
	return 0;
}
