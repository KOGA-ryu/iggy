#include "app/iggy3d/menu/CreativeUiDrawList.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const iggy3d::ProductUiPrimitive* findPrimitive(
    const iggy3d::ProductUiDrawList& list,
    std::string_view semanticId) {
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    if (primitive.semanticId == semanticId) {
      return &primitive;
    }
  }
  return nullptr;
}

const iggy3d::UiHitRegion* findHitRegion(
    const iggy3d::ProductUiDrawList& list,
    std::string_view semanticId) {
  for (const iggy3d::UiHitRegion& hit : list.hitRegions) {
    if (hit.semanticId == semanticId) {
      return &hit;
    }
  }
  return nullptr;
}

std::size_t primitiveIndex(const iggy3d::ProductUiDrawList& list,
                           std::string_view semanticId) {
  for (std::size_t index = 0; index < list.primitives.size(); ++index) {
    if (list.primitives[index].semanticId == semanticId) {
      return index;
    }
  }
  return list.primitives.size();
}

bool hasPrefix(std::string_view text, std::string_view prefix) {
  return text.rfind(prefix, 0) == 0;
}

bool rectEquals(const iggy3d::ProductUiRect& lhs,
                const iggy3d::ProductUiRect& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width &&
         lhs.height == rhs.height;
}

std::uint64_t countKind(const iggy3d::ProductUiDrawList& list,
                        iggy3d::ProductUiPrimitiveKind kind) {
  std::uint64_t count = 0;
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    if (primitive.kind == kind) {
      ++count;
    }
  }
  return count;
}

bool rowHitMatchesTextPrimitive(const iggy3d::ProductUiDrawList& list,
                                std::string_view semanticId) {
  const iggy3d::ProductUiPrimitive* primitive =
      findPrimitive(list, semanticId);
  const iggy3d::UiHitRegion* hit = findHitRegion(list, semanticId);
  return expect(primitive != nullptr, "matching primitive exists") &&
         expect(hit != nullptr, "matching hit exists") &&
         expect(primitive->kind == iggy3d::ProductUiPrimitiveKind::Text,
                "matching primitive text") &&
         expect(hit->semanticId == primitive->semanticId,
                "matching semantic") &&
         expect(rectEquals(hit->rect, primitive->rect), "matching rect") &&
         expect(hit->kind == iggy3d::UiHitKind::Row, "matching hit row kind") &&
         expect(hit->enabled == primitive->enabled,
                "matching enabled state") &&
         expect(hit->action == iggy3d::FrontendAction::None,
                "matching hit action none");
}

cr::CreativeUiModel defaultCreativeUiModel() {
  return cr::buildCreativeUiModel(cr::makeDefaultCreativeUiBuildRequest()).model;
}

cr::CreativeUiModel populatedCreativeUiModel() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
  request.inspectionState.inspectedTarget.value = 84;
  request.measurementState.active = true;
  request.measurementState.hasMeasurement = true;
  request.measurementState.startPoint = {1.0, 2.0, cr::TargetRef{7}};
  request.measurementState.currentPoint = {3.0, 4.0, cr::TargetRef{9}};
  request.measurementState.sampleCount = 2;
  request.ghostState.visible = true;
  request.ghostState.sourceTool = cr::Tool::Measure;
  request.ghostState.rawPoint = {1.2, 2.7};
  request.ghostState.snappedPoint = {1.0, 3.0};
  request.ghostState.target.value = 99;
  request.ghostState.snapAccepted = true;
  request.ghostState.snapApplied = true;
  request.ghostState.snapChanged = true;
  request.ghostState.updateCount = 5;
  return cr::buildCreativeUiModel(request).model;
}

cr::CreativeUiModel disabledRowCreativeUiModel() {
  cr::CreativeUiModel model = defaultCreativeUiModel();
  for (cr::CreativeUiRow& row : model.rows) {
    if (row.id == "active_tool") {
      row.flags &= ~cr::kCreativeUiRowFlagEnabled;
      break;
    }
  }
  return model;
}

bool nullModelFailsClosed() {
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.virtualWidth = 1024;
  request.virtualHeight = 768;
  request.theme = iggy3d::ProductUiThemeId::Journal;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  return expect(!list.ready, "null not ready") &&
         expect(!list.partial, "null not partial") &&
         expect(list.status == "product_creative_ui_model_missing",
                "null status") &&
         expect(list.reasonCode == "product_creative_ui_model_missing",
                "null reason") &&
         expect(list.virtualWidth == 1024U, "null width") &&
         expect(list.virtualHeight == 768U, "null height") &&
         expect(list.theme == iggy3d::ProductUiThemeId::Journal,
                "null theme") &&
         expect(list.primitives.empty(), "null primitives") &&
         expect(list.hitRegions.empty(), "null hit regions") &&
         expect(list.hitRegionCount == 0U, "null hit count");
}

bool defaultModelProducesReadyDrawList() {
  const cr::CreativeUiModel model = defaultCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  request.virtualWidth = 1440;
  request.virtualHeight = 900;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  return expect(list.ready, "default ready") &&
         expect(!list.partial, "default not partial") &&
         expect(list.status == "product_creative_ui_draw_list_ready",
                "default status") &&
         expect(list.reasonCode == "product_creative_ui_draw_list_ready",
                "default reason") &&
         expect(list.virtualWidth == 1440U, "default width") &&
         expect(list.virtualHeight == 900U, "default height") &&
         expect(list.theme == iggy3d::ProductUiThemeId::System,
                "default theme");
}

bool primitivesAreNonInteractiveCreativeSemantics() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  bool ok = true;
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    ok = expect(hasPrefix(primitive.semanticId, "creative."),
                "creative semantic prefix") &&
         ok;
    ok = expect(primitive.action == iggy3d::FrontendAction::None,
                "primitive action none") &&
         ok;
  }
  for (const iggy3d::UiHitRegion& hit : list.hitRegions) {
    ok = expect(hasPrefix(hit.semanticId, "creative."),
                "hit creative semantic prefix") &&
         ok;
    ok = expect(hit.kind == iggy3d::UiHitKind::Row, "hit row kind") && ok;
    ok = expect(hit.action == iggy3d::FrontendAction::None,
                "hit action none") &&
         ok;
  }

  return ok && expect(list.hitRegionCount == list.hitRegions.size(),
                      "hit region count size");
}

bool defaultModelEmitsVisiblePanelsAndRowsOnly() {
  const cr::CreativeUiModel model = defaultCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const iggy3d::ProductUiPrimitive* active =
      findPrimitive(list, "creative.row.tools.active_tool");
  const iggy3d::ProductUiPrimitive* status =
      findPrimitive(list, "creative.row.status.creative_status");
  const iggy3d::ProductUiPrimitive* snap =
      findPrimitive(list, "creative.row.snap.snap_settings");

  return expect(findPrimitive(list, "creative.panel.tools") != nullptr,
                "tools panel") &&
         expect(findPrimitive(list, "creative.panel.status") != nullptr,
                "status panel") &&
         expect(findPrimitive(list, "creative.panel.snap") != nullptr,
                "snap panel") &&
         expect(findPrimitive(list, "creative.panel.selection") == nullptr,
                "hidden selection panel") &&
         expect(findPrimitive(list, "creative.panel.inspection") == nullptr,
                "hidden inspection panel") &&
         expect(findPrimitive(list, "creative.panel.measurement") == nullptr,
                "hidden measurement panel") &&
         expect(findPrimitive(list, "creative.panel.ghost") == nullptr,
                "hidden ghost panel") &&
         expect(active != nullptr && active->text == "Active Tool: Select",
                "active row text") &&
         expect(status != nullptr && hasPrefix(status->text, "Creative Status:"),
                "status row text") &&
         expect(snap != nullptr &&
                    snap->text ==
                        "Snap Settings: mode=Grid axes=XY step=(1.00, 1.00) "
                        "origin=(0.00, 0.00)",
                "snap row text") &&
         expect(rowHitMatchesTextPrimitive(list,
                                           "creative.row.tools.active_tool"),
                "active row hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.status.creative_status"),
                "status row hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.snap.snap_settings"),
                "snap row hit") &&
         expect(findHitRegion(list,
                              "creative.row.selection.selected_target") ==
                    nullptr,
                "hidden selection hit") &&
         expect(findHitRegion(list,
                              "creative.row.inspection.inspected_target") ==
                    nullptr,
                "hidden inspection hit") &&
         expect(findHitRegion(list,
                              "creative.row.measurement.measurement_state") ==
                    nullptr,
                "hidden measurement hit") &&
         expect(findHitRegion(list, "creative.row.ghost.ghost_preview") ==
                    nullptr,
                "hidden ghost hit");
}

bool populatedModelPreservesCreativeOrderAndText() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const iggy3d::ProductUiPrimitive* selected =
      findPrimitive(list, "creative.row.selection.selected_target");
  const iggy3d::ProductUiPrimitive* inspected =
      findPrimitive(list, "creative.row.inspection.inspected_target");
  const iggy3d::ProductUiPrimitive* measurement =
      findPrimitive(list, "creative.row.measurement.measurement_state");
  const iggy3d::ProductUiPrimitive* start =
      findPrimitive(list, "creative.row.measurement.measurement_start");
  const iggy3d::ProductUiPrimitive* ghost =
      findPrimitive(list, "creative.row.ghost.ghost_preview");
  const std::size_t selectionPanel =
      primitiveIndex(list, "creative.panel.selection");
  const std::size_t inspectionPanel =
      primitiveIndex(list, "creative.panel.inspection");
  const std::size_t measurementPanel =
      primitiveIndex(list, "creative.panel.measurement");
  const std::size_t ghostPanel = primitiveIndex(list, "creative.panel.ghost");

  return expect(selected != nullptr &&
                    selected->text == "Selected Target: target=42",
                "selected row text") &&
         expect(inspected != nullptr &&
                    inspected->text == "Inspected Target: target=84",
                "inspected row text") &&
         expect(measurement != nullptr &&
                    measurement->text == "Measurement State: active samples=2",
                "measurement row text") &&
         expect(start != nullptr &&
                    start->text ==
                        "Measurement Start: point=(1.00, 2.00) target=7",
                "measurement start text") &&
         expect(ghost != nullptr &&
                    ghost->text ==
                        "Ghost Preview: raw=(1.20, 2.70) snapped=(1.00, 3.00) "
                        "target=99 tool=Measure",
                "ghost row text") &&
         expect(selectionPanel != list.primitives.size(),
                "selection panel exists") &&
         expect(inspectionPanel != list.primitives.size(),
                "inspection panel exists") &&
         expect(measurementPanel != list.primitives.size(),
                "measurement panel exists") &&
         expect(ghostPanel != list.primitives.size(), "ghost panel exists") &&
         expect(selectionPanel < inspectionPanel,
                "selection before inspection") &&
         expect(measurementPanel < ghostPanel,
                "measurement before ghost") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.selection.selected_target"),
                "selected hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.inspection.inspected_target"),
                "inspected hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.measurement.measurement_state"),
                "measurement hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.measurement.measurement_start"),
                "measurement start hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.ghost.ghost_preview"),
                "ghost hit");
}

bool countersMatchPrimitiveContents() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const std::uint64_t textCount =
      countKind(list, iggy3d::ProductUiPrimitiveKind::Text);
  const std::uint64_t panelCount =
      countKind(list, iggy3d::ProductUiPrimitiveKind::Panel);

  return expect(list.primitiveCount == list.primitives.size(),
                "primitive count") &&
         expect(list.textCount == textCount, "text count") &&
         expect(list.rectCount == panelCount, "rect count") &&
         expect(list.rowCount == textCount, "row count") &&
         expect(list.disabledRowCount == 0U, "disabled row count") &&
         expect(list.hitRegionCount == list.hitRegions.size(),
                "hit count size") &&
         expect(list.hitRegionCount == list.rowCount, "hit count row count");
}

bool disabledRowsEmitDisabledHitRegions() {
  const cr::CreativeUiModel model = disabledRowCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const iggy3d::ProductUiPrimitive* active =
      findPrimitive(list, "creative.row.tools.active_tool");
  const iggy3d::UiHitRegion* activeHit =
      findHitRegion(list, "creative.row.tools.active_tool");

  return expect(list.disabledRowCount == 1U, "disabled row count one") &&
         expect(active != nullptr, "disabled active primitive") &&
         expect(activeHit != nullptr, "disabled active hit") &&
         expect(!active->enabled, "disabled primitive enabled false") &&
         expect(!activeHit->enabled, "disabled hit enabled false") &&
         expect(activeHit->action == iggy3d::FrontendAction::None,
                "disabled hit action none") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.tools.active_tool"),
                "disabled hit matches primitive");
}

bool repeatedBuildIsStable() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList first =
      iggy3d::buildProductCreativeUiDrawList(request);
  const iggy3d::ProductUiDrawList second =
      iggy3d::buildProductCreativeUiDrawList(request);

  return expect(first.primitiveCount == second.primitiveCount,
                "repeat primitive count") &&
         expect(first.primitives.size() == second.primitives.size(),
                "repeat primitive size") &&
         expect(first.primitives[0].semanticId == second.primitives[0].semanticId,
                "repeat first semantic") &&
         expect(first.primitives[1].text == second.primitives[1].text,
                "repeat first text") &&
         expect(first.primitives.back().semanticId ==
                    second.primitives.back().semanticId,
                "repeat last semantic") &&
         expect(first.primitives.back().text == second.primitives.back().text,
                "repeat last text") &&
         expect(first.hitRegionCount == second.hitRegionCount,
                "repeat hit count") &&
         expect(first.hitRegions.size() == second.hitRegions.size(),
                "repeat hit size") &&
         expect(first.hitRegions.front().semanticId ==
                    second.hitRegions.front().semanticId,
                "repeat first hit") &&
         expect(first.hitRegions.back().semanticId ==
                    second.hitRegions.back().semanticId,
                "repeat last hit");
}

}  // namespace

int main() {
  const bool ok = nullModelFailsClosed() &&
                  defaultModelProducesReadyDrawList() &&
                  primitivesAreNonInteractiveCreativeSemantics() &&
                  defaultModelEmitsVisiblePanelsAndRowsOnly() &&
                  populatedModelPreservesCreativeOrderAndText() &&
                  countersMatchPrimitiveContents() &&
                  disabledRowsEmitDisabledHitRegions() &&
                  repeatedBuildIsStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
