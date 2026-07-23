#pragma once

#include <array>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "imgui.h"

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopModel.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorState.hpp"
#include "EditorUiInput.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

// Internal cluster header for the desktop panel widget owners. One header for
// the pair, not one per TU: EditorDesktopPanels.cpp keeps the menu, read-only
// toolbar, bottom diagnostics, status bar, and panel orchestration, and
// delegates the two panel bodies to EditorDesktopOutliner.cpp and
// EditorDesktopInspector.cpp. Widgets emit typed commands only; the dispatcher
// remains the sole mutator.

namespace iggy3d_creative_app {

// The persistent selection as the facade currently reports it (ids + primary),
// before it is resolved against document truth.
struct CreativeDesktopLiveSelection {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  iggy3d::creative::CreativeObjectId primaryObjectId =
      iggy3d::creative::kInvalidObjectId;
};

[[nodiscard]] CreativeDesktopLiveSelection creativeDesktopLiveSelection(
    const iggy3d::creative::CreativeAppState& appState);

// Project-panel body. Renders the searchable hierarchy and converts pure
// selection plans into SelectObjects commands. Called inside the caller's
// Begin/End for the `Project` dock window.
void buildCreativeEditorDesktopOutlinerPanel(
    CreativeEditorDesktopUiState& desktopUi,
    const iggy3d::creative::CreativeAppState& appState,
    bool playModeActive,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands);

// Inspector-panel body: zero/single/multi general object properties, then the
// existing logic-link authoring, diagnostics, and Play-mode runtime monitor.
// Called inside the caller's Begin/End for the `Inspector` dock window.
void buildCreativeEditorDesktopInspectorPanel(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands);

// Exact controls for the live transform preview. These mutate only the
// transient editor session; the existing transform commit remains the sole
// document/history boundary.
void appendCreativeDesktopActiveTransformInspector(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    bool playModeActive);

// Transient named volume-region controls. Region edits do not mutate the
// document; operation commit remains owned by the held-item dispatcher.
void appendCreativeDesktopVolumeInspector(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& document,
    std::span<const iggy3d::creative::CreativeObjectId> selectedObjectIds);

// Terrain generator body for the Inspector's dedicated tab. Widgets edit only
// transient recipe state and emit semantic workflow commands; the dispatcher
// owns preview/apply/cancel behavior and document history.
void buildCreativeEditorDesktopTerrainGenerationPanel(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopGeneratedSourceSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    CreativeDesktopGeneratedSourceScopeCache& scopeCache,
    const iggy3d::creative::CreativeDocument& document,
    iggy3d::creative::CreativeObjectId objectId,
    iggy3d::creative::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands);
void appendCreativeDesktopGeneratedBuildingSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    iggy3d::creative::CreativeObjectId objectId,
    std::size_t buildingIndex,
    bool disabled,
    CreativeDesktopCommandFrame& commands);
void appendCreativeDesktopGeneratedRoomSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    iggy3d::creative::CreativeObjectId objectId,
    iggy3d::creative::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands);
void appendCreativeDesktopGeneratedLevelSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    iggy3d::creative::CreativeObjectId objectId,
    iggy3d::creative::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands);

// Shared canonical-wall editor used by both the 2D source Properties panel and
// the generated-object Inspector. It emits the generic source-property
// preview/commit commands; callers remain presentation-only.
void appendCreativeDesktopTopologyEdgeSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    std::size_t topologyEdgeIndex,
    CreativeDesktopCommandFrame& commands);

template <typename Payload>
void queueCreativeDesktopGeneratedPropertyEdit(
    CreativeDesktopPropertyEditIntent intent,
    CreativeDesktopCommandId previewCommand,
    CreativeDesktopCommandId commitCommand,
    Payload payload,
    CreativeDesktopCommandFrame& commands) {
  switch (intent) {
    case CreativeDesktopPropertyEditIntent::Preview:
      commands.push(previewCommand, std::move(payload));
      return;
    case CreativeDesktopPropertyEditIntent::Commit:
      commands.push(commitCommand, std::move(payload));
      return;
    case CreativeDesktopPropertyEditIntent::Cancel:
      commands.push(
          CreativeDesktopCommandId::WorldLayoutCancelGeneratedSettingsPreview);
      return;
    case CreativeDesktopPropertyEditIntent::None:
    default:
      return;
  }
}

template <typename Settings>
void queueCreativeDesktopWorldLayoutPropertyEdit(
    CreativeDesktopPropertyEditIntent intent,
    std::size_t index,
    std::string_view stableKey,
    const Settings& settings,
    CreativeDesktopCommandFrame& commands) {
  CreativeDesktopWorldLayoutPropertyEditPhase phase{};
  switch (intent) {
    case CreativeDesktopPropertyEditIntent::Preview:
      phase = CreativeDesktopWorldLayoutPropertyEditPhase::Preview;
      break;
    case CreativeDesktopPropertyEditIntent::Commit:
      phase = CreativeDesktopWorldLayoutPropertyEditPhase::Commit;
      break;
    case CreativeDesktopPropertyEditIntent::Cancel:
      phase = CreativeDesktopWorldLayoutPropertyEditPhase::Cancel;
      break;
    case CreativeDesktopPropertyEditIntent::None:
    default:
      return;
  }
  constexpr iggy3d::creative::CreativeWorldLayoutTable table =
      creativeDesktopWorldLayoutPropertyTable<Settings>();
  commands.push(
      CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
      CreativeDesktopWorldLayoutPropertyEditPayload{
          phase, table, index, std::string(stableKey), settings});
}

inline void observeCreativeDesktopContinuousPropertyWidget(
    CreativeDesktopPropertyEditActivity& activity,
    bool changed) noexcept {
  observeCreativeDesktopContinuousPropertyEdit(
      activity, changed, ImGui::IsItemDeactivatedAfterEdit());
}

inline void observeCreativeDesktopDiscretePropertyWidget(
    CreativeDesktopPropertyEditActivity& activity,
    bool changed) noexcept {
  observeCreativeDesktopDiscretePropertyEdit(activity, changed);
}

template <typename RoofSettings>
void drawCreativeStructuralRoofSettingsWidgets(
    RoofSettings& settings,
    CreativeDesktopPropertyEditActivity& activity,
    const char* id) {
  namespace cr = iggy3d::creative;
  ImGui::PushID(id);

  constexpr std::array kStyles{
      cr::CreativeStructuralRoofStyle::Flat,
      cr::CreativeStructuralRoofStyle::Shed,
      cr::CreativeStructuralRoofStyle::Gable,
      cr::CreativeStructuralRoofStyle::Hip};
  bool styleChanged = false;
  ImGui::SetNextItemWidth(148.0F);
  if (ImGui::BeginCombo("Style", cr::toString(settings.roofStyle).data())) {
    for (const cr::CreativeStructuralRoofStyle style : kStyles) {
      const bool selected = settings.roofStyle == style;
      if (ImGui::Selectable(cr::toString(style).data(), selected)) {
        settings.roofStyle = style;
        styleChanged = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  observeCreativeDesktopDiscretePropertyEdit(activity, styleChanged);

  ImGui::SetNextItemWidth(112.0F);
  const bool overhangChanged = ImGui::InputDouble(
      "Overhang", &settings.roofOverhangCells, 0.25, 1.0, "%.2f");
  observeCreativeDesktopContinuousPropertyEdit(
      activity, overhangChanged, ImGui::IsItemDeactivatedAfterEdit());

  if (settings.roofStyle == cr::CreativeStructuralRoofStyle::Shed) {
    constexpr std::array kDirections{
        cr::CreativeStructuralRoofSlopeDirection::PositiveX,
        cr::CreativeStructuralRoofSlopeDirection::NegativeX,
        cr::CreativeStructuralRoofSlopeDirection::PositiveZ,
        cr::CreativeStructuralRoofSlopeDirection::NegativeZ};
    bool directionChanged = false;
    ImGui::SetNextItemWidth(148.0F);
    if (ImGui::BeginCombo(
            "Direction",
            cr::toString(settings.roofSlopeDirection).data())) {
      for (const cr::CreativeStructuralRoofSlopeDirection direction :
           kDirections) {
        const bool selected = settings.roofSlopeDirection == direction;
        if (ImGui::Selectable(cr::toString(direction).data(), selected)) {
          settings.roofSlopeDirection = direction;
          directionChanged = true;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    observeCreativeDesktopDiscretePropertyEdit(activity, directionChanged);
  }

  if (settings.roofStyle == cr::CreativeStructuralRoofStyle::Gable ||
      settings.roofStyle == cr::CreativeStructuralRoofStyle::Hip) {
    constexpr std::array kAxes{cr::CreativeStructuralRoofRidgeAxis::X,
                               cr::CreativeStructuralRoofRidgeAxis::Z};
    bool axisChanged = false;
    ImGui::SetNextItemWidth(148.0F);
    if (ImGui::BeginCombo(
            "Ridge", cr::toString(settings.roofRidgeAxis).data())) {
      for (const cr::CreativeStructuralRoofRidgeAxis axis : kAxes) {
        const bool selected = settings.roofRidgeAxis == axis;
        if (ImGui::Selectable(cr::toString(axis).data(), selected)) {
          settings.roofRidgeAxis = axis;
          axisChanged = true;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    observeCreativeDesktopDiscretePropertyEdit(activity, axisChanged);
  }

  if (settings.roofStyle != cr::CreativeStructuralRoofStyle::Flat) {
    ImGui::SetNextItemWidth(112.0F);
    const bool pitchChanged = ImGui::InputDouble(
        "Pitch", &settings.roofPitchDegrees, 1.0, 5.0, "%.1f deg");
    observeCreativeDesktopContinuousPropertyEdit(
        activity, pitchChanged, ImGui::IsItemDeactivatedAfterEdit());
  }

  constexpr std::array kMaterials{
      cr::CreativeStructuralMaterial::Blockout,
      cr::CreativeStructuralMaterial::Plaster,
      cr::CreativeStructuralMaterial::Timber,
      cr::CreativeStructuralMaterial::Stone,
      cr::CreativeStructuralMaterial::Brick};
  bool materialChanged = false;
  ImGui::SetNextItemWidth(148.0F);
  if (ImGui::BeginCombo("Material", cr::toString(settings.roofMaterial).data())) {
    for (const cr::CreativeStructuralMaterial material : kMaterials) {
      const bool selected = settings.roofMaterial == material;
      if (ImGui::Selectable(cr::toString(material).data(), selected)) {
        settings.roofMaterial = material;
        materialChanged = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  observeCreativeDesktopDiscretePropertyEdit(activity, materialChanged);
  ImGui::PopID();
}

struct CreativeDoorSettingsWidgetActivity {
  bool discreteChanged = false;
  bool continuousChanged = false;
  bool continuousDeactivated = false;
};

[[nodiscard]] inline CreativeDoorSettingsWidgetActivity
drawCreativeDoorSettingsWidgets(
    iggy3d::creative::CreativeDoorSettings& settings,
    const char* id) {
  namespace cr = iggy3d::creative;
  CreativeDoorSettingsWidgetActivity activity;
  ImGui::PushID(id);

  constexpr const char* kArrangementLabels[] = {"Single leaf", "Double leaf"};
  int arrangement = static_cast<int>(settings.leafArrangement);
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::Combo("Leaves", &arrangement, kArrangementLabels,
                   static_cast<int>(std::size(kArrangementLabels)))) {
    settings.leafArrangement =
        static_cast<cr::CreativeDoorLeafArrangement>(arrangement);
    activity.discreteChanged = true;
  }

  constexpr const char* kHingeLabels[] = {"Minimum edge", "Maximum edge"};
  int hinge = static_cast<int>(settings.hingeSide);
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::Combo("Primary hinge", &hinge, kHingeLabels,
                   static_cast<int>(std::size(kHingeLabels)))) {
    settings.hingeSide = static_cast<cr::CreativeDoorHingeSide>(hinge);
    activity.discreteChanged = true;
  }

  constexpr const char* kSwingLabels[] = {"Side A (- normal)",
                                           "Side B (+ normal)"};
  int swing = static_cast<int>(settings.swingSide);
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::Combo("Swing side", &swing, kSwingLabels,
                   static_cast<int>(std::size(kSwingLabels)))) {
    settings.swingSide = static_cast<cr::CreativeDoorSwingSide>(swing);
    activity.discreteChanged = true;
  }

  constexpr const char* kInitialStateLabels[] = {"Closed", "Open"};
  int initialState = static_cast<int>(settings.initialState);
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::Combo("Initial state", &initialState, kInitialStateLabels,
                   static_cast<int>(std::size(kInitialStateLabels)))) {
    settings.initialState =
        static_cast<cr::CreativeDoorInitialState>(initialState);
    activity.discreteChanged = true;
  }

  activity.discreteChanged |=
      ImGui::Checkbox("Gameplay locked", &settings.gameplayLocked);
  ImGui::SetNextItemWidth(128.0F);
  activity.continuousChanged = ImGui::InputDouble(
      "Transition seconds", &settings.transitionSeconds, 0.05, 0.25, "%.2f");
  activity.continuousDeactivated = ImGui::IsItemDeactivatedAfterEdit();
  ImGui::PopID();
  return activity;
}

[[nodiscard]] inline bool creativeDesktopWorldLayoutPropertyPreviewActive(
    const CreativeEditorWorldLayoutState& state,
    iggy3d::creative::CreativeWorldLayoutTable table,
    std::size_t index) noexcept {
  return state.liveEditPreviewVisible && state.propertyPreviewKey.active &&
         state.propertyPreviewKey.sourceRevision == state.revision &&
         state.propertyPreviewKey.table == table &&
         state.propertyPreviewKey.index == index;
}

template <typename Settings>
void finishCreativeDesktopWorldLayoutPropertyEdit(
    CreativeDesktopPropertyEditActivity activity,
    const Settings& current,
    Settings& draft,
    bool valid,
    const char* resetLabel,
    CreativeEditorWorldLayoutState& state,
    std::size_t index,
    std::string_view stableKey,
    CreativeDesktopCommandFrame& commands) {
  constexpr iggy3d::creative::CreativeWorldLayoutTable table =
      creativeDesktopWorldLayoutPropertyTable<Settings>();
  bool dirty = !(current == draft);
  ImGui::BeginDisabled(!dirty);
  const bool resetRequested = ImGui::Button(resetLabel);
  ImGui::EndDisabled();
  if (resetRequested) {
    draft = current;
    dirty = false;
  }
  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          activity, dirty, valid,
          creativeDesktopWorldLayoutPropertyPreviewActive(state, table,
                                                          index),
          resetRequested);
  queueCreativeDesktopWorldLayoutPropertyEdit(
      intent, index, stableKey, draft, commands);
}

// Non-truncating adapter shared by inspector owners; imgui_stdlib is not
// vendored in this checkout.
bool creativeDesktopInputTextStdString(const char* label, std::string* value,
                                       ImGuiInputTextFlags flags = 0);

// Shared by the Inspector's logic rows and the bottom Diagnostics tab, so the
// helper is not duplicated across the two TUs. Defined in
// EditorDesktopInspector.cpp.
void queueCreativeDesktopObjectNavigation(
    CreativeDesktopCommandFrame& commands,
    iggy3d::creative::CreativeObjectId objectId,
    bool playModeActive);
[[nodiscard]] ImVec4 creativeDesktopLogicDiagnosticColor(
    iggy3d::creative::CreativeLogicDiagnosticSeverity severity) noexcept;

}  // namespace iggy3d_creative_app
