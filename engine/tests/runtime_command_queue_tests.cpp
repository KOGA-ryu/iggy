#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "actor:queue" };
const iggy::ResourceId TargetId { "target:queue" };

iggy::runtime::GameplayCommandFrame2D Frame(std::vector<iggy::runtime::GameplayCommand2D> commands)
{
	return { commands };
}

iggy::runtime::GameplayCommandFrame2D MoveFrame(float x, float y)
{
	return Frame({ iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(ActorId, { x, y }) });
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return Frame({ iggy::runtime::GameplayCommand2DFactory {}.wait(ActorId) });
}

iggy::runtime::GameplayCommandFrame2D InteractFrame()
{
	return Frame({ iggy::runtime::GameplayCommand2DFactory {}.interact(ActorId, TargetId) });
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

void TestDefaultEmptyQueuePopsNoFrameAndDrainsEmpty()
{
	const iggy::runtime::RuntimeCommandQueueState queue;

	const iggy::runtime::RuntimeCommandQueuePopResult pop = iggy::runtime::RuntimeCommandQueue {}.pop(queue);
	const iggy::runtime::RuntimeCommandQueueDrainResult drain = iggy::runtime::RuntimeCommandQueue {}.drain(queue);

	Expect(!pop.hasFrame, "empty queue pop should report no frame");
	Expect(pop.queue.frames.empty(), "empty queue pop should preserve empty queue");
	Expect(drain.frames.empty(), "empty queue drain should return no frames");
	Expect(drain.queue.frames.empty(), "empty queue drain should return empty queue");
}

void TestPushOneFrameThenPopReturnsSameFrame()
{
	const iggy::runtime::GameplayCommandFrame2D frame = MoveFrame(1.0F, 2.0F);

	const iggy::runtime::RuntimeCommandQueuePushResult push = iggy::runtime::RuntimeCommandQueue {}.push({}, frame);
	const iggy::runtime::RuntimeCommandQueuePopResult pop = iggy::runtime::RuntimeCommandQueue {}.pop(push.queue);

	Expect(push.status == iggy::runtime::RuntimeCommandQueueStatus::Accepted, "single frame push should be accepted");
	Expect(push.queue.frames.size() == 1, "single frame push should append one frame");
	Expect(pop.hasFrame, "single frame pop should report a frame");
	Expect(SameFrame(pop.frame, frame), "single frame pop should return same frame data");
	Expect(pop.queue.frames.empty(), "single frame pop should leave queue empty");
}

void TestPushMultipleFramesPreservesFifoOrder()
{
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame(1.0F, 1.0F);
	const iggy::runtime::GameplayCommandFrame2D second = WaitFrame();
	const iggy::runtime::GameplayCommandFrame2D third = InteractFrame();

	iggy::runtime::RuntimeCommandQueueState queue;
	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, first).queue;
	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, second).queue;
	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, third).queue;

	const iggy::runtime::RuntimeCommandQueuePopResult firstPop = iggy::runtime::RuntimeCommandQueue {}.pop(queue);
	const iggy::runtime::RuntimeCommandQueuePopResult secondPop = iggy::runtime::RuntimeCommandQueue {}.pop(firstPop.queue);
	const iggy::runtime::RuntimeCommandQueuePopResult thirdPop = iggy::runtime::RuntimeCommandQueue {}.pop(secondPop.queue);

	Expect(SameFrame(firstPop.frame, first), "first pop should return oldest frame");
	Expect(SameFrame(secondPop.frame, second), "second pop should return second frame");
	Expect(SameFrame(thirdPop.frame, third), "third pop should return newest frame");
	Expect(thirdPop.queue.frames.empty(), "popping all frames should leave queue empty");
}

void TestDrainReturnsAllFramesInOrderAndClearsQueue()
{
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame(3.0F, 4.0F);
	const iggy::runtime::GameplayCommandFrame2D second = WaitFrame();
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(first);
	queue.frames.push_back(second);

	const iggy::runtime::RuntimeCommandQueueDrainResult result = iggy::runtime::RuntimeCommandQueue {}.drain(queue);

	Expect(result.frames.size() == 2, "drain should return all queued frames");
	if (result.frames.size() == 2) {
		Expect(SameFrame(result.frames[0], first), "drain should preserve first frame");
		Expect(SameFrame(result.frames[1], second), "drain should preserve second frame");
	}
	Expect(result.queue.frames.empty(), "drain should return empty queue");
}

void TestEmptyGameplayCommandFrameCanBeQueuedAndPopped()
{
	const iggy::runtime::GameplayCommandFrame2D emptyFrame;

	const iggy::runtime::RuntimeCommandQueuePushResult push = iggy::runtime::RuntimeCommandQueue {}.push({}, emptyFrame);
	const iggy::runtime::RuntimeCommandQueuePopResult pop = iggy::runtime::RuntimeCommandQueue {}.pop(push.queue);

	Expect(push.status == iggy::runtime::RuntimeCommandQueueStatus::Accepted, "empty command frame should be accepted");
	Expect(pop.hasFrame, "empty command frame pop should still report a frame");
	Expect(pop.frame.commands.empty(), "empty command frame should round-trip as empty");
}

void TestMaxFramesZeroMeansUnbounded()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	const iggy::runtime::RuntimeCommandQueueConfig config { 0 };

	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, MoveFrame(1.0F, 1.0F), config).queue;
	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, MoveFrame(2.0F, 2.0F), config).queue;
	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, MoveFrame(3.0F, 3.0F), config).queue;

	Expect(queue.frames.size() == 3, "maxFrames zero should allow unbounded pushes");
}

void TestMaxFramesRejectsWhenFullAndLeavesQueueUnchanged()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame(1.0F, 1.0F);
	const iggy::runtime::GameplayCommandFrame2D rejected = MoveFrame(2.0F, 2.0F);
	queue = iggy::runtime::RuntimeCommandQueue {}.push(queue, first, { 1 }).queue;
	const iggy::runtime::RuntimeCommandQueueState beforeReject = queue;

	const iggy::runtime::RuntimeCommandQueuePushResult result = iggy::runtime::RuntimeCommandQueue {}.push(queue, rejected, { 1 });

	Expect(result.status == iggy::runtime::RuntimeCommandQueueStatus::RejectedFull, "full bounded queue should reject push");
	Expect(SameQueue(result.queue, beforeReject), "rejected push should return input queue unchanged");
	Expect(result.queue.frames.size() == 1 && SameFrame(result.queue.frames[0], first), "rejected push should not append rejected frame");
}

void TestOperationsDoNotMutateInputs()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame(4.0F, 4.0F));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	iggy::runtime::GameplayCommandFrame2D frame = InteractFrame();
	const iggy::runtime::GameplayCommandFrame2D frameBefore = frame;

	const iggy::runtime::RuntimeCommandQueuePushResult push = iggy::runtime::RuntimeCommandQueue {}.push(queue, frame);
	const iggy::runtime::RuntimeCommandQueuePopResult pop = iggy::runtime::RuntimeCommandQueue {}.pop(queue);
	const iggy::runtime::RuntimeCommandQueueDrainResult drain = iggy::runtime::RuntimeCommandQueue {}.drain(queue);

	Expect(push.status == iggy::runtime::RuntimeCommandQueueStatus::Accepted, "immutability push setup should accept frame");
	Expect(pop.hasFrame, "immutability pop setup should pop frame");
	Expect(drain.frames.size() == 1, "immutability drain setup should drain frame copy");
	Expect(SameQueue(queue, queueBefore), "queue operations should not mutate input queue");
	Expect(SameFrame(frame, frameBefore), "queue push should not mutate input frame");
}

} // namespace

int main()
{
	TestDefaultEmptyQueuePopsNoFrameAndDrainsEmpty();
	TestPushOneFrameThenPopReturnsSameFrame();
	TestPushMultipleFramesPreservesFifoOrder();
	TestDrainReturnsAllFramesInOrderAndClearsQueue();
	TestEmptyGameplayCommandFrameCanBeQueuedAndPopped();
	TestMaxFramesZeroMeansUnbounded();
	TestMaxFramesRejectsWhenFullAndLeavesQueueUnchanged();
	TestOperationsDoNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
