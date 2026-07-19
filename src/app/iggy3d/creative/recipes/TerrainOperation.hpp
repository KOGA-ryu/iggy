#pragma once

#include "app/iggy3d/creative/recipes/TerrainComposition.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

using CreativeTerrainOperationId = std::uint64_t;

inline constexpr CreativeTerrainOperationId kInvalidCreativeTerrainOperationId =
    0U;
inline constexpr std::uint32_t kCreativeTerrainOperationStackVersion = 1U;
inline constexpr std::size_t kCreativeTerrainOperationCapacity = 64U;

struct CreativeTerrainOperation {
  CreativeTerrainOperationId id = kInvalidCreativeTerrainOperationId;
  bool enabled = true;
  CreativeTerrainGeneratorRecipe generation{};
  CreativeTerrainCompositionRecipe composition{};

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainOperation&,
      const CreativeTerrainOperation&) noexcept = default;
};

struct CreativeTerrainOperationStack {
  std::uint32_t version = kCreativeTerrainOperationStackVersion;
  CreativeTerrainOperationId nextOperationId = 1U;
  CreativeTerrainHeightField baseHeightField;
  std::vector<CreativeTerrainOperation> operations;
};

enum class CreativeTerrainOperationReplayStatus : std::uint8_t {
  NotRequested,
  InvalidTerrain,
  InvalidStack,
  GenerationRejected,
  CompositionRejected,
  Ready,
};

struct CreativeTerrainOperationReplayReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainOperationReplayStatus status =
      CreativeTerrainOperationReplayStatus::NotRequested;
  std::uint64_t operationCount = 0U;
  std::uint64_t enabledOperationCount = 0U;
  std::uint64_t disabledOperationCount = 0U;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t featheredCellCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::size_t failedOperationIndex = 0U;
  CreativeTerrainOperationId failedOperationId =
      kInvalidCreativeTerrainOperationId;
  std::string_view reasonCode =
      "creative_terrain_operation_replay_not_requested";
};

struct CreativeTerrainOperationReplayResult {
  CreativeTerrainHeightField heightField;
  CreativeTerrainOperationReplayReceipt receipt;
};

enum class CreativeTerrainOperationMutationKind : std::uint8_t {
  Add,
  Update,
  SetEnabled,
  Move,
  Remove,
  Count,
};

struct CreativeTerrainOperationMutationRequest {
  CreativeTerrainOperationMutationKind kind =
      CreativeTerrainOperationMutationKind::Add;
  CreativeTerrainOperationId operationId =
      kInvalidCreativeTerrainOperationId;
  CreativeTerrainGeneratorRecipe generation{};
  CreativeTerrainCompositionRecipe composition{};
  bool enabled = true;
  std::size_t targetIndex = 0U;
};

enum class CreativeTerrainOperationMutationStatus : std::uint8_t {
  NotRequested,
  InvalidTerrain,
  InvalidStack,
  InvalidRequest,
  CapacityExceeded,
  IdExhausted,
  NotFound,
  ReplayRejected,
  NoChange,
  Applied,
};

struct CreativeTerrainOperationMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainOperationMutationStatus status =
      CreativeTerrainOperationMutationStatus::NotRequested;
  CreativeTerrainOperationMutationKind kind =
      CreativeTerrainOperationMutationKind::Add;
  CreativeTerrainOperationId operationId =
      kInvalidCreativeTerrainOperationId;
  std::size_t operationIndexBefore = 0U;
  std::size_t operationIndexAfter = 0U;
  std::uint64_t operationCountBefore = 0U;
  std::uint64_t operationCountAfter = 0U;
  CreativeTerrainOperationReplayReceipt replay{};
  std::string_view reasonCode =
      "creative_terrain_operation_mutation_not_requested";
};

struct CreativeTerrainOperationMutationPlan {
  CreativeTerrainOperationStack stack;
  CreativeTerrainHeightField heightField;
  CreativeTerrainOperationMutationReceipt receipt;
};

[[nodiscard]] bool creativeTerrainHeightFieldsEqual(
    const CreativeTerrainHeightField& lhs,
    const CreativeTerrainHeightField& rhs) noexcept;
[[nodiscard]] bool validateCreativeTerrainOperationStack(
    const CreativeTerrainOperationStack& stack) noexcept;
[[nodiscard]] const CreativeTerrainOperation* findCreativeTerrainOperation(
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationId operationId) noexcept;
[[nodiscard]] std::size_t findCreativeTerrainOperationIndex(
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationId operationId) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainOperationReplayStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainOperationMutationStatus status) noexcept;

// Replays enabled operations in vector order from the immutable baked base.
// Each operation consumes the exact field produced by its predecessor. Work is
// bounded by 64 operations and 8192 cells per operation; failure is atomic.
[[nodiscard]] CreativeTerrainOperationReplayResult
replayCreativeTerrainOperations(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainOperationStack& stack);

// Plans one stack mutation without touching document truth. The current
// derived field must equal a replay of the input stack, preventing edits from
// building on stale procedural state. Add captures currentDerived as the base
// when the stack is empty; removing the final operation restores that base.
[[nodiscard]] CreativeTerrainOperationMutationPlan
planCreativeTerrainOperationMutation(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainHeightField& currentDerived,
    const CreativeTerrainOperationStack& currentStack,
    const CreativeTerrainOperationMutationRequest& request);

}  // namespace iggy3d::creative
