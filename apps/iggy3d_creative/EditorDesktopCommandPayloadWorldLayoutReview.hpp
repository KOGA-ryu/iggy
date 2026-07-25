#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopWorldLayoutOpeningSettingsPayload {
  std::size_t openingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningSettings settings;
};

struct CreativeDesktopGeneratedOpeningSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutOpeningSettings settings;
};

struct CreativeDesktopWorldLayoutOpeningInsertPayload {
  std::size_t openingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningInsertOperation operation =
      CreativeEditorWorldLayoutOpeningInsertOperation::FitAssetToOpening;
  std::string assetId;
  iggy3d::creative::CreativeVec3 assetScale{1.0, 1.0, 1.0};
};

enum class CreativeDesktopWorldLayoutAssetRepairOperation : std::uint8_t {
  RefreshBounds,
  ReplaceAsset,
  UseProceduralInsert,
  Count,
};

struct CreativeDesktopWorldLayoutAssetRepairPayload {
  CreativeDesktopWorldLayoutAssetRepairOperation operation =
      CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string expectedAssetId;
  std::string replacementAssetId;
};

struct CreativeDesktopWorldLayoutBuildingRepairPayload {
  iggy3d::creative::CreativeWorldLayoutBuildingUsabilityIssue issue;
  std::string stableKey;
};

struct CreativeDesktopWorldLayoutOpeningManipulationPayload {
  CreativeEditorWorldLayoutOpeningManipulationPhase phase =
      CreativeEditorWorldLayoutOpeningManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutConfirmPayload {
  std::vector<iggy3d::creative::CreativeWorldLayoutConflictDecision>
      conflictDecisions;
  std::vector<iggy3d::creative::CreativeWorldLayoutTerrainConflictDecision>
      terrainConflictDecisions;
};

}  // namespace iggy3d_creative_app
