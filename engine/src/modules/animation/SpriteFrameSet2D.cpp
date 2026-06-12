#include "modules/animation/SpriteFrameSet2D.hpp"

#include <utility>

namespace {

bool HasEarlierMatchingClipId(const std::vector<iggy::animation::SpriteClip2D> &clips, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (clips[index].clipId == clips[currentIndex].clipId)
			return true;
	}
	return false;
}

} // namespace

namespace iggy::animation {

SpriteFrameSet2D::SpriteFrameSet2D(std::vector<SpriteClip2D> clips)
    : clips_(std::move(clips))
{
}

bool SpriteFrameSet2D::containsClip(const ResourceId &clipId) const
{
	return findClip(clipId) != nullptr;
}

const SpriteClip2D *SpriteFrameSet2D::findClip(const ResourceId &clipId) const
{
	for (const SpriteClip2D &clip : clips_) {
		if (clip.clipId == clipId)
			return &clip;
	}
	return nullptr;
}

const std::vector<SpriteClip2D> &SpriteFrameSet2D::clips() const
{
	return clips_;
}

SpriteFrameSet2DBuildResult SpriteFrameSet2DBuilder::build(std::vector<SpriteClip2D> clips) const
{
	SpriteFrameSet2DBuildResult result;
	for (std::size_t clipIndex = 0; clipIndex < clips.size(); ++clipIndex) {
		const SpriteClip2D &clip = clips[clipIndex];
		if (clip.clipId.empty()) {
			result.issues.push_back({
				SpriteFrameSet2DIssueCode::EmptyClipId,
				clip.clipId,
				0,
			});
		}
		if (HasEarlierMatchingClipId(clips, clipIndex)) {
			result.issues.push_back({
				SpriteFrameSet2DIssueCode::DuplicateClipId,
				clip.clipId,
				0,
			});
		}
		if (clip.frames.empty()) {
			result.issues.push_back({
				SpriteFrameSet2DIssueCode::EmptyClipFrames,
				clip.clipId,
				0,
			});
		}
		for (std::size_t frameIndex = 0; frameIndex < clip.frames.size(); ++frameIndex) {
			if (clip.frames[frameIndex].duration <= 0.0F) {
				result.issues.push_back({
					SpriteFrameSet2DIssueCode::InvalidFrameDuration,
					clip.clipId,
					frameIndex,
				});
			}
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.frameSet = SpriteFrameSet2D { std::move(clips) };
	return result;
}

} // namespace iggy::animation
