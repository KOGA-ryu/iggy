#include "EditorActionHints.hpp"

#include <algorithm>
#include <array>
#include <span>

#include "EditorState.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"

namespace iggy3d_creative_app {
namespace {
namespace cr = iggy3d::creative;

struct HintSpecBuffer {
  std::array<cr::CreativeActionHintSpec, cr::kCreativeActionHintCapacity>
      specs{};
  std::size_t count = 0U;
  bool capacityExceeded = false;

  [[nodiscard]] std::span<const cr::CreativeActionHintSpec> items()
      const noexcept {
    return {specs.data(), count};
  }
};

void appendHint(HintSpecBuffer& buffer,
                cr::CreativeInputActionId action,
                std::string_view label) noexcept {
  if (buffer.count >= buffer.specs.size()) {
    buffer.capacityExceeded = true;
    return;
  }
  buffer.specs[buffer.count++] = {{{action, cr::CreativeInputActionId::Count}},
                                  1U, label};
}

void appendHintPair(HintSpecBuffer& buffer,
                    cr::CreativeInputActionId first,
                    cr::CreativeInputActionId second,
                    std::string_view label) noexcept {
  if (buffer.count >= buffer.specs.size()) {
    buffer.capacityExceeded = true;
    return;
  }
  buffer.specs[buffer.count++] = {{{first, second}}, 2U, label};
}

void appendToolAccess(HintSpecBuffer& buffer) noexcept {
  if (buffer.count < buffer.specs.size()) {
    appendHint(buffer, cr::CreativeInputActionId::ToggleToolWheel, "Tools");
  }
  if (buffer.count < buffer.specs.size()) {
    appendHint(buffer, cr::CreativeInputActionId::ToggleCatalog, "Catalog");
  }
}

void appendGamepadHotbar(HintSpecBuffer& buffer) noexcept {
  appendHintPair(buffer, cr::CreativeInputActionId::HotbarPrevious,
                 cr::CreativeInputActionId::HotbarNext, "Hotbar");
}

void appendQuickEdit(HintSpecBuffer& buffer,
                     const CreativeEditorState& editor) noexcept {
  const cr::CreativeHeldItemKind held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind;
  if (held == cr::CreativeHeldItemKind::TerrainControl) {
    appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                   cr::CreativeInputActionId::QuickEditNext, "Height");
    appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                   cr::CreativeInputActionId::QuickEditIncrease, "Radius");
    return;
  }
  if (held == cr::CreativeHeldItemKind::TerrainGrade) {
    appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                   cr::CreativeInputActionId::QuickEditNext, "End height");
    appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                   cr::CreativeInputActionId::QuickEditIncrease, "Width");
    return;
  }
  if (held == cr::CreativeHeldItemKind::TerrainSculpt) {
    appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                   cr::CreativeInputActionId::QuickEditNext, "Strength");
    appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                   cr::CreativeInputActionId::QuickEditIncrease, "Radius");
    return;
  }
  if (editor.quickEdit.options.count == 0U) {
    return;
  }
  appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                 cr::CreativeInputActionId::QuickEditNext, "Setting");
  appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                 cr::CreativeInputActionId::QuickEditIncrease, "Adjust");
}

void appendViewportKeyboardHints(HintSpecBuffer& buffer,
                                 const CreativeEditorState& editor,
                                 cr::CreativeHeldItemKind held) noexcept {
  switch (held) {
    case cr::CreativeHeldItemKind::Material:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction, "Place");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Remove");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::MaterialBrush:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction, "Paint");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Erase");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::ConnectedFill:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Fill region");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction,
                 "Erase region");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::SurfaceExtrude:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Extrude");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction,
                 "Remove layer");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::TerrainControl:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 editor.terrain.selectionValid ? "Apply edit" : "Paint rods");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction,
                 editor.terrain.selectionValid ? "Cancel edit" : "Erase rods");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Select rod");
      appendQuickEdit(buffer, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainGrade:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Apply grade");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction,
                 "Cancel grade");
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 "Set start rod");
      appendQuickEdit(buffer, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainSculpt:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction, "Sculpt");
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction,
                 "Cancel sculpt");
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 "Sample height");
      appendQuickEdit(buffer, editor);
      break;
    case cr::CreativeHeldItemKind::ObjectSelect:
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Select");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::ObjectMove:
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Move");
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Transform");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::VolumeSelect:
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Corner 1");
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Corner 2");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Expand");
      break;
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Corner 1");
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Apply area");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::VolumeReplace:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction, "Apply");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::LinearArray:
      appendHint(buffer, cr::CreativeInputActionId::PrimaryAction, "Select");
      appendHint(buffer, cr::CreativeInputActionId::SecondaryAction,
                 "Apply array");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::Count:
      return;
  }
  appendToolAccess(buffer);
}

void appendViewportGamepadHints(HintSpecBuffer& buffer,
                                const CreativeEditorState& editor,
                                cr::CreativeHeldItemKind held) noexcept {
  switch (held) {
    case cr::CreativeHeldItemKind::Material:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Place");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Remove");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::MaterialBrush:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Paint");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Erase");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::ConnectedFill:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction,
                 "Fill region");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction,
                 "Erase region");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::SurfaceExtrude:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Extrude");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction,
                 "Remove layer");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::TerrainControl:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction,
                 editor.terrain.selectionValid ? "Apply edit" : "Paint rods");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction,
                 editor.terrain.selectionValid ? "Cancel edit" : "Erase rods");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Select rod");
      break;
    case cr::CreativeHeldItemKind::TerrainGrade:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction,
                 "Apply grade");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction,
                 "Cancel grade");
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 "Set start rod");
      break;
    case cr::CreativeHeldItemKind::TerrainSculpt:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Sculpt");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction,
                 "Cancel sculpt");
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 "Sample height");
      break;
    case cr::CreativeHeldItemKind::ObjectSelect:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Select");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::ObjectMove:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Move");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::VolumeSelect:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction,
                 "Next corner");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Expand");
      break;
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction,
                 editor.volume.selection.phase ==
                         cr::CreativeVolumeSelectionPhase::FirstCorner
                     ? "Apply area"
                     : "Corner 1");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::VolumeReplace:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction, "Apply");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::LinearArray:
      appendHint(buffer, cr::CreativeInputActionId::AcceptAction,
                 "Select/Use");
      appendHint(buffer, cr::CreativeInputActionId::RejectAction, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case cr::CreativeHeldItemKind::Count:
      return;
  }
  appendQuickEdit(buffer, editor);
  if (buffer.count < buffer.specs.size()) {
    appendHint(buffer, cr::CreativeInputActionId::ToggleToolWheel, "Tools");
  }
  if (buffer.count < buffer.specs.size()) {
    appendHint(buffer, cr::CreativeInputActionId::ToggleCatalog, "Catalog");
  }
  if (buffer.count < buffer.specs.size()) {
    appendGamepadHotbar(buffer);
  }
}

void appendContextHints(HintSpecBuffer& buffer,
                        cr::CreativeInputContext context,
                        bool assigningToolWheel) noexcept {
  switch (context) {
    case cr::CreativeInputContext::Catalog:
      appendHintPair(buffer, cr::CreativeInputActionId::CatalogPrevious,
                     cr::CreativeInputActionId::CatalogNext, "Browse");
      appendHintPair(buffer,
                     cr::CreativeInputActionId::CatalogPreviousVariant,
                     cr::CreativeInputActionId::CatalogNextVariant, "Variant");
      appendHintPair(buffer, cr::CreativeInputActionId::CatalogPreviousPage,
                     cr::CreativeInputActionId::CatalogNextPage, "Page");
      appendHint(buffer, cr::CreativeInputActionId::CatalogConfirm, "Equip");
      appendHint(buffer, cr::CreativeInputActionId::CatalogAssignToolWheel,
                 "Assign wheel");
      appendHint(buffer, cr::CreativeInputActionId::CatalogClose, "Close");
      return;
    case cr::CreativeInputContext::ToolWheel:
      appendHintPair(buffer, cr::CreativeInputActionId::ToolWheelPrevious,
                     cr::CreativeInputActionId::ToolWheelNext, "Choose");
      appendHint(buffer, cr::CreativeInputActionId::ToolWheelConfirm,
                 assigningToolWheel ? "Assign" : "Equip");
      if (!assigningToolWheel) {
        appendHint(buffer, cr::CreativeInputActionId::ToolWheelOptions,
                   "Settings");
      }
      appendHint(buffer, cr::CreativeInputActionId::ToolWheelClose,
                 assigningToolWheel ? "Back" : "Close");
      if (!assigningToolWheel) {
        appendHint(buffer, cr::CreativeInputActionId::ToggleToolWheel,
                   "Close wheel");
      }
      return;
    case cr::CreativeInputContext::ToolOptions:
      appendHintPair(buffer, cr::CreativeInputActionId::ToolOptionsPrevious,
                     cr::CreativeInputActionId::ToolOptionsNext, "Option");
      appendHintPair(buffer, cr::CreativeInputActionId::ToolOptionsDecrease,
                     cr::CreativeInputActionId::ToolOptionsIncrease, "Adjust");
      appendHint(buffer, cr::CreativeInputActionId::ToolOptionsConfirm,
                 "Apply");
      appendHint(buffer, cr::CreativeInputActionId::ToolOptionsClose, "Close");
      return;
    case cr::CreativeInputContext::TransformPreview:
      appendHint(buffer, cr::CreativeInputActionId::ConfirmActiveTool, "Apply");
      appendHint(buffer, cr::CreativeInputActionId::CancelActiveTool, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::ToggleTransformControls,
                 "Controls");
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease, "Rotate");
      appendHintPair(buffer, cr::CreativeInputActionId::FlyDown,
                     cr::CreativeInputActionId::FlyUp, "Fly");
      return;
    case cr::CreativeInputContext::TransformControls:
      appendHintPair(buffer,
                     cr::CreativeInputActionId::TransformControlPrevious,
                     cr::CreativeInputActionId::TransformControlNext,
                     "Control");
      appendHint(buffer, cr::CreativeInputActionId::ConfirmActiveTool, "Apply");
      appendHint(buffer, cr::CreativeInputActionId::CancelActiveTool, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::ToggleTransformControls,
                 "Close");
      return;
    case cr::CreativeInputContext::Controls:
      appendHintPair(buffer, cr::CreativeInputActionId::ControlsPrevious,
                     cr::CreativeInputActionId::ControlsNext, "Row");
      appendHintPair(buffer, cr::CreativeInputActionId::ControlsDecrease,
                     cr::CreativeInputActionId::ControlsIncrease, "Adjust");
      appendHint(buffer, cr::CreativeInputActionId::ControlsActivate, "Select");
      appendHint(buffer, cr::CreativeInputActionId::ControlsResetDefaults,
                 "Reset");
      appendHint(buffer, cr::CreativeInputActionId::ControlsClose, "Close");
      return;
    case cr::CreativeInputContext::EditorViewport:
    case cr::CreativeInputContext::TextEntry:
    case cr::CreativeInputContext::Modal:
    case cr::CreativeInputContext::Capture:
    case cr::CreativeInputContext::Count:
      return;
  }
}

[[nodiscard]] constexpr cr::CreativeInputPlatform inputPlatform() noexcept {
#if defined(__APPLE__)
  return cr::CreativeInputPlatform::MacOS;
#else
  return cr::CreativeInputPlatform::WindowsLinux;
#endif
}

}  // namespace

cr::CreativeActionHintFrame resolveCreativeEditorActionHints(
    const CreativeEditorState& editor,
    cr::CreativeInputContext inputContext,
    cr::CreativeControlDevice activeDevice,
    bool captureMode) noexcept {
  if (captureMode || inputContext == cr::CreativeInputContext::Capture ||
      inputContext == cr::CreativeInputContext::TextEntry ||
      inputContext == cr::CreativeInputContext::Modal) {
    return {};
  }
  HintSpecBuffer specs;
  if (inputContext == cr::CreativeInputContext::EditorViewport) {
    const cr::CreativeHeldItemKind held =
        cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind;
    if (activeDevice == cr::CreativeControlDevice::Gamepad) {
      appendViewportGamepadHints(specs, editor, held);
    } else {
      appendViewportKeyboardHints(specs, editor, held);
    }
  } else {
    appendContextHints(
        specs, inputContext,
        editor.catalog.toolWheelAssignmentCatalogEntryIndex.has_value());
  }
  if (specs.capacityExceeded) {
    cr::CreativeActionHintFrame overflow;
    overflow.capacityExceeded = true;
    return overflow;
  }
  return cr::resolveCreativeActionHints(
      editor.controlProfile, inputContext, activeDevice, inputPlatform(),
      specs.items());
}

cr::CreativeUiWidgetFrame buildCreativeEditorActionHintWidgetFrame(
    const cr::CreativeActionHintFrame& hints,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept {
  cr::CreativeUiWidgetFrame frame;
  cr::resetCreativeUiWidgetFrame(frame, drawableWidth, drawableHeight);
  if (hints.invalidInput || hints.capacityExceeded || hints.count == 0U) {
    return frame;
  }

  constexpr float kOuterMargin = 8.0F;
  constexpr float kGap = 5.0F;
  constexpr float kMinimumItemWidth = 126.0F;
  constexpr float kRibbonHeight = 44.0F;
  constexpr float kTextPadding = 16.0F;
  constexpr float kGlyphWidth = 10.0F;
  constexpr float kGlyphAdvance = 12.0F;
  const float availableWidth =
      std::max(1.0F, static_cast<float>(drawableWidth) - kOuterMargin * 2.0F);
  if (availableWidth < kMinimumItemWidth) {
    return frame;
  }
  const auto textWidth = [=](std::size_t length) {
    return length == 0U
               ? 0.0F
               : kGlyphWidth +
                     static_cast<float>(length - 1U) * kGlyphAdvance;
  };
  std::array<float, cr::kCreativeActionHintCapacity> itemWidths{};
  std::size_t visibleCount = 0U;
  float totalWidth = 0.0F;
  for (const cr::CreativeActionHint& hint : hints.items()) {
    const float requestedWidth = std::max(
        kMinimumItemWidth,
        std::max(textWidth(hint.chord.length), textWidth(hint.label.length)) +
            kTextPadding);
    const float nextTotal = totalWidth +
                            (visibleCount == 0U ? 0.0F : kGap) +
                            requestedWidth;
    if (nextTotal > availableWidth) {
      frame.textTruncated = visibleCount == 0U;
      break;
    }
    itemWidths[visibleCount++] = requestedWidth;
    totalWidth = nextTotal;
  }
  const float originX =
      std::max(kOuterMargin,
               (static_cast<float>(drawableWidth) - totalWidth) * 0.5F);
  const float originY = std::max(
      kOuterMargin, static_cast<float>(drawableHeight) - kRibbonHeight - 86.0F);

  float tileX = originX;
  for (std::size_t index = 0U; index < visibleCount; ++index) {
    const cr::CreativeActionHint& hint = hints.hints[index];
    const float itemWidth = itemWidths[index];
    const iggy3d::CreativeUiRect tile{
        tileX, originY, itemWidth, kRibbonHeight};
    static_cast<void>(cr::appendCreativeUiPanel(frame, tile));
    static_cast<void>(cr::appendCreativeUiLabel(
        frame, {tile.x + 8.0F, tile.y + 7.0F, tile.width - 16.0F, 12.0F},
        hint.chord.view(), iggy3d::CreativeUiTone::Accent));
    static_cast<void>(cr::appendCreativeUiLabel(
        frame, {tile.x + 8.0F, tile.y + 25.0F, tile.width - 16.0F, 12.0F},
        hint.label.view(), iggy3d::CreativeUiTone::TextPrimary));
    tileX += itemWidth + kGap;
  }
  frame.textTruncated = frame.textTruncated || hints.textTruncated;
  return frame;
}

void appendCreativeEditorActionHintsOverlay(
    const CreativeEditorState& editor,
    cr::CreativeInputContext inputContext,
    cr::CreativeControlDevice activeDevice,
    bool captureMode,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  // Full-screen widget panels already expose their controls in-place.
  if (inputContext == cr::CreativeInputContext::Catalog ||
      inputContext == cr::CreativeInputContext::ToolOptions ||
      inputContext == cr::CreativeInputContext::Controls) {
    return;
  }
  const cr::CreativeActionHintFrame hints = resolveCreativeEditorActionHints(
      editor, inputContext, activeDevice, captureMode);
  const cr::CreativeUiWidgetFrame widgetFrame =
      buildCreativeEditorActionHintWidgetFrame(hints, drawableWidth,
                                               drawableHeight);
  static_cast<void>(iggy3d::appendCreativeUiWidgetOverlay(
      widgetFrame, drawableWidth, drawableHeight, uiRects, glyphs));
}

}  // namespace iggy3d_creative_app
