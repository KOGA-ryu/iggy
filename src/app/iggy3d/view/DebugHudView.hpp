#pragma once

#if defined(IGGY3D_HAS_SDL3)

struct SDL_Renderer;

namespace iggy3d {

struct GameplayFeedback;
struct InteractionModeHud;
struct MovementDebugHud;
struct NpcBehaviorDebugHud;
struct PhysicsDebugHud;
struct PositionHud;
struct ProductRoomEditorHud;

void drawCameraHeading(SDL_Renderer& renderer, float yawDegrees);
void drawGameplayFeedback(SDL_Renderer& renderer,
                          const GameplayFeedback* feedback);
void drawInteractionModeHud(SDL_Renderer& renderer,
                            const InteractionModeHud* hud);
void drawMovementDebugHud(SDL_Renderer& renderer,
                          const MovementDebugHud* hud);
void drawNpcBehaviorDebugHud(SDL_Renderer& renderer,
                             const NpcBehaviorDebugHud* hud);
void drawPhysicsDebugHud(SDL_Renderer& renderer,
                         const PhysicsDebugHud* hud);
void drawRoomEditorHud(SDL_Renderer& renderer,
                       const ProductRoomEditorHud* hud);
void drawPositionHud(SDL_Renderer& renderer,
                     const PositionHud* hud);

}  // namespace iggy3d

#endif
