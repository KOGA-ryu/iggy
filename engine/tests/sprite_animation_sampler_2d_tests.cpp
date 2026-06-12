#include <cstdlib>
#include <vector>

#include "modules/animation/SpriteAnimationSampler2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

const iggy::ResourceId IdleClip { "clip:idle" };
const iggy::ResourceId TextureA { "texture:a" };
const iggy::ResourceId TextureB { "texture:b" };
const iggy::ResourceId TextureC { "texture:c" };

iggy::Rect2 Rect(float x, float y, float width, float height)
{
	return { { x, y }, { width, height } };
}

bool SameRect(iggy::Rect2 actual, iggy::Rect2 expected)
{
	return NearVec(actual.position, expected.position) && NearVec(actual.size, expected.size);
}

iggy::animation::SpriteFrame2D Frame(float duration, iggy::ResourceId textureId = TextureA, iggy::Rect2 sourceRect = Rect(0.0F, 0.0F, 16.0F, 16.0F))
{
	return { textureId, sourceRect, duration };
}

iggy::animation::SpriteClip2D Clip(std::vector<iggy::animation::SpriteFrame2D> frames)
{
	return { IdleClip, frames, true };
}

iggy::animation::SpriteAnimationState2D State(float elapsed, bool playing = true, bool completed = false, float speed = 1.0F)
{
	return { IdleClip, elapsed, speed, playing, completed };
}

void ExpectSample(const iggy::animation::SpriteAnimationSampleResult &result, std::size_t frameIndex, const iggy::animation::SpriteFrame2D *frame, float localFrameTime, const char *message)
{
	Expect(result.status == iggy::animation::SpriteAnimationSampleStatus::Sampled && result.frameIndex == frameIndex && result.frame == frame && Near(result.localFrameTime, localFrameTime), message);
}

void TestMissingClipReturnsClipMissing()
{
	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.0F), nullptr });

	Expect(result.status == iggy::animation::SpriteAnimationSampleStatus::ClipMissing, "missing clip should return ClipMissing");
	Expect(result.frame == nullptr, "missing clip should return null frame");
}

void TestEmptyClipFramesReturnNoFrames()
{
	const iggy::animation::SpriteClip2D clip = Clip({});

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.0F), &clip });

	Expect(result.status == iggy::animation::SpriteAnimationSampleStatus::NoFrames, "empty clip frames should return NoFrames");
	Expect(result.frame == nullptr, "empty clip frames should return null frame");
}

void TestOnlyNonPositiveDurationsReturnNoFrames()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.0F), Frame(-0.2F) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.0F), &clip });

	Expect(result.status == iggy::animation::SpriteAnimationSampleStatus::NoFrames, "only non-positive durations should return NoFrames");
	Expect(result.frame == nullptr, "only non-positive durations should return null frame");
}

void TestElapsedZeroSamplesFirstPositiveFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.0F), &clip });

	ExpectSample(result, 0, &clip.frames[0], 0.0F, "elapsed zero should sample first positive frame");
}

void TestElapsedInsideFirstFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.1F), &clip });

	ExpectSample(result, 0, &clip.frames[0], 0.1F, "elapsed inside first frame should sample first frame with local time");
}

void TestElapsedInsideLaterFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB), Frame(0.2F, TextureC) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.4F), &clip });

	ExpectSample(result, 1, &clip.frames[1], 0.15F, "elapsed inside later frame should sample original frame index with local time");
}

void TestBoundarySelectsNextPositiveFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.25F), &clip });

	ExpectSample(result, 1, &clip.frames[1], 0.0F, "elapsed exactly at frame boundary should select next positive frame");
}

void TestTotalDurationReturnsLastFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.75F), &clip });

	ExpectSample(result, 1, &clip.frames[1], 0.5F, "elapsed exactly at total duration should return last frame with full local time");
}

void TestNegativeElapsedClampsToFirstPositiveFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(-1.0F), &clip });

	ExpectSample(result, 0, &clip.frames[0], 0.0F, "negative elapsed should clamp to first positive frame");
}

void TestElapsedBeyondDurationClampsToLastFrame()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(9.0F), &clip });

	ExpectSample(result, 1, &clip.frames[1], 0.5F, "elapsed beyond duration should clamp to last positive frame");
}

void TestNonPositiveFramesAreSkippedAndOriginalIndexIsReturned()
{
	const iggy::animation::SpriteClip2D clip = Clip({
		Frame(0.0F, TextureA),
		Frame(0.25F, TextureB),
		Frame(-0.1F, TextureA),
		Frame(0.5F, TextureC),
	});

	const iggy::animation::SpriteAnimationSampleResult first = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.0F), &clip });
	const iggy::animation::SpriteAnimationSampleResult second = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.25F), &clip });

	ExpectSample(first, 1, &clip.frames[1], 0.0F, "sampler should skip non-positive first frame and return original index 1");
	ExpectSample(second, 3, &clip.frames[3], 0.0F, "boundary should skip non-positive frame and return original index 3");
}

void TestSampledFramePointerPreservesMetadata()
{
	const iggy::Rect2 sourceRect = Rect(4.0F, 8.0F, 12.0F, 16.0F);
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB, sourceRect) });

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ State(0.3F), &clip });

	ExpectSample(result, 1, &clip.frames[1], 0.05F, "sample should return pointer to original frame");
	if (result.frame != nullptr) {
		Expect(result.frame->textureId == TextureB, "sampled frame pointer should expose texture id exactly");
		Expect(SameRect(result.frame->sourceRect, sourceRect), "sampled frame pointer should expose source rect exactly");
		Expect(Near(result.frame->duration, 0.5F), "sampled frame pointer should expose duration exactly");
	}
}

void TestSamplingDoesNotChangeState()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F), Frame(0.5F) });
	const iggy::animation::SpriteAnimationState2D state = State(0.3F, false, true, 2.0F);

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ state, &clip });

	Expect(result.status == iggy::animation::SpriteAnimationSampleStatus::Sampled, "state mutation test should still sample a frame");
	Expect(state.clipId == IdleClip && Near(state.elapsed, 0.3F) && Near(state.speed, 2.0F) && !state.playing && state.completed, "sampling should not mutate input state");
}

void TestSamplerIgnoresPlayingCompletedAndSpeed()
{
	const iggy::animation::SpriteClip2D clip = Clip({ Frame(0.25F, TextureA), Frame(0.5F, TextureB) });
	const iggy::animation::SpriteAnimationState2D state = State(0.3F, false, true, 100.0F);

	const iggy::animation::SpriteAnimationSampleResult result = iggy::animation::SpriteAnimationSampler2D {}.sample({ state, &clip });

	ExpectSample(result, 1, &clip.frames[1], 0.05F, "sampler should select by elapsed only, regardless of playing/completed/speed");
}

} // namespace

int main()
{
	TestMissingClipReturnsClipMissing();
	TestEmptyClipFramesReturnNoFrames();
	TestOnlyNonPositiveDurationsReturnNoFrames();
	TestElapsedZeroSamplesFirstPositiveFrame();
	TestElapsedInsideFirstFrame();
	TestElapsedInsideLaterFrame();
	TestBoundarySelectsNextPositiveFrame();
	TestTotalDurationReturnsLastFrame();
	TestNegativeElapsedClampsToFirstPositiveFrame();
	TestElapsedBeyondDurationClampsToLastFrame();
	TestNonPositiveFramesAreSkippedAndOriginalIndexIsReturned();
	TestSampledFramePointerPreservesMetadata();
	TestSamplingDoesNotChangeState();
	TestSamplerIgnoresPlayingCompletedAndSpeed();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
