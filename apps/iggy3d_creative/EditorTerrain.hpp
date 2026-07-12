#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/TerrainGrade.hpp"
#include "app/iggy3d/creative/tools/TerrainSculpt.hpp"
#include "render/FrameInput.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct WorldRay;

inline constexpr std::size_t kCreativeTerrainStrokeVisitedCapacity =
    iggy3d::creative::kCreativeTerrainControlCapacity;

struct CreativeTerrainStrokeState {
  iggy3d::creative::CreativeMaterialRepeatState repeat{};
  iggy3d::creative::CreativeDocumentHistoryTransaction transaction{};
  std::array<iggy3d::creative::CreativeTerrainCoord2,
             kCreativeTerrainStrokeVisitedCapacity>
      visited{};
  std::uint16_t visitedCount = 0U;
  std::uint16_t acceptedMutationCount = 0U;
  bool capacityReached = false;
  bool cancelOnly = false;
};

struct CreativeTerrainGradeState {
  bool anchorValid = false;
  iggy3d::creative::CreativeTerrainCoord2 anchorCoord{};
  std::uint16_t anchorHeightCells = 4U;
  std::uint16_t targetHeightCells = 4U;
  std::uint16_t radiusCells = 4U;
};

struct CreativeTerrainSculptStrokeState {
  iggy3d::creative::CreativeMaterialRepeatState repeat{};
  iggy3d::creative::CreativeDocumentHistoryTransaction transaction{};
  std::uint16_t acceptedMutationCount = 0U;
};

struct CreativeTerrainSculptPreviewCache {
  bool valid = false;
  bool renderAccepted = false;
  std::uint64_t documentId = 0U;
  std::uint64_t documentRevision = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeTerrainCoord2 center{};
  iggy3d::creative::CreativeTerrainSculptMode mode =
      iggy3d::creative::CreativeTerrainSculptMode::Flatten;
  iggy3d::creative::CreativeTerrainSculptFalloff falloff =
      iggy3d::creative::CreativeTerrainSculptFalloff::Uniform;
  std::uint16_t radiusCells = 4U;
  std::uint16_t strengthCells = 1U;
  std::uint16_t targetHeightCells = 4U;
  iggy3d::creative::CreativeTerrainSculptPlan plan{};
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch> patches;
};

struct CreativeTerrainSculptState {
  std::uint16_t targetHeightCells = 4U;
  CreativeTerrainSculptStrokeState stroke{};
  CreativeTerrainSculptPreviewCache preview{};
};

struct CreativeEditorTerrainState {
  std::uint16_t heightCells = 4U;
  std::uint16_t radiusCells = 4U;
  std::uint64_t documentId = 0U;
  bool hoverValid = false;
  iggy3d::creative::CreativeTerrainCoord2 hoverCoord{};
  bool selectionValid = false;
  iggy3d::creative::CreativeTerrainCoord2 selectedCoord{};
  iggy3d::creative::CreativeTerrainControlPoint selectedOriginal{};
  iggy3d::creative::CreativeTerrainMutationReceipt lastMutation{};
  CreativeTerrainStrokeState stroke{};
  CreativeTerrainGradeState grade{};
  CreativeTerrainSculptState sculpt{};
};

enum class CreativeEditorTerrainEditKind : std::uint8_t {
  Upsert,
  Remove,
  Sample,
};

struct CreativeEditorTerrainEditReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainEditKind kind =
      CreativeEditorTerrainEditKind::Upsert;
  iggy3d::creative::CreativeTerrainCoord2 coord{};
  iggy3d::creative::CreativeTerrainMutationReceipt mutation{};
  std::string_view reasonCode = "creative_editor_terrain_not_requested";
};

enum class CreativeEditorTerrainGradeAction : std::uint8_t {
  Anchor,
  Apply,
  Cancel,
};

struct CreativeEditorTerrainGradeReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainGradeAction action =
      CreativeEditorTerrainGradeAction::Anchor;
  iggy3d::creative::CreativeTerrainCoord2 targetCoord{};
  iggy3d::creative::CreativeTerrainGradePlan plan{};
  iggy3d::creative::CreativeTerrainMutationReceipt mutation{};
  std::string_view reasonCode = "creative_editor_terrain_grade_not_requested";
};

enum class CreativeEditorTerrainSculptAction : std::uint8_t {
  Apply,
  SampleHeight,
  Cancel,
};

struct CreativeEditorTerrainSculptReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainSculptAction action =
      CreativeEditorTerrainSculptAction::Apply;
  iggy3d::creative::CreativeTerrainCoord2 targetCoord{};
  iggy3d::creative::CreativeTerrainSculptPlan plan{};
  iggy3d::creative::CreativeTerrainMutationReceipt mutation{};
  std::string_view reasonCode = "creative_editor_terrain_sculpt_not_requested";
};

[[nodiscard]] bool resolveCreativeEditorTerrainPointerCoord(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2& target) noexcept;

[[nodiscard]] bool resolveCreativeEditorTerrainGradeTarget(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2& target) noexcept;
[[nodiscard]] iggy3d::creative::CreativeTerrainGradePlan
planCreativeEditorTerrainGrade(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2 target) noexcept;

[[nodiscard]] CreativeEditorTerrainEditReceipt
applyCreativeEditorTerrainEditWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorTerrainEditKind kind,
    std::string_view source);

[[nodiscard]] CreativeEditorTerrainGradeReceipt
beginCreativeEditorTerrainGrade(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainGradeReceipt
applyCreativeEditorTerrainGradeWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainGradeReceipt
cancelCreativeEditorTerrainGrade(CreativeEditorState& editor) noexcept;

[[nodiscard]] iggy3d::creative::CreativeTerrainSculptPlan
planCreativeEditorTerrainSculpt(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2 target) noexcept;
[[nodiscard]] CreativeEditorTerrainSculptReceipt
applyCreativeEditorTerrainSculptWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainSculptReceipt
sampleCreativeEditorTerrainSculptHeight(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainSculptReceipt
cancelCreativeEditorTerrainSculpt(CreativeEditorState& editor) noexcept;

[[nodiscard]] bool processCreativeEditorTerrainQuickEdit(
    CreativeEditorTerrainState& state,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainQuickEditLabel(
    const CreativeEditorTerrainState& state);
[[nodiscard]] bool processCreativeEditorTerrainGradeQuickEdit(
    CreativeTerrainGradeState& state,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainGradeQuickEditLabel(
    const CreativeTerrainGradeState& state);
[[nodiscard]] bool processCreativeEditorTerrainSculptQuickEdit(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainSculptQuickEditLabel(
    const CreativeEditorState& editor);

void clearCreativeEditorTerrainInteraction(
    CreativeEditorTerrainState& state,
    std::uint64_t documentId) noexcept;
void updateCreativeEditorTerrainAim(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    WorldRay ray,
    float occluderDistanceMeters) noexcept;

void processCreativeTerrainStrokeFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);
void finalizeCreativeTerrainStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);
void processCreativeTerrainSculptStrokeFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);
void finalizeCreativeTerrainSculptStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);

[[nodiscard]] bool refreshCreativeEditorTerrainSculptPreview(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor);
void appendCreativeEditorTerrainSculptOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

void appendCreativeEditorTerrainFootprintOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    iggy3d::creative::CreativeGridSettings grid,
    iggy3d::creative::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness);

void appendCreativeEditorTerrainOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
