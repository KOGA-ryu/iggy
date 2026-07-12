#pragma once

#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/TerrainPaint.hpp"
#include "render/FrameInput.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {
struct CreativeAppState;
class CreativeDocument;
}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorTerrainPaintState {
  iggy3d::creative::CreativeMaterialRepeatState repeat{};
  iggy3d::creative::CreativeDocumentHistoryTransaction transaction{};
  iggy3d::creative::CreativeTerrainPaintPlan lastPlan{};
  iggy3d::creative::CreativeTerrainMaterialMutationReceipt lastMutation{};
  std::uint16_t acceptedMutationCount = 0U;
};

void processCreativeEditorTerrainPaintFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);
void finalizeCreativeEditorTerrainPaintStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);
[[nodiscard]] bool sampleCreativeEditorTerrainMaterial(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
void appendCreativeEditorTerrainPaintOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines);

}  // namespace iggy3d_creative_app
