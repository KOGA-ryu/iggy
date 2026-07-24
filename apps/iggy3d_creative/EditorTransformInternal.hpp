#pragma once

#include "EditorTransformState.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativePlacementClearanceCache;

namespace detail {

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept;
[[nodiscard]] bool moveSourceAvailable(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard) noexcept;
[[nodiscard]] const cr::CreativeObject* transformSourceObject(
    const CreativeEditorSelectionTransformState& state,
    cr::CreativeObjectId objectId) noexcept;
[[nodiscard]] bool worldLayoutBuildingRoute(
    const CreativeEditorSelectionTransformState& state) noexcept;
[[nodiscard]] bool patternRecipeRoute(
    const CreativeEditorSelectionTransformState& state) noexcept;
[[nodiscard]] bool terrainOperationRoute(
    const CreativeEditorSelectionTransformState& state) noexcept;
[[nodiscard]] bool planarSourceRoute(
    const CreativeEditorSelectionTransformState& state) noexcept;
void refreshTransformPlan(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state);
void refreshTransformClearance(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    const CreativePlacementClearanceCache* cache);
void refreshResolvedTarget(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state);
[[nodiscard]] CreativeEditorTransformMode firstAvailableTransformMode(
    const CreativeEditorSelectionTransformState& state) noexcept;
[[nodiscard]] bool quarterTurnDegrees(double degrees) noexcept;

}  // namespace detail
}  // namespace iggy3d_creative_app
