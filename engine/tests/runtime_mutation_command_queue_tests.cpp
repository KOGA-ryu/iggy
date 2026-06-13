#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeMutationCommandQueue.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "actor:mutation-queue" };
const iggy::ResourceId TargetId { "target:mutation-queue" };

iggy::LevelTileEdit Edit(int x, int y, bool walkable)
{
	return { { x, y }, walkable };
}

iggy::runtime::GameplayCommandFrame2D CommandFrame(std::vector<iggy::runtime::GameplayCommand2D> commands)
{
	return { commands };
}

iggy::runtime::GameplayCommandFrame2D MoveCommandFrame(float x, float y)
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(ActorId, { x, y }) });
}

iggy::runtime::GameplayCommandFrame2D WaitCommandFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(ActorId) });
}

iggy::runtime::GameplayCommandFrame2D InteractCommandFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.interact(ActorId, TargetId) });
}

iggy::runtime::RuntimeSessionMutationCommandFrame Frame(
	std::vector<iggy::LevelTileEdit> edits,
	iggy::runtime::GameplayCommandFrame2D commandFrame)
{
	return { edits, commandFrame };
}

bool SameEdit(const iggy::LevelTileEdit &actual, const iggy::LevelTileEdit &expected)
{
	return actual.tile == expected.tile && actual.walkable == expected.walkable;
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameCommandFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameFrame(
	const iggy::runtime::RuntimeSessionMutationCommandFrame &actual,
	const iggy::runtime::RuntimeSessionMutationCommandFrame &expected)
{
	if (actual.levelEdits.size() != expected.levelEdits.size())
		return false;
	for (std::size_t index = 0; index < actual.levelEdits.size(); ++index) {
		if (!SameEdit(actual.levelEdits[index], expected.levelEdits[index]))
			return false;
	}
	return SameCommandFrame(actual.commandFrame, expected.commandFrame);
}

bool SameQueue(
	const iggy::runtime::RuntimeMutationCommandQueueState &actual,
	const iggy::runtime::RuntimeMutationCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
}

void TestDefaultEmptyQueuePopsNoFrameAndDrainsEmpty()
{
	const iggy::runtime::RuntimeMutationCommandQueueState queue;

	const iggy::runtime::RuntimeMutationCommandQueuePopResult pop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(queue);
	const iggy::runtime::RuntimeMutationCommandQueueDrainResult drain = iggy::runtime::RuntimeMutationCommandQueue {}.drain(queue);

	Expect(!pop.hasFrame, "empty mutation command queue pop should report no frame");
	Expect(pop.queue.frames.empty(), "empty mutation command queue pop should preserve empty queue");
	Expect(drain.frames.empty(), "empty mutation command queue drain should return no frames");
	Expect(drain.queue.frames.empty(), "empty mutation command queue drain should return empty queue");
}

void TestPushOneFrameThenPopReturnsSameFrame()
{
	const iggy::runtime::RuntimeSessionMutationCommandFrame frame = Frame(
		{ Edit(1, 2, false), Edit(3, 4, true) },
		MoveCommandFrame(5.0F, 6.0F));

	const iggy::runtime::RuntimeMutationCommandQueuePushResult push = iggy::runtime::RuntimeMutationCommandQueue {}.push({}, frame);
	const iggy::runtime::RuntimeMutationCommandQueuePopResult pop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(push.queue);

	Expect(push.status == iggy::runtime::RuntimeMutationCommandQueueStatus::Accepted, "single mutation command frame push should be accepted");
	Expect(push.queue.frames.size() == 1, "single mutation command frame push should append one frame");
	Expect(pop.hasFrame, "single mutation command frame pop should report a frame");
	Expect(SameFrame(pop.frame, frame), "single mutation command frame pop should return same edits and commands");
	Expect(pop.queue.frames.empty(), "single mutation command frame pop should leave queue empty");
}

void TestPushMultipleFramesPreservesFifoOrder()
{
	const iggy::runtime::RuntimeSessionMutationCommandFrame first = Frame({ Edit(0, 0, false) }, MoveCommandFrame(1.0F, 1.0F));
	const iggy::runtime::RuntimeSessionMutationCommandFrame second = Frame({ Edit(1, 0, true) }, WaitCommandFrame());
	const iggy::runtime::RuntimeSessionMutationCommandFrame third = Frame({ Edit(2, 0, false) }, InteractCommandFrame());
	iggy::runtime::RuntimeMutationCommandQueueState queue;
	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, first).queue;
	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, second).queue;
	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, third).queue;

	const iggy::runtime::RuntimeMutationCommandQueuePopResult firstPop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(queue);
	const iggy::runtime::RuntimeMutationCommandQueuePopResult secondPop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(firstPop.queue);
	const iggy::runtime::RuntimeMutationCommandQueuePopResult thirdPop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(secondPop.queue);

	Expect(SameFrame(firstPop.frame, first), "first mutation command queue pop should return oldest frame");
	Expect(SameFrame(secondPop.frame, second), "second mutation command queue pop should return second frame");
	Expect(SameFrame(thirdPop.frame, third), "third mutation command queue pop should return newest frame");
	Expect(thirdPop.queue.frames.empty(), "popping all mutation command frames should leave queue empty");
}

void TestDrainReturnsAllFramesInOrderAndClearsQueue()
{
	const iggy::runtime::RuntimeSessionMutationCommandFrame first = Frame({ Edit(3, 1, false) }, MoveCommandFrame(3.0F, 4.0F));
	const iggy::runtime::RuntimeSessionMutationCommandFrame second = Frame({ Edit(4, 1, true) }, WaitCommandFrame());
	iggy::runtime::RuntimeMutationCommandQueueState queue;
	queue.frames.push_back(first);
	queue.frames.push_back(second);

	const iggy::runtime::RuntimeMutationCommandQueueDrainResult result = iggy::runtime::RuntimeMutationCommandQueue {}.drain(queue);

	Expect(result.frames.size() == 2, "mutation command queue drain should return all queued frames");
	if (result.frames.size() == 2) {
		Expect(SameFrame(result.frames[0], first), "mutation command queue drain should preserve first frame");
		Expect(SameFrame(result.frames[1], second), "mutation command queue drain should preserve second frame");
	}
	Expect(result.queue.frames.empty(), "mutation command queue drain should return empty queue");
}

void TestEmptyDefaultFrameCanBeQueuedAndPopped()
{
	const iggy::runtime::RuntimeSessionMutationCommandFrame emptyFrame;

	const iggy::runtime::RuntimeMutationCommandQueuePushResult push = iggy::runtime::RuntimeMutationCommandQueue {}.push({}, emptyFrame);
	const iggy::runtime::RuntimeMutationCommandQueuePopResult pop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(push.queue);

	Expect(push.status == iggy::runtime::RuntimeMutationCommandQueueStatus::Accepted, "empty mutation command frame should be accepted");
	Expect(pop.hasFrame, "empty mutation command frame pop should still report a frame");
	Expect(pop.frame.levelEdits.empty(), "empty mutation command frame should round-trip with no edits");
	Expect(pop.frame.commandFrame.commands.empty(), "empty mutation command frame should round-trip with no commands");
}

void TestMaxFramesZeroMeansUnbounded()
{
	iggy::runtime::RuntimeMutationCommandQueueState queue;
	const iggy::runtime::RuntimeMutationCommandQueueConfig config { 0 };

	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, Frame({ Edit(0, 0, false) }, MoveCommandFrame(1.0F, 1.0F)), config).queue;
	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, Frame({ Edit(1, 0, true) }, MoveCommandFrame(2.0F, 2.0F)), config).queue;
	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, Frame({ Edit(2, 0, false) }, MoveCommandFrame(3.0F, 3.0F)), config).queue;

	Expect(queue.frames.size() == 3, "mutation command queue maxFrames zero should allow unbounded pushes");
}

void TestMaxFramesRejectsWhenFullAndLeavesQueueUnchanged()
{
	iggy::runtime::RuntimeMutationCommandQueueState queue;
	const iggy::runtime::RuntimeSessionMutationCommandFrame first = Frame({ Edit(0, 1, false) }, MoveCommandFrame(1.0F, 1.0F));
	const iggy::runtime::RuntimeSessionMutationCommandFrame rejected = Frame({ Edit(1, 1, true) }, MoveCommandFrame(2.0F, 2.0F));
	queue = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, first, { 1 }).queue;
	const iggy::runtime::RuntimeMutationCommandQueueState beforeReject = queue;

	const iggy::runtime::RuntimeMutationCommandQueuePushResult result = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, rejected, { 1 });

	Expect(result.status == iggy::runtime::RuntimeMutationCommandQueueStatus::RejectedFull, "full bounded mutation command queue should reject push");
	Expect(SameQueue(result.queue, beforeReject), "rejected mutation command push should return input queue unchanged");
	Expect(result.queue.frames.size() == 1 && SameFrame(result.queue.frames[0], first), "rejected mutation command push should not append rejected frame");
}

void TestOperationsDoNotMutateInputs()
{
	iggy::runtime::RuntimeMutationCommandQueueState queue;
	queue.frames.push_back(Frame({ Edit(5, 5, false) }, MoveCommandFrame(4.0F, 4.0F)));
	const iggy::runtime::RuntimeMutationCommandQueueState queueBefore = queue;
	iggy::runtime::RuntimeSessionMutationCommandFrame frame = Frame({ Edit(6, 6, true) }, InteractCommandFrame());
	const iggy::runtime::RuntimeSessionMutationCommandFrame frameBefore = frame;

	const iggy::runtime::RuntimeMutationCommandQueuePushResult push = iggy::runtime::RuntimeMutationCommandQueue {}.push(queue, frame);
	const iggy::runtime::RuntimeMutationCommandQueuePopResult pop = iggy::runtime::RuntimeMutationCommandQueue {}.pop(queue);
	const iggy::runtime::RuntimeMutationCommandQueueDrainResult drain = iggy::runtime::RuntimeMutationCommandQueue {}.drain(queue);

	Expect(push.status == iggy::runtime::RuntimeMutationCommandQueueStatus::Accepted, "mutation command queue immutability push setup should accept frame");
	Expect(pop.hasFrame, "mutation command queue immutability pop setup should pop frame");
	Expect(drain.frames.size() == 1, "mutation command queue immutability drain setup should drain frame copy");
	Expect(SameQueue(queue, queueBefore), "mutation command queue operations should not mutate input queue");
	Expect(SameFrame(frame, frameBefore), "mutation command queue push should not mutate input frame");
}

} // namespace

int main()
{
	TestDefaultEmptyQueuePopsNoFrameAndDrainsEmpty();
	TestPushOneFrameThenPopReturnsSameFrame();
	TestPushMultipleFramesPreservesFifoOrder();
	TestDrainReturnsAllFramesInOrderAndClearsQueue();
	TestEmptyDefaultFrameCanBeQueuedAndPopped();
	TestMaxFramesZeroMeansUnbounded();
	TestMaxFramesRejectsWhenFullAndLeavesQueueUnchanged();
	TestOperationsDoNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
