#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutEditReceipt {
  bool accepted = false;
  bool changed = false;
  std::string reasonCode = "creative_editor_world_layout_not_requested";
};

struct CreativeEditorSelectionSynchronizationReceipt {
  bool accepted = false;
  bool changed = false;
  bool sourceSelected = false;
  std::size_t selectedObjectCount = 0U;
  cr::CreativeWorldLayoutSourceRef source{};
  std::string_view reasonCode =
      "creative_editor_selection_sync_not_requested";
};

struct CreativeEditorWorldLayoutPreviewReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutStatus status =
      cr::CreativeWorldLayoutStatus::NotRequested;
  std::string reasonCode = "creative_editor_world_layout_preview_not_requested";
};

struct CreativeEditorWorldLayoutApplyReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode = "creative_editor_world_layout_apply_not_requested";
};

struct CreativeEditorWorldLayoutAdoptionReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutTable table = cr::CreativeWorldLayoutTable::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode =
      "creative_editor_world_layout_adoption_not_requested";
};

// One read-only truth shared by Plan, Elevation, and the 3D viewport. The
// pointed-to layout is owned by state and remains valid until state mutates.
struct CreativeEditorWorldLayoutInspection {
  const cr::CreativeWorldLayout* source = nullptr;
  CreativeEditorWorldLayoutInspectionSourceKind sourceKind =
      CreativeEditorWorldLayoutInspectionSourceKind::Authored;
  CreativeEditorWorldLayoutPreviewValidity previewValidity =
      CreativeEditorWorldLayoutPreviewValidity::None;
  cr::CreativeWorldLayoutSourceRef authoredSource;
  std::uint64_t contentRevision = 0U;
  bool volatileSource = false;
  std::string_view reasonCode =
      "creative_editor_world_layout_inspection_not_requested";
};

}  // namespace iggy3d_creative_app
