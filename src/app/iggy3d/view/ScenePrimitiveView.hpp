#pragma once

#if defined(IGGY3D_HAS_SDL3)

struct SDL_Renderer;

namespace iggy3d {

struct ProductViewportFrame;
struct TopDownMapOverlay;

void drawFirstPersonPrimitiveViewport(SDL_Renderer& renderer,
                                      const ProductViewportFrame* frame);
void drawTopDownMapPrimitives(SDL_Renderer& renderer,
                              const ProductViewportFrame* frame,
                              const TopDownMapOverlay* overlay);

}  // namespace iggy3d

#endif
