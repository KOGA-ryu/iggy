#pragma once

#include <cstdint>
#include <string_view>

#include "app/frontend/FrontendRoute.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {

struct ProductAppWindowState;
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

enum class ProductActiveMouseCapturePolicy : std::uint8_t {
  Released,
  RelativeGameplay,
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
  const WorldSetupDraft* worldSetupDraft = nullptr;
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

struct ProductActiveSurfaceContext {
  FrontendState frontend;
  bool gameplayActive = false;
  bool hasActiveSession = false;
  bool roomEditorReady = false;
};

struct ProductActiveSurfaceFrame {
  ProductFrontendSurface activeSurface = ProductFrontendSurface::None;
  ProductFrontendSurface parentSurface = ProductFrontendSurface::None;
  ProductInputSurface inputSurface = ProductInputSurface::None;
  MenuOwner inputOwner = MenuOwner::None;
  MenuOwner parentOwner = MenuOwner::None;
  bool gameplayInputSuppressed = true;
  ProductActiveMouseCapturePolicy mouseCapturePolicy =
      ProductActiveMouseCapturePolicy::Released;
  bool acceptsMenuActions = false;
  bool acceptsSystemActions = true;
  bool acceptsPlayerActions = false;
  bool acceptsEditorActions = false;
  std::string_view status = "product_active_surface_ready";
};

struct ProductFrontendRouteFrame {
  ProductFrontendOwnerDecision owner;
  FrontendRouteResult route;
  bool routed = false;
  bool routeModelAvailable = true;
  std::string_view routeModelName = "none";
  bool hasWorldSetupRoute = false;
  WorldSetupRouteResult worldSetupRoute;
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
std::string_view productActiveMouseCapturePolicyName(
    ProductActiveMouseCapturePolicy policy);

ProductActiveSurfaceContext productActiveSurfaceContextForWindow(
    const FrontendState& frontend,
    const ProductAppWindowState& window);

ProductActiveSurfaceFrame resolveProductActiveSurface(
    const ProductActiveSurfaceContext& context);

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
