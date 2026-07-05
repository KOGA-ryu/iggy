#include "app/iggy3d/menu/CreativeUiProjection.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"

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

cr::CreativeUiModel defaultCreativeUiModel() {
  return cr::buildCreativeUiModel(cr::makeDefaultCreativeUiBuildRequest()).model;
}

cr::CreativeUiModel populatedCreativeUiModel() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
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

void populateSelectedFacade(cr::Facade& facade) {
  // Move selects like Select until the drag slice (TV1-F/G) lands.
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = 88;
  static_cast<void>(facade.dispatchToolInput(input));
}

bool countsMirrorDrawList(
    const iggy3d::ProductCreativeUiProjection& projection) {
  const iggy3d::ProductCreativeUiProjectionReceipt& receipt =
      projection.receipt;
  const iggy3d::ProductUiDrawList& drawList = projection.drawList;
  return expect(receipt.ready == drawList.ready, "ready mirrored") &&
         expect(receipt.partial == drawList.partial, "partial mirrored") &&
         expect(receipt.status == drawList.status, "status mirrored") &&
         expect(receipt.reasonCode == drawList.reasonCode,
                "reason mirrored") &&
         expect(receipt.primitiveCount == drawList.primitiveCount,
                "primitive count mirrored") &&
         expect(receipt.textCount == drawList.textCount,
                "text count mirrored") &&
         expect(receipt.rectCount == drawList.rectCount,
                "rect count mirrored") &&
         expect(receipt.rowCount == drawList.rowCount,
                "row count mirrored") &&
         expect(receipt.disabledRowCount == drawList.disabledRowCount,
                "disabled row count mirrored") &&
         expect(receipt.hitRegionCount == drawList.hitRegionCount,
                "hit count mirrored");
}

bool nullInputFailsClosed() {
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.virtualWidth = 1024;
  request.virtualHeight = 768;
  request.theme = iggy3d::ProductUiThemeId::Journal;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  return expect(projection.receipt.requested, "null requested") &&
         expect(!projection.receipt.ready, "null not ready") &&
         expect(!projection.receipt.partial, "null not partial") &&
         expect(projection.receipt.status ==
                    "product_creative_ui_model_missing",
                "null status") &&
         expect(projection.receipt.reasonCode ==
                    "product_creative_ui_model_missing",
                "null reason") &&
         expect(!projection.receipt.usedModel, "null no model") &&
         expect(!projection.receipt.usedFacade, "null no facade") &&
         expect(projection.receipt.virtualWidth == 1024U, "null width") &&
         expect(projection.receipt.virtualHeight == 768U, "null height") &&
         expect(projection.receipt.theme == iggy3d::ProductUiThemeId::Journal,
                "null theme") &&
         expect(projection.receipt.panelCount == 0U, "null panel count") &&
         expect(projection.receipt.modelRowCount == 0U,
                "null model row count") &&
         expect(projection.receipt.primitiveCount == 0U,
                "null primitive count") &&
         expect(projection.receipt.textCount == 0U, "null text count") &&
         expect(projection.receipt.rectCount == 0U, "null rect count") &&
         expect(projection.receipt.rowCount == 0U, "null row count") &&
         expect(projection.receipt.disabledRowCount == 0U,
                "null disabled row count") &&
         expect(projection.receipt.hitRegionCount == 0U, "null hit count") &&
         expect(!projection.drawList.ready, "null draw list not ready") &&
         expect(projection.drawList.primitives.empty(), "null primitives") &&
         expect(projection.drawList.hitRegions.empty(), "null hit regions");
}

bool modelInputBuildsReadyProjection() {
  const cr::CreativeUiModel model = defaultCreativeUiModel();
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.model = &model;
  request.virtualWidth = 1440;
  request.virtualHeight = 900;
  request.theme = iggy3d::ProductUiThemeId::Journal;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  return expect(projection.receipt.requested, "model requested") &&
         expect(projection.receipt.ready, "model ready") &&
         expect(projection.receipt.usedModel, "model used") &&
         expect(!projection.receipt.usedFacade, "facade not used") &&
         expect(projection.receipt.panelCount == model.panels.size(),
                "model panel count") &&
         expect(projection.receipt.modelRowCount == model.rows.size(),
                "model row count") &&
         expect(projection.drawList.virtualWidth == 1440U,
                "model draw width") &&
         expect(projection.receipt.virtualWidth == 1440U,
                "model receipt width") &&
         expect(projection.drawList.virtualHeight == 900U,
                "model draw height") &&
         expect(projection.receipt.virtualHeight == 900U,
                "model receipt height") &&
         expect(projection.drawList.theme == iggy3d::ProductUiThemeId::Journal,
                "model draw theme") &&
         expect(projection.receipt.theme == iggy3d::ProductUiThemeId::Journal,
                "model receipt theme") &&
         countsMirrorDrawList(projection);
}

bool facadeInputBuildsFromFacadeState() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.creative = &app;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  const iggy3d::ProductUiPrimitive* selected =
      findPrimitive(projection.drawList,
                    "creative.row.selection.selected_target");
  const iggy3d::ProductUiPrimitive* active =
      findPrimitive(projection.drawList, "creative.row.tools.tool_move");

  return expect(projection.receipt.ready, "facade ready") &&
         expect(projection.receipt.usedFacade, "facade used") &&
         expect(!projection.receipt.usedModel, "facade model not used") &&
         expect(projection.receipt.panelCount == 7U, "facade panel count") &&
         expect(projection.receipt.modelRowCount > 3U,
                "facade model rows populated") &&
         expect(active != nullptr && active->text == "Move (active)",
                "facade active tool row") &&
         expect(selected != nullptr &&
                    selected->text ==
                        "Selected: target=88 visible=unknown",
                "facade selected row") &&
         countsMirrorDrawList(projection);
}

bool modelTakesPrecedenceOverFacade() {
  const cr::CreativeUiModel model = defaultCreativeUiModel();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.model = &model;
  request.creative = &app;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  return expect(projection.receipt.usedModel, "precedence model used") &&
         expect(!projection.receipt.usedFacade, "precedence facade unused") &&
         expect(projection.receipt.panelCount == model.panels.size(),
                "precedence panel count") &&
         expect(projection.receipt.modelRowCount == model.rows.size(),
                "precedence row count") &&
         expect(findPrimitive(projection.drawList,
                              "creative.row.selection.selected_target") ==
                    nullptr,
                "precedence ignores facade selection");
}

bool populatedModelHasNonzeroCounts() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.model = &model;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  return expect(projection.receipt.panelCount > 0U, "populated panels") &&
         expect(projection.receipt.modelRowCount > 0U, "populated rows") &&
         expect(projection.receipt.primitiveCount > 0U,
                "populated primitives") &&
         expect(projection.receipt.textCount > 0U, "populated text") &&
         expect(projection.receipt.rectCount > 0U, "populated rect") &&
         expect(projection.receipt.rowCount > 0U, "populated row count") &&
         expect(projection.receipt.hitRegionCount > 0U,
                "populated hit count") &&
         expect(projection.receipt.hitRegionCount ==
                    projection.drawList.hitRegions.size(),
                "populated hit count mirrors regions") &&
         expect(projection.receipt.hitRegionCount ==
                    projection.receipt.rowCount,
                "populated hit count mirrors row count");
}

bool primitivesRemainNonInteractive() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.model = &model;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  bool ok = true;
  for (const iggy3d::ProductUiPrimitive& primitive :
       projection.drawList.primitives) {
    ok = expect(primitive.action == iggy3d::FrontendAction::None,
                "primitive action none") &&
         ok;
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = nullInputFailsClosed() &&
                  modelInputBuildsReadyProjection() &&
                  facadeInputBuildsFromFacadeState() &&
                  modelTakesPrecedenceOverFacade() &&
                  populatedModelHasNonzeroCounts() &&
                  primitivesRemainNonInteractive();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
