#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopWorldLayoutBuildingBlockoutPayload {
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingSelectionPayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutBuildingManipulationPayload {
  CreativeEditorWorldLayoutBuildingManipulationPhase phase =
      CreativeEditorWorldLayoutBuildingManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutBuildingDuplicatePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
};

struct CreativeDesktopWorldLayoutBuildingTransformPayload {
  CreativeEditorWorldLayoutBuildingTransformPhase phase =
      CreativeEditorWorldLayoutBuildingTransformPhase::Preview;
  iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation operation =
      iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation::
      RotateRight90;
};

struct CreativeDesktopGeneratedBuildingOperationPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutGeneratedBuildingOperation operation =
      CreativeEditorWorldLayoutGeneratedBuildingOperation::Move;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
};

struct CreativeDesktopWorldLayoutBuildingGroundingPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutBuildingGroundingSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingArchitecturePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  iggy3d::creative::CreativeWorldLayoutArchitecturalProfile profile;
};

struct CreativeDesktopWorldLayoutBuildingTemplateCapturePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string label;
};

struct CreativeDesktopWorldLayoutBuildingTemplateSyncPayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutBuildingTemplateRefreshMode mode =
      iggy3d::creative::CreativeWorldLayoutBuildingTemplateRefreshMode::
          SafeInstances;
};

struct CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload {
  std::size_t templateIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload {
  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase =
      CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation operation =
      iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation::
          RotateRight90;
};

}  // namespace iggy3d_creative_app
