#include <cstdlib>
#include <vector>

#include "modules/animation/SpriteFrameSet2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

const iggy::ResourceId IdleClip { "clip:idle" };
const iggy::ResourceId RunClip { "clip:run" };
const iggy::ResourceId TextureId { "texture:hero" };

bool SameRect(iggy::Rect2 actual, iggy::Rect2 expected)
{
	return NearVec(actual.position, expected.position) && NearVec(actual.size, expected.size);
}

iggy::Rect2 Rect(float x, float y, float width, float height)
{
	return { { x, y }, { width, height } };
}

iggy::animation::SpriteFrame2D Frame(float duration, iggy::ResourceId textureId = TextureId, iggy::Rect2 sourceRect = Rect(0.0F, 0.0F, 16.0F, 16.0F))
{
	return { textureId, sourceRect, duration };
}

iggy::animation::SpriteClip2D Clip(iggy::ResourceId clipId, std::vector<iggy::animation::SpriteFrame2D> frames, bool loop = true)
{
	return { clipId, frames, loop };
}

void ExpectFrame(const iggy::animation::SpriteFrame2D &frame, const iggy::ResourceId &textureId, iggy::Rect2 sourceRect, float duration, const char *message)
{
	Expect(frame.textureId == textureId && SameRect(frame.sourceRect, sourceRect) && Near(frame.duration, duration), message);
}

void TestEmptyInputBuildsValidEmptyFrameSet()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({});

	Expect(result.built, "empty input should build a valid sprite frame set");
	Expect(result.issues.empty(), "empty input should produce no issues");
	Expect(result.frameSet.clips().empty(), "empty input should produce empty clips");
}

void TestSingleValidClipBuildsAndFindsExactClip()
{
	const iggy::Rect2 sourceRect = Rect(2.0F, 4.0F, 8.0F, 12.0F);
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.25F, TextureId, sourceRect) }, false),
	});

	Expect(result.built, "single valid clip should build");
	Expect(result.issues.empty(), "single valid clip should produce no issues");
	Expect(result.frameSet.containsClip(IdleClip), "frame set should contain valid clip id");
	const iggy::animation::SpriteClip2D *found = result.frameSet.findClip(IdleClip);
	Expect(found != nullptr, "findClip should return valid clip");
	if (found != nullptr) {
		Expect(found->clipId == IdleClip, "found clip should preserve clip id");
		Expect(!found->loop, "found clip should preserve loop false");
		Expect(found->frames.size() == 1, "found clip should preserve frame count");
		if (found->frames.size() == 1)
			ExpectFrame(found->frames[0], TextureId, sourceRect, 0.25F, "found frame should preserve texture id, source rect, and duration");
	}
}

void TestMissingClipLookupReturnsFalseAndNull()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F) }),
	});

	Expect(result.built, "lookup test setup should build");
	Expect(!result.frameSet.containsClip(RunClip), "missing clip id should not be contained");
	Expect(result.frameSet.findClip(RunClip) == nullptr, "missing clip id should return null from findClip");
}

void TestMultipleValidClipsPreserveInputOrder()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F) }),
		Clip(RunClip, { Frame(0.08F), Frame(0.09F) }),
		Clip(iggy::ResourceId { "clip:attack" }, { Frame(0.2F) }),
	});

	Expect(result.built, "multiple valid clips should build");
	Expect(result.frameSet.clips().size() == 3, "multiple valid clips should be preserved");
	if (result.frameSet.clips().size() == 3) {
		Expect(result.frameSet.clips()[0].clipId == IdleClip, "first clip should preserve input order");
		Expect(result.frameSet.clips()[1].clipId == RunClip, "second clip should preserve input order");
		Expect(result.frameSet.clips()[2].clipId == iggy::ResourceId { "clip:attack" }, "third clip should preserve input order");
	}
}

void TestEmptyClipIdFailsWithIssueAndEmptyFrameSet()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip({}, { Frame(0.1F) }),
	});

	Expect(!result.built, "empty clip id should fail frame set build");
	Expect(result.frameSet.clips().empty(), "empty clip id should return empty frame set");
	Expect(result.issues.size() == 1, "empty clip id should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::animation::SpriteFrameSet2DIssueCode::EmptyClipId, "empty clip id issue should use EmptyClipId code");
		Expect(result.issues[0].clipId.empty(), "empty clip id issue should preserve empty clip id");
		Expect(result.issues[0].frameIndex == 0, "empty clip id issue should use frame index zero");
	}
}

void TestDuplicateClipIdFailsWithIssueAndEmptyFrameSet()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F) }),
		Clip(IdleClip, { Frame(0.2F) }, false),
	});

	Expect(!result.built, "duplicate clip id should fail frame set build");
	Expect(result.frameSet.clips().empty(), "duplicate clip id should return empty frame set");
	Expect(result.issues.size() == 1, "duplicate clip id should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::animation::SpriteFrameSet2DIssueCode::DuplicateClipId, "duplicate clip id issue should use DuplicateClipId code");
		Expect(result.issues[0].clipId == IdleClip, "duplicate clip id issue should preserve duplicate clip id");
		Expect(result.issues[0].frameIndex == 0, "duplicate clip id issue should use frame index zero");
	}
}

void TestEmptyFramesFailsWithIssue()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, {}),
	});

	Expect(!result.built, "empty clip frames should fail frame set build");
	Expect(result.frameSet.clips().empty(), "empty clip frames should return empty frame set");
	Expect(result.issues.size() == 1, "empty clip frames should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::animation::SpriteFrameSet2DIssueCode::EmptyClipFrames, "empty frames issue should use EmptyClipFrames code");
		Expect(result.issues[0].clipId == IdleClip, "empty frames issue should preserve clip id");
		Expect(result.issues[0].frameIndex == 0, "empty frames issue should use frame index zero");
	}
}

void TestInvalidFrameDurationsFailWithFrameIndexes()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.0F), Frame(0.1F), Frame(-0.25F) }),
	});

	Expect(!result.built, "invalid frame durations should fail frame set build");
	Expect(result.frameSet.clips().empty(), "invalid frame durations should return empty frame set");
	Expect(result.issues.size() == 2, "invalid frame durations should produce one issue per invalid frame");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::animation::SpriteFrameSet2DIssueCode::InvalidFrameDuration && result.issues[0].frameIndex == 0, "zero duration should report invalid frame index 0");
		Expect(result.issues[1].code == iggy::animation::SpriteFrameSet2DIssueCode::InvalidFrameDuration && result.issues[1].frameIndex == 2, "negative duration should report invalid frame index 2");
	}
}

void TestMultipleIssuesReportedInDeterministicOrder()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F) }),
		Clip({}, {}),
		Clip(IdleClip, { Frame(0.0F), Frame(-0.5F) }),
	});

	Expect(!result.built, "multiple frame set issues should fail build");
	Expect(result.frameSet.clips().empty(), "multiple frame set issues should return empty frame set");
	Expect(result.issues.size() == 5, "multiple frame set issues should all be reported");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::animation::SpriteFrameSet2DIssueCode::EmptyClipId, "empty clip id should be first issue for empty invalid clip");
		Expect(result.issues[1].code == iggy::animation::SpriteFrameSet2DIssueCode::EmptyClipFrames, "empty frames should follow empty clip id for same clip");
		Expect(result.issues[2].code == iggy::animation::SpriteFrameSet2DIssueCode::DuplicateClipId, "duplicate clip id should be reported before frame duration issues");
		Expect(result.issues[3].code == iggy::animation::SpriteFrameSet2DIssueCode::InvalidFrameDuration && result.issues[3].frameIndex == 0, "first invalid duplicate frame should preserve frame index 0");
		Expect(result.issues[4].code == iggy::animation::SpriteFrameSet2DIssueCode::InvalidFrameDuration && result.issues[4].frameIndex == 1, "second invalid duplicate frame should preserve frame index 1");
	}
}

void TestEmptyTextureIdIsAllowedAndPreserved()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F, {}) }),
	});

	Expect(result.built, "empty texture id should be allowed");
	const iggy::animation::SpriteClip2D *found = result.frameSet.findClip(IdleClip);
	Expect(found != nullptr, "empty texture id clip should be findable");
	if (found != nullptr && found->frames.size() == 1)
		Expect(found->frames[0].textureId.empty(), "empty texture id should be preserved");
	else
		Expect(false, "empty texture id clip should preserve one frame");
}

void TestSourceRectIsPreservedExactlyWithoutValidation()
{
	const iggy::Rect2 sourceRect = Rect(8.0F, 16.0F, -4.0F, 0.0F);
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F, TextureId, sourceRect) }),
	});

	Expect(result.built, "source rect shape should not be validated in this slice");
	const iggy::animation::SpriteClip2D *found = result.frameSet.findClip(IdleClip);
	Expect(found != nullptr, "source rect preservation clip should be findable");
	if (found != nullptr && found->frames.size() == 1)
		Expect(SameRect(found->frames[0].sourceRect, sourceRect), "source rect should be preserved exactly");
	else
		Expect(false, "source rect preservation clip should preserve one frame");
}

void TestLoopFalseIsAllowedAndPreserved()
{
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(IdleClip, { Frame(0.1F) }, false),
	});

	Expect(result.built, "loop false should be allowed");
	const iggy::animation::SpriteClip2D *found = result.frameSet.findClip(IdleClip);
	Expect(found != nullptr, "loop false clip should be findable");
	if (found != nullptr)
		Expect(!found->loop, "loop false should be preserved");
}

void TestNamespacedAndUnqualifiedClipIdsAreDistinct()
{
	const iggy::ResourceId namespaced { "clip:idle" };
	const iggy::ResourceId unqualified { "idle" };
	const iggy::animation::SpriteFrameSet2DBuildResult result = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		Clip(namespaced, { Frame(0.1F) }),
		Clip(unqualified, { Frame(0.2F) }),
	});

	Expect(result.built, "namespaced and unqualified clip ids should build as distinct clips");
	Expect(result.frameSet.containsClip(namespaced), "frame set should contain namespaced clip id");
	Expect(result.frameSet.containsClip(unqualified), "frame set should contain unqualified clip id");
	const iggy::animation::SpriteClip2D *found = result.frameSet.findClip(unqualified);
	Expect(found != nullptr, "unqualified clip should be findable");
	if (found != nullptr && found->frames.size() == 1)
		Expect(Near(found->frames[0].duration, 0.2F), "unqualified lookup should find exact unqualified clip");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyFrameSet();
	TestSingleValidClipBuildsAndFindsExactClip();
	TestMissingClipLookupReturnsFalseAndNull();
	TestMultipleValidClipsPreserveInputOrder();
	TestEmptyClipIdFailsWithIssueAndEmptyFrameSet();
	TestDuplicateClipIdFailsWithIssueAndEmptyFrameSet();
	TestEmptyFramesFailsWithIssue();
	TestInvalidFrameDurationsFailWithFrameIndexes();
	TestMultipleIssuesReportedInDeterministicOrder();
	TestEmptyTextureIdIsAllowedAndPreserved();
	TestSourceRectIsPreservedExactlyWithoutValidation();
	TestLoopFalseIsAllowedAndPreserved();
	TestNamespacedAndUnqualifiedClipIdsAreDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
