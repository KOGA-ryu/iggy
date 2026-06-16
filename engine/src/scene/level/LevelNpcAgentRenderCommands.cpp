#include "scene/level/LevelNpcAgentRenderCommands.hpp"

#include <cmath>

#include "core/math/Aabb2.hpp"

namespace {

iggy::Aabb2 BoundsForAgent(iggy::Vec2 position, iggy::Vec2 size, iggy::Vec2 anchor)
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

render::RenderCommandList2D LevelNpcAgentRenderCommands::build(const std::vector<npc_ai::NpcAgentEntry> &agents, const LevelNpcAgentRenderCommandConfig &config) const
{
	render::RenderCommandList2D commands;
	const render::RenderCommandListBuilder2D builder;
	for (const npc_ai::NpcAgentEntry &agent : agents)
		builder.addQuad(commands, BoundsForAgent(agent.state.position, config.size, config.anchor), config.materialId, config.layer);
	return commands;
}

} // namespace iggy
