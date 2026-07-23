#pragma once

#include "app/iggy3d/creative/recipes/TerrainComposition.hpp"
#include "app/iggy3d/creative/recipes/TerrainGradeRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainLandform.hpp"
#include "app/iggy3d/creative/recipes/TerrainPathSource.hpp"
#include "app/iggy3d/creative/tools/TerrainProfile.hpp"
#include "app/iggy3d/creative/recipes/TerrainRegionRecipe.hpp"
#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

using CreativeTerrainOperationId = std::uint64_t;

inline constexpr CreativeTerrainOperationId kInvalidCreativeTerrainOperationId =
    0U;
inline constexpr std::uint32_t kCreativeTerrainOperationStackVersion = 10U;
inline constexpr std::size_t kCreativeTerrainOperationCapacity = 64U;
inline constexpr std::size_t kCreativeTerrainOperationSourceKeyCapacity =
    512U;

enum class CreativeTerrainOperationKind : std::uint8_t {
  GeneratedTerrain,
  Region,
  Grade,
  Profile,
  Path,
  Stamp,
  Landform,
  Count,
};

enum class CreativeTerrainOperationOwner : std::uint8_t {
  Manual,
  WorldLayout,
  Count,
};

struct CreativeTerrainOperation {
  CreativeTerrainOperationId id = kInvalidCreativeTerrainOperationId;
  bool enabled = true;
  CreativeTerrainOperationOwner owner =
      CreativeTerrainOperationOwner::Manual;
  std::string sourceKey;
  CreativeTerrainOperationKind kind =
      CreativeTerrainOperationKind::GeneratedTerrain;
  CreativeTerrainGeneratorRecipe generation{};
  CreativeTerrainCompositionRecipe composition{};
  CreativeTerrainRegionRecipe region{};
  CreativeTerrainGradeRecipe grade{};
  CreativeTerrainProfileRecipe profile{};
  CreativeTerrainPathSourceRecipe path{};
  CreativeTerrainStampRecipe stamp{};
  CreativeTerrainLandformRecipe landform{};

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainOperation&,
      const CreativeTerrainOperation&) noexcept = default;
};

struct CreativeTerrainOperationStack {
  std::uint32_t version = kCreativeTerrainOperationStackVersion;
  CreativeTerrainOperationId nextOperationId = 1U;
  CreativeTerrainHeightField baseHeightField;
  CreativeTerrainMaterialField baseMaterialField;
  std::vector<CreativeTerrainHardEdge> baseHardEdges;
  std::vector<CreativeTerrainOperation> operations;
};

// Optional transient acceleration for one path operation during replay. The
// durable stack remains the only source of truth; this cache is never saved.
struct CreativeTerrainOperationReplayCache {
  CreativeTerrainOperationId pathOperationId =
      kInvalidCreativeTerrainOperationId;
  CreativeTerrainPathSourceCache* pathSource = nullptr;
};

enum class CreativeTerrainOperationReplayStatus : std::uint8_t {
  NotRequested,
  InvalidTerrain,
  InvalidStack,
  GenerationRejected,
  CompositionRejected,
  RegionRejected,
  GradeRejected,
  ProfileRejected,
  PathRejected,
  StampRejected,
  LandformRejected,
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
  std::uint64_t regionOperationCount = 0U;
  std::uint64_t gradeOperationCount = 0U;
  std::uint64_t profileOperationCount = 0U;
  std::uint64_t pathOperationCount = 0U;
  std::uint64_t stampOperationCount = 0U;
  std::uint64_t landformOperationCount = 0U;
  std::uint64_t pathSegmentCount = 0U;
  std::uint64_t pathCenterlineCellCount = 0U;
  std::uint64_t pathGeneratedControlCount = 0U;
  std::uint64_t pathRebuiltSegmentCount = 0U;
  std::uint64_t pathReusedSegmentCount = 0U;
  std::uint64_t materialEditCount = 0U;
  std::uint64_t outputMaterialOverrideCount = 0U;
  std::uint64_t outputHardEdgeCount = 0U;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t evaluatedOctaveCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t materialModifiedCellCount = 0U;
  std::uint64_t protectedCellCount = 0U;
  std::uint64_t featheredCellCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::uint64_t materialHash = 0U;
  std::uint64_t hardEdgeHash = 0U;
  CreativeTerrainGradeReadout lastGradeReadout{};
  std::size_t failedOperationIndex = 0U;
  CreativeTerrainOperationId failedOperationId =
      kInvalidCreativeTerrainOperationId;
  std::string_view reasonCode =
      "creative_terrain_operation_replay_not_requested";
};

struct CreativeTerrainOperationReplayResult {
  CreativeTerrainHeightField heightField;
  CreativeTerrainMaterialField materialField;
  std::vector<CreativeTerrainHardEdge> hardEdges;
  CreativeTerrainOperationReplayReceipt receipt;
};

enum class CreativeTerrainOperationMutationKind : std::uint8_t {
  Add,
  Update,
  SetEnabled,
  Move,
  Remove,
  BakeAll,
  Count,
};

struct CreativeTerrainOperationMutationRequest {
  CreativeTerrainOperationMutationKind kind =
      CreativeTerrainOperationMutationKind::Add;
  CreativeTerrainOperationId operationId =
      kInvalidCreativeTerrainOperationId;
  CreativeTerrainOperationOwner owner =
      CreativeTerrainOperationOwner::Manual;
  std::string sourceKey;
  CreativeTerrainOperationKind operationKind =
      CreativeTerrainOperationKind::GeneratedTerrain;
  CreativeTerrainGeneratorRecipe generation{};
  CreativeTerrainCompositionRecipe composition{};
  CreativeTerrainRegionRecipe region{};
  CreativeTerrainGradeRecipe grade{};
  CreativeTerrainProfileRecipe profile{};
  CreativeTerrainPathSourceRecipe path{};
  CreativeTerrainStampRecipe stamp{};
  CreativeTerrainLandformRecipe landform{};
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
  CreativeTerrainMaterialField materialField;
  std::vector<CreativeTerrainHardEdge> hardEdges;
  CreativeTerrainOperationMutationReceipt receipt;
};

[[nodiscard]] bool creativeTerrainHeightFieldsEqual(
    const CreativeTerrainHeightField& lhs,
    const CreativeTerrainHeightField& rhs) noexcept;
[[nodiscard]] std::uint64_t hashCreativeTerrainHeightField(
    const CreativeTerrainHeightField& field) noexcept;
[[nodiscard]] bool creativeTerrainMaterialFieldsEqual(
    const CreativeTerrainMaterialField& lhs,
    const CreativeTerrainMaterialField& rhs) noexcept;
[[nodiscard]] std::uint64_t hashCreativeTerrainMaterialField(
    const CreativeTerrainMaterialField& field) noexcept;
[[nodiscard]] bool creativeTerrainHardEdgesEqual(
    std::span<const CreativeTerrainHardEdge> lhs,
    std::span<const CreativeTerrainHardEdge> rhs) noexcept;
[[nodiscard]] std::uint64_t hashCreativeTerrainHardEdges(
    std::span<const CreativeTerrainHardEdge> edges) noexcept;
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
    CreativeTerrainOperationKind kind) noexcept;
[[nodiscard]] bool parseCreativeTerrainOperationKind(
    std::string_view text,
    CreativeTerrainOperationKind& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainOperationOwner owner) noexcept;
[[nodiscard]] bool parseCreativeTerrainOperationOwner(
    std::string_view text,
    CreativeTerrainOperationOwner& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainOperationMutationStatus status) noexcept;

// Replays enabled operations in vector order from the immutable baked base.
// Each operation consumes the exact field produced by its predecessor. Work is
// bounded by 64 operations and 8192 cells per operation; failure is atomic.
[[nodiscard]] CreativeTerrainOperationReplayResult
replayCreativeTerrainOperations(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationReplayCache* cache = nullptr);

// Plans one stack mutation without touching document truth. The current
// derived field must equal a replay of the input stack, preventing edits from
// building on stale procedural state. Add captures currentDerived as the base
// when the stack is empty; removing the final operation restores that base.
// BakeAll preserves the exact current height/material result while clearing
// every procedural operation; it does not pretend to detach one layer while
// silently rebasing later layers.
[[nodiscard]] CreativeTerrainOperationMutationPlan
planCreativeTerrainOperationMutation(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainHeightField& currentDerived,
    const CreativeTerrainMaterialField& currentDerivedMaterial,
    const CreativeTerrainOperationStack& currentStack,
    const CreativeTerrainOperationMutationRequest& request,
    CreativeTerrainPathSourceCache* pathCache = nullptr,
    const std::vector<CreativeTerrainHardEdge>* currentDerivedHardEdges =
        nullptr);

}  // namespace iggy3d::creative
