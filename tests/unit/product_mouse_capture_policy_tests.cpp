#include "app/iggy3d/window/MouseCapturePolicy.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "render/RenderDiagnostics.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

struct MouseCapturePolicyCase {
  const char* name = "";
  iggy3d::ProductMouseCapturePolicyRequest request;
  bool requested = false;
  const char* status = "mouse_capture_not_requested";
  const char* reasonCode = "mouse_capture_gameplay_inactive";
  const char* mode = "none";
  const char* inputOwner = "none";
};

bool policyCases() {
  const MouseCapturePolicyCase cases[] = {
      {
          "gameplay_player_capture",
          {
              true,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
          },
          true,
          "mouse_capture_requested",
          "mouse_capture_gameplay_mouselook",
          "relative",
          "gameplay",
      },
      {
          "inactive_gameplay_releases",
          {
              false,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_gameplay_inactive",
          "none",
          "gameplay",
      },
      {
          "pause_menu_releases",
          {
              true,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Pause,
              true,
              true,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_frontend_blocked",
          "gameplay_released",
          "pause",
      },
      {
          "starter_menu_releases",
          {
              true,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Starter,
              true,
              true,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_frontend_blocked",
          "gameplay_released",
          "starter",
      },
      {
          "settings_menu_releases",
          {
              true,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Settings,
              true,
              true,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_frontend_blocked",
          "gameplay_released",
          "settings",
      },
      {
          "editor_owner_releases",
          {
              true,
              iggy3d::ProductInteractionMode::Creative,
              iggy3d::MenuOwner::Editor,
              false,
              true,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_input_owner_blocked",
          "gameplay_released",
          "editor",
      },
      {
          "creative_gameplay_capture",
          {
              true,
              iggy3d::ProductInteractionMode::Creative,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
          },
          true,
          "mouse_capture_requested",
          "mouse_capture_gameplay_mouselook",
          "relative",
          "gameplay",
      },
      {
          "unfocused_window_releases",
          {
              true,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Gameplay,
              false,
              false,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_window_unfocused",
          "gameplay_released",
          "gameplay",
      },
      {
          "no_window_reports_no_capture",
          {
              true,
              iggy3d::ProductInteractionMode::Player,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
              false,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_no_window",
          "no_window",
          "gameplay",
      },
  };

  bool ok = true;
  for (const MouseCapturePolicyCase& testCase : cases) {
    const iggy3d::ProductMouseCapturePolicy policy =
        iggy3d::buildProductMouseCapturePolicy(testCase.request);
    const std::string requestedMessage = std::string(testCase.name) + " requested";
    const std::string statusMessage = std::string(testCase.name) + " status";
    const std::string reasonMessage = std::string(testCase.name) + " reason";
    const std::string modeMessage = std::string(testCase.name) + " mode";
    const std::string ownerMessage = std::string(testCase.name) + " owner";
    ok = expect(policy.requested == testCase.requested, requestedMessage.c_str()) &&
         ok;
    ok = expect(policy.status == testCase.status, statusMessage.c_str()) &&
         ok;
    ok = expect(policy.reasonCode == testCase.reasonCode, reasonMessage.c_str()) &&
         ok;
    ok = expect(policy.mode == testCase.mode, modeMessage.c_str()) &&
         ok;
    ok = expect(policy.inputOwner == testCase.inputOwner, ownerMessage.c_str()) &&
         ok;
  }
  return ok;
}

bool receiptCarriesMouseCaptureProof() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world = iggy3d::defaultProductWorldTemplate();
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppWindowState window;
  window.mouseCaptureRequested = true;
  window.mouseCaptureActive = true;
  window.mouseCaptureStatus = "mouse_capture_active";
  window.mouseCaptureReasonCode = "mouse_capture_active";
  window.mouseCaptureMode = "relative";
  window.mouseCaptureInputOwner = "gameplay";

  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window,
                                     saves);

  return expect(iggy3d::hasReceiptField(receipt, "mouse_capture_requested",
                                        "true"),
                "receipt mouse capture requested") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_active",
                                        "true"),
                "receipt mouse capture active") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_status",
                                        "mouse_capture_active"),
                "receipt mouse capture status") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_reason_code",
                                        "mouse_capture_active"),
                "receipt mouse capture reason") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_mode",
                                        "relative"),
                "receipt mouse capture mode") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_input_owner",
                                        "gameplay"),
                "receipt mouse capture owner");
}

}  // namespace

int main() {
  const bool ok = policyCases() && receiptCarriesMouseCaptureProof();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_mouse_capture_policy_tests=pass\n";
  return EXIT_SUCCESS;
}
