#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputIntentGate2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId TargetId { "target:gate" };

void ExpectIntentEquals(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

iggy::PlayerInputIntentGate2DResult Evaluate(iggy::PlayerInputContext2D context, iggy::PlayerInputIntent2D intent)
{
	return iggy::PlayerInputIntentGate2D {}.evaluate(context, intent);
}

void ExpectAccepted(iggy::PlayerInputIntentGate2DResult result, iggy::PlayerInputIntent2D intent, const char *message)
{
	Expect(result.accepted, message);
	Expect(result.reason == iggy::PlayerInputIntentBlockReason::None, message);
	Expect(result.intentStatus == iggy::PlayerInputIntent2DStatus::Valid, message);
	ExpectIntentEquals(result.intent, intent, message);
}

void ExpectBlocked(
	iggy::PlayerInputIntentGate2DResult result,
	iggy::PlayerInputIntent2D intent,
	iggy::PlayerInputIntentBlockReason reason,
	iggy::PlayerInputIntent2DStatus intentStatus,
	const char *message)
{
	Expect(!result.accepted, message);
	Expect(result.reason == reason, message);
	Expect(result.intentStatus == intentStatus, message);
	ExpectIntentEquals(result.intent, intent, message);
}

std::vector<iggy::PlayerInputIntent2D> ValidIntents()
{
	return {
		{},
		iggy::playerWaitIntent(),
		iggy::playerMoveToPointIntent({ -3.5F, 9.25F }),
		iggy::playerMoveToTileIntent({ -4, 7 }),
		iggy::playerInteractIntent(TargetId),
		iggy::playerInspectIntent(TargetId),
		iggy::playerCancelIntent(),
	};
}

void TestDefaultContextAcceptsValidIntentTypes()
{
	const iggy::PlayerInputContext2D context;
	for (const iggy::PlayerInputIntent2D &intent : ValidIntents()) {
		ExpectAccepted(Evaluate(context, intent), intent, "default context should accept valid intent");
	}
}

void TestInvalidTargetIntentsBlockBeforeContext()
{
	iggy::PlayerInputContext2D context;
	context.playerControlEnabled = false;
	context.worldInputEnabled = false;
	context.interactionEnabled = false;

	const iggy::PlayerInputIntent2D invalidInteract = iggy::playerInteractIntent({});
	const iggy::PlayerInputIntent2D invalidInspect = iggy::playerInspectIntent({});

	ExpectBlocked(
		Evaluate(context, invalidInteract),
		invalidInteract,
		iggy::PlayerInputIntentBlockReason::InvalidIntent,
		iggy::PlayerInputIntent2DStatus::MissingTarget,
		"invalid interact should block before context");
	ExpectBlocked(
		Evaluate(context, invalidInspect),
		invalidInspect,
		iggy::PlayerInputIntentBlockReason::InvalidIntent,
		iggy::PlayerInputIntent2DStatus::MissingTarget,
		"invalid inspect should block before context");
}

void TestPlayerControlDisabledBlocksAllValidIntentTypes()
{
	iggy::PlayerInputContext2D context;
	context.playerControlEnabled = false;

	for (const iggy::PlayerInputIntent2D &intent : ValidIntents()) {
		ExpectBlocked(
			Evaluate(context, intent),
			intent,
			iggy::PlayerInputIntentBlockReason::PlayerControlDisabled,
			iggy::PlayerInputIntent2DStatus::Valid,
			"player control disabled should block valid intent");
	}
}

void TestWorldInputDisabledBlocksMovementAndTargetIntentsOnly()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;

	ExpectAccepted(Evaluate(context, {}), {}, "world input disabled should allow none");
	ExpectAccepted(Evaluate(context, iggy::playerWaitIntent()), iggy::playerWaitIntent(), "world input disabled should allow wait");
	ExpectAccepted(Evaluate(context, iggy::playerCancelIntent()), iggy::playerCancelIntent(), "world input disabled should allow cancel");

	const iggy::PlayerInputIntent2D movePoint = iggy::playerMoveToPointIntent({ 1.0F, 2.0F });
	const iggy::PlayerInputIntent2D moveTile = iggy::playerMoveToTileIntent({ -2, 5 });
	const iggy::PlayerInputIntent2D interact = iggy::playerInteractIntent(TargetId);
	const iggy::PlayerInputIntent2D inspect = iggy::playerInspectIntent(TargetId);

	ExpectBlocked(Evaluate(context, movePoint), movePoint, iggy::PlayerInputIntentBlockReason::WorldInputDisabled, iggy::PlayerInputIntent2DStatus::Valid, "world input disabled should block MoveToPoint");
	ExpectBlocked(Evaluate(context, moveTile), moveTile, iggy::PlayerInputIntentBlockReason::WorldInputDisabled, iggy::PlayerInputIntent2DStatus::Valid, "world input disabled should block MoveToTile");
	ExpectBlocked(Evaluate(context, interact), interact, iggy::PlayerInputIntentBlockReason::WorldInputDisabled, iggy::PlayerInputIntent2DStatus::Valid, "world input disabled should block Interact before interaction context");
	ExpectBlocked(Evaluate(context, inspect), inspect, iggy::PlayerInputIntentBlockReason::WorldInputDisabled, iggy::PlayerInputIntent2DStatus::Valid, "world input disabled should block Inspect before interaction context");
}

void TestInteractionDisabledBlocksTargetIntentsAndAllowsMovement()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;

	const iggy::PlayerInputIntent2D movePoint = iggy::playerMoveToPointIntent({ 1.0F, 2.0F });
	const iggy::PlayerInputIntent2D moveTile = iggy::playerMoveToTileIntent({ -2, 5 });
	const iggy::PlayerInputIntent2D interact = iggy::playerInteractIntent(TargetId);
	const iggy::PlayerInputIntent2D inspect = iggy::playerInspectIntent(TargetId);

	ExpectAccepted(Evaluate(context, movePoint), movePoint, "interaction disabled should allow MoveToPoint");
	ExpectAccepted(Evaluate(context, moveTile), moveTile, "interaction disabled should allow MoveToTile");
	ExpectBlocked(Evaluate(context, interact), interact, iggy::PlayerInputIntentBlockReason::InteractionDisabled, iggy::PlayerInputIntent2DStatus::Valid, "interaction disabled should block Interact");
	ExpectBlocked(Evaluate(context, inspect), inspect, iggy::PlayerInputIntentBlockReason::InteractionDisabled, iggy::PlayerInputIntent2DStatus::Valid, "interaction disabled should block Inspect");
}

void TestCancelDisabledBlocksCancelOnly()
{
	iggy::PlayerInputContext2D context;
	context.cancelEnabled = false;

	const iggy::PlayerInputIntent2D cancel = iggy::playerCancelIntent();
	ExpectBlocked(Evaluate(context, cancel), cancel, iggy::PlayerInputIntentBlockReason::CancelDisabled, iggy::PlayerInputIntent2DStatus::Valid, "cancel disabled should block Cancel");

	ExpectAccepted(Evaluate(context, {}), {}, "cancel disabled should allow None");
	ExpectAccepted(Evaluate(context, iggy::playerWaitIntent()), iggy::playerWaitIntent(), "cancel disabled should allow Wait");
	ExpectAccepted(Evaluate(context, iggy::playerMoveToPointIntent({ 1.0F, 2.0F })), iggy::playerMoveToPointIntent({ 1.0F, 2.0F }), "cancel disabled should allow MoveToPoint");
	ExpectAccepted(Evaluate(context, iggy::playerInteractIntent(TargetId)), iggy::playerInteractIntent(TargetId), "cancel disabled should allow Interact");
}

void TestBlockReasonPrecedenceAfterValidation()
{
	iggy::PlayerInputContext2D allDisabled;
	allDisabled.playerControlEnabled = false;
	allDisabled.worldInputEnabled = false;
	allDisabled.interactionEnabled = false;
	allDisabled.cancelEnabled = false;

	const iggy::PlayerInputIntent2D movePoint = iggy::playerMoveToPointIntent({ 1.0F, 2.0F });
	const iggy::PlayerInputIntent2D cancel = iggy::playerCancelIntent();

	ExpectBlocked(Evaluate(allDisabled, movePoint), movePoint, iggy::PlayerInputIntentBlockReason::PlayerControlDisabled, iggy::PlayerInputIntent2DStatus::Valid, "player control disabled should win over world input disabled");
	ExpectBlocked(Evaluate(allDisabled, cancel), cancel, iggy::PlayerInputIntentBlockReason::PlayerControlDisabled, iggy::PlayerInputIntent2DStatus::Valid, "player control disabled should win over cancel disabled");

	iggy::PlayerInputContext2D worldAndInteractionDisabled;
	worldAndInteractionDisabled.worldInputEnabled = false;
	worldAndInteractionDisabled.interactionEnabled = false;
	const iggy::PlayerInputIntent2D interact = iggy::playerInteractIntent(TargetId);
	ExpectBlocked(Evaluate(worldAndInteractionDisabled, interact), interact, iggy::PlayerInputIntentBlockReason::WorldInputDisabled, iggy::PlayerInputIntent2DStatus::Valid, "world input disabled should win over interaction disabled");
}

void TestPreservesIntentPayloadsExactly()
{
	iggy::PlayerInputContext2D context;
	context.playerControlEnabled = false;

	const iggy::PlayerInputIntent2D movePoint = iggy::playerMoveToPointIntent({ -3.5F, 9.25F });
	const iggy::PlayerInputIntent2D moveTile = iggy::playerMoveToTileIntent({ -4, 7 });
	const iggy::PlayerInputIntent2D interact = iggy::playerInteractIntent(TargetId);

	ExpectBlocked(Evaluate(context, movePoint), movePoint, iggy::PlayerInputIntentBlockReason::PlayerControlDisabled, iggy::PlayerInputIntent2DStatus::Valid, "blocked MoveToPoint should preserve point payload");
	ExpectBlocked(Evaluate(context, moveTile), moveTile, iggy::PlayerInputIntentBlockReason::PlayerControlDisabled, iggy::PlayerInputIntent2DStatus::Valid, "blocked MoveToTile should preserve negative tile payload");
	ExpectBlocked(Evaluate(context, interact), interact, iggy::PlayerInputIntentBlockReason::PlayerControlDisabled, iggy::PlayerInputIntent2DStatus::Valid, "blocked Interact should preserve target id payload");
}

void TestInputsAreNotMutated()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::PlayerInputContext2D contextBefore = context;
	iggy::PlayerInputIntent2D intent = iggy::playerMoveToTileIntent({ -3, 8 });
	const iggy::PlayerInputIntent2D intentBefore = intent;

	const iggy::PlayerInputIntentGate2DResult result = Evaluate(context, intent);

	Expect(!result.accepted, "immutability setup should block movement");
	Expect(context.playerControlEnabled == contextBefore.playerControlEnabled, "gate should not mutate player control flag");
	Expect(context.worldInputEnabled == contextBefore.worldInputEnabled, "gate should not mutate world input flag");
	Expect(context.interactionEnabled == contextBefore.interactionEnabled, "gate should not mutate interaction flag");
	Expect(context.cancelEnabled == contextBefore.cancelEnabled, "gate should not mutate cancel flag");
	ExpectIntentEquals(intent, intentBefore, "gate should not mutate input intent");
}

} // namespace

int main()
{
	TestDefaultContextAcceptsValidIntentTypes();
	TestInvalidTargetIntentsBlockBeforeContext();
	TestPlayerControlDisabledBlocksAllValidIntentTypes();
	TestWorldInputDisabledBlocksMovementAndTargetIntentsOnly();
	TestInteractionDisabledBlocksTargetIntentsAndAllowsMovement();
	TestCancelDisabledBlocksCancelOnly();
	TestBlockReasonPrecedenceAfterValidation();
	TestPreservesIntentPayloadsExactly();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
