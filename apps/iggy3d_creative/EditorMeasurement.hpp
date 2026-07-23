#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "EditorInteraction.hpp"
#include "app/iggy3d/creative/tools/Measure.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

namespace iggy3d_creative_app {

enum class CreativeEditorMeasurementPointStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidTarget,
  MissingObject,
  InvalidObjectBounds,
  MissingOpening,
  InvalidOpeningGeometry,
  MissingLevel,
  Ready,
};

struct CreativeEditorMeasurementPointRequest {
  const iggy3d::creative::CreativeDocument* document = nullptr;
  const CreativeEditorWorldTarget* target = nullptr;
  const iggy3d::creative::CreativeWorldLayout* worldLayout = nullptr;
  std::size_t activeLevelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeMeasurementSnapMode snapMode =
      iggy3d::creative::CreativeMeasurementSnapMode::Auto;
  double snapStepMeters = 1.0;
  double vertexToleranceMeters = 0.25;
};

struct CreativeEditorMeasurementPointResult {
  bool requested = false;
  bool accepted = false;
  CreativeEditorMeasurementPointStatus status =
      CreativeEditorMeasurementPointStatus::NotRequested;
  iggy3d::creative::CreativeMeasurementSnapMode requestedMode =
      iggy3d::creative::CreativeMeasurementSnapMode::Auto;
  iggy3d::creative::CreativeMeasurementSnapMode appliedMode =
      iggy3d::creative::CreativeMeasurementSnapMode::Auto;
  iggy3d::creative::CreativeMeasurementPoint point;
  iggy3d::creative::CreativeWorldLayoutSourceRef worldLayoutSource;
  double sourceDistanceMeters = 0.0;
  std::string_view reasonCode =
      "creative_editor_measurement_point_not_requested";
};

struct CreativeEditorMeasurementActionReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorMeasurementPointResult point;
  iggy3d::creative::CreativeMeasurementReceipt measurement;
  std::string_view reasonCode =
      "creative_editor_measurement_action_not_requested";
};

// Resolves one aimed world target into a measurement endpoint. Explicit
// semantic modes fail closed; Auto uses opening > nearby vertex > surface,
// with document-grid fallback only for empty-space aiming.
[[nodiscard]] CreativeEditorMeasurementPointResult
resolveCreativeEditorMeasurementPoint(
    const CreativeEditorMeasurementPointRequest& request) noexcept;

// Frame overload owns all editor-to-kernel policy: current snap increment,
// generated-source freshness, active level, and vertex attraction tolerance.
[[nodiscard]] CreativeEditorMeasurementPointResult
resolveCreativeEditorMeasurementPoint(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept;

[[nodiscard]] CreativeEditorMeasurementActionReceipt
appendCreativeEditorMeasurementPoint(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept;
[[nodiscard]] CreativeEditorMeasurementActionReceipt
previewCreativeEditorMeasurementPoint(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept;
[[nodiscard]] CreativeEditorMeasurementActionReceipt
completeCreativeEditorMeasurement(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept;
[[nodiscard]] CreativeEditorMeasurementActionReceipt
cancelCreativeEditorMeasurement(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept;

[[nodiscard]] std::string formatCreativeEditorMeasurementReadout(
    const iggy3d::creative::CreativeMeasurementState& state);

// Converts canonical world-space measurement geometry to document-grid cell
// coordinates once for both Plan and Elevation renderers.
[[nodiscard]] iggy3d::creative::CreativeMeasurementGeometry
projectCreativeEditorMeasurementGeometryToGrid(
    const iggy3d::creative::CreativeMeasurementGeometry& worldGeometry,
    const iggy3d::creative::CreativeGridSettings& grid) noexcept;

}  // namespace iggy3d_creative_app
