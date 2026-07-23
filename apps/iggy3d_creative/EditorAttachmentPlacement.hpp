#pragma once

#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorWorldTarget;
struct CreativePlacementClearanceCache;

enum class CreativeAssetAlignmentStatus : std::uint8_t {
  Ready,
  InvalidMode,
  InvalidTarget,
  FloorRequired,
  WallRequired,
  SurfaceRequired,
};

struct CreativeAssetAlignmentPlan {
  iggy3d::creative::CreativeGridTarget target{};
  iggy3d::creative::CreativePlacementCompatibilityResult compatibility{};
  iggy3d::creative::CreativeAssetAlignmentMode mode =
      iggy3d::creative::CreativeAssetAlignmentMode::Grid;
  CreativeAssetAlignmentStatus status =
      CreativeAssetAlignmentStatus::InvalidTarget;
  bool orientToSurfaceNormal = false;
  bool valid = false;
};

struct CreativeEditorPlacementResolution {
  CreativeBrushPlacementAdmission admission;
  CreativeAssetAlignmentPlan alignment;
  iggy3d::creative::CreativeAttachmentSnapResult attachment;
  bool socketTargeted = false;
};

enum class CreativeEditorObjectReattachmentStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  SourceObjectMissing,
  SourceAssetMissing,
  TargetObjectMissing,
  TargetInsideSourceHierarchy,
  HierarchyTransformRejected,
  SnapRejected,
  ClearanceBlocked,
  Ready,
  MutationRejected,
  Applied,
};

struct CreativeEditorObjectReattachmentPlan {
  CreativeEditorObjectReattachmentStatus status =
      CreativeEditorObjectReattachmentStatus::NotRequested;
  iggy3d::creative::CreativeAttachmentSnapResult snap;
  iggy3d::creative::CreativePlacementClearanceResult clearance;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId targetObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t hierarchyObjectCount = 0U;
  bool accepted = false;
};

struct CreativeEditorObjectReattachmentReceipt {
  CreativeEditorObjectReattachmentStatus status =
      CreativeEditorObjectReattachmentStatus::NotRequested;
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId targetObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  bool accepted = false;
  bool changed = false;
};

[[nodiscard]] CreativeAssetAlignmentPlan resolveCreativeAssetAlignment(
    const iggy3d::creative::CreativeGridTarget& target,
    iggy3d::creative::CreativeAssetAlignmentMode mode) noexcept;
[[nodiscard]] CreativeBrushPlacementAdmissionStatus
creativeAssetAlignmentAdmissionStatus(
    CreativeAssetAlignmentStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeEditorObjectReattachmentStatus status) noexcept;

[[nodiscard]] CreativeEditorObjectReattachmentPlan
planCreativeEditorObjectReattachment(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& assetCatalog,
    iggy3d::creative::CreativeObjectId sourceObjectId,
    iggy3d::creative::CreativeObjectId targetObjectId,
    iggy3d::creative::CreativeVec3 aimPoint,
    iggy3d::creative::CreativeAssetAttachmentMode attachmentMode =
        iggy3d::creative::CreativeAssetAttachmentMode::AimSocket,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

[[nodiscard]] CreativeEditorObjectReattachmentReceipt
applyCreativeEditorObjectReattachment(
    iggy3d::creative::CreativeDocument& document,
    const CreativeEditorObjectReattachmentPlan& plan);

[[nodiscard]] CreativeEditorPlacementResolution
resolveCreativeEditorPlacement(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const CreativeEditorWorldTarget& target,
    iggy3d::creative::CreativePlacementYaw placementYaw,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    const CreativePlacementClearanceCache* clearanceCache = nullptr,
    iggy3d::creative::CreativeAssetAlignmentMode alignmentMode =
        iggy3d::creative::CreativeAssetAlignmentMode::Grid,
    iggy3d::creative::CreativeAssetAttachmentMode attachmentMode =
        iggy3d::creative::CreativeAssetAttachmentMode::BestMatch) noexcept;

}  // namespace iggy3d_creative_app
