#include "EditorCatalogLayout.hpp"

#include <algorithm>

namespace iggy3d_creative_app {

[[nodiscard]] CatalogLayout catalogLayout(std::uint32_t drawableWidth,
                                          std::uint32_t drawableHeight) {
  CatalogLayout layout;
  const std::int32_t width = static_cast<std::int32_t>(drawableWidth);
  const std::int32_t height = static_cast<std::int32_t>(drawableHeight);
  layout.panelWidth = static_cast<std::uint32_t>(
      std::max(1, std::min(760, width - 16)));
  layout.panelHeight = static_cast<std::uint32_t>(
      std::max(120, std::min(560, height - 96)));
  layout.panelX = std::max(8, (width - static_cast<std::int32_t>(
                                         layout.panelWidth)) /
                                  2);
  layout.panelY = std::max(8, (height - static_cast<std::int32_t>(
                                          layout.panelHeight) -
                              48) /
                                  2);
  layout.searchY = layout.panelY + 48;
  layout.rowsY = layout.searchY + 48;
  layout.footerY = layout.panelY +
                   static_cast<std::int32_t>(layout.panelHeight) - 38;
  const std::uint32_t tabAreaWidth =
      layout.panelWidth > 16U ? layout.panelWidth - 16U : layout.panelWidth;
  layout.tabWidth = std::min(92U, std::max(1U, tabAreaWidth / 2U));
  const std::int32_t tabsWidth =
      static_cast<std::int32_t>(layout.tabWidth * 2U);
  layout.tabsX =
      layout.panelX +
      std::max(0, static_cast<std::int32_t>(layout.panelWidth) - tabsWidth - 8);
  layout.tabsY = layout.panelY + 9;
  const std::int32_t rowsHeight = std::max(0, layout.footerY - layout.rowsY);
  layout.visibleRows = static_cast<std::size_t>(
      std::max(1, rowsHeight / static_cast<std::int32_t>(layout.rowHeight)));
  layout.contentX = layout.panelX + 16;
  layout.contentWidth =
      layout.panelWidth > 32U ? layout.panelWidth - 32U : 1U;
  layout.showDetails = layout.panelWidth >= 600U;
  layout.listWidth = layout.showDetails
                         ? std::max(280U, layout.contentWidth * 58U / 100U)
                         : layout.contentWidth;
  const std::uint32_t detailGap = layout.showDetails ? 16U : 0U;
  layout.detailX = layout.contentX +
                   static_cast<std::int32_t>(layout.listWidth + detailGap);
  layout.detailWidth =
      layout.showDetails && layout.contentWidth > layout.listWidth + detailGap
          ? layout.contentWidth - layout.listWidth - detailGap
          : 0U;
  layout.equipX = layout.panelX + static_cast<std::int32_t>(layout.panelWidth) -
                  16 - static_cast<std::int32_t>(layout.equipWidth);
  layout.equipY = layout.footerY + 5;
  layout.assignWheelX =
      layout.equipX - 8 - static_cast<std::int32_t>(layout.assignWheelWidth);
  return layout;
}

[[nodiscard]] CatalogRect previousShapeButton(
    const CatalogLayout& layout) noexcept {
  return {layout.detailX + 12, layout.rowsY + 32, 30U, 28U};
}

[[nodiscard]] CatalogRect nextShapeButton(
    const CatalogLayout& layout) noexcept {
  return {layout.detailX + static_cast<std::int32_t>(layout.detailWidth) - 42,
          layout.rowsY + 32, 30U, 28U};
}

[[nodiscard]] CatalogRect equipButton(const CatalogLayout& layout) noexcept {
  return {layout.equipX, layout.equipY, layout.equipWidth,
          layout.equipHeight};
}

[[nodiscard]] CatalogRect assignWheelButton(
    const CatalogLayout& layout) noexcept {
  return {layout.assignWheelX, layout.equipY, layout.assignWheelWidth,
          layout.equipHeight};
}

}  // namespace iggy3d_creative_app
