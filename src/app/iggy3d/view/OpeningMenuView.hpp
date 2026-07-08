#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"

#if defined(IGGY3D_HAS_SDL3)
struct SDL_Renderer;
#endif

namespace iggy3d {

struct DebugProjectionResult;
struct FrontendState;
struct GameplayFeedback;
struct InteractionModeHud;
struct MovementDebugHud;
struct NpcBehaviorDebugHud;
struct PhysicsDebugHud;
struct PositionHud;
struct ProductAppOptions;
struct ProductRoomEditorHud;
struct ProductSaveBridgeResult;
struct ProductViewportFrame;
struct ProductWorldTemplate;
struct TopDownMapOverlay;
struct WorldSetupDraft;

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
