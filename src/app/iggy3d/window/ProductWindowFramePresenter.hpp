#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/gameplay/ProductGameplayProjectionRefresh.hpp"
#include "app/iggy3d/window/ProductWindowRendererLifecycle.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d {

struct ProductWindowFramePresenterRequest {
  const ProductAppOptions& options;
  const ProductWorldTemplate& world;
  const FrontendState& frontend;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::None;
  const WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  const ProductSaveBridgeResult& saves;
  SdlWindow& sdlWindow;
  ProductWindowRendererState& renderer;
  const ProductGameplayProjectionFrame& projectionFrame;
};

void presentProductWindowFrame(ProductWindowFramePresenterRequest request);

}  // namespace iggy3d
