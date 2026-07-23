#include "EditorDesktopWidgets.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <string>

#include "EditorTransform.hpp"

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

[[nodiscard]] bool editsWorldLayoutBuilding(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.ownershipRoute ==
         CreativeEditorTransformOwnershipRoute::WorldLayoutBuilding;
}

constexpr std::array kTranslationAxes{
    cr::CreativeSelectionPlacementAxis::Free,
    cr::CreativeSelectionPlacementAxis::X,
    cr::CreativeSelectionPlacementAxis::Y,
    cr::CreativeSelectionPlacementAxis::Z};

constexpr std::array kRotationAxes{cr::CreativeAxis3::X, cr::CreativeAxis3::Y,
                                   cr::CreativeAxis3::Z};

constexpr std::array kTransformPivots{
    CreativeEditorTransformPivot::SelectionAnchor,
    CreativeEditorTransformPivot::ActiveObjectOrigin,
    CreativeEditorTransformPivot::IndividualOrigins};

[[nodiscard]] const char* transformPivotLabel(
    CreativeEditorTransformPivot pivot) noexcept {
  switch (pivot) {
    case CreativeEditorTransformPivot::SelectionAnchor:
      return "Selection anchor";
    case CreativeEditorTransformPivot::ActiveObjectOrigin:
      return "Active object origin";
    case CreativeEditorTransformPivot::IndividualOrigins:
      return "Individual origins";
    case CreativeEditorTransformPivot::Count:
      break;
  }
  return "Invalid";
}

void drawPlacementMode(const cr::CreativeAppState& appState,
                       CreativeEditorSelectionTransformState& state) {
  const bool moving = state.mode == cr::CreativeSelectionPlacementMode::Move;
  ImGui::BeginDisabled(moving || !state.moveAvailable);
  if (ImGui::Button("Move##active_transform")) {
    static_cast<void>(setCreativeEditorTransformPlacementMode(
        appState, state, cr::CreativeSelectionPlacementMode::Move));
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!moving);
  if (ImGui::Button("Duplicate##active_transform")) {
    static_cast<void>(setCreativeEditorTransformPlacementMode(
        appState, state, cr::CreativeSelectionPlacementMode::Copy));
  }
  ImGui::EndDisabled();
}

void drawPivot(const cr::CreativeAppState& appState,
               CreativeEditorSelectionTransformState& state) {
  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::BeginCombo("Pivot", transformPivotLabel(state.pivot))) {
    for (const CreativeEditorTransformPivot pivot : kTransformPivots) {
      const bool selected = state.pivot == pivot;
      if (ImGui::Selectable(transformPivotLabel(pivot), selected) &&
          !selected) {
        static_cast<void>(
            setCreativeEditorTransformPivot(appState, state, pivot));
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}

void drawCoordinateSpace(const cr::CreativeAppState& appState,
                         CreativeEditorSelectionTransformState& state) {
  ImGui::TextUnformatted("Axes");
  ImGui::SameLine();
  const bool world =
      state.request.coordinateSpace ==
      cr::CreativeSelectionPlacementCoordinateSpace::World;
  if (ImGui::RadioButton("World##active_transform", world) && !world) {
    static_cast<void>(setCreativeEditorTransformCoordinateSpace(
        appState, state,
        cr::CreativeSelectionPlacementCoordinateSpace::World));
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Local##active_transform", !world) && world) {
    static_cast<void>(setCreativeEditorTransformCoordinateSpace(
        appState, state,
        cr::CreativeSelectionPlacementCoordinateSpace::Local));
  }
}

void drawTranslation(const cr::CreativeAppState& appState,
                     CreativeEditorSelectionTransformState& state) {
  std::array target{state.request.targetAnchor.x, state.request.targetAnchor.y,
                    state.request.targetAnchor.z};
  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::InputScalarN("Target", ImGuiDataType_Double, target.data(),
                          static_cast<int>(target.size()))) {
    static_cast<void>(setCreativeEditorTransformTargetAnchor(
        appState, state, {target[0], target[1], target[2]}));
  }

  ImGui::SetNextItemWidth(132.0F);
  if (ImGui::BeginCombo("Move axis", cr::toString(state.constraint).data())) {
    for (const cr::CreativeSelectionPlacementAxis axis : kTranslationAxes) {
      if (editsWorldLayoutBuilding(state) &&
          axis == cr::CreativeSelectionPlacementAxis::Y) {
        continue;
      }
      const bool selected = state.constraint == axis;
      if (ImGui::Selectable(cr::toString(axis).data(), selected) && !selected) {
        static_cast<void>(
            setCreativeEditorTransformConstraint(appState, state, axis));
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(
      state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FollowAim);
  if (ImGui::Button("Use aim target")) {
    static_cast<void>(resumeCreativeEditorTransformAim(appState, state));
  }
  ImGui::EndDisabled();
}

void drawRotation(const cr::CreativeAppState& appState,
                  CreativeEditorSelectionTransformState& state) {
  ImGui::SetNextItemWidth(132.0F);
  if (ImGui::BeginCombo("Rotation axis",
                        cr::toString(state.rotationAxis).data())) {
    for (const cr::CreativeAxis3 axis : kRotationAxes) {
      if (editsWorldLayoutBuilding(state) && axis != cr::CreativeAxis3::Y) {
        continue;
      }
      const bool selected = state.rotationAxis == axis;
      if (ImGui::Selectable(cr::toString(axis).data(), selected) && !selected) {
        static_cast<void>(setCreativeEditorTransformRotationDegrees(
            appState, state, axis, state.rotationDegrees));
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  double degrees = state.rotationDegrees;
  ImGui::SetNextItemWidth(132.0F);
  const bool quarterTurnsOnly =
      state.preflight.capabilities.rotation ==
      cr::CreativeObjectRotationSupport::QuarterTurns;
  const double step = quarterTurnsOnly ? 90.0 : 1.0;
  const double fastStep = quarterTurnsOnly ? 90.0 : 15.0;
  if (ImGui::InputDouble("Rotation", &degrees, step, fastStep, "%.3f deg")) {
    if (quarterTurnsOnly) {
      degrees = std::round(degrees / 90.0) * 90.0;
    }
    static_cast<void>(setCreativeEditorTransformRotationDegrees(
        appState, state, state.rotationAxis, degrees));
  }
}

void drawScale(const cr::CreativeAppState& appState,
               CreativeEditorSelectionTransformState& state) {
  const cr::CreativeVec3 current =
      creativeEditorTransformScaleFactor(state);
  std::array scale{current.x, current.y, current.z};
  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::InputScalarN("Scale", ImGuiDataType_Double, scale.data(),
                          static_cast<int>(scale.size()))) {
    static_cast<void>(setCreativeEditorTransformScaleFactor(
        appState, state, {scale[0], scale[1], scale[2]}));
  }
}

}  // namespace

void appendCreativeDesktopActiveTransformInspector(
    CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    bool playModeActive) {
  CreativeEditorSelectionTransformState& state = editor.transform;
  if (!state.active) {
    return;
  }

  ImGui::SeparatorText("Active transform");
  ImGui::TextDisabled("%s source  |  %zu object%s",
                      editsWorldLayoutBuilding(state)
                          ? "Building"
                          : state.source ==
                                    CreativeEditorTransformSource::Selection
                                ? "Selection"
                                : "Clipboard",
                      state.sourceClipboard.objects.size(),
                      state.sourceClipboard.objects.size() == 1U ? "" : "s");

  ImGui::BeginDisabled(playModeActive);
  drawPlacementMode(appState, state);
  ImGui::BeginDisabled(editsWorldLayoutBuilding(state));
  drawPivot(appState, state);
  drawCoordinateSpace(appState, state);
  ImGui::EndDisabled();
  ImGui::BeginDisabled(!state.preflight.capabilities.translate);
  drawTranslation(appState, state);
  ImGui::EndDisabled();
  ImGui::BeginDisabled(
      state.preflight.capabilities.rotation ==
      cr::CreativeObjectRotationSupport::None);
  drawRotation(appState, state);
  ImGui::EndDisabled();
  ImGui::BeginDisabled(
      state.preflight.capabilities.scale ==
      cr::CreativeObjectScaleSupport::None);
  drawScale(appState, state);
  ImGui::EndDisabled();

  if (!state.targetPositionable) {
    ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
                       "Target unavailable");
  } else if (!state.plan.accepted) {
    if (state.clearance.evaluated && !state.clearance.allowed &&
        state.clearance.blockingObjectId != cr::kInvalidObjectId) {
      ImGui::TextColored(
          ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%s  |  object #%llu",
          cr::toString(state.clearance.status).data(),
          static_cast<unsigned long long>(
              state.clearance.blockingObjectId));
    } else if (state.clearance.evaluated && !state.clearance.allowed) {
      ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%s",
                         cr::toString(state.clearance.status).data());
    } else {
      ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%s",
                         state.plan.reasonCode.c_str());
    }
  } else {
    ImGui::TextColored(ImVec4{0.38F, 0.92F, 0.58F, 1.0F},
                       "Preview ready  |  %zu object%s", state.plan.objects.size(),
                       state.plan.objects.size() == 1U ? "" : "s");
  }

  if (ImGui::Button("Reset##active_transform")) {
    static_cast<void>(applyCreativeEditorTransformControl(
        appState, state, CreativeEditorTransformControl::Reset));
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(!state.targetPositionable || !state.plan.accepted ||
                       state.commitRequested);
  if (ImGui::Button("Apply##active_transform")) {
    static_cast<void>(requestCreativeEditorSelectionTransformCommit(state));
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Cancel##active_transform")) {
    static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
        state, "desktop_transform_cancel"));
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
