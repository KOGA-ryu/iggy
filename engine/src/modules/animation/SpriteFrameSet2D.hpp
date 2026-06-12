#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Rect2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy::animation {

struct SpriteFrame2D {
	ResourceId textureId;
	Rect2 sourceRect;
	float duration = 0.0F;
};

struct SpriteClip2D {
	ResourceId clipId;
	std::vector<SpriteFrame2D> frames;
	bool loop = true;
};

enum class SpriteFrameSet2DIssueCode {
	EmptyClipId,
	DuplicateClipId,
	EmptyClipFrames,
	InvalidFrameDuration,
};

struct SpriteFrameSet2DIssue {
	SpriteFrameSet2DIssueCode code = SpriteFrameSet2DIssueCode::EmptyClipId;
	ResourceId clipId;
	std::size_t frameIndex = 0;
};

class SpriteFrameSet2D {
public:
	SpriteFrameSet2D() = default;
	explicit SpriteFrameSet2D(std::vector<SpriteClip2D> clips);

	[[nodiscard]] bool containsClip(const ResourceId &clipId) const;
	[[nodiscard]] const SpriteClip2D *findClip(const ResourceId &clipId) const;
	[[nodiscard]] const std::vector<SpriteClip2D> &clips() const;

private:
	std::vector<SpriteClip2D> clips_;
};

struct SpriteFrameSet2DBuildResult {
	bool built = false;
	SpriteFrameSet2D frameSet;
	std::vector<SpriteFrameSet2DIssue> issues;
};

class SpriteFrameSet2DBuilder {
public:
	[[nodiscard]] SpriteFrameSet2DBuildResult build(std::vector<SpriteClip2D> clips) const;
};

} // namespace iggy::animation
