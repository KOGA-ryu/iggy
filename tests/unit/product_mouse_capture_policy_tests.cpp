#include "app/iggy3d/window/MouseCapturePolicy.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
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
          "creative_document_gameplay_releases_for_editor_pointer",
          {
              true,
              iggy3d::ProductInteractionMode::Creative,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
              true,
              true,
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_creative_editor_pointer",
          "gameplay_released",
          "gameplay",
      },
      {
          // TV1-H (TD-8): Navigate re-engages relative capture for mouse-look.
          "creative_document_navigate_recaptures",
          {
              true,
              iggy3d::ProductInteractionMode::Creative,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
              true,
              true,   // creativeDocumentActive
              true,   // creativeNavigateActive
          },
          true,
          "mouse_capture_requested",
          "mouse_capture_creative_navigate_mouselook",
          "relative",
          "gameplay",
      },
      {
          // Select/Move/Measure keep the released free cursor: Navigate off but
          // still in the creative document.
          "creative_document_non_navigate_stays_released",
          {
              true,
              iggy3d::ProductInteractionMode::Creative,
              iggy3d::MenuOwner::Gameplay,
              false,
              true,
              true,
              true,    // creativeDocumentActive
              false,   // creativeNavigateActive
          },
          false,
          "mouse_capture_not_requested",
          "mouse_capture_creative_editor_pointer",
          "gameplay_released",
          "gameplay",
      },
      {
          "legacy_creative_gameplay_keeps_relative_capture",
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

iggy3d::ProductMouseCapturePolicy policyForFrontend(
    iggy3d::FrontendState frontend,
    iggy3d::MenuOwner owner) {
  frontend.inputOwned = false;
  return iggy3d::buildProductMouseCapturePolicy({
      true,
      iggy3d::ProductInteractionMode::Player,
      owner,
      iggy3d::frontendBlocksGameplayInput(frontend),
      true,
      true,
  });
}

bool frontendBlockedSurfacesReleaseMouseCapture() {
  iggy3d::FrontendState settings;
  settings.screen = iggy3d::FrontendScreen::Settings;
  settings.childScreen = iggy3d::FrontendScreen::Pause;
  const iggy3d::ProductMouseCapturePolicy settingsPolicy =
      policyForFrontend(settings, iggy3d::MenuOwner::Settings);

  iggy3d::FrontendState deleteConfirm;
  deleteConfirm.screen = iggy3d::FrontendScreen::DeleteConfirm;
  deleteConfirm.childScreen = iggy3d::FrontendScreen::Gameplay;
  const iggy3d::ProductMouseCapturePolicy deletePolicy =
      policyForFrontend(deleteConfirm, iggy3d::MenuOwner::Starter);

  iggy3d::FrontendState exitConfirm;
  exitConfirm.screen = iggy3d::FrontendScreen::ExitConfirm;
  exitConfirm.childScreen = iggy3d::FrontendScreen::Gameplay;
  const iggy3d::ProductMouseCapturePolicy exitPolicy =
      policyForFrontend(exitConfirm, iggy3d::MenuOwner::Starter);

  iggy3d::FrontendState gameplay;
  iggy3d::enterFrontendGameplay(gameplay, iggy3d::FrontendAction::CreateAndEnter);
  const iggy3d::ProductMouseCapturePolicy gameplayPolicy =
      policyForFrontend(gameplay, iggy3d::MenuOwner::Gameplay);

  return expect(!settingsPolicy.requested, "settings mouse capture released") &&
         expect(settingsPolicy.reasonCode == "mouse_capture_frontend_blocked",
                "settings mouse capture blocked reason") &&
         expect(!deletePolicy.requested, "delete confirm mouse capture released") &&
         expect(deletePolicy.reasonCode == "mouse_capture_frontend_blocked",
                "delete confirm mouse capture blocked reason") &&
         expect(!exitPolicy.requested, "exit confirm mouse capture released") &&
         expect(exitPolicy.reasonCode == "mouse_capture_frontend_blocked",
                "exit confirm mouse capture blocked reason") &&
         expect(gameplayPolicy.requested, "gameplay mouse capture requested") &&
         expect(gameplayPolicy.reasonCode == "mouse_capture_gameplay_mouselook",
                "gameplay mouse capture reason");
}

bool receiptCarriesMouseCaptureProof() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world = iggy3d::defaultProductWorldTemplate();
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppWindowState window;
  window.inputDevice.mouseCapture.requested = true;
  window.inputDevice.mouseCapture.active = true;
  window.inputDevice.mouseCapture.status = "mouse_capture_active";
  window.inputDevice.mouseCapture.reasonCode = "mouse_capture_active";
  window.inputDevice.mouseCapture.mode = "relative";
  window.inputDevice.mouseCapture.inputOwner = "gameplay";
  // TV1-H: the Navigate mirror is receipt-visible (fly active state).
  window.creativeAuthoring.creativeNavigateActive = true;

  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window,
                                     saves);

  return expect(iggy3d::hasReceiptField(receipt, "creative_navigate_active",
                                        "true"),
                "receipt creative navigate active") &&
         expect(iggy3d::hasReceiptField(receipt, "mouse_capture_requested",
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
  const bool ok = policyCases() && frontendBlockedSurfacesReleaseMouseCapture() &&
                  receiptCarriesMouseCaptureProof();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_mouse_capture_policy_tests=pass\n";
  return EXIT_SUCCESS;
}
