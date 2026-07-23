#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorMapValidationDiagnostics.hpp"

namespace iggy3d_creative_app {

void buildCreativeEditorMapValidationPanel(
    CreativeEditorMapValidationCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands);

void buildCreativeEditorMapValidationPassStatus(
    const CreativeEditorMapValidationCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog);

}  // namespace iggy3d_creative_app
