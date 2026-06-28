#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductWindowLoopRequest {
  const ProductAppOptions& options;
  const ProductWorldTemplate& world;
  FrontendState& frontend;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState window;
  FrontendSettings& settings;
  const ProductSaveBridgeResult& saves;
};

ProductAppWindowState runProductWindowLoop(const ProductWindowLoopRequest& request);

}  // namespace iggy3d
