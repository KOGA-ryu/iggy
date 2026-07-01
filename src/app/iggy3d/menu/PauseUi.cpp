#include "app/iggy3d/menu/PauseUi.hpp"

#include <string>

#include "app/iggy3d/ui/Widget.hpp"

namespace iggy3d {
namespace {

// The pause "page" — a centered journal leaf over the paused world.
constexpr float kPanelX = 440.0F;
constexpr float kPanelY = 140.0F;
constexpr float kPanelW = 400.0F;
constexpr float kPanelH = 440.0F;
constexpr float kRowX = 476.0F;
constexpr float kRowStartY = 268.0F;
constexpr float kRowStep = 52.0F;

std::string pauseId(std::string_view suffix) {
  std::string id = "pause.";
  id.append(suffix);
  return id;
}

std::string pauseRowId(FrontendAction action, std::string_view suffix) {
  std::string id = "pause.row.";
  id.append(frontendActionName(action));
  id.push_back('.');
  id.append(suffix);
  return id;
}

}  // namespace

std::string_view productPauseActionLabel(FrontendAction action) {
  // branch-gate: BG-1217
  switch (action) {
    case FrontendAction::Resume:
      return "Resume";
    case FrontendAction::Save:
      return "Save";
    case FrontendAction::SaveAndExit:
      return "Save & Exit";
    case FrontendAction::ReturnToTitle:
      return "Return to Title";
    case FrontendAction::ExitGame:
      return "Quit Game";
    case FrontendAction::Settings:
      return "Settings";
    case FrontendAction::DevTools:
      return "Dev Tools";
    case FrontendAction::LoadSave:
      return "Load Map";
    case FrontendAction::EditRoom:
      return "Edit Room";
    case FrontendAction::LeaveEditor:
      return "Leave Editor";
    default:
      return frontendActionName(action);
  }
}

ProductUiDrawList buildProductPauseUiDrawList(
    const ProductPauseUiRequest& request) {
  // branch-gate: BG-1073
  if (request.model == nullptr) {
    ProductUiDrawList list;
    list.ready = false;
    list.status = "product_pause_ui_not_ready";
    list.reasonCode = "product_pause_ui_missing_model";
    list.virtualWidth = request.virtualWidth;
    list.virtualHeight = request.virtualHeight;
    list.theme = ProductUiThemeId::Journal;
    return list;
  }
  const PauseMenuModel& model = *request.model;

  ProductUiDrawList list;
  list.ready = true;
  list.status = "product_pause_ui_ready";
  list.reasonCode = "product_pause_ui_ready";
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  list.theme = ProductUiThemeId::Journal;  // pause = open your journal
  list.selectedAction = std::string(frontendActionName(model.selected));

  WidgetOutput out;
  emit(UiPanel{.rect = {kPanelX, kPanelY, kPanelW, kPanelH},
               .kind = ProductUiPrimitiveKind::Panel,
               .tone = ProductUiTone::SurfaceRaised,
               .semanticId = pauseId("page")},
       out);
  emit(UiText{.rect = {kRowX, 190.0F, kPanelW - 72.0F, 40.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = pauseId("title"),
              .text = "PAUSED"},
       out);
  emit(UiPanel{.rect = {kRowX, 226.0F, 120.0F, 3.0F},
               .kind = ProductUiPrimitiveKind::Border,
               .tone = ProductUiTone::Border,
               .semanticId = pauseId("title.rule")},
       out);

  float y = kRowStartY;
  for (const MenuRowModel& row : model.rows) {
    const bool selected = row.action == model.selected;
    // branch-gate: BG-1073
    if (selected) {
      emit(UiPanel{.rect = {kRowX - 14.0F, y - 8.0F, kPanelW - 44.0F, 40.0F},
                   .kind = ProductUiPrimitiveKind::Highlight,
                   .tone = ProductUiTone::Selected,
                   .semanticId = pauseRowId(row.action, "highlight"),
                   .selected = true},
           out);
    }
    emit(UiText{.rect = {kRowX, y, kPanelW - 72.0F, 28.0F},
                // branch-gate: BG-1073
                .tone = row.enabled ? (selected ? ProductUiTone::TextPrimary
                                                : ProductUiTone::TextMuted)
                                    : ProductUiTone::Disabled,
                .semanticId = pauseRowId(row.action, "label"),
                .text = std::string(productPauseActionLabel(row.action)),
                .action = row.action,
                .selected = selected,
                .enabled = row.enabled},
         out);
    y += kRowStep;
  }

  appendWidgetOutput(list, out);
  list.primitiveCount = list.primitives.size();
  return list;
}

}  // namespace iggy3d
