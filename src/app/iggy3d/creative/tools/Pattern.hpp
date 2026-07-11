#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/tools/Clipboard.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeLinearArrayInstanceCapacity = 32;
inline constexpr std::uint64_t
    kCreativeLinearArrayGeneratedObjectCapacity = 512;

enum class CreativeLinearArrayDirection : std::uint8_t {
  PositiveX,
  NegativeX,
  PositiveY,
  NegativeY,
  PositiveZ,
  NegativeZ,
  Count,
};

enum class CreativeLinearArrayCopyCount : std::uint8_t {
  One,
  Two,
  Four,
  Eight,
  Sixteen,
  ThirtyTwo,
  Count,
};

enum class CreativeLinearArraySpacing : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeLinearArrayStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  InvalidRequest,
  OperationLimitExceeded,
  MissingObject,
  ObjectIdExhausted,
  PasteRejected,
  Planned,
  Applied,
};

struct CreativeLinearArrayInstance {
  std::uint32_t ordinal = 0;
  CreativeVec3 offset{};
};

struct CreativeLinearArrayPlanRequest {
  std::uint64_t sourceObjectCount = 0;
  CreativeLinearArrayDirection direction =
      CreativeLinearArrayDirection::PositiveX;
  CreativeLinearArrayCopyCount copyCount =
      CreativeLinearArrayCopyCount::Four;
  CreativeLinearArraySpacing spacing =
      CreativeLinearArraySpacing::OneCell;
  double cellSize = 1.0;
  std::uint64_t maxGeneratedObjects =
      kCreativeLinearArrayGeneratedObjectCapacity;
};

struct CreativeLinearArrayPlanReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeLinearArrayStatus status =
      CreativeLinearArrayStatus::NotRequested;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t copyCount = 0;
  std::uint64_t generatedObjectCount = 0;
  std::array<CreativeLinearArrayInstance,
             kCreativeLinearArrayInstanceCapacity>
      instances{};
  std::size_t instanceCount = 0;
  std::string_view reasonCode = "creative_linear_array_not_requested";

  [[nodiscard]] std::span<const CreativeLinearArrayInstance>
  plannedInstances() const noexcept {
    return {instances.data(), instanceCount};
  }
};

struct CreativeLinearArrayRequest {
  CreativeLinearArrayDirection direction =
      CreativeLinearArrayDirection::PositiveX;
  CreativeLinearArrayCopyCount copyCount =
      CreativeLinearArrayCopyCount::Four;
  CreativeLinearArraySpacing spacing =
      CreativeLinearArraySpacing::OneCell;
  double cellSize = 1.0;
  std::uint64_t maxGeneratedObjects =
      kCreativeLinearArrayGeneratedObjectCapacity;
};

struct CreativeLinearArrayReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeLinearArrayStatus status =
      CreativeLinearArrayStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t generatedObjectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::size_t finalCopyFirstObjectIndex = 0;
  std::size_t finalCopyObjectCount = 0;
  CreativeLinearArrayPlanReceipt plan;
  CreativeClipboardCopyReceipt copyReceipt;
  CreativeClipboardBatchPasteReceipt pasteReceipt;
  std::string message = "creative_linear_array_not_requested";

  [[nodiscard]] std::span<const CreativeObjectId>
  generatedObjectIds() const noexcept {
    return pasteReceipt.pastedObjectIds;
  }

  [[nodiscard]] std::span<const CreativeObjectId>
  finalCopyObjectIds() const noexcept {
    if (finalCopyFirstObjectIndex > pasteReceipt.pastedObjectIds.size() ||
        finalCopyObjectCount >
            pasteReceipt.pastedObjectIds.size() - finalCopyFirstObjectIndex) {
      return {};
    }
    return {pasteReceipt.pastedObjectIds.data() + finalCopyFirstObjectIndex,
            finalCopyObjectCount};
  }
};

[[nodiscard]] std::string_view toString(
    CreativeLinearArrayDirection direction) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLinearArrayCopyCount count) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLinearArraySpacing spacing) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLinearArrayStatus status) noexcept;

[[nodiscard]] std::uint32_t creativeLinearArrayCopyCountValue(
    CreativeLinearArrayCopyCount count) noexcept;
[[nodiscard]] std::uint32_t creativeLinearArraySpacingCells(
    CreativeLinearArraySpacing spacing) noexcept;

// O(k), allocation-free, where k is the requested copy count (maximum 32).
// Every offset is derived from its ordinal, so floating-point error does not
// accumulate from one instance to the next.
[[nodiscard]] CreativeLinearArrayPlanReceipt planCreativeLinearArray(
    const CreativeLinearArrayPlanRequest& request) noexcept;

// O(n log n + n * k), plus one target-document staging copy, where n is the
// unique selected-object count and k is copy count. The document is published
// only after every generated copy succeeds.
[[nodiscard]] CreativeLinearArrayReceipt createCreativeLinearArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeLinearArrayRequest& request);

}  // namespace iggy3d::creative
