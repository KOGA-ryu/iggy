#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct LevelNpcAgentRenderCommandConfig {
	ResourceId materialId;
	Vec2 size { 1.0F, 1.0F };
	Vec2 anchor { 0.5F, 0.5F };
	int layer = 0;
};

class LevelNpcAgentRenderCommands {
public:
	[[nodiscard]] render::RenderCommandList2D build(const std::vector<npc_ai::NpcAgentEntry> &agents, const LevelNpcAgentRenderCommandConfig &config) const;
};

} // namespace iggy
