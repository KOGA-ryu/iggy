#include "app/iggy3d/creative/world/WorldLayoutTerrainImpact.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <utility>

namespace iggy3d::creative {
namespace {

struct CoordLess {
  [[nodiscard]] bool operator()(CreativeTerrainCoord2 lhs,
                                CreativeTerrainCoord2 rhs) const noexcept {
    return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
  }
};

struct ControlOwner {
  std::size_t sourceIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeTerrainControlPoint control{};
};

struct MaterialOwner {
  std::size_t sourceIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
};

void reject(CreativeWorldLayoutTerrainImpactPlan& plan,
            CreativeWorldLayoutTerrainImpactPlanStatus status,
            std::string_view reasonCode,
            CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None,
            std::size_t index = kInvalidCreativeWorldLayoutIndex,
            std::string_view kernelReasonCode = {}) {
  plan.accepted = false;
  plan.status = status;
  plan.failedTable = table;
  plan.failedIndex = index;
  plan.reasonCode = reasonCode;
  if (!kernelReasonCode.empty()) {
    plan.kernelReasonCode = kernelReasonCode;
  }
}

[[nodiscard]] bool validDocument(const CreativeDocument& document) noexcept {
  return document.isValid() && document.id() != kInvalidDocumentId &&
         document.terrainField().validateInvariants() &&
         document.terrainMaterialField().validateInvariants();
}

[[nodiscard]] bool registerKey(std::set<std::string>& keys,
                               std::string_view key) {
  return validCreativeWorldLayoutStableKey(key) && keys.emplace(key).second;
}

void includeHeight(CreativeWorldLayoutTerrainSourceImpact& impact,
                   std::uint16_t heightCells) noexcept {
  if (!impact.hasHeightRange) {
    impact.hasHeightRange = true;
    impact.minimumHeightCells = heightCells;
    impact.maximumHeightCells = heightCells;
    return;
  }
  impact.minimumHeightCells = std::min(impact.minimumHeightCells, heightCells);
  impact.maximumHeightCells = std::max(impact.maximumHeightCells, heightCells);
}

void recordControlOutputs(
    const CreativeDocument& staged,
    const CreativeTerrainRecipePlan& recipe,
    std::size_t sourceIndex,
    CreativeWorldLayoutTerrainSourceImpact& impact,
    std::map<CreativeTerrainCoord2, ControlOwner, CoordLess>& owners) {
  std::set<CreativeTerrainCoord2, CoordLess> changedCoords;
  for (const CreativeTerrainControlEdit& edit : recipe.controlEdits) {
    const CreativeTerrainControlPoint* existing =
        staged.terrainField().controlAt(edit.control.coord);
    const bool changes = edit.kind == CreativeTerrainEditKind::Upsert
                             ? existing == nullptr || *existing != edit.control
                             : edit.kind == CreativeTerrainEditKind::Remove &&
                                   existing != nullptr;
    if (!changes) {
      continue;
    }
    ++impact.effectiveControlEditCount;
    if (edit.kind == CreativeTerrainEditKind::Upsert) {
      changedCoords.insert(edit.control.coord);
    } else {
      owners.erase(edit.control.coord);
    }
  }
  impact.authoredControlCount = recipe.controlOutputs.size();
  for (const CreativeTerrainControlPoint& output : recipe.controlOutputs) {
    includeHeight(impact, output.heightCells);
    if (!owners.contains(output.coord) || changedCoords.contains(output.coord)) {
      owners[output.coord] = {sourceIndex, output};
    }
  }
}

void recordMaterialOutputs(
    const CreativeDocument& staged,
    const CreativeTerrainRecipePlan& recipe,
    std::size_t sourceIndex,
    CreativeWorldLayoutTerrainSourceImpact& impact,
    std::map<CreativeTerrainCoord2, MaterialOwner, CoordLess>& owners) {
  std::set<CreativeTerrainCoord2, CoordLess> changedCoords;
  for (const CreativeTerrainMaterialEdit& edit : recipe.materialEdits) {
    const CreativeTerrainMaterial desired =
        edit.kind == CreativeTerrainMaterialEditKind::Clear
            ? CreativeTerrainMaterial::Grass
            : edit.material;
    if (staged.terrainMaterialField().materialAt(edit.coord) == desired) {
      continue;
    }
    ++impact.effectiveMaterialEditCount;
    changedCoords.insert(edit.coord);
  }
  impact.authoredMaterialCount = recipe.materialOutputs.size();
  for (const CreativeTerrainMaterialOverride& output :
       recipe.materialOutputs) {
    if (!owners.contains(output.coord) || changedCoords.contains(output.coord)) {
      owners[output.coord] = {sourceIndex, output.material};
    }
  }
}

[[nodiscard]] bool applyRecipe(
    CreativeDocument& staged,
    const CreativeTerrainRecipeResult& recipe,
    std::size_t sourceIndex,
    CreativeWorldLayoutTerrainSourceImpact& impact,
    std::map<CreativeTerrainCoord2, ControlOwner, CoordLess>& controlOwners,
    std::map<CreativeTerrainCoord2, MaterialOwner, CoordLess>& materialOwners) {
  impact.hasGridBounds = true;
  impact.minimumCoord = recipe.plan.minimumCoord;
  impact.maximumCoord = recipe.plan.maximumCoord;
  recordControlOutputs(staged, recipe.plan, sourceIndex, impact,
                       controlOwners);
  recordMaterialOutputs(staged, recipe.plan, sourceIndex, impact,
                        materialOwners);
  if (!recipe.plan.controlEdits.empty() &&
      !staged.applyTerrainControlEdits(recipe.plan.controlEdits).accepted) {
    return false;
  }
  return recipe.plan.materialEdits.empty() ||
         staged.applyTerrainMaterialEdits(recipe.plan.materialEdits).accepted;
}

void finalizeSources(
    const CreativeDocument& document,
    const CreativeDocument& staged,
    const std::map<CreativeTerrainCoord2, ControlOwner, CoordLess>&
        controlOwners,
    const std::map<CreativeTerrainCoord2, MaterialOwner, CoordLess>&
        materialOwners,
    CreativeWorldLayoutTerrainImpactPlan& plan) {
  for (const auto& [coord, owner] : controlOwners) {
    const CreativeTerrainControlPoint* control =
        staged.terrainField().controlAt(coord);
    if (control != nullptr && owner.sourceIndex < plan.sources.size()) {
      plan.sources[owner.sourceIndex].controls.push_back(*control);
    }
  }
  for (const auto& [coord, owner] : materialOwners) {
    if (owner.sourceIndex < plan.sources.size()) {
      plan.sources[owner.sourceIndex].materials.push_back(
          {coord, owner.material});
    }
  }

  for (CreativeWorldLayoutTerrainSourceImpact& impact : plan.sources) {
    const bool authored = impact.authoredControlCount > 0U ||
                          impact.authoredMaterialCount > 0U;
    if (!authored) {
      impact.status = CreativeWorldLayoutTerrainImpactStatus::NoEffect;
      continue;
    }
    if (impact.controls.empty() && impact.materials.empty()) {
      const bool changed = impact.effectiveControlEditCount > 0U ||
                           impact.effectiveMaterialEditCount > 0U;
      impact.status = changed
                          ? CreativeWorldLayoutTerrainImpactStatus::Overridden
                          : CreativeWorldLayoutTerrainImpactStatus::NoEffect;
      continue;
    }
    const bool controlsMatch =
        std::all_of(impact.controls.begin(), impact.controls.end(),
                    [&document](const CreativeTerrainControlPoint& expected) {
                      const CreativeTerrainControlPoint* live =
                          document.terrainField().controlAt(expected.coord);
                      return live != nullptr && *live == expected;
                    });
    const bool materialsMatch = std::all_of(
        impact.materials.begin(), impact.materials.end(),
        [&document](const CreativeWorldLayoutTerrainMaterialImpact& expected) {
          return document.terrainMaterialField().materialAt(expected.coord) ==
                 expected.material;
        });
    impact.status = controlsMatch && materialsMatch
                        ? CreativeWorldLayoutTerrainImpactStatus::Current
                        : CreativeWorldLayoutTerrainImpactStatus::Drifted;

    std::set<CreativeTerrainCoord2, CoordLess> influence;
    for (const CreativeTerrainControlPoint& control : impact.controls) {
      const std::int32_t radius = control.radiusCells;
      for (std::int32_t dz = -radius; dz <= radius; ++dz) {
        for (std::int32_t dx = -radius; dx <= radius; ++dx) {
          if (static_cast<std::int64_t>(dx) * dx +
                  static_cast<std::int64_t>(dz) * dz >
              static_cast<std::int64_t>(radius) * radius) {
            continue;
          }
          const std::int64_t x =
              static_cast<std::int64_t>(control.coord.x) + dx;
          const std::int64_t z =
              static_cast<std::int64_t>(control.coord.z) + dz;
          if (x < std::numeric_limits<std::int32_t>::min() ||
              x > std::numeric_limits<std::int32_t>::max() ||
              z < std::numeric_limits<std::int32_t>::min() ||
              z > std::numeric_limits<std::int32_t>::max()) {
            impact.influenceCellsClipped = true;
            continue;
          }
          const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                            static_cast<std::int32_t>(z)};
          if (influence.contains(coord)) {
            continue;
          }
          if (influence.size() >= kCreativeTerrainRenderPatchCapacity) {
            impact.influenceCellsClipped = true;
            continue;
          }
          influence.insert(coord);
        }
      }
    }
    if (!impact.influenceCellsClipped) {
      impact.influenceCells.assign(influence.begin(), influence.end());
    }
  }
}

}  // namespace

std::string_view
toString(CreativeWorldLayoutTerrainImpactStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutTerrainImpactStatus::NoEffect:
      return "NoEffect";
    case CreativeWorldLayoutTerrainImpactStatus::Current:
      return "Current";
    case CreativeWorldLayoutTerrainImpactStatus::Drifted:
      return "Drifted";
    case CreativeWorldLayoutTerrainImpactStatus::Overridden:
      return "Overridden";
    case CreativeWorldLayoutTerrainImpactStatus::Count:
      break;
  }
  return "Invalid";
}

std::string_view
toString(CreativeWorldLayoutTerrainImpactPlanStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutTerrainImpactPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutTerrainImpactPlanStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeWorldLayoutTerrainImpactPlanStatus::InvalidLayout:
      return "InvalidLayout";
    case CreativeWorldLayoutTerrainImpactPlanStatus::KernelRejected:
      return "KernelRejected";
    case CreativeWorldLayoutTerrainImpactPlanStatus::MutationRejected:
      return "MutationRejected";
    case CreativeWorldLayoutTerrainImpactPlanStatus::Empty:
      return "Empty";
    case CreativeWorldLayoutTerrainImpactPlanStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeWorldLayoutTerrainImpactPlan
buildCreativeWorldLayoutTerrainImpactPlan(const CreativeDocument& document,
                                          const CreativeWorldLayout& layout) {
  CreativeWorldLayoutTerrainImpactPlan plan;
  plan.requested = true;
  plan.sourceDocumentId = document.id();
  plan.sourceDocumentRevision = document.revision();
  plan.sourceTerrainRevision = document.terrainField().revision();
  plan.sourceMaterialRevision = document.terrainMaterialField().revision();
  if (!validDocument(document)) {
    reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::InvalidDocument,
           "creative_world_layout_terrain_impact_document_invalid");
    return plan;
  }
  if (layout.schemaVersion != kCreativeWorldLayoutSchemaVersion ||
      layout.terrainOwnership >
          CreativeWorldLayoutTerrainOwnership::ReplaceAll) {
    reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::InvalidLayout,
           "creative_world_layout_terrain_impact_layout_invalid");
    return plan;
  }
  if (layout.terrainProfiles.empty() && layout.terrainPaths.empty()) {
    plan.accepted = true;
    plan.status = CreativeWorldLayoutTerrainImpactPlanStatus::Empty;
    plan.reasonCode = "creative_world_layout_terrain_impact_empty";
    return plan;
  }

  CreativeDocument staged;
  if (layout.terrainOwnership ==
      CreativeWorldLayoutTerrainOwnership::PreserveExisting) {
    staged = document;
  } else {
    staged = CreativeDocument::create("World Layout terrain attribution");
    static_cast<void>(staged.assignId(document.id()));
    static_cast<void>(staged.setGridSettings(document.gridSettings()));
  }
  const std::string layoutPathPrefix =
      creativeWorldLayoutTerrainPathSourceKey(layout.stableKey, {});
  const std::string layoutLandformPrefix =
      creativeWorldLayoutTerrainLandformSourceKey(layout.stableKey, {});
  std::vector<CreativeTerrainOperationId> ownedTerrainOperationIds;
  for (const CreativeTerrainOperation& operation :
       staged.terrainOperationStack().operations) {
    if (operation.owner == CreativeTerrainOperationOwner::WorldLayout &&
        (operation.sourceKey.starts_with(layoutPathPrefix) ||
         operation.sourceKey.starts_with(layoutLandformPrefix))) {
      ownedTerrainOperationIds.push_back(operation.id);
    }
  }
  for (const CreativeTerrainOperationId operationId :
       ownedTerrainOperationIds) {
    CreativeTerrainOperationMutationRequest remove;
    remove.kind = CreativeTerrainOperationMutationKind::Remove;
    remove.operationId = operationId;
    if (!staged.applyTerrainOperationMutation(remove).accepted) {
      reject(plan,
             CreativeWorldLayoutTerrainImpactPlanStatus::MutationRejected,
             "creative_world_layout_terrain_impact_operation_reset_rejected");
      return plan;
    }
  }
  std::map<CreativeTerrainCoord2, ControlOwner, CoordLess> controlOwners;
  std::map<CreativeTerrainCoord2, MaterialOwner, CoordLess> materialOwners;
  std::set<std::string> stableKeys;

  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& symbol =
        layout.terrainProfiles[index];
    if (!registerKey(stableKeys, symbol.stableKey) ||
        (symbol.usesLandformRecipe
             ? !isValidCreativeTerrainLandformRecipe(symbol.landform)
             : symbol.kind == CreativeTerrainRecipeKind::Terrace ||
                   symbol.kind == CreativeTerrainRecipeKind::Cliff ||
                   symbol.blend != CreativeTerrainProfileBlend::Set ||
                   symbol.rodPolicy != CreativeTerrainProfileRodPolicy::Fill)) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::InvalidLayout,
             "creative_world_layout_terrain_impact_profile_invalid",
             CreativeWorldLayoutTable::TerrainProfile, index);
      return plan;
    }
    if (symbol.usesLandformRecipe) {
      CreativeTerrainLandformKind expectedKind =
          CreativeTerrainLandformKind::Count;
      if (!creativeTerrainRecipeLandformKind(symbol.kind, expectedKind) ||
          expectedKind != symbol.landform.kind) {
        reject(plan,
               CreativeWorldLayoutTerrainImpactPlanStatus::InvalidLayout,
               "creative_world_layout_terrain_impact_landform_kind_invalid",
               CreativeWorldLayoutTable::TerrainProfile, index);
        return plan;
      }
      continue;
    }
    const std::size_t sourceIndex = plan.sources.size();
    CreativeWorldLayoutTerrainSourceImpact impact;
    impact.table = CreativeWorldLayoutTable::TerrainProfile;
    impact.index = index;
    impact.stableKey = symbol.stableKey;
    plan.sources.push_back(std::move(impact));
    CreativeTerrainProfileRecipeRequest request;
    request.document = &staged;
    request.kind = symbol.kind;
    request.center = symbol.center;
    request.baseHeightCells = symbol.baseHeightCells;
    request.radiusCells = symbol.radiusCells;
    request.amplitudeCells = symbol.amplitudeCells;
    request.spacingCells = symbol.spacingCells;
    request.blend = symbol.blend;
    request.rodPolicy = symbol.rodPolicy;
    request.direction = symbol.direction;
    request.frequency = symbol.frequency;
    const CreativeTerrainRecipeResult recipe =
        buildCreativeTerrainProfileRecipe(request);
    if (!recipe.receipt.accepted) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::KernelRejected,
             recipe.receipt.reasonCode,
             CreativeWorldLayoutTable::TerrainProfile, index,
             recipe.receipt.kernelReasonCode);
      return plan;
    }
    if (!applyRecipe(staged, recipe, sourceIndex, plan.sources.back(),
                     controlOwners, materialOwners)) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::MutationRejected,
             "creative_world_layout_terrain_impact_profile_stage_rejected",
             CreativeWorldLayoutTable::TerrainProfile, index);
      return plan;
    }
  }

  finalizeSources(document, staged, controlOwners, materialOwners, plan);

  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& symbol =
        layout.terrainProfiles[index];
    if (!symbol.usesLandformRecipe) {
      continue;
    }
    CreativeWorldLayoutTerrainSourceImpact impact;
    impact.table = CreativeWorldLayoutTable::TerrainProfile;
    impact.index = index;
    impact.stableKey = symbol.stableKey;
    impact.hasGridBounds = true;
    impact.minimumCoord = symbol.landform.bounds.minimum;
    impact.maximumCoord = {
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(symbol.landform.bounds.minimum.x) +
            symbol.landform.bounds.widthCells - 1),
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(symbol.landform.bounds.minimum.z) +
            symbol.landform.bounds.depthCells - 1)};

    const CreativeTerrainSurfacePlan surface =
        buildCreativeComposedTerrainSurfacePlan(
            staged.terrainField(), staged.terrainHeightField());
    const CreativeTerrainLandformResult recipe =
        buildCreativeTerrainLandform(
            staged.terrainHeightField(), surface,
            staged.terrainMaterialField(), symbol.landform);
    if (!surface.accepted || !recipe.receipt.accepted) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::KernelRejected,
             "creative_world_layout_terrain_impact_landform_kernel_rejected",
             CreativeWorldLayoutTable::TerrainProfile, index,
             recipe.receipt.reasonCode);
      return plan;
    }
    impact.authoredControlCount = recipe.receipt.modifiedCellCount;
    impact.authoredMaterialCount = recipe.materialEdits.size();
    for (std::int64_t z = impact.minimumCoord.z;
         z <= impact.maximumCoord.z; ++z) {
      for (std::int64_t x = impact.minimumCoord.x;
           x <= impact.maximumCoord.x; ++x) {
        const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                          static_cast<std::int32_t>(z)};
        const std::optional<std::uint16_t> height =
            recipe.heightField.heightAt(coord);
        if (height.has_value()) {
          includeHeight(impact, *height);
        }
        if (height == staged.terrainHeightField().heightAt(coord)) {
          continue;
        }
        if (impact.influenceCells.size() >=
            kCreativeTerrainRenderPatchCapacity) {
          impact.influenceCells.clear();
          impact.influenceCellsClipped = true;
          break;
        }
        impact.influenceCells.push_back(coord);
      }
      if (impact.influenceCellsClipped) {
        break;
      }
    }
    for (const CreativeTerrainMaterialEdit& edit : recipe.materialEdits) {
      if (edit.kind == CreativeTerrainMaterialEditKind::Set) {
        impact.materials.push_back({edit.coord, edit.material});
      }
    }

    const std::string sourceKey =
        creativeWorldLayoutTerrainLandformSourceKey(layout.stableKey,
                                                    symbol.stableKey);
    const CreativeTerrainOperation* live = nullptr;
    for (const CreativeTerrainOperation& operation :
         document.terrainOperationStack().operations) {
      if (operation.owner == CreativeTerrainOperationOwner::WorldLayout &&
          operation.sourceKey == sourceKey) {
        if (live != nullptr) {
          reject(
              plan,
              CreativeWorldLayoutTerrainImpactPlanStatus::InvalidDocument,
              "creative_world_layout_terrain_impact_landform_source_duplicate",
              CreativeWorldLayoutTable::TerrainProfile, index);
          return plan;
        }
        live = &operation;
      }
    }
    impact.status =
        live != nullptr && live->enabled &&
                live->kind == CreativeTerrainOperationKind::Landform &&
                live->landform == symbol.landform
            ? CreativeWorldLayoutTerrainImpactStatus::Current
            : CreativeWorldLayoutTerrainImpactStatus::Drifted;
    if (impact.status != CreativeWorldLayoutTerrainImpactStatus::Current) {
      impact.effectiveControlEditCount = recipe.receipt.modifiedCellCount;
      impact.effectiveMaterialEditCount = recipe.materialEdits.size();
    }
    plan.sources.push_back(std::move(impact));

    CreativeTerrainOperationMutationRequest request;
    request.kind = CreativeTerrainOperationMutationKind::Add;
    request.owner = CreativeTerrainOperationOwner::WorldLayout;
    request.sourceKey = sourceKey;
    request.operationKind = CreativeTerrainOperationKind::Landform;
    request.landform = symbol.landform;
    const CreativeTerrainOperationMutationReceipt mutation =
        staged.applyTerrainOperationMutation(request);
    if (!mutation.accepted) {
      reject(plan,
             CreativeWorldLayoutTerrainImpactPlanStatus::MutationRejected,
             "creative_world_layout_terrain_impact_landform_stage_rejected",
             CreativeWorldLayoutTable::TerrainProfile, index);
      return plan;
    }
  }

  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    if (!registerKey(stableKeys, symbol.stableKey) ||
        !isValidCreativeTerrainPathSourceRecipe(symbol.recipe)) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::InvalidLayout,
             "creative_world_layout_terrain_impact_path_invalid",
             CreativeWorldLayoutTable::TerrainPath, index);
      return plan;
    }
    CreativeWorldLayoutTerrainSourceImpact impact;
    impact.table = CreativeWorldLayoutTable::TerrainPath;
    impact.index = index;
    impact.stableKey = symbol.stableKey;
    const CreativeTerrainSurfacePlan surface =
        buildCreativeComposedTerrainSurfacePlan(
            staged.terrainField(), staged.terrainHeightField());
    const CreativeTerrainPathSourceResult recipe =
        buildCreativeTerrainPathSourceRecipe(
            staged.terrainHeightField(), surface,
            staged.terrainMaterialField(), symbol.recipe);
    if (!surface.accepted || !recipe.receipt.accepted) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::KernelRejected,
             "creative_world_layout_terrain_impact_path_kernel_rejected",
             CreativeWorldLayoutTable::TerrainPath, index,
             recipe.receipt.reasonCode);
      return plan;
    }
    for (const CreativeTerrainPathSegmentReceipt& segment : recipe.segments) {
      const CreativeTerrainHeightFieldBounds bounds = segment.impactBounds;
      const CreativeTerrainCoord2 maximum{
          static_cast<std::int32_t>(
              static_cast<std::int64_t>(bounds.minimum.x) +
              bounds.widthCells - 1),
          static_cast<std::int32_t>(
              static_cast<std::int64_t>(bounds.minimum.z) +
              bounds.depthCells - 1)};
      if (!impact.hasGridBounds) {
        impact.hasGridBounds = true;
        impact.minimumCoord = bounds.minimum;
        impact.maximumCoord = maximum;
      } else {
        impact.minimumCoord.x =
            std::min(impact.minimumCoord.x, bounds.minimum.x);
        impact.minimumCoord.z =
            std::min(impact.minimumCoord.z, bounds.minimum.z);
        impact.maximumCoord.x = std::max(impact.maximumCoord.x, maximum.x);
        impact.maximumCoord.z = std::max(impact.maximumCoord.z, maximum.z);
      }
    }
    impact.authoredControlCount = recipe.receipt.modifiedCellCount;
    impact.authoredMaterialCount = recipe.materialEdits.size();
    if (impact.hasGridBounds) {
      for (std::int64_t z = impact.minimumCoord.z;
           z <= impact.maximumCoord.z; ++z) {
        for (std::int64_t x = impact.minimumCoord.x;
             x <= impact.maximumCoord.x; ++x) {
          const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                            static_cast<std::int32_t>(z)};
          const std::uint16_t height = recipe.heightField.heightAt(coord)
                                           .value_or(
                                               kCreativeTerrainEmptyHeightCells);
          includeHeight(impact, height);
          if (recipe.heightField.heightAt(coord) ==
              staged.terrainHeightField().heightAt(coord)) {
            continue;
          }
          if (impact.influenceCells.size() >=
              kCreativeTerrainRenderPatchCapacity) {
            impact.influenceCells.clear();
            impact.influenceCellsClipped = true;
            break;
          }
          impact.influenceCells.push_back(coord);
        }
        if (impact.influenceCellsClipped) {
          break;
        }
      }
    }
    for (const CreativeTerrainMaterialEdit& edit : recipe.materialEdits) {
      if (edit.kind == CreativeTerrainMaterialEditKind::Set) {
        impact.materials.push_back({edit.coord, edit.material});
      }
    }

    const std::string sourceKey = creativeWorldLayoutTerrainPathSourceKey(
        layout.stableKey, symbol.stableKey);
    const CreativeTerrainOperation* live = nullptr;
    for (const CreativeTerrainOperation& operation :
         document.terrainOperationStack().operations) {
      if (operation.owner == CreativeTerrainOperationOwner::WorldLayout &&
          operation.sourceKey == sourceKey) {
        if (live != nullptr) {
          reject(plan,
                 CreativeWorldLayoutTerrainImpactPlanStatus::InvalidDocument,
                 "creative_world_layout_terrain_impact_path_source_duplicate",
                 CreativeWorldLayoutTable::TerrainPath, index);
          return plan;
        }
        live = &operation;
      }
    }
    impact.status =
        live != nullptr && live->enabled &&
                live->kind == CreativeTerrainOperationKind::Path &&
                live->path == symbol.recipe
            ? CreativeWorldLayoutTerrainImpactStatus::Current
            : CreativeWorldLayoutTerrainImpactStatus::Drifted;
    if (impact.status != CreativeWorldLayoutTerrainImpactStatus::Current) {
      impact.effectiveControlEditCount = recipe.receipt.modifiedCellCount;
      impact.effectiveMaterialEditCount = recipe.materialEdits.size();
    }
    plan.sources.push_back(std::move(impact));

    CreativeTerrainOperationMutationRequest request;
    request.kind = CreativeTerrainOperationMutationKind::Add;
    request.operationId = kInvalidCreativeTerrainOperationId;
    request.owner = CreativeTerrainOperationOwner::WorldLayout;
    request.sourceKey = sourceKey;
    request.operationKind = CreativeTerrainOperationKind::Path;
    request.path = symbol.recipe;
    const CreativeTerrainOperationMutationReceipt mutation =
        staged.applyTerrainOperationMutation(request);
    if (!mutation.accepted) {
      reject(plan, CreativeWorldLayoutTerrainImpactPlanStatus::MutationRejected,
             "creative_world_layout_terrain_impact_path_stage_rejected",
             CreativeWorldLayoutTable::TerrainPath, index);
      return plan;
    }
  }
  plan.accepted = true;
  plan.status = CreativeWorldLayoutTerrainImpactPlanStatus::Ready;
  plan.reasonCode = "creative_world_layout_terrain_impact_ready";
  return plan;
}

const CreativeWorldLayoutTerrainSourceImpact*
findCreativeWorldLayoutTerrainSourceImpact(
    const CreativeWorldLayoutTerrainImpactPlan& plan,
    CreativeWorldLayoutTable table,
    std::size_t index) noexcept {
  const auto found = std::find_if(
      plan.sources.begin(), plan.sources.end(),
      [table, index](const CreativeWorldLayoutTerrainSourceImpact& impact) {
        return impact.table == table && impact.index == index;
      });
  return found == plan.sources.end() ? nullptr : &*found;
}

bool creativeWorldLayoutTerrainImpactWorldBounds(
    const CreativeWorldLayoutTerrainSourceImpact& impact,
    CreativeGridSettings grid,
    CreativeBounds& output) noexcept {
  if (!impact.hasGridBounds || !isFiniteCreativeVec3(grid.origin) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return false;
  }
  const double maximumX = static_cast<double>(impact.maximumCoord.x) + 1.0;
  const double maximumZ = static_cast<double>(impact.maximumCoord.z) + 1.0;
  const double maximumHeight =
      impact.hasHeightRange
          ? static_cast<double>(impact.maximumHeightCells) + 0.25
          : 1.0;
  output = {{grid.origin.x + impact.minimumCoord.x * grid.cellSizeMeters,
             grid.origin.y,
             grid.origin.z + impact.minimumCoord.z * grid.cellSizeMeters},
            {grid.origin.x + maximumX * grid.cellSizeMeters,
             grid.origin.y + maximumHeight * grid.cellSizeMeters,
             grid.origin.z + maximumZ * grid.cellSizeMeters}};
  return isFiniteCreativeVec3(output.min) && isFiniteCreativeVec3(output.max) &&
         output.min.x < output.max.x && output.min.y < output.max.y &&
         output.min.z < output.max.z;
}

}  // namespace iggy3d::creative
