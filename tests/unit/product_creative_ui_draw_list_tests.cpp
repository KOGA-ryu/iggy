#include "app/iggy3d/creative/ui/UiDrawList.hpp"

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

bool rectsOverlap(const iggy3d::ProductUiRect& lhs,
                  const iggy3d::ProductUiRect& rhs) {
  return lhs.x < rhs.x + rhs.width && lhs.x + lhs.width > rhs.x &&
         lhs.y < rhs.y + rhs.height && lhs.y + lhs.height > rhs.y;
}

bool rectInsideVirtualFrame(const iggy3d::ProductUiRect& rect,
                            std::uint32_t width,
                            std::uint32_t height) {
  return rect.x >= 0.0F && rect.y >= 0.0F &&
         rect.x + rect.width <= static_cast<float>(width) &&
         rect.y + rect.height <= static_cast<float>(height);
}

bool textFitsPrimitiveRect(const iggy3d::ProductUiPrimitive& primitive) {
  constexpr float kGlyphAdvance = 12.0F;
  return static_cast<float>(primitive.text.size()) * kGlyphAdvance <=
         primitive.rect.width;
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

cr::CreativeUiObjectSummary objectSummary(cr::Id target,
                                          cr::CreativeObjectKind kind,
                                          bool visible) {
  cr::CreativeUiObjectSummary summary;
  summary.target.value = target;
  summary.objectKind = kind;
  summary.exists = true;
  summary.visible = visible;
  return summary;
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
  request.objectSummaries.push_back(
      objectSummary(42, cr::CreativeObjectKind::Room, true));
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

cr::CreativeUiModel selectedTargetModel(cr::Id target,
                                        bool withSummary,
                                        bool visible) {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = target;
  if (withSummary) {
    request.objectSummaries.push_back(
        objectSummary(target, cr::CreativeObjectKind::Room, visible));
  }
  return cr::buildCreativeUiModel(request).model;
}

cr::CreativeUiModel inspectorCreativeUiModel() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
  cr::CreativeUiObjectSummary summary;
  summary.target.value = 42;
  summary.objectKind = cr::CreativeObjectKind::Crate;
  summary.exists = true;
  summary.visible = true;
  summary.locked = false;
  summary.name = "Crate A";
  summary.objectId = 42;
  summary.layerId = 3;
  summary.bounds.min = {1.0, 2.0, 3.0};
  summary.bounds.max = {5.0, 6.0, 7.0};
  summary.position = {1.0, 2.0, 3.0};
  request.objectSummaries.push_back(summary);
  return cr::buildCreativeUiModel(request).model;
}

cr::CreativeUiModel disabledRowCreativeUiModel() {
  cr::CreativeUiModel model = defaultCreativeUiModel();
  for (cr::CreativeUiRow& row : model.rows) {
    if (row.id == "tool_select") {
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

  const iggy3d::ProductUiPrimitive* toolSelect =
      findPrimitive(list, "creative.row.tools.tool_select");
  const iggy3d::ProductUiPrimitive* toolMove =
      findPrimitive(list, "creative.row.tools.tool_move");
  const iggy3d::ProductUiPrimitive* toolMeasure =
      findPrimitive(list, "creative.row.tools.tool_measure");
  const iggy3d::ProductUiPrimitive* toolNavigate =
      findPrimitive(list, "creative.row.tools.tool_navigate");
  const iggy3d::ProductUiPrimitive* createRoom =
      findPrimitive(list, "creative.row.create.create_room");
  const iggy3d::ProductUiPrimitive* createCrate =
      findPrimitive(list, "creative.row.create.create_crate");
  const iggy3d::ProductUiPrimitive* status =
      findPrimitive(list, "creative.row.status.creative_status");
  const iggy3d::ProductUiPrimitive* snap =
      findPrimitive(list, "creative.row.snap.snap_settings");

  const iggy3d::ProductUiPrimitive* inspectorEmpty =
      findPrimitive(list, "creative.row.selection.inspector_empty");

  return expect(findPrimitive(list, "creative.panel.tools") != nullptr,
                "tools panel") &&
         expect(findPrimitive(list, "creative.panel.create") != nullptr,
                "create panel") &&
         expect(findPrimitive(list, "creative.panel.status") != nullptr,
                "status panel") &&
         expect(findPrimitive(list, "creative.panel.snap") != nullptr,
                "snap panel") &&
         // The inspector (Selection panel) is always present; with no
         // selection it shows its resting row (TL-6).
         expect(findPrimitive(list, "creative.panel.selection") != nullptr,
                "inspector panel present") &&
         expect(inspectorEmpty != nullptr &&
                    inspectorEmpty->text == "No selection - click an object",
                "inspector resting row text") &&
         expect(findPrimitive(list,
                              "creative.row.selection.selected_target") ==
                    nullptr,
                "no selected row without selection") &&
         expect(findPrimitive(list, "creative.panel.measurement") == nullptr,
                "hidden measurement panel") &&
         expect(findPrimitive(list, "creative.panel.ghost") == nullptr,
                "hidden ghost panel") &&
         expect(toolSelect != nullptr &&
                    toolSelect->text == "Select (active)",
                "select tool row text") &&
         expect(toolMove != nullptr && toolMove->text == "Move",
                "move tool row text") &&
         expect(toolMeasure != nullptr && toolMeasure->text == "Measure",
                "measure tool row text") &&
         expect(toolNavigate != nullptr && toolNavigate->text == "Navigate",
                "navigate tool row text") &&
         expect(createRoom != nullptr && createRoom->text == "Create Room",
                "create room row text") &&
         expect(createCrate != nullptr &&
                    createCrate->text == "Create Crate",
                "create crate row text") &&
         expect(status != nullptr && status->text == "Creative: Ready",
                "status row text") &&
         expect(snap != nullptr && snap->text == "Snap: Grid XY 1.00x1.00",
                "snap row text") &&
         expect(rowHitMatchesTextPrimitive(list,
                                           "creative.row.tools.tool_select"),
                "select tool row hit") &&
         expect(rowHitMatchesTextPrimitive(list,
                                           "creative.row.tools.tool_move"),
                "move tool row hit") &&
         expect(rowHitMatchesTextPrimitive(list,
                                           "creative.row.tools.tool_measure"),
                "measure tool row hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.tools.tool_navigate"),
                "navigate tool row hit") &&
         expect(rowHitMatchesTextPrimitive(list,
                                           "creative.row.create.create_room"),
                "create room row hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.create.create_crate"),
                "create crate row hit") &&
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
                              "creative.row.measurement.measurement_state") ==
                    nullptr,
                "hidden measurement hit") &&
         expect(findHitRegion(list, "creative.row.ghost.ghost_preview") ==
                    nullptr,
                "hidden ghost hit");
}

bool toolPaletteTextMarksOnlyActiveToolAcrossAllFourTools() {
  struct Row {
    cr::Tool tool;
    std::string_view semanticId;
    std::string_view activeText;
    std::string_view inactiveText;
  };
  const Row rows[] = {
      {cr::Tool::Select, "creative.row.tools.tool_select",
       "Select (active)", "Select"},
      {cr::Tool::Move, "creative.row.tools.tool_move",
       "Move (active)", "Move"},
      {cr::Tool::Measure, "creative.row.tools.tool_measure",
       "Measure (active)", "Measure"},
      {cr::Tool::Navigate, "creative.row.tools.tool_navigate",
       "Navigate (active)", "Navigate"},
  };

  bool ok = true;
  for (const Row& activeRow : rows) {
    cr::CreativeUiBuildRequest buildRequest =
        cr::makeDefaultCreativeUiBuildRequest();
    buildRequest.toolState.activeTool = activeRow.tool;
    const cr::CreativeUiModel model =
        cr::buildCreativeUiModel(buildRequest).model;
    iggy3d::ProductCreativeUiDrawListRequest request;
    request.model = &model;
    const iggy3d::ProductUiDrawList list =
        iggy3d::buildProductCreativeUiDrawList(request);
    for (const Row& row : rows) {
      const iggy3d::ProductUiPrimitive* primitive =
          findPrimitive(list, row.semanticId);
      const std::string_view expected =
          row.tool == activeRow.tool ? row.activeText : row.inactiveText;
      ok = expect(primitive != nullptr && primitive->text == expected,
                  "tool row text tracks active tool") &&
           ok;
    }
  }
  return ok;
}

bool defaultModelUsesSeparatedPanelZones() {
  const cr::CreativeUiModel model = defaultCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  request.virtualWidth = 1280;
  request.virtualHeight = 720;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const iggy3d::ProductUiPrimitive* tools =
      findPrimitive(list, "creative.panel.tools");
  const iggy3d::ProductUiPrimitive* create =
      findPrimitive(list, "creative.panel.create");
  const iggy3d::ProductUiPrimitive* status =
      findPrimitive(list, "creative.panel.status");
  const iggy3d::ProductUiPrimitive* snap =
      findPrimitive(list, "creative.panel.snap");

  return expect(tools != nullptr, "layout tools panel") &&
         expect(create != nullptr, "layout create panel") &&
         expect(status != nullptr, "layout status panel") &&
         expect(snap != nullptr, "layout snap panel") &&
         expect(rectInsideVirtualFrame(tools->rect, 1280, 720),
                "tools inside frame") &&
         expect(rectInsideVirtualFrame(create->rect, 1280, 720),
                "create inside frame") &&
         expect(rectInsideVirtualFrame(status->rect, 1280, 720),
                "status inside frame") &&
         expect(rectInsideVirtualFrame(snap->rect, 1280, 720),
                "snap inside frame") &&
         expect(!rectsOverlap(tools->rect, create->rect),
                "tools and create separated") &&
         expect(!rectsOverlap(create->rect, status->rect),
                "create and status separated") &&
         expect(!rectsOverlap(status->rect, snap->rect),
                "status and snap separated") &&
         expect(create->rect.x == tools->rect.x,
                "create stacked in tools column") &&
         expect(create->rect.y > tools->rect.y + tools->rect.height,
                "create below tools") &&
         expect(status->rect.y > create->rect.y + create->rect.height,
                "status below create") &&
         expect(snap->rect.y > status->rect.y + status->rect.height,
                "snap below status");
}

bool populatedModelPreservesCreativeOrderAndText() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const iggy3d::ProductUiPrimitive* selected =
      findPrimitive(list, "creative.row.selection.selected_target");
  const iggy3d::ProductUiPrimitive* measurement =
      findPrimitive(list, "creative.row.measurement.measurement_state");
  const iggy3d::ProductUiPrimitive* start =
      findPrimitive(list, "creative.row.measurement.measurement_start");
  const iggy3d::ProductUiPrimitive* ghost =
      findPrimitive(list, "creative.row.ghost.ghost_preview");
  const std::size_t selectionPanel =
      primitiveIndex(list, "creative.panel.selection");
  const std::size_t measurementPanel =
      primitiveIndex(list, "creative.panel.measurement");
  const std::size_t ghostPanel = primitiveIndex(list, "creative.panel.ghost");

  return expect(selected != nullptr &&
                    selected->text == "Selected: target=42 visible=true",
                "selected row text") &&
         expect(measurement != nullptr &&
                    measurement->text == "Measure: active samples=2",
                "measurement row text") &&
         expect(start != nullptr &&
                    start->text == "Start: (1.00,2.00) target=7",
                "measurement start text") &&
         expect(ghost != nullptr &&
                    ghost->text == "Ghost: (1.20,2.70)->(1.00,3.00) target=99",
                "ghost row text") &&
         expect(selectionPanel != list.primitives.size(),
                "selection panel exists") &&
         expect(measurementPanel != list.primitives.size(),
                "measurement panel exists") &&
         expect(ghostPanel != list.primitives.size(), "ghost panel exists") &&
         expect(selectionPanel < measurementPanel,
                "selection before measurement") &&
         expect(measurementPanel < ghostPanel,
                "measurement before ghost") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.selection.selected_target"),
                "selected hit") &&
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

bool populatedModelTextStaysInsidePrimitiveRects() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  request.virtualWidth = 1280;
  request.virtualHeight = 720;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  bool ok = true;
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    ok = expect(rectInsideVirtualFrame(primitive.rect, 1280, 720),
                "primitive inside frame") &&
         ok;
    if (primitive.kind == iggy3d::ProductUiPrimitiveKind::Text) {
      ok = expect(textFitsPrimitiveRect(primitive),
                  "creative text fits primitive rect") &&
           ok;
    }
  }
  return ok;
}

bool selectedTargetVisibilityTextHandlesHiddenAndUnknown() {
  const cr::CreativeUiModel hiddenModel = selectedTargetModel(42, true, false);
  iggy3d::ProductCreativeUiDrawListRequest hiddenRequest;
  hiddenRequest.model = &hiddenModel;
  const iggy3d::ProductUiDrawList hiddenList =
      iggy3d::buildProductCreativeUiDrawList(hiddenRequest);
  const iggy3d::ProductUiPrimitive* hidden =
      findPrimitive(hiddenList, "creative.row.selection.selected_target");

  const cr::CreativeUiModel missingModel = selectedTargetModel(77, false, false);
  iggy3d::ProductCreativeUiDrawListRequest missingRequest;
  missingRequest.model = &missingModel;
  const iggy3d::ProductUiDrawList missingList =
      iggy3d::buildProductCreativeUiDrawList(missingRequest);
  const iggy3d::ProductUiPrimitive* missing =
      findPrimitive(missingList, "creative.row.selection.selected_target");

  return expect(hidden != nullptr &&
                    hidden->semanticId ==
                        "creative.row.selection.selected_target",
                "hidden semantic unchanged") &&
         expect(hidden != nullptr &&
                    hidden->text == "Selected: target=42 visible=false",
                "hidden selected text") &&
         expect(missing != nullptr &&
                    missing->text == "Selected: target=77 visible=unknown",
                "missing selected text") &&
         expect(rowHitMatchesTextPrimitive(
                    hiddenList, "creative.row.selection.selected_target"),
                "hidden selected hit") &&
         expect(rowHitMatchesTextPrimitive(
                    missingList, "creative.row.selection.selected_target"),
                "missing selected hit");
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
      findPrimitive(list, "creative.row.tools.tool_select");
  const iggy3d::UiHitRegion* activeHit =
      findHitRegion(list, "creative.row.tools.tool_select");

  return expect(list.disabledRowCount == 1U, "disabled row count one") &&
         expect(active != nullptr, "disabled active primitive") &&
         expect(activeHit != nullptr, "disabled active hit") &&
         expect(!active->enabled, "disabled primitive enabled false") &&
         expect(!activeHit->enabled, "disabled hit enabled false") &&
         expect(activeHit->action == iggy3d::FrontendAction::None,
                "disabled hit action none") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.tools.tool_select"),
                "disabled hit matches primitive");
}

bool inspectorRowsRenderExactTextForKnownObject() {
  const cr::CreativeUiModel model = inspectorCreativeUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  // Wide frame so the full inspector rows render untruncated for the pin.
  request.virtualWidth = 2400;
  request.virtualHeight = 1200;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(request);

  const iggy3d::ProductUiPrimitive* selected =
      findPrimitive(list, "creative.row.selection.selected_target");
  const iggy3d::ProductUiPrimitive* kind =
      findPrimitive(list, "creative.row.selection.inspector_kind");
  const iggy3d::ProductUiPrimitive* id =
      findPrimitive(list, "creative.row.selection.inspector_id");
  const iggy3d::ProductUiPrimitive* name =
      findPrimitive(list, "creative.row.selection.inspector_name");
  const iggy3d::ProductUiPrimitive* visible =
      findPrimitive(list, "creative.row.selection.inspector_visible");
  const iggy3d::ProductUiPrimitive* locked =
      findPrimitive(list, "creative.row.selection.inspector_locked");
  const iggy3d::ProductUiPrimitive* bounds =
      findPrimitive(list, "creative.row.selection.inspector_bounds");
  const iggy3d::ProductUiPrimitive* position =
      findPrimitive(list, "creative.row.selection.inspector_position");
  const iggy3d::ProductUiPrimitive* layer =
      findPrimitive(list, "creative.row.selection.inspector_layer");

  return expect(selected != nullptr &&
                    selected->text == "Selected: target=42 visible=true",
                "inspector selected text") &&
         expect(kind != nullptr && kind->text == "Kind: Crate",
                "inspector kind text") &&
         expect(id != nullptr && id->text == "Id: 42", "inspector id text") &&
         expect(name != nullptr && name->text == "Name: Crate A",
                "inspector name text") &&
         expect(visible != nullptr && visible->text == "Visible: true",
                "inspector visible text") &&
         expect(locked != nullptr && locked->text == "Locked: false",
                "inspector locked text") &&
         // Bounds shows min/max (size lives in the row fields for v1.5).
         expect(bounds != nullptr &&
                    bounds->text ==
                        "Bounds: min(1.00,2.00,3.00) max(5.00,6.00,7.00)",
                "inspector bounds text") &&
         expect(position != nullptr &&
                    position->text == "Position: (1.00,2.00,3.00)",
                "inspector position text") &&
         expect(layer != nullptr && layer->text == "Layer: 3",
                "inspector layer text") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.selection.inspector_visible"),
                "inspector visible hit") &&
         expect(rowHitMatchesTextPrimitive(
                    list, "creative.row.selection.inspector_locked"),
                "inspector locked hit");
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
                  toolPaletteTextMarksOnlyActiveToolAcrossAllFourTools() &&
                  defaultModelUsesSeparatedPanelZones() &&
                  populatedModelPreservesCreativeOrderAndText() &&
                  populatedModelTextStaysInsidePrimitiveRects() &&
                  selectedTargetVisibilityTextHandlesHiddenAndUnknown() &&
                  countersMatchPrimitiveContents() &&
                  disabledRowsEmitDisabledHitRegions() &&
                  inspectorRowsRenderExactTextForKnownObject() &&
                  repeatedBuildIsStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
