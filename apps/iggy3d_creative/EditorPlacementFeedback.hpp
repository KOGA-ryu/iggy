#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/spatial/PlacementClearance.hpp"

namespace iggy3d::creative {

class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeBrushPlacementAdmission;
struct CreativeBrushPlacementMutationReceipt;
struct CreativeEditorInteractionState;
struct CreativeEditorOverlayFrame;
struct CreativeEditorOverlayFrameRequest;

enum class CreativeEditorPlacementFeedbackStatus : std::uint8_t {
  None,
  Placed,
  Rejected,
};

enum class CreativeEditorPlacementRejectionReason : std::uint8_t {
  None,
  ActionRejected,
  InvalidTarget,
  FloorRequired,
  WallRequired,
  SurfaceRequired,
  UnsupportedBrush,
  InvalidGeometry,
  UnsupportedPolicy,
  FaceDisallowed,
  TargetIncompatible,
  AttachmentUnavailable,
  AttachmentIncompatible,
  AttachmentOccupied,
  ClearanceBlocked,
  Occupied,
  InvalidPlan,
  ObjectRejected,
  VoxelRejected,
  SemanticSourceOwned,
  ExternalReference,
  CapacityReached,
  HistoryUnavailable,
};

inline constexpr std::uint64_t kCreativeEditorPlacementFeedbackFrames = 36U;
inline constexpr std::size_t kCreativeEditorPlacementFeedbackTextCapacity =
    48U;
static_assert(kCreativeEditorPlacementFeedbackTextCapacity <=
              static_cast<std::size_t>(UINT8_MAX));
inline constexpr std::uint32_t
    kCreativeEditorPlacementInvalidTargetSegmentKind = 11U;
inline constexpr std::uint32_t
    kCreativeEditorPlacementBlockerSegmentKind = 12U;

struct CreativeEditorPlacementFeedback {
  CreativeEditorPlacementFeedbackStatus status =
      CreativeEditorPlacementFeedbackStatus::None;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::uint64_t frameIndex = 0;
  bool voxelPlaced = false;
  iggy3d::creative::CreativeGridCoord3 voxelCell{};
  iggy3d::creative::CreativeBounds voxelBounds{};
  iggy3d::creative::CreativePlacementClearanceResult clearance{};
  CreativeEditorPlacementRejectionReason rejectionReason =
      CreativeEditorPlacementRejectionReason::None;
};

struct CreativeEditorPlacementFeedbackColor {
  float r = 0.95F;
  float g = 0.95F;
  float b = 0.95F;
  float a = 1.0F;
};

struct CreativeEditorPlacementFeedbackText {
  std::array<char, kCreativeEditorPlacementFeedbackTextCapacity> bytes{};
  std::uint8_t length = 0U;

  [[nodiscard]] std::string_view view() const noexcept {
    return {bytes.data(), length};
  }

  [[nodiscard]] bool empty() const noexcept { return length == 0U; }
};

struct CreativeEditorPlacementFeedbackViewModel {
  CreativeEditorPlacementFeedbackStatus status =
      CreativeEditorPlacementFeedbackStatus::None;
  CreativeEditorPlacementFeedbackColor color{};
  CreativeEditorPlacementFeedbackText label{};
  bool visible = false;
};

static_assert(std::is_trivially_copyable_v<
              CreativeEditorPlacementFeedbackViewModel>);
static_assert(std::is_standard_layout_v<
              CreativeEditorPlacementFeedbackViewModel>);

struct CreativeEditorPlacementVisualizationReceipt {
  std::array<iggy3d::creative::CreativeVec3, 8U> attemptedCorners{};
  iggy3d::creative::CreativePlacementClearanceResult clearance{};
  iggy3d::creative::CreativeTransform attemptedTransform{};
  std::uint8_t attemptedCornerCount = 0U;
  bool targetAvailable = false;
  bool attemptedTransformAvailable = false;
};

static_assert(std::is_trivially_copyable_v<
              CreativeEditorPlacementVisualizationReceipt>);
static_assert(std::is_standard_layout_v<
              CreativeEditorPlacementVisualizationReceipt>);

[[nodiscard]] constexpr bool creativeEditorPlacementFeedbackVisible(
    const CreativeEditorPlacementFeedback& feedback,
    std::uint64_t frameIndex) noexcept {
  return feedback.status != CreativeEditorPlacementFeedbackStatus::None &&
         frameIndex >= feedback.frameIndex &&
         frameIndex - feedback.frameIndex <
             kCreativeEditorPlacementFeedbackFrames;
}

void clearCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction) noexcept;
void setCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    CreativeEditorPlacementFeedbackStatus status,
    std::uint64_t frameIndex,
    iggy3d::creative::CreativeObjectKind objectKind =
        iggy3d::creative::CreativeObjectKind::Unknown,
    iggy3d::creative::CreativeObjectId objectId =
        iggy3d::creative::kInvalidObjectId) noexcept;
void setCreativeEditorPlacementRejectionFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    iggy3d::creative::CreativeObjectKind objectKind,
    const iggy3d::creative::CreativePlacementClearanceResult& clearance =
        {},
    CreativeEditorPlacementRejectionReason reason =
        CreativeEditorPlacementRejectionReason::ActionRejected) noexcept;
void setCreativeEditorVoxelPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    iggy3d::creative::CreativeObjectKind objectKind,
    iggy3d::creative::CreativeGridCoord3 voxelCell,
    iggy3d::creative::CreativeBounds voxelBounds) noexcept;
void setCreativeEditorPlacementAdmissionRejectionFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    const CreativeBrushPlacementAdmission& admission) noexcept;
void setCreativeEditorPlacementMutationFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    const CreativeBrushPlacementMutationReceipt& receipt) noexcept;

[[nodiscard]] CreativeEditorPlacementFeedbackViewModel
creativeEditorPlacementFeedbackViewModel(
    const CreativeEditorPlacementFeedback& feedback,
    std::uint64_t frameIndex,
    const iggy3d::creative::CreativeDocument* document = nullptr);

void appendCreativeEditorPlacementClearanceWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

}  // namespace iggy3d_creative_app
