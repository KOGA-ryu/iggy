#include "app/iggy3d/ui/Widget.hpp"

#include <iostream>
#include <string_view>

#include "app/iggy3d/menu/DrawList.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

bool textWidgetEmitsOneTextPrimitiveAndNoHitRegionWithoutAction() {
  iggy3d::WidgetOutput out;
  iggy3d::emit(iggy3d::UiText{{10.0F, 20.0F, 100.0F, 24.0F},
                              iggy3d::ProductUiTone::TextMuted,
                              "widget.text.label",
                              "HELLO"},
               out);
  bool ok = true;
  ok &= expect(out.primitives.size() == 1U, "one primitive emitted");
  ok &= expect(out.hitRegions.empty(), "no hit region without an action");
  const iggy3d::ProductUiPrimitive& primitive = out.primitives.front();
  ok &= expect(primitive.kind == iggy3d::ProductUiPrimitiveKind::Text, "text kind");
  ok &= expect(primitive.tone == iggy3d::ProductUiTone::TextMuted, "tone copied");
  ok &= expect(primitive.rect.x == 10.0F && primitive.rect.y == 20.0F,
               "rect copied");
  ok &= expect(primitive.semanticId == "widget.text.label", "semantic id copied");
  ok &= expect(primitive.text == "HELLO", "text copied");
  ok &= expect(primitive.enabled, "enabled defaults true");
  return ok;
}

bool interactiveTextEmitsHitRegionFromTheSameRect() {
  iggy3d::WidgetOutput out;
  iggy3d::emit(iggy3d::UiText{{452.0F, 508.0F, 260.0F, 26.0F},
                              iggy3d::ProductUiTone::Accent,
                              "widget.text.confirm",
                              "CONFIRM DELETE",
                              iggy3d::FrontendAction::Delete},
               out);
  bool ok = true;
  ok &= expect(out.primitives.size() == 1U, "one primitive emitted");
  ok &= expect(out.hitRegions.size() == 1U, "one hit region for the action");
  const iggy3d::ProductUiPrimitive& primitive = out.primitives.front();
  const iggy3d::UiHitRegion& hit = out.hitRegions.front();
  // The hit region is derived from the SAME rect that drew the primitive, so
  // drawing and hit-testing cannot drift the way the hand-written duplicates do.
  ok &= expect(hit.semanticId == "widget.text.confirm", "hit id matches");
  ok &= expect(hit.rect.x == primitive.rect.x && hit.rect.y == primitive.rect.y &&
                   hit.rect.width == primitive.rect.width &&
                   hit.rect.height == primitive.rect.height,
               "hit rect equals draw rect");
  ok &= expect(hit.kind == iggy3d::UiHitKind::Button, "hit kind is button");
  ok &= expect(hit.action == iggy3d::FrontendAction::Delete, "hit action copied");
  ok &= expect(hit.enabled, "hit enabled copied");
  return ok;
}

bool panelWidgetAndAppendPreserveKindCountBookkeeping() {
  iggy3d::WidgetOutput out;
  iggy3d::emit(iggy3d::UiPanel{{0.0F, 0.0F, 1280.0F, 92.0F},
                               iggy3d::ProductUiPrimitiveKind::Panel,
                               iggy3d::ProductUiTone::SurfaceRaised,
                               "widget.panel.header"},
               out);
  iggy3d::emit(iggy3d::UiText{{4.0F, 4.0F, 80.0F, 20.0F},
                              iggy3d::ProductUiTone::TextPrimary,
                              "widget.panel.header.title",
                              "IGGY3D"},
               out);
  iggy3d::ProductUiDrawList list;
  iggy3d::appendWidgetOutput(list, out);
  bool ok = true;
  ok &= expect(list.primitives.size() == 2U, "two primitives appended in order");
  ok &= expect(list.rectCount == 1U, "panel counted as a rect");
  ok &= expect(list.textCount == 1U, "text counted as text");
  ok &= expect(list.primitives.front().kind == iggy3d::ProductUiPrimitiveKind::Panel,
               "panel kind preserved and ordered first");
  return ok;
}

bool appendWidgetOutputThreadsHitRegionsOntoTheList() {
  iggy3d::WidgetOutput out;
  iggy3d::emit(iggy3d::UiText{.rect = {0.0F, 0.0F, 100.0F, 24.0F},
                              .tone = iggy3d::ProductUiTone::TextMuted,
                              .semanticId = "widget.plain.label",
                              .text = "PLAIN"},
               out);
  iggy3d::emit(iggy3d::UiText{.rect = {0.0F, 30.0F, 100.0F, 24.0F},
                              .tone = iggy3d::ProductUiTone::Accent,
                              .semanticId = "widget.action.button",
                              .text = "GO",
                              .action = iggy3d::FrontendAction::Back},
               out);
  iggy3d::ProductUiDrawList list;
  iggy3d::appendWidgetOutput(list, out);
  bool ok = true;
  ok &= expect(list.primitives.size() == 2U, "both primitives appended");
  ok &= expect(list.hitRegions.size() == 1U,
               "only the interactive widget adds a hit region");
  ok &= expect(list.hitRegionCount == 1U, "hit region count tracks the lane");
  ok &= expect(!list.hitRegions.empty() &&
                   list.hitRegions.front().semanticId == "widget.action.button",
               "hit region carries the interactive widget id");
  ok &= expect(!list.hitRegions.empty() &&
                   list.hitRegions.front().action == iggy3d::FrontendAction::Back,
               "hit region carries the action");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= textWidgetEmitsOneTextPrimitiveAndNoHitRegionWithoutAction();
  ok &= interactiveTextEmitsHitRegionFromTheSameRect();
  ok &= panelWidgetAndAppendPreserveKindCountBookkeeping();
  ok &= appendWidgetOutputThreadsHitRegionsOntoTheList();
  if (!ok) {
    return 1;
  }
  std::cout << "product_ui_widget_tests=pass\n";
  return 0;
}
