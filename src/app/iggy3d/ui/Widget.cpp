#include "app/iggy3d/ui/Widget.hpp"

#include <utility>

namespace iggy3d {
namespace {

UiHitRegion makeButtonHitRegion(const std::string& semanticId,
                                ProductUiRect rect,
                                FrontendAction action,
                                bool enabled) {
  UiHitRegion region;
  region.semanticId = semanticId;
  region.rect = rect;
  region.kind = UiHitKind::Button;
  region.action = action;
  region.enabled = enabled;
  return region;
}

}  // namespace

void emit(const UiText& widget, WidgetOutput& out) {
  ProductUiPrimitive primitive;
  primitive.kind = ProductUiPrimitiveKind::Text;
  primitive.tone = widget.tone;
  primitive.rect = widget.rect;
  primitive.semanticId = widget.semanticId;
  primitive.text = widget.text;
  primitive.action = widget.action;
  primitive.selected = widget.selected;
  primitive.enabled = widget.enabled;
  out.primitives.push_back(std::move(primitive));
  // branch-gate: BG-1073
  if (widget.action != FrontendAction::None) {
    out.hitRegions.push_back(makeButtonHitRegion(
        widget.semanticId, widget.rect, widget.action, widget.enabled));
  }
}

void emit(const UiPanel& widget, WidgetOutput& out) {
  ProductUiPrimitive primitive;
  primitive.kind = widget.kind;
  primitive.tone = widget.tone;
  primitive.rect = widget.rect;
  primitive.semanticId = widget.semanticId;
  primitive.action = widget.action;
  primitive.selected = widget.selected;
  primitive.enabled = widget.enabled;
  out.primitives.push_back(std::move(primitive));
  // branch-gate: BG-1073
  if (widget.action != FrontendAction::None) {
    out.hitRegions.push_back(makeButtonHitRegion(
        widget.semanticId, widget.rect, widget.action, widget.enabled));
  }
}

void appendWidgetOutput(ProductUiDrawList& list, const WidgetOutput& out) {
  for (const ProductUiPrimitive& primitive : out.primitives) {
    list.primitives.push_back(primitive);
    // branch-gate: BG-1073
    if (primitive.kind == ProductUiPrimitiveKind::Text) {
      ++list.textCount;
    } else {
      ++list.rectCount;
    }
  }
  for (const UiHitRegion& region : out.hitRegions) {
    list.hitRegions.push_back(region);
    ++list.hitRegionCount;
  }
}

}  // namespace iggy3d
