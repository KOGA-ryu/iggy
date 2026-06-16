#include "scene/level/LevelRenderFrame2D.hpp"

#include "servers/render/RenderCommandList2DComposer.hpp"

namespace iggy {

LevelRenderFrame2DResult LevelRenderFrame2D::build(const LevelRuntimeState &state, CameraState presentationCamera, const LevelRenderFrame2DConfig &config) const
{
	LevelRenderFrame2DResult result;
	result.cameraView = CameraView {}.visibleWorldBounds(presentationCamera, config.cameraView);

	const render::RenderCommandList2DComposer composer;
	if (config.useTileChunkCache && config.tileChunkCache != nullptr) {
		result.visibleTileChunks = LevelTileRenderChunkVisibility {}.query(*config.tileChunkCache, result.cameraView.bounds);
		result.tileChunkCommands = LevelTileRenderChunkCommands {}.build(*config.tileChunkCache, result.visibleTileChunks);
		result.usedTileChunkCache = true;
		composer.append(result.commands, result.tileChunkCommands.commands);
	} else {
		result.visibleTiles = LevelVisibleTiles {}.query(state.map, result.cameraView.bounds);
		result.tileDrawList = LevelTileDrawList {}.build(state.map, result.visibleTiles.tiles);

		const render::RenderCommandList2D tileCommands = LevelTileRenderCommands {}.build(result.tileDrawList, config.tileCommands);
		composer.append(result.commands, tileCommands);
	}

	if (config.includeNpcCommands) {
		const render::RenderCommandList2D npcCommands = LevelNpcAgentRenderCommands {}.build(state.npcAgents, config.npcCommands);
		composer.append(result.commands, npcCommands);
	}

	return result;
}

} // namespace iggy
