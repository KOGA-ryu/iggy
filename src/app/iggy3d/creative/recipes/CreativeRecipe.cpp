#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kRecipeSchemaTag = "creative_recipe_schema:1";

void setStatus(CreativeRecipeMaterializeReceipt& receipt,
               CreativeRecipeStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
}

void setStatus(CreativeRecipeApplyReceipt& receipt,
               CreativeRecipeStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
}

[[nodiscard]] bool validStableKey(std::string_view key) noexcept {
  if (key.empty() || key.size() > 128U) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    const unsigned char character = static_cast<unsigned char>(value);
    return std::isalnum(character) != 0 || value == '_' || value == '-' ||
           value == '.';
  });
}

[[nodiscard]] bool hasTag(std::span<const std::string> tags,
                          std::string_view expected) noexcept {
  return std::any_of(tags.begin(), tags.end(), [expected](const std::string& tag) {
    return tag == expected;
  });
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!hasTag(tags, tag)) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] bool validObjectRole(CreativeRecipeObjectRole role) noexcept {
  return role == CreativeRecipeObjectRole::Source ||
         role == CreativeRecipeObjectRole::Generated;
}

[[nodiscard]] bool validRecipeKind(CreativeRecipeKind kind) noexcept {
  return kind == CreativeRecipeKind::Building ||
         kind == CreativeRecipeKind::ObjectLibrary;
}

}  // namespace

std::string_view toString(CreativeRecipeKind kind) noexcept {
  switch (kind) {
    case CreativeRecipeKind::Unknown:
      return "Unknown";
    case CreativeRecipeKind::Building:
      return "Building";
    case CreativeRecipeKind::ObjectLibrary:
      return "ObjectLibrary";
  }
  return "Unknown";
}

std::string_view toString(CreativeRecipeObjectRole role) noexcept {
  switch (role) {
    case CreativeRecipeObjectRole::Unknown:
      return "Unknown";
    case CreativeRecipeObjectRole::Source:
      return "Source";
    case CreativeRecipeObjectRole::Generated:
      return "Generated";
  }
  return "Unknown";
}

std::string_view toString(CreativeRecipeStatus status) noexcept {
  switch (status) {
    case CreativeRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeRecipeStatus::InvalidRecipe:
      return "InvalidRecipe";
    case CreativeRecipeStatus::InvalidSchema:
      return "InvalidSchema";
    case CreativeRecipeStatus::InvalidObjectPlan:
      return "InvalidObjectPlan";
    case CreativeRecipeStatus::DuplicateStableKey:
      return "DuplicateStableKey";
    case CreativeRecipeStatus::InvalidParentReference:
      return "InvalidParentReference";
    case CreativeRecipeStatus::ObjectIdOverflow:
      return "ObjectIdOverflow";
    case CreativeRecipeStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeRecipeStatus::Ready:
      return "Ready";
    case CreativeRecipeStatus::ApplyRejected:
      return "ApplyRejected";
    case CreativeRecipeStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

std::string creativeRecipeKindTag(CreativeRecipeKind kind) {
  switch (kind) {
    case CreativeRecipeKind::Building:
      return "creative_recipe:building";
    case CreativeRecipeKind::ObjectLibrary:
      return "creative_recipe:object_library";
    case CreativeRecipeKind::Unknown:
      return "creative_recipe:unknown";
  }
  return "creative_recipe:unknown";
}

std::string creativeRecipeRoleTag(CreativeRecipeObjectRole role) {
  switch (role) {
    case CreativeRecipeObjectRole::Source:
      return "creative_recipe_role:source";
    case CreativeRecipeObjectRole::Generated:
      return "creative_recipe_role:generated";
    case CreativeRecipeObjectRole::Unknown:
      return "creative_recipe_role:unknown";
  }
  return "creative_recipe_role:unknown";
}

std::string creativeRecipeInstanceKeyTag(std::string_view instanceKey) {
  return "creative_recipe_instance:" + std::string(instanceKey);
}

std::string creativeRecipeStableKeyTag(std::string_view stableKey) {
  return "creative_recipe_key:" + std::string(stableKey);
}

bool creativeRecipeRequestHasProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeRecipeKind kind,
    CreativeRecipeObjectRole role,
    std::string_view stableKey) {
  return hasTag(request.tags, creativeRecipeKindTag(kind)) &&
         hasTag(request.tags, creativeRecipeRoleTag(role)) &&
         hasTag(request.tags, creativeRecipeStableKeyTag(stableKey)) &&
         hasTag(request.tags, kRecipeSchemaTag);
}

bool creativeRecipeRequestHasInstanceProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeRecipeKind kind,
    std::string_view instanceKey,
    CreativeRecipeObjectRole role,
    std::string_view stableKey) {
  return creativeRecipeRequestHasProvenance(request, kind, role, stableKey) &&
         hasTag(request.tags, creativeRecipeInstanceKeyTag(instanceKey));
}

bool creativeRecipeObjectHasProvenance(const CreativeObject& object,
                                       CreativeRecipeKind kind,
                                       CreativeRecipeObjectRole role,
                                       std::string_view stableKey) {
  return hasTag(object.tags, creativeRecipeKindTag(kind)) &&
         hasTag(object.tags, creativeRecipeRoleTag(role)) &&
         hasTag(object.tags, creativeRecipeStableKeyTag(stableKey)) &&
         hasTag(object.tags, kRecipeSchemaTag);
}

bool creativeRecipeObjectHasInstanceProvenance(
    const CreativeObject& object,
    CreativeRecipeKind kind,
    std::string_view instanceKey,
    CreativeRecipeObjectRole role,
    std::string_view stableKey) {
  return creativeRecipeObjectHasProvenance(object, kind, role, stableKey) &&
         hasTag(object.tags, creativeRecipeInstanceKeyTag(instanceKey));
}

CreativeRecipeMaterializeResult materializeCreativeRecipe(
    const CreativeRecipePlan& plan,
    CreativeObjectId firstObjectId) {
  CreativeRecipeMaterializeResult result;
  result.receipt.requested = true;
  result.receipt.kind = plan.kind;
  result.receipt.schemaVersion = plan.schemaVersion;
  result.receipt.firstObjectId = firstObjectId;
  result.receipt.objectCount = plan.objects.size();

  if (!validRecipeKind(plan.kind) || !validStableKey(plan.instanceKey) ||
      plan.objects.empty()) {
    setStatus(result.receipt, CreativeRecipeStatus::InvalidRecipe,
              "creative_recipe_invalid");
    return result;
  }
  if (plan.schemaVersion != kCreativeRecipeSchemaVersion) {
    setStatus(result.receipt, CreativeRecipeStatus::InvalidSchema,
              "creative_recipe_schema_unsupported");
    return result;
  }
  if (firstObjectId == kInvalidObjectId ||
      plan.objects.size() >
          std::numeric_limits<CreativeObjectId>::max() - firstObjectId) {
    setStatus(result.receipt, CreativeRecipeStatus::ObjectIdOverflow,
              "creative_recipe_object_id_overflow");
    return result;
  }

  std::unordered_set<std::string> stableKeys;
  stableKeys.reserve(plan.objects.size());
  result.createRequests.reserve(plan.objects.size());
  for (std::size_t index = 0; index < plan.objects.size(); ++index) {
    const CreativeRecipeObjectPlan& object = plan.objects[index];
    result.receipt.failedObjectIndex = index;
    if (object.createRequest.kind == CreativeObjectKind::Unknown ||
        !validObjectRole(object.role) || !validStableKey(object.stableKey)) {
      setStatus(result.receipt, CreativeRecipeStatus::InvalidObjectPlan,
                "creative_recipe_object_plan_invalid");
      result.createRequests.clear();
      return result;
    }
    if (!stableKeys.insert(object.stableKey).second) {
      setStatus(result.receipt, CreativeRecipeStatus::DuplicateStableKey,
                "creative_recipe_stable_key_duplicate");
      result.createRequests.clear();
      return result;
    }
    if (object.parentObjectIndex.has_value() &&
        (object.createRequest.parentId.has_value() ||
         *object.parentObjectIndex >= index)) {
      setStatus(result.receipt, CreativeRecipeStatus::InvalidParentReference,
                "creative_recipe_parent_reference_invalid");
      result.createRequests.clear();
      return result;
    }

    CreativeDocumentCreateRequest create = object.createRequest;
    if (object.parentObjectIndex.has_value()) {
      create.parentId = firstObjectId + *object.parentObjectIndex;
      ++result.receipt.resolvedParentCount;
    }
    appendTagOnce(create.tags, creativeRecipeKindTag(plan.kind));
    appendTagOnce(create.tags, creativeRecipeInstanceKeyTag(plan.instanceKey));
    appendTagOnce(create.tags, creativeRecipeRoleTag(object.role));
    appendTagOnce(create.tags, creativeRecipeStableKeyTag(object.stableKey));
    appendTagOnce(create.tags, std::string(kRecipeSchemaTag));
    result.createRequests.push_back(std::move(create));

    if (object.role == CreativeRecipeObjectRole::Source) {
      ++result.receipt.sourceObjectCount;
    } else {
      ++result.receipt.generatedObjectCount;
    }
  }

  result.receipt.accepted = true;
  result.receipt.failedObjectIndex = 0U;
  setStatus(result.receipt, CreativeRecipeStatus::Ready,
            "creative_recipe_ready");
  return result;
}

CreativeRecipeApplyReceipt applyCreativeRecipe(Facade& facade,
                                               const CreativeRecipePlan& plan) {
  CreativeRecipeApplyReceipt receipt;
  receipt.requested = true;
  const CreativeDocument& document = facade.document();
  if (!document.isValid() || document.id() == kInvalidDocumentId ||
      document.nextObjectId() == kInvalidObjectId) {
    setStatus(receipt, CreativeRecipeStatus::InvalidDocument,
              "creative_recipe_document_invalid");
    return receipt;
  }

  CreativeRecipeMaterializeResult materialized =
      materializeCreativeRecipe(plan, document.nextObjectId());
  receipt.materializeReceipt = materialized.receipt;
  if (!materialized.receipt.accepted) {
    setStatus(receipt, materialized.receipt.status,
              materialized.receipt.reasonCode);
    return receipt;
  }

  receipt.createReceipt =
      facade.createDocumentObjectsAtomically(materialized.createRequests);
  if (!receipt.createReceipt.accepted || !receipt.createReceipt.changed) {
    setStatus(receipt, CreativeRecipeStatus::ApplyRejected,
              receipt.createReceipt.reasonCode);
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  setStatus(receipt, CreativeRecipeStatus::Applied,
            "creative_recipe_applied");
  return receipt;
}

CreativeRecipeApplyReceipt applyCreativeRecipeWithHistory(
    CreativeAppState& appState,
    const CreativeRecipePlan& plan,
    std::string_view source) {
  CreativeDocumentHistoryTransaction transaction =
      beginCreativeHistoryTransaction(appState.facade, source);
  CreativeRecipeApplyReceipt receipt = applyCreativeRecipe(appState.facade, plan);
  if (!receipt.accepted || !receipt.changed) {
    cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }

  receipt.historyReceipt = commitCreativeHistoryTransaction(
      appState.history, std::move(transaction), appState.facade);
  if (!receipt.historyReceipt.accepted || !receipt.historyReceipt.recorded) {
    // The document mutation remains valid if history is disabled; the receipt
    // exposes that fact so the caller can report it rather than silently claim
    // undo support.
    receipt.reasonCode = std::string(receipt.historyReceipt.reasonCode);
  }
  return receipt;
}

}  // namespace iggy3d::creative
