#include "EditorToolOptions.hpp"

#include <array>
#include <string>
#include <string_view>

#include "EditorPlacement.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorToolCapabilities.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool assetPlacementOption(
    cr::CreativeToolOptionId option) noexcept {
  switch (option) {
    case cr::CreativeToolOptionId::AssetPlacementMode:
    case cr::CreativeToolOptionId::AssetScatterRadius:
    case cr::CreativeToolOptionId::AssetScatterDensity:
    case cr::CreativeToolOptionId::AssetScatterSpacing:
    case cr::CreativeToolOptionId::AssetScatterYaw:
    case cr::CreativeToolOptionId::AssetScatterScale:
    case cr::CreativeToolOptionId::AssetScatterSlope:
      return true;
    default:
      return false;
  }
}

constexpr std::array kAssetScatterOptions{
    cr::CreativeToolOptionId::AssetScatterRadius,
    cr::CreativeToolOptionId::AssetScatterDensity,
    cr::CreativeToolOptionId::AssetScatterSpacing,
    cr::CreativeToolOptionId::AssetScatterYaw,
    cr::CreativeToolOptionId::AssetScatterScale,
    cr::CreativeToolOptionId::AssetScatterSlope,
};

}  // namespace

cr::CreativeToolOptionList creativeEditorToolOptionsForEntry(
    cr::CreativeHotbarEntry entry,
    const cr::CreativeToolSettings& settings) noexcept {
  cr::CreativeToolOptionList options =
      cr::creativeToolOptionsForHeldItem(entry.kind, settings);
  if (describeCreativeEditorToolCapability(entry.kind).optionFilterProfile !=
      CreativeEditorToolOptionFilterProfile::MaterialPlacement) {
    return options;
  }

  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(entry.objectKind);
  if (entry.objectKind == cr::CreativeObjectKind::Unknown ||
      descriptor.kind != entry.objectKind ||
      !descriptorSupportsBrushPlacement(descriptor)) {
    return {};
  }

  const bool supportsYaw =
      creativeBrushSupportsPlacementYaw(entry.objectKind);
  const bool usesFixedVoxelGrid =
      descriptor.placementPolicy.storagePolicy ==
      cr::CreativePlacementStoragePolicy::VoxelCell;
  const bool importedAsset = !cr::creativeHotbarAssetId(entry).empty();
  cr::CreativeToolOptionList filtered;
  const auto appendOption = [&filtered](cr::CreativeToolOptionId option) {
    if (filtered.count >= filtered.ids.size()) {
      filtered.capacityExceeded = true;
      return;
    }
    filtered.ids[filtered.count++] = option;
  };
  if (importedAsset) {
    appendOption(cr::CreativeToolOptionId::AssetPlacementMode);
    if (settings.assetPlacementMode ==
        cr::CreativeAssetPlacementMode::Scatter) {
      for (const cr::CreativeToolOptionId option : kAssetScatterOptions) {
        appendOption(option);
      }
      return filtered;
    }
  }
  for (std::size_t readIndex = 0U; readIndex < options.count; ++readIndex) {
    const cr::CreativeToolOptionId option = options.ids[readIndex];
    if (assetPlacementOption(option) ||
        (!supportsYaw && option == cr::CreativeToolOptionId::PlacementYaw) ||
        (usesFixedVoxelGrid &&
         option == cr::CreativeToolOptionId::SnapIncrement)) {
      continue;
    }
    appendOption(option);
  }
  return filtered;
}

CreativeEditorToolOptionsCommandList
creativeEditorToolOptionCommandsForEntry(
    cr::CreativeHotbarEntry entry) noexcept {
  CreativeEditorToolOptionsCommandList commands;
  switch (describeCreativeEditorToolCapability(entry.kind).commandProfile) {
    case CreativeEditorToolCommandProfile::None:
      break;
    case CreativeEditorToolCommandProfile::MaterialBrush:
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot;
      break;
    case CreativeEditorToolCommandProfile::ObjectGroup:
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::EditGroupContents;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::UpdateSavedAsset;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstance;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::RefreshSafeSavedAssetInstances;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::ForceRefreshSavedAssetInstances;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::UngroupSelection;
      break;
    case CreativeEditorToolCommandProfile::ObjectMove:
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::TransformSelection;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::ResetSelectionTransform;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::DuplicateSelection;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::DeleteSelection;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::ToggleSelectionLocked;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::DetachAttachment;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::GroupSelection;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::UngroupSelection;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::EditGroupContents;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::UpdateSavedAsset;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstance;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::RefreshSafeSavedAssetInstances;
      commands.ids[commands.count++] =
          CreativeEditorToolOptionsCommandId::ForceRefreshSavedAssetInstances;
      break;
    case CreativeEditorToolCommandProfile::Count:
      break;
  }
  return commands;
}

std::size_t creativeEditorToolOptionsRowCount(
    const CreativeEditorToolOptionsState& state) noexcept {
  return state.options.count + state.commands.count;
}

namespace {

[[nodiscard]] bool sameQuickEditTarget(
    const cr::CreativeHotbarEntry& lhs,
    const cr::CreativeHotbarEntry& rhs) noexcept {
  return lhs.kind == rhs.kind && lhs.objectKind == rhs.objectKind &&
         cr::creativeHotbarAssetId(lhs) == cr::creativeHotbarAssetId(rhs);
}

void rebuildQuickEditOptions(CreativeEditorState& editor,
                             bool resetSelection) {
  CreativeEditorQuickEditState& state = editor.quickEdit;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool targetChanged = !sameQuickEditTarget(state.targetEntry, held);
  state.targetEntry = held;
  state.options =
      creativeEditorToolOptionsForEntry(held, editor.toolSettings);
  if (resetSelection || targetChanged || state.options.count == 0U) {
    state.selectedIndex = 0U;
  } else if (state.selectedIndex >= state.options.count) {
    state.selectedIndex = state.options.count - 1U;
  }
}

}  // namespace

void syncCreativeEditorQuickEdit(CreativeEditorState& editor) {
  rebuildQuickEditOptions(editor, false);
}

bool processCreativeEditorQuickEditAction(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  switch (describeCreativeEditorToolCapability(held.kind).quickEditProfile) {
    case CreativeEditorQuickEditProfile::LogicLink:
      switch (action) {
        case cr::CreativeInputActionId::QuickEditPrevious:
        case cr::CreativeInputActionId::QuickEditDecrease:
          return cycleCreativeEditorLogicLinkAction(editor.logicLinks, -1);
        case cr::CreativeInputActionId::QuickEditNext:
        case cr::CreativeInputActionId::QuickEditIncrease:
          return cycleCreativeEditorLogicLinkAction(editor.logicLinks, 1);
        default:
          return false;
      }
    case CreativeEditorQuickEditProfile::TerrainControl:
      return processCreativeEditorTerrainQuickEdit(editor.terrain, action);
    case CreativeEditorQuickEditProfile::TerrainGrade:
      return processCreativeEditorTerrainGradeQuickEdit(editor.terrain.grade,
                                                        action);
    case CreativeEditorQuickEditProfile::TerrainSculpt:
      return processCreativeEditorTerrainSculptQuickEdit(editor, action);
    case CreativeEditorQuickEditProfile::TerrainProfile:
      return processCreativeEditorTerrainProfileQuickEdit(editor, action);
    case CreativeEditorQuickEditProfile::TerrainPath:
      return processCreativeEditorTerrainPathQuickEdit(editor, action);
    case CreativeEditorQuickEditProfile::TerrainRegion:
      return processCreativeEditorTerrainRegionQuickEdit(editor, action);
    case CreativeEditorQuickEditProfile::None:
      return false;
    case CreativeEditorQuickEditProfile::Generic:
    case CreativeEditorQuickEditProfile::Count:
      break;
  }
  rebuildQuickEditOptions(editor, false);
  CreativeEditorQuickEditState& state = editor.quickEdit;
  if (state.options.count == 0U || state.options.capacityExceeded) {
    return false;
  }

  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      const std::size_t before = state.selectedIndex;
      const std::int32_t direction =
          action == cr::CreativeInputActionId::QuickEditPrevious ? -1 : 1;
      const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
          state.selectedIndex, state.options.count, direction);
      if (next.valid) {
        state.selectedIndex = next.index;
      }
      return state.selectedIndex != before;
    }
    case cr::CreativeInputActionId::QuickEditDecrease:
    case cr::CreativeInputActionId::QuickEditIncrease: {
      const cr::CreativeToolOptionId option =
          state.options.ids[state.selectedIndex];
      const std::int32_t direction =
          action == cr::CreativeInputActionId::QuickEditDecrease ? -1 : 1;
      const cr::CreativeToolOptionAdjustReceipt receipt =
          cr::adjustCreativeToolOption(editor.toolSettings, option, direction,
                                       editor.brushPalette);
      if (!receipt.changed) {
        return false;
      }
      editor.placeCellSize =
          cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement);
      static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
          editor.interaction.materialBrushPresets,
          editor.interaction.hotbar, editor.toolSettings));
      rebuildQuickEditOptions(editor, false);
      return true;
    }
    default:
      return false;
  }
}

std::string creativeEditorQuickEditStatusLabel(
    const CreativeEditorState& editor) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  switch (describeCreativeEditorToolCapability(held.kind).quickEditProfile) {
    case CreativeEditorQuickEditProfile::LogicLink:
      return std::string{cr::toString(editor.logicLinks.action)};
    case CreativeEditorQuickEditProfile::TerrainControl:
      return creativeEditorTerrainQuickEditLabel(editor.terrain);
    case CreativeEditorQuickEditProfile::TerrainGrade:
      return creativeEditorTerrainGradeQuickEditLabel(editor.terrain.grade);
    case CreativeEditorQuickEditProfile::TerrainSculpt:
      return creativeEditorTerrainSculptQuickEditLabel(editor);
    case CreativeEditorQuickEditProfile::TerrainProfile:
      return creativeEditorTerrainProfileQuickEditLabel(editor);
    case CreativeEditorQuickEditProfile::TerrainPath:
      return creativeEditorTerrainPathQuickEditLabel(editor);
    case CreativeEditorQuickEditProfile::TerrainRegion:
      return creativeEditorTerrainRegionQuickEditLabel(editor);
    case CreativeEditorQuickEditProfile::None:
      return {};
    case CreativeEditorQuickEditProfile::Generic:
    case CreativeEditorQuickEditProfile::Count:
      break;
  }
  const CreativeEditorQuickEditState& state = editor.quickEdit;
  if (!sameQuickEditTarget(state.targetEntry, held) ||
      state.selectedIndex >= state.options.count) {
    return {};
  }
  const cr::CreativeToolOptionId option =
      state.options.ids[state.selectedIndex];
  const cr::CreativeToolOptionDescriptor* descriptor =
      cr::creativeToolOptionDescriptor(option);
  if (descriptor == nullptr) {
    return {};
  }
  std::string label(descriptor->label);
  label.append(" ");
  label.append(cr::creativeToolOptionValueLabel(editor.toolSettings, option));
  return label;
}

}  // namespace iggy3d_creative_app
