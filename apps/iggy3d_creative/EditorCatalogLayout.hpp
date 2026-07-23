#pragma once

#include <cstddef>
#include <cstdint>

namespace iggy3d_creative_app {

struct CatalogLayout {
  std::int32_t panelX = 0;
  std::int32_t panelY = 0;
  std::uint32_t panelWidth = 0;
  std::uint32_t panelHeight = 0;
  std::int32_t searchY = 0;
  std::int32_t rowsY = 0;
  std::int32_t footerY = 0;
  std::int32_t tabsX = 0;
  std::int32_t tabsY = 0;
  std::uint32_t tabWidth = 92;
  std::uint32_t tabHeight = 28;
  std::uint32_t rowHeight = 30;
  std::size_t visibleRows = 1;
  std::int32_t contentX = 0;
  std::uint32_t contentWidth = 0;
  std::uint32_t listWidth = 0;
  std::int32_t detailX = 0;
  std::uint32_t detailWidth = 0;
  bool showDetails = false;
  std::int32_t equipX = 0;
  std::int32_t equipY = 0;
  std::uint32_t equipWidth = 88;
  std::uint32_t equipHeight = 28;
  std::int32_t assignWheelX = 0;
  std::uint32_t assignWheelWidth = 136;
  std::int32_t replaceX = 0;
  std::uint32_t replaceWidth = 176;
};

struct CatalogRect {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

[[nodiscard]] CatalogLayout catalogLayout(
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight);
[[nodiscard]] CatalogRect previousShapeButton(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect nextShapeButton(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect equipButton(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect assignWheelButton(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect replaceSelectionButton(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect catalogAssetThumbnailRect(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect previousAssetMaterialVariantButton(
    const CatalogLayout& layout) noexcept;
[[nodiscard]] CatalogRect nextAssetMaterialVariantButton(
    const CatalogLayout& layout) noexcept;

}  // namespace iggy3d_creative_app
