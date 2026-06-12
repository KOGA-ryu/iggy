#include <cstdlib>

#include "scene/sprite/SpriteRenderCommands2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

const iggy::ResourceId ClipId { "clip:idle" };
const iggy::ResourceId MaterialId { "material:hero" };
const iggy::ResourceId TextureId { "texture:hero" };

bool SameBounds(iggy::Aabb2 actual, iggy::Aabb2 expected)
{
	return NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max);
}

bool SameRect(iggy::Rect2 actual, iggy::Rect2 expected)
{
	return NearVec(actual.position, expected.position) && NearVec(actual.size, expected.size);
}

iggy::Rect2 Rect(float x, float y, float width, float height)
{
	return { { x, y }, { width, height } };
}

iggy::animation::SpriteFrame2D Frame(float duration = 0.1F, iggy::ResourceId textureId = TextureId, iggy::Rect2 sourceRect = Rect(0.0F, 0.0F, 16.0F, 16.0F))
{
	return { textureId, sourceRect, duration };
}

iggy::animation::SpriteAnimationSampleResult Sample(const iggy::animation::SpriteFrame2D *frame)
{
	return { iggy::animation::SpriteAnimationSampleStatus::Sampled, 0, frame, 0.0F };
}

iggy::SpriteRenderCommandConfig2D Config(int layer = 3)
{
	return { MaterialId, { 2.0F, 3.0F }, { 1.0F, 1.0F }, { 0.5F, 0.5F }, layer };
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void ExpectTexturePayload(const iggy::render::RenderCommand2D &command, const iggy::ResourceId &textureId, iggy::Rect2 sourceRect, const char *message)
{
	Expect(command.texture.textureId == textureId && SameRect(command.texture.sourceRect, sourceRect) && command.texture.hasSourceRect, message);
}

void TestClipMissingSampleDoesNotEmit()
{
	const iggy::animation::SpriteAnimationSampleResult sample;

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(sample, Config());

	Expect(!result.emitted, "ClipMissing sample should not emit");
	Expect(result.commands.commands.empty(), "ClipMissing sample should produce empty commands");
}

void TestNoFramesSampleDoesNotEmit()
{
	const iggy::animation::SpriteAnimationSampleResult sample { iggy::animation::SpriteAnimationSampleStatus::NoFrames, 0, nullptr, 0.0F };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(sample, Config());

	Expect(!result.emitted, "NoFrames sample should not emit");
	Expect(result.commands.commands.empty(), "NoFrames sample should produce empty commands");
}

void TestSampledNullFrameDoesNotEmit()
{
	const iggy::animation::SpriteAnimationSampleResult sample { iggy::animation::SpriteAnimationSampleStatus::Sampled, 0, nullptr, 0.0F };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(sample, Config());

	Expect(!result.emitted, "Sampled null frame should not emit");
	Expect(result.commands.commands.empty(), "Sampled null frame should produce empty commands");
}

void TestSampledFrameEmitsOneQuad()
{
	const iggy::animation::SpriteFrame2D frame = Frame();

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), Config(4));

	Expect(result.emitted, "Sampled frame should emit");
	Expect(result.commands.commands.size() == 1, "Sampled frame should emit one command");
	if (result.commands.commands.size() == 1) {
		ExpectCommand(result.commands.commands[0], { { 1.5F, 2.5F }, { 2.5F, 3.5F } }, MaterialId, 4, 0, "Sampled frame should emit one quad with default bounds");
		ExpectTexturePayload(result.commands.commands[0], TextureId, Rect(0.0F, 0.0F, 16.0F, 16.0F), "Sampled frame should emit texture id and source rect payload");
	}
}

void TestBoundsUsePositionSizeAndAnchor()
{
	const iggy::animation::SpriteFrame2D frame = Frame();
	const iggy::SpriteRenderCommandConfig2D config { MaterialId, { 10.0F, 20.0F }, { 2.0F, 4.0F }, { 0.5F, 0.5F }, 1 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), config);

	Expect(result.commands.commands.size() == 1, "bounds test should emit one command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(result.commands.commands[0], { { 9.0F, 18.0F }, { 11.0F, 22.0F } }, MaterialId, 1, 0, "bounds should match position minus effective size times anchor");
}

void TestNonDefaultAnchorChangesBounds()
{
	const iggy::animation::SpriteFrame2D frame = Frame();
	const iggy::SpriteRenderCommandConfig2D config { MaterialId, { 10.0F, 20.0F }, { 2.0F, 4.0F }, { 0.0F, 1.0F }, 1 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), config);

	Expect(result.commands.commands.size() == 1, "non-default anchor test should emit one command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(result.commands.commands[0], { { 10.0F, 16.0F }, { 12.0F, 20.0F } }, MaterialId, 1, 0, "non-default anchor should change bounds without clamping");
}

void TestNegativeSizeIsNormalized()
{
	const iggy::animation::SpriteFrame2D frame = Frame();
	const iggy::SpriteRenderCommandConfig2D config { MaterialId, { 0.0F, 0.0F }, { -2.0F, -4.0F }, { 0.5F, 0.5F }, 1 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), config);

	Expect(result.commands.commands.size() == 1, "negative size test should emit one command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(result.commands.commands[0], { { -1.0F, -2.0F }, { 1.0F, 2.0F } }, MaterialId, 1, 0, "negative size should normalize by absolute value");
}

void TestZeroSizeProducesDegenerateBounds()
{
	const iggy::animation::SpriteFrame2D frame = Frame();
	const iggy::SpriteRenderCommandConfig2D config { MaterialId, { 5.0F, 6.0F }, { 0.0F, 0.0F }, { 0.5F, 0.5F }, 1 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), config);

	Expect(result.commands.commands.size() == 1, "zero size test should emit one command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(result.commands.commands[0], { { 5.0F, 6.0F }, { 5.0F, 6.0F } }, MaterialId, 1, 0, "zero size should produce degenerate bounds");
}

void TestMaterialIdAndLayerAreCopied()
{
	const iggy::animation::SpriteFrame2D frame = Frame();
	const iggy::ResourceId material { "material:custom" };
	const iggy::SpriteRenderCommandConfig2D config { material, { 0.0F, 0.0F }, { 1.0F, 1.0F }, { 0.0F, 0.0F }, -7 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), config);

	Expect(result.commands.commands.size() == 1, "material/layer test should emit one command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(result.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, material, -7, 0, "material id and layer should be copied exactly");
}

void TestEmptyMaterialIdIsPreserved()
{
	const iggy::animation::SpriteFrame2D frame = Frame();
	const iggy::SpriteRenderCommandConfig2D config { {}, { 0.0F, 0.0F }, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 0 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), config);

	Expect(result.commands.commands.size() == 1, "empty material id test should emit one command");
	if (result.commands.commands.size() == 1)
		Expect(result.commands.commands[0].materialId.empty(), "empty material id should be preserved without validation");
}

void TestEmptyFrameTextureIdIsPreserved()
{
	const iggy::Rect2 sourceRect = Rect(1.0F, 2.0F, 3.0F, 4.0F);
	const iggy::animation::SpriteFrame2D frame = Frame(0.1F, {}, sourceRect);

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), Config());

	Expect(result.commands.commands.size() == 1, "empty frame texture id test should emit one command");
	if (result.commands.commands.size() == 1)
		Expect(result.commands.commands[0].texture.textureId.empty() && result.commands.commands[0].texture.hasSourceRect && SameRect(result.commands.commands[0].texture.sourceRect, sourceRect), "empty sampled frame texture id should be preserved with source rect payload present");
}

void TestSourceRectIsPreservedExactly()
{
	const iggy::Rect2 sourceRect = Rect(-4.0F, 8.0F, 0.0F, -16.0F);
	const iggy::animation::SpriteFrame2D frame = Frame(0.1F, TextureId, sourceRect);

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), Config());

	Expect(result.commands.commands.size() == 1, "source rect preservation test should emit one command");
	if (result.commands.commands.size() == 1)
		ExpectTexturePayload(result.commands.commands[0], TextureId, sourceRect, "sprite render command should preserve zero/negative source rect values exactly");
}

void TestSampleFrameIsNotModified()
{
	const iggy::Rect2 sourceRect = Rect(4.0F, 8.0F, 12.0F, 16.0F);
	const iggy::animation::SpriteFrame2D frame = Frame(0.25F, TextureId, sourceRect);

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(Sample(&frame), Config());

	Expect(result.emitted, "frame mutation test should emit a command");
	Expect(frame.textureId == TextureId, "sprite render bridge should not modify frame texture id");
	Expect(SameRect(frame.sourceRect, sourceRect), "sprite render bridge should not modify frame source rect");
	Expect(Near(frame.duration, 0.25F), "sprite render bridge should not modify frame duration");
}

void TestManualAnimationToRenderCommandComposition()
{
	const iggy::animation::SpriteFrameSet2DBuildResult frameSet = iggy::animation::SpriteFrameSet2DBuilder {}.build({
		{ ClipId, { Frame(0.2F, TextureId, Rect(0.0F, 0.0F, 16.0F, 16.0F)), Frame(0.3F, iggy::ResourceId { "texture:hero_run" }, Rect(16.0F, 0.0F, 16.0F, 16.0F)) }, true },
	});
	const iggy::animation::SpriteClip2D *clip = frameSet.frameSet.findClip(ClipId);
	const iggy::animation::SpriteAnimationSampleResult sample = iggy::animation::SpriteAnimationSampler2D {}.sample({ { ClipId, 0.25F, 1.0F, true, false }, clip });
	const iggy::SpriteRenderCommandConfig2D config { MaterialId, { 3.0F, 4.0F }, { 2.0F, 2.0F }, { 0.5F, 1.0F }, 6 };

	const iggy::SpriteRenderCommandBuildResult2D result = iggy::SpriteRenderCommands2D {}.build(sample, config);

	Expect(frameSet.built, "manual composition should build frame set");
	Expect(sample.status == iggy::animation::SpriteAnimationSampleStatus::Sampled && sample.frameIndex == 1, "manual composition should sample second frame");
	Expect(result.emitted && result.commands.commands.size() == 1, "manual composition should emit one render command");
	if (result.commands.commands.size() == 1) {
		ExpectCommand(result.commands.commands[0], { { 2.0F, 2.0F }, { 4.0F, 4.0F } }, MaterialId, 6, 0, "manual composition should produce generic render command");
		ExpectTexturePayload(result.commands.commands[0], iggy::ResourceId { "texture:hero_run" }, Rect(16.0F, 0.0F, 16.0F, 16.0F), "manual composition should carry sampled second frame texture payload");
	}
}

} // namespace

int main()
{
	TestClipMissingSampleDoesNotEmit();
	TestNoFramesSampleDoesNotEmit();
	TestSampledNullFrameDoesNotEmit();
	TestSampledFrameEmitsOneQuad();
	TestBoundsUsePositionSizeAndAnchor();
	TestNonDefaultAnchorChangesBounds();
	TestNegativeSizeIsNormalized();
	TestZeroSizeProducesDegenerateBounds();
	TestMaterialIdAndLayerAreCopied();
	TestEmptyMaterialIdIsPreserved();
	TestEmptyFrameTextureIdIsPreserved();
	TestSourceRectIsPreservedExactly();
	TestSampleFrameIsNotModified();
	TestManualAnimationToRenderCommandComposition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
