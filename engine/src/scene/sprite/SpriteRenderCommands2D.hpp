#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/animation/SpriteAnimationSampler2D.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct SpriteRenderCommandConfig2D {
	ResourceId materialId;
	Vec2 position;
	Vec2 size { 1.0F, 1.0F };
	Vec2 anchor { 0.5F, 0.5F };
	int layer = 0;
};

struct SpriteRenderCommandBuildResult2D {
	render::RenderCommandList2D commands;
	bool emitted = false;
};

class SpriteRenderCommands2D {
public:
	[[nodiscard]] SpriteRenderCommandBuildResult2D build(const animation::SpriteAnimationSampleResult &sample, const SpriteRenderCommandConfig2D &config) const;
};

} // namespace iggy
