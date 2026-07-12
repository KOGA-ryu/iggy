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

enum class CreativeEditorTerrainPaintRegionPhase : std::uint8_t {
  Empty,
  FirstCorner,
  Complete,
};

struct CreativeEditorTerrainPaintPreviewEdge {
  iggy3d::Vec3 start{};
  iggy3d::Vec3 end{};
};

struct CreativeEditorTerrainPaintPreviewCache {
  bool valid = false;
  bool targetValid = false;
  std::uint64_t documentId = 0U;
  std::uint64_t terrainRevision = 0U;
  std::uint64_t materialRevision = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeVec3 gridOrigin{};
  double gridCellSizeMeters = 0.0;
  iggy3d::creative::CreativeTerrainPaintMode mode =
      iggy3d::creative::CreativeTerrainPaintMode::Brush;
  iggy3d::creative::CreativeTerrainMaterial material =
      iggy3d::creative::CreativeTerrainMaterial::Grass;
  iggy3d::creative::CreativeTerrainPaintSource source =
      iggy3d::creative::CreativeTerrainPaintSource::Any;
  iggy3d::creative::CreativeTerrainPaintRadius radius =
      iggy3d::creative::CreativeTerrainPaintRadius::OneCell;
  CreativeEditorTerrainPaintRegionPhase regionPhase =
      CreativeEditorTerrainPaintRegionPhase::Empty;
  iggy3d::creative::CreativeTerrainCoord2 target{};
  iggy3d::creative::CreativeTerrainCoord2 firstCorner{};
  iggy3d::creative::CreativeTerrainCoord2 secondCorner{};
  iggy3d::creative::CreativeTerrainPaintPlan plan{};
  std::vector<CreativeEditorTerrainPaintPreviewEdge> edges;
};

struct CreativeEditorTerrainPaintState {
  iggy3d::creative::CreativeMaterialRepeatState repeat{};
  iggy3d::creative::CreativeDocumentHistoryTransaction transaction{};
  iggy3d::creative::CreativeTerrainPaintPlan lastPlan{};
  iggy3d::creative::CreativeTerrainMaterialMutationReceipt lastMutation{};
  CreativeEditorTerrainPaintPreviewCache preview{};
  iggy3d::creative::CreativeTerrainPaintMode activeMode =
      iggy3d::creative::CreativeTerrainPaintMode::Brush;
  CreativeEditorTerrainPaintRegionPhase regionPhase =
      CreativeEditorTerrainPaintRegionPhase::Empty;
  iggy3d::creative::CreativeTerrainCoord2 firstCorner{};
  iggy3d::creative::CreativeTerrainCoord2 secondCorner{};
  std::uint16_t acceptedMutationCount = 0U;
  bool modeInitialized = false;
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
