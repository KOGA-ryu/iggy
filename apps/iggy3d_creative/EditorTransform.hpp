#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

struct CreativeEditorState;

enum class CreativeEditorTransformSource : std::uint8_t {
  Clipboard,
  Selection,
};

enum class CreativeEditorTransformControl : std::uint8_t {
  RotatePositive,
  MirrorX,
  CycleConstraint,
  ToggleMode,
  Confirm,
  Cancel,
  MirrorZ,
  RotateNegative,
  Reset,
  Count,
};

inline constexpr std::size_t kCreativeEditorTransformControlCount =
    static_cast<std::size_t>(CreativeEditorTransformControl::Count);

struct CreativeEditorTransformCommitReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  cr::CreativeSelectionPlacementMode mode =
      cr::CreativeSelectionPlacementMode::Copy;
  cr::CreativeClipboardPasteReceipt copyReceipt{};
  cr::CreativeSelectionPlacementReceipt moveReceipt{};
  std::string reasonCode = "editor_transform_not_requested";
};

struct CreativeEditorSelectionTransformState {
  bool active = false;
  bool aimTargetPositionable = false;
  bool targetPositionable = false;
  bool commitRequested = false;
  bool controlsOpen = false;
  bool moveAvailable = false;
  bool fineNudgeActive = false;
  CreativeEditorTransformSource source =
      CreativeEditorTransformSource::Clipboard;
  cr::CreativeSelectionPlacementMode mode =
      cr::CreativeSelectionPlacementMode::Copy;
  cr::CreativeSelectionPlacementAxis constraint =
      cr::CreativeSelectionPlacementAxis::Free;
  std::size_t selectedControl = 0;
  cr::CreativeVec3 aimTargetAnchor{};
  cr::CreativeVec3 nudgeOffset{};
  double snapStepMeters = 1.0;
  cr::CreativeClipboard sourceClipboard{};
  cr::CreativeSelectionPlacementRequest request{};
  cr::CreativeSelectionPlacementTargetResult targetResolution{};
  cr::CreativeSelectionPlacementNudgeReceipt lastNudge{};
  cr::CreativeSelectionPlacementPlan plan{};
  CreativeEditorTransformCommitReceipt lastCommit{};
};

struct CreativeEditorTransformFrameRequest {
  iggy3d::SdlWindow& window;
  cr::CreativeAppState& appState;
  CreativeEditorState& editor;
  const cr::CreativeInputRouteResult& routedInput;
  float directionX = 0.0F;
  float directionY = 0.0F;
  std::int32_t nudgeWheelSteps = 0;
  bool fineNudge = false;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeEditorTransformFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
};

[[nodiscard]] std::string_view toString(
    CreativeEditorTransformControl control) noexcept;

[[nodiscard]] bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool requestCreativeEditorSelectionTransformCommit(
    CreativeEditorSelectionTransformState& state) noexcept;
[[nodiscard]] bool cancelCreativeEditorSelectionTransformPreview(
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool setCreativeEditorTransformConstraint(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis constraint);
[[nodiscard]] bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine);
[[nodiscard]] bool applyCreativeEditorTransformControl(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control);

[[nodiscard]] CreativeEditorTransformCommitReceipt
processCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source,
    double snapStepMeters = 1.0);

[[nodiscard]] CreativeEditorTransformFrameResult
processCreativeEditorTransformFrame(
    const CreativeEditorTransformFrameRequest& request);

[[nodiscard]] std::size_t appendCreativeEditorSelectionTransformPreview(
    const CreativeEditorSelectionTransformState& state,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

void appendCreativeEditorTransformOverlay(
    const CreativeEditorSelectionTransformState& state,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
