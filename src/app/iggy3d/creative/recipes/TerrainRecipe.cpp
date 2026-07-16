#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d::creative {
namespace {

void setStatus(CreativeTerrainRecipeReceipt& receipt,
               CreativeTerrainRecipeStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

void setStatus(CreativeTerrainRecipePreviewResult& result,
               CreativeTerrainRecipeStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  result.status = status;
  result.reasonCode = std::string(reasonCode);
  result.accepted = accepted;
}

void setStatus(CreativeTerrainRecipeApplyReceipt& receipt,
               CreativeTerrainRecipeStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

[[nodiscard]] bool validDocument(const CreativeDocument* document) noexcept {
  return document != nullptr && document->isValid() &&
         document->id() != kInvalidDocumentId &&
         document->terrainField().validateInvariants() &&
         document->terrainMaterialField().validateInvariants();
}

[[nodiscard]] bool profileKind(CreativeTerrainRecipeKind kind,
                               CreativeTerrainProfileKind& profile) noexcept {
  switch (kind) {
    case CreativeTerrainRecipeKind::Hill:
      profile = CreativeTerrainProfileKind::Hill;
      return true;
    case CreativeTerrainRecipeKind::Valley:
      profile = CreativeTerrainProfileKind::Basin;
      return true;
    case CreativeTerrainRecipeKind::Crater:
      profile = CreativeTerrainProfileKind::Crater;
      return true;
    case CreativeTerrainRecipeKind::Ridge:
      profile = CreativeTerrainProfileKind::Ridge;
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool pathKind(CreativeTerrainRecipeKind kind,
                            CreativeTerrainPathKind& path) noexcept {
  switch (kind) {
    case CreativeTerrainRecipeKind::Road:
      path = CreativeTerrainPathKind::Road;
      return true;
    case CreativeTerrainRecipeKind::River:
      path = CreativeTerrainPathKind::River;
      return true;
    case CreativeTerrainRecipeKind::Ditch:
      path = CreativeTerrainPathKind::Trench;
      return true;
    case CreativeTerrainRecipeKind::RidgeLine:
      path = CreativeTerrainPathKind::Ridge;
      return true;
    default:
      return false;
  }
}

[[nodiscard]] CreativeTerrainMaterial semanticPathMaterial(
    CreativeTerrainRecipeKind kind) noexcept {
  switch (kind) {
    case CreativeTerrainRecipeKind::Road:
      return CreativeTerrainMaterial::Dirt;
    case CreativeTerrainRecipeKind::River:
    case CreativeTerrainRecipeKind::Ditch:
      return CreativeTerrainMaterial::Sand;
    default:
      return CreativeTerrainMaterial::Count;
  }
}

void initializePlanSource(CreativeTerrainRecipePlan& plan,
                          const CreativeDocument& document,
                          CreativeTerrainRecipeKind kind) {
  plan.kind = kind;
  plan.sourceDocumentId = document.id();
  plan.sourceDocumentRevision = document.revision();
  plan.sourceTerrainRevision = document.terrainField().revision();
  plan.sourceMaterialRevision = document.terrainMaterialField().revision();
}

[[nodiscard]] bool sourceMatches(const CreativeDocument& document,
                                 const CreativeTerrainRecipePlan& plan) noexcept {
  return document.isValid() && document.id() == plan.sourceDocumentId &&
         document.revision() == plan.sourceDocumentRevision &&
         document.terrainField().revision() == plan.sourceTerrainRevision &&
         document.terrainMaterialField().revision() ==
             plan.sourceMaterialRevision;
}

[[nodiscard]] bool profileBounds(CreativeTerrainCoord2 center,
                                 std::uint16_t radius,
                                 CreativeTerrainCoord2& minimum,
                                 CreativeTerrainCoord2& maximum) noexcept {
  const std::int64_t minimumX =
      static_cast<std::int64_t>(center.x) - radius;
  const std::int64_t minimumZ =
      static_cast<std::int64_t>(center.z) - radius;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(center.x) + radius;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(center.z) + radius;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      maximumX > std::numeric_limits<std::int32_t>::max() ||
      maximumZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  minimum = {static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)};
  maximum = {static_cast<std::int32_t>(maximumX),
             static_cast<std::int32_t>(maximumZ)};
  return true;
}

[[nodiscard]] bool columnInfluencedByControls(
    CreativeTerrainCoord2 coord,
    std::span<const CreativeTerrainControlPoint> controls) noexcept {
  for (const CreativeTerrainControlPoint& control : controls) {
    const std::int64_t dx = static_cast<std::int64_t>(coord.x) - control.coord.x;
    const std::int64_t dz = static_cast<std::int64_t>(coord.z) - control.coord.z;
    const std::int64_t radius = control.radiusCells;
    if (dx * dx + dz * dz <= radius * radius) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool appendPathMaterialEdits(
    const CreativeDocument& document,
    const CreativeTerrainPathPlan& path,
    CreativeTerrainMaterial material,
    std::vector<CreativeTerrainMaterialEdit>& output) {
  if (material >= CreativeTerrainMaterial::Count) {
    return true;
  }
  CreativeTerrainField staged = document.terrainField();
  const CreativeTerrainMutationReceipt applied = staged.apply(path.items());
  if (!applied.accepted) {
    return false;
  }
  const CreativeTerrainSurfacePlan surface =
      buildCreativeTerrainSurfacePlan(staged);
  if (!surface.accepted) {
    return false;
  }

  output.reserve(surface.columns.size());
  for (const CreativeTerrainColumn& column : surface.columns) {
    if (!columnInfluencedByControls(column.coord, path.finalControls()) ||
        document.terrainMaterialField().materialAt(column.coord) == material) {
      continue;
    }
    const CreativeTerrainMaterialEditKind editKind =
        material == CreativeTerrainMaterial::Grass
            ? CreativeTerrainMaterialEditKind::Clear
            : CreativeTerrainMaterialEditKind::Set;
    output.push_back({editKind, column.coord, material});
  }
  return output.size() <= kCreativeTerrainMaterialOverrideCapacity;
}

[[nodiscard]] bool applyPlanToDocument(
    CreativeDocument& document,
    const CreativeTerrainRecipePlan& plan,
    CreativeTerrainMutationReceipt& terrainReceipt,
    CreativeTerrainMaterialMutationReceipt& materialReceipt,
    std::string& reasonCode) {
  if (!plan.controlEdits.empty()) {
    terrainReceipt = document.applyTerrainControlEdits(plan.controlEdits);
    if (!terrainReceipt.accepted) {
      reasonCode = std::string(terrainReceipt.reasonCode);
      return false;
    }
  }
  if (!plan.materialEdits.empty()) {
    materialReceipt = document.applyTerrainMaterialEdits(plan.materialEdits);
    if (!materialReceipt.accepted) {
      reasonCode = std::string(materialReceipt.reasonCode);
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view toString(CreativeTerrainRecipeKind kind) noexcept {
  switch (kind) {
    case CreativeTerrainRecipeKind::Hill:
      return "Hill";
    case CreativeTerrainRecipeKind::Valley:
      return "Valley";
    case CreativeTerrainRecipeKind::Crater:
      return "Crater";
    case CreativeTerrainRecipeKind::Ridge:
      return "Ridge";
    case CreativeTerrainRecipeKind::Road:
      return "Road";
    case CreativeTerrainRecipeKind::River:
      return "River";
    case CreativeTerrainRecipeKind::Ditch:
      return "Ditch";
    case CreativeTerrainRecipeKind::RidgeLine:
      return "RidgeLine";
    case CreativeTerrainRecipeKind::Count:
      return "Invalid";
  }
  return "Invalid";
}

std::string_view toString(CreativeTerrainRecipeStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainRecipeStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeTerrainRecipeStatus::InvalidKind:
      return "InvalidKind";
    case CreativeTerrainRecipeStatus::KernelRejected:
      return "KernelRejected";
    case CreativeTerrainRecipeStatus::MaterialPlanRejected:
      return "MaterialPlanRejected";
    case CreativeTerrainRecipeStatus::NoChange:
      return "NoChange";
    case CreativeTerrainRecipeStatus::Ready:
      return "Ready";
    case CreativeTerrainRecipeStatus::StalePlan:
      return "StalePlan";
    case CreativeTerrainRecipeStatus::MutationRejected:
      return "MutationRejected";
    case CreativeTerrainRecipeStatus::InstallRejected:
      return "InstallRejected";
    case CreativeTerrainRecipeStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

CreativeTerrainRecipeResult buildCreativeTerrainProfileRecipe(
    const CreativeTerrainProfileRecipeRequest& request) {
  CreativeTerrainRecipeResult result;
  result.receipt.requested = true;
  result.receipt.kind = request.kind;
  if (!validDocument(request.document)) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::InvalidDocument,
              "creative_terrain_recipe_document_invalid");
    return result;
  }

  CreativeTerrainProfileKind profile = CreativeTerrainProfileKind::Hill;
  if (!profileKind(request.kind, profile)) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::InvalidKind,
              "creative_terrain_profile_recipe_kind_invalid");
    return result;
  }

  CreativeTerrainProfileRequest kernel;
  kernel.field = &request.document->terrainField();
  kernel.center = request.center;
  kernel.baseHeightCells = request.baseHeightCells;
  kernel.profile = profile;
  kernel.blend = request.blend;
  kernel.rodPolicy = request.rodPolicy;
  kernel.direction = request.direction;
  kernel.radiusCells = request.radiusCells;
  kernel.amplitudeCells = request.amplitudeCells;
  kernel.spacingCells = request.spacingCells;
  kernel.frequency = request.frequency;
  const CreativeTerrainProfilePlan profilePlan =
      buildCreativeTerrainProfilePlan(kernel);
  result.receipt.kernelReasonCode = std::string(profilePlan.reasonCode);
  if (!profilePlan.accepted) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::KernelRejected,
              profilePlan.reasonCode);
    return result;
  }

  initializePlanSource(result.plan, *request.document, request.kind);
  if (!profileBounds(request.center, request.radiusCells,
                     result.plan.minimumCoord, result.plan.maximumCoord)) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::KernelRejected,
              "creative_terrain_profile_coordinate_overflow");
    result.plan = {};
    return result;
  }
  result.plan.controlEdits.assign(profilePlan.items().begin(),
                                  profilePlan.items().end());
  result.receipt.controlEditCount = result.plan.controlEdits.size();
  if (result.plan.controlEdits.empty()) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::NoChange,
              "creative_terrain_recipe_no_change", true);
    return result;
  }
  setStatus(result.receipt, CreativeTerrainRecipeStatus::Ready,
            "creative_terrain_recipe_ready", true);
  return result;
}

CreativeTerrainRecipeResult buildCreativeTerrainPathRecipe(
    const CreativeTerrainPathRecipeRequest& request) {
  CreativeTerrainRecipeResult result;
  result.receipt.requested = true;
  result.receipt.kind = request.kind;
  if (!validDocument(request.document)) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::InvalidDocument,
              "creative_terrain_recipe_document_invalid");
    return result;
  }

  CreativeTerrainPathKind kind = CreativeTerrainPathKind::Road;
  if (!pathKind(request.kind, kind)) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::InvalidKind,
              "creative_terrain_path_recipe_kind_invalid");
    return result;
  }

  CreativeTerrainPathRequest kernel;
  kernel.field = &request.document->terrainField();
  kernel.points = request.points;
  kernel.kind = kind;
  kernel.elevation = request.elevation;
  kernel.halfWidthCells = request.halfWidthCells;
  kernel.amplitudeCells = request.amplitudeCells;
  const CreativeTerrainPathPlan pathPlan =
      buildCreativeTerrainPathPlan(kernel);
  result.receipt.kernelReasonCode = std::string(pathPlan.reasonCode);
  if (!pathPlan.accepted) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::KernelRejected,
              pathPlan.reasonCode);
    return result;
  }

  initializePlanSource(result.plan, *request.document, request.kind);
  result.plan.minimumCoord = pathPlan.minimumCoord;
  result.plan.maximumCoord = pathPlan.maximumCoord;
  result.plan.controlEdits.assign(pathPlan.items().begin(),
                                  pathPlan.items().end());
  CreativeTerrainMaterial material = request.material;
  if (material != CreativeTerrainMaterial::Count &&
      !isValidCreativeTerrainMaterial(material)) {
    setStatus(result.receipt,
              CreativeTerrainRecipeStatus::MaterialPlanRejected,
              "creative_terrain_recipe_material_invalid");
    result.plan = {};
    return result;
  }
  if (material == CreativeTerrainMaterial::Count) {
    material = semanticPathMaterial(request.kind);
  }
  if (request.paintSurface && material < CreativeTerrainMaterial::Count &&
      !appendPathMaterialEdits(*request.document, pathPlan, material,
                               result.plan.materialEdits)) {
    setStatus(result.receipt,
              CreativeTerrainRecipeStatus::MaterialPlanRejected,
              "creative_terrain_recipe_material_plan_rejected");
    result.plan = {};
    return result;
  }

  result.receipt.controlEditCount = result.plan.controlEdits.size();
  result.receipt.materialEditCount = result.plan.materialEdits.size();
  if (result.plan.controlEdits.empty() && result.plan.materialEdits.empty()) {
    setStatus(result.receipt, CreativeTerrainRecipeStatus::NoChange,
              "creative_terrain_recipe_no_change", true);
    return result;
  }
  setStatus(result.receipt, CreativeTerrainRecipeStatus::Ready,
            "creative_terrain_recipe_ready", true);
  return result;
}

CreativeTerrainRecipePreviewResult previewCreativeTerrainRecipe(
    const CreativeDocument& document,
    const CreativeTerrainRecipePlan& plan) {
  CreativeTerrainRecipePreviewResult result;
  result.requested = true;
  if (!sourceMatches(document, plan)) {
    setStatus(result, CreativeTerrainRecipeStatus::StalePlan,
              "creative_terrain_recipe_plan_stale");
    return result;
  }

  CreativeDocument staged = document;
  std::string applyReason;
  if (!applyPlanToDocument(staged, plan, result.terrainReceipt,
                           result.materialReceipt, applyReason)) {
    setStatus(result, CreativeTerrainRecipeStatus::MutationRejected,
              applyReason);
    return result;
  }
  const CreativeTerrainSurfacePlan surface =
      buildCreativeTerrainSurfacePlan(staged.terrainField());
  if (!surface.accepted) {
    setStatus(result, CreativeTerrainRecipeStatus::MutationRejected,
              surface.reasonCode);
    return result;
  }
  result.renderPlan = buildCreativeTerrainRenderPlan(
      surface, staged.terrainMaterialField(), staged.gridSettings().origin,
      staged.gridSettings().cellSizeMeters);
  if (!result.renderPlan.accepted) {
    setStatus(result, CreativeTerrainRecipeStatus::MutationRejected,
              result.renderPlan.reasonCode);
    return result;
  }
  setStatus(result, CreativeTerrainRecipeStatus::Ready,
            "creative_terrain_recipe_preview_ready", true);
  return result;
}

CreativeTerrainRecipeApplyReceipt applyCreativeTerrainRecipe(
    Facade& facade,
    const CreativeTerrainRecipePlan& plan) {
  CreativeTerrainRecipeApplyReceipt receipt;
  receipt.requested = true;
  if (!sourceMatches(facade.document(), plan)) {
    setStatus(receipt, CreativeTerrainRecipeStatus::StalePlan,
              "creative_terrain_recipe_plan_stale");
    return receipt;
  }
  if (plan.controlEdits.empty() && plan.materialEdits.empty()) {
    setStatus(receipt, CreativeTerrainRecipeStatus::NoChange,
              "creative_terrain_recipe_no_change", true);
    return receipt;
  }

  CreativeDocument staged = facade.document();
  std::string applyReason;
  if (!applyPlanToDocument(staged, plan, receipt.terrainReceipt,
                           receipt.materialReceipt, applyReason)) {
    setStatus(receipt, CreativeTerrainRecipeStatus::MutationRejected,
              applyReason);
    return receipt;
  }
  const bool changed = receipt.terrainReceipt.changed ||
                       receipt.materialReceipt.changed;
  if (!changed) {
    setStatus(receipt, CreativeTerrainRecipeStatus::NoChange,
              "creative_terrain_recipe_no_change", true);
    return receipt;
  }

  receipt.installReceipt = facade.installDocument(std::move(staged));
  if (!receipt.installReceipt.accepted || !receipt.installReceipt.changed) {
    setStatus(receipt, CreativeTerrainRecipeStatus::InstallRejected,
              receipt.installReceipt.reasonCode);
    return receipt;
  }
  receipt.changed = true;
  setStatus(receipt, CreativeTerrainRecipeStatus::Applied,
            "creative_terrain_recipe_applied", true);
  return receipt;
}

CreativeTerrainRecipeApplyReceipt applyCreativeTerrainRecipeWithHistory(
    CreativeAppState& appState,
    const CreativeTerrainRecipePlan& plan,
    std::string_view source) {
  CreativeDocumentHistoryTransaction transaction =
      beginCreativeHistoryTransaction(appState.facade, source);
  CreativeTerrainRecipeApplyReceipt receipt =
      applyCreativeTerrainRecipe(appState.facade, plan);
  if (!receipt.accepted || !receipt.changed) {
    cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }
  receipt.historyReceipt = commitCreativeHistoryTransaction(
      appState.history, std::move(transaction), appState.facade);
  if (!receipt.historyReceipt.accepted || !receipt.historyReceipt.recorded) {
    receipt.reasonCode = std::string(receipt.historyReceipt.reasonCode);
  }
  return receipt;
}

}  // namespace iggy3d::creative
