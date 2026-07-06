#include "app/iggy3d/creative/bridge/UiWindowFrame.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/window/Loop.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::RenderReceipt receiptFor(const iggy3d::ProductAppWindowState& window) {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  return iggy3d::buildProductAppReceipt(options,
                                        world,
                                        frontend,
                                        settings,
                                        window,
                                        saves);
}

bool expectReceiptField(const iggy3d::RenderReceipt& receipt,
                        std::string_view key,
                        std::string_view value,
                        std::string_view message) {
  return expect(iggy3d::hasReceiptField(receipt, key, value), message);
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

void prepopulateProductVulkanMenuUi(iggy3d::ProductAppWindowState& window) {
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiPartial = true;
  window.productVulkanMenuUiStatus = "preexisting_ui_status";
  window.productVulkanMenuUiReasonCode = "preexisting_ui_reason";
  window.productVulkanMenuUiPrimitiveCount = 101;
  window.productVulkanMenuUiTextCount = 102;
  window.productVulkanMenuUiRectCount = 103;
  window.productVulkanMenuUiRowCount = 104;
  window.productVulkanMenuUiSelectedAction = "preexisting_action";
}

iggy3d::ProductUiDrawList readyCreativeDrawList() {
  iggy3d::ProductUiDrawList drawList;
  drawList.ready = true;
  return drawList;
}

iggy3d::ProductAppWindowState creativeWindow() {
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreative.saveId = "creative_save";
  window.activeCreative.worldId = "world_001";
  window.activeCreative.documentId = 42U;
  return window;
}

bool overlayInputAvailabilityAllowsRenderableReadyDrawList() {
  const iggy3d::ProductUiDrawList drawList = readyCreativeDrawList();
  const iggy3d::ProductCreativeUiOverlayInputAvailability availability =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &drawList,
              true,
              true,
              1280,
              720,
              true});

  return expect(availability.requested, "availability requested") &&
         expect(availability.drawListAvailable, "availability draw list") &&
         expect(availability.drawListReady, "availability ready") &&
         expect(availability.rendererCanRenderCreativeOverlay,
                "availability renderer") &&
         expect(availability.windowDrawable, "availability window drawable") &&
         expect(availability.drawableAvailable,
                "availability drawable extent") &&
         expect(availability.gameplayOverlayRenderable,
                "availability gameplay overlay") &&
         expect(availability.inputAvailable, "availability input") &&
         expect(availability.status ==
                    "product_creative_ui_overlay_input_available",
                "availability status");
}

bool overlayInputAvailabilityRejectsUnrenderableRenderer() {
  const iggy3d::ProductUiDrawList drawList = readyCreativeDrawList();
  const iggy3d::ProductCreativeUiOverlayInputAvailability availability =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &drawList,
              false,
              true,
              1280,
              720,
              true});

  return expect(availability.requested, "renderer requested") &&
         expect(availability.drawListAvailable, "renderer draw available") &&
         expect(availability.drawListReady, "renderer draw ready") &&
         expect(!availability.inputAvailable, "renderer input unavailable") &&
         expect(availability.status ==
                    "product_creative_ui_overlay_input_renderer_unavailable",
                "renderer status");
}

bool overlayInputAvailabilityStatusesAreStable() {
  const iggy3d::ProductUiDrawList readyDrawList = readyCreativeDrawList();
  iggy3d::ProductUiDrawList notReadyDrawList;
  notReadyDrawList.ready = false;

  const iggy3d::ProductCreativeUiOverlayInputAvailability missing =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability({});
  const iggy3d::ProductCreativeUiOverlayInputAvailability notReady =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &notReadyDrawList,
              true,
              true,
              1280,
              720,
              true});
  const iggy3d::ProductCreativeUiOverlayInputAvailability notDrawable =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &readyDrawList,
              true,
              false,
              1280,
              720,
              true});
  const iggy3d::ProductCreativeUiOverlayInputAvailability noExtent =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &readyDrawList,
              true,
              true,
              0,
              720,
              true});
  const iggy3d::ProductCreativeUiOverlayInputAvailability noGameplay =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &readyDrawList,
              true,
              true,
              1280,
              720,
              false});

  return expect(missing.status ==
                    "product_creative_ui_overlay_input_draw_list_missing",
                "missing status") &&
         expect(notReady.status ==
                    "product_creative_ui_overlay_input_draw_list_not_ready",
                "not ready status") &&
         expect(notDrawable.status ==
                    "product_creative_ui_overlay_input_window_not_drawable",
                "not drawable status") &&
         expect(noExtent.status ==
                    "product_creative_ui_overlay_input_drawable_unavailable",
                "no extent status") &&
         expect(noGameplay.status ==
                    "product_creative_ui_overlay_input_gameplay_unavailable",
                "no gameplay status") &&
         expect(!missing.inputAvailable, "missing unavailable") &&
         expect(!notReady.inputAvailable, "not ready unavailable") &&
         expect(!notDrawable.inputAvailable, "not drawable unavailable") &&
         expect(!noExtent.inputAvailable, "no extent unavailable") &&
         expect(!noGameplay.inputAvailable, "no gameplay unavailable");
}

bool logicalDimensionsAreUsedWhenDrawableIsHighDpi() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                      &app,
                                                      2560,
                                                      1440,
                                                      640,
                                                      360,
                                                      iggy3d::ProductUiThemeId::System,
                                                      1280,
                                                      720});

  return expect(frame.receipt.ready, "logical frame ready") &&
         expect(frame.projection.drawList.virtualWidth == 1280U,
                "logical draw list width") &&
         expect(frame.projection.drawList.virtualHeight == 720U,
                "logical draw list height") &&
         expect(window.creativeUiProjection.virtualWidth == 1280U,
                "logical width used") &&
         expect(window.creativeUiProjection.virtualHeight == 720U,
                "logical height used");
}

bool fallbackDimensionsAreUsedWhenDrawableZero() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                      &app,
                                                      0,
                                                      0,
                                                      1366,
                                                      768,
                                                      iggy3d::ProductUiThemeId::Journal});

  return expect(frame.receipt.ready, "fallback frame ready") &&
         expect(window.creativeUiProjection.virtualWidth == 1366U,
                "fallback width used") &&
         expect(window.creativeUiProjection.virtualHeight == 768U,
                "fallback height used") &&
         expect(window.creativeUiProjection.theme == "journal",
                "fallback theme copied");
}

bool guardDimensionsAreUsedWhenDrawableAndFallbackZero() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                      &app,
                                                      0,
                                                      0,
                                                      0,
                                                      0,
                                                      iggy3d::ProductUiThemeId::System});

  return expect(frame.receipt.ready, "guard frame ready") &&
         expect(window.creativeUiProjection.virtualWidth == 1280U,
                "guard width used") &&
         expect(window.creativeUiProjection.virtualHeight == 720U,
                "guard height used");
}

bool inactiveWindowRecordsInactiveProjection() {
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                      &app,
                                                      1280,
                                                      720,
                                                      1280,
                                                      720,
                                                      iggy3d::ProductUiThemeId::System});

  return expect(!frame.receipt.active, "inactive not active") &&
         expect(!frame.receipt.projected, "inactive not projected") &&
         expect(frame.receipt.recorded, "inactive recorded") &&
         expect(frame.receipt.status == "product_creative_ui_frame_inactive",
                "inactive status") &&
         expect(!window.creativeUiProjection.requested,
                "inactive requested false") &&
         expect(!window.creativeUiProjection.ready, "inactive ready false") &&
         expect(window.creativeUiProjection.status ==
                    "product_creative_ui_frame_inactive",
                "inactive window status") &&
         expect(window.creativeUiProjection.primitiveCount == 0U,
                "inactive primitive count zero") &&
         expect(window.creativeUiProjection.textCount == 0U,
                "inactive text count zero") &&
         expect(window.creativeUiProjection.rectCount == 0U,
                "inactive rect count zero") &&
         expect(window.creativeUiProjection.rowCount == 0U,
                "inactive row count zero") &&
         expect(window.creativeUiProjection.hitRegionCount == 0U,
                "inactive hit count zero");
}

bool creativeWindowWithFacadeRecordsReadyProjection() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                      &app,
                                                      1280,
                                                      720,
                                                      1280,
                                                      720,
                                                      iggy3d::ProductUiThemeId::System});

  return expect(frame.receipt.requested, "ready requested") &&
         expect(frame.receipt.active, "ready active") &&
         expect(frame.receipt.projected, "ready projected") &&
         expect(frame.receipt.recorded, "ready recorded") &&
         expect(frame.receipt.ready, "ready frame ready") &&
         expect(window.creativeUiProjection.ready, "window ready") &&
         expect(window.creativeUiProjection.usedFacade, "window used facade") &&
         expect(!window.creativeUiProjection.usedModel, "window model unused") &&
         expect(window.creativeUiProjection.panelCount > 0U,
                "panel count nonzero") &&
         expect(window.creativeUiProjection.modelRowCount > 0U,
                "model row count nonzero") &&
         expect(window.creativeUiProjection.primitiveCount > 0U,
                "primitive count nonzero") &&
         expect(window.creativeUiProjection.textCount > 0U,
                "text count nonzero") &&
         expect(window.creativeUiProjection.rectCount > 0U,
                "rect count nonzero") &&
         expect(window.creativeUiProjection.rowCount > 0U,
                "row count nonzero") &&
         expect(window.creativeUiProjection.hitRegionCount > 0U,
                "hit count nonzero") &&
         expect(window.creativeUiProjection.hitRegionCount ==
                    window.creativeUiProjection.rowCount,
                "hit count mirrors row count");
}

bool creativeWindowWithNullFacadeRecordsFacadeMissing() {
  iggy3d::ProductAppWindowState window = creativeWindow();

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                      nullptr,
                                                      1280,
                                                      720,
                                                      1280,
                                                      720,
                                                      iggy3d::ProductUiThemeId::System});

  return expect(frame.receipt.requested, "missing requested") &&
         expect(frame.receipt.active, "missing active") &&
         expect(!frame.receipt.projected, "missing not projected") &&
         expect(frame.receipt.recorded, "missing recorded") &&
         expect(!frame.receipt.ready, "missing not ready") &&
         expect(frame.receipt.status ==
                    "product_creative_ui_frame_facade_missing",
                "missing frame status") &&
         expect(window.creativeUiProjection.status ==
                    "product_creative_ui_frame_facade_missing",
                "missing window status") &&
         expect(window.creativeUiProjection.primitiveCount == 0U,
                "missing primitive count zero");
}

bool productVulkanMenuUiFieldsAreUnchanged() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  prepopulateProductVulkanMenuUi(window);
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  static_cast<void>(iggy3d::buildProductCreativeUiWindowFrame(
      iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                  &app,
                                                  1280,
                                                  720,
                                                  1280,
                                                  720,
                                                  iggy3d::ProductUiThemeId::System}));

  return expect(window.productVulkanMenuUiReady,
                "vulkan ui ready unchanged") &&
         expect(window.productVulkanMenuUiPartial,
                "vulkan ui partial unchanged") &&
         expect(window.productVulkanMenuUiStatus == "preexisting_ui_status",
                "vulkan ui status unchanged") &&
         expect(window.productVulkanMenuUiReasonCode ==
                    "preexisting_ui_reason",
                "vulkan ui reason unchanged") &&
         expect(window.productVulkanMenuUiPrimitiveCount == 101U,
                "vulkan ui primitive count unchanged") &&
         expect(window.productVulkanMenuUiTextCount == 102U,
                "vulkan ui text count unchanged") &&
         expect(window.productVulkanMenuUiRectCount == 103U,
                "vulkan ui rect count unchanged") &&
         expect(window.productVulkanMenuUiRowCount == 104U,
                "vulkan ui row count unchanged") &&
         expect(window.productVulkanMenuUiSelectedAction ==
                    "preexisting_action",
                "vulkan ui action unchanged");
}

bool facadeStateIsNotMutatedByBridge() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);

  const cr::Tool activeToolBefore = facade.toolState().activeTool;
  const cr::TargetRef selectedBefore =
      facade.selectionState().selectedTarget;

  static_cast<void>(iggy3d::buildProductCreativeUiWindowFrame(
      iggy3d::ProductCreativeUiWindowFrameRequest{&window,
                                                  &app,
                                                  1280,
                                                  720,
                                                  1280,
                                                  720,
                                                  iggy3d::ProductUiThemeId::System}));

  return expect(facade.toolState().activeTool == activeToolBefore,
                "facade active tool unchanged") &&
         expect(facade.selectionState().selectedTarget.value ==
                    selectedBefore.value,
                "facade selected target unchanged");
}

bool noWindowLoopDoesNotCallBridge() {
  iggy3d::ProductAppOptions options;
  options.windowMode = iggy3d::ProductWindowMode::NoWindow;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window = creativeWindow();
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductWindowLoopResult loopResult =
      iggy3d::runProductWindowLoop(iggy3d::ProductWindowLoopRequest{
          options,
          world,
          frontend,
          activeSession,
          worldSetupDraft,
          window,
          settings,
          saves,
          &app});
  const iggy3d::RenderReceipt receipt = receiptFor(loopResult.window);

  return expect(!loopResult.window.requested, "no-window not requested") &&
         expect(loopResult.window.creativeUiProjection.status ==
                    "creative_ui_projection_not_requested",
                "no-window bridge not called") &&
         expect(!loopResult.window.creativeUiProjection.requested,
                "no-window creative requested false") &&
         expect(loopResult.window.creativeUiProjection.primitiveCount == 0U,
                "no-window primitive count zero") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "creative_ui_projection_not_requested",
                            "no-window receipt not requested");
}

}  // namespace

int main() {
  const bool ok = overlayInputAvailabilityAllowsRenderableReadyDrawList() &&
                  overlayInputAvailabilityRejectsUnrenderableRenderer() &&
                  overlayInputAvailabilityStatusesAreStable() &&
                  logicalDimensionsAreUsedWhenDrawableIsHighDpi() &&
                  fallbackDimensionsAreUsedWhenDrawableZero() &&
                  guardDimensionsAreUsedWhenDrawableAndFallbackZero() &&
                  inactiveWindowRecordsInactiveProjection() &&
                  creativeWindowWithFacadeRecordsReadyProjection() &&
                  creativeWindowWithNullFacadeRecordsFacadeMissing() &&
                  productVulkanMenuUiFieldsAreUnchanged() &&
                  facadeStateIsNotMutatedByBridge() &&
                  noWindowLoopDoesNotCallBridge();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
