#include "app/iggy3d/ProductUiDrawList.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "app/frontend/StarterScreen.hpp"

namespace iggy3d {
namespace {

constexpr float kHeaderY = 34.0F;
constexpr float kHeaderHeight = 58.0F;
constexpr float kRowX = 62.0F;
constexpr float kRowY = 150.0F;
constexpr float kRowWidth = 260.0F;
constexpr float kRowHeight = 36.0F;
constexpr float kRowStep = 52.0F;
constexpr float kContentX = 390.0F;
constexpr float kContentY = 92.0F;
constexpr float kContentWidth = 890.0F;
constexpr float kContentHeight = 556.0F;
constexpr float kFooterY = 648.0F;
constexpr float kFooterHeight = 72.0F;

enum class ProductStarterUiContext {
  Root,
  ChildPartial,
  UnsupportedScreen,
  MissingFrontend,
};

struct ProductUiToneDescriptor {
  ProductUiTone tone;
  std::string_view name;
  ProductUiColor color;
};

struct ProductUiPrimitiveKindDescriptor {
  ProductUiPrimitiveKind kind;
  std::string_view name;
};

struct ProductStarterUiBuildDescriptor {
  ProductStarterUiContext context;
  bool partial;
  std::string_view status;
  std::string_view reasonCode;
  std::string_view statusText;
};

constexpr std::array<ProductUiPrimitiveKindDescriptor, 5> kPrimitiveKindDescriptors{
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Panel, "panel"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Rect, "rect"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Text, "text"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Border, "border"},
    ProductUiPrimitiveKindDescriptor{ProductUiPrimitiveKind::Highlight, "highlight"},
};

constexpr std::array<ProductUiToneDescriptor, 9> kToneDescriptors{
    ProductUiToneDescriptor{ProductUiTone::Surface, "surface", {0.07F, 0.09F, 0.11F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::SurfaceRaised,
                            "surface_raised",
                            {0.11F, 0.14F, 0.16F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::TextPrimary,
                            "text_primary",
                            {0.90F, 0.93F, 0.84F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::TextMuted,
                            "text_muted",
                            {0.64F, 0.70F, 0.67F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Accent, "accent", {0.49F, 0.79F, 0.69F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Selected,
                            "selected",
                            {0.25F, 0.37F, 0.34F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Disabled,
                            "disabled",
                            {0.31F, 0.35F, 0.35F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Border, "border", {0.18F, 0.23F, 0.25F, 1.0F}},
    ProductUiToneDescriptor{ProductUiTone::Status, "status", {0.50F, 0.56F, 0.53F, 1.0F}},
};

constexpr std::array<ProductStarterUiBuildDescriptor, 2> kStarterUiBuildDescriptors{
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::Root,
        false,
        "product_ui_draw_list_ready",
        "product_ui_draw_list_ready",
        "starter root menu ready",
    },
    ProductStarterUiBuildDescriptor{
        ProductStarterUiContext::ChildPartial,
        true,
        "product_ui_draw_list_partial",
        "starter_child_panel_not_modeled",
        "starter child panel pending shared ui draw-list",
    },
};

constexpr std::array<ProductUiPrimitiveKind, 2> kRowBackgroundKinds{
    ProductUiPrimitiveKind::Rect,
    ProductUiPrimitiveKind::Highlight,
};

constexpr std::array<ProductUiTone, 2> kRowBackgroundTones{
    ProductUiTone::SurfaceRaised,
    ProductUiTone::Selected,
};

constexpr std::array<ProductUiTone, 2> kRowTextTones{
    ProductUiTone::Disabled,
    ProductUiTone::TextPrimary,
};

constexpr std::array<std::uint64_t, 2> kDisabledRowCountDeltas{1U, 0U};

const ProductUiPrimitiveKindDescriptor& primitiveKindDescriptor(
    ProductUiPrimitiveKind kind) {
  for (const ProductUiPrimitiveKindDescriptor& descriptor : kPrimitiveKindDescriptors) {
    // branch-gate: BG-1073
    if (descriptor.kind == kind) {
      return descriptor;
    }
  }
  return kPrimitiveKindDescriptors.front();
}

const ProductUiToneDescriptor& toneDescriptor(ProductUiTone tone) {
  for (const ProductUiToneDescriptor& descriptor : kToneDescriptors) {
    // branch-gate: BG-1073
    if (descriptor.tone == tone) {
      return descriptor;
    }
  }
  return kToneDescriptors.front();
}

const ProductStarterUiBuildDescriptor& starterUiBuildDescriptor(
    ProductStarterUiContext context) {
  for (const ProductStarterUiBuildDescriptor& descriptor : kStarterUiBuildDescriptors) {
    // branch-gate: BG-1073
    if (descriptor.context == context) {
      return descriptor;
    }
  }
  return kStarterUiBuildDescriptors.front();
}

ProductStarterUiContext starterUiContextFor(const ProductUiDrawListRequest& request) {
  // branch-gate: BG-1073
  if (request.frontend == nullptr) {
    return ProductStarterUiContext::MissingFrontend;
  }
  // branch-gate: BG-1073
  if (request.frontend->screen != FrontendScreen::Starter) {
    return ProductStarterUiContext::UnsupportedScreen;
  }
  // branch-gate: BG-1073
  if (request.frontend->childScreen != FrontendScreen::Gameplay) {
    return ProductStarterUiContext::ChildPartial;
  }
  return ProductStarterUiContext::Root;
}

std::string makeStarterSemanticId(std::string_view suffix) {
  std::string semanticId = "starter.";
  semanticId.append(suffix);
  return semanticId;
}

std::string makeStarterRowSemanticId(FrontendAction action, std::string_view suffix) {
  std::string semanticId = "starter.row.";
  semanticId.append(frontendActionName(action));
  semanticId.push_back('.');
  semanticId.append(suffix);
  return semanticId;
}

void emitRect(ProductUiDrawList& list,
              ProductUiPrimitiveKind kind,
              ProductUiTone tone,
              ProductUiRect rect,
              std::string semanticId,
              FrontendAction action = FrontendAction::None,
              bool selected = false,
              bool enabled = true) {
  ProductUiPrimitive primitive;
  primitive.kind = kind;
  primitive.tone = tone;
  primitive.rect = rect;
  primitive.semanticId = std::move(semanticId);
  primitive.action = action;
  primitive.selected = selected;
  primitive.enabled = enabled;
  list.primitives.push_back(std::move(primitive));
  ++list.rectCount;
}

void emitText(ProductUiDrawList& list,
              ProductUiTone tone,
              ProductUiRect rect,
              std::string semanticId,
              std::string_view text,
              FrontendAction action = FrontendAction::None,
              bool selected = false,
              bool enabled = true) {
  ProductUiPrimitive primitive;
  primitive.kind = ProductUiPrimitiveKind::Text;
  primitive.tone = tone;
  primitive.rect = rect;
  primitive.semanticId = std::move(semanticId);
  primitive.text = std::string(text);
  primitive.action = action;
  primitive.selected = selected;
  primitive.enabled = enabled;
  list.primitives.push_back(std::move(primitive));
  ++list.textCount;
}

void emitStarterFrame(ProductUiDrawList& list) {
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::SurfaceRaised,
           {0.0F, 0.0F, 1280.0F, kHeaderHeight + kHeaderY},
           makeStarterSemanticId("header.panel"));
  emitText(list,
           ProductUiTone::TextPrimary,
           {46.0F, kHeaderY, 180.0F, 42.0F},
           makeStarterSemanticId("header.title"),
           "IGGY3D");
  emitText(list,
           ProductUiTone::Accent,
           {330.0F, 44.0F, 320.0F, 32.0F},
           makeStarterSemanticId("header.subtitle"),
           "OPENING MENU");
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::SurfaceRaised,
           {0.0F, 92.0F, 390.0F, 556.0F},
           makeStarterSemanticId("menu.panel"));
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::Surface,
           {kContentX, kContentY, kContentWidth, kContentHeight},
           makeStarterSemanticId("content.panel"));
  emitRect(list,
           ProductUiPrimitiveKind::Panel,
           ProductUiTone::SurfaceRaised,
           {0.0F, kFooterY, 1280.0F, kFooterHeight},
           makeStarterSemanticId("footer.panel"));
}

void emitStarterRows(ProductUiDrawList& list, const StarterScreenModel& model) {
  float y = kRowY;
  for (const FrontendAction action : model.actions) {
    const bool selected = action == model.selected;
    const bool enabled = starterActionEnabled(action, model.compatibleSaveCount);
    const std::size_t selectedIndex = static_cast<std::size_t>(selected);
    const std::size_t enabledIndex = static_cast<std::size_t>(enabled);
    const ProductUiPrimitiveKind rowKind = kRowBackgroundKinds[selectedIndex];
    const ProductUiTone rowTone = kRowBackgroundTones[selectedIndex];
    const ProductUiTone textTone = kRowTextTones[enabledIndex];
    emitRect(list,
             rowKind,
             rowTone,
             {kRowX - 12.0F, y - 8.0F, kRowWidth, kRowHeight},
             makeStarterRowSemanticId(action, "background"),
             action,
             selected,
             enabled);
    emitText(list,
             textTone,
             {kRowX + 18.0F, y, kRowWidth - 30.0F, 28.0F},
             makeStarterRowSemanticId(action, "label"),
             starterActionLabel(action),
             action,
             selected,
             enabled);
    list.disabledRowCount += kDisabledRowCountDeltas[enabledIndex];
    ++list.rowCount;
    y += kRowStep;
  }
}

void emitStarterStatus(ProductUiDrawList& list,
                       const FrontendState& frontend,
                       ProductStarterUiContext context) {
  const ProductStarterUiBuildDescriptor& descriptor =
      starterUiBuildDescriptor(context);
  emitText(list,
           ProductUiTone::Status,
           {44.0F, 674.0F, 960.0F, 24.0F},
           makeStarterSemanticId("status"),
           descriptor.statusText);
  emitText(list,
           ProductUiTone::TextMuted,
           {850.0F, 230.0F, 320.0F, 26.0F},
           makeStarterSemanticId("content.child_screen"),
           frontendScreenName(frontend.childScreen));
}

ProductUiDrawList rejectedList(const ProductUiDrawListRequest& request,
                               std::string_view status,
                               std::string_view reasonCode) {
  ProductUiDrawList list;
  list.ready = false;
  list.partial = false;
  list.status = std::string(status);
  list.reasonCode = std::string(reasonCode);
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  return list;
}

}  // namespace

std::string_view productUiPrimitiveKindName(ProductUiPrimitiveKind kind) {
  return primitiveKindDescriptor(kind).name;
}

std::string_view productUiToneName(ProductUiTone tone) {
  return toneDescriptor(tone).name;
}

ProductUiColor productUiToneColor(ProductUiTone tone) {
  return toneDescriptor(tone).color;
}

ProductUiDrawList buildProductStarterUiDrawList(
    const ProductUiDrawListRequest& request) {
  const ProductStarterUiContext context = starterUiContextFor(request);
  // branch-gate: BG-1073
  if (context == ProductStarterUiContext::MissingFrontend) {
    return rejectedList(request,
                        "product_ui_draw_list_not_ready",
                        "product_ui_draw_list_missing_frontend");
  }
  // branch-gate: BG-1073
  if (context == ProductStarterUiContext::UnsupportedScreen) {
    return rejectedList(request,
                        "product_ui_draw_list_unsupported_screen",
                        "product_ui_draw_list_requires_starter");
  }

  const FrontendState& frontend = *request.frontend;
  const StarterScreenModel model =
      buildStarterScreenModel(request.compatibleSaveCount, frontend.selectedAction);
  const ProductStarterUiBuildDescriptor& descriptor =
      starterUiBuildDescriptor(context);
  ProductUiDrawList list;
  list.ready = true;
  list.partial = descriptor.partial;
  list.status = std::string(descriptor.status);
  list.reasonCode = std::string(descriptor.reasonCode);
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  list.selectedAction = std::string(frontendActionName(model.selected));

  emitStarterFrame(list);
  emitStarterRows(list, model);
  emitStarterStatus(list, frontend, context);
  list.primitiveCount = list.primitives.size();
  return list;
}

}  // namespace iggy3d
