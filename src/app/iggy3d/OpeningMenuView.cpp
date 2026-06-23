#include "app/iggy3d/OpeningMenuView.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

namespace iggy3d {
namespace {

using GlyphRows = std::array<std::uint8_t, 7>;

GlyphRows glyphFor(char c) {
  switch (static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) {
    case 'A':
      return {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    case 'B':
      return {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
    case 'C':
      return {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
    case 'D':
      return {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
    case 'E':
      return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    case 'F':
      return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
    case 'G':
      return {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F};
    case 'H':
      return {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    case 'I':
      return {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
    case 'J':
      return {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
    case 'K':
      return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    case 'L':
      return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
    case 'M':
      return {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
    case 'N':
      return {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    case 'O':
      return {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    case 'P':
      return {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    case 'Q':
      return {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
    case 'R':
      return {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
    case 'S':
      return {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    case 'T':
      return {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    case 'U':
      return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    case 'V':
      return {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
    case 'W':
      return {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
    case 'X':
      return {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
    case 'Y':
      return {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
    case 'Z':
      return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
    case '0':
      return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
    case '1':
      return {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
    case '2':
      return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
    case '3':
      return {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
    case '4':
      return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
    case '5':
      return {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
    case '6':
      return {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
    case '7':
      return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    case '8':
      return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
    case '9':
      return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
    case '-':
      return {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
    case '.':
      return {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
    case ':':
      return {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00};
    case '>':
      return {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10};
    default:
      break;
  }
  return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
}

void setColor(SDL_Renderer& renderer, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  SDL_SetRenderDrawColor(&renderer, r, g, b, 255);
}

void fillRect(SDL_Renderer& renderer, float x, float y, float w, float h) {
  SDL_FRect rect{x, y, w, h};
  SDL_RenderFillRect(&renderer, &rect);
}

void drawText(SDL_Renderer& renderer, std::string_view text, float x, float y, float scale) {
  float cursor = x;
  for (const char c : text) {
    if (c == ' ') {
      cursor += 6.0F * scale;
      continue;
    }
    const GlyphRows glyph = glyphFor(c);
    for (std::size_t row = 0; row < glyph.size(); ++row) {
      for (std::size_t col = 0; col < 5; ++col) {
        const bool on = (glyph[row] & (1U << (4U - col))) != 0U;
        if (on) {
          fillRect(renderer, cursor + static_cast<float>(col) * scale,
                   y + static_cast<float>(row) * scale, scale, scale);
        }
      }
    }
    cursor += 6.0F * scale;
  }
}

float screenX(float worldX) {
  return 640.0F + worldX * 92.0F;
}

float screenY(float worldZ) {
  return 394.0F - worldZ * 92.0F;
}

void drawMarker(SDL_Renderer& renderer,
                const ProductPrimitiveDrawItem& item) {
  setColor(renderer, item.color.r, item.color.g, item.color.b);
  const float x = screenX(item.worldPosition.x);
  const float y = screenY(item.worldPosition.z);
  const float size = item.markerSize;
  fillRect(renderer, x - size * 0.5F, y - size * 0.5F, size, size);
}

void drawFocusIndicator(SDL_Renderer& renderer, const ProductPrimitiveDrawItem& item) {
  const float x = screenX(item.worldPosition.x);
  const float y = screenY(item.worldPosition.z);
  setColor(renderer, 226, 230, 211);
  fillRect(renderer, x - 18.0F, y - 2.0F, 36.0F, 4.0F);
  fillRect(renderer, x - 2.0F, y - 18.0F, 4.0F, 36.0F);
}

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

std::string roundedDegrees(float value) {
  return std::to_string(static_cast<int>(std::lround(value)));
}

void drawPrimitiveItem(SDL_Renderer& renderer, const ProductPrimitiveDrawItem& item) {
  if (!item.visible) {
    return;
  }

  switch (item.kind) {
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
      drawFocusIndicator(renderer, item);
      return;
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
      drawMarker(renderer, item);
      return;
  }
}

void drawGrid(SDL_Renderer& renderer) {
  setColor(renderer, 18, 28, 29);
  fillRect(renderer, 80.0F, 130.0F, 1120.0F, 480.0F);
  setColor(renderer, 32, 48, 48);
  for (int i = 0; i <= 14; ++i) {
    const float x = 80.0F + static_cast<float>(i) * 80.0F;
    fillRect(renderer, x, 130.0F, 2.0F, 480.0F);
  }
  for (int i = 0; i <= 6; ++i) {
    const float y = 130.0F + static_cast<float>(i) * 80.0F;
    fillRect(renderer, 80.0F, y, 1120.0F, 2.0F);
  }
}

void drawMenuRow(SDL_Renderer& renderer,
                 std::string_view label,
                 bool selected,
                 bool enabled,
                 float x,
                 float y) {
  if (selected) {
    setColor(renderer, 36, 96, 116);
    fillRect(renderer, x - 18.0F, y - 8.0F, 310.0F, 34.0F);
    setColor(renderer, 245, 247, 232);
    drawText(renderer, ">", x - 10.0F, y, 3.0F);
  }
  if (enabled) {
    setColor(renderer, 238, 239, 220);
  } else {
    setColor(renderer, 118, 124, 126);
  }
  drawText(renderer, label, x + 18.0F, y, 3.0F);
}

std::string_view settingsTabLabel(FrontendSettingsTab tab) {
  switch (tab) {
    case FrontendSettingsTab::Input:
      return "Input";
    case FrontendSettingsTab::Controls:
      return "Controls";
    case FrontendSettingsTab::Camera:
      return "Camera";
    case FrontendSettingsTab::Gameplay:
      return "Gameplay";
    case FrontendSettingsTab::VideoDisplay:
      return "Video Display";
    case FrontendSettingsTab::Audio:
      return "Audio";
    case FrontendSettingsTab::Accessibility:
      return "Accessibility";
    case FrontendSettingsTab::Developer:
      return "Developer";
    case FrontendSettingsTab::None:
      break;
  }
  return "None";
}

void drawPanelRow(SDL_Renderer& renderer,
                  std::string_view label,
                  bool selected,
                  float x,
                  float y) {
  if (selected) {
    setColor(renderer, 48, 76, 92);
    fillRect(renderer, x - 16.0F, y - 8.0F, 360.0F, 30.0F);
  }
  setColor(renderer, selected ? 242 : 174, selected ? 245 : 190, selected ? 220 : 182);
  drawText(renderer, label, x, y, 2.0F);
}

void drawNewWorldPanel(SDL_Renderer& renderer,
                       const ProductWorldTemplate& world,
                       const ProductSaveBridgeResult& saves) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "NEW WORLD", 450.0F, 152.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "CREATE A SAVE AND ENTER THE FIRST WORLD", 452.0F, 210.0F, 2.0F);
  drawText(renderer, "PACKAGE", 452.0F, 275.0F, 2.0F);
  drawText(renderer, world.packageId, 452.0F, 304.0F, 2.0F);
  drawText(renderer, "SCENARIO", 452.0F, 356.0F, 2.0F);
  drawText(renderer, world.scenarioId, 452.0F, 385.0F, 2.0F);
  drawText(renderer, "SAVES", 452.0F, 437.0F, 2.0F);
  drawText(renderer, std::to_string(saves.slots.slots.size()), 542.0F, 437.0F, 2.0F);
}

void drawDevToolsPanel(SDL_Renderer& renderer, FrontendDevToolsCategory selected) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "DEV TOOLS", 450.0F, 128.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "SELECT A DIAGNOSTIC CATEGORY", 452.0F, 185.0F, 2.0F);

  float y = 230.0F;
  for (const FrontendDevToolsCategory category : devToolsCategoryOrder()) {
    drawPanelRow(renderer, devToolsCategoryLabel(category), category == selected, 458.0F, y);
    y += 34.0F;
  }

  setColor(renderer, 126, 201, 176);
  drawText(renderer, "READOUTS", 850.0F, 230.0F, 2.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "RUNTIME STATE", 850.0F, 272.0F, 2.0F);
  drawText(renderer, "INPUT OWNER", 850.0F, 304.0F, 2.0F);
  drawText(renderer, "RENDERER STATUS", 850.0F, 336.0F, 2.0F);
  drawText(renderer, "READ ONLY V1", 850.0F, 394.0F, 2.0F);
}

void drawSettingsPanel(SDL_Renderer& renderer, FrontendSettingsTab selected) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "SETTINGS", 450.0F, 128.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "SELECT A SETTINGS CATEGORY", 452.0F, 185.0F, 2.0F);

  float y = 230.0F;
  for (const FrontendSettingsTab tab : settingsTabOrder()) {
    drawPanelRow(renderer, settingsTabLabel(tab), tab == selected, 458.0F, y);
    y += 34.0F;
  }

  setColor(renderer, 126, 201, 176);
  drawText(renderer, "CURRENT", 850.0F, 230.0F, 2.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "INPUT AUTO", 850.0F, 272.0F, 2.0F);
  drawText(renderer, "LOOK 1.000", 850.0F, 304.0F, 2.0F);
  drawText(renderer, "CAMERA FIRST PERSON", 850.0F, 336.0F, 2.0F);
  drawText(renderer, "APPLY RESTORE BACK", 850.0F, 394.0F, 2.0F);
}

bool drawGameplayPanel(SDL_Renderer& renderer,
                       std::uint64_t runtimeStateHash,
                       const ProductPrimitiveDrawList* drawList,
                       std::size_t sceneItemCount,
                       const DebugProjectionResult* debug,
                       float cameraYawDegrees,
                       float cameraPitchDegrees) {
  setColor(renderer, 10, 16, 18);
  SDL_RenderClear(&renderer);

  if (drawList == nullptr || drawList->gridVisible) {
    drawGrid(renderer);
  }

  if (drawList != nullptr) {
    for (const ProductPrimitiveDrawItem& item : drawList->items) {
      drawPrimitiveItem(renderer, item);
    }
  }

  setColor(renderer, 226, 230, 211);
  drawText(renderer, "IGGY3D GAMEPLAY", 84.0F, 42.0F, 5.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "FIRST PERSON GAMEPLAY VIEW", 88.0F, 104.0F, 3.0F);
  drawCameraHeading(renderer, cameraYawDegrees);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "CAMERA FIRST PERSON", 870.0F, 286.0F, 2.0F);
  drawText(renderer, "YAW", 870.0F, 324.0F, 2.0F);
  drawText(renderer, roundedDegrees(cameraYawDegrees), 938.0F, 324.0F, 2.0F);
  drawText(renderer, "PITCH", 870.0F, 356.0F, 2.0F);
  drawText(renderer, roundedDegrees(cameraPitchDegrees), 974.0F, 356.0F, 2.0F);
  drawText(renderer, "RUNTIME OWNS GAME STATE", 88.0F, 630.0F, 2.0F);
  drawText(renderer, "STATE HASH", 480.0F, 630.0F, 2.0F);
  drawText(renderer, std::to_string(runtimeStateHash), 640.0F, 630.0F, 2.0F);
  if (drawList != nullptr) {
    drawText(renderer, "SCENE ITEMS", 88.0F, 668.0F, 2.0F);
    drawText(renderer, std::to_string(sceneItemCount), 274.0F, 668.0F, 2.0F);
    drawText(renderer, "DEBUG ITEMS", 384.0F, 668.0F, 2.0F);
    const std::size_t debugCount = debug == nullptr ? 0U : debug->items.size();
    drawText(renderer, std::to_string(debugCount), 570.0F, 668.0F, 2.0F);
  }
  return true;
}

}  // namespace

OpeningMenuHitTestResult openingMenuActionAt(const FrontendState& frontend, float x, float y) {
  const std::array<FrontendAction, 6> rows = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };
  float rowY = 150.0F;
  for (const FrontendAction action : rows) {
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

  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
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

  if (frontend.childScreen == FrontendScreen::Settings) {
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
                                         bool gameplayActive,
                                         std::uint64_t runtimeStateHash,
                                         const ProductPrimitiveDrawList* drawList,
                                         std::size_t sceneItemCount,
                                         const DebugProjectionResult* debug,
                                         float cameraYawDegrees,
                                         float cameraPitchDegrees,
                                         const ProductSaveBridgeResult& saves) {
  OpeningMenuViewState state;
  SDL_SetRenderDrawBlendMode(&renderer, SDL_BLENDMODE_BLEND);

  if (gameplayActive || frontend.screen == FrontendScreen::Gameplay) {
    state.cameraHeadingDrawn =
        drawGameplayPanel(renderer, runtimeStateHash, drawList, sceneItemCount, debug,
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
  drawText(renderer, "OPENING MENU", 330.0F, 44.0F, 3.0F);

  setColor(renderer, 27, 33, 37);
  fillRect(renderer, 0.0F, 92.0F, 390.0F, 556.0F);
  setColor(renderer, 18, 22, 25);
  fillRect(renderer, 390.0F, 92.0F, 890.0F, 556.0F);

  const std::array<FrontendAction, 6> rows = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };
  float y = 150.0F;
  for (const FrontendAction action : rows) {
    const bool enabled = action != FrontendAction::Continue || saves.slots.compatibleCount > 0;
    drawMenuRow(renderer, frontendActionName(action), action == frontend.selectedAction, enabled,
                62.0F, y);
    y += 52.0F;
    ++state.rowCount;
  }

  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    drawDevToolsPanel(renderer, frontend.devToolsCategory);
  } else if (frontend.childScreen == FrontendScreen::Settings) {
    drawSettingsPanel(renderer, selectedSettingsTab);
  } else {
    drawNewWorldPanel(renderer, world, saves);
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
