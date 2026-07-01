#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/frontend/FrontendState.hpp"  // FrontendAction
#include "app/iggy3d/menu/DrawList.hpp"     // ProductUiPrimitive/Rect/Tone/DrawList

namespace iggy3d {

// L1 widget layer — the reusable "bricks". A widget is a pure value: it carries
// its props and, via emit(), appends draw primitives (the renderer lane) and hit
// regions (the input lane) to a WidgetOutput. No retained state, no per-frame
// objects — the same (state -> output) determinism as the rest of the engine, so
// every widget is receipt-testable. See docs/ui/ui_architecture.md.
//
// v1 note: widgets take EXPLICIT rects (absolute positioning). The layout engine
// (Stack/Grid auto-positioning) is a later, deliberately re-baselined step; this
// layer is emission-preserving so a screen can be rebuilt with a byte-identical
// draw-list receipt.

// UiHitKind / UiHitRegion are defined in menu/DrawList.hpp (L0) so the draw list
// can carry a durable hit-region lane alongside its primitives. A widget emits a
// region from the SAME rect that produced its draw primitive, so drawing and
// hit-testing can never drift apart.

// What every widget emits: visuals for the renderer + hit regions for input.
struct WidgetOutput {
  std::vector<ProductUiPrimitive> primitives;
  std::vector<UiHitRegion> hitRegions;
};

// L1: Text — a run of glyphs. Non-interactive unless given an action, in which
// case it also emits a Button hit region (text-as-button, the current idiom).
struct UiText {
  ProductUiRect rect;
  ProductUiTone tone = ProductUiTone::TextPrimary;
  std::string semanticId;
  std::string text;
  FrontendAction action = FrontendAction::None;
  bool selected = false;
  bool enabled = true;
};

// L1: Panel — a filled surface (the Panel/Rect/Border/Highlight kinds share it).
struct UiPanel {
  ProductUiRect rect;
  ProductUiPrimitiveKind kind = ProductUiPrimitiveKind::Panel;
  ProductUiTone tone = ProductUiTone::Surface;
  std::string semanticId;
  FrontendAction action = FrontendAction::None;
  bool selected = false;
  bool enabled = true;
};

void emit(const UiText& widget, WidgetOutput& out);
void emit(const UiPanel& widget, WidgetOutput& out);

// Splat a widget output into the L0 draw list, preserving the kind-based count
// bookkeeping (textCount / rectCount) that receipts assert — mirrors the counting
// the hand-written emitText/emitRect helpers do. Hit regions are carried onto the
// draw list's hit-region lane (list.hitRegions) so they are captured and
// receipt-guarded, not dropped. (The input router does not consume them yet.)
void appendWidgetOutput(ProductUiDrawList& list, const WidgetOutput& out);

}  // namespace iggy3d
