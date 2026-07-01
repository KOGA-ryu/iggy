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

// The input lane's view of an interactive widget. It is computed from the SAME
// rect that produced the draw primitive, so drawing and hit-testing can never
// drift apart (the duplicated hand-math they drift from lives in OpeningMenuView).
enum class UiHitKind : std::uint8_t {
  None,
  Button,
  Row,
  Slider,
  Toggle,
  Viewport,
};

struct UiHitRegion {
  std::string semanticId;
  ProductUiRect rect;
  UiHitKind kind = UiHitKind::None;
  FrontendAction action = FrontendAction::None;
  bool enabled = true;
};

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
// the hand-written emitText/emitRect helpers do. Hit regions ride on the
// WidgetOutput for the input lane (not yet consumed by the draw list).
void appendWidgetOutput(ProductUiDrawList& list, const WidgetOutput& out);

}  // namespace iggy3d
