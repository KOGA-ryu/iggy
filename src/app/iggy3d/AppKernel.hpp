#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"        // FrontendState, FrontendSettings
#include "app/frontend/WorldSetupModel.hpp"       // WorldSetupDraft
#include "app/iggy3d/Options.hpp"                  // ProductAppOptions
#include "app/iggy3d/ReceiptBuilder.hpp"           // ProductAppWindowState
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"          // ProductSaveBridgeResult
#include "runtime/session/Session.hpp"

namespace iggy3d {

// The app-layer runtime coordinator. It owns the app-lifetime state that was, until now, a bundle of
// loose locals in runProductApp threaded by reference into every phase -- which is exactly why it is
// the home into which per-domain Systems get extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md). L0: owns the state + the run() lifecycle; behavior is identical
// to the previous free-function flow. It COORDINATES; it does not implement domain logic. Each member
// below is a future extraction target for an owned System, shrinking `window` toward deletion.
class AppKernel {
 public:
  // Startup -> orchestration -> finalize. Returns the process exit code.
  [[nodiscard]] int run(const ProductAppOptions& options);

  ProductAppWindowState window;
  std::optional<Session> activeSession;
  creative::CreativeAppState creativeApp;
  FrontendState frontend;
  ProductSaveBridgeResult saves;
  FrontendSettings settings;
  WorldSetupDraft worldSetupDraft;
};

}  // namespace iggy3d
