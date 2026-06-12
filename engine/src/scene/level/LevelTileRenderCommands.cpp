#include "scene/level/LevelTileRenderCommands.hpp"

namespace iggy {

render::RenderCommandList2D LevelTileRenderCommands::build(const LevelTileDrawListResult &drawList, const LevelTileRenderCommandConfig &config) const
{
	render::RenderCommandList2D commands;
	render::RenderCommandListBuilder2D builder;
	for (const LevelTileDrawItem &item : drawList.items)
		builder.addQuad(commands, item.worldBounds, item.walkable ? config.materials.walkableMaterialId : config.materials.blockedMaterialId, config.layer);
	return commands;
}

} // namespace iggy
