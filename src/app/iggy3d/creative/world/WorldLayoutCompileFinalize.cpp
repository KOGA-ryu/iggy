#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"

namespace iggy3d::creative::world_layout_compile {

void finalizeWorldLayoutCompileResult(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutCompileResult& result) {
  const bool hasSourceSymbols =
      !layout.buildings.empty() || !layout.levels.empty() ||
      !layout.rooms.empty() || !layout.boxes.empty() ||
      !layout.walls.empty() || !layout.openings.empty() ||
      !layout.objects.empty() ||
      !layout.terrainProfiles.empty() || !layout.terrainPaths.empty();
  const bool hasOperations = !result.plan.objectDetachIds.empty() ||
                             !result.plan.objectRemoveIds.empty() ||
                             !result.plan.objectRecipePatches.empty() ||
                             !result.plan.objectRecipes.empty() ||
                             !result.plan.terrainEdits.empty() ||
                             !result.plan.materialEdits.empty() ||
                             !result.plan.terrainOperationMutations.empty();
  if (!hasSourceSymbols && !hasOperations) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::Empty,
              "creative_world_layout_empty");
    result.plan = {};
    return;
  }

  const CreativeWorldLayoutPreviewResult preview =
      previewCreativeWorldLayoutPlan(document, result.plan);
  if (!preview.accepted) {
    setStatus(result.receipt, preview.status, preview.reasonCode);
    result.plan = {};
    return;
  }

  result.receipt.buildingCount = layout.buildings.size();
  result.receipt.objectRecipeCount =
      result.plan.objectRecipePatches.size() +
      result.plan.objectRecipes.size();
  result.receipt.objectDetachCount = result.plan.objectDetachIds.size();
  result.receipt.objectRemoveCount = result.plan.objectRemoveIds.size();
  result.receipt.terrainControlEditCount = result.plan.terrainEdits.size();
  result.receipt.terrainMaterialEditCount = result.plan.materialEdits.size();
  result.receipt.terrainOperationMutationCount =
      result.plan.terrainOperationMutations.size();
  const bool changed = preview.status == CreativeWorldLayoutStatus::Ready;
  setStatus(result.receipt, preview.status,
            changed ? "creative_world_layout_ready"
                    : "creative_world_layout_no_change",
            true);
}

}  // namespace iggy3d::creative::world_layout_compile
