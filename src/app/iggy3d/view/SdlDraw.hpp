#pragma once

#if defined(IGGY3D_HAS_SDL3)

#include <array>
#include <cstdint>
#include <string_view>

struct SDL_Renderer;

namespace iggy3d {

using GlyphRows = std::array<std::uint8_t, 7>;

GlyphRows glyphFor(char c);
void setColor(SDL_Renderer& renderer,
              std::uint8_t r,
              std::uint8_t g,
              std::uint8_t b);
void fillRect(SDL_Renderer& renderer, float x, float y, float w, float h);
void drawText(SDL_Renderer& renderer,
              std::string_view text,
              float x,
              float y,
              float scale);

}  // namespace iggy3d

#endif
