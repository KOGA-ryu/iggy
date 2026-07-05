#include "app/iggy3d/menu/UiHitRouter.hpp"

#include "app/iggy3d/creative/ui/Ui.hpp"
#include "app/iggy3d/creative/ui/UiDrawList.hpp"

#include <array>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::UiHitRegion region(std::string semanticId,
                           iggy3d::ProductUiRect rect,
                           bool enabled = true,
                           iggy3d::FrontendAction action =
                               iggy3d::FrontendAction::None,
                           iggy3d::UiHitKind kind = iggy3d::UiHitKind::Row) {
  iggy3d::UiHitRegion hit;
  hit.semanticId = std::move(semanticId);
  hit.rect = rect;
  hit.kind = kind;
  hit.action = action;
  hit.enabled = enabled;
  return hit;
}

iggy3d::ProductUiDrawList drawListWithRegions(
    std::initializer_list<iggy3d::UiHitRegion> regions,
    bool ready = true) {
  iggy3d::ProductUiDrawList list;
  list.ready = ready;
  list.status = ready ? "ready" : "not_ready";
  list.reasonCode = list.status;
  list.hitRegions.assign(regions.begin(), regions.end());
  list.hitRegionCount = list.hitRegions.size();
  return list;
}

bool nullOrEmptyLayersMissClosed() {
  const iggy3d::ProductUiHitRouteReceipt nullReceipt =
      iggy3d::routeProductUiHit({});

  const iggy3d::ProductUiHitLayer unusedLayer;
  iggy3d::ProductUiHitRouteRequest emptyRequest;
  emptyRequest.layers = &unusedLayer;
  emptyRequest.layerCount = 0U;
  const iggy3d::ProductUiHitRouteReceipt emptyReceipt =
      iggy3d::routeProductUiHit(emptyRequest);

  return expect(nullReceipt.requested, "null requested") &&
         expect(!nullReceipt.hit, "null no hit") &&
         expect(!nullReceipt.consumed, "null not consumed") &&
         expect(nullReceipt.message == "ui_hit_no_layers",
                "null no layers message") &&
         expect(emptyReceipt.requested, "empty requested") &&
         expect(!emptyReceipt.hit, "empty no hit") &&
         expect(emptyReceipt.message == "ui_hit_no_layers",
                "empty no layers message");
}

bool invalidLayersAreSkipped() {
  iggy3d::ProductUiDrawList disabledLayerList = drawListWithRegions({
      region("disabled_layer.row", {0.0F, 0.0F, 100.0F, 100.0F}),
  });
  iggy3d::ProductUiDrawList notReadyList = drawListWithRegions({
      region("not_ready.row", {0.0F, 0.0F, 100.0F, 100.0F}),
  }, false);
  iggy3d::ProductUiDrawList readyList = drawListWithRegions({
      region("ready.row", {0.0F, 0.0F, 100.0F, 100.0F}),
  });
  const std::array layers{
      iggy3d::ProductUiHitLayer{iggy3d::ProductUiHitSurface::PauseMenu,
                                nullptr,
                                true},
      iggy3d::ProductUiHitLayer{iggy3d::ProductUiHitSurface::StarterMenu,
                                &disabledLayerList,
                                false},
      iggy3d::ProductUiHitLayer{iggy3d::ProductUiHitSurface::Notebook,
                                &notReadyList,
                                true},
      iggy3d::ProductUiHitLayer{iggy3d::ProductUiHitSurface::CreativeOverlay,
                                &readyList,
                                true},
  };

  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit({layers.data(), layers.size(), 50.0F, 50.0F});

  return expect(receipt.hit, "valid later layer hit") &&
         expect(receipt.consumed, "valid later layer consumed") &&
         expect(receipt.surface == iggy3d::ProductUiHitSurface::CreativeOverlay,
                "valid later layer surface") &&
         expect(receipt.layerIndex == 3U, "valid later layer index") &&
         expect(receipt.semanticId == "ready.row", "valid later semantic");
}

bool outsideAllRegionsMisses() {
  iggy3d::ProductUiDrawList list = drawListWithRegions({
      region("row", {10.0F, 10.0F, 10.0F, 10.0F}),
  });
  const iggy3d::ProductUiHitLayer layer{
      iggy3d::ProductUiHitSurface::StarterMenu,
      &list,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit({&layer, 1U, 30.0F, 30.0F});

  return expect(receipt.requested, "miss requested") &&
         expect(!receipt.hit, "miss no hit") &&
         expect(!receipt.consumed, "miss not consumed") &&
         expect(receipt.message == "ui_hit_miss", "miss message");
}

bool enabledRegionConsumesAndCopiesFields() {
  iggy3d::ProductUiDrawList list = drawListWithRegions({
      region("pause.resume",
             {10.0F, 10.0F, 50.0F, 20.0F},
             true,
             iggy3d::FrontendAction::Resume,
             iggy3d::UiHitKind::Button),
  });
  const iggy3d::ProductUiHitLayer layer{
      iggy3d::ProductUiHitSurface::PauseMenu,
      &list,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit({&layer, 1U, 20.0F, 15.0F});

  return expect(receipt.hit, "enabled hit") &&
         expect(receipt.enabled, "enabled true") &&
         expect(receipt.consumed, "enabled consumed") &&
         expect(receipt.surface == iggy3d::ProductUiHitSurface::PauseMenu,
                "enabled surface") &&
         expect(receipt.kind == iggy3d::UiHitKind::Button,
                "enabled kind") &&
         expect(receipt.action == iggy3d::FrontendAction::Resume,
                "enabled action") &&
         expect(receipt.layerIndex == 0U, "enabled layer index") &&
         expect(receipt.regionIndex == 0U, "enabled region index") &&
         expect(receipt.semanticId == "pause.resume", "enabled semantic") &&
         expect(receipt.message == "ui_hit_consumed", "enabled message");
}

bool callerLayerOrderSetsPriority() {
  iggy3d::ProductUiDrawList top = drawListWithRegions({
      region("top.row", {0.0F, 0.0F, 100.0F, 100.0F}),
  });
  iggy3d::ProductUiDrawList lower = drawListWithRegions({
      region("lower.row", {0.0F, 0.0F, 100.0F, 100.0F}),
  });
  const std::array layers{
      iggy3d::ProductUiHitLayer{iggy3d::ProductUiHitSurface::CreativeOverlay,
                                &top,
                                true},
      iggy3d::ProductUiHitLayer{iggy3d::ProductUiHitSurface::StarterMenu,
                                &lower,
                                true},
  };
  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit({layers.data(), layers.size(), 5.0F, 5.0F});

  return expect(receipt.hit, "layer priority hit") &&
         expect(receipt.surface == iggy3d::ProductUiHitSurface::CreativeOverlay,
                "top layer surface") &&
         expect(receipt.layerIndex == 0U, "top layer index") &&
         expect(receipt.semanticId == "top.row", "top layer semantic");
}

bool backToFrontRegionOrderWithinLayer() {
  iggy3d::ProductUiDrawList list = drawListWithRegions({
      region("back.row", {0.0F, 0.0F, 100.0F, 100.0F}),
      region("front.row", {0.0F, 0.0F, 100.0F, 100.0F}),
  });
  const iggy3d::ProductUiHitLayer layer{
      iggy3d::ProductUiHitSurface::StarterMenu,
      &list,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit({&layer, 1U, 5.0F, 5.0F});

  return expect(receipt.hit, "back front hit") &&
         expect(receipt.regionIndex == 1U, "front region index") &&
         expect(receipt.semanticId == "front.row", "front semantic");
}

bool disabledRegionReportsButDoesNotConsume() {
  iggy3d::ProductUiDrawList list = drawListWithRegions({
      region("disabled.row",
             {0.0F, 0.0F, 100.0F, 100.0F},
             false,
             iggy3d::FrontendAction::Settings),
  });
  const iggy3d::ProductUiHitLayer layer{
      iggy3d::ProductUiHitSurface::PauseMenu,
      &list,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit({&layer, 1U, 5.0F, 5.0F});

  return expect(receipt.hit, "disabled hit") &&
         expect(!receipt.enabled, "disabled enabled false") &&
         expect(!receipt.consumed, "disabled not consumed") &&
         expect(receipt.action == iggy3d::FrontendAction::Settings,
                "disabled action copied") &&
         expect(receipt.semanticId == "disabled.row", "disabled semantic") &&
         expect(receipt.message == "ui_hit_disabled", "disabled message");
}

bool disabledEnabledOverlapRespectsBackToFront() {
  iggy3d::ProductUiDrawList disabledFront = drawListWithRegions({
      region("enabled.back", {0.0F, 0.0F, 100.0F, 100.0F}, true),
      region("disabled.front", {0.0F, 0.0F, 100.0F, 100.0F}, false),
  });
  iggy3d::ProductUiDrawList enabledFront = drawListWithRegions({
      region("disabled.back", {0.0F, 0.0F, 100.0F, 100.0F}, false),
      region("enabled.front", {0.0F, 0.0F, 100.0F, 100.0F}, true),
  });
  const iggy3d::ProductUiHitLayer disabledFrontLayer{
      iggy3d::ProductUiHitSurface::StarterMenu,
      &disabledFront,
      true,
  };
  const iggy3d::ProductUiHitLayer enabledFrontLayer{
      iggy3d::ProductUiHitSurface::StarterMenu,
      &enabledFront,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt disabledReceipt =
      iggy3d::routeProductUiHit({&disabledFrontLayer, 1U, 5.0F, 5.0F});
  const iggy3d::ProductUiHitRouteReceipt enabledReceipt =
      iggy3d::routeProductUiHit({&enabledFrontLayer, 1U, 5.0F, 5.0F});

  return expect(disabledReceipt.hit, "disabled front hit") &&
         expect(!disabledReceipt.consumed, "disabled front not consumed") &&
         expect(disabledReceipt.semanticId == "disabled.front",
                "disabled front semantic") &&
         expect(enabledReceipt.hit, "enabled front hit") &&
         expect(enabledReceipt.consumed, "enabled front consumed") &&
         expect(enabledReceipt.semanticId == "enabled.front",
                "enabled front semantic");
}

bool rectEdgesAreLeftTopInclusiveRightBottomExclusive() {
  iggy3d::ProductUiDrawList list = drawListWithRegions({
      region("edge.row", {10.0F, 20.0F, 30.0F, 40.0F}),
  });
  const iggy3d::ProductUiHitLayer layer{
      iggy3d::ProductUiHitSurface::Notebook,
      &list,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt leftTop =
      iggy3d::routeProductUiHit({&layer, 1U, 10.0F, 20.0F});
  const iggy3d::ProductUiHitRouteReceipt right =
      iggy3d::routeProductUiHit({&layer, 1U, 40.0F, 20.0F});
  const iggy3d::ProductUiHitRouteReceipt bottom =
      iggy3d::routeProductUiHit({&layer, 1U, 10.0F, 60.0F});

  return expect(leftTop.hit, "left top inclusive") &&
         expect(!right.hit, "right exclusive") &&
         expect(right.message == "ui_hit_miss", "right miss") &&
         expect(!bottom.hit, "bottom exclusive") &&
         expect(bottom.message == "ui_hit_miss", "bottom miss");
}

bool creativeRowHitRoutesAsCreativeOverlayRow() {
  const iggy3d::creative::CreativeUiModel model =
      iggy3d::creative::buildCreativeUiModel(
          iggy3d::creative::makeDefaultCreativeUiBuildRequest())
          .model;
  iggy3d::ProductCreativeUiDrawListRequest drawRequest;
  drawRequest.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductCreativeUiDrawList(drawRequest);
  const iggy3d::UiHitRegion& hit = list.hitRegions.front();
  const iggy3d::ProductUiHitLayer layer{
      iggy3d::ProductUiHitSurface::CreativeOverlay,
      &list,
      true,
  };
  const iggy3d::ProductUiHitRouteReceipt receipt =
      iggy3d::routeProductUiHit(
          {&layer, 1U, hit.rect.x, hit.rect.y});

  return expect(receipt.hit, "creative row hit") &&
         expect(receipt.consumed, "creative row consumed") &&
         expect(receipt.surface ==
                    iggy3d::ProductUiHitSurface::CreativeOverlay,
                "creative surface") &&
         expect(receipt.kind == iggy3d::UiHitKind::Row, "creative row kind") &&
         expect(receipt.action == iggy3d::FrontendAction::None,
                "creative action none") &&
         expect(receipt.semanticId == hit.semanticId,
                "creative semantic copied") &&
         expect(receipt.message == "ui_hit_consumed", "creative message");
}

}  // namespace

int main() {
  const bool ok = nullOrEmptyLayersMissClosed() &&
                  invalidLayersAreSkipped() &&
                  outsideAllRegionsMisses() &&
                  enabledRegionConsumesAndCopiesFields() &&
                  callerLayerOrderSetsPriority() &&
                  backToFrontRegionOrderWithinLayer() &&
                  disabledRegionReportsButDoesNotConsume() &&
                  disabledEnabledOverlapRespectsBackToFront() &&
                  rectEdgesAreLeftTopInclusiveRightBottomExclusive() &&
                  creativeRowHitRoutesAsCreativeOverlayRow();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
