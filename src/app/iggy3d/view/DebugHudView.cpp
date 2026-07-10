#include "app/iggy3d/view/DebugHudView.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>

#include <cmath>
#include <cstdint>

#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"
#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/view/SdlDraw.hpp"

namespace iggy3d {
namespace {

void setFeedbackToneColor(SDL_Renderer& renderer, FeedbackTone tone) {
  switch (tone) {
    case FeedbackTone::Pass:
      setColor(renderer, 126, 201, 176);
      return;
    case FeedbackTone::Warn:
      setColor(renderer, 220, 178, 86);
      return;
    case FeedbackTone::Fail:
      setColor(renderer, 222, 112, 96);
      return;
    case FeedbackTone::Neutral:
      break;
  }
  setColor(renderer, 166, 184, 177);
}

}  // namespace

void drawCameraHeading(SDL_Renderer& renderer, float yawDegrees) {
  constexpr float kPi = 3.14159265358979323846F;
  const float radians = yawDegrees * kPi / 180.0F;
  const float originX = 1040.0F;
  const float originY = 188.0F;
  const float endX = originX + std::sin(radians) * 58.0F;
  const float endY = originY - std::cos(radians) * 58.0F;

  setColor(renderer, 42, 52, 56);
  fillRect(renderer, originX - 44.0F, originY - 44.0F, 88.0F, 88.0F);
  setColor(renderer, 80, 170, 236);
  SDL_RenderLine(&renderer, originX, originY, endX, endY);
  fillRect(renderer, endX - 4.0F, endY - 4.0F, 8.0F, 8.0F);
  setColor(renderer, 226, 230, 211);
  fillRect(renderer, originX - 3.0F, originY - 3.0F, 6.0F, 6.0F);
}

void drawGameplayFeedback(SDL_Renderer& renderer,
                          const GameplayFeedback* feedback) {
  if (feedback == nullptr || !feedback->visible) {
    return;
  }

  setColor(renderer, 18, 24, 27);
  fillRect(renderer, 860.0F, 410.0F, 330.0F, 150.0F);
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "ACTION FEEDBACK", 878.0F, 430.0F, 2.0F);

  float y = 462.0F;
  for (const GameplayFeedbackLine& line : feedback->lines) {
    if (!line.visible) {
      continue;
    }
    setColor(renderer, 166, 184, 177);
    drawText(renderer, line.label, 878.0F, y, 2.0F);
    setFeedbackToneColor(renderer, line.tone);
    drawText(renderer, line.value, 1010.0F, y, 2.0F);
    y += 24.0F;
  }
}

void drawInteractionModeHud(SDL_Renderer& renderer,
                            const InteractionModeHud* hud) {
  // branch-gate: BG-1065
  if (hud == nullptr || !hud->visible) {
    return;
  }

  setColor(renderer, 18, 24, 27);
  fillRect(renderer, 862.0F, 238.0F, 328.0F, 34.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "MODE", 878.0F, 248.0F, 2.0F);
  setFeedbackToneColor(renderer, hud->tone);
  drawText(renderer, hud->label, 970.0F, 248.0F, 2.0F);
}

void drawMovementDebugHud(SDL_Renderer& renderer,
                          const MovementDebugHud* hud) {
  if (hud == nullptr || !hud->visible) {
    return;
  }

  setColor(renderer, 14, 21, 23);
  fillRect(renderer, 92.0F, 146.0F, 500.0F, 190.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "MOVEMENT DEBUG", 108.0F, 164.0F, 2.0F);

  float y = 198.0F;
  for (const MovementDebugHudLine& line : hud->lines) {
    if (!line.visible) {
      continue;
    }
    setColor(renderer, 166, 184, 177);
    drawText(renderer, line.label, 108.0F, y, 2.0F);
    setFeedbackToneColor(renderer, line.tone);
    drawText(renderer, line.value, 226.0F, y, 2.0F);
    y += 22.0F;
  }
}

void drawNpcBehaviorDebugHud(SDL_Renderer& renderer,
                             const NpcBehaviorDebugHud* hud) {
  if (hud == nullptr || !hud->visible) {
    return;
  }

  setColor(renderer, 14, 21, 23);
  fillRect(renderer, 700.0F, 146.0F, 490.0F, 220.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "NPC DEBUG", 716.0F, 164.0F, 2.0F);

  float y = 198.0F;
  std::uint64_t drawn = 0;
  for (const NpcBehaviorDebugHudLine& line : hud->lines) {
    if (!line.visible) {
      continue;
    }
    if (drawn >= 8U) {
      break;
    }
    setFeedbackToneColor(renderer, line.tone);
    drawText(renderer, line.text, 716.0F, y, 1.0F);
    y += 20.0F;
    ++drawn;
  }
}

void drawPhysicsDebugHud(SDL_Renderer& renderer,
                         const PhysicsDebugHud* hud) {
  // branch-gate: BG-1110
  if (hud == nullptr || !hud->visible) {
    return;
  }

  setColor(renderer, 14, 21, 23);
  fillRect(renderer, 92.0F, 360.0F, 720.0F, 150.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "PHYSICS DEBUG", 108.0F, 378.0F, 2.0F);

  float y = 408.0F;
  std::uint64_t drawn = 0;
  for (const PhysicsDebugHudLine& line : hud->lines) {
    // branch-gate: BG-1110
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1110
    if (drawn >= 5U) {
      break;
    }
    setFeedbackToneColor(renderer, line.tone);
    drawText(renderer, line.text, 108.0F, y, 1.0F);
    y += 20.0F;
    ++drawn;
  }
}

void drawRoomEditorHud(SDL_Renderer& renderer,
                       const ProductRoomEditorHud* hud) {
  // branch-gate: BG-1035
  if (hud == nullptr || !hud->visible) {
    return;
  }

  setColor(renderer, 18, 24, 27);
  fillRect(renderer, 88.0F, 388.0F, 560.0F, 180.0F);
  setColor(renderer, 245, 214, 96);
  drawText(renderer, "ROOM EDITOR", 106.0F, 408.0F, 2.0F);

  float y = 440.0F;
  std::uint64_t drawn = 0;
  for (const ProductRoomEditorHudLine& line : hud->lines) {
    // branch-gate: BG-1035
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1035
    if (drawn >= 6U) {
      break;
    }
    setColor(renderer, 166, 184, 177);
    drawText(renderer, line.text, 106.0F, y, 1.0F);
    y += 20.0F;
    ++drawn;
  }
}

void drawPositionHud(SDL_Renderer& renderer,
                     const PositionHud* hud) {
  // branch-gate: BG-1192
  if (hud == nullptr || !hud->visible) {
    return;
  }

  setColor(renderer, 14, 21, 23);
  fillRect(renderer, 88.0F, 572.0F, 360.0F, 54.0F);

  float y = 582.0F;
  std::uint64_t drawn = 0;
  for (const PositionHudLine& line : hud->lines) {
    // branch-gate: BG-1192
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1192
    if (drawn >= 3U) {
      break;
    }
    setColor(renderer, 226, 230, 211);
    drawText(renderer, line.text, 100.0F, y, 1.0F);
    y += 16.0F;
    ++drawn;
  }
}

}  // namespace iggy3d

#endif
