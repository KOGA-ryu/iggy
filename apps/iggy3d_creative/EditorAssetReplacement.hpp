#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "content/assets/StaticMeshAsset.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

inline constexpr std::size_t kCreativeAssetReplacementCapacity = 256U;

enum class CreativeAssetReplacementStatus : std::uint8_t {
  NotRequested,
  Ready,
  InvalidDocument,
  InvalidSelection,
  CapacityExceeded,
  InvalidTarget,
  UnsupportedObject,
  MissingSourceAsset,
  LockedObject,
  CustomBounds,
  NoChange,
  StaleDocument,
  MutationRejected,
  Applied,
  Cancelled,
};

struct CreativeAssetReplacementPlan {
  bool accepted = false;
  CreativeAssetReplacementStatus status =
      CreativeAssetReplacementStatus::NotRequested;
  cr::CreativeDocumentId sourceDocumentId = cr::kInvalidDocumentId;
  std::uint64_t sourceRevision = 0;
  cr::CreativeObjectKind targetObjectKind = cr::CreativeObjectKind::Unknown;
  std::string targetAssetId;
  std::vector<cr::CreativeObjectId> objectIds;
  std::vector<cr::CreativeMutationRequest> mutations;
  std::string_view reasonCode = "creative_asset_replace_not_requested";
};

struct CreativeEditorAssetReplacementState {
  bool active = false;
  CreativeAssetReplacementPlan plan;
  cr::CreativeDocument previewDocument;
  CreativeAssetReplacementStatus lastStatus =
      CreativeAssetReplacementStatus::NotRequested;
  std::string reasonCode = "creative_asset_replace_not_requested";
};

struct CreativeAssetReplacementBeginReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeAssetReplacementStatus status =
      CreativeAssetReplacementStatus::NotRequested;
  std::size_t objectCount = 0;
  std::string_view reasonCode = "creative_asset_replace_not_requested";
};

struct CreativeAssetReplacementCommitReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeAssetReplacementStatus status =
      CreativeAssetReplacementStatus::NotRequested;
  std::size_t objectCount = 0;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  cr::CreativeHistoryRecordReceipt historyReceipt;
  std::string_view reasonCode = "creative_asset_replace_not_requested";
};

struct CreativeEditorAssetReplacementFrameRequest {
  cr::CreativeAppState& appState;
  CreativeEditorAssetReplacementState& state;
  const cr::CreativeInputRouteResult& routedInput;
};

struct CreativeEditorAssetReplacementFrameResult {
  bool blockWorldActions = false;
  bool finished = false;
  CreativeAssetReplacementCommitReceipt commitReceipt;
};

[[nodiscard]] std::string_view toString(
    CreativeAssetReplacementStatus status) noexcept;

[[nodiscard]] CreativeAssetReplacementPlan planCreativeAssetReplacement(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> selectedObjectIds,
    const iggy3d::StaticMeshAssetCatalog& catalog,
    cr::CreativeObjectKind targetObjectKind,
    std::string_view targetAssetId);

[[nodiscard]] CreativeAssetReplacementBeginReceipt
beginCreativeEditorAssetReplacement(
    const cr::CreativeAppState& appState,
    const iggy3d::StaticMeshAssetCatalog& catalog,
    cr::CreativeObjectKind targetObjectKind,
    std::string_view targetAssetId,
    CreativeEditorAssetReplacementState& state);

[[nodiscard]] CreativeAssetReplacementCommitReceipt
commitCreativeEditorAssetReplacement(
    cr::CreativeAppState& appState,
    CreativeEditorAssetReplacementState& state);

[[nodiscard]] bool cancelCreativeEditorAssetReplacement(
    CreativeEditorAssetReplacementState& state,
    std::string_view reasonCode = "creative_asset_replace_cancelled") noexcept;

[[nodiscard]] const cr::CreativeDocument&
creativeEditorAssetReplacementRenderDocument(
    const CreativeEditorAssetReplacementState& state,
    const cr::CreativeDocument& liveDocument) noexcept;

[[nodiscard]] CreativeEditorAssetReplacementFrameResult
processCreativeEditorAssetReplacementFrame(
    const CreativeEditorAssetReplacementFrameRequest& request);

[[nodiscard]] std::size_t appendCreativeEditorAssetReplacementWireframes(
    const CreativeEditorAssetReplacementState& state,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

void appendCreativeEditorAssetReplacementOverlay(
    const CreativeEditorAssetReplacementState& state,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
