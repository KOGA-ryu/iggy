#include "app/iggy3d/creative/ui/UiDrawList.hpp"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d {
namespace {

constexpr float kOverlayX = 24.0F;
constexpr float kOverlayY = 24.0F;
constexpr float kPanelPadding = 10.0F;
constexpr float kRowHeight = 28.0F;
constexpr float kPanelGap = 8.0F;
constexpr float kToolsPanelWidth = 360.0F;
constexpr float kInfoPanelWidth = 560.0F;
constexpr float kTargetPanelWidth = 620.0F;
constexpr float kGlyphAdvance = 12.0F;

struct PanelLayoutCursor {
  float leftY = kOverlayY;
  float rightY = kOverlayY;
};

struct PanelLayout {
  float x = kOverlayX;
  float y = kOverlayY;
  float width = kToolsPanelWidth;
};

[[nodiscard]] bool hasFlag(creative::CreativeUiRowFlagMask flags,
                           creative::CreativeUiRowFlagMask flag) noexcept {
  return (flags & flag) != 0;
}

[[nodiscard]] bool isRowEnabled(const creative::CreativeUiRow& row) noexcept {
  return hasFlag(row.flags, creative::kCreativeUiRowFlagEnabled);
}

[[nodiscard]] std::string_view panelName(
    creative::CreativeUiPanelKind kind) noexcept {
  switch (kind) {
    case creative::CreativeUiPanelKind::Tools:
      return "tools";
    case creative::CreativeUiPanelKind::Create:
      return "create";
    case creative::CreativeUiPanelKind::Status:
      return "status";
    case creative::CreativeUiPanelKind::Selection:
      return "selection";
    case creative::CreativeUiPanelKind::Measurement:
      return "measurement";
    case creative::CreativeUiPanelKind::Ghost:
      return "ghost";
    case creative::CreativeUiPanelKind::Snap:
      return "snap";
  }
  return "unknown";
}

[[nodiscard]] std::string_view toolName(creative::Tool tool) noexcept {
  switch (tool) {
    case creative::Tool::Select:
      return "Select";
    case creative::Tool::Move:
      return "Move";
    case creative::Tool::Measure:
      return "Measure";
    case creative::Tool::Navigate:
      return "Navigate";
  }
  return "Unknown";
}

[[nodiscard]] std::string_view snapModeName(
    creative::CreativeSnapMode mode) noexcept {
  switch (mode) {
    case creative::CreativeSnapMode::Disabled:
      return "Disabled";
    case creative::CreativeSnapMode::Grid:
      return "Grid";
  }
  return "Unknown";
}

[[nodiscard]] std::string axisMaskName(
    creative::CreativeSnapAxisMask axes) {
  if (axes == creative::kCreativeSnapAxisXY) {
    return "XY";
  }
  if (axes == creative::kCreativeSnapAxisX) {
    return "X";
  }
  if (axes == creative::kCreativeSnapAxisY) {
    return "Y";
  }
  if (axes == creative::kCreativeSnapAxisNone) {
    return "None";
  }
  return "Mixed";
}

[[nodiscard]] std::string formatDouble(double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  return stream.str();
}

[[nodiscard]] std::string makeSemanticId(std::string_view prefix,
                                         std::string_view stem) {
  std::string id = "creative.";
  id.append(prefix);
  id.push_back('.');
  id.append(stem);
  return id;
}

[[nodiscard]] std::string rowSemanticId(const creative::CreativeUiRow& row,
                                        std::size_t rowIndex) {
  std::string id = makeSemanticId("row", panelName(row.panel));
  id.push_back('.');
  if (!row.id.empty()) {
    id.append(row.id);
  } else {
    id.append(std::to_string(rowIndex));
  }
  return id;
}

void appendVisibilityText(std::string& text,
                          const creative::CreativeUiRow& row) {
  text.append(" visible=");
  if (!hasFlag(row.flags, creative::kCreativeUiRowFlagObjectKnown)) {
    text.append("unknown");
    return;
  }

  text.append(hasFlag(row.flags, creative::kCreativeUiRowFlagObjectVisible)
                  ? "true"
                  : "false");
}

[[nodiscard]] std::string boolFlagText(const creative::CreativeUiRow& row,
                                       creative::CreativeUiRowFlagMask flag) {
  if (!hasFlag(row.flags, creative::kCreativeUiRowFlagObjectKnown)) {
    return "unknown";
  }
  return hasFlag(row.flags, flag) ? "true" : "false";
}

void appendVec3Text(std::string& text, double x, double y, double z) {
  text.push_back('(');
  text.append(formatDouble(x));
  text.push_back(',');
  text.append(formatDouble(y));
  text.push_back(',');
  text.append(formatDouble(z));
  text.push_back(')');
}

[[nodiscard]] std::string rowText(const creative::CreativeUiRow& row) {
  std::string text(row.label.empty() ? row.id : row.label);
  switch (row.kind) {
    case creative::CreativeUiRowKind::ToolButton:
      text = std::string(toolName(row.tool));
      if (hasFlag(row.flags, creative::kCreativeUiRowFlagActive)) {
        text.append(" (active)");
      }
      break;
    case creative::CreativeUiRowKind::CreateObject:
    case creative::CreativeUiRowKind::RebuildRoom:
    case creative::CreativeUiRowKind::ToolUndo:
      break;
    case creative::CreativeUiRowKind::StatusSummary:
      text = "Creative: ";
      if (hasFlag(row.flags, creative::kCreativeUiRowFlagActive)) {
        text.append("Measuring");
      } else if (hasFlag(row.flags, creative::kCreativeUiRowFlagHasTarget)) {
        text.append("Selected");
      } else if (hasFlag(row.flags, creative::kCreativeUiRowFlagHasMeasurement)) {
        text.append("Measurement");
      } else {
        text.append("Ready");
      }
      break;
    case creative::CreativeUiRowKind::SelectedTarget:
      text = "Selected: count=";
      text.append(std::to_string(row.data0));
      text.append(" primary=");
      text.append(std::to_string(row.target.value));
      appendVisibilityText(text, row);
      break;
    case creative::CreativeUiRowKind::InspectorEmpty:
      text = "No selection - click an object";
      break;
    case creative::CreativeUiRowKind::InspectorKind:
      text = "Kind: ";
      text.append(creative::toString(row.objectKind));
      break;
    case creative::CreativeUiRowKind::InspectorId:
      text = "Id: ";
      text.append(std::to_string(row.data0));
      break;
    case creative::CreativeUiRowKind::InspectorName:
      text = "Name: ";
      text.append(row.name);
      break;
    case creative::CreativeUiRowKind::InspectorVisible:
      text = "Visible: ";
      text.append(
          boolFlagText(row, creative::kCreativeUiRowFlagObjectVisible));
      break;
    case creative::CreativeUiRowKind::InspectorLocked:
      text = "Locked: ";
      text.append(boolFlagText(row, creative::kCreativeUiRowFlagObjectLocked));
      break;
    case creative::CreativeUiRowKind::InspectorDeleteSelected:
    case creative::CreativeUiRowKind::InspectorGenerateRoomShell:
    case creative::CreativeUiRowKind::InspectorRemoveRoomShell:
      break;
    case creative::CreativeUiRowKind::InspectorBounds:
      // min/max are shown; size is carried in the row fields (max-min) for the
      // v1.5 numeric editor and stays out of the fixed-width line (TD-9).
      text = "Bounds: min";
      appendVec3Text(text, row.primaryX, row.primaryY, row.primaryZ);
      text.append(" max");
      appendVec3Text(text, row.secondaryX, row.secondaryY, row.secondaryZ);
      break;
    case creative::CreativeUiRowKind::InspectorPosition:
      text = "Position: ";
      appendVec3Text(text, row.primaryX, row.primaryY, row.primaryZ);
      break;
    case creative::CreativeUiRowKind::InspectorLayer:
      text = "Layer: ";
      text.append(std::to_string(row.data1));
      break;
    case creative::CreativeUiRowKind::MeasurementState:
      text = "Measure: ";
      text.append(hasFlag(row.flags, creative::kCreativeUiRowFlagActive)
                      ? "active"
                      : "completed");
      text.append(" samples=");
      text.append(std::to_string(row.data0));
      break;
    case creative::CreativeUiRowKind::MeasurementStartPoint:
    case creative::CreativeUiRowKind::MeasurementCurrentPoint:
      text = row.kind == creative::CreativeUiRowKind::MeasurementStartPoint
                 ? "Start: ("
                 : "Current: (";
      text.append(formatDouble(row.primaryX));
      text.push_back(',');
      text.append(formatDouble(row.primaryY));
      text.append(") target=");
      text.append(std::to_string(row.target.value));
      break;
    case creative::CreativeUiRowKind::GhostPreview:
      text = "Ghost: (";
      text.append(formatDouble(row.primaryX));
      text.push_back(',');
      text.append(formatDouble(row.primaryY));
      text.append(")->(");
      text.append(formatDouble(row.secondaryX));
      text.push_back(',');
      text.append(formatDouble(row.secondaryY));
      text.append(") target=");
      text.append(std::to_string(row.target.value));
      break;
    case creative::CreativeUiRowKind::SnapSettings:
      text = "Snap: ";
      text.append(snapModeName(
          static_cast<creative::CreativeSnapMode>(row.data0)));
      text.push_back(' ');
      text.append(axisMaskName(static_cast<creative::CreativeSnapAxisMask>(
          row.data1)));
      text.push_back(' ');
      text.append(formatDouble(row.primaryX));
      text.push_back('x');
      text.append(formatDouble(row.primaryY));
      if (row.secondaryX != 0.0 || row.secondaryY != 0.0) {
        text.append(" @ ");
        text.append(formatDouble(row.secondaryX));
        text.push_back(',');
        text.append(formatDouble(row.secondaryY));
      }
      break;
  }
  return text;
}

[[nodiscard]] float panelHeight(std::size_t rowCount) noexcept {
  return (2.0F * kPanelPadding) +
         static_cast<float>(rowCount) * kRowHeight;
}

[[nodiscard]] std::string fitTextToWidth(std::string text,
                                         float width) {
  if (width <= kGlyphAdvance) {
    return {};
  }
  const std::size_t maxChars =
      static_cast<std::size_t>(std::max(1.0F, width / kGlyphAdvance));
  if (text.size() <= maxChars) {
    return text;
  }
  if (maxChars <= 3U) {
    text.resize(maxChars);
    return text;
  }
  text.resize(maxChars - 3U);
  text.append("...");
  return text;
}

[[nodiscard]] float constrainedPanelWidth(float desiredWidth,
                                          std::uint32_t virtualWidth) noexcept {
  const float maxWidth = std::max(120.0F,
                                  static_cast<float>(virtualWidth) -
                                      (2.0F * kOverlayX));
  return std::min(desiredWidth, maxWidth);
}

[[nodiscard]] PanelLayout panelLayoutFor(
    const creative::CreativeUiPanel& panel,
    std::uint32_t virtualWidth,
    std::uint32_t virtualHeight,
    PanelLayoutCursor& cursor) noexcept {
  const float height = panelHeight(panel.rowCount);
  PanelLayout layout;

  switch (panel.kind) {
    case creative::CreativeUiPanelKind::Tools:
    case creative::CreativeUiPanelKind::Create:
      layout = {kOverlayX,
                cursor.leftY,
                constrainedPanelWidth(kToolsPanelWidth, virtualWidth)};
      cursor.leftY += height + kPanelGap;
      return layout;
    case creative::CreativeUiPanelKind::Status:
      layout = {kOverlayX,
                std::max(cursor.leftY,
                         static_cast<float>(virtualHeight) - kOverlayY -
                             (2.0F * height) - kPanelGap),
                constrainedPanelWidth(kInfoPanelWidth, virtualWidth)};
      return layout;
    case creative::CreativeUiPanelKind::Snap:
      layout = {kOverlayX,
                std::max(cursor.leftY,
                         static_cast<float>(virtualHeight) - kOverlayY -
                             height),
                constrainedPanelWidth(kInfoPanelWidth, virtualWidth)};
      return layout;
    case creative::CreativeUiPanelKind::Selection:
    case creative::CreativeUiPanelKind::Measurement:
    case creative::CreativeUiPanelKind::Ghost: {
      layout.width = constrainedPanelWidth(kTargetPanelWidth, virtualWidth);
      layout.x =
          std::max(kOverlayX,
                   static_cast<float>(virtualWidth) - kOverlayX - layout.width);
      layout.y = cursor.rightY;
      cursor.rightY += height + kPanelGap;
      return layout;
    }
  }

  return layout;
}

[[nodiscard]] std::size_t visiblePanelCount(
    const creative::CreativeUiModel& model) noexcept {
  std::size_t count = 0;
  for (const creative::CreativeUiPanel& panel : model.panels) {
    if (panel.visible) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] std::size_t visibleRowCount(
    const creative::CreativeUiModel& model) noexcept {
  std::size_t count = 0;
  for (const creative::CreativeUiPanel& panel : model.panels) {
    if (!panel.visible) {
      continue;
    }
    const std::size_t end = panel.firstRow + panel.rowCount;
    for (std::size_t rowIndex = panel.firstRow;
         rowIndex < end && rowIndex < model.rows.size();
         ++rowIndex) {
      ++count;
    }
  }
  return count;
}

void emitPanel(CreativeUiDrawList& list,
               const creative::CreativeUiPanel& panel,
               const PanelLayout& layout) {
  CreativeUiPrimitive primitive;
  primitive.kind = CreativeUiPrimitiveKind::Panel;
  primitive.tone = CreativeUiTone::SurfaceRaised;
  primitive.rect = {layout.x, layout.y, layout.width, panelHeight(panel.rowCount)};
  primitive.semanticId = makeSemanticId("panel", panelName(panel.kind));
  primitive.enabled = panel.enabled;
  list.primitives.push_back(std::move(primitive));
  ++list.rectCount;
}

void emitRowText(CreativeUiDrawList& list,
                 const creative::CreativeUiRow& row,
                 std::size_t rowIndex,
                 const PanelLayout& layout,
                 float y) {
  const bool enabled = isRowEnabled(row);
  CreativeUiPrimitive primitive;
  primitive.kind = CreativeUiPrimitiveKind::Text;
  primitive.tone = enabled ? CreativeUiTone::TextPrimary
                           : CreativeUiTone::Disabled;
  primitive.rect = {layout.x + kPanelPadding,
                    y,
                    layout.width - (2.0F * kPanelPadding),
                    kRowHeight};
  primitive.semanticId = rowSemanticId(row, rowIndex);
  primitive.text = fitTextToWidth(rowText(row), primitive.rect.width);
  primitive.enabled = enabled;

  CreativeUiHitRegion hit;
  hit.semanticId = primitive.semanticId;
  hit.rect = primitive.rect;
  hit.kind = CreativeUiHitKind::Row;
  hit.enabled = enabled;

  list.primitives.push_back(std::move(primitive));
  list.hitRegions.push_back(std::move(hit));
  ++list.textCount;
  ++list.rowCount;
  if (!enabled) {
    ++list.disabledRowCount;
  }
}

}  // namespace

CreativeUiDrawList buildProductCreativeUiDrawList(
    const ProductCreativeUiDrawListRequest& request) {
  CreativeUiDrawList list;
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  list.theme = request.theme;

  if (request.model == nullptr) {
    list.ready = false;
    list.partial = false;
    list.status = "product_creative_ui_model_missing";
    list.reasonCode = "product_creative_ui_model_missing";
    return list;
  }

  const creative::CreativeUiModel& model = *request.model;
  list.ready = true;
  list.partial = false;
  list.status = "product_creative_ui_draw_list_ready";
  list.reasonCode = "product_creative_ui_draw_list_ready";
  const std::size_t visibleRows = visibleRowCount(model);
  list.primitives.reserve(visiblePanelCount(model) + visibleRows);
  list.hitRegions.reserve(visibleRows);

  PanelLayoutCursor cursor;
  for (const creative::CreativeUiPanel& panel : model.panels) {
    if (!panel.visible) {
      continue;
    }

    const PanelLayout layout =
        panelLayoutFor(panel, request.virtualWidth, request.virtualHeight, cursor);
    emitPanel(list, panel, layout);
    float rowY = layout.y + kPanelPadding;
    const std::size_t end = panel.firstRow + panel.rowCount;
    for (std::size_t rowIndex = panel.firstRow;
         rowIndex < end && rowIndex < model.rows.size();
         ++rowIndex) {
      emitRowText(list, model.rows[rowIndex], rowIndex, layout, rowY);
      rowY += kRowHeight;
    }
  }

  list.primitiveCount = list.primitives.size();
  list.hitRegionCount = list.hitRegions.size();
  return list;
}

}  // namespace iggy3d
