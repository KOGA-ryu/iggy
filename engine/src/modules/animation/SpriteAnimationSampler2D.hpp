#pragma once

#include <cstddef>

#include "modules/animation/SpriteAnimationState2D.hpp"

namespace iggy::animation {

enum class SpriteAnimationSampleStatus {
	ClipMissing,
	NoFrames,
	Sampled,
};

struct SpriteAnimationSampleInput {
	SpriteAnimationState2D state;
	const SpriteClip2D *clip = nullptr;
};

struct SpriteAnimationSampleResult {
	SpriteAnimationSampleStatus status = SpriteAnimationSampleStatus::ClipMissing;
	std::size_t frameIndex = 0;
	const SpriteFrame2D *frame = nullptr;
	float localFrameTime = 0.0F;
};

class SpriteAnimationSampler2D {
public:
	[[nodiscard]] SpriteAnimationSampleResult sample(const SpriteAnimationSampleInput &input) const;
};

} // namespace iggy::animation
