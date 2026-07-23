#include "EditorCatalogInternal.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>

#include "EditorCatalogLayout.hpp"
#include "EditorState.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] std::string fitAssetText(std::string_view text,
                                       std::uint32_t widthPixels) {
  const std::size_t maxCharacters =
      std::max<std::size_t>(4U, widthPixels / 8U);
  if (text.size() <= maxCharacters) {
    return std::string(text);
  }
  std::string output(text.substr(0, maxCharacters - 3U));
  output.append("...");
  return output;
}

void appendAssetText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                     std::string_view text,
                     std::int32_t x,
                     std::int32_t y,
                     std::uint32_t drawableWidth,
                     std::uint32_t drawableHeight,
                     float r,
                     float g,
                     float b) {
  iggy3d::DebugHudLayoutResult layout = iggy3d::layoutDebugHudTextAt(
      text, x, y, drawableWidth, drawableHeight);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = r;
    quad.g = g;
    quad.b = b;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

[[nodiscard]] bool sameThumbnailPixel(
    iggy3d::StaticMeshThumbnailPixel lhs,
    iggy3d::StaticMeshThumbnailPixel rhs) noexcept {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

void appendCatalogAssetThumbnail(
    const cr::CreativeCatalogEntry& entry,
    const CatalogLayout& layout,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  const CatalogRect rect = catalogAssetThumbnailRect(layout);
  uiRects.push_back(
      {rect.x, rect.y, rect.width, rect.height, 0.035F, 0.045F, 0.052F, 1.0F});
  const iggy3d::StaticMeshAssetThumbnail& thumbnail = entry.assetThumbnail;
  if (thumbnail.valid) {
    for (std::size_t y = 0U; y < iggy3d::kStaticMeshThumbnailExtent; ++y) {
      std::size_t x = 0U;
      while (x < iggy3d::kStaticMeshThumbnailExtent) {
        const iggy3d::StaticMeshThumbnailPixel pixel =
            thumbnail.pixels[y * iggy3d::kStaticMeshThumbnailExtent + x];
        if (pixel.a == 0U) {
          ++x;
          continue;
        }
        std::size_t end = x + 1U;
        while (end < iggy3d::kStaticMeshThumbnailExtent &&
               sameThumbnailPixel(
                   pixel,
                   thumbnail.pixels[y * iggy3d::kStaticMeshThumbnailExtent +
                                    end])) {
          ++end;
        }
        const std::int32_t x0 =
            rect.x + static_cast<std::int32_t>(
                         x * rect.width / iggy3d::kStaticMeshThumbnailExtent);
        const std::int32_t x1 =
            rect.x + static_cast<std::int32_t>(
                         end * rect.width / iggy3d::kStaticMeshThumbnailExtent);
        const std::int32_t y0 =
            rect.y + static_cast<std::int32_t>(
                         y * rect.height / iggy3d::kStaticMeshThumbnailExtent);
        const std::int32_t y1 =
            rect.y + static_cast<std::int32_t>(
                         (y + 1U) * rect.height /
                         iggy3d::kStaticMeshThumbnailExtent);
        uiRects.push_back(
            {x0, y0, static_cast<std::uint32_t>(std::max(1, x1 - x0)),
             static_cast<std::uint32_t>(std::max(1, y1 - y0)),
             static_cast<float>(pixel.r) / 255.0F,
             static_cast<float>(pixel.g) / 255.0F,
             static_cast<float>(pixel.b) / 255.0F, 1.0F});
        x = end;
      }
    }
  } else {
    appendAssetText(glyphs, entry.authoredComposite ? "EDITABLE COMPOSITE"
                                                    : "NO THUMBNAIL",
                    rect.x + 10,
                    rect.y + static_cast<std::int32_t>(rect.height / 2U) - 4,
                    drawableWidth, drawableHeight, 0.78F, 0.42F, 0.34F);
  }
  constexpr float kBorderR = 0.30F;
  constexpr float kBorderG = 0.38F;
  constexpr float kBorderB = 0.42F;
  uiRects.push_back({rect.x, rect.y, rect.width, 1U, kBorderR, kBorderG,
                     kBorderB, 1.0F});
  uiRects.push_back({rect.x, rect.y + static_cast<std::int32_t>(rect.height) - 1,
                     rect.width, 1U, kBorderR, kBorderG, kBorderB, 1.0F});
  uiRects.push_back({rect.x, rect.y, 1U, rect.height, kBorderR, kBorderG,
                     kBorderB, 1.0F});
  uiRects.push_back({rect.x + static_cast<std::int32_t>(rect.width) - 1, rect.y,
                     1U, rect.height, kBorderR, kBorderG, kBorderB, 1.0F});
}

void appendCompactDetailRow(
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::string_view label,
    std::string_view value,
    const CatalogLayout& layout,
    std::int32_t y,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) {
  appendAssetText(glyphs, label, layout.detailX + 4, y, drawableWidth,
                  drawableHeight, 0.52F, 0.62F, 0.68F);
  const std::uint32_t valueWidth =
      layout.detailWidth > 82U ? layout.detailWidth - 82U : 1U;
  appendAssetText(glyphs, fitAssetText(value, valueWidth), layout.detailX + 78,
                  y, drawableWidth, drawableHeight, 0.88F, 0.92F, 0.94F);
}

[[nodiscard]] std::string catalogSocketSummary(
    const cr::CreativeCatalogEntry& entry) {
  if (entry.assetAttachmentSockets.empty()) {
    return "NONE";
  }
  std::string output = std::to_string(entry.assetAttachmentSockets.size());
  output.append(" | ");
  const std::size_t shown =
      std::min<std::size_t>(2U, entry.assetAttachmentSockets.size());
  for (std::size_t index = 0U; index < shown; ++index) {
    if (index != 0U) {
      output.append(", ");
    }
    const iggy3d::StaticMeshAttachmentSocket& socket =
        entry.assetAttachmentSockets[index];
    output.append(socket.role ==
                          iggy3d::StaticMeshAttachmentSocketRole::Receiver
                      ? "R:"
                      : "P:");
    output.append(socket.name);
  }
  if (entry.assetAttachmentSockets.size() > shown) {
    output.append(" +");
    output.append(std::to_string(entry.assetAttachmentSockets.size() - shown));
  }
  return output;
}

}  // namespace

void appendCatalogAssetDetails(
    const CreativeEditorState& editor,
    const cr::CreativeCatalogEntry& entry,
    const CatalogLayout& layout,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  appendCatalogAssetThumbnail(entry, layout, drawableWidth, drawableHeight,
                              uiRects, glyphs);
  const CatalogRect previous = previousAssetMaterialVariantButton(layout);
  const CatalogRect next = nextAssetMaterialVariantButton(layout);
  uiRects.push_back({previous.x, previous.y, previous.width, previous.height,
                     0.12F, 0.14F, 0.16F, 1.0F});
  uiRects.push_back({next.x, next.y, next.width, next.height, 0.12F, 0.14F,
                     0.16F, 1.0F});
  appendAssetText(glyphs, "<", previous.x + 10, previous.y + 6,
                  drawableWidth, drawableHeight, 0.90F, 0.93F, 0.95F);
  appendAssetText(glyphs, ">", next.x + 10, next.y + 6, drawableWidth,
                  drawableHeight, 0.90F, 0.93F, 0.95F);
  const std::uint32_t variantWidth =
      layout.detailWidth > 76U ? layout.detailWidth - 76U : 1U;
  appendAssetText(
      glyphs,
      fitAssetText(
          creativeEditorCatalogAssetMaterialVariantLabel(editor.catalog, entry),
          variantWidth),
      previous.x + 36, previous.y + 6, drawableWidth, drawableHeight, 0.88F,
      0.92F, 0.94F);

  const cr::CreativeCatalogAssetInspection inspection =
      cr::inspectCreativeCatalogAsset(entry);
  std::int32_t detailY = previous.y + 34;
  appendCompactDetailRow(glyphs, "ID",
                         cr::creativeHotbarAssetId(entry.hotbarEntry), layout,
                         detailY, drawableWidth, drawableHeight);
  detailY += 20;
  appendCompactDetailRow(
      glyphs, "CATEGORY",
      entry.assetAuthoringMetadata.categoryId.empty()
          ? std::string_view{"UNCATEGORIZED"}
          : std::string_view{entry.assetAuthoringMetadata.categoryId},
      layout, detailY, drawableWidth, drawableHeight);
  detailY += 20;
  char dimensions[96];
  std::snprintf(dimensions, sizeof(dimensions), "%.2f x %.2f x %.2f m",
                inspection.bounds.size.x, inspection.bounds.size.y,
                inspection.bounds.size.z);
  appendCompactDetailRow(glyphs, "SIZE", dimensions, layout, detailY,
                         drawableWidth, drawableHeight);
  detailY += 20;
  char pivot[96];
  std::snprintf(pivot, sizeof(pivot), "%+.2f %+.2f %+.2f",
                inspection.pivotFromCenter.x, inspection.pivotFromCenter.y,
                inspection.pivotFromCenter.z);
  appendCompactDetailRow(glyphs, "PIVOT", pivot, layout, detailY,
                         drawableWidth, drawableHeight);
  detailY += 20;
  std::string collision(iggy3d::toString(inspection.collisionMode));
  collision.append(" | ");
  collision.append(std::to_string(
      inspection.collisionMode == iggy3d::StaticMeshCollisionMode::Bounds
          ? 1U
          : inspection.collisionPartCount));
  collision.append(" BOX");
  if (inspection.walkable) {
    collision.append(" | WALKABLE");
  }
  appendCompactDetailRow(glyphs, "COLLISION", collision, layout, detailY,
                         drawableWidth, drawableHeight);
  detailY += 20;
  appendCompactDetailRow(glyphs, "SOCKETS", catalogSocketSummary(entry),
                         layout, detailY, drawableWidth, drawableHeight);
  detailY += 20;
  char materials[96];
  std::snprintf(materials, sizeof(materials), "%zu MATERIALS | %zu VARIANTS",
                inspection.materialCount, inspection.materialVariantCount);
  appendCompactDetailRow(glyphs, "SURFACE", materials, layout, detailY,
                         drawableWidth, drawableHeight);
  detailY += 20;
  char version[32];
  std::snprintf(version, sizeof(version), "%016llX",
                static_cast<unsigned long long>(inspection.contentHash));
  appendCompactDetailRow(glyphs, "VERSION", version, layout, detailY,
                         drawableWidth, drawableHeight);
}

}  // namespace iggy3d_creative_app
