#pragma once

#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/StructuralPlacement.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorStructuralSpanState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeVec3 firstAnchor{};
  bool active = false;
};

[[nodiscard]] bool creativeEditorUsesStructuralSpan(
    const iggy3d::creative::CreativeHotbarEntry& held) noexcept;

[[nodiscard]] CreativeBrushPlacementAdmission
resolveCreativeEditorStructuralSpanPlacement(
    const CreativeEditorStructuralSpanState& state,
    iggy3d::creative::CreativeDocumentId documentId,
    const iggy3d::creative::CreativeHotbarEntry& held,
    const iggy3d::creative::CreativeGridTarget& target) noexcept;

// Returns true when the structural span owner consumed this frame. An
// inactive primary/reject press returns false so ordinary removal still owns
// that input outside a draft.
[[nodiscard]] bool processCreativeEditorStructuralSpanInput(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions);

void cancelCreativeEditorStructuralSpan(
    CreativeEditorState& editor) noexcept;

}  // namespace iggy3d_creative_app
