#include "EditorTransform.hpp"
#include "EditorTransformInternal.hpp"

#include <cmath>
#include <string_view>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d_creative_app {
namespace detail {

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

}  // namespace detail

namespace {

[[nodiscard]] cr::CreativeVec3 add(cr::CreativeVec3 lhs,
                                   cr::CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] cr::CreativeVec3 scale(cr::CreativeVec3 value,
                                     double factor) noexcept {
  return {value.x * factor, value.y * factor, value.z * factor};
}

[[nodiscard]] double dot(cr::CreativeVec3 lhs,
                         cr::CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] bool normalized(cr::CreativeVec3 value,
                              cr::CreativeVec3& output) noexcept {
  const double lengthSquared = dot(value, value);
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-16) {
    return false;
  }
  output = scale(value, 1.0 / std::sqrt(lengthSquared));
  return cr::isFiniteCreativeVec3(output);
}


}  // namespace

CreativeEditorTransformAxisRaySample sampleCreativeEditorTransformAxisRay(
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection) noexcept {
  CreativeEditorTransformAxisRaySample result;
  if (!cr::isFiniteCreativeVec3(rayOrigin) ||
      !cr::isFiniteCreativeVec3(axisOrigin)) {
    return result;
  }
  cr::CreativeVec3 ray{};
  cr::CreativeVec3 axis{};
  if (!normalized(rayDirection, ray) || !normalized(axisDirection, axis)) {
    return result;
  }

  const cr::CreativeVec3 fromAxis = detail::subtract(rayOrigin, axisOrigin);
  const double rayAxisDot = dot(ray, axis);
  const double rayFromAxisDot = dot(ray, fromAxis);
  const double axisFromAxisDot = dot(axis, fromAxis);
  const double denominator = 1.0 - rayAxisDot * rayAxisDot;
  if (!std::isfinite(denominator) || denominator <= 1.0e-8) {
    return result;
  }
  result.rayParameter =
      (rayAxisDot * axisFromAxisDot - rayFromAxisDot) / denominator;
  result.axisParameter =
      (axisFromAxisDot - rayAxisDot * rayFromAxisDot) / denominator;
  result.valid = std::isfinite(result.rayParameter) &&
                 std::isfinite(result.axisParameter) &&
                 result.rayParameter >= 0.0;
  if (!result.valid) {
    result = {};
  }
  return result;
}

bool beginCreativeEditorFreeTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 initialAimAnchor) {
  if (!state.active || !detail::transformTranslationAvailable(state) ||
      !cr::isFiniteCreativeVec3(initialAimAnchor)) {
    return false;
  }
  if (state.constraint != cr::CreativeSelectionPlacementAxis::Free) {
    static_cast<void>(setCreativeEditorTransformConstraint(
        appState, state, cr::CreativeSelectionPlacementAxis::Free));
  }
  state.pointerGesture = {};
  state.pointerGesture.kind = CreativeEditorTransformPointerGestureKind::Free;
  state.pointerGesture.initialAimAnchor = initialAimAnchor;
  return true;
}

bool beginCreativeEditorAxisTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis axis,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) {
  if (!state.active || !detail::transformTranslationAvailable(state) ||
      axis == cr::CreativeSelectionPlacementAxis::Free ||
      axis == cr::CreativeSelectionPlacementAxis::Count) {
    return false;
  }
  cr::CreativeVec3 normalizedAxis{};
  if (!normalized(axisDirection, normalizedAxis)) {
    return false;
  }
  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(
          rayOrigin, rayDirection, axisOrigin, normalizedAxis);
  if (!sample.valid) {
    return false;
  }
  if (state.constraint != axis) {
    static_cast<void>(setCreativeEditorTransformConstraint(appState, state,
                                                           axis));
  }
  if (state.constraint != axis) {
    return false;
  }
  state.pointerGesture = {};
  state.pointerGesture.kind = CreativeEditorTransformPointerGestureKind::Axis;
  state.pointerGesture.axis = axis;
  state.pointerGesture.axisOrigin = axisOrigin;
  state.pointerGesture.axisDirection = normalizedAxis;
  state.pointerGesture.initialAxisParameter = sample.axisParameter;
  return true;
}

bool updateCreativeEditorTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) {
  if (!state.active ||
      state.pointerGesture.kind ==
          CreativeEditorTransformPointerGestureKind::None) {
    return false;
  }

  cr::CreativeVec3 resolvedTarget{};
  if (state.pointerGesture.kind ==
      CreativeEditorTransformPointerGestureKind::Free) {
    if (!targetPositionable || !cr::isFiniteCreativeVec3(targetAnchor)) {
      return false;
    }
    resolvedTarget = add(
        state.request.sourceAnchor,
        detail::subtract(targetAnchor, state.pointerGesture.initialAimAnchor));
  } else {
    const CreativeEditorTransformAxisRaySample sample =
        sampleCreativeEditorTransformAxisRay(
            rayOrigin, rayDirection, state.pointerGesture.axisOrigin,
            state.pointerGesture.axisDirection);
    if (!sample.valid) {
      return false;
    }
    resolvedTarget = add(
        state.request.sourceAnchor,
        scale(state.pointerGesture.axisDirection,
              sample.axisParameter -
                  state.pointerGesture.initialAxisParameter));
  }
  if (!cr::isFiniteCreativeVec3(resolvedTarget)) {
    return false;
  }
  static_cast<void>(
      setCreativeEditorTransformTargetAnchor(appState, state, resolvedTarget));
  state.pointerGesture.changed =
      state.pointerGesture.changed ||
      !cr::creativeVec3ExactlyEqual(state.request.targetAnchor,
                                    state.request.sourceAnchor);
  return true;
}

bool finishCreativeEditorTransformPointerGesture(
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  if (!state.active ||
      state.pointerGesture.kind ==
          CreativeEditorTransformPointerGestureKind::None) {
    return false;
  }
  const bool changed =
      state.pointerGesture.changed && state.targetPositionable &&
      state.plan.accepted &&
      !cr::creativeVec3ExactlyEqual(state.request.targetAnchor,
                                    state.request.sourceAnchor);
  state.pointerGesture = {};
  if (!changed) {
    return cancelCreativeEditorSelectionTransformPreview(state, source);
  }
  return requestCreativeEditorSelectionTransformCommit(state);
}

}  // namespace iggy3d_creative_app
