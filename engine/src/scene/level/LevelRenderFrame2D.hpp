#pragma once

#include "scene/camera/CameraState.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelNpcAgentRenderCommands.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/level/LevelTileRenderChunkCommands.hpp"
#include "scene/level/LevelTileRenderChunkVisibility.hpp"
#include "scene/level/LevelTileRenderCommands.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct LevelRenderFrame2DConfig {
	CameraViewConfig cameraView;
	LevelTileRenderCommandConfig tileCommands;
	LevelNpcAgentRenderCommandConfig npcCommands;
	bool includeNpcCommands = true;
	bool useTileChunkCache = false;
	const LevelTileRenderChunkCache *tileChunkCache = nullptr;
};

struct LevelRenderFrame2DResult {
	CameraViewResult cameraView;
	LevelVisibleTilesResult visibleTiles;
	LevelTileDrawListResult tileDrawList;
	LevelTileRenderChunkVisibilityResult visibleTileChunks;
	LevelTileRenderChunkCommandResult tileChunkCommands;
	bool usedTileChunkCache = false;
	render::RenderCommandList2D commands;
};

class LevelRenderFrame2D {
public:
	[[nodiscard]] LevelRenderFrame2DResult build(const LevelRuntimeState &state, CameraState presentationCamera, const LevelRenderFrame2DConfig &config) const;
};

} // namespace iggy
