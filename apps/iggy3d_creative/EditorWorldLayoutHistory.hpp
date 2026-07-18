#pragma once

#include <string_view>

#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/history/History.hpp"

namespace iggy3d_creative_app {

inline constexpr std::string_view kCreativeEditorWorldLayoutHistorySidecarType =
    "iggy3d.editor.world_layout";
inline constexpr std::uint32_t
    kCreativeEditorWorldLayoutHistorySidecarVersion = 1U;

[[nodiscard]] CreativeEditorWorldLayoutSnapshot
captureCreativeEditorWorldLayoutSnapshot(
    const CreativeEditorWorldLayoutState& state);

[[nodiscard]] bool encodeCreativeEditorWorldLayoutHistorySidecar(
    const CreativeEditorWorldLayoutSnapshot& snapshot,
    cr::CreativeHistorySidecar& output);

[[nodiscard]] bool decodeCreativeEditorWorldLayoutHistorySidecar(
    const cr::CreativeHistorySidecar& sidecar,
    CreativeEditorWorldLayoutSnapshot& output);

void installCreativeEditorWorldLayoutSnapshot(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutSnapshot snapshot);

[[nodiscard]] bool creativeEditorWorldLayoutSourceUndoAvailable(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceRedoAvailable(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] std::uint64_t creativeEditorWorldLayoutSourceUndoDepth(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] std::uint64_t creativeEditorWorldLayoutSourceRedoDepth(
    const CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] cr::CreativeWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutPlanWithHistory(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    const cr::CreativeWorldLayoutPlan& plan,
    CreativeEditorWorldLayoutSnapshot committedSnapshot,
    std::string_view source);

[[nodiscard]] cr::CreativeHistoryApplyReceipt
applyCreativeEditorWorldLayoutHistory(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    cr::CreativeHistoryDirection direction);

}  // namespace iggy3d_creative_app
