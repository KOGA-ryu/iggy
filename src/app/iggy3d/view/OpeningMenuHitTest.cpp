#include "app/iggy3d/view/OpeningMenuHitTest.hpp"

#include "app/iggy3d/menu/PauseUi.hpp"
#include "app/iggy3d/menu/UiHitRouter.hpp"

namespace iggy3d {
namespace {

bool uiRectContains(const ProductUiRect& rect, float x, float y) {
  return x >= rect.x && x <= rect.x + rect.width &&
         y >= rect.y && y <= rect.y + rect.height;
}

// Looks up the interactive-widget action at (x, y) among the hit regions the
// widget layer (docs/ui/ui_architecture.md) emitted for the current child
// screen's content. Regions are searched back to front so overlaps resolve to
// the later topmost visual.
FrontendAction hitRegionActionAt(const std::vector<UiHitRegion>& regions,
                                 float x,
                                 float y) {
  for (auto it = regions.rbegin(); it != regions.rend(); ++it) {
    // branch-gate: BG-1121
    if (it->action != FrontendAction::None && uiRectContains(it->rect, x, y)) {
      return it->action;
    }
  }
  return FrontendAction::None;
}

OpeningMenuHitTestResult pauseMenuActionAt(const ProductUiDrawListRequest& request,
                                           float x,
                                           float y) {
  PauseMenuContext pauseContext;
  pauseContext.pauseOpen = true;
  pauseContext.runtimeSessionAvailable = request.gameplayActive;
  pauseContext.saveRootWritable = request.saveRootWritable;
  pauseContext.compatibleSaveCount = request.compatibleSaveCount;
  pauseContext.developerToolsEnabled = request.developerToolsEnabled;
  pauseContext.activeRoomEditable = request.activeRoomEditable;
  pauseContext.roomEditingReady = request.roomEditingReady;

  const PauseMenuModel pauseModel =
      buildPauseMenuModel(pauseContext, request.frontend->selectedAction);
  ProductPauseUiRequest pauseUiRequest;
  pauseUiRequest.model = &pauseModel;
  pauseUiRequest.virtualWidth = request.virtualWidth;
  pauseUiRequest.virtualHeight = request.virtualHeight;
  const ProductUiDrawList pauseUi = buildProductPauseUiDrawList(pauseUiRequest);
  const ProductUiHitLayer layer{
      ProductUiHitSurface::PauseMenu,
      &pauseUi,
      true,
  };
  const ProductUiHitRouteReceipt route =
      routeProductUiHit(ProductUiHitRouteRequest{&layer, 1U, x, y});
  if (!route.hit || !route.consumed || route.action == FrontendAction::None) {
    return {};
  }

  OpeningMenuHitTestResult result;
  result.hit = true;
  result.area = OpeningMenuHitArea::StarterAction;
  result.action = route.action;
  return result;
}

}  // namespace

bool openingMenuUsesPauseRows(const FrontendState& frontend) {
  // branch-gate: BG-1029
  return frontend.screen == FrontendScreen::Pause ||
         frontend.childScreen == FrontendScreen::Pause;
}

const std::vector<FrontendAction>& openingMenuActionOrderForFrontend(
    const FrontendState& frontend) {
  // branch-gate: BG-1029
  if (openingMenuUsesPauseRows(frontend)) {
    return pauseActionOrder();
  }
  return starterActionOrder();
}

ProductFrontendSurface openingMenuDetailSurfaceFor(
    const FrontendState& frontend) {
  ProductActiveSurfaceContext context;
  context.frontend = frontend;
  return resolveProductActiveSurface(context).activeSurface;
}

OpeningMenuHitTestResult openingMenuActionAt(const ProductUiDrawListRequest& request,
                                             float x,
                                             float y) {
  const FrontendState& frontend = *request.frontend;
  const ProductFrontendSurface detailSurface =
      openingMenuDetailSurfaceFor(frontend);
  if (frontendPauseMenuOpen(frontend)) {
    return pauseMenuActionAt(request, x, y);
  }
  // The hit regions come from the SAME request the frame path draws with (built by
  // buildProductStarterUiDrawListRequest), so the LoadSave/DeleteConfirm button
  // lookups below cannot drift from what was drawn. Screens not yet migrated to
  // widgets emit no regions, so this is a no-op for them.
  const ProductUiDrawList hitTestUi = buildProductStarterUiDrawList(request);
  float rowY = 150.0F;
  for (const FrontendAction action : openingMenuActionOrderForFrontend(frontend)) {
    const bool hitX = x >= 30.0F && x <= 370.0F;
    const bool hitY = y >= rowY - 14.0F && y <= rowY + 38.0F;
    if (hitX && hitY) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::StarterAction;
      result.action = action;
      return result;
    }
    rowY += 52.0F;
  }

  // branch-gate: BG-1121
  if (frontend.childScreen == FrontendScreen::NewWorld) {
    // branch-gate: BG-1121
    if (x >= 430.0F && x <= 760.0F && y >= 490.0F && y <= 536.0F) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::NewWorldCreate;
      return result;
    }
    // branch-gate: BG-1121
    if (x >= 830.0F && x <= 960.0F && y >= 490.0F && y <= 536.0F) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::NewWorldBack;
      return result;
    }
    // branch-gate: BG-1121
    if (x >= 430.0F && x <= 540.0F && y >= 542.0F && y <= 586.0F) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::NewWorldPreviousDungeon;
      return result;
    }
    // branch-gate: BG-1121
    if (x >= 550.0F && x <= 680.0F && y >= 542.0F && y <= 586.0F) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::NewWorldNextDungeon;
      return result;
    }
  }

  // branch-gate: BG-1121
  if (frontend.childScreen == FrontendScreen::LoadSave) {
    float slotY = 318.0F;
    for (std::size_t i = 0; i < 5U; ++i) {
      // branch-gate: BG-1121
      if (x >= 430.0F && x <= 1120.0F && y >= slotY - 12.0F && y <= slotY + 24.0F) {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::LoadSaveSlot;
        result.saveSlotIndex = i;
        return result;
      }
      slotY += 38.0F;
    }
    switch (hitRegionActionAt(hitTestUi.hitRegions, x, y)) {  // branch-gate: BG-1121
      case FrontendAction::Load: {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::LoadSaveLoad;
        return result;
      }
      case FrontendAction::Delete: {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::LoadSaveDelete;
        return result;
      }
      case FrontendAction::Back: {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::LoadSaveBack;
        return result;
      }
      default:
        break;
    }
  }

  // branch-gate: BG-1121
  if (frontend.childScreen == FrontendScreen::DeleteConfirm) {
    switch (hitRegionActionAt(hitTestUi.hitRegions, x, y)) {  // branch-gate: BG-1121
      case FrontendAction::Delete: {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::DeleteConfirmConfirm;
        return result;
      }
      case FrontendAction::Back: {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::DeleteConfirmBack;
        return result;
      }
      default:
        break;
    }
  }

  // branch-gate: BG-1141
  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    // branch-gate: BG-1141
    if (x >= 830.0F && x <= 960.0F && y >= 382.0F && y <= 424.0F) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::DevToolsBack;
      return result;
    }
    float panelY = 230.0F;
    for (const FrontendDevToolsCategory category : devToolsCategoryOrder()) {
      if (x >= 430.0F && x <= 820.0F && y >= panelY - 12.0F && y <= panelY + 24.0F) {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::DevToolsCategory;
        result.devToolsCategory = category;
        return result;
      }
      panelY += 34.0F;
    }
  }

  // branch-gate: BG-1141
  if (detailSurface == ProductFrontendSurface::Settings) {
    // branch-gate: BG-1141
    if (x >= 830.0F && x <= 960.0F && y >= 382.0F && y <= 424.0F) {
      OpeningMenuHitTestResult result;
      result.hit = true;
      result.area = OpeningMenuHitArea::SettingsBack;
      return result;
    }
    float panelY = 230.0F;
    for (const FrontendSettingsTab tab : settingsTabOrder()) {
      if (x >= 430.0F && x <= 820.0F && y >= panelY - 12.0F && y <= panelY + 24.0F) {
        OpeningMenuHitTestResult result;
        result.hit = true;
        result.area = OpeningMenuHitArea::SettingsTab;
        result.settingsTab = tab;
        return result;
      }
      panelY += 34.0F;
    }
  }

  return {};
}

}  // namespace iggy3d
