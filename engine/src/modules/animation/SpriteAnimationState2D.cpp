#include "modules/animation/SpriteAnimationState2D.hpp"

#include <cmath>

namespace {

float TotalDuration(const iggy::animation::SpriteClip2D &clip)
{
	float total = 0.0F;
	for (const iggy::animation::SpriteFrame2D &frame : clip.frames)
		total += frame.duration;
	return total;
}

float WrapElapsed(float elapsed, float duration)
{
	float wrapped = std::fmod(elapsed, duration);
	if (wrapped < 0.0F)
		wrapped += duration;
	return wrapped;
}

} // namespace

namespace iggy::animation {

SpriteAnimationAdvanceResult SpriteAnimationState2DStepper::advance(const SpriteAnimationAdvanceInput &input) const
{
	SpriteAnimationAdvanceResult result;
	result.state = input.state;

	if (input.clip == nullptr) {
		result.clipMissing = true;
		return result;
	}

	if (input.delta <= 0.0F || !input.state.playing || input.state.speed <= 0.0F)
		return result;

	const float totalDuration = TotalDuration(*input.clip);
	if (input.clip->frames.empty() || totalDuration <= 0.0F)
		return result;

	const float newElapsed = input.state.elapsed + (input.delta * input.state.speed);
	result.advanced = true;
	result.state.clipId = input.state.clipId;
	result.state.speed = input.state.speed;

	if (input.clip->loop) {
		result.state.elapsed = WrapElapsed(newElapsed, totalDuration);
		result.state.playing = true;
		result.state.completed = false;
		return result;
	}

	if (newElapsed >= totalDuration) {
		result.state.elapsed = totalDuration;
		result.state.playing = false;
		result.state.completed = true;
		return result;
	}

	result.state.elapsed = newElapsed;
	result.state.playing = true;
	result.state.completed = false;
	return result;
}

} // namespace iggy::animation
