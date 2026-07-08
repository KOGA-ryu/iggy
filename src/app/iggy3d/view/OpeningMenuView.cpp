#include "app/iggy3d/view/OpeningMenuView.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/debug/InteractionModeHud.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/PauseUi.hpp"
#include "app/iggy3d/menu/UiHitRouter.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/view/DebugHudView.hpp"
#include "app/iggy3d/view/MenuPanelsView.hpp"
#include "app/iggy3d/view/ScenePrimitiveView.hpp"
#include "app/iggy3d/view/SdlDraw.hpp"
#include "app/iggy3d/view/ViewportFraming.hpp"

namespace iggy3d {
namespace {

bool usesPauseMenuRows(const FrontendState& frontend) {
  // branch-gate: BG-1029
  return frontend.screen == FrontendScreen::Pause ||
         frontend.childScreen == FrontendScreen::Pause;
}

const std::vector<FrontendAction>& menuActionOrderForFrontend(
    const FrontendState& frontend) {
  // branch-gate: BG-1029
  if (usesPauseMenuRows(frontend)) {
    return pauseActionOrder();
  }
  return starterActionOrder();
}

std::string_view menuTitleForFrontend(const FrontendState& frontend) {
  // branch-gate: BG-1029
  return usesPauseMenuRows(frontend) ? "PAUSE MENU" : "OPENING MENU";
}

std::string roundedDegrees(float value) {
  return std::to_string(static_cast<int>(std::lround(value)));
}

bool drawGameplayPanel(SDL_Renderer& renderer,
                       std::uint64_t runtimeStateHash,
                       const ProductViewportFrame* frame,
                       const GameplayFeedback* feedback,
                       const InteractionModeHud* interactionModeHud,
                       const TopDownMapOverlay* topDownMapOverlay,
                       const MovementDebugHud* movementHud,
                       const NpcBehaviorDebugHud* npcHud,
                       const PhysicsDebugHud* physicsHud,
                       const PositionHud* positionHud,
                       const ProductRoomEditorHud* roomEditorHud,
                       const ProductGameplayMovementTuning& movementTuning,
                       ProductGameplayMovementTuningField movementTuningField,
                       bool movementTuningVisible,
                       std::size_t sceneItemCount,
                       const DebugProjectionResult* debug,
                       float cameraYawDegrees,
                       float cameraPitchDegrees) {
  setColor(renderer, 10, 16, 18);
  SDL_RenderClear(&renderer);

  drawFirstPersonPrimitiveViewport(renderer, frame);
  drawTopDownMapPrimitives(renderer, frame, topDownMapOverlay);

  setColor(renderer, 226, 230, 211);
  drawText(renderer, "IGGY3D GAMEPLAY", 84.0F, 42.0F, 5.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "FIRST-PERSON PRIMITIVE VIEW", 88.0F, 104.0F, 3.0F);
  drawCameraHeading(renderer, cameraYawDegrees);
  drawInteractionModeHud(renderer, interactionModeHud);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "CAMERA HEADING", 870.0F, 286.0F, 2.0F);
  drawText(renderer, "YAW", 870.0F, 324.0F, 2.0F);
  drawText(renderer, roundedDegrees(cameraYawDegrees), 938.0F, 324.0F, 2.0F);
  drawText(renderer, "PITCH", 870.0F, 356.0F, 2.0F);
  drawText(renderer, roundedDegrees(cameraPitchDegrees), 974.0F, 356.0F, 2.0F);
  drawMovementDebugHud(renderer, movementHud);
  drawNpcBehaviorDebugHud(renderer, npcHud);
  drawPhysicsDebugHud(renderer, physicsHud);
  drawRoomEditorHud(renderer, roomEditorHud);
  drawPositionHud(renderer, positionHud);
  drawGameplayMovementTuningHud(renderer,
                                movementTuning,
                                movementTuningField,
                                movementTuningVisible);
  drawGameplayFeedback(renderer, feedback);
  drawText(renderer, "RUNTIME OWNS GAME STATE", 88.0F, 630.0F, 2.0F);
  drawText(renderer, "STATE HASH", 480.0F, 630.0F, 2.0F);
  drawText(renderer, std::to_string(runtimeStateHash), 640.0F, 630.0F, 2.0F);
  if (frame != nullptr) {
    drawText(renderer, "SCENE ITEMS", 88.0F, 668.0F, 2.0F);
    drawText(renderer, std::to_string(sceneItemCount), 274.0F, 668.0F, 2.0F);
    drawText(renderer, "DEBUG ITEMS", 384.0F, 668.0F, 2.0F);
    const std::size_t debugCount = debug == nullptr ? 0U : debug->items.size();
    drawText(renderer, std::to_string(debugCount), 570.0F, 668.0F, 2.0F);
  }
  return true;
}

bool uiRectContains(const ProductUiRect& rect, float x, float y) {
  return x >= rect.x && x <= rect.x + rect.width &&
         y >= rect.y && y <= rect.y + rect.height;
}

// Looks up the interactive-widget action at (x, y) among the hit regions the
// widget layer (docs/ui/ui_architecture.md) emitted for the current child
// screen's content — the SAME rects that produced the draw primitives, so
// hit-testing cannot drift from what is drawn. Regions are searched back to
// front (reverse emission order) so an overlap between adjacent buttons
// resolves the same way the visual stack would: the later (topmost) one wins.
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
  for (const FrontendAction action : menuActionOrderForFrontend(frontend)) {
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

OpeningMenuViewState drawOpeningMenuView(SDL_Renderer& renderer,
                                         const ProductAppOptions& options,
                                         const ProductWorldTemplate& world,
                                         const FrontendState& frontend,
                                         FrontendSettingsTab selectedSettingsTab,
                                         const ProductGameplayMovementTuning& movementTuning,
                                         ProductGameplayMovementTuningField movementTuningField,
                                         bool movementTuningVisible,
                                         const WorldSetupDraft& worldSetupDraft,
                                         bool dungeonDraftEditMode,
                                         bool dungeonDraftModified,
                                         std::uint64_t dungeonDraftCursorRow,
                                         std::uint64_t dungeonDraftCursorColumn,
                                         const std::string& dungeonDraftSelectedGlyph,
                                         const std::string& dungeonDraftLastGlyph,
                                         bool gameplayActive,
                                         std::uint64_t runtimeStateHash,
                                         const ProductViewportFrame* frame,
                                         const GameplayFeedback* feedback,
                                         const InteractionModeHud* interactionModeHud,
                                         const TopDownMapOverlay* topDownMapOverlay,
                                         const MovementDebugHud* movementHud,
                                         const NpcBehaviorDebugHud* npcHud,
                                         const PhysicsDebugHud* physicsHud,
                                         const PositionHud* positionHud,
                                         const ProductRoomEditorHud* roomEditorHud,
                                         std::size_t sceneItemCount,
                                         const DebugProjectionResult* debug,
                                         float cameraYawDegrees,
                                         float cameraPitchDegrees,
                                         const ProductSaveBridgeResult& saves,
                                         const std::string& deleteCandidateId) {
  OpeningMenuViewState state;
  SDL_SetRenderDrawBlendMode(&renderer, SDL_BLENDMODE_BLEND);
  (void)gameplayActive;

  if (frontend.screen == FrontendScreen::Gameplay) {
    state.cameraHeadingDrawn =
        drawGameplayPanel(renderer, runtimeStateHash, frame, feedback,
                          interactionModeHud, topDownMapOverlay, movementHud, npcHud,
                          physicsHud, positionHud, roomEditorHud,
                          movementTuning, movementTuningField, movementTuningVisible,
                          sceneItemCount, debug,
                          cameraYawDegrees, cameraPitchDegrees);
    SDL_RenderPresent(&renderer);
    state.textDrawn = true;
    return state;
  }

  setColor(renderer, 12, 15, 18);
  SDL_RenderClear(&renderer);

  setColor(renderer, 24, 30, 34);
  fillRect(renderer, 0.0F, 0.0F, 1280.0F, 92.0F);
  setColor(renderer, 231, 236, 214);
  drawText(renderer, "IGGY3D", 46.0F, 34.0F, 5.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, menuTitleForFrontend(frontend), 330.0F, 44.0F, 3.0F);

  setColor(renderer, 27, 33, 37);
  fillRect(renderer, 0.0F, 92.0F, 390.0F, 556.0F);
  setColor(renderer, 18, 22, 25);
  fillRect(renderer, 390.0F, 92.0F, 890.0F, 556.0F);

  float y = 150.0F;
  for (const FrontendAction action : menuActionOrderForFrontend(frontend)) {
    const bool enabled = usesPauseMenuRows(frontend) ||
                         action != FrontendAction::Continue ||
                         saves.slots.compatibleCount > 0;
    drawMenuRow(renderer, frontendActionName(action), action == frontend.selectedAction, enabled,
                62.0F, y);
    y += 52.0F;
    ++state.rowCount;
  }

  const ProductFrontendSurface detailSurface =
      openingMenuDetailSurfaceFor(frontend);
  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    drawDevToolsPanel(renderer, frontend.devToolsCategory);
  } else if (detailSurface == ProductFrontendSurface::Settings) {
    drawSettingsPanel(renderer,
                      selectedSettingsTab,
                      movementTuning,
                      movementTuningField);
  // branch-gate: BG-1121
  } else if (frontend.childScreen == FrontendScreen::LoadSave) {
    drawLoadSavePanel(renderer, frontend.saveBrowserMode, saves);
  // branch-gate: BG-1121
  } else if (frontend.childScreen == FrontendScreen::DeleteConfirm) {
    // Resolve the panel text from the SAME shared source the draw-list lane uses (sd2).
    drawDeleteConfirmPanel(
        renderer, resolveProductDeleteConfirmModel(deleteCandidateId, saves));
  // branch-gate: BG-1141
  } else if (frontend.childScreen == FrontendScreen::NewWorld) {
    drawNewWorldPanel(renderer,
                      world,
                      saves,
                      worldSetupDraft,
                      dungeonDraftEditMode,
                      dungeonDraftModified,
                      dungeonDraftCursorRow,
                      dungeonDraftCursorColumn,
                      dungeonDraftSelectedGlyph,
                      dungeonDraftLastGlyph);
  } else {
    drawStarterDetailPanel(renderer);
  }

  setColor(renderer, 24, 30, 34);
  fillRect(renderer, 0.0F, 648.0F, 1280.0F, 72.0F);
  setColor(renderer, 164, 178, 170);
  drawText(renderer, "ENTER CONFIRM   ESC EXIT   INPUT", 44.0F, 674.0F, 2.0F);
  drawText(renderer, productInputBackendName(options.inputBackend), 560.0F, 674.0F, 2.0F);
  drawText(renderer, "RENDERER", 748.0F, 674.0F, 2.0F);
  drawText(renderer, productRendererRequestName(options.renderer), 910.0F, 674.0F, 2.0F);

  SDL_RenderPresent(&renderer);
  state.textDrawn = true;
  state.selectedRowDrawn = frontend.selectedAction != FrontendAction::None;
  return state;
}

}  // namespace iggy3d
#endif
