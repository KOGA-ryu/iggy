#include "runtime/RuntimeGameplayProductInputTargetContext.hpp"

#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputBinding2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using Status =
	iggy::runtime::RuntimeGameplayProductInputTargetContextStatus;
using QueryStatus =
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::PlayerInputBindingContext2D BaseContext()
{
	iggy::PlayerInputBindingContext2D context;
	context.input.playerControlEnabled = false;
	context.input.worldInputEnabled = true;
	context.input.interactionEnabled = false;
	context.input.cancelEnabled = true;
	context.hasCurrentPlayerTile = true;
	context.currentPlayerTile = { 3, -2 };
	context.hasSelectedTargetId = true;
	context.selectedTargetId = Id("target:selected");
	context.hasHoveredTargetId = true;
	context.hoveredTargetId = Id("target:hovered-before");
	return context;
}

iggy::runtime::RuntimeGameplayProductInteractionTargetQueryResult Query(
	QueryStatus status,
	bool hasTarget = false,
	iggy::ResourceId targetId = {},
	bool reachable = false)
{
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryResult result;
	result.status = status;
	result.hasTarget = hasTarget;
	result.targetId = targetId;
	result.reachable = reachable;
	if (hasTarget) {
		result.target.id = targetId;
		result.target.enabled = true;
		result.target.radius = 1.0F;
	}
	return result;
}

iggy::runtime::RuntimeGameplayProductInputTargetContextResult Project(
	iggy::PlayerInputBindingContext2D base,
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryResult target)
{
	return iggy::runtime::RuntimeGameplayProductInputTargetContext {}.project({
		base,
		target,
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

void ExpectUnchanged(
	QueryStatus queryStatus,
	const char *message)
{
	const iggy::PlayerInputBindingContext2D base = BaseContext();
	const auto result = Project(base, Query(queryStatus));

	Expect(result.status == Status::Unchanged, message);
	Expect(result.hoveredTargetId.empty(), message);
	ExpectContextEquals(result.bindingContext, base, message);
}

void TestBaseContextRemainsUnchangedForNonFoundStatuses()
{
	ExpectUnchanged(QueryStatus::NotLoaded,
		"NotLoaded query should leave base binding context unchanged");
	ExpectUnchanged(QueryStatus::MissingQuery,
		"MissingQuery query should leave base binding context unchanged");
	ExpectUnchanged(QueryStatus::TargetNotFound,
		"TargetNotFound query should leave base binding context unchanged");
}

void TestValidTargetFoundProjectsHoveredTarget()
{
	const iggy::PlayerInputBindingContext2D base = BaseContext();
	const iggy::ResourceId targetId = Id("target:hovered-after");

	const auto result =
		Project(base, Query(QueryStatus::TargetFound, true, targetId));

	Expect(result.status == Status::TargetProjected,
		"valid found target should project hovered context");
	Expect(result.hoveredTargetId == targetId,
		"valid found target should copy diagnostic hovered id");
	Expect(result.bindingContext.hasHoveredTargetId,
		"valid found target should enable hovered target flag");
	Expect(result.bindingContext.hoveredTargetId == targetId,
		"valid found target should replace hovered target id");
	Expect(result.bindingContext.hasSelectedTargetId ==
			base.hasSelectedTargetId &&
			result.bindingContext.selectedTargetId == base.selectedTargetId,
		"valid found target should preserve selected target");
	Expect(result.bindingContext.hasCurrentPlayerTile ==
			base.hasCurrentPlayerTile &&
			result.bindingContext.currentPlayerTile == base.currentPlayerTile,
		"valid found target should preserve current player tile");
	Expect(result.bindingContext.input.playerControlEnabled ==
			base.input.playerControlEnabled &&
			result.bindingContext.input.worldInputEnabled ==
				base.input.worldInputEnabled &&
			result.bindingContext.input.interactionEnabled ==
				base.input.interactionEnabled &&
			result.bindingContext.input.cancelEnabled ==
				base.input.cancelEnabled,
		"valid found target should preserve input gates");
}

void TestInvalidTargetFoundLeavesBaseUnchanged()
{
	const iggy::PlayerInputBindingContext2D base = BaseContext();

	const auto noTarget =
		Project(base, Query(QueryStatus::TargetFound, false, Id("target:no")));
	const auto emptyTarget =
		Project(base, Query(QueryStatus::TargetFound, true, {}));

	Expect(noTarget.status == Status::Unchanged,
		"TargetFound without hasTarget should leave context unchanged");
	Expect(emptyTarget.status == Status::Unchanged,
		"TargetFound with empty target id should leave context unchanged");
	ExpectContextEquals(noTarget.bindingContext, base,
		"TargetFound without hasTarget should preserve base context exactly");
	ExpectContextEquals(emptyTarget.bindingContext, base,
		"TargetFound with empty target id should preserve base context exactly");
}

void TestReachabilityDoesNotGateProjection()
{
	const iggy::PlayerInputBindingContext2D base = BaseContext();

	const auto reachable =
		Project(base, Query(QueryStatus::TargetFound, true, Id("target:near"), true));
	const auto outOfRange =
		Project(base, Query(QueryStatus::TargetFound, true, Id("target:far"), false));

	Expect(reachable.status == Status::TargetProjected,
		"reachable found target should project hovered target");
	Expect(outOfRange.status == Status::TargetProjected,
		"out-of-range found target should still project hovered target");
	Expect(reachable.bindingContext.hoveredTargetId == Id("target:near"),
		"reachable found target should preserve target id");
	Expect(outOfRange.bindingContext.hoveredTargetId == Id("target:far"),
		"out-of-range found target should preserve target id");
}

void TestExistingHoveredTargetIsPreservedOrReplacedByPolicy()
{
	iggy::PlayerInputBindingContext2D base = BaseContext();
	base.hoveredTargetId = Id("target:existing-hover");

	const auto unchanged =
		Project(base, Query(QueryStatus::TargetNotFound));
	const auto projected =
		Project(base,
			Query(QueryStatus::TargetFound, true, Id("target:replacement")));

	Expect(unchanged.status == Status::Unchanged,
		"unchanged path should preserve existing hover status");
	Expect(unchanged.bindingContext.hasHoveredTargetId,
		"unchanged path should preserve existing hover flag");
	Expect(unchanged.bindingContext.hoveredTargetId ==
			Id("target:existing-hover"),
		"unchanged path should preserve existing hover id");
	Expect(projected.status == Status::TargetProjected,
		"projected path should report TargetProjected");
	Expect(projected.bindingContext.hoveredTargetId == Id("target:replacement"),
		"projected path should replace existing hover id");
}

void TestSelectedTargetIsNeverSetOrCleared()
{
	iggy::PlayerInputBindingContext2D withoutSelected = BaseContext();
	withoutSelected.hasSelectedTargetId = false;
	withoutSelected.selectedTargetId = {};
	const auto projectedWithoutSelected =
		Project(withoutSelected,
			Query(QueryStatus::TargetFound, true, Id("target:hovered")));
	const auto unchangedWithSelected =
		Project(BaseContext(), Query(QueryStatus::TargetNotFound));

	Expect(!projectedWithoutSelected.bindingContext.hasSelectedTargetId,
		"target projection should not set selected target flag");
	Expect(projectedWithoutSelected.bindingContext.selectedTargetId.empty(),
		"target projection should not set selected target id");
	Expect(unchangedWithSelected.bindingContext.hasSelectedTargetId,
		"unchanged projection should preserve selected target flag");
	Expect(unchangedWithSelected.bindingContext.selectedTargetId ==
			Id("target:selected"),
		"unchanged projection should preserve selected target id");
}

void TestBindingStillPrefersSelectedTargetOverProjectedHoveredTarget()
{
	iggy::PlayerInputBindingContext2D base = BaseContext();
	const auto projected =
		Project(base,
			Query(QueryStatus::TargetFound, true, Id("target:projected-hover")));

	iggy::PlayerInputBindingAction2D action;
	action.type = iggy::PlayerInputBindingAction2DType::Interact;
	const iggy::PlayerInputBinding2DResult binding =
		iggy::PlayerInputBinding2D {}.bind(
			projected.bindingContext,
			{ action });

	Expect(binding.intents.size() == 1,
		"selected plus projected-hover context should emit one interact intent");
	if (binding.intents.size() == 1) {
		Expect(binding.intents[0].type ==
				iggy::PlayerInputIntent2DType::Interact,
			"selected plus projected-hover binding should emit interact intent");
		Expect(binding.intents[0].targetId == Id("target:selected"),
			"PlayerInputBinding2D should still prefer selected target over projected hovered target");
	}
}

void TestQueryReportInputIsNotMutated()
{
	const iggy::PlayerInputBindingContext2D base = BaseContext();
	const auto targetBefore =
		Query(QueryStatus::TargetFound, true, Id("target:immutable"), false);
	iggy::runtime::RuntimeGameplayProductInputTargetContextInput input;
	input.base = base;
	input.target = targetBefore;

	const auto result =
		iggy::runtime::RuntimeGameplayProductInputTargetContext {}.project(
			input);

	Expect(result.status == Status::TargetProjected,
		"immutability setup should project hovered target");
	ExpectContextEquals(input.base, base,
		"target context projection should not mutate input base context");
	Expect(input.target.status == targetBefore.status &&
			input.target.hasTarget == targetBefore.hasTarget &&
			input.target.targetId == targetBefore.targetId &&
			input.target.reachable == targetBefore.reachable,
		"target context projection should not mutate input query report");
}

} // namespace

int main()
{
	TestBaseContextRemainsUnchangedForNonFoundStatuses();
	TestValidTargetFoundProjectsHoveredTarget();
	TestInvalidTargetFoundLeavesBaseUnchanged();
	TestReachabilityDoesNotGateProjection();
	TestExistingHoveredTargetIsPreservedOrReplacedByPolicy();
	TestSelectedTargetIsNeverSetOrCleared();
	TestBindingStillPrefersSelectedTargetOverProjectedHoveredTarget();
	TestQueryReportInputIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
