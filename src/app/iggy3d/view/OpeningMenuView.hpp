#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/debug/InteractionModeHud.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"
#include "app/iggy3d/view/PrimitiveDrawList.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/debug/TopDownMapOverlay.hpp"
#include "app/iggy3d/view/ViewportFraming.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

#if defined(IGGY3D_HAS_SDL3)
struct SDL_Renderer;
#endif

namespace iggy3d {

struct DebugProjectionResult;

struct OpeningMenuViewState {
  bool textDrawn = false;
  bool selectedRowDrawn = false;
  bool cameraHeadingDrawn = false;
  unsigned int rowCount = 0;
};

#if defined(IGGY3D_HAS_SDL3)
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
                                         const std::string& deleteCandidateId);
#endif

}  // namespace iggy3d
