#include "app/iggy3d/view/MenuPanelsView.hpp"

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

#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/view/SdlDraw.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"

namespace iggy3d {
namespace {

std::string selectedDraftGlyphLabel(const std::string& glyph) {
  // branch-gate: BG-1149
  if (glyph.empty()) {
    return "TOOL . FLOOR";
  }
  // branch-gate: BG-1149
  switch (glyph.front()) {
    case '#':
      return "TOOL # WALL";
    case '.':
      return "TOOL . FLOOR";
    case 'P':
      return "TOOL P PLAYER";
    case 'K':
      return "TOOL K KEY";
    case '$':
      return "TOOL $ PICKUP";
    case 'E':
      return "TOOL E EXIT";
    case '+':
      return "TOOL + DOOR";
    case 'C':
      return "TOOL C CRATE";
    case '^':
      return "TOOL ^ RAMP N";
    case 'v':
      return "TOOL v RAMP S";
    case '<':
      return "TOOL < RAMP W";
    case '>':
      return "TOOL > RAMP E";
  }
  return "TOOL " + glyph;
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

std::string fixedFloat(float value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

std::string sliderBar(float value, float minValue, float maxValue) {
  constexpr int kSegments = 12;
  // branch-gate: BG-1210
  const float normalized =
      maxValue <= minValue ? 0.0F : (value - minValue) / (maxValue - minValue);
  const int filled = std::clamp(
      static_cast<int>(std::lround(std::clamp(normalized, 0.0F, 1.0F) *
                                   static_cast<float>(kSegments))),
      0,
      kSegments);
  std::string bar = "[";
  for (int index = 0; index < kSegments; ++index) {
    // branch-gate: BG-1210
    bar += index < filled ? '#' : '.';
  }
  bar += "]";
  return bar;
}

void drawMovementTuningRows(
    SDL_Renderer& renderer,
    const ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField selectedField) {
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "MOVEMENT TUNING", 790.0F, 230.0F, 2.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "LEFT RIGHT ADJUST", 790.0F, 258.0F, 1.0F);
  drawText(renderer, "ENTER NEXT FIELD", 790.0F, 274.0F, 1.0F);

  float y = 304.0F;
  for (const ProductGameplayMovementTuningFieldDescriptor& descriptor :
       kProductGameplayMovementTuningFields) {
    const float value = tuning.*(descriptor.value);
    const bool selected = descriptor.field == selectedField;
    const std::array<std::uint8_t, 2U> red{166, 245};
    const std::array<std::uint8_t, 2U> green{184, 214};
    const std::array<std::uint8_t, 2U> blue{177, 96};
    const std::size_t colorIndex = static_cast<std::size_t>(selected);
    setColor(renderer, red[colorIndex], green[colorIndex], blue[colorIndex]);
    // branch-gate: BG-1210
    std::string row =
        std::string{selected ? "> " : "  "} + std::string{descriptor.label} +
        " ";
    // branch-gate: BG-1210
    if (descriptor.kind == ProductGameplayMovementTuningFieldKind::Toggle) {
      row += value >= 0.5F ? "ON " : "OFF ";
    } else {
      // branch-gate: BG-1210
      row += fixedFloat(value, descriptor.step < 0.05F ? 2 : 1) + " ";
    }
    row += sliderBar(value, descriptor.minValue, descriptor.maxValue);
    drawText(renderer, row, 790.0F, y, 1.0F);
    y += 20.0F;
  }
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

void drawAsciiPreviewLines(SDL_Renderer& renderer,
                           std::string_view text,
                           float x,
                           float y,
                           bool showCursor,
                           std::uint64_t cursorRow,
                           std::uint64_t cursorColumn) {
  std::size_t lineStart = 0;
  std::size_t row = 0;
  while (lineStart < text.size() && row < 8U) {
    std::size_t lineEnd = text.find('\n', lineStart);
    if (lineEnd == std::string_view::npos) {
      lineEnd = text.size();
    }
    const std::string_view line = text.substr(lineStart, lineEnd - lineStart);
    if (!line.empty()) {
      std::string rowText(line);
      // branch-gate: BG-1145
      if (showCursor && row == cursorRow && cursorColumn < line.size()) {
        rowText.clear();
        rowText.append(line.substr(0U, static_cast<std::size_t>(cursorColumn)));
        rowText.push_back('[');
        rowText.push_back(line[static_cast<std::size_t>(cursorColumn)]);
        rowText.push_back(']');
        rowText.append(line.substr(static_cast<std::size_t>(cursorColumn) + 1U));
        setColor(renderer, 126, 201, 176);
      } else {
        setColor(renderer, 226, 230, 211);
      }
      drawText(renderer, rowText, x, y + static_cast<float>(row) * 22.0F, 1.6F);
    }
    lineStart = lineEnd + 1U;
    ++row;
  }
}

}  // namespace

void drawGameplayMovementTuningHud(
    SDL_Renderer& renderer,
    const ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField selectedField,
    bool visible) {
  // branch-gate: BG-1212
  if (!visible) {
    return;
  }

  setColor(renderer, 14, 21, 23);
  fillRect(renderer, 820.0F, 330.0F, 384.0F, 368.0F);
  setColor(renderer, 245, 214, 96);
  drawText(renderer, "MOVEMENT TUNING", 838.0F, 346.0F, 2.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "F4 HIDE  ENTER/UP/DOWN FIELD", 838.0F, 376.0F, 1.0F);
  drawText(renderer, "LEFT/RIGHT VALUE", 838.0F, 392.0F, 1.0F);

  float y = 418.0F;
  std::uint64_t drawn = 0U;
  for (const ProductGameplayMovementTuningFieldDescriptor& descriptor :
       kProductGameplayMovementTuningFields) {
    // branch-gate: BG-1212
    if (drawn >= productGameplayMovementTuningFieldCount()) {
      break;
    }
    const bool selected = descriptor.field == selectedField;
    // branch-gate: BG-1212
    setColor(renderer,
             selected ? 245 : 166,  // branch-gate: BG-1212
             selected ? 214 : 184,  // branch-gate: BG-1212
             selected ? 96 : 177);  // branch-gate: BG-1212
    const float value = tuning.*(descriptor.value);
    std::string row =
        std::string{selected ? "> " : "  "} +  // branch-gate: BG-1212
        std::string{descriptor.label} + " ";
    // branch-gate: BG-1212
    if (descriptor.kind == ProductGameplayMovementTuningFieldKind::Toggle) {
      row += value >= 0.5F ? "ON" : "OFF";
    } else {
      row += fixedFloat(value,
                        descriptor.step < 0.05F ? 2 : 1);  // branch-gate: BG-1212
    }
    drawText(renderer, row, 838.0F, y, 1.0F);
    y += 16.0F;
    ++drawn;
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

void drawStarterDetailPanel(SDL_Renderer& renderer) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "START", 450.0F, 152.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "SELECT A MENU ITEM", 452.0F, 210.0F, 2.0F);
  drawText(renderer, "CONTINUE LOADS YOUR NEWEST SAVE", 452.0F, 272.0F, 2.0F);
  drawText(renderer, "BUILD MAP OPENS THE TEMPLATE EDITOR", 452.0F, 314.0F, 2.0F);
  drawText(renderer, "EXISTING SAVES OPENS SAVE SLOTS", 452.0F, 356.0F, 2.0F);
  drawText(renderer, "SETTINGS AND DEV TOOLS OPEN PANELS", 452.0F, 398.0F, 2.0F);
}

void drawNewWorldPanel(SDL_Renderer& renderer,
                       const ProductWorldTemplate& world,
                       const ProductSaveBridgeResult& saves,
                       const WorldSetupDraft& draft,
                       bool dungeonDraftEditMode,
                       bool dungeonDraftModified,
                       std::uint64_t dungeonDraftCursorRow,
                       std::uint64_t dungeonDraftCursorColumn,
                       const std::string& dungeonDraftSelectedGlyph,
                       const std::string& dungeonDraftLastGlyph) {
  const std::size_t selectedIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  const std::size_t dungeonCount = productBuiltinDungeonCatalog().size();
  const std::string selectionLabel =
      selectedIndex < dungeonCount
          ? std::to_string(selectedIndex + 1U) + " / " +
                std::to_string(dungeonCount)
          : std::string{"CUSTOM"};

  setColor(renderer, 226, 230, 211);
  drawText(renderer, "MAP BUILDER", 450.0F, 152.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer,
           dungeonDraftEditMode
               ? "EDIT MODE   ARROWS MOVE   1# 2. 3P 4K 5$ 6E 7+ 8C R=RESET 9^ 0v -< => SELECT"
               : "UP DOWN SELECT TEMPLATE   TAB EDIT   CONFIRM BUILD",
           452.0F,
           210.0F,
           2.0F);
  drawText(renderer, "TEMPLATE", 452.0F, 260.0F, 2.0F);
  drawText(renderer, draft.worldName, 452.0F, 289.0F, 2.0F);
  drawText(renderer, "SELECTED", 714.0F, 260.0F, 2.0F);
  drawText(renderer, selectionLabel, 714.0F, 289.0F, 2.0F);
  drawText(renderer, "DRAFT", 714.0F, 338.0F, 2.0F);
  drawText(renderer, dungeonDraftModified ? "CUSTOM" : "TEMPLATE", 714.0F, 367.0F, 2.0F);
  drawText(renderer, "CURSOR", 714.0F, 416.0F, 2.0F);
  drawText(renderer,
           std::to_string(dungeonDraftCursorRow) + "," +
               std::to_string(dungeonDraftCursorColumn),
           714.0F,
           445.0F,
           2.0F);
  drawText(renderer, "ASCII ROOM", 452.0F, 338.0F, 2.0F);
  drawText(renderer, draft.asciiRoomId, 452.0F, 367.0F, 2.0F);
  drawText(renderer, "MAP SOURCE", 452.0F, 416.0F, 2.0F);
  drawText(renderer, draft.asciiRoomSourceName, 452.0F, 445.0F, 2.0F);
  drawText(renderer, "PACKAGE", 850.0F, 260.0F, 2.0F);
  drawText(renderer, world.packageId, 850.0F, 289.0F, 2.0F);
  drawText(renderer, "SCENARIO", 850.0F, 338.0F, 2.0F);
  drawText(renderer, world.scenarioId, 850.0F, 367.0F, 2.0F);
  drawText(renderer, "SAVES", 850.0F, 416.0F, 2.0F);
  drawText(renderer, std::to_string(saves.slots.slots.size()), 940.0F, 416.0F, 2.0F);
  drawText(renderer, "ASCII PREVIEW", 850.0F, 468.0F, 2.0F);
  drawText(renderer,
           selectedDraftGlyphLabel(dungeonDraftSelectedGlyph),
           1040.0F,
           446.0F,
           2.0F);
  drawText(renderer,
           dungeonDraftLastGlyph.empty() ? "LAST none"
                                         : "LAST " + dungeonDraftLastGlyph,
           1040.0F,
           468.0F,
           2.0F);
  drawAsciiPreviewLines(renderer,
                        draft.asciiRoomText,
                        850.0F,
                        500.0F,
                        dungeonDraftEditMode,
                        dungeonDraftCursorRow,
                        dungeonDraftCursorColumn);
  setColor(renderer, 126, 201, 176);
  drawText(renderer,
           dungeonDraftEditMode ? "TAB EXIT EDIT   CONFIRM BUILD" :
                                  "CONFIRM TO BUILD",
           452.0F,
           508.0F,
           2.0F);
  // branch-gate: BG-1139
  drawText(renderer, dungeonDraftEditMode ? "" : "PREV", 452.0F, 556.0F, 2.0F);
  // branch-gate: BG-1139
  drawText(renderer, dungeonDraftEditMode ? "" : "NEXT", 570.0F, 556.0F, 2.0F);
  drawText(renderer, "BACK", 850.0F, 508.0F, 2.0F);
}

void drawLoadSavePanel(SDL_Renderer& renderer,
                       FrontendSaveBrowserMode mode,
                       const ProductSaveBridgeResult& saves) {
  const bool deleteMode = mode == FrontendSaveBrowserMode::Delete;
  setColor(renderer, 226, 230, 211);
  // branch-gate: BG-1121
  drawText(renderer, deleteMode ? "DELETE WORLD" : "LOAD MAP", 450.0F, 152.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer,
           // branch-gate: BG-1121
           deleteMode ? "UP DOWN SELECT WORLD   CONFIRM DELETE" :
                        "UP DOWN SELECT MAP   CONFIRM LOAD",
           452.0F,
           210.0F,
           2.0F);
  drawText(renderer, "SLOTS", 452.0F, 260.0F, 2.0F);
  drawText(renderer, std::to_string(saves.slots.slots.size()), 558.0F, 260.0F, 2.0F);
  drawText(renderer, "COMPATIBLE", 714.0F, 260.0F, 2.0F);
  drawText(renderer, std::to_string(saves.slots.compatibleCount), 910.0F, 260.0F, 2.0F);

  float slotY = 318.0F;
  const std::size_t visibleSlotCount =
      std::min<std::size_t>(saves.slots.slots.size(), 5U);
  // branch-gate: BG-1121
  if (visibleSlotCount == 0U) {
    drawText(renderer, "NO COMPATIBLE SAVES", 452.0F, slotY, 2.0F);
  }
  for (std::size_t i = 0; i < visibleSlotCount; ++i) {
    const SaveSlotPreview& slot = saves.slots.slots[i];
    // branch-gate: BG-1121
    if (slot.enabled) {
      setColor(renderer, 174, 190, 182);
    } else {
      setColor(renderer, 106, 118, 116);
    }
    // branch-gate: BG-1121
    drawText(renderer, slot.displayTitle.empty() ? slot.id : slot.displayTitle,
             452.0F,
             slotY,
             2.0F);
    // branch-gate: BG-1121
    drawText(renderer,
             slot.enabled ? "READY" : slot.reason,
             850.0F,
             slotY,
             2.0F);
    slotY += 38.0F;
  }

  setColor(renderer, 126, 201, 176);
  // branch-gate: BG-1121
  if (deleteMode) {
    setColor(renderer, 236, 118, 86);
    drawText(renderer, "DELETE SELECTED", 452.0F, 548.0F, 2.0F);
    setColor(renderer, 126, 201, 176);
    drawText(renderer, "BACK", 760.0F, 548.0F, 2.0F);
  } else {
    drawText(renderer, "LOAD SELECTED", 452.0F, 548.0F, 2.0F);
    drawText(renderer, "DELETE SELECTED", 690.0F, 548.0F, 2.0F);
    drawText(renderer, "BACK", 1010.0F, 548.0F, 2.0F);
  }
}

void drawDeleteConfirmPanel(SDL_Renderer& renderer,
                            const ProductDeleteConfirmModel& model) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "DELETE MAP", 450.0F, 152.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "THIS MOVES THE MAP TO DELETED MAPS", 452.0F, 210.0F, 2.0F);
  drawText(renderer, "MAP", 452.0F, 260.0F, 2.0F);
  // sd2: the map value + status render the delete candidate's real title/status from the shared
  // resolveProductDeleteConfirmModel (same model the draw-list lane emits), not a static string.
  drawText(renderer, model.mapTitle, 452.0F, 292.0F, 2.0F);
  drawText(renderer, "STATUS", 452.0F, 350.0F, 2.0F);
  drawText(renderer, model.statusText, 452.0F, 382.0F, 2.0F);

  setColor(renderer, 236, 118, 86);
  drawText(renderer, "CONFIRM DELETE", 452.0F, 508.0F, 2.0F);
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "BACK", 760.0F, 508.0F, 2.0F);
}

void drawDevToolsPanel(SDL_Renderer& renderer, FrontendDevToolsCategory selected) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "DEV TOOLS", 450.0F, 128.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, devToolsFunctionKeyHintLabel(), 452.0F, 185.0F, 2.0F);

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
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "BACK", 850.0F, 394.0F, 2.0F);
}

void drawSettingsPanel(SDL_Renderer& renderer,
                       FrontendSettingsTab selected,
                       const ProductGameplayMovementTuning& movementTuning,
                       ProductGameplayMovementTuningField movementTuningField) {
  setColor(renderer, 226, 230, 211);
  drawText(renderer, "SETTINGS", 450.0F, 128.0F, 4.0F);
  setColor(renderer, 166, 184, 177);
  drawText(renderer, "SELECT A SETTINGS CATEGORY", 452.0F, 185.0F, 2.0F);

  float y = 230.0F;
  for (const FrontendSettingsTab tab : settingsTabOrder()) {
    drawPanelRow(renderer, settingsTabLabel(tab), tab == selected, 458.0F, y);
    y += 34.0F;
  }

  // branch-gate: BG-1211
  if (selected == FrontendSettingsTab::Gameplay) {
    drawMovementTuningRows(renderer, movementTuning, movementTuningField);
  } else {
    setColor(renderer, 126, 201, 176);
    drawText(renderer, "CURRENT", 850.0F, 230.0F, 2.0F);
    setColor(renderer, 166, 184, 177);
    drawText(renderer, "INPUT AUTO", 850.0F, 272.0F, 2.0F);
    drawText(renderer, "LOOK 1.000", 850.0F, 304.0F, 2.0F);
    drawText(renderer, "CAMERA FIRST PERSON", 850.0F, 336.0F, 2.0F);
  }
  setColor(renderer, 126, 201, 176);
  drawText(renderer, "BACK", 850.0F, 394.0F, 2.0F);
}

}  // namespace iggy3d

#endif
