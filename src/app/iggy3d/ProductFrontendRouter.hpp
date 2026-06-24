#pragma once

#include <cstdint>
#include <string_view>

#include "app/frontend/FrontendRoute.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/MenuInput.hpp"

namespace iggy3d {

struct StarterScreenModel;
struct SettingsRouteContext;
struct PauseMenuModel;
struct DevToolsMenuModel;
struct SaveBrowserModel;

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
  const StarterScreenModel* starterModel = nullptr;
  const SettingsRouteContext* settingsContext = nullptr;
  const PauseMenuModel* pauseModel = nullptr;
  const DevToolsMenuModel* devToolsModel = nullptr;
  const SaveBrowserModel* saveBrowserModel = nullptr;
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

struct ProductFrontendRouteFrame {
  ProductFrontendOwnerDecision owner;
  FrontendRouteResult route;
  bool routed = false;
  bool routeModelAvailable = true;
  std::string_view routeModelName = "none";
};

struct ProductFrontendRouteSummary {
  bool routed = false;
  bool accepted = false;
  std::string_view inputOwner = "none";
  std::string_view activeSurface = "none";
  std::string_view parentOwner = "none";
  std::string_view inputAction = "none";
  std::string_view screenBefore = "boot";
  std::string_view childBefore = "gameplay";
  std::string_view screenAfter = "boot";
  std::string_view childAfter = "gameplay";
  std::string_view transition = "none";
  bool closeRequested = false;
  bool gameplayInputSuppressed = false;
  bool ownerModelAvailable = true;
  std::string_view ownerModelName = "none";
  bool routeModelAvailable = true;
  std::string_view routeModelName = "none";
  std::string_view status = "frontend_route_ignored";
  std::string_view reason = "frontend_route_ignored";
};

std::string_view productFrontendSurfaceName(ProductFrontendSurface surface);

ProductFrontendOwnerDecision chooseProductFrontendOwner(
    const ProductFrontendRouteContext& context);

ProductFrontendRouteFrame routeProductFrontendAction(
    const ProductFrontendRouteContext& context,
    FrontendAction action);

ProductFrontendRouteSummary summarizeProductFrontendRoute(
    const ProductFrontendRouteContext& context,
    const ProductFrontendRouteFrame& frame,
    FrontendAction inputAction);

}  // namespace iggy3d
