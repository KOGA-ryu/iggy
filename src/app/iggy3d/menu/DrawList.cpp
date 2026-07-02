#include "app/iggy3d/menu/DrawList.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/SaveBrowser.hpp"
#include "app/frontend/StarterScreen.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/ui/Widget.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"

namespace iggy3d {
namespace {

constexpr float kHeaderY = 34.0F;
constexpr float kHeaderHeight = 58.0F;
constexpr float kRowX = 62.0F;
constexpr float kRowY = 150.0F;
constexpr float kRowWidth = 260.0F;
constexpr float kRowHeight = 36.0F;
constexpr float kRowStep = 52.0F;
constexpr float kContentX = 390.0F;
constexpr float kContentY = 92.0F;
constexpr float kContentWidth = 890.0F;
constexpr float kContentHeight = 556.0F;
constexpr float kFooterY = 648.0F;
constexpr float kFooterHeight = 72.0F;

enum class ProductStarterUiContext {
  Root,
  NewWorld,
  LoadSave,
  DeleteConfirm,
  Settings,
  DevTools,
  ChildPartial,
  UnsupportedScreen,
  MissingFrontend,
};

struct ProductUiToneDescriptor {
  ProductUiTone tone;
  std::string_view name;
  ProductUiColor color;
};

struct ProductUiPrimitiveKindDescriptor {
  ProductUiPrimitiveKind kind;
  std::string_view name;
};

struct ProductStarterUiBuildDescriptor {
  ProductStarterUiContext context;
  bool partial;
  std::string_view status;
  std::string_view reasonCode;
  std::string_view statusText;
};

constexpr std::array<ProductUiPrimitiveKindDescriptor, 5> kPrimitiveKindDescriptors{
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Panel, "panel"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Rect, "rect"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Text, "text"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Border, "border"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Highlight, "highlight"},
};

constexpr std::array<ProductUiToneDescriptor, 9> kToneDescriptors{
    ProductUiToneDescriptor{ProductUiTone::Surface, "surface", {0.07F, 0.09F, 0.11F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::SurfaceRaised,
                            "surface_raised",
                            {0.11F, 0.14F, 0.16F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::TextPrimary,
                            "text_primary",
                            {0.90F, 0.93F, 0.84F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::TextMuted,
                            "text_muted",
                            {0.64F, 0.70F, 0.67F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Accent, "accent", {0.49F, 0.79F, 0.69F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Selected,
                            "selected",
                            {0.25F, 0.37F, 0.34F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Disabled,
                            "disabled",
                            {0.31F, 0.35F, 0.35F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Border, "border", {0.18F, 0.23F, 0.25F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Status, "status", {0.50F, 0.56F, 0.53F, 1.0F}},
};

constexpr std::array<ProductStarterUiBuildDescriptor, 7> kStarterUiBuildDescriptors{
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::Root,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "starter root menu ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::NewWorld,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "new world selector ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::LoadSave,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "save slot selector ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::DeleteConfirm,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "delete confirmation ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::Settings,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "settings panel ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::DevTools,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "dev tools panel ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::ChildPartial,
        true,
        "product_ui_draw_list_partial",
        "starter_child_panel_unsupported",
        "unsupported starter child panel",
    },
};

constexpr std::array<ProductUiPrimitiveKind, 2> kRowBackgroundKinds{
    ProductUiPrimitiveKind::Rect,
    ProductUiPrimitiveKind::Highlight,
};

constexpr std::array<ProductUiTone, 2> kRowBackgroundTones{
    ProductUiTone::SurfaceRaised,
    ProductUiTone::Selected,
};

constexpr std::array<ProductUiTone, 2> kRowTextTones{
    ProductUiTone::Disabled,
    ProductUiTone::TextPrimary,
};

constexpr std::array<std::uint64_t, 2> kDisabledRowCountDeltas{1U, 0U};

const ProductUiPrimitiveKindDescriptor& primitiveKindDescriptor(
    ProductUiPrimitiveKind kind) {
  for (const ProductUiPrimitiveKindDescriptor& descriptor : kPrimitiveKindDescriptors) {
    // branch-gate: BG-1073
    if (descriptor.kind == kind) {
      return descriptor;
    }
  }
  return kPrimitiveKindDescriptors.front();
}

const ProductUiToneDescriptor& toneDescriptor(ProductUiTone tone) {
  for (const ProductUiToneDescriptor& descriptor : kToneDescriptors) {
    // branch-gate: BG-1073
    if (descriptor.tone == tone) {
      return descriptor;
    }
  }
  return kToneDescriptors.front();
}

const ProductStarterUiBuildDescriptor& starterUiBuildDescriptor(
    ProductStarterUiContext context) {
  for (const ProductStarterUiBuildDescriptor& descriptor : kStarterUiBuildDescriptors) {
    // branch-gate: BG-1073
    if (descriptor.context == context) {
      return descriptor;
    }
  }
  return kStarterUiBuildDescriptors.front();
}

ProductStarterUiContext starterUiContextFor(const ProductUiDrawListRequest& request) {
  // branch-gate: BG-1073
  if (request.frontend == nullptr) {
    return ProductStarterUiContext::MissingFrontend;
  }
  // branch-gate: BG-1073
  if (request.frontend->screen != FrontendScreen::Starter) {
    return ProductStarterUiContext::UnsupportedScreen;
  }
  // branch-gate: BG-1217
  switch (request.frontend->childScreen) {
    case FrontendScreen::Gameplay:
      return ProductStarterUiContext::Root;
    case FrontendScreen::NewWorld:
      return ProductStarterUiContext::NewWorld;
    case FrontendScreen::LoadSave:
      return ProductStarterUiContext::LoadSave;
    case FrontendScreen::DeleteConfirm:
      return ProductStarterUiContext::DeleteConfirm;
    case FrontendScreen::Settings:
      return ProductStarterUiContext::Settings;
    case FrontendScreen::StarterDevTools:
      return ProductStarterUiContext::DevTools;
    case FrontendScreen::BootStatus:
    case FrontendScreen::Starter:
    case FrontendScreen::Pause:
    case FrontendScreen::DevOverlay:
    case FrontendScreen::ExitConfirm:
      return ProductStarterUiContext::ChildPartial;
  }
  return ProductStarterUiContext::ChildPartial;
}

std::string makeStarterSemanticId(std::string_view suffix) {
  std::string semanticId = "starter.";
  semanticId.append(suffix);
  return semanticId;
}

std::string makeStarterRowSemanticId(FrontendAction action, std::string_view suffix) {
  std::string semanticId = "starter.row.";
  semanticId.append(frontendActionName(action));
  semanticId.push_back('.');
  semanticId.append(suffix);
  return semanticId;
}

void emitRect(ProductUiDrawList& list,
              ProductUiPrimitiveKind kind,
              ProductUiTone tone,
              ProductUiRect rect,
              std::string semanticId,
              FrontendAction action = FrontendAction::None,
              bool selected = false,
              bool enabled = true) {
  ProductUiPrimitive primitive;
  primitive.kind = kind;
  primitive.tone = tone;
  primitive.rect = rect;
  primitive.semanticId = std::move(semanticId);
  primitive.action = action;
  primitive.selected = selected;
  primitive.enabled = enabled;
  list.primitives.push_back(std::move(primitive));
  ++list.rectCount;
}

void emitText(ProductUiDrawList& list,
              ProductUiTone tone,
              ProductUiRect rect,
              std::string semanticId,
              std::string_view text,
              FrontendAction action = FrontendAction::None,
              bool selected = false,
              bool enabled = true) {
  ProductUiPrimitive primitive;
  primitive.kind = ProductUiPrimitiveKind::Text;
  primitive.tone = tone;
  primitive.rect = rect;
  primitive.semanticId = std::move(semanticId);
  primitive.text = std::string(text);
  primitive.action = action;
  primitive.selected = selected;
  primitive.enabled = enabled;
  list.primitives.push_back(std::move(primitive));
  ++list.textCount;
}

void emitStarterFrame(ProductUiDrawList& list) {
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::SurfaceRaised,
           {0.0F, 0.0F, 1280.0F, kHeaderHeight + kHeaderY},
           makeStarterSemanticId("header.panel"));
  emitText(list,
           ProductUiTone::TextPrimary,
           {46.0F, kHeaderY, 180.0F, 42.0F},
           makeStarterSemanticId("header.title"),
           "IGGY3D");
  emitText(list,
           ProductUiTone::Accent,
           {330.0F, 44.0F, 320.0F, 32.0F},
           makeStarterSemanticId("header.subtitle"),
           "OPENING MENU");
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::SurfaceRaised,
           {0.0F, 92.0F, 390.0F, 556.0F},
           makeStarterSemanticId("menu.panel"));
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::Surface,
           {kContentX, kContentY, kContentWidth, kContentHeight},
           makeStarterSemanticId("content.panel"));
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::SurfaceRaised,
           {0.0F, kFooterY, 1280.0F, kFooterHeight},
           makeStarterSemanticId("footer.panel"));
}

void emitStarterRows(ProductUiDrawList& list, const StarterScreenModel& model) {
  float y = kRowY;
  for (const FrontendAction action : model.actions) {
    const bool selected = action == model.selected;
    const bool enabled = starterActionEnabled(action, model.compatibleSaveCount);
    const std::size_t selectedIndex = static_cast<std::size_t>(selected);
    const std::size_t enabledIndex = static_cast<std::size_t>(enabled);
    const ProductUiPrimitiveKind rowKind = kRowBackgroundKinds[selectedIndex];
    const ProductUiTone rowTone = kRowBackgroundTones[selectedIndex];
    const ProductUiTone textTone = kRowTextTones[enabledIndex];
    emitRect(list,
             rowKind,
             rowTone,
             {kRowX - 12.0F, y - 8.0F, kRowWidth, kRowHeight},
             makeStarterRowSemanticId(action, "background"),
             action,
             selected,
             enabled);
    emitText(list,
             textTone,
             {kRowX + 18.0F, y, kRowWidth - 30.0F, 28.0F},
             makeStarterRowSemanticId(action, "label"),
             starterActionLabel(action),
             action,
             selected,
             enabled);
    list.disabledRowCount += kDisabledRowCountDeltas[enabledIndex];
    ++list.rowCount;
    y += kRowStep;
  }
}

void emitStarterStatus(ProductUiDrawList& list,
                       const FrontendState& frontend,
                       ProductStarterUiContext context) {
  const ProductStarterUiBuildDescriptor& descriptor =
      starterUiBuildDescriptor(context);
  emitText(list,
           ProductUiTone::Status,
           {44.0F, 674.0F, 960.0F, 24.0F},
           makeStarterSemanticId("status"),
           descriptor.statusText);
  emitText(list,
           ProductUiTone::TextMuted,
           {850.0F, 230.0F, 320.0F, 26.0F},
           makeStarterSemanticId("content.child_screen"),
           frontendScreenName(frontend.childScreen));
}

std::string selectionLabelFor(const WorldSetupDraft& draft) {
  const std::size_t dungeonCount = productBuiltinDungeonCatalog().size();
  const std::size_t selectedIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  // branch-gate: BG-1143
  if (selectedIndex >= dungeonCount) {
    return "CUSTOM";
  }
  return std::to_string(selectedIndex + 1U) + " / " +
         std::to_string(dungeonCount);
}

std::vector<std::string_view> asciiPreviewRows(std::string_view text) {
  std::vector<std::string_view> rows;
  std::size_t lineStart = 0;
  while (lineStart < text.size() && rows.size() < 8U) {
    std::size_t lineEnd = text.find('\n', lineStart);
    // branch-gate: BG-1144
    if (lineEnd == std::string_view::npos) {
      lineEnd = text.size();
    }
    const std::string_view line = text.substr(lineStart, lineEnd - lineStart);
    // branch-gate: BG-1144
    if (!line.empty()) {
      rows.push_back(line);
    }
    lineStart = lineEnd + 1U;
  }
  return rows;
}

std::string cursorAnnotatedAsciiRow(std::string_view line,
                                    std::uint64_t row,
                                    std::uint64_t cursorRow,
                                    std::uint64_t cursorColumn) {
  // branch-gate: BG-1144
  if (row != cursorRow || cursorColumn >= line.size()) {
    return std::string(line);
  }
  std::string annotated;
  annotated.reserve(line.size() + 2U);
  annotated.append(line.substr(0U, static_cast<std::size_t>(cursorColumn)));
  annotated.push_back('[');
  annotated.push_back(line[static_cast<std::size_t>(cursorColumn)]);
  annotated.push_back(']');
  annotated.append(line.substr(static_cast<std::size_t>(cursorColumn) + 1U));
  return annotated;
}

std::string lastGlyphText(const std::string& glyph) {
  // branch-gate: BG-1144
  if (glyph.empty() || glyph == "none") {
    return "LAST none";
  }
  return "LAST " + glyph;
}

std::string selectedGlyphText(const std::string& glyph) {
  // branch-gate: BG-1148
  if (glyph.empty()) {
    return "TOOL . FLOOR";
  }
  const char selected = glyph.front();
  // branch-gate: BG-1148
  switch (selected) {
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

void emitAsciiDraftRows(WidgetOutput& out,
                        const WorldSetupDraft& draft,
                        const ProductUiDrawListRequest& request) {
  const std::vector<std::string_view> rows = asciiPreviewRows(draft.asciiRoomText);
  float rowY = 500.0F;
  for (std::size_t row = 0; row < rows.size(); ++row) {
    // branch-gate: BG-1144
    const std::string rowText =
        request.dungeonDraftEditMode
            ? cursorAnnotatedAsciiRow(rows[row],
                                      static_cast<std::uint64_t>(row),
                                      request.dungeonDraftCursorRow,
                                      request.dungeonDraftCursorColumn)
            : std::string(rows[row]);
    emit(UiText{.rect = {850.0F, rowY, 360.0F, 22.0F},
                // branch-gate: BG-1144
                .tone = request.dungeonDraftEditMode &&
                                row == request.dungeonDraftCursorRow
                            ? ProductUiTone::Accent
                            : ProductUiTone::TextPrimary,
                .semanticId = makeStarterSemanticId("content.new_world.ascii_row_" +
                                                    std::to_string(row)),
                .text = rowText},
         out);
    rowY += 22.0F;
  }
}

void emitNewWorldContent(ProductUiDrawList& list,
                         const ProductUiDrawListRequest& request) {
  const WorldSetupDraft* draft = request.worldSetupDraft;
  // Rebuilt from the L1 widget layer (docs/ui/ui_architecture.md).
  // Emission-preserving: same rects/tones/ids/order as the hand-emit, so the
  // draw-list receipt stays byte-identical. Designated initializers name each
  // field so a semanticId/text or selected/enabled transposition is a compile
  // mismatch, not a silent swap. The CREATE and BACK action widgets also emit
  // hit regions.
  WidgetOutput out;
  emit(UiText{.rect = {452.0F, 150.0F, 360.0F, 42.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.title"),
              .text = "MAP BUILDER"},
       out);
  emit(UiText{.rect = {452.0F, 210.0F, 700.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.instructions"),
              .text =
                  request.dungeonDraftEditMode
                      ? "EDIT MODE   ARROWS MOVE   1# 2. 3P 4K 5$ 6E 7+ 8C R=RESET 9^ 0v -< => SELECT"
                      : "UP DOWN SELECT TEMPLATE   TAB EDIT   CONFIRM BUILD"},
       out);
  // branch-gate: BG-1143
  if (draft == nullptr) {
    emit(UiText{.rect = {452.0F, 272.0F, 620.0F, 26.0F},
                .tone = ProductUiTone::Status,
                .semanticId = makeStarterSemanticId("content.new_world.unavailable"),
                .text = "WORLD SETUP DRAFT UNAVAILABLE"},
         out);
    appendWidgetOutput(list, out);
    return;
  }

  emit(UiText{.rect = {452.0F, 260.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.dungeon_label"),
              .text = "TEMPLATE"},
       out);
  emit(UiText{.rect = {452.0F, 289.0F, 360.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.dungeon_value"),
              .text = draft->worldName},
       out);
  emit(UiText{.rect = {714.0F, 260.0F, 170.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.selection_label"),
              .text = "SELECTED"},
       out);
  emit(UiText{.rect = {714.0F, 289.0F, 150.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.selection_value"),
              .text = selectionLabelFor(*draft)},
       out);
  emit(UiText{.rect = {714.0F, 338.0F, 170.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.draft_label"),
              .text = "DRAFT"},
       out);
  emit(UiText{.rect = {714.0F, 367.0F, 150.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.draft_value"),
              // branch-gate: BG-1144
              .text = request.dungeonDraftModified ? "CUSTOM" : "TEMPLATE"},
       out);
  emit(UiText{.rect = {714.0F, 416.0F, 170.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.cursor_label"),
              .text = "CURSOR"},
       out);
  emit(UiText{.rect = {714.0F, 445.0F, 150.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.cursor_value"),
              .text = std::to_string(request.dungeonDraftCursorRow) + "," +
                      std::to_string(request.dungeonDraftCursorColumn)},
       out);
  emit(UiText{.rect = {452.0F, 338.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.room_id_label"),
              .text = "ASCII ROOM"},
       out);
  emit(UiText{.rect = {452.0F, 367.0F, 360.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.room_id_value"),
              .text = draft->asciiRoomId},
       out);
  emit(UiText{.rect = {452.0F, 416.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.source_label"),
              .text = "MAP SOURCE"},
       out);
  emit(UiText{.rect = {452.0F, 445.0F, 520.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.new_world.source_value"),
              .text = draft->asciiRoomSourceName},
       out);
  emit(UiText{.rect = {850.0F, 468.0F, 190.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId =
                  makeStarterSemanticId("content.new_world.ascii_preview_label"),
              .text = "ASCII PREVIEW"},
       out);
  emit(UiText{.rect = {1030.0F, 446.0F, 170.0F, 22.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.selected_glyph"),
              .text = selectedGlyphText(request.dungeonDraftSelectedGlyph)},
       out);
  emit(UiText{.rect = {1030.0F, 468.0F, 170.0F, 22.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.new_world.last_glyph"),
              .text = lastGlyphText(request.dungeonDraftLastGlyph)},
       out);
  emitAsciiDraftRows(out, *draft, request);
  emit(UiText{.rect = {452.0F, 508.0F, 260.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.new_world.create"),
              .text = request.dungeonDraftEditMode ? "TAB EXIT EDIT   CONFIRM BUILD"
                                                   : "CONFIRM TO BUILD",
              .action = FrontendAction::CreateAndEnter,
              .selected = false,
              .enabled = true},
       out);
  emit(UiText{.rect = {452.0F, 556.0F, 90.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.new_world.prev"),
              .text = "PREV"},
       out);
  emit(UiText{.rect = {570.0F, 556.0F, 90.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.new_world.next"),
              .text = "NEXT"},
       out);
  emit(UiText{.rect = {850.0F, 508.0F, 100.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.new_world.back"),
              .text = "BACK",
              .action = FrontendAction::Back,
              .selected = false,
              .enabled = true},
       out);
  appendWidgetOutput(list, out);
}

std::string saveSlotTitle(const SaveSlotRingItem& item) {
  // branch-gate: BG-1073
  return item.title.empty() ? item.id : item.title;
}

std::string saveSlotStatusText(const SaveSlotRingItem& item) {
  // branch-gate: BG-1073
  return item.enabled ? "READY" : item.status;
}

void emitLoadSaveAction(WidgetOutput& out,
                        const SaveSlotActionSpec& action,
                        float x,
                        ProductUiTone tone) {
  emit(UiText{.rect = {x, 548.0F, 240.0F, 26.0F},
              // branch-gate: BG-1073
              .tone = action.enabled ? tone : ProductUiTone::Disabled,
              .semanticId = makeStarterSemanticId(
                  "content.load_save.action." +
                  std::string(frontendActionName(action.action))),
              .text = std::string(action.label),
              .action = action.action,
              .enabled = action.enabled},
       out);
}

void emitLoadSaveContent(ProductUiDrawList& list,
                         const ProductUiDrawListRequest& request) {
  const FrontendState& frontend = *request.frontend;
  const SaveSlotList emptySlots;
  const SaveSlotList& slots =
      // branch-gate: BG-1073
      request.saves == nullptr ? emptySlots : request.saves->slots;
  const SaveBrowserModel browser =
      buildSaveBrowserModel(slots,
                            request.selectedSaveId,
                            frontend.saveBrowserMode);
  const bool deleteMode =
      frontend.saveBrowserMode == FrontendSaveBrowserMode::Delete;

  // Save browser rebuilt from the L1 widget layer (docs/ui/ui_architecture.md).
  // Emission-preserving: same rects/tones/ids/order as the hand-emit, so the
  // draw-list receipt stays byte-identical. Designated initializers name each
  // field so a semanticId/text or selected/enabled transposition is a compile
  // mismatch, not a silent swap. The action widgets also emit hit regions.
  WidgetOutput out;
  emit(UiText{.rect = {450.0F, 152.0F, 360.0F, 42.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.load_save.title"),
              // branch-gate: BG-1073
              .text = deleteMode ? "DELETE WORLD" : "LOAD MAP"},
       out);
  emit(UiText{.rect = {452.0F, 210.0F, 700.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.load_save.instructions"),
              // branch-gate: BG-1073
              .text = deleteMode ? "UP DOWN SELECT WORLD   CONFIRM DELETE"
                                 : "UP DOWN SELECT MAP   CONFIRM LOAD"},
       out);
  emit(UiText{.rect = {452.0F, 260.0F, 100.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.load_save.slots_label"),
              .text = "SLOTS"},
       out);
  emit(UiText{.rect = {558.0F, 260.0F, 120.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.load_save.slots_value"),
              .text = std::to_string(slots.slots.size())},
       out);
  emit(UiText{.rect = {714.0F, 260.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.load_save.compatible_label"),
              .text = "COMPATIBLE"},
       out);
  emit(UiText{.rect = {910.0F, 260.0F, 120.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.load_save.compatible_value"),
              .text = std::to_string(slots.compatibleCount)},
       out);

  float slotY = 318.0F;
  const std::size_t visibleSlotCount =
      std::min<std::size_t>(browser.ring.items.size(), 5U);
  // branch-gate: BG-1073
  if (visibleSlotCount == 0U) {
    emit(UiText{.rect = {452.0F, slotY, 360.0F, 26.0F},
                .tone = ProductUiTone::TextMuted,
                .semanticId = makeStarterSemanticId("content.load_save.empty"),
                .text = "NO COMPATIBLE SAVES"},
         out);
  }
  for (std::size_t i = 0; i < visibleSlotCount; ++i) {
    const SaveSlotRingItem& item = browser.ring.items[i];
    const bool selected = i == static_cast<std::size_t>(browser.ring.selectedIndex);
    emit(UiText{.rect = {452.0F, slotY, 360.0F, 26.0F},
                // branch-gate: BG-1073
                .tone = item.enabled ? ProductUiTone::TextPrimary
                                     : ProductUiTone::Disabled,
                .semanticId = makeStarterSemanticId("content.load_save.slot_" +
                                                    std::to_string(i) + ".title"),
                .text = saveSlotTitle(item),
                .selected = selected,
                .enabled = item.enabled},
         out);
    emit(UiText{.rect = {850.0F, slotY, 260.0F, 26.0F},
                // branch-gate: BG-1073
                .tone = item.enabled ? ProductUiTone::TextMuted
                                     : ProductUiTone::Disabled,
                .semanticId = makeStarterSemanticId("content.load_save.slot_" +
                                                    std::to_string(i) + ".status"),
                .text = saveSlotStatusText(item),
                .selected = selected,
                .enabled = item.enabled},
         out);
    slotY += 38.0F;
  }

  // branch-gate: BG-1073
  if (deleteMode) {
    emitLoadSaveAction(out, browser.actions[0], 452.0F, ProductUiTone::Accent);
    emitLoadSaveAction(out, browser.actions[1], 760.0F, ProductUiTone::Accent);
  } else {
    emitLoadSaveAction(out, browser.actions[0], 452.0F, ProductUiTone::Accent);
    emitLoadSaveAction(out, browser.actions[1], 690.0F, ProductUiTone::Accent);
    emitLoadSaveAction(out, browser.actions[2], 1010.0F, ProductUiTone::Accent);
  }
  appendWidgetOutput(list, out);
}

void emitDeleteConfirmContent(ProductUiDrawList& list,
                              const ProductDeleteConfirmModel& model) {
  // First screen rebuilt from the L1 widget layer (docs/ui/ui_architecture.md).
  // NOT emission-preserving (sd2): the map value and status value are now DYNAMIC — they render
  // the delete candidate's real title/status from resolveProductDeleteConfirmModel (with a fixed
  // fallback when no candidate is set) instead of the old static "SELECTED MAP"/"CONFIRM OPEN".
  // Every other primitive (rects/tones/semanticIds, and the interactive confirm/back hit regions)
  // is unchanged. Both presentation lanes render `model` from the same shared resolver.
  WidgetOutput out;
  emit(UiText{.rect = {450.0F, 152.0F, 360.0F, 42.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.delete_confirm.title"),
              .text = "DELETE MAP"},
       out);
  emit(UiText{.rect = {452.0F, 210.0F, 620.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId =
                  makeStarterSemanticId("content.delete_confirm.instructions"),
              .text = "THIS MOVES THE MAP TO DELETED MAPS"},
       out);
  emit(UiText{.rect = {452.0F, 260.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.delete_confirm.map_label"),
              .text = "MAP"},
       out);
  emit(UiText{.rect = {452.0F, 292.0F, 320.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.delete_confirm.map_value"),
              .text = model.mapTitle},
       out);
  emit(UiText{.rect = {452.0F, 350.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId =
                  makeStarterSemanticId("content.delete_confirm.status_label"),
              .text = "STATUS"},
       out);
  emit(UiText{.rect = {452.0F, 382.0F, 320.0F, 26.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId =
                  makeStarterSemanticId("content.delete_confirm.status_value"),
              .text = model.statusText},
       out);
  emit(UiText{.rect = {452.0F, 508.0F, 260.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.delete_confirm.confirm"),
              .text = "CONFIRM DELETE",
              .action = FrontendAction::Delete},
       out);
  emit(UiText{.rect = {760.0F, 508.0F, 100.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.delete_confirm.back"),
              .text = "BACK",
              .action = FrontendAction::Back},
       out);
  appendWidgetOutput(list, out);
}

void emitSettingsContent(ProductUiDrawList& list,
                         FrontendSettingsTab selectedTab) {
  // Rebuilt from the L1 widget layer (docs/ui/ui_architecture.md).
  // Emission-preserving: same rects/tones/ids/order as the hand-emit, so the
  // draw-list receipt stays byte-identical. Designated initializers name each
  // field so a semanticId/text or selected/enabled transposition is a compile
  // mismatch, not a silent swap. The BACK action widget also emits a hit region.
  WidgetOutput out;
  emit(UiText{.rect = {450.0F, 128.0F, 360.0F, 42.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.settings.title"),
              .text = "SETTINGS"},
       out);
  emit(UiText{.rect = {452.0F, 185.0F, 520.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.settings.instructions"),
              .text = "SELECT A SETTINGS CATEGORY"},
       out);

  float y = 230.0F;
  for (const FrontendSettingsTab tab : settingsTabOrder()) {
    const bool selected = tab == selectedTab;
    emit(UiText{.rect = {458.0F, y, 280.0F, 26.0F},
                // branch-gate: BG-1073
                .tone = selected ? ProductUiTone::Accent : ProductUiTone::TextMuted,
                .semanticId = makeStarterSemanticId(
                    "content.settings.tab." +
                    std::string(frontendSettingsTabName(tab))),
                .text = std::string(frontendSettingsTabName(tab)),
                .selected = selected,
                .enabled = true},
         out);
    y += 34.0F;
  }

  emit(UiText{.rect = {850.0F, 230.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.settings.current_label"),
              .text = "CURRENT"},
       out);
  emit(UiText{.rect = {850.0F, 272.0F, 300.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.settings.current_input"),
              .text = "INPUT AUTO"},
       out);
  emit(UiText{.rect = {850.0F, 304.0F, 300.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.settings.current_look"),
              .text = "LOOK 1.000"},
       out);
  emit(UiText{.rect = {850.0F, 336.0F, 300.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.settings.current_camera"),
              .text = "CAMERA FIRST PERSON"},
       out);
  emit(UiText{.rect = {850.0F, 394.0F, 100.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.settings.back"),
              .text = "BACK",
              .action = FrontendAction::Back,
              .selected = false,
              .enabled = true},
       out);
  appendWidgetOutput(list, out);
}

void emitDevToolsContent(ProductUiDrawList& list,
                         FrontendDevToolsCategory selectedCategory) {
  const DevToolsMenuModel model = buildDevToolsMenuModel(selectedCategory);
  // Rebuilt from the L1 widget layer (docs/ui/ui_architecture.md).
  // Emission-preserving: same rects/tones/ids/order as the hand-emit, so the
  // draw-list receipt stays byte-identical. Designated initializers name each
  // field so a semanticId/text or selected/enabled transposition is a compile
  // mismatch, not a silent swap. The BACK action widget also emits a hit region.
  WidgetOutput out;
  emit(UiText{.rect = {450.0F, 128.0F, 360.0F, 42.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = makeStarterSemanticId("content.dev_tools.title"),
              .text = "DEV TOOLS"},
       out);
  emit(UiText{.rect = {452.0F, 185.0F, 760.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.dev_tools.function_keys"),
              .text = std::string(devToolsFunctionKeyHintLabel())},
       out);

  float y = 230.0F;
  for (const FrontendDevToolsCategory category : model.categories) {
    const bool selected = category == model.selected;
    emit(UiText{.rect = {458.0F, y, 280.0F, 26.0F},
                // branch-gate: BG-1073
                .tone = selected ? ProductUiTone::Accent : ProductUiTone::TextMuted,
                .semanticId = makeStarterSemanticId(
                    "content.dev_tools.category." +
                    std::string(frontendDevToolsCategoryName(category))),
                .text = std::string(devToolsCategoryLabel(category)),
                .selected = selected,
                .enabled = true},
         out);
    y += 34.0F;
  }

  emit(UiText{.rect = {850.0F, 230.0F, 180.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.dev_tools.readouts_label"),
              .text = "READOUTS"},
       out);
  emit(UiText{.rect = {850.0F, 272.0F, 300.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.dev_tools.runtime_state"),
              .text = "RUNTIME STATE"},
       out);
  emit(UiText{.rect = {850.0F, 304.0F, 300.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.dev_tools.input_owner"),
              .text = "INPUT OWNER"},
       out);
  emit(UiText{.rect = {850.0F, 336.0F, 300.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = makeStarterSemanticId("content.dev_tools.renderer_status"),
              .text = "RENDERER STATUS"},
       out);
  emit(UiText{.rect = {850.0F, 394.0F, 100.0F, 26.0F},
              .tone = ProductUiTone::Accent,
              .semanticId = makeStarterSemanticId("content.dev_tools.back"),
              .text = "BACK",
              .action = FrontendAction::Back,
              .selected = false,
              .enabled = true},
       out);
  appendWidgetOutput(list, out);
}

ProductUiDrawList rejectedList(const ProductUiDrawListRequest& request,
                               std::string_view status,
                               std::string_view reasonCode) {
  ProductUiDrawList list;
  list.ready = false;
  list.partial = false;
  list.status = std::string(status);
  list.reasonCode = std::string(reasonCode);
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  return list;
}

}  // namespace

std::string_view productUiPrimitiveKindName(ProductUiPrimitiveKind kind) {
  return primitiveKindDescriptor(kind).name;
}

std::string_view productUiToneName(ProductUiTone tone) {
  return toneDescriptor(tone).name;
}

ProductUiColor productUiToneColor(ProductUiTone tone) {
  return toneDescriptor(tone).color;
}

namespace {

// The System theme is exactly the long-standing descriptor palette, built from the
// same table so it can never drift from productUiToneColor(tone) — every existing
// receipt keeps its colours.
ProductUiTheme makeSystemTheme() {
  constexpr std::array<ProductUiTone, 9> kAllTones{
      ProductUiTone::Surface,   ProductUiTone::SurfaceRaised,
      ProductUiTone::TextPrimary, ProductUiTone::TextMuted,
      ProductUiTone::Accent,    ProductUiTone::Selected,
      ProductUiTone::Disabled,  ProductUiTone::Border,
      ProductUiTone::Status};
  ProductUiTheme theme;
  for (const ProductUiTone tone : kAllTones) {
    theme.tones[static_cast<std::size_t>(tone)] = toneDescriptor(tone).color;
  }
  return theme;
}

// The Journal (Moleskine) palette: light cream page, dark graphite ink, leather
// cover, brass accents — the same tones the System theme resolves dark, resolved
// as an aged notebook. Tune freely; this is data.
ProductUiTheme makeJournalTheme() {
  ProductUiTheme theme;
  auto& t = theme.tones;
  t[static_cast<std::size_t>(ProductUiTone::Surface)] = {0.169F, 0.125F, 0.094F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::SurfaceRaised)] = {0.925F, 0.890F, 0.804F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::TextPrimary)] = {0.247F, 0.224F, 0.180F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::TextMuted)] = {0.420F, 0.384F, 0.322F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::Accent)] = {0.725F, 0.541F, 0.235F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::Selected)] = {0.851F, 0.780F, 0.627F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::Disabled)] = {0.604F, 0.565F, 0.471F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::Border)] = {0.761F, 0.706F, 0.549F, 1.0F};
  t[static_cast<std::size_t>(ProductUiTone::Status)] = {0.478F, 0.427F, 0.329F, 1.0F};
  return theme;
}

}  // namespace

const ProductUiTheme& productUiTheme(ProductUiThemeId id) {
  static const ProductUiTheme kSystem = makeSystemTheme();
  static const ProductUiTheme kJournal = makeJournalTheme();
  // branch-gate: BG-1217
  switch (id) {
    case ProductUiThemeId::Journal:
      return kJournal;
    case ProductUiThemeId::System:
      break;
  }
  return kSystem;
}

ProductUiColor productUiToneColor(ProductUiTone tone,
                                  const ProductUiTheme& theme) {
  return theme.tones[static_cast<std::size_t>(tone)];
}

ProductUiDrawList buildProductStarterUiDrawList(
    const ProductUiDrawListRequest& request) {
  const ProductStarterUiContext context = starterUiContextFor(request);
  // branch-gate: BG-1073
  if (context == ProductStarterUiContext::MissingFrontend) {
    return rejectedList(request,
                        "product_ui_draw_list_not_ready",
                        "product_ui_draw_list_missing_frontend");
  }
  // branch-gate: BG-1073
  if (context == ProductStarterUiContext::UnsupportedScreen) {
    return rejectedList(request,
                        "product_ui_draw_list_unsupported_screen",
                        "product_ui_draw_list_requires_starter");
  }

  const FrontendState& frontend = *request.frontend;
  const StarterScreenModel model =
      buildStarterScreenModel(request.compatibleSaveCount, frontend.selectedAction);
  const ProductStarterUiBuildDescriptor& descriptor =
      starterUiBuildDescriptor(context);
  ProductUiDrawList list;
  list.ready = true;
  list.partial = descriptor.partial;
  list.status = std::string(descriptor.status);
  list.reasonCode = std::string(descriptor.reasonCode);
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  list.selectedAction = std::string(frontendActionName(model.selected));

  emitStarterFrame(list);
  emitStarterRows(list, model);
  // branch-gate: BG-1217
  switch (context) {
    case ProductStarterUiContext::NewWorld:
      emitNewWorldContent(list, request);
      break;
    case ProductStarterUiContext::LoadSave:
      emitLoadSaveContent(list, request);
      break;
    case ProductStarterUiContext::DeleteConfirm: {
      // Resolve the panel text from the live catalog + the tracked candidate id (one shared
      // source; the SDL view resolves the same model). A null catalog falls back deterministically.
      const ProductSaveBridgeResult emptyCatalog;
      emitDeleteConfirmContent(
          list, resolveProductDeleteConfirmModel(
                    request.deleteCandidateId,
                    request.saves != nullptr ? *request.saves : emptyCatalog));
      break;
    }
    case ProductStarterUiContext::Settings:
      emitSettingsContent(list, request.settingsTab);
      break;
    case ProductStarterUiContext::DevTools:
      emitDevToolsContent(list, frontend.devToolsCategory);
      break;
    case ProductStarterUiContext::Root:
    case ProductStarterUiContext::ChildPartial:
    case ProductStarterUiContext::UnsupportedScreen:
    case ProductStarterUiContext::MissingFrontend:
      emitStarterStatus(list, frontend, context);
      break;
  }
  list.primitiveCount = list.primitives.size();
  return list;
}

ProductDeleteConfirmModel resolveProductDeleteConfirmModel(
    std::string_view candidateId, const ProductSaveBridgeResult& saves) {
  // Fallback when nothing is meaningfully selected — never blank, never garbage.
  ProductDeleteConfirmModel model{"NO MAP SELECTED", "NO MAP SELECTED"};
  if (candidateId.empty() || candidateId == "none") {
    return model;
  }
  for (const SaveSlotPreview& slot : saves.slots.slots) {
    if (slot.id == candidateId) {
      // Prefer the human title; fall back to the id so a titleless slot still names something.
      model.mapTitle = !slot.displayTitle.empty() ? slot.displayTitle : slot.id;
      model.statusText = "READY TO DELETE";
      return model;
    }
  }
  return model;  // candidate set but not in the catalog -> fallback
}

ProductUiDrawListRequest buildProductStarterUiDrawListRequest(
    const FrontendState& frontend,
    const ProductSaveBridgeResult& saves,
    const WorldSetupDraft& worldSetupDraft,
    FrontendSettingsTab settingsTab,
    const ProductStarterUiDraftState& draft) {
  ProductUiDrawListRequest request;
  request.frontend = &frontend;
  request.compatibleSaveCount = saves.slots.compatibleCount;
  request.virtualWidth = 1280U;
  request.virtualHeight = 720U;
  request.worldSetupDraft = &worldSetupDraft;
  request.dungeonDraftEditMode = draft.dungeonDraftEditMode;
  request.dungeonDraftModified = draft.dungeonDraftModified;
  request.dungeonDraftCursorRow = draft.dungeonDraftCursorRow;
  request.dungeonDraftCursorColumn = draft.dungeonDraftCursorColumn;
  request.dungeonDraftSelectedGlyph = draft.dungeonDraftSelectedGlyph;
  request.dungeonDraftLastGlyph = draft.dungeonDraftLastGlyph;
  request.settingsTab = settingsTab;
  request.saves = &saves;
  request.selectedSaveId = draft.selectedSaveId;
  request.deleteCandidateId = draft.deleteCandidateId;
  return request;
}

}  // namespace iggy3d
