#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "EditorToolOptions.hpp"

namespace iggy3d::creative {
struct CreativeAuthoredAssetDefinition;
}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorAuthoredAssetLibrary;
struct CreativeDesktopObjectActionContext;
struct CreativeEditorDesktopUiState;
struct CreativeEditorWorldLayoutState;

using CreativeEditorObjectActionCapability =
    iggy3d::creative::CreativeSemanticObjectActionAdmission;
using CreativeEditorObjectActionCapabilities =
    iggy3d::creative::CreativeSemanticObjectActionAdmissions;

enum class CreativeEditorObjectActionActivationKind : std::uint8_t {
  BeginTransformSession,
  ApplyResetTransform,
  DuplicateSelection,
  DeleteSelection,
  ToggleSelectionVisibility,
  ToggleSelectionLocked,
  ReattachAttachment,
  DetachAttachment,
  GroupSelection,
  UngroupSelection,
  SaveSelectionAsAsset,
  UpdateSavedAsset,
  RefreshSavedAssetInstance,
  RefreshSafeSavedAssetInstances,
  ForceRefreshSavedAssetInstances,
  Count,
};

struct CreativeEditorObjectActionDescriptor {
  CreativeEditorToolOptionsCommandId command =
      CreativeEditorToolOptionsCommandId::Count;
  iggy3d::creative::CreativeSemanticObjectAction semanticAction =
      iggy3d::creative::CreativeSemanticObjectAction::Count;
  CreativeEditorObjectActionActivationKind activation =
      CreativeEditorObjectActionActivationKind::Count;
  std::string_view label;
};

inline constexpr CreativeEditorToolOptionsCommandId
    kFirstCreativeEditorObjectActionCommand =
        CreativeEditorToolOptionsCommandId::TransformSelection;
inline constexpr CreativeEditorToolOptionsCommandId
    kLastCreativeEditorObjectActionCommand =
        CreativeEditorToolOptionsCommandId::ForceRefreshSavedAssetInstances;
inline constexpr std::size_t kCreativeEditorObjectActionDescriptorCount =
    static_cast<std::size_t>(kLastCreativeEditorObjectActionCommand) -
    static_cast<std::size_t>(kFirstCreativeEditorObjectActionCommand) + 1U;

inline constexpr std::array<CreativeEditorObjectActionDescriptor,
                            kCreativeEditorObjectActionDescriptorCount>
    kCreativeEditorObjectActionDescriptors{{
        {CreativeEditorToolOptionsCommandId::TransformSelection,
         iggy3d::creative::CreativeSemanticObjectAction::TransformSelection,
         CreativeEditorObjectActionActivationKind::BeginTransformSession,
         "TRANSFORM"},
        {CreativeEditorToolOptionsCommandId::ResetSelectionTransform,
         iggy3d::creative::CreativeSemanticObjectAction::TransformSelection,
         CreativeEditorObjectActionActivationKind::ApplyResetTransform,
         "RESET TRANSFORM"},
        {CreativeEditorToolOptionsCommandId::DuplicateSelection,
         iggy3d::creative::CreativeSemanticObjectAction::Duplicate,
         CreativeEditorObjectActionActivationKind::DuplicateSelection,
         "DUPLICATE"},
        {CreativeEditorToolOptionsCommandId::DeleteSelection,
         iggy3d::creative::CreativeSemanticObjectAction::Delete,
         CreativeEditorObjectActionActivationKind::DeleteSelection, "DELETE"},
        {CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility,
         iggy3d::creative::CreativeSemanticObjectAction::SetVisible,
         CreativeEditorObjectActionActivationKind::ToggleSelectionVisibility,
         "VISIBILITY"},
        {CreativeEditorToolOptionsCommandId::ToggleSelectionLocked,
         iggy3d::creative::CreativeSemanticObjectAction::SetLocked,
         CreativeEditorObjectActionActivationKind::ToggleSelectionLocked,
         "LOCK"},
        {CreativeEditorToolOptionsCommandId::ReattachAttachment,
         iggy3d::creative::CreativeSemanticObjectAction::StructuralMutation,
         CreativeEditorObjectActionActivationKind::ReattachAttachment,
         "REATTACH"},
        {CreativeEditorToolOptionsCommandId::DetachAttachment,
         iggy3d::creative::CreativeSemanticObjectAction::StructuralMutation,
         CreativeEditorObjectActionActivationKind::DetachAttachment, "DETACH"},
        {CreativeEditorToolOptionsCommandId::GroupSelection,
         iggy3d::creative::CreativeSemanticObjectAction::StructuralMutation,
         CreativeEditorObjectActionActivationKind::GroupSelection, "GROUP"},
        {CreativeEditorToolOptionsCommandId::UngroupSelection,
         iggy3d::creative::CreativeSemanticObjectAction::StructuralMutation,
         CreativeEditorObjectActionActivationKind::UngroupSelection, "UNGROUP"},
        {CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset,
         iggy3d::creative::CreativeSemanticObjectAction::Count,
         CreativeEditorObjectActionActivationKind::SaveSelectionAsAsset,
         "SAVE AS ASSET"},
        {CreativeEditorToolOptionsCommandId::UpdateSavedAsset,
         iggy3d::creative::CreativeSemanticObjectAction::Count,
         CreativeEditorObjectActionActivationKind::UpdateSavedAsset,
         "UPDATE SAVED ASSET"},
        {CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstance,
         iggy3d::creative::CreativeSemanticObjectAction::Count,
         CreativeEditorObjectActionActivationKind::RefreshSavedAssetInstance,
         "REFRESH THIS INSTANCE"},
        {CreativeEditorToolOptionsCommandId::RefreshSafeSavedAssetInstances,
         iggy3d::creative::CreativeSemanticObjectAction::Count,
         CreativeEditorObjectActionActivationKind::
             RefreshSafeSavedAssetInstances,
         "REFRESH SAFE INSTANCES"},
        {CreativeEditorToolOptionsCommandId::ForceRefreshSavedAssetInstances,
         iggy3d::creative::CreativeSemanticObjectAction::Count,
         CreativeEditorObjectActionActivationKind::
             ForceRefreshSavedAssetInstances,
         "FORCE REFRESH ALL"},
    }};

[[nodiscard]] constexpr bool creativeEditorObjectActionDescriptorsValid()
    noexcept {
  for (std::size_t index = 0U;
       index < kCreativeEditorObjectActionDescriptors.size(); ++index) {
    const CreativeEditorObjectActionDescriptor& descriptor =
        kCreativeEditorObjectActionDescriptors[index];
    if (static_cast<std::size_t>(descriptor.command) !=
            static_cast<std::size_t>(kFirstCreativeEditorObjectActionCommand) +
                index ||
        descriptor.activation ==
            CreativeEditorObjectActionActivationKind::Count ||
        descriptor.label.empty()) {
      return false;
    }
    if (descriptor.semanticAction !=
            iggy3d::creative::CreativeSemanticObjectAction::Count &&
        iggy3d::creative::creativeSemanticObjectActionDescriptor(
            descriptor.semanticAction) == nullptr) {
      return false;
    }
  }
  return true;
}

static_assert(creativeEditorObjectActionDescriptorsValid());

[[nodiscard]] constexpr const CreativeEditorObjectActionDescriptor*
creativeEditorObjectActionDescriptor(
    CreativeEditorToolOptionsCommandId command) noexcept {
  const std::size_t commandIndex = static_cast<std::size_t>(command);
  const std::size_t firstIndex =
      static_cast<std::size_t>(kFirstCreativeEditorObjectActionCommand);
  return commandIndex >= firstIndex &&
                 commandIndex - firstIndex <
                     kCreativeEditorObjectActionDescriptors.size()
             ? &kCreativeEditorObjectActionDescriptors[commandIndex -
                                                        firstIndex]
             : nullptr;
}

// Resolves the current Facade selection once for execution hints and every UI
// surface. Selection identity, hierarchy locks, ownership, and source freshness
// are carried together so callers cannot reconstruct only part of admission.
[[nodiscard]] iggy3d::creative::CreativeSemanticObjectActionFacts
resolveCreativeEditorObjectActionFacts(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] const CreativeDesktopObjectActionContext&
refreshCreativeEditorDesktopObjectActionContext(
    CreativeEditorDesktopUiState& desktopUi,
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorObjectActionCapability
creativeEditorObjectActionCapability(
    const CreativeEditorObjectActionCapabilities& capabilities,
    iggy3d::creative::CreativeSemanticObjectAction action) noexcept;
[[nodiscard]] bool creativeEditorObjectActionAvailable(
    const CreativeEditorObjectActionCapabilities& capabilities,
    iggy3d::creative::CreativeSemanticObjectAction action) noexcept;

// Equips a saved authored-asset definition into the active hotbar slot
// (extracted from the SaveSelectionAsAsset object action so the desktop command
// dispatcher can equip too). Returns whether the hotbar accepted the asset. Does
// NOT set a status label — the caller owns messaging.
[[nodiscard]] bool equipCreativeEditorAuthoredAssetToHotbar(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAuthoredAssetDefinition& definition);

[[nodiscard]] bool creativeEditorCommandIsObjectAction(
    CreativeEditorToolOptionsCommandId command) noexcept;

void refreshCreativeEditorObjectActionContext(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorAuthoredAssetLibrary& authoredAssets,
    CreativeEditorToolOptionsState& state,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr) noexcept;

[[nodiscard]] bool creativeEditorObjectActionEnabled(
    const CreativeEditorState& editor,
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) noexcept;

[[nodiscard]] std::string creativeEditorObjectActionValueLabel(
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command);

[[nodiscard]] bool activateCreativeEditorObjectAction(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorToolOptionsCommandId command,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr,
    const CreativePlacementClearanceCache* placementClearanceCache = nullptr);

}  // namespace iggy3d_creative_app
