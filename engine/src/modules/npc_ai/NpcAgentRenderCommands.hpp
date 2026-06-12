#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy::npc_ai {

struct NpcAgentRenderCommandConfig {
	ResourceId materialId;
	Vec2 size { 1.0F, 1.0F };
	Vec2 anchor { 0.5F, 0.5F };
	int layer = 0;
};

class NpcAgentRenderCommands {
public:
	[[nodiscard]] render::RenderCommandList2D build(const std::vector<NpcAgentEntry> &agents, const NpcAgentRenderCommandConfig &config) const;
};

} // namespace iggy::npc_ai
