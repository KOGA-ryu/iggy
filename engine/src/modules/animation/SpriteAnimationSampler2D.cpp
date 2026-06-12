#include "modules/animation/SpriteAnimationSampler2D.hpp"

namespace {

struct PositiveDurationSummary {
	float totalDuration = 0.0F;
	std::size_t lastFrameIndex = 0;
	const iggy::animation::SpriteFrame2D *lastFrame = nullptr;
};

PositiveDurationSummary SummarizePositiveFrames(const iggy::animation::SpriteClip2D &clip)
{
	PositiveDurationSummary summary;
	for (std::size_t index = 0; index < clip.frames.size(); ++index) {
		const iggy::animation::SpriteFrame2D &frame = clip.frames[index];
		if (frame.duration <= 0.0F)
			continue;
		summary.totalDuration += frame.duration;
		summary.lastFrameIndex = index;
		summary.lastFrame = &frame;
	}
	return summary;
}

float ClampElapsed(float elapsed, float totalDuration)
{
	if (elapsed < 0.0F)
		return 0.0F;
	if (elapsed > totalDuration)
		return totalDuration;
	return elapsed;
}

} // namespace

namespace iggy::animation {

SpriteAnimationSampleResult SpriteAnimationSampler2D::sample(const SpriteAnimationSampleInput &input) const
{
	SpriteAnimationSampleResult result;
	if (input.clip == nullptr)
		return result;

	if (input.clip->frames.empty()) {
		result.status = SpriteAnimationSampleStatus::NoFrames;
		return result;
	}

	const PositiveDurationSummary summary = SummarizePositiveFrames(*input.clip);
	if (summary.totalDuration <= 0.0F) {
		result.status = SpriteAnimationSampleStatus::NoFrames;
		return result;
	}

	const float sampleElapsed = ClampElapsed(input.state.elapsed, summary.totalDuration);
	if (sampleElapsed == summary.totalDuration) {
		result.status = SpriteAnimationSampleStatus::Sampled;
		result.frameIndex = summary.lastFrameIndex;
		result.frame = summary.lastFrame;
		result.localFrameTime = summary.lastFrame->duration;
		return result;
	}

	float frameStart = 0.0F;
	for (std::size_t index = 0; index < input.clip->frames.size(); ++index) {
		const SpriteFrame2D &frame = input.clip->frames[index];
		if (frame.duration <= 0.0F)
			continue;

		const float frameEnd = frameStart + frame.duration;
		if (sampleElapsed < frameEnd) {
			result.status = SpriteAnimationSampleStatus::Sampled;
			result.frameIndex = index;
			result.frame = &frame;
			result.localFrameTime = sampleElapsed - frameStart;
			return result;
		}
		frameStart = frameEnd;
	}

	result.status = SpriteAnimationSampleStatus::Sampled;
	result.frameIndex = summary.lastFrameIndex;
	result.frame = summary.lastFrame;
	result.localFrameTime = summary.lastFrame->duration;
	return result;
}

} // namespace iggy::animation
