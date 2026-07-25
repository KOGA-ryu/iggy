#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d::creative {
struct CreativeCatalogState;
}

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutReview(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const iggy3d::creative::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands, bool editingDisabled,
    bool repairDisabled);

}  // namespace iggy3d_creative_app
