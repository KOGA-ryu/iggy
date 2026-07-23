#pragma once

#include "app/iggy3d/creative/tools/MeasurementTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

using CreativeMeasurementAnnotationId = std::uint64_t;

inline constexpr CreativeMeasurementAnnotationId
    kInvalidCreativeMeasurementAnnotationId = 0U;
inline constexpr std::uint32_t kCreativeMeasurementAnnotationStoreVersion = 1U;
inline constexpr std::size_t kCreativeMeasurementAnnotationCapacity = 256U;
inline constexpr std::size_t kCreativeMeasurementAnnotationPointCapacity = 256U;
inline constexpr std::size_t kCreativeMeasurementAnnotationNameCapacity = 96U;

struct CreativeMeasurementAnnotationPoint {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  CreativeMeasurementSnapKind snapKind = CreativeMeasurementSnapKind::None;

  [[nodiscard]] friend bool operator==(
      const CreativeMeasurementAnnotationPoint&,
      const CreativeMeasurementAnnotationPoint&) noexcept = default;
};

struct CreativeMeasurementAnnotation {
  CreativeMeasurementAnnotationId id =
      kInvalidCreativeMeasurementAnnotationId;
  std::string name;
  CreativeMeasurementMode mode = CreativeMeasurementMode::Distance;
  CreativeMeasurementAxis axis = CreativeMeasurementAxis::X;
  bool closePath = false;
  std::array<CreativeMeasurementAnnotationPoint,
             kCreativeMeasurementAnnotationPointCapacity>
      points{};
  std::size_t pointCount = 0U;

  [[nodiscard]] friend bool operator==(
      const CreativeMeasurementAnnotation&,
      const CreativeMeasurementAnnotation&) noexcept = default;
};

struct CreativeMeasurementAnnotationStore {
  std::uint32_t version = kCreativeMeasurementAnnotationStoreVersion;
  CreativeMeasurementAnnotationId nextAnnotationId = 1U;
  std::vector<CreativeMeasurementAnnotation> annotations;
};

enum class CreativeMeasurementAnnotationBuildStatus : std::uint8_t {
  NotRequested,
  IncompleteMeasurement,
  InvalidMeasurement,
  InvalidName,
  Ready,
};

struct CreativeMeasurementAnnotationBuildResult {
  bool requested = false;
  bool accepted = false;
  CreativeMeasurementAnnotationBuildStatus status =
      CreativeMeasurementAnnotationBuildStatus::NotRequested;
  CreativeMeasurementAnnotation annotation;
  std::string_view reasonCode =
      "creative_measurement_annotation_not_requested";
};

enum class CreativeMeasurementAnnotationMutationKind : std::uint8_t {
  Add,
  Remove,
  Count,
};

struct CreativeMeasurementAnnotationMutationRequest {
  CreativeMeasurementAnnotationMutationKind kind =
      CreativeMeasurementAnnotationMutationKind::Add;
  CreativeMeasurementAnnotationId annotationId =
      kInvalidCreativeMeasurementAnnotationId;
  CreativeMeasurementAnnotation annotation;
};

enum class CreativeMeasurementAnnotationMutationStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidStore,
  InvalidRequest,
  CapacityExceeded,
  IdExhausted,
  NotFound,
  Applied,
};

struct CreativeMeasurementAnnotationMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeMeasurementAnnotationMutationKind kind =
      CreativeMeasurementAnnotationMutationKind::Add;
  CreativeMeasurementAnnotationMutationStatus status =
      CreativeMeasurementAnnotationMutationStatus::NotRequested;
  CreativeMeasurementAnnotationId annotationId =
      kInvalidCreativeMeasurementAnnotationId;
  std::uint64_t annotationCountBefore = 0U;
  std::uint64_t annotationCountAfter = 0U;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::string_view reasonCode =
      "creative_measurement_annotation_not_requested";
};

struct CreativeMeasurementState;
struct CreativeMeasurementGeometry;

[[nodiscard]] CreativeMeasurementAnnotationBuildResult
buildCreativeMeasurementAnnotation(const CreativeMeasurementState& state,
                                   std::string_view name);
[[nodiscard]] bool validateCreativeMeasurementAnnotation(
    const CreativeMeasurementAnnotation& annotation) noexcept;
[[nodiscard]] bool validateCreativeMeasurementAnnotationStore(
    const CreativeMeasurementAnnotationStore& store) noexcept;
[[nodiscard]] const CreativeMeasurementAnnotation*
findCreativeMeasurementAnnotation(
    const CreativeMeasurementAnnotationStore& store,
    CreativeMeasurementAnnotationId annotationId) noexcept;
[[nodiscard]] CreativeMeasurementGeometry buildCreativeMeasurementGeometry(
    const CreativeMeasurementAnnotation& annotation) noexcept;
[[nodiscard]] CreativeMeasurementAnnotationMutationReceipt
applyCreativeMeasurementAnnotationMutation(
    CreativeMeasurementAnnotationStore& store,
    const CreativeMeasurementAnnotationMutationRequest& request);

}  // namespace iggy3d::creative
