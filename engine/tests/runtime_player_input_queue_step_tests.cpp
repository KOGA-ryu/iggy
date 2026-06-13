#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputQueueStep.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "player:input-queue" };
const iggy::ResourceId TargetId { "target:input-queue" };
const iggy::ResourceId ExistingActorId { "player:existing" };

iggy::runtime::GameplayCommandFrame2D MoveFrame(float x, float y)
{
	return { { iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(ExistingActorId, { x, y }) } };
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

bool SameQueue(const iggy::runtime::RuntimeCommandQueueState &actual, const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
}

void ExpectIntentEquals(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void TestEmptyIntentsQueueEmptyCommandFrame()
{
	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push({}, {}, ActorId, {});

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "empty intents should queue successfully");
	Expect(result.mapping.frame.commands.empty(), "empty intents should map to empty frame");
	Expect(!result.mapping.hasIssues(), "empty intents should have no mapping issues");
	Expect(result.push.status == iggy::runtime::RuntimeCommandQueueStatus::Accepted, "empty mapped frame should still be pushed");
	Expect(result.queue.frames.size() == 1, "empty mapped frame should be queued as one frame");
	Expect(result.queue.frames[0].commands.empty(), "queued empty mapped frame should have no commands");
}

void TestValidIntentsQueueOneFrameWithAcceptedCommandsInOrder()
{
	const iggy::Vec2 point { -4.0F, 7.5F };
	const iggy::TileCoord tile { -2, 9 };
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent(point),
		iggy::playerMoveToTileIntent(tile),
		iggy::playerInteractIntent(TargetId),
		iggy::playerWaitIntent(),
	};

	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push({}, {}, ActorId, intents);

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "valid intents should queue");
	Expect(result.mapping.frame.commands.size() == 4, "valid intents should map all commands");
	Expect(result.queue.frames.size() == 1, "valid intents should queue one command frame");
	Expect(result.queue.frames[0].commands.size() == 4, "queued command frame should preserve mapped command count");

	const iggy::runtime::GameplayCommandFrame2D &frame = result.queue.frames[0];
	Expect(frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "first queued command should be MoveToPoint");
	Expect(NearVec(frame.commands[0].targetPoint, point), "queued MoveToPoint should preserve point");
	Expect(frame.commands[1].type == iggy::runtime::GameplayCommand2DType::MoveToTile, "second queued command should be MoveToTile");
	Expect(frame.commands[1].targetTile == tile, "queued MoveToTile should preserve negative tile");
	Expect(frame.commands[2].type == iggy::runtime::GameplayCommand2DType::Interact, "third queued command should be Interact");
	Expect(frame.commands[2].targetId == TargetId, "queued Interact should preserve target id");
	Expect(frame.commands[3].type == iggy::runtime::GameplayCommand2DType::Wait, "fourth queued command should be Wait");
	for (const iggy::runtime::GameplayCommand2D &command : frame.commands) {
		Expect(command.actorId == ActorId, "queued command should preserve actor id");
		Expect(iggy::runtime::valid(command), "accepted queued command should validate");
	}
}

void TestMixedIntentsQueueAcceptedCommandsAndPreserveIssues()
{
	const iggy::PlayerInputIntent2D validMove = iggy::playerMoveToPointIntent({ 1.0F, 2.0F });
	const iggy::PlayerInputIntent2D invalidInteract = iggy::playerInteractIntent({});
	const iggy::PlayerInputIntent2D unsupportedInspect = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D unsupportedCancel = iggy::playerCancelIntent();
	const iggy::PlayerInputIntent2D validWait = iggy::playerWaitIntent();
	const std::vector<iggy::PlayerInputIntent2D> intents {
		validMove,
		invalidInteract,
		unsupportedInspect,
		unsupportedCancel,
		validWait,
	};

	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push({}, {}, ActorId, intents);

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "mixed intents should still queue mapped frame");
	Expect(result.mapping.hasIssues(), "mixed intents should preserve mapping issues");
	Expect(result.mapping.issues.size() == 3, "invalid and unsupported intents should be reported");
	Expect(result.queue.frames.size() == 1, "mixed intents should queue one frame");
	Expect(result.queue.frames[0].commands.size() == 2, "queued frame should include only accepted commands");
	Expect(result.queue.frames[0].commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "first accepted command should preserve order");
	Expect(result.queue.frames[0].commands[1].type == iggy::runtime::GameplayCommand2DType::Wait, "second accepted command should preserve order");

	Expect(result.mapping.issues[0].intentIndex == 1, "invalid interact issue should preserve original index");
	Expect(result.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::InvalidIntent, "invalid interact issue should preserve mapper status");
	Expect(result.mapping.issues[0].map.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "invalid interact issue should preserve intent status");
	ExpectIntentEquals(result.mapping.issues[0].intent, invalidInteract, "invalid interact issue should preserve original intent");

	Expect(result.mapping.issues[1].intentIndex == 2, "unsupported inspect issue should preserve original index");
	Expect(result.mapping.issues[1].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported inspect issue should preserve mapper status");
	ExpectIntentEquals(result.mapping.issues[1].intent, unsupportedInspect, "unsupported inspect issue should preserve original intent");

	Expect(result.mapping.issues[2].intentIndex == 3, "unsupported cancel issue should preserve original index");
	Expect(result.mapping.issues[2].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported cancel issue should preserve mapper status");
	ExpectIntentEquals(result.mapping.issues[2].intent, unsupportedCancel, "unsupported cancel issue should preserve original intent");
}

void TestEmptyActorIdIsPreservedInQueuedCommands()
{
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 3.0F, 4.0F }),
		iggy::playerWaitIntent(),
	};

	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push({}, {}, {}, intents);

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "empty actor id input should queue");
	Expect(result.queue.frames.size() == 1, "empty actor id input should queue one frame");
	Expect(result.queue.frames[0].commands.size() == 2, "empty actor id input should queue mapped commands");
	Expect(result.queue.frames[0].commands[0].actorId.empty(), "first queued command should preserve empty actor id");
	Expect(result.queue.frames[0].commands[1].actorId.empty(), "second queued command should preserve empty actor id");
}

void TestBoundedFullQueueRejectsAndLeavesQueueUnchanged()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame(9.0F, 9.0F));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 1.0F, 1.0F }),
	};

	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push(queue, { 1 }, ActorId, intents);

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "full queue should reject mapped frame");
	Expect(result.push.status == iggy::runtime::RuntimeCommandQueueStatus::RejectedFull, "push result should preserve queue rejection");
	Expect(result.mapping.frame.commands.size() == 1, "mapping should still run even when queue is full");
	Expect(SameQueue(result.queue, queueBefore), "top-level queue should remain unchanged after rejection");
	Expect(SameQueue(result.push.queue, queueBefore), "push diagnostics should preserve unchanged queue");
}

void TestExistingFramesKeepFifoOrderAfterSuccessfulPush()
{
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame(1.0F, 1.0F);
	const iggy::runtime::GameplayCommandFrame2D second = MoveFrame(2.0F, 2.0F);
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(first);
	queue.frames.push_back(second);

	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push(
		queue,
		{},
		ActorId,
		{ iggy::playerWaitIntent() });

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "non-full queue should accept mapped frame");
	Expect(result.queue.frames.size() == 3, "successful push should append after existing frames");
	Expect(SameFrame(result.queue.frames[0], first), "first existing frame should remain first");
	Expect(SameFrame(result.queue.frames[1], second), "second existing frame should remain second");
	Expect(result.queue.frames[2].commands.size() == 1, "new mapped frame should be appended");
	Expect(result.queue.frames[2].commands[0].type == iggy::runtime::GameplayCommand2DType::Wait, "new appended frame should contain mapped wait command");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame(6.0F, 6.0F));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	iggy::runtime::RuntimeCommandQueueConfig config { 3 };
	const iggy::runtime::RuntimeCommandQueueConfig configBefore = config;
	iggy::ResourceId actorId = ActorId;
	const iggy::ResourceId actorIdBefore = actorId;
	std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 5.0F, 6.0F }),
		iggy::playerCancelIntent(),
	};
	const std::vector<iggy::PlayerInputIntent2D> intentsBefore = intents;

	const iggy::runtime::RuntimePlayerInputQueueResult result = iggy::runtime::RuntimePlayerInputQueueStep {}.push(queue, config, actorId, intents);

	Expect(result.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "immutability setup should queue");
	Expect(SameQueue(queue, queueBefore), "step should not mutate input queue");
	Expect(config.maxFrames == configBefore.maxFrames, "step should not mutate queue config");
	Expect(actorId == actorIdBefore, "step should not mutate actor id");
	Expect(intents.size() == intentsBefore.size(), "step should not mutate intent vector size");
	for (std::size_t index = 0; index < intents.size(); ++index) {
		ExpectIntentEquals(intents[index], intentsBefore[index], "step should not mutate intents");
	}
}

} // namespace

int main()
{
	TestEmptyIntentsQueueEmptyCommandFrame();
	TestValidIntentsQueueOneFrameWithAcceptedCommandsInOrder();
	TestMixedIntentsQueueAcceptedCommandsAndPreserveIssues();
	TestEmptyActorIdIsPreservedInQueuedCommands();
	TestBoundedFullQueueRejectsAndLeavesQueueUnchanged();
	TestExistingFramesKeepFifoOrderAfterSuccessfulPush();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
