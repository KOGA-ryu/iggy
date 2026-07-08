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
#include "app/iggy3d/view/OpeningMenuHitTest.hpp"
#include "app/iggy3d/view/ScenePrimitiveView.hpp"
#include "app/iggy3d/view/SdlDraw.hpp"
#include "app/iggy3d/view/ViewportFraming.hpp"

namespace iggy3d {
namespace {

std::string_view menuTitleForFrontend(const FrontendState& frontend) {
  // branch-gate: BG-1029
  return openingMenuUsesPauseRows(frontend) ? "PAUSE MENU" : "OPENING MENU";
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

}  // namespace

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
  for (const FrontendAction action : openingMenuActionOrderForFrontend(frontend)) {
    const bool enabled = openingMenuUsesPauseRows(frontend) ||
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
