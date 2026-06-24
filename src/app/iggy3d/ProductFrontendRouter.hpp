#pragma once

#include <cstdint>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/MenuInput.hpp"

namespace iggy3d {

enum class ProductFrontendSurface : std::uint8_t {
  None,
  BootStatus,
  ConfirmDialog,
  SaveSelector,
  WorldSetup,
  Settings,
  DevTools,
  Pause,
  Starter,
  Gameplay,
  Editor,
};

struct ProductFrontendRouteContext {
  FrontendState frontend;
  bool gameplayActive = false;
  bool hasActiveSession = false;
};

struct ProductFrontendOwnerDecision {
  MenuOwner inputOwner = MenuOwner::None;
  ProductFrontendSurface activeSurface = ProductFrontendSurface::None;
  MenuOwner parentOwner = MenuOwner::None;
  bool gameplayInputSuppressed = false;
  bool modelAvailable = true;
  std::string_view modelName = "none";
  std::string_view status = "product_frontend_owner_ready";
};

std::string_view productFrontendSurfaceName(ProductFrontendSurface surface);

ProductFrontendOwnerDecision chooseProductFrontendOwner(
    const ProductFrontendRouteContext& context);

}  // namespace iggy3d
