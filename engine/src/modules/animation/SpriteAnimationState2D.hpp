#pragma once

#include "core/resource/ResourceId.hpp"
#include "modules/animation/SpriteFrameSet2D.hpp"

namespace iggy::animation {

struct SpriteAnimationState2D {
	ResourceId clipId;
	float elapsed = 0.0F;
	float speed = 1.0F;
	bool playing = true;
	bool completed = false;
};

struct SpriteAnimationAdvanceInput {
	SpriteAnimationState2D state;
	const SpriteClip2D *clip = nullptr;
	float delta = 0.0F;
};

struct SpriteAnimationAdvanceResult {
	SpriteAnimationState2D state;
	bool advanced = false;
	bool clipMissing = false;
};

class SpriteAnimationState2DStepper {
public:
	[[nodiscard]] SpriteAnimationAdvanceResult advance(const SpriteAnimationAdvanceInput &input) const;
};

} // namespace iggy::animation
