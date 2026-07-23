#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeAuthoringOperationRecordVersion = 2U;

enum class CreativeAuthoringFamily : std::uint8_t {
  Building,
  ObjectLibrary,
  Road,
  Watercourse,
  Bridge,
  RetainingEdge,
  Terrain,
  Volume,
  Pattern,
  AssetScatter,
  Prefab,
  WorldLayout,
  Count,
};

enum class CreativeAuthoringLifecycle : std::uint8_t {
  Parametric,
  Destructive,
  Observational,
  Count,
};

enum class CreativeAuthoringOperationKind : std::uint8_t {
  Apply,
  Reconcile,
  Destructive,
  Count,
};

enum class CreativeAuthoringSourceStore : std::uint8_t {
  None,
  AuthoredAssetLibrary,
  TerrainOperationStack,
  PatternRecipeStore,
  WorldLayoutSource,
  Count,
};

enum class CreativeAuthoringCapability : std::uint16_t {
  Plan = 1U << 0U,
  Preview = 1U << 1U,
  Apply = 1U << 2U,
  Reconcile = 1U << 3U,
  History = 1U << 4U,
  DurableSource = 1U << 5U,
  DestructiveRecord = 1U << 6U,
  FrontendNeutral = 1U << 7U,
};

using CreativeAuthoringCapabilities = std::uint16_t;

[[nodiscard]] constexpr CreativeAuthoringCapabilities capability(
    CreativeAuthoringCapability value) noexcept {
  return static_cast<CreativeAuthoringCapabilities>(value);
}

[[nodiscard]] constexpr CreativeAuthoringCapabilities operator|(
    CreativeAuthoringCapability lhs,
    CreativeAuthoringCapability rhs) noexcept {
  return capability(lhs) | capability(rhs);
}

[[nodiscard]] constexpr CreativeAuthoringCapabilities operator|(
    CreativeAuthoringCapabilities lhs,
    CreativeAuthoringCapability rhs) noexcept {
  return lhs | capability(rhs);
}

struct CreativeAuthoringContract {
  CreativeAuthoringFamily family = CreativeAuthoringFamily::Count;
  CreativeAuthoringLifecycle lifecycle = CreativeAuthoringLifecycle::Count;
  CreativeAuthoringSourceStore sourceStore =
      CreativeAuthoringSourceStore::Count;
  CreativeAuthoringCapabilities capabilities = 0U;
  std::string_view stableName;
};

struct CreativeAuthoringOperationRecord {
  std::uint32_t version = kCreativeAuthoringOperationRecordVersion;
  CreativeAuthoringFamily family = CreativeAuthoringFamily::Count;
  CreativeAuthoringOperationKind kind = CreativeAuthoringOperationKind::Count;
  CreativeAuthoringLifecycle lifecycle = CreativeAuthoringLifecycle::Count;
  std::string action;
  std::uint64_t requestFingerprint = 0U;
  std::uint64_t affectedMemberCount = 0U;

  friend bool operator==(const CreativeAuthoringOperationRecord&,
                         const CreativeAuthoringOperationRecord&) = default;
};

enum class CreativeAuthoringContractValidationStatus : std::uint8_t {
  Valid,
  WrongCount,
  MisorderedFamily,
  InvalidEnum,
  MissingStableName,
  MissingCoreCapability,
  ParametricContractInvalid,
  DestructiveContractInvalid,
  ObservationalContractInvalid,
};

struct CreativeAuthoringContractValidation {
  CreativeAuthoringContractValidationStatus status =
      CreativeAuthoringContractValidationStatus::Valid;
  CreativeAuthoringFamily family = CreativeAuthoringFamily::Count;

  [[nodiscard]] constexpr bool valid() const noexcept {
    return status == CreativeAuthoringContractValidationStatus::Valid;
  }
};

inline constexpr CreativeAuthoringCapabilities kParametricCapabilities =
    CreativeAuthoringCapability::Plan |
    CreativeAuthoringCapability::Preview |
    CreativeAuthoringCapability::Apply |
    CreativeAuthoringCapability::Reconcile |
    CreativeAuthoringCapability::History |
    CreativeAuthoringCapability::DurableSource |
    CreativeAuthoringCapability::FrontendNeutral;

inline constexpr CreativeAuthoringCapabilities kDestructiveCapabilities =
    CreativeAuthoringCapability::Plan |
    CreativeAuthoringCapability::Preview |
    CreativeAuthoringCapability::Apply |
    CreativeAuthoringCapability::History |
    CreativeAuthoringCapability::DestructiveRecord |
    CreativeAuthoringCapability::FrontendNeutral;

inline constexpr std::array<
    CreativeAuthoringContract,
    static_cast<std::size_t>(CreativeAuthoringFamily::Count)>
    kCreativeAuthoringContracts{{
        {CreativeAuthoringFamily::Building,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "building"},
        {CreativeAuthoringFamily::ObjectLibrary,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "object_library"},
        {CreativeAuthoringFamily::Road,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "road"},
        {CreativeAuthoringFamily::Watercourse,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "watercourse"},
        {CreativeAuthoringFamily::Bridge,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "bridge"},
        {CreativeAuthoringFamily::RetainingEdge,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "retaining_edge"},
        {CreativeAuthoringFamily::Terrain,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::TerrainOperationStack,
         kParametricCapabilities,
         "terrain"},
        {CreativeAuthoringFamily::Volume,
         CreativeAuthoringLifecycle::Destructive,
         CreativeAuthoringSourceStore::None,
         kDestructiveCapabilities,
         "volume"},
        {CreativeAuthoringFamily::Pattern,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::PatternRecipeStore,
         kParametricCapabilities |
             CreativeAuthoringCapability::DestructiveRecord,
         "pattern"},
        {CreativeAuthoringFamily::AssetScatter,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::PatternRecipeStore,
         kParametricCapabilities |
             CreativeAuthoringCapability::DestructiveRecord,
         "asset_scatter"},
        {CreativeAuthoringFamily::Prefab,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::AuthoredAssetLibrary,
         kParametricCapabilities |
             CreativeAuthoringCapability::DestructiveRecord,
         "prefab"},
        {CreativeAuthoringFamily::WorldLayout,
         CreativeAuthoringLifecycle::Parametric,
         CreativeAuthoringSourceStore::WorldLayoutSource,
         kParametricCapabilities,
         "world_layout"},
    }};

[[nodiscard]] constexpr std::span<const CreativeAuthoringContract>
creativeAuthoringContracts() noexcept {
  return kCreativeAuthoringContracts;
}

[[nodiscard]] constexpr bool creativeAuthoringContractHas(
    const CreativeAuthoringContract& contract,
    CreativeAuthoringCapability capabilityValue) noexcept {
  return (contract.capabilities & capability(capabilityValue)) != 0U;
}

[[nodiscard]] constexpr const CreativeAuthoringContract*
findCreativeAuthoringContract(CreativeAuthoringFamily family) noexcept {
  const std::size_t index = static_cast<std::size_t>(family);
  return index < kCreativeAuthoringContracts.size()
             ? &kCreativeAuthoringContracts[index]
             : nullptr;
}

[[nodiscard]] constexpr CreativeAuthoringContractValidation
validateCreativeAuthoringContracts(
    std::span<const CreativeAuthoringContract> contracts) noexcept {
  if (contracts.size() !=
      static_cast<std::size_t>(CreativeAuthoringFamily::Count)) {
    return {CreativeAuthoringContractValidationStatus::WrongCount,
            CreativeAuthoringFamily::Count};
  }
  constexpr CreativeAuthoringCapabilities kSharedCapabilities =
      CreativeAuthoringCapability::Plan |
      CreativeAuthoringCapability::Preview |
      CreativeAuthoringCapability::FrontendNeutral;
  for (std::size_t index = 0U; index < contracts.size(); ++index) {
    const CreativeAuthoringContract& contract = contracts[index];
    if (static_cast<std::size_t>(contract.family) != index) {
      return {CreativeAuthoringContractValidationStatus::MisorderedFamily,
              contract.family};
    }
    if (contract.lifecycle >= CreativeAuthoringLifecycle::Count ||
        contract.sourceStore >= CreativeAuthoringSourceStore::Count) {
      return {CreativeAuthoringContractValidationStatus::InvalidEnum,
              contract.family};
    }
    if (contract.stableName.empty()) {
      return {CreativeAuthoringContractValidationStatus::MissingStableName,
              contract.family};
    }
    if ((contract.capabilities & kSharedCapabilities) !=
        kSharedCapabilities) {
      return {
          CreativeAuthoringContractValidationStatus::MissingCoreCapability,
          contract.family};
    }
    const bool applies = creativeAuthoringContractHas(
        contract, CreativeAuthoringCapability::Apply);
    const bool ownsHistory = creativeAuthoringContractHas(
        contract, CreativeAuthoringCapability::History);
    const bool durable = creativeAuthoringContractHas(
        contract, CreativeAuthoringCapability::DurableSource);
    const bool reconciles = creativeAuthoringContractHas(
        contract, CreativeAuthoringCapability::Reconcile);
    const bool destructiveRecord = creativeAuthoringContractHas(
        contract, CreativeAuthoringCapability::DestructiveRecord);
    if (contract.lifecycle == CreativeAuthoringLifecycle::Parametric &&
        (contract.sourceStore == CreativeAuthoringSourceStore::None ||
         !applies || !ownsHistory || !durable || !reconciles)) {
      return {
          CreativeAuthoringContractValidationStatus::ParametricContractInvalid,
          contract.family};
    }
    if (contract.lifecycle == CreativeAuthoringLifecycle::Destructive &&
        (contract.sourceStore != CreativeAuthoringSourceStore::None ||
         !applies || !ownsHistory || durable || reconciles ||
         !destructiveRecord)) {
      return {
          CreativeAuthoringContractValidationStatus::DestructiveContractInvalid,
          contract.family};
    }
    if (contract.lifecycle == CreativeAuthoringLifecycle::Observational &&
        (contract.sourceStore != CreativeAuthoringSourceStore::None || durable ||
         reconciles || destructiveRecord || applies || ownsHistory)) {
      return {
          CreativeAuthoringContractValidationStatus::ObservationalContractInvalid,
          contract.family};
    }
  }
  return {};
}

[[nodiscard]] inline bool validateCreativeAuthoringOperationRecord(
    const CreativeAuthoringOperationRecord& record) noexcept {
  const CreativeAuthoringContract* contract =
      findCreativeAuthoringContract(record.family);
  if (record.version != kCreativeAuthoringOperationRecordVersion ||
      contract == nullptr || record.kind >= CreativeAuthoringOperationKind::Count ||
      record.action.empty() || record.action.size() > 128U ||
      record.requestFingerprint == 0U ||
      !creativeAuthoringContractHas(
          *contract, CreativeAuthoringCapability::History)) {
    return false;
  }
  switch (record.kind) {
    case CreativeAuthoringOperationKind::Apply:
      return record.lifecycle == contract->lifecycle &&
             creativeAuthoringContractHas(
                 *contract, CreativeAuthoringCapability::Apply);
    case CreativeAuthoringOperationKind::Reconcile:
      return record.lifecycle == CreativeAuthoringLifecycle::Parametric &&
             creativeAuthoringContractHas(
                 *contract, CreativeAuthoringCapability::Reconcile);
    case CreativeAuthoringOperationKind::Destructive:
      return record.lifecycle == CreativeAuthoringLifecycle::Destructive &&
             creativeAuthoringContractHas(
                 *contract, CreativeAuthoringCapability::DestructiveRecord);
    case CreativeAuthoringOperationKind::Count:
      return false;
  }
  return false;
}

[[nodiscard]] inline std::optional<CreativeAuthoringOperationRecord>
makeCreativeAuthoringOperationRecord(
    CreativeAuthoringFamily family,
    CreativeAuthoringOperationKind kind,
    std::string_view action,
    std::uint64_t requestFingerprint,
    std::uint64_t affectedMemberCount) {
  const CreativeAuthoringContract* contract =
      findCreativeAuthoringContract(family);
  if (contract == nullptr) {
    return std::nullopt;
  }
  CreativeAuthoringOperationRecord record;
  record.family = family;
  record.kind = kind;
  record.lifecycle =
      kind == CreativeAuthoringOperationKind::Destructive
          ? CreativeAuthoringLifecycle::Destructive
          : contract->lifecycle;
  record.action = std::string(action);
  record.requestFingerprint = requestFingerprint;
  record.affectedMemberCount = affectedMemberCount;
  return validateCreativeAuthoringOperationRecord(record)
             ? std::optional<CreativeAuthoringOperationRecord>{
                   std::move(record)}
             : std::nullopt;
}

}  // namespace iggy3d::creative
