#pragma once

#include "modules/npc_ai/NpcAgentRenderCommands.hpp"
#include "scene/camera/CameraState.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "scene/level/LevelTileRenderCommands.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct LevelRenderFrame2DConfig {
	CameraViewConfig cameraView;
	LevelTileRenderCommandConfig tileCommands;
	npc_ai::NpcAgentRenderCommandConfig npcCommands;
	bool includeNpcCommands = true;
};

struct LevelRenderFrame2DResult {
	CameraViewResult cameraView;
	LevelVisibleTilesResult visibleTiles;
	LevelTileDrawListResult tileDrawList;
	render::RenderCommandList2D commands;
};

class LevelRenderFrame2D {
public:
	[[nodiscard]] LevelRenderFrame2DResult build(const LevelRuntimeState &state, CameraState presentationCamera, const LevelRenderFrame2DConfig &config) const;
};

} // namespace iggy
