#pragma once

#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/SurfaceExtrude.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorSurfaceExtrudeCache {
  iggy3d::creative::CreativeSurfaceExtrudePlan plan{};
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t refreshCount = 0U;
  iggy3d::creative::CreativeGridCoord3 seedCell{};
  iggy3d::creative::CreativeGridCoord3 outward{};
  iggy3d::creative::CreativeSurfaceExtrudeKind kind =
      iggy3d::creative::CreativeSurfaceExtrudeKind::Extrude;
  iggy3d::creative::CreativeSurfaceExtrudeDepth depth =
      iggy3d::creative::CreativeSurfaceExtrudeDepth::OneCell;
  iggy3d::creative::CreativeConnectedFillLimit affectedCellLimit =
      iggy3d::creative::CreativeConnectedFillLimit::Cells256;
  bool valid = false;
};

struct CreativeEditorSurfaceExtrudeReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  iggy3d::creative::CreativeSurfaceExtrudeKind kind =
      iggy3d::creative::CreativeSurfaceExtrudeKind::Extrude;
  iggy3d::creative::CreativeSurfaceExtrudeStatus planStatus =
      iggy3d::creative::CreativeSurfaceExtrudeStatus::NotRequested;
  iggy3d::creative::CreativeVoxelMutationStatus mutationStatus =
      iggy3d::creative::CreativeVoxelMutationStatus::NotRequested;
  std::uint16_t surfaceCellCount = 0U;
  std::uint16_t plannedCellCount = 0U;
  std::uint64_t changedCellCount = 0U;
  std::string_view reasonCode = "creative_surface_extrude_not_requested";
};

[[nodiscard]] bool creativeSurfaceFaceOffset(
    iggy3d::creative::CreativeVec3 faceNormal,
    iggy3d::creative::CreativeGridCoord3& output) noexcept;

[[nodiscard]] const iggy3d::creative::CreativeSurfaceExtrudePlan&
resolveCreativeEditorSurfaceExtrudePlan(
    CreativeEditorSurfaceExtrudeCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    iggy3d::creative::CreativeGridCoord3 seedCell,
    iggy3d::creative::CreativeGridCoord3 outward,
    iggy3d::creative::CreativeSurfaceExtrudeKind kind,
    iggy3d::creative::CreativeSurfaceExtrudeDepth depth,
    iggy3d::creative::CreativeConnectedFillLimit affectedCellLimit) noexcept;

void invalidateCreativeEditorSurfaceExtrudeCache(
    CreativeEditorSurfaceExtrudeCache& cache) noexcept;

[[nodiscard]] CreativeEditorSurfaceExtrudeReceipt
applyCreativeEditorSurfaceExtrudeWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::creative::CreativeSurfaceExtrudeKind kind,
    std::string_view source);

}  // namespace iggy3d_creative_app
