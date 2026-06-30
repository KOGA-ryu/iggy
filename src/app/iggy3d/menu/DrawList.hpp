#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult;
struct WorldSetupDraft;

enum class ProductUiPrimitiveKind : std::uint8_t {
  Panel,
  Rect,
  Text,
  Border,
  Highlight,
};

enum class ProductUiTone : std::uint8_t {
  Surface,
  SurfaceRaised,
  TextPrimary,
  TextMuted,
  Accent,
  Selected,
  Disabled,
  Border,
  Status,
};

struct ProductUiRect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

struct ProductUiColor {
  float r = 0.0F;
  float g = 0.0F;
  float b = 0.0F;
  float a = 1.0F;
};

struct ProductUiPrimitive {
  ProductUiPrimitiveKind kind = ProductUiPrimitiveKind::Rect;
  ProductUiTone tone = ProductUiTone::Surface;
  ProductUiRect rect;
  std::string semanticId;
  std::string text;
  FrontendAction action = FrontendAction::None;
  bool selected = false;
  bool enabled = true;
};

struct ProductUiDrawList {
  bool ready = false;
  bool partial = false;
  std::string status = "product_ui_draw_list_not_ready";
  std::string reasonCode = "product_ui_draw_list_not_ready";
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  std::vector<ProductUiPrimitive> primitives;
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t disabledRowCount = 0;
  std::string selectedAction = "none";
};

struct ProductUiDrawListRequest {
  const FrontendState* frontend = nullptr;
  std::uint64_t compatibleSaveCount = 0;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  const WorldSetupDraft* worldSetupDraft = nullptr;
  bool dungeonDraftEditMode = false;
  bool dungeonDraftModified = false;
  std::uint64_t dungeonDraftCursorRow = 0;
  std::uint64_t dungeonDraftCursorColumn = 0;
  std::string dungeonDraftSelectedGlyph = ".";
  std::string dungeonDraftLastGlyph = "none";
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  const ProductSaveBridgeResult* saves = nullptr;
};

std::string_view productUiPrimitiveKindName(ProductUiPrimitiveKind kind);
std::string_view productUiToneName(ProductUiTone tone);
ProductUiColor productUiToneColor(ProductUiTone tone);

ProductUiDrawList buildProductStarterUiDrawList(
    const ProductUiDrawListRequest& request);

}  // namespace iggy3d
