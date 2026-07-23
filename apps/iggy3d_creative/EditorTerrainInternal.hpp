#pragma once

#include "EditorTerrain.hpp"
#include "app/iggy3d/creative/tools/TerrainSeed.hpp"

namespace iggy3d_creative_app::terrain_detail {

[[nodiscard]] bool terrainPointerCoord(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2& coord) noexcept;

[[nodiscard]] iggy3d::creative::CreativeBounds terrainRodBounds(
    iggy3d::creative::CreativeGridSettings grid,
    iggy3d::creative::CreativeTerrainControlPoint control,
    double widthCells) noexcept;

[[nodiscard]] iggy3d::creative::CreativeTerrainSeedPlan terrainSeedPlan(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2 center,
    iggy3d::creative::CreativeMaterialStrokeKind kind) noexcept;

}  // namespace iggy3d_creative_app::terrain_detail
