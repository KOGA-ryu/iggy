#include "app/iggy3d/menu/CreativeUiDrawList.hpp"

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
constexpr float kPanelWidth = 300.0F;
constexpr float kPanelPadding = 10.0F;
constexpr float kRowHeight = 28.0F;
constexpr float kPanelGap = 8.0F;

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
    case creative::CreativeUiPanelKind::Status:
      return "status";
    case creative::CreativeUiPanelKind::Selection:
      return "selection";
    case creative::CreativeUiPanelKind::Inspection:
      return "inspection";
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
    case creative::Tool::Inspect:
      return "Inspect";
    case creative::Tool::Measure:
      return "Measure";
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

[[nodiscard]] std::string rowText(const creative::CreativeUiRow& row) {
  std::string text(row.label.empty() ? row.id : row.label);
  switch (row.kind) {
    case creative::CreativeUiRowKind::ActiveTool:
      text = "Tool: ";
      text.append(toolName(row.tool));
      break;
    case creative::CreativeUiRowKind::CreateRoom:
      break;
    case creative::CreativeUiRowKind::StatusSummary:
      text = "Status: ";
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
    case creative::CreativeUiRowKind::InspectedTarget:
      text = row.kind == creative::CreativeUiRowKind::SelectedTarget
                 ? "Selected: target="
                 : "Inspected: target=";
      text.append(std::to_string(row.target.value));
      appendVisibilityText(text, row);
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

void emitPanel(ProductUiDrawList& list,
               const creative::CreativeUiPanel& panel,
               float y) {
  ProductUiPrimitive primitive;
  primitive.kind = ProductUiPrimitiveKind::Panel;
  primitive.tone = ProductUiTone::SurfaceRaised;
  primitive.rect = {kOverlayX,
                    y,
                    kPanelWidth,
                    (2.0F * kPanelPadding) +
                        static_cast<float>(panel.rowCount) * kRowHeight};
  primitive.semanticId = makeSemanticId("panel", panelName(panel.kind));
  primitive.enabled = panel.enabled;
  list.primitives.push_back(std::move(primitive));
  ++list.rectCount;
}

void emitRowText(ProductUiDrawList& list,
                 const creative::CreativeUiRow& row,
                 std::size_t rowIndex,
                 float y) {
  const bool enabled = isRowEnabled(row);
  ProductUiPrimitive primitive;
  primitive.kind = ProductUiPrimitiveKind::Text;
  primitive.tone = enabled ? ProductUiTone::TextPrimary
                           : ProductUiTone::Disabled;
  primitive.rect = {kOverlayX + kPanelPadding,
                    y,
                    kPanelWidth - (2.0F * kPanelPadding),
                    kRowHeight};
  primitive.semanticId = rowSemanticId(row, rowIndex);
  primitive.text = rowText(row);
  primitive.enabled = enabled;

  UiHitRegion hit;
  hit.semanticId = primitive.semanticId;
  hit.rect = primitive.rect;
  hit.kind = UiHitKind::Row;
  hit.action = FrontendAction::None;
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

ProductUiDrawList buildProductCreativeUiDrawList(
    const ProductCreativeUiDrawListRequest& request) {
  ProductUiDrawList list;
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

  float y = kOverlayY;
  for (const creative::CreativeUiPanel& panel : model.panels) {
    if (!panel.visible) {
      continue;
    }

    emitPanel(list, panel, y);
    float rowY = y + kPanelPadding;
    const std::size_t end = panel.firstRow + panel.rowCount;
    for (std::size_t rowIndex = panel.firstRow;
         rowIndex < end && rowIndex < model.rows.size();
         ++rowIndex) {
      emitRowText(list, model.rows[rowIndex], rowIndex, rowY);
      rowY += kRowHeight;
    }

    y += (2.0F * kPanelPadding) +
         static_cast<float>(panel.rowCount) * kRowHeight + kPanelGap;
  }

  list.primitiveCount = list.primitives.size();
  list.hitRegionCount = list.hitRegions.size();
  return list;
}

}  // namespace iggy3d
