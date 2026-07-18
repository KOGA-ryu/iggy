#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeRecipeSchemaVersion = 1U;

enum class CreativeRecipeKind : std::uint8_t {
  Unknown,
  Building,
  ObjectLibrary,
};

enum class CreativeRecipeObjectRole : std::uint8_t {
  Unknown,
  Source,
  Generated,
};

enum class CreativeRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidRecipe,
  InvalidSchema,
  InvalidObjectPlan,
  DuplicateStableKey,
  InvalidParentReference,
  ObjectIdOverflow,
  InvalidDocument,
  Ready,
  ApplyRejected,
  Applied,
};

// Recipe plans refer to newly-created parents by object index rather than by a
// guessed document id. Materialization resolves those references against the
// live document allocator immediately before the atomic create transaction.
struct CreativeRecipeObjectPlan {
  CreativeDocumentCreateRequest createRequest;
  CreativeRecipeObjectRole role = CreativeRecipeObjectRole::Generated;
  std::string stableKey;
  std::optional<std::size_t> parentObjectIndex = std::nullopt;
};

struct CreativeRecipePlan {
  CreativeRecipeKind kind = CreativeRecipeKind::Unknown;
  std::uint32_t schemaVersion = kCreativeRecipeSchemaVersion;
  std::string instanceKey;
  std::string instanceName;
  // Zero keeps ordinary one-shot recipes unversioned. Regenerating owners set
  // this from fingerprintCreativeRecipePlan before materialization. Each
  // materialized object also receives its own generated-output fingerprint so
  // later reconciliation can distinguish source changes from 3D refinements.
  std::uint64_t definitionFingerprint = 0U;
  std::vector<CreativeRecipeObjectPlan> objects;
};

struct CreativeRecipeMaterializeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeRecipeStatus status = CreativeRecipeStatus::NotRequested;
  CreativeRecipeKind kind = CreativeRecipeKind::Unknown;
  std::uint32_t schemaVersion = 0U;
  CreativeObjectId firstObjectId = kInvalidObjectId;
  std::uint64_t objectCount = 0U;
  std::uint64_t sourceObjectCount = 0U;
  std::uint64_t generatedObjectCount = 0U;
  std::uint64_t resolvedParentCount = 0U;
  std::size_t failedObjectIndex = 0U;
  std::string reasonCode = "creative_recipe_not_requested";
};

struct CreativeRecipeMaterializeResult {
  std::vector<CreativeDocumentCreateRequest> createRequests;
  CreativeRecipeMaterializeReceipt receipt;
};

struct CreativeRecipeApplyReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRecipeStatus status = CreativeRecipeStatus::NotRequested;
  CreativeRecipeMaterializeReceipt materializeReceipt;
  CreativeFacadeDocumentBatchCreateReceipt createReceipt;
  CreativeHistoryRecordReceipt historyReceipt;
  std::string reasonCode = "creative_recipe_apply_not_requested";
};

[[nodiscard]] std::string_view toString(CreativeRecipeKind kind) noexcept;
[[nodiscard]] std::string_view toString(CreativeRecipeObjectRole role) noexcept;
[[nodiscard]] std::string_view toString(CreativeRecipeStatus status) noexcept;

[[nodiscard]] std::string creativeRecipeKindTag(CreativeRecipeKind kind);
[[nodiscard]] std::string creativeRecipeRoleTag(
    CreativeRecipeObjectRole role);
[[nodiscard]] std::string creativeRecipeInstanceKeyTag(
    std::string_view instanceKey);
[[nodiscard]] std::string creativeRecipeStableKeyTag(
    std::string_view stableKey);
[[nodiscard]] std::string creativeRecipeDefinitionFingerprintTag(
    std::uint64_t fingerprint);
[[nodiscard]] std::string creativeRecipeOutputFingerprintTag(
    std::uint64_t fingerprint);
[[nodiscard]] std::uint64_t fingerprintCreativeRecipePlan(
    const CreativeRecipePlan& plan) noexcept;
[[nodiscard]] std::uint64_t fingerprintCreativeRecipeObjectPlan(
    const CreativeRecipePlan& plan,
    std::size_t objectIndex) noexcept;
[[nodiscard]] std::uint64_t fingerprintCreativeRecipeObjectState(
    const CreativeObject& object,
    std::string_view parentStableKey) noexcept;
[[nodiscard]] bool isCreativeRecipeManagementTag(
    std::string_view tag) noexcept;
[[nodiscard]] bool creativeRecipeRequestHasProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeRecipeKind kind,
    CreativeRecipeObjectRole role,
    std::string_view stableKey);
[[nodiscard]] bool creativeRecipeRequestHasInstanceProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeRecipeKind kind,
    std::string_view instanceKey,
    CreativeRecipeObjectRole role,
    std::string_view stableKey);
[[nodiscard]] bool creativeRecipeRequestHasDefinitionFingerprint(
    const CreativeDocumentCreateRequest& request,
    std::uint64_t fingerprint);
[[nodiscard]] bool creativeRecipeObjectHasProvenance(
    const CreativeObject& object,
    CreativeRecipeKind kind,
    CreativeRecipeObjectRole role,
    std::string_view stableKey);
[[nodiscard]] bool creativeRecipeObjectHasInstanceProvenance(
    const CreativeObject& object,
    CreativeRecipeKind kind,
    std::string_view instanceKey,
    CreativeRecipeObjectRole role,
    std::string_view stableKey);
[[nodiscard]] std::string_view creativeRecipeObjectInstanceKey(
    const CreativeObject& object) noexcept;
[[nodiscard]] std::string_view creativeRecipeObjectStableKey(
    const CreativeObject& object) noexcept;
[[nodiscard]] std::uint64_t creativeRecipeObjectOutputFingerprint(
    const CreativeObject& object) noexcept;
[[nodiscard]] bool creativeRecipeObjectHasDefinitionFingerprint(
    const CreativeObject& object,
    std::uint64_t fingerprint);

[[nodiscard]] CreativeRecipeMaterializeResult materializeCreativeRecipe(
    const CreativeRecipePlan& plan,
    CreativeObjectId firstObjectId);

// Applies a materialized recipe as one atomic document replacement. Callers
// that expose undo should use applyCreativeRecipeWithHistory below.
[[nodiscard]] CreativeRecipeApplyReceipt applyCreativeRecipe(
    Facade& facade,
    const CreativeRecipePlan& plan);

// The shared authoring entry point for both 3D tools and future 2D symbols.
// A successful recipe application records exactly one document snapshot.
[[nodiscard]] CreativeRecipeApplyReceipt applyCreativeRecipeWithHistory(
    CreativeAppState& appState,
    const CreativeRecipePlan& plan,
    std::string_view source);

}  // namespace iggy3d::creative
