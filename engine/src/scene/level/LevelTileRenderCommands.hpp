#pragma once

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct LevelTileRenderMaterials {
	ResourceId walkableMaterialId;
	ResourceId blockedMaterialId;
};

struct LevelTileRenderCommandConfig {
	LevelTileRenderMaterials materials;
	int layer = 0;
};

class LevelTileRenderCommands {
public:
	[[nodiscard]] render::RenderCommandList2D build(const LevelTileDrawListResult &drawList, const LevelTileRenderCommandConfig &config) const;
};

} // namespace iggy
