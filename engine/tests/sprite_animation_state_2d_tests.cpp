#include <cstdlib>
#include <vector>

#include "modules/animation/SpriteAnimationState2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;

const iggy::ResourceId IdleClip { "clip:idle" };
const iggy::ResourceId RunClip { "clip:run" };

iggy::animation::SpriteFrame2D Frame(float duration)
{
	return { iggy::ResourceId { "texture:hero" }, { { 0.0F, 0.0F }, { 16.0F, 16.0F } }, duration };
}

iggy::animation::SpriteClip2D Clip(iggy::ResourceId clipId, std::vector<iggy::animation::SpriteFrame2D> frames, bool loop)
{
	return { clipId, frames, loop };
}

iggy::animation::SpriteAnimationState2D State(float elapsed = 0.0F, float speed = 1.0F, bool playing = true, bool completed = false, iggy::ResourceId clipId = IdleClip)
{
	return { clipId, elapsed, speed, playing, completed };
}

bool SameState(const iggy::animation::SpriteAnimationState2D &actual, const iggy::animation::SpriteAnimationState2D &expected)
{
	return actual.clipId == expected.clipId && Near(actual.elapsed, expected.elapsed) && Near(actual.speed, expected.speed) && actual.playing == expected.playing && actual.completed == expected.completed;
}

void ExpectState(const iggy::animation::SpriteAnimationState2D &actual, const iggy::animation::SpriteAnimationState2D &expected, const char *message)
{
	Expect(SameState(actual, expected), message);
}

void TestMissingClipPreservesState()
{
	const iggy::animation::SpriteAnimationState2D state = State(0.25F, 1.5F, true, false, RunClip);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, nullptr, 0.1F });

	ExpectState(result.state, state, "missing clip should preserve state");
	Expect(!result.advanced, "missing clip should not advance");
	Expect(result.clipMissing, "missing clip should report clipMissing");
}

void TestNonPositiveDeltaDoesNotAdvanceAndPreservesFlags()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.1F) }, true);
	const iggy::animation::SpriteAnimationState2D state = State(0.25F, 1.0F, true, true);

	const iggy::animation::SpriteAnimationAdvanceResult zero = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &clip, 0.0F });
	const iggy::animation::SpriteAnimationAdvanceResult negative = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &clip, -0.1F });

	ExpectState(zero.state, state, "zero delta should preserve state and flags");
	Expect(!zero.advanced && !zero.clipMissing, "zero delta should not advance or report missing clip");
	ExpectState(negative.state, state, "negative delta should preserve state and flags");
	Expect(!negative.advanced && !negative.clipMissing, "negative delta should not advance or report missing clip");
}

void TestStoppedStateDoesNotAdvance()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.1F), Frame(0.1F) }, true);
	const iggy::animation::SpriteAnimationState2D state = State(0.05F, 1.0F, false, false);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &clip, 0.1F });

	ExpectState(result.state, state, "stopped state should preserve state");
	Expect(!result.advanced && !result.clipMissing, "stopped state should not advance");
}

void TestNonPositiveSpeedDoesNotAdvanceAndPreservesSpeed()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.1F), Frame(0.1F) }, true);
	const iggy::animation::SpriteAnimationState2D zeroSpeed = State(0.05F, 0.0F);
	const iggy::animation::SpriteAnimationState2D negativeSpeed = State(0.05F, -2.0F);

	const iggy::animation::SpriteAnimationAdvanceResult zero = iggy::animation::SpriteAnimationState2DStepper {}.advance({ zeroSpeed, &clip, 0.1F });
	const iggy::animation::SpriteAnimationAdvanceResult negative = iggy::animation::SpriteAnimationState2DStepper {}.advance({ negativeSpeed, &clip, 0.1F });

	ExpectState(zero.state, zeroSpeed, "zero speed should preserve state exactly");
	ExpectState(negative.state, negativeSpeed, "negative speed should preserve state exactly");
	Expect(!zero.advanced && !negative.advanced, "non-positive speeds should not advance");
}

void TestLoopingClipAdvancesWithinDuration()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, true);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(0.1F, 2.0F), &clip, 0.1F });

	Expect(result.advanced && !result.clipMissing, "looping clip should advance");
	Expect(Near(result.state.elapsed, 0.3F), "looping clip should advance by delta times speed");
	Expect(result.state.playing && !result.state.completed, "looping clip should remain playing and incomplete");
}

void TestNonLoopingClipAdvancesBeforeEnd()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, false);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(0.1F, 1.5F), &clip, 0.2F });

	Expect(result.advanced && !result.clipMissing, "non-looping clip should advance before end");
	Expect(Near(result.state.elapsed, 0.4F), "non-looping clip should advance by delta times speed before end");
	Expect(result.state.playing && !result.state.completed, "non-looping clip before end should remain playing and incomplete");
}

void TestLoopingClipWrapsWhenExceedingDuration()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, true);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(0.45F), &clip, 0.1F });

	Expect(result.advanced, "looping clip should advance across end");
	Expect(Near(result.state.elapsed, 0.05F), "looping clip should wrap elapsed into clip duration");
	Expect(result.state.playing && !result.state.completed, "looping wrap should keep playing and clear completed");
}

void TestLoopingClipHandlesLargeDeltaAcrossMultipleLoops()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, true);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(0.1F), &clip, 1.25F });

	Expect(result.advanced, "looping clip should advance large deltas");
	Expect(Near(result.state.elapsed, 0.35F), "looping clip should wrap large delta deterministically");
}

void TestLoopingClipNormalizesOutOfRangeElapsedDuringAdvance()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, true);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(1.2F), &clip, 0.1F });

	Expect(result.advanced, "looping clip should advance out-of-range elapsed");
	Expect(Near(result.state.elapsed, 0.3F), "looping clip should normalize out-of-range elapsed during positive advance");
}

void TestNonLoopingClipClampsAtEndAndCompletes()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, false);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(0.45F), &clip, 0.1F });

	Expect(result.advanced, "non-looping clip should advance to end");
	Expect(Near(result.state.elapsed, 0.5F), "non-looping clip should clamp to total duration");
	Expect(!result.state.playing && result.state.completed, "non-looping clip should stop and complete at end");
}

void TestNonLoopingClipClampsAlreadyOutOfRangeElapsedDuringAdvance()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.2F), Frame(0.3F) }, false);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ State(2.0F), &clip, 0.1F });

	Expect(result.advanced, "non-looping clip should handle out-of-range elapsed during positive advance");
	Expect(Near(result.state.elapsed, 0.5F), "non-looping clip should clamp already-out-of-range elapsed to total duration");
	Expect(!result.state.playing && result.state.completed, "non-looping out-of-range elapsed should complete");
}

void TestClipIdAndSpeedArePreserved()
{
	const iggy::animation::SpriteClip2D clip = Clip(RunClip, { Frame(0.5F) }, true);
	const iggy::animation::SpriteAnimationState2D state = State(0.1F, 1.25F, true, false, RunClip);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &clip, 0.1F });

	Expect(result.state.clipId == RunClip, "advance should preserve clip id exactly");
	Expect(Near(result.state.speed, 1.25F), "advance should preserve speed exactly");
}

void TestAdvancingLoopingClipClearsCompleted()
{
	const iggy::animation::SpriteClip2D clip = Clip(IdleClip, { Frame(0.5F) }, true);
	const iggy::animation::SpriteAnimationState2D state = State(0.1F, 1.0F, true, true);

	const iggy::animation::SpriteAnimationAdvanceResult result = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &clip, 0.1F });

	Expect(result.advanced, "looping completed state should still advance if playing");
	Expect(result.state.playing && !result.state.completed, "advancing looping clip should reset completed false");
}

void TestInvalidDirectClipInputDoesNotAdvance()
{
	const iggy::animation::SpriteClip2D emptyClip = Clip(IdleClip, {}, true);
	const iggy::animation::SpriteClip2D zeroTotalClip = Clip(IdleClip, { Frame(0.1F), Frame(-0.1F) }, true);
	const iggy::animation::SpriteAnimationState2D state = State(0.1F);

	const iggy::animation::SpriteAnimationAdvanceResult emptyResult = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &emptyClip, 0.1F });
	const iggy::animation::SpriteAnimationAdvanceResult zeroTotalResult = iggy::animation::SpriteAnimationState2DStepper {}.advance({ state, &zeroTotalClip, 0.1F });

	ExpectState(emptyResult.state, state, "empty direct clip input should preserve state");
	ExpectState(zeroTotalResult.state, state, "non-positive total duration direct clip input should preserve state");
	Expect(!emptyResult.advanced && !zeroTotalResult.advanced, "invalid direct clip inputs should not advance");
}

} // namespace

int main()
{
	TestMissingClipPreservesState();
	TestNonPositiveDeltaDoesNotAdvanceAndPreservesFlags();
	TestStoppedStateDoesNotAdvance();
	TestNonPositiveSpeedDoesNotAdvanceAndPreservesSpeed();
	TestLoopingClipAdvancesWithinDuration();
	TestNonLoopingClipAdvancesBeforeEnd();
	TestLoopingClipWrapsWhenExceedingDuration();
	TestLoopingClipHandlesLargeDeltaAcrossMultipleLoops();
	TestLoopingClipNormalizesOutOfRangeElapsedDuringAdvance();
	TestNonLoopingClipClampsAtEndAndCompletes();
	TestNonLoopingClipClampsAlreadyOutOfRangeElapsedDuringAdvance();
	TestClipIdAndSpeedArePreserved();
	TestAdvancingLoopingClipClearsCompleted();
	TestInvalidDirectClipInputDoesNotAdvance();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
