#include "app/iggy3d/window/RendererLifecycle.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>

#include "app/input/InputAction.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/window/Loop.hpp"
#include "render/RenderDiagnostics.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAppWindowState requestedWindow() {
  iggy3d::ProductAppWindowState window;
  window.productVulkanRenderer.requested = true;
  return window;
}

iggy3d::FrontendState starterFrontend(iggy3d::FrontendAction selected) {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Starter;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  frontend.selectedAction = selected;
  frontend.status = "starter_screen_ready";
  return frontend;
}

bool notRequestedReportsStableBlocker() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  return expect(!readiness.ready, "not requested not ready") &&
         expect(readiness.backendBuilt == iggy3d::productWindowVulkanBackendBuilt(),
                "backend built reflects compile configuration") &&
         expect(readiness.status == "product_vulkan_not_requested",
                "not requested status") &&
         expect(readiness.reasonCode == "product_vulkan_not_requested",
                "not requested reason");
}

bool unavailableRendererReportsRuntimeBlockerWhenBackendBuilt() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanReasonCode = "renderer_unavailable";
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(!readiness.ready, "backend-unavailable build not ready") &&
           expect(readiness.status == "product_vulkan_backend_unavailable",
                  "backend-unavailable build status") &&
           expect(readiness.reasonCode == "product_vulkan_backend_unavailable",
                  "backend-unavailable build reason");
  }
  return expect(!readiness.ready, "unavailable renderer not ready") &&
         expect(readiness.status == "product_vulkan_renderer_unavailable",
                "unavailable renderer status") &&
         expect(readiness.reasonCode == "renderer_unavailable",
                "unavailable renderer reason");
}

bool cpuMeshReadinessIsRequiredBeforeGameplayReady() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRenderer.created = true;
  window.productVulkanRenderer.ready = true;
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(readiness.status == "product_vulkan_backend_unavailable",
                  "cpu-ready gate blocked by unavailable backend");
  }
  return expect(!readiness.ready, "cpu mesh missing not ready") &&
         expect(readiness.status == "product_vulkan_room_mesh_cpu_not_ready",
                "cpu mesh missing status") &&
         expect(readiness.reasonCode == "product_vulkan_room_mesh_cpu_not_ready",
                "cpu mesh missing reason");
}

bool frameSubmitIsRequiredBeforeGameplayReady() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRenderer.created = true;
  window.productVulkanRenderer.ready = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanReasonCode = "product_vulkan_waiting_for_gameplay_room";
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(readiness.status == "product_vulkan_backend_unavailable",
                  "frame-submit gate blocked by unavailable backend");
  }
  return expect(!readiness.ready, "frame missing not ready") &&
         expect(readiness.status == "product_vulkan_frame_not_submitted",
                "frame missing status") &&
         expect(readiness.reasonCode == "product_vulkan_waiting_for_gameplay_room",
                "frame missing reason");
}

bool roomMeshBackendPresentationIsRequiredBeforeGameplayReady() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRenderer.created = true;
  window.productVulkanRenderer.ready = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanRenderingPath = "diagnostic";
  window.productVulkanRecordMode = "none";
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(readiness.status == "product_vulkan_backend_unavailable",
                  "room-mesh gate blocked by unavailable backend");
  }
  return expect(!readiness.ready, "room mesh backend missing not ready") &&
         expect(readiness.status == "product_vulkan_room_mesh_not_presented",
                "room mesh backend missing status") &&
         expect(readiness.reasonCode == "product_vulkan_room_mesh_not_presented",
                "room mesh backend missing reason");
}

bool roomMeshFramePathReportsGameplayReadyWhenBackendBuilt() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRenderer.created = true;
  window.productVulkanRenderer.ready = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanRenderingPath = "package_room_meshes";
  window.productVulkanRecordMode = "room_mesh_draws";
  window.viewport.productVulkanRoomMeshBackendPresented = true;
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(!readiness.ready, "ready case blocked when backend absent") &&
           expect(readiness.status == "product_vulkan_backend_unavailable",
                  "ready case backend absent status");
  }
  return expect(readiness.ready, "room mesh frame path ready") &&
         expect(readiness.status == "product_vulkan_gameplay_ready",
                "gameplay ready status") &&
         expect(readiness.reasonCode == "product_vulkan_gameplay_ready",
                "gameplay ready reason");
}

bool starterMenuUiDrawListStatusIsStable() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  const iggy3d::ProductUiDrawList drawList =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  iggy3d::recordProductVulkanMenuUiDrawList(window, "starter", drawList);
  return expect(window.productVulkanMenu.requested, "menu requested") &&
         expect(window.productVulkanMenu.visible, "menu visible") &&
         expect(window.productVulkanMenu.status == "product_vulkan_menu_ui_ready",
                "menu ui ready status") &&
         expect(window.productVulkanMenu.reasonCode == "product_ui_draw_list_ready",
                "menu ui ready reason") &&
         expect(window.productVulkanMenu.surface == "starter",
                "menu ui surface") &&
         expect(window.productVulkanMenu.uiReady, "menu ui ready") &&
         expect(!window.productVulkanMenu.uiPartial, "menu ui not partial") &&
         expect(window.productVulkanMenu.uiStatus == "product_ui_draw_list_ready",
                "menu ui draw list status") &&
         expect(window.productVulkanMenu.uiPrimitiveCount == 26U,
                "menu ui primitive count") &&
         expect(window.productVulkanMenu.uiTextCount == 13U,
                "menu ui text count") &&
         expect(window.productVulkanMenu.uiRectCount == 13U,
                "menu ui rect count") &&
         expect(window.productVulkanMenu.uiRowCount == 9U,
                "menu ui row count") &&
         expect(window.productVulkanMenu.uiSelectedAction == "new_world",
                "menu ui selected action");
}

bool starterMenuSubmitMarksFramePresented() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  const iggy3d::ProductUiDrawList drawList =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  iggy3d::recordProductVulkanMenuUiDrawList(window, "starter", drawList);

  iggy3d::RenderSubmitResult submit;
  submit.outcome = iggy3d::RenderOutcome::Ok;
  submit.reason = {"product_menu_ui_presented", "product menu ui presented"};
  iggy3d::appendReceiptField(submit.receipt, "rendering_path", "product_menu_ui");
  iggy3d::appendReceiptField(submit.receipt, "record_mode", "ui_primitives");
  iggy3d::appendReceiptField(submit.receipt, "reason_code", "product_menu_ui_presented");
  iggy3d::recordProductVulkanSubmit(window, submit);

  return expect(window.productVulkanFrameSubmitted, "menu frame submitted") &&
         expect(window.productVulkanRenderingPath == "product_menu_ui",
                "menu rendering path") &&
         expect(window.productVulkanRecordMode == "ui_primitives",
                "menu record mode") &&
         expect(window.productVulkanMenu.visible, "menu visible after submit") &&
         expect(window.productVulkanMenu.status ==
                    "product_vulkan_menu_frame_submitted",
                "menu submitted status") &&
         expect(window.productVulkanMenu.reasonCode == "product_menu_ui_presented",
                "menu submitted reason");
}

bool productReceiptCarriesReadinessFields() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRenderer.created = true;
  window.productVulkanRenderer.ready = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanRenderingPath = "package_room_meshes";
  window.productVulkanRecordMode = "room_mesh_draws";
  window.viewport.productVulkanRoomMeshBackendPresented = true;
  const iggy3d::FrontendState menuFrontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  const iggy3d::ProductUiDrawList menuDrawList =
      iggy3d::buildProductStarterUiDrawList({&menuFrontend, 0U, 1280U, 720U});
  iggy3d::recordProductVulkanMenuUiDrawList(window, "starter", menuDrawList);
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);

  const bool expectedReady = iggy3d::productWindowVulkanBackendBuilt();
  return expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_backend_built",
                    expectedReady ? "true" : "false"),
                "receipt includes backend built") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_gameplay_ready",
                    expectedReady ? "true" : "false"),
                "receipt includes gameplay readiness") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_gameplay_status",
                    expectedReady ? "product_vulkan_gameplay_ready"
                                  : "product_vulkan_backend_unavailable"),
                "receipt includes gameplay readiness status") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_gameplay_reason_code",
                    expectedReady ? "product_vulkan_gameplay_ready"
                                  : "product_vulkan_backend_unavailable"),
                "receipt includes gameplay readiness reason") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_requested",
                                        "true"),
                "receipt includes menu requested") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_visible",
                                        "true"),
                "receipt includes menu visible true") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_menu_status",
                    "product_vulkan_menu_ui_ready"),
                "receipt includes menu ui ready status") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_menu_reason_code",
                    "product_ui_draw_list_ready"),
                "receipt includes menu ui ready reason") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_surface",
                                        "starter"),
                "receipt includes menu surface") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_ready",
                                        "true"),
                "receipt includes menu ui ready") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_status",
                                        "product_ui_draw_list_ready"),
                "receipt includes menu ui status") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_primitive_count",
                                        "26"),
                "receipt includes menu ui primitive count") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_text_count",
                                        "13"),
                "receipt includes menu ui text count") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_rect_count",
                                        "13"),
                "receipt includes menu ui rect count") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_row_count",
                                        "9"),
                "receipt includes menu ui row count") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "product_vulkan_menu_ui_selected_action",
                                        "new_world"),
                "receipt includes menu ui selected action");
}

bool noWindowGameplayReportsMouseCaptureNotApplied() {
  iggy3d::ProductAppOptions options;
  options.windowMode = iggy3d::ProductWindowMode::NoWindow;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Gameplay;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  saves.slots.compatibleCount = 7;  // sentinel: proves the returned catalog is THIS one

  const iggy3d::ProductWindowLoopResult loopResult =
      iggy3d::runProductWindowLoop(iggy3d::ProductWindowLoopRequest{
          options, world, frontend, activeSession, worldSetupDraft, window,
          settings, saves});
  const iggy3d::ProductAppWindowState& result = loopResult.window;
  // Seam (a): the loop hands its catalog back and the receipt is built from THAT catalog. The
  // no-window path never mutates it, so it flows through unchanged (byte-identical receipt).
  const iggy3d::RenderReceipt receipt = iggy3d::buildProductAppReceipt(
      options, world, frontend, settings, result, loopResult.saves);

  return expect(loopResult.saves.slots.compatibleCount == 7,
                "loop returns its final save catalog to the caller") &&
         expect(!result.requested, "no-window not requested") &&
         expect(!result.created, "no-window not created") &&
         expect(!result.inputDevice.mouseCapture.requested,
                "no-window mouse capture not requested") &&
         expect(!result.inputDevice.mouseCapture.active,
                "no-window mouse capture not active") &&
         expect(result.inputDevice.mouseCapture.status == "mouse_capture_not_requested",
                "no-window mouse capture status") &&
         expect(result.inputDevice.mouseCapture.reasonCode == "mouse_capture_no_window",
                "no-window mouse capture reason") &&
         expect(result.inputDevice.mouseCapture.mode == "no_window",
                "no-window mouse capture mode") &&
         expect(result.inputDevice.mouseCapture.inputOwner == "gameplay",
                "no-window mouse capture owner") &&
         expect(iggy3d::hasReceiptField(receipt, "settings_debug_overlay_enabled",
                                        "false"),
                "receipt records debug overlay setting") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_requested",
                                        "false"),
                "no-window receipt capture requested") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_active",
                                        "false"),
                "no-window receipt capture active") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_reason_code",
                                        "mouse_capture_no_window"),
                "no-window receipt capture reason") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_mode",
                                        "no_window"),
                "no-window receipt capture mode") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_input_owner",
                                        "gameplay"),
                "no-window receipt capture owner");
}

// Seam (b): the in-window new-world action durably writes the initial save AND refreshes the
// loop-local catalog (sd1). Drive the handler headlessly against a real temp save root and prove
// the context catalog now reflects the new durable save. Removing the refresh line in
// ActionHandlers.cpp turns this RED.
bool inWindowNewWorldRefreshesCatalog() {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path saveRoot = fs::temp_directory_path() / "iggy3d_sd1_new_world_refresh";
  fs::remove_all(saveRoot, ec);
  fs::create_directories(saveRoot, ec);

  iggy3d::ProductAppOptions options;
  options.saveRoot = saveRoot;
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.worldSetup.dungeonDraftEditMode = false;  // MenuConfirm launches the world (not paint)
  iggy3d::ProductSaveBridgeResult saves;          // empty catalog before the new world

  const bool emptyBefore = saves.slots.compatibleCount == 0;

  iggy3d::ProductNewWorldMenuActionContext context{
      frontend, options, saves, activeSession, worldSetupDraft, window};
  (void)iggy3d::applyProductNewWorldMenuAction(iggy3d::InputAction::MenuConfirm, context);

  const bool refreshed = saves.slots.compatibleCount > 0;  // durable save is now visible
  fs::remove_all(saveRoot, ec);

  return expect(emptyBefore, "catalog empty before in-window new-world") &&
         expect(refreshed,
                "in-window new-world refreshes the loop-local catalog (compatible slot appeared)");
}

}  // namespace

int main() {
  const bool ok =
      notRequestedReportsStableBlocker() &&
      unavailableRendererReportsRuntimeBlockerWhenBackendBuilt() &&
      cpuMeshReadinessIsRequiredBeforeGameplayReady() &&
      frameSubmitIsRequiredBeforeGameplayReady() &&
      roomMeshBackendPresentationIsRequiredBeforeGameplayReady() &&
      roomMeshFramePathReportsGameplayReadyWhenBackendBuilt() &&
      starterMenuUiDrawListStatusIsStable() &&
      starterMenuSubmitMarksFramePresented() &&
      productReceiptCarriesReadinessFields() &&
      noWindowGameplayReportsMouseCaptureNotApplied() &&
      inWindowNewWorldRefreshesCatalog();
  if (!ok) {
    return 1;
  }
  std::cout << "product_window_renderer_lifecycle_tests=pass\n";
  return 0;
}
