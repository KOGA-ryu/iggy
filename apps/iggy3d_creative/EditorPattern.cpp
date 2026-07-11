#include "EditorPattern.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

cr::CreativeLinearArrayRequest creativeEditorLinearArrayRequest(
    const cr::CreativeToolSettings& settings,
    double cellSize) noexcept {
  cr::CreativeLinearArrayRequest request;
  request.direction = settings.arrayDirection;
  request.copyCount = settings.arrayCopyCount;
  request.spacing = settings.arraySpacing;
  request.cellSize = cellSize;
  return request;
}

cr::CreativeLinearArrayReceipt applyCreativeEditorLinearArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    double cellSize,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  state.lastReceipt = appState.facade.createLinearArrayFromSelection(
      creativeEditorLinearArrayRequest(settings, cellSize));
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastReceipt.accepted && state.lastReceipt.changed,
      state.lastReceipt.message));

  SDL_Log("iggy3d_creative: LINEAR_ARRAY status='%s' accepted=%d changed=%d "
          "sourceObjects=%llu generatedObjects=%llu direction='%s' copies='%s' "
          "spacing='%s' reasonCode='%s'",
          std::string(cr::toString(state.lastReceipt.status)).c_str(),
          state.lastReceipt.accepted ? 1 : 0,
          state.lastReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(state.lastReceipt.sourceObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.generatedObjectCount),
          std::string(cr::toString(settings.arrayDirection)).c_str(),
          std::string(cr::toString(settings.arrayCopyCount)).c_str(),
          std::string(cr::toString(settings.arraySpacing)).c_str(),
          state.lastReceipt.message.c_str());
  return state.lastReceipt;
}

std::size_t appendCreativeEditorLinearArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::LinearArray) {
    return 0U;
  }

  const std::span<const cr::TargetRef> selected =
      cr::selectedTargetList(appState.facade.selectionState());
  std::uint64_t sourceObjectCount = 0U;
  for (cr::TargetRef target : selected) {
    if (target.value != cr::kInvalidId &&
        appState.facade.findObject(
            static_cast<cr::CreativeObjectId>(target.value)) != nullptr) {
      ++sourceObjectCount;
    }
  }

  const cr::CreativeLinearArrayRequest request =
      creativeEditorLinearArrayRequest(editor.toolSettings,
                                       editor.placeCellSize);
  cr::CreativeLinearArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = sourceObjectCount;
  planRequest.direction = request.direction;
  planRequest.copyCount = request.copyCount;
  planRequest.spacing = request.spacing;
  planRequest.cellSize = request.cellSize;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  const cr::CreativeLinearArrayPlanReceipt plan =
      cr::planCreativeLinearArray(planRequest);
  if (!plan.accepted) {
    return 0U;
  }

  const std::size_t before = wireLines.size();
  const iggy3d::RenderLineColor color{0.22F, 0.88F, 1.0F, 0.90F};
  for (const cr::CreativeLinearArrayInstance& instance :
       plan.plannedInstances()) {
    const cr::CreativeCoreVec3Conversion offset =
        cr::creativeVec3ToCoreChecked(instance.offset);
    if (!offset.converted) {
      continue;
    }
    for (cr::TargetRef target : selected) {
      if (target.value == cr::kInvalidId) {
        continue;
      }
      const cr::CreativeObject* object = appState.facade.findObject(
          static_cast<cr::CreativeObjectId>(target.value));
      if (object == nullptr) {
        continue;
      }
      const VisualBounds bounds = visualBoundsForObject(*object);
      appendStandaloneWireframeBoxEdges(
          wireLines, bounds.min + offset.value, bounds.max + offset.value,
          color,
          std::max(0.025F, wireThickness * 0.8F));
    }
  }
  return wireLines.size() - before;
}

}  // namespace iggy3d_creative_app
