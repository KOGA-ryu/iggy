#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/tools/Pattern.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;
struct CreativeToolSettings;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorPatternState {
  iggy3d::creative::CreativeLinearArrayReceipt lastReceipt;
  iggy3d::creative::CreativeRadialArrayReceipt lastRadialReceipt;
};

[[nodiscard]] iggy3d::creative::CreativeLinearArrayRequest
creativeEditorLinearArrayRequest(
    const iggy3d::creative::CreativeToolSettings& settings,
    double cellSize) noexcept;

[[nodiscard]] iggy3d::creative::CreativeRadialArrayRequest
creativeEditorRadialArrayRequest(
    const iggy3d::creative::CreativeToolSettings& settings,
    iggy3d::creative::CreativeVec3 pivot) noexcept;

[[nodiscard]] iggy3d::creative::CreativeLinearArrayReceipt
applyCreativeEditorLinearArrayWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const iggy3d::creative::CreativeToolSettings& settings,
    double cellSize,
    std::string_view source);

[[nodiscard]] iggy3d::creative::CreativeRadialArrayReceipt
applyCreativeEditorRadialArrayWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const iggy3d::creative::CreativeToolSettings& settings,
    iggy3d::creative::CreativeVec3 pivot,
    std::string_view source);

[[nodiscard]] bool applyCreativeEditorArrayWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const iggy3d::creative::CreativeToolSettings& settings,
    double cellSize,
    bool pivotValid,
    iggy3d::creative::CreativeVec3 pivot,
    std::string_view source);

[[nodiscard]] std::size_t appendCreativeEditorLinearArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

[[nodiscard]] std::size_t appendCreativeEditorRadialArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

[[nodiscard]] std::size_t appendCreativeEditorArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
