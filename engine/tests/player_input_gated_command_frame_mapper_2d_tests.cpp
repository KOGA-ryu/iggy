#include <cstdlib>
#include <vector>

#include "scene/player/PlayerInputGatedCommandFrameMapper2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "player:gated-frame-mapper" };
const iggy::ResourceId TargetId { "target:gated-frame-mapper" };

void ExpectIntentEquals(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

iggy::PlayerInputGatedCommandFrameMapper2DResult Map(
	iggy::ResourceId actorId,
	const iggy::PlayerInputContext2D &context,
	const std::vector<iggy::PlayerInputIntent2D> &intents)
{
	return iggy::PlayerInputGatedCommandFrameMapper2D {}.map(actorId, context, intents);
}

void TestEmptyInput()
{
	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map(ActorId, {}, {});

	Expect(result.frame.commands.empty(), "empty gated mapper should produce empty frame");
	Expect(result.gateIssues.empty(), "empty gated mapper should produce no gate issues");
	Expect(result.mapping.frame.commands.empty(), "empty gated mapper should preserve empty mapping result");
	Expect(result.mapping.issues.empty(), "empty gated mapper should produce no mapping issues");
	Expect(!result.hasIssues(), "empty gated mapper should report no issues");
}

void TestDefaultContextMapsValidSupportedIntents()
{
	const iggy::Vec2 point { -2.5F, 8.75F };
	const iggy::TileCoord tile { -3, 11 };
	const std::vector<iggy::PlayerInputIntent2D> intents {
		{},
		iggy::playerMoveToPointIntent(point),
		iggy::playerMoveToTileIntent(tile),
		iggy::playerInteractIntent(TargetId),
		iggy::playerWaitIntent(),
	};

	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map(ActorId, {}, intents);

	Expect(result.gateIssues.empty(), "default supported intents should have no gate issues");
	Expect(result.mapping.issues.empty(), "default supported intents should have no mapping issues");
	Expect(!result.hasIssues(), "default supported intents should report no issues");
	Expect(result.frame.commands.size() == 5, "default supported intents should map all commands");
	Expect(SameFrame(result.frame, result.mapping.frame), "top-level frame should match mapping frame exactly");

	Expect(result.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::None, "first command should be None");
	Expect(result.frame.commands[0].actorId == ActorId, "None command should preserve actor id");
	Expect(result.frame.commands[1].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "second command should be MoveToPoint");
	Expect(NearVec(result.frame.commands[1].targetPoint, point), "MoveToPoint command should preserve point");
	Expect(result.frame.commands[2].type == iggy::runtime::GameplayCommand2DType::MoveToTile, "third command should be MoveToTile");
	Expect(result.frame.commands[2].targetTile == tile, "MoveToTile command should preserve negative tile");
	Expect(result.frame.commands[3].type == iggy::runtime::GameplayCommand2DType::Interact, "fourth command should be Interact");
	Expect(result.frame.commands[3].targetId == TargetId, "Interact command should preserve target id");
	Expect(result.frame.commands[4].type == iggy::runtime::GameplayCommand2DType::Wait, "fifth command should be Wait");
}

void TestInvalidInteractAndInspectAreGateIssuesOnly()
{
	const iggy::PlayerInputIntent2D invalidInteract = iggy::playerInteractIntent({});
	const iggy::PlayerInputIntent2D invalidInspect = iggy::playerInspectIntent({});
	const iggy::PlayerInputIntent2D validWait = iggy::playerWaitIntent();

	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map(ActorId, {}, {
		invalidInteract,
		invalidInspect,
		validWait,
	});

	Expect(result.frame.commands.size() == 1, "invalid target intents should be excluded before mapping");
	Expect(result.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::Wait, "valid wait should still map");
	Expect(result.gateIssues.size() == 2, "invalid target intents should be gate issues");
	Expect(result.mapping.issues.empty(), "invalid target intents should not appear as mapping issues");
	Expect(result.hasIssues(), "invalid target intents should report issues");

	Expect(result.gateIssues[0].intentIndex == 0, "invalid interact should preserve original index");
	Expect(result.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::InvalidIntent, "invalid interact should preserve InvalidIntent reason");
	Expect(result.gateIssues[0].gate.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "invalid interact should preserve MissingTarget");
	ExpectIntentEquals(result.gateIssues[0].gate.intent, invalidInteract, "invalid interact should preserve intent");

	Expect(result.gateIssues[1].intentIndex == 1, "invalid inspect should preserve original index");
	Expect(result.gateIssues[1].gate.reason == iggy::PlayerInputIntentBlockReason::InvalidIntent, "invalid inspect should preserve InvalidIntent reason");
	Expect(result.gateIssues[1].gate.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "invalid inspect should preserve MissingTarget");
	ExpectIntentEquals(result.gateIssues[1].gate.intent, invalidInspect, "invalid inspect should preserve intent");
}

void TestDisabledPlayerWorldInteractionAndCancelContextsBlockGateIssues()
{
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 1.0F, 2.0F }),
		iggy::playerInteractIntent(TargetId),
		iggy::playerCancelIntent(),
		iggy::playerWaitIntent(),
	};

	iggy::PlayerInputContext2D playerDisabled;
	playerDisabled.playerControlEnabled = false;
	const iggy::PlayerInputGatedCommandFrameMapper2DResult playerBlocked = Map(ActorId, playerDisabled, intents);
	Expect(playerBlocked.frame.commands.empty(), "player disabled should block every valid intent before mapping");
	Expect(playerBlocked.gateIssues.size() == 4, "player disabled should report every valid intent as gate issue");
	Expect(playerBlocked.mapping.issues.empty(), "player disabled should produce no mapping issues");
	for (const iggy::PlayerInputGatedIntentIssue &issue : playerBlocked.gateIssues) {
		Expect(issue.gate.reason == iggy::PlayerInputIntentBlockReason::PlayerControlDisabled, "player disabled should use PlayerControlDisabled reason");
	}

	iggy::PlayerInputContext2D worldDisabled;
	worldDisabled.worldInputEnabled = false;
	const iggy::PlayerInputGatedCommandFrameMapper2DResult worldBlocked = Map(ActorId, worldDisabled, intents);
	Expect(worldBlocked.frame.commands.size() == 1, "world disabled should map accepted wait command");
	Expect(worldBlocked.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::Wait, "world disabled should preserve accepted wait command");
	Expect(worldBlocked.gateIssues.size() == 2, "world disabled should block movement and target intents");
	Expect(worldBlocked.mapping.issues.size() == 1, "world disabled should let accepted cancel reach mapping issue");
	Expect(worldBlocked.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "world disabled accepted cancel should remain unsupported by mapper");
	Expect(worldBlocked.gateIssues[0].intentIndex == 0, "world disabled should block move at original index");
	Expect(worldBlocked.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::WorldInputDisabled, "world disabled should use WorldInputDisabled for move");
	Expect(worldBlocked.gateIssues[1].intentIndex == 1, "world disabled should block interact at original index");
	Expect(worldBlocked.gateIssues[1].gate.reason == iggy::PlayerInputIntentBlockReason::WorldInputDisabled, "world disabled should use WorldInputDisabled for interact");

	iggy::PlayerInputContext2D interactionDisabled;
	interactionDisabled.interactionEnabled = false;
	const iggy::PlayerInputGatedCommandFrameMapper2DResult interactionBlocked = Map(ActorId, interactionDisabled, intents);
	Expect(interactionBlocked.frame.commands.size() == 2, "interaction disabled should map accepted movement and wait commands");
	Expect(interactionBlocked.gateIssues.size() == 1, "interaction disabled should block interact only in fixture");
	Expect(interactionBlocked.mapping.issues.size() == 1, "interaction disabled should let accepted cancel reach mapping issue");
	Expect(interactionBlocked.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "interaction disabled accepted cancel should remain unsupported by mapper");
	Expect(interactionBlocked.gateIssues[0].intentIndex == 1, "interaction disabled should preserve interact index");
	Expect(interactionBlocked.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::InteractionDisabled, "interaction disabled should use InteractionDisabled");

	iggy::PlayerInputContext2D cancelDisabled;
	cancelDisabled.cancelEnabled = false;
	const iggy::PlayerInputGatedCommandFrameMapper2DResult cancelBlocked = Map(ActorId, cancelDisabled, intents);
	Expect(cancelBlocked.frame.commands.size() == 3, "cancel disabled should allow supported non-cancel intents");
	Expect(cancelBlocked.gateIssues.size() == 1, "cancel disabled should block cancel only");
	Expect(cancelBlocked.gateIssues[0].intentIndex == 2, "cancel disabled should preserve cancel index");
	Expect(cancelBlocked.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::CancelDisabled, "cancel disabled should use CancelDisabled");
}

void TestDefaultContextLetsInspectAndCancelReachMappingIssues()
{
	const iggy::PlayerInputIntent2D inspect = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D cancel = iggy::playerCancelIntent();

	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map(ActorId, {}, { inspect, cancel });

	Expect(result.frame.commands.empty(), "unsupported but gate-accepted intents should not produce commands");
	Expect(result.gateIssues.empty(), "valid inspect and cancel should not be gate issues by default");
	Expect(result.mapping.issues.size() == 2, "valid inspect and cancel should become mapping issues");
	Expect(result.hasIssues(), "mapping issues should make gated mapper report issues");
	Expect(result.mapping.issues[0].intentIndex == 0, "inspect mapping issue should preserve accepted-list index");
	Expect(result.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "inspect should be unsupported by command mapper");
	ExpectIntentEquals(result.mapping.issues[0].intent, inspect, "inspect mapping issue should preserve intent");
	Expect(result.mapping.issues[1].intentIndex == 1, "cancel mapping issue should preserve accepted-list index");
	Expect(result.mapping.issues[1].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "cancel should be unsupported by command mapper");
	ExpectIntentEquals(result.mapping.issues[1].intent, cancel, "cancel mapping issue should preserve intent");
}

void TestMixedBlockedUnsupportedAndAcceptedIntents()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::PlayerInputIntent2D blockedMove = iggy::playerMoveToPointIntent({ 1.0F, 2.0F });
	const iggy::PlayerInputIntent2D unsupportedCancel = iggy::playerCancelIntent();
	const iggy::PlayerInputIntent2D acceptedWait = iggy::playerWaitIntent();
	const iggy::PlayerInputIntent2D invalidInteract = iggy::playerInteractIntent({});
	const std::vector<iggy::PlayerInputIntent2D> intents {
		blockedMove,
		unsupportedCancel,
		acceptedWait,
		invalidInteract,
	};

	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map(ActorId, context, intents);

	Expect(result.frame.commands.size() == 1, "mixed gated mapper should produce one accepted command");
	Expect(result.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::Wait, "mixed gated mapper should preserve accepted wait command");
	Expect(result.gateIssues.size() == 2, "mixed gated mapper should report blocked and invalid gate issues");
	Expect(result.mapping.issues.size() == 1, "mixed gated mapper should report unsupported accepted intent as mapping issue");
	Expect(result.hasIssues(), "mixed gated mapper should report issues");

	Expect(result.gateIssues[0].intentIndex == 0, "blocked move should preserve original index");
	Expect(result.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::WorldInputDisabled, "blocked move should preserve world disabled reason");
	ExpectIntentEquals(result.gateIssues[0].gate.intent, blockedMove, "blocked move should preserve intent");
	Expect(result.gateIssues[1].intentIndex == 3, "invalid interact should preserve original index");
	Expect(result.gateIssues[1].gate.reason == iggy::PlayerInputIntentBlockReason::InvalidIntent, "invalid interact should preserve invalid reason");
	ExpectIntentEquals(result.gateIssues[1].gate.intent, invalidInteract, "invalid interact should preserve intent");

	Expect(result.mapping.issues[0].intentIndex == 0, "unsupported cancel should be first accepted intent passed to mapper");
	Expect(result.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported cancel should preserve mapper status");
	ExpectIntentEquals(result.mapping.issues[0].intent, unsupportedCancel, "unsupported cancel should preserve mapping intent");
}

void TestEmptyActorIdPreservedInAcceptedCommands()
{
	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map({}, {}, {
		iggy::playerMoveToPointIntent({ 3.0F, 4.0F }),
		iggy::playerWaitIntent(),
	});

	Expect(result.frame.commands.size() == 2, "empty actor id should still map accepted commands");
	Expect(result.frame.commands[0].actorId.empty(), "first accepted command should preserve empty actor id");
	Expect(result.frame.commands[1].actorId.empty(), "second accepted command should preserve empty actor id");
}

void TestInputsAreNotMutated()
{
	iggy::ResourceId actorId = ActorId;
	const iggy::ResourceId actorIdBefore = actorId;
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::PlayerInputContext2D contextBefore = context;
	std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 1.0F, 2.0F }),
		iggy::playerCancelIntent(),
		iggy::playerWaitIntent(),
	};
	const std::vector<iggy::PlayerInputIntent2D> intentsBefore = intents;

	const iggy::PlayerInputGatedCommandFrameMapper2DResult result = Map(actorId, context, intents);

	Expect(result.hasIssues(), "immutability setup should produce issue");
	Expect(actorId == actorIdBefore, "gated mapper should not mutate actor id");
	Expect(context.playerControlEnabled == contextBefore.playerControlEnabled, "gated mapper should not mutate player control flag");
	Expect(context.worldInputEnabled == contextBefore.worldInputEnabled, "gated mapper should not mutate world input flag");
	Expect(context.interactionEnabled == contextBefore.interactionEnabled, "gated mapper should not mutate interaction flag");
	Expect(context.cancelEnabled == contextBefore.cancelEnabled, "gated mapper should not mutate cancel flag");
	Expect(intents.size() == intentsBefore.size(), "gated mapper should not mutate intent vector size");
	for (std::size_t index = 0; index < intents.size(); ++index) {
		ExpectIntentEquals(intents[index], intentsBefore[index], "gated mapper should not mutate intents");
	}
}

} // namespace

int main()
{
	TestEmptyInput();
	TestDefaultContextMapsValidSupportedIntents();
	TestInvalidInteractAndInspectAreGateIssuesOnly();
	TestDisabledPlayerWorldInteractionAndCancelContextsBlockGateIssues();
	TestDefaultContextLetsInspectAndCancelReachMappingIssues();
	TestMixedBlockedUnsupportedAndAcceptedIntents();
	TestEmptyActorIdPreservedInAcceptedCommands();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
