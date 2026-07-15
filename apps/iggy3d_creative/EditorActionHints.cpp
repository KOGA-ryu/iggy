#include "EditorActionHints.hpp"

#include <algorithm>
#include <array>
#include <span>

#include "EditorState.hpp"
#include "EditorToolCapabilities.hpp"
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

[[nodiscard]] std::string_view terrainControlApplyLabel(
    const CreativeEditorState& editor) noexcept {
  switch (editor.toolSettings.terrainRodStampMode) {
    case cr::CreativeTerrainRodStampMode::Single:
      return editor.terrain.selectionValid ? "Apply edit" : "Paint rods";
    case cr::CreativeTerrainRodStampMode::Seed:
      return "Seed rods";
    case cr::CreativeTerrainRodStampMode::Count:
      break;
  }
  return "Terrain action";
}

[[nodiscard]] std::string_view terrainControlRejectLabel(
    const CreativeEditorState& editor) noexcept {
  switch (editor.toolSettings.terrainRodStampMode) {
    case cr::CreativeTerrainRodStampMode::Single:
      return editor.terrain.selectionValid ? "Cancel edit" : "Erase rods";
    case cr::CreativeTerrainRodStampMode::Seed:
      return "Clear rods";
    case cr::CreativeTerrainRodStampMode::Count:
      break;
  }
  return "Cancel terrain";
}

[[nodiscard]] std::string_view terrainControlPickLabel(
    const CreativeEditorState& editor) noexcept {
  switch (editor.toolSettings.terrainRodStampMode) {
    case cr::CreativeTerrainRodStampMode::Single:
      return "Select rod";
    case cr::CreativeTerrainRodStampMode::Seed:
      return "Sample rod";
    case cr::CreativeTerrainRodStampMode::Count:
      break;
  }
  return "Terrain sample";
}

void appendQuickEdit(HintSpecBuffer& buffer,
                     const CreativeEditorState& editor) noexcept {
  const cr::CreativeHeldItemKind held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind;
  switch (describeCreativeEditorToolCapability(held).quickEditProfile) {
    case CreativeEditorQuickEditProfile::TerrainControl:
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                     cr::CreativeInputActionId::QuickEditNext, "Height");
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease, "Radius");
      return;
    case CreativeEditorQuickEditProfile::TerrainGrade:
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                     cr::CreativeInputActionId::QuickEditNext, "End height");
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease, "Width");
      return;
    case CreativeEditorQuickEditProfile::TerrainSculpt:
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                     cr::CreativeInputActionId::QuickEditNext, "Strength");
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease, "Radius");
      return;
    case CreativeEditorQuickEditProfile::TerrainProfile:
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                     cr::CreativeInputActionId::QuickEditNext, "Amplitude");
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease, "Radius");
      return;
    case CreativeEditorQuickEditProfile::TerrainPath:
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                     cr::CreativeInputActionId::QuickEditNext,
                     cr::creativeTerrainPathUsesDepth(
                         editor.toolSettings.terrainPathKind)
                         ? "Depth"
                         : "Rise");
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease, "Width");
      return;
    case CreativeEditorQuickEditProfile::TerrainRegion:
      if (editor.terrain.region.stamp.active) {
        appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                       cr::CreativeInputActionId::QuickEditNext, "Control");
        std::string_view adjustment = "Rotate";
        switch (editor.terrain.region.stamp.selectedControl) {
          case CreativeTerrainStampTransformControl::Rotation: break;
          case CreativeTerrainStampTransformControl::MirrorX:
            adjustment = "Mirror X";
            break;
          case CreativeTerrainStampTransformControl::MirrorZ:
            adjustment = "Mirror Z";
            break;
          case CreativeTerrainStampTransformControl::HeightOffset:
            adjustment = "Height";
            break;
          case CreativeTerrainStampTransformControl::Count: return;
        }
        appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                       cr::CreativeInputActionId::QuickEditIncrease,
                       adjustment);
        return;
      }
      if (cr::creativeTerrainRegionUsesTargetHeight(
              editor.toolSettings.terrainRegionOperation)) {
        appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                       cr::CreativeInputActionId::QuickEditNext, "Target");
      } else if (cr::creativeTerrainRegionUsesAmount(
                     editor.toolSettings.terrainRegionOperation)) {
        appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                       cr::CreativeInputActionId::QuickEditNext, "Amount");
      }
      appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                     cr::CreativeInputActionId::QuickEditIncrease,
                     "Operation");
      return;
    case CreativeEditorQuickEditProfile::None:
      return;
    case CreativeEditorQuickEditProfile::Generic:
    case CreativeEditorQuickEditProfile::Count:
      break;
  }
  if (editor.quickEdit.options.count == 0U) {
    return;
  }
  appendHintPair(buffer, cr::CreativeInputActionId::QuickEditPrevious,
                 cr::CreativeInputActionId::QuickEditNext, "Setting");
  appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                 cr::CreativeInputActionId::QuickEditIncrease, "Adjust");
}

void appendViewportHints(HintSpecBuffer& buffer,
                         const CreativeEditorState& editor,
                         cr::CreativeHeldItemKind held,
                         cr::CreativeControlDevice device) noexcept {
  const bool gamepad = device == cr::CreativeControlDevice::Gamepad;
  const cr::CreativeInputActionId positiveAction =
      gamepad ? cr::CreativeInputActionId::AcceptAction
              : cr::CreativeInputActionId::SecondaryAction;
  const cr::CreativeInputActionId negativeAction =
      gamepad ? cr::CreativeInputActionId::RejectAction
              : cr::CreativeInputActionId::PrimaryAction;
  const CreativeEditorToolCapability& capability =
      describeCreativeEditorToolCapability(held);
  const bool keyboardQuickEdit = capability.keyboardQuickEditHints;
  switch (capability.actionHintProfile) {
    case CreativeEditorActionHintProfile::Material:
      appendHint(buffer, positiveAction, "Place");
      appendHint(buffer, negativeAction, "Remove");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::MaterialBrush:
      appendHint(buffer, positiveAction, "Paint");
      appendHint(buffer, negativeAction, "Erase");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::ConnectedFill:
      appendHint(buffer, positiveAction, "Fill region");
      appendHint(buffer, negativeAction, "Erase region");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::SurfaceExtrude:
      appendHint(buffer, positiveAction, "Extrude");
      appendHint(buffer, negativeAction, "Remove layer");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::TerrainControl:
      appendHint(buffer, positiveAction, terrainControlApplyLabel(editor));
      appendHint(buffer, negativeAction, terrainControlRejectLabel(editor));
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 terrainControlPickLabel(editor));
      break;
    case CreativeEditorActionHintProfile::TerrainPaint:
      switch (editor.toolSettings.terrainPaintMode) {
        case cr::CreativeTerrainPaintMode::Brush:
          appendHint(buffer, positiveAction, "Paint surface");
          appendHint(buffer, negativeAction, "Restore grass");
          break;
        case cr::CreativeTerrainPaintMode::Connected:
          appendHint(buffer, positiveAction, "Fill connected");
          appendHint(buffer, negativeAction, "Restore connected");
          break;
        case cr::CreativeTerrainPaintMode::Region:
          switch (editor.terrainPaint.regionPhase) {
            case CreativeEditorTerrainPaintRegionPhase::Empty:
              appendHint(buffer, positiveAction, "Corner 1");
              break;
            case CreativeEditorTerrainPaintRegionPhase::FirstCorner:
              appendHint(buffer, positiveAction, "Corner 2");
              appendHint(buffer, negativeAction, "Cancel region");
              break;
            case CreativeEditorTerrainPaintRegionPhase::Complete:
              appendHint(buffer, positiveAction, "Replace region");
              appendHint(buffer, negativeAction, "Back to corner 2");
              break;
          }
          break;
        case cr::CreativeTerrainPaintMode::Count:
          break;
      }
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 "Sample surface");
      break;
    case CreativeEditorActionHintProfile::TerrainGrade:
      appendHint(buffer, positiveAction, "Apply grade");
      appendHint(buffer, negativeAction, "Cancel grade");
      appendHint(buffer, cr::CreativeInputActionId::PickAction,
                 "Set start rod");
      break;
    case CreativeEditorActionHintProfile::TerrainSculpt:
      appendHint(buffer, positiveAction, "Sculpt");
      appendHint(buffer, negativeAction, "Cancel sculpt");
      if (cr::creativeTerrainSculptUsesTargetHeight(
              editor.toolSettings.terrainSculptMode)) {
        appendHint(buffer, cr::CreativeInputActionId::PickAction,
                   "Sample height");
      }
      break;
    case CreativeEditorActionHintProfile::TerrainProfile:
      appendHint(buffer, positiveAction, "Apply profile");
      if (editor.terrain.profile.baseLocked) {
        appendHint(buffer, negativeAction, "Auto base");
      }
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Lock base");
      break;
    case CreativeEditorActionHintProfile::TerrainPath:
      appendHint(buffer, positiveAction, "Commit path");
      appendHint(buffer, negativeAction, "Back point");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Add point");
      break;
    case CreativeEditorActionHintProfile::TerrainRegion: {
      if (editor.terrain.region.stamp.active) {
        appendHint(buffer, positiveAction, "Stamp terrain");
        appendHint(buffer, negativeAction, "Cancel stamp");
        break;
      }
      std::string_view positiveLabel = "Corner 1";
      if (editor.volume.selection.phase ==
          cr::CreativeVolumeSelectionPhase::FirstCorner) {
        positiveLabel = "Corner 2";
      } else if (editor.volume.selection.phase ==
                 cr::CreativeVolumeSelectionPhase::Complete) {
        positiveLabel = "Apply region";
      }
      appendHint(buffer, positiveAction, positiveLabel);
      appendHint(buffer, negativeAction, "Cancel");
      if (cr::creativeTerrainRegionUsesTargetHeight(
              editor.toolSettings.terrainRegionOperation)) {
        appendHint(buffer, cr::CreativeInputActionId::PickAction,
                   "Sample height");
      }
      break;
    }
    case CreativeEditorActionHintProfile::ObjectSelect:
      appendHint(buffer, gamepad ? positiveAction : negativeAction, "Select");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::ObjectMove:
      appendHint(buffer, gamepad ? positiveAction : negativeAction, "Move");
      appendHint(buffer, gamepad ? negativeAction : positiveAction,
                 gamepad ? "Cancel" : "Transform");
      if (gamepad) {
        appendHint(buffer, cr::CreativeInputActionId::QuickEditNext,
                   "Transform");
      }
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::ObjectGroup:
      appendHint(buffer, positiveAction, "Group / ungroup");
      break;
    case CreativeEditorActionHintProfile::VolumeSelect:
      appendHint(buffer, gamepad ? positiveAction : negativeAction,
                 gamepad ? "Next corner" : "Corner 1");
      appendHint(buffer, gamepad ? negativeAction : positiveAction,
                 gamepad ? "Cancel" : "Corner 2");
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Expand");
      break;
    case CreativeEditorActionHintProfile::DirectShapeVolume:
      if (gamepad) {
        appendHint(buffer, positiveAction,
                   editor.volume.selection.phase ==
                           cr::CreativeVolumeSelectionPhase::FirstCorner
                       ? "Apply area"
                       : "Corner 1");
        appendHint(buffer, negativeAction, "Cancel");
      } else {
        appendHint(buffer, negativeAction, "Corner 1");
        appendHint(buffer, positiveAction, "Apply area");
      }
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::VolumeOperation:
      appendHint(buffer, positiveAction, "Apply");
      if (gamepad) {
        appendHint(buffer, negativeAction, "Cancel");
      }
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::LinearArray:
      if (gamepad) {
        appendHint(buffer, positiveAction, "Select/Use");
        appendHint(buffer, negativeAction, "Cancel");
      } else {
        appendHint(buffer, negativeAction, "Select");
        appendHint(buffer, positiveAction, "Apply array");
      }
      appendHint(buffer, cr::CreativeInputActionId::PickAction, "Pick block");
      break;
    case CreativeEditorActionHintProfile::None:
    case CreativeEditorActionHintProfile::Count:
      return;
  }
  if (gamepad || keyboardQuickEdit) {
    appendQuickEdit(buffer, editor);
  }
  appendToolAccess(buffer);
  if (gamepad && buffer.count < buffer.specs.size()) {
    appendGamepadHotbar(buffer);
  }
}

void appendContextHints(HintSpecBuffer& buffer,
                        const CreativeEditorState& editor,
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
      appendHint(
          buffer, cr::CreativeInputActionId::CatalogConfirm,
          editor.catalog.assetAction ==
                  CreativeEditorCatalogAssetAction::ReplaceSelection
              ? "Replace"
              : editor.catalog.assetAction ==
                        CreativeEditorCatalogAssetAction::ManageAsset
                    ? "Manage"
                    : "Equip");
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
    case cr::CreativeInputContext::AssetReplacementPreview:
      appendHint(buffer, cr::CreativeInputActionId::ConfirmActiveTool,
                 "Replace");
      appendHint(buffer, cr::CreativeInputActionId::CancelActiveTool, "Cancel");
      appendHintPair(buffer, cr::CreativeInputActionId::FlyDown,
                     cr::CreativeInputActionId::FlyUp, "Fly");
      return;
    case cr::CreativeInputContext::AssetLibrary:
      appendHintPair(buffer, cr::CreativeInputActionId::CatalogPrevious,
                     cr::CreativeInputActionId::CatalogNext, "Asset");
      appendHintPair(buffer, cr::CreativeInputActionId::CatalogPreviousVariant,
                     cr::CreativeInputActionId::CatalogNextVariant, "Action");
      appendHint(buffer, cr::CreativeInputActionId::CatalogConfirm, "Apply");
      appendHint(buffer, cr::CreativeInputActionId::CatalogClose, "Back");
      return;
    case cr::CreativeInputContext::AuthoredAssetEditMenu:
      appendHintPair(buffer, cr::CreativeInputActionId::ToolOptionsPrevious,
                     cr::CreativeInputActionId::ToolOptionsNext, "Action");
      appendHint(buffer, cr::CreativeInputActionId::ConfirmActiveTool,
                 "Select");
      appendHint(buffer, cr::CreativeInputActionId::CancelActiveTool, "Back");
      return;
    case cr::CreativeInputContext::TransformPreview:
      appendHint(buffer, cr::CreativeInputActionId::ConfirmActiveTool, "Apply");
      appendHint(buffer, cr::CreativeInputActionId::CancelActiveTool, "Cancel");
      appendHint(buffer, cr::CreativeInputActionId::ToggleTransformControls,
                 "Controls");
      appendHint(buffer, cr::CreativeInputActionId::QuickEditNext, "Mode");
      {
        std::string_view adjustment = "Axis";
        switch (editor.transform.transformMode) {
          case CreativeEditorTransformMode::Move: break;
          case CreativeEditorTransformMode::Rotate:
            switch (editor.transform.constraint) {
              case cr::CreativeSelectionPlacementAxis::X:
                adjustment = "Rotate X";
                break;
              case cr::CreativeSelectionPlacementAxis::Z:
                adjustment = "Rotate Z";
                break;
              case cr::CreativeSelectionPlacementAxis::Free:
              case cr::CreativeSelectionPlacementAxis::Y:
              case cr::CreativeSelectionPlacementAxis::Count:
                adjustment = "Rotate Y";
                break;
            }
            break;
          case CreativeEditorTransformMode::Scale:
            switch (editor.transform.constraint) {
              case cr::CreativeSelectionPlacementAxis::X:
                adjustment = "Scale X";
                break;
              case cr::CreativeSelectionPlacementAxis::Y:
                adjustment = "Scale Y";
                break;
              case cr::CreativeSelectionPlacementAxis::Z:
                adjustment = "Scale Z";
                break;
              case cr::CreativeSelectionPlacementAxis::Free:
                adjustment = "Scale all";
                break;
              case cr::CreativeSelectionPlacementAxis::Count:
                adjustment = "Scale";
                break;
            }
            break;
          case CreativeEditorTransformMode::Count:
            adjustment = "Adjust";
            break;
        }
        appendHintPair(buffer, cr::CreativeInputActionId::QuickEditDecrease,
                       cr::CreativeInputActionId::QuickEditIncrease,
                       adjustment);
      }
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
    case cr::CreativeInputContext::RuntimePlay:
    case cr::CreativeInputContext::TextEntry:
    case cr::CreativeInputContext::Modal:
    case cr::CreativeInputContext::Capture:
    case cr::CreativeInputContext::DesktopUi:
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
      inputContext == cr::CreativeInputContext::Modal ||
      inputContext == cr::CreativeInputContext::DesktopUi) {
    return {};
  }
  HintSpecBuffer specs;
  if (inputContext == cr::CreativeInputContext::EditorViewport) {
    const cr::CreativeHeldItemKind held =
        cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind;
    appendViewportHints(specs, editor, held, activeDevice);
  } else {
    appendContextHints(
        specs, editor, inputContext,
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
