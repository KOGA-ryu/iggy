#pragma once

#include <string_view>

#include "EditorTransformState.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutState;
struct CreativePlacementClearanceCache;

[[nodiscard]] std::string_view toString(
    CreativeEditorTransformControl control) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeEditorTransformMode mode) noexcept;
[[nodiscard]] cr::CreativeVec3 creativeEditorTransformScaleFactor(
    const CreativeEditorSelectionTransformState& state) noexcept;

[[nodiscard]] bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    CreativeEditorTransformAnchorPolicy anchorPolicy =
        CreativeEditorTransformAnchorPolicy::FollowAim,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] bool beginCreativeEditorTerrainOperationTransformPreview(
    const cr::CreativeAppState& appState,
    cr::CreativeTerrainOperationId operationId,
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
[[nodiscard]] bool setCreativeEditorTransformTargetAnchor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 targetAnchor);
[[nodiscard]] bool setCreativeEditorTransformRotationDegrees(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeAxis3 axis, double degrees);
[[nodiscard]] bool setCreativeEditorTransformScaleFactor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 scaleFactor);
[[nodiscard]] bool setCreativeEditorTransformPlacementMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementMode mode);
[[nodiscard]] bool setCreativeEditorTransformPivot(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformPivot pivot);
[[nodiscard]] bool setCreativeEditorTransformCoordinateSpace(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementCoordinateSpace coordinateSpace);
[[nodiscard]] bool resumeCreativeEditorTransformAim(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state);
[[nodiscard]] CreativeEditorTransformAxisRaySample
sampleCreativeEditorTransformAxisRay(
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection) noexcept;
[[nodiscard]] bool beginCreativeEditorFreeTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 initialAimAnchor);
[[nodiscard]] bool beginCreativeEditorAxisTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis axis,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection);
[[nodiscard]] bool updateCreativeEditorTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection);
[[nodiscard]] bool finishCreativeEditorTransformPointerGesture(
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine);
[[nodiscard]] bool cycleCreativeEditorTransformMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state);
[[nodiscard]] bool adjustCreativeEditorTransformSetting(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction);
[[nodiscard]] bool applyCreativeEditorTransformControl(
    const cr::CreativeAppState& appState,
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
    double snapStepMeters = 1.0,
    CreativeEditorWorldLayoutState* worldLayout = nullptr,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

}  // namespace iggy3d_creative_app
