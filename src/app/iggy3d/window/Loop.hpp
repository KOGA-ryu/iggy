#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

namespace creative {
struct CreativeAppState;
}  // namespace creative

struct ProductWindowLoopRequest {
  const ProductAppOptions& options;
  const ProductWorldTemplate& world;
  FrontendState& frontend;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState window;
  FrontendSettings& settings;
  const ProductSaveBridgeResult& saves;
  creative::CreativeAppState* creativeApp = nullptr;
};

// The loop returns BOTH the final window state and the final save catalog: the loop-local
// catalog absorbs in-window mutations (soft-delete, new-world) that the caller's pre-loop scan
// cannot see, so the receipt must be built from THIS returned catalog, not a second scan.
struct ProductWindowLoopResult {
  ProductAppWindowState window;
  ProductSaveBridgeResult saves;
};

ProductWindowLoopResult runProductWindowLoop(const ProductWindowLoopRequest& request);

}  // namespace iggy3d
