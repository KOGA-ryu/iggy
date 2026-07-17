#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "EditorPicking.hpp"
#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/StructuralPlacement.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativePlacementClearanceCache;

struct CreativeEditorStructuralSpanState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeVec3 firstAnchor{};
  bool active = false;
};

struct CreativeEditorStructuralSpanEditState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::uint64_t sourceRevision = 0U;
  iggy3d::creative::CreativeTransform sourceTransform{};
  iggy3d::creative::CreativeBounds sourceBounds{};
  iggy3d::creative::CreativeStructuralSpanEndpoint selectedEndpoint =
      iggy3d::creative::CreativeStructuralSpanEndpoint::Count;
  iggy3d::creative::CreativeStructuralSpanEditPlan preview{};
  iggy3d::creative::CreativeVec3 targetAnchor{};
  bool available = false;
  bool active = false;
  bool targetAvailable = false;
};

struct CreativeEditorStructuralSpanEndpointHandleFrame {
  std::array<PathPointHandleHit, 2U> handles{};
  std::uint8_t count = 0U;

  [[nodiscard]] std::span<const PathPointHandleHit> items() const noexcept {
    return {handles.data(), count};
  }
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
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

void cancelCreativeEditorStructuralSpan(
    CreativeEditorState& editor) noexcept;

void syncCreativeEditorStructuralSpanEditState(
    const iggy3d::creative::CreativeAppState& appState,
    CreativeEditorStructuralSpanEditState& state) noexcept;
void resetCreativeEditorStructuralSpanEdit(
    CreativeEditorStructuralSpanEditState& state) noexcept;
[[nodiscard]] bool cancelCreativeEditorStructuralSpanEdit(
    CreativeEditorStructuralSpanEditState& state) noexcept;
void refreshCreativeEditorStructuralSpanEditPreview(
    const iggy3d::creative::CreativeAppState& appState,
    CreativeEditorStructuralSpanEditState& state,
    bool targetAvailable,
    iggy3d::creative::CreativeVec3 targetAnchor) noexcept;

[[nodiscard]] bool processCreativeEditorStructuralSpanEditInput(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::span<const PathPointHandleHit> endpointHandles,
    float targetPixelX,
    float targetPixelY);

[[nodiscard]] CreativeEditorStructuralSpanEndpointHandleFrame
buildCreativeEditorStructuralSpanEndpointHandles(
    const iggy3d::creative::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept;

[[nodiscard]] CreativeBrushPlacementPlan
creativeEditorStructuralSpanEditPreviewPlan(
    const CreativeEditorStructuralSpanEditState& state) noexcept;

[[nodiscard]] std::size_t appendCreativeEditorStructuralSpanEditWireframe(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorStructuralSpanEditState& state,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
