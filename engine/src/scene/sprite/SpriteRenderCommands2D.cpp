#include "scene/sprite/SpriteRenderCommands2D.hpp"

#include <cmath>

#include "core/math/Aabb2.hpp"

namespace {

iggy::Aabb2 BoundsForSprite(iggy::Vec2 position, iggy::Vec2 size, iggy::Vec2 anchor)
{
	const iggy::Vec2 effectiveSize { std::fabs(size.x), std::fabs(size.y) };
	const iggy::Vec2 min {
		position.x - (effectiveSize.x * anchor.x),
		position.y - (effectiveSize.y * anchor.y),
	};
	return { min, min + effectiveSize };
}

} // namespace

namespace iggy {

SpriteRenderCommandBuildResult2D SpriteRenderCommands2D::build(const animation::SpriteAnimationSampleResult &sample, const SpriteRenderCommandConfig2D &config) const
{
	SpriteRenderCommandBuildResult2D result;
	if (sample.status != animation::SpriteAnimationSampleStatus::Sampled || sample.frame == nullptr)
		return result;

	render::RenderCommandListBuilder2D {}.addTexturedQuad(result.commands, BoundsForSprite(config.position, config.size, config.anchor), config.materialId, sample.frame->textureId, sample.frame->sourceRect, config.layer);
	result.emitted = true;
	return result;
}

} // namespace iggy
