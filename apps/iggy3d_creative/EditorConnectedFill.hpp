#pragma once

#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/ConnectedFill.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorConnectedFillCache {
  iggy3d::creative::CreativeConnectedFillPlan plan{};
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t refreshCount = 0U;
  iggy3d::creative::CreativeGridCoord3 seedCell{};
  iggy3d::creative::CreativeConnectedFillLimit limit =
      iggy3d::creative::CreativeConnectedFillLimit::Cells256;
  bool valid = false;
};

enum class CreativeConnectedFillEditKind : std::uint8_t {
  Paint,
  Erase,
  Count,
};

struct CreativeEditorConnectedFillReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeConnectedFillEditKind editKind =
      CreativeConnectedFillEditKind::Paint;
  iggy3d::creative::CreativeConnectedFillStatus planStatus =
      iggy3d::creative::CreativeConnectedFillStatus::NotRequested;
  iggy3d::creative::CreativeVoxelMutationStatus mutationStatus =
      iggy3d::creative::CreativeVoxelMutationStatus::NotRequested;
  std::uint16_t plannedCellCount = 0U;
  std::uint64_t changedCellCount = 0U;
  std::string_view reasonCode = "creative_connected_fill_not_requested";
};

[[nodiscard]] const iggy3d::creative::CreativeConnectedFillPlan&
resolveCreativeEditorConnectedFillPlan(
    CreativeEditorConnectedFillCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    iggy3d::creative::CreativeGridCoord3 seedCell,
    iggy3d::creative::CreativeConnectedFillLimit limit) noexcept;
void invalidateCreativeEditorConnectedFillCache(
    CreativeEditorConnectedFillCache& cache) noexcept;

[[nodiscard]] CreativeEditorConnectedFillReceipt
applyCreativeEditorConnectedFillWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeConnectedFillEditKind editKind,
    std::string_view source);

}  // namespace iggy3d_creative_app
