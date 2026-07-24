#include "EditorPattern.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"
#include "core/math/OrientedBox.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] std::vector<cr::CreativeObjectId>
selectedHierarchyObjectIds(const cr::CreativeAppState& appState) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const std::span<const cr::TargetRef> targets =
      cr::selectedTargetList(selection);
  std::vector<cr::CreativeObjectId> selected;
  selected.reserve(targets.empty() ? 1U : targets.size());
  for (cr::TargetRef target : targets) {
    if (target.value != cr::kInvalidId) {
      selected.push_back(static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (selected.empty() && selection.selectedTarget.value != cr::kInvalidId) {
    selected.push_back(static_cast<cr::CreativeObjectId>(
        selection.selectedTarget.value));
  }
  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(appState.facade.document(), selected);
  return hierarchy.accepted ? hierarchy.objectIds : selected;
}

[[nodiscard]] std::vector<cr::CreativeObjectId> patternSourceObjectIds(
    const cr::CreativeAppState& appState,
    cr::CreativePatternRecipeKind kind) {
  const cr::CreativePatternRecipe* recipe =
      creativeEditorSelectedPatternRecipe(appState);
  return recipe != nullptr && recipe->kind == kind
             ? recipe->sourceObjectIds
             : selectedHierarchyObjectIds(appState);
}

[[nodiscard]] cr::CreativePatternRecipe prospectivePatternRecipe(
    const cr::CreativeAppState& appState,
    cr::CreativePatternRecipeKind kind) {
  const cr::CreativePatternRecipe* existing =
      creativeEditorSelectedPatternRecipe(appState);
  if (existing != nullptr && existing->kind == kind) {
    cr::CreativePatternRecipe result = *existing;
    result.generatedObjectIds.clear();
    return result;
  }
  cr::CreativePatternRecipe result;
  result.id = appState.facade.document().patternRecipeStore().nextRecipeId;
  result.kind = kind;
  result.sourceObjectIds = patternSourceObjectIds(appState, kind);
  return result;
}

[[nodiscard]] std::optional<cr::CreativeAuthoringOperationRecord>
patternOperationRecord(const cr::CreativePatternRecipe& recipe,
                       cr::CreativeAuthoringOperationKind kind,
                       std::string_view action,
                       cr::CreativeAuthoringFamily family =
                           cr::CreativeAuthoringFamily::Pattern) {
  return cr::makeCreativeAuthoringOperationRecord(
      family, kind, action,
      cr::fingerprintCreativePatternRecipeSource(recipe), 0U);
}

[[nodiscard]] iggy3d::Vec3 rotateAroundPivot(
    iggy3d::Vec3 point,
    iggy3d::Vec3 pivot,
    cr::CreativeAxis3 axis,
    double radians) noexcept {
  const iggy3d::Vec3 local = point - pivot;
  const cr::CreativeVec3 rotated = cr::rotateCreativeVectorAxisAngle(
      {static_cast<double>(local.x), static_cast<double>(local.y),
       static_cast<double>(local.z)},
      axis, radians);
  return pivot + iggy3d::Vec3{static_cast<float>(rotated.x),
                              static_cast<float>(rotated.y),
                              static_cast<float>(rotated.z)};
}

[[nodiscard]] VisualBounds rotateVisualBounds(
    VisualBounds bounds,
    iggy3d::Vec3 pivot,
    cr::CreativeAxis3 axis,
    double radians) noexcept {
  VisualBounds output;
  output.min = {std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  output.max = {std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()};
  for (std::size_t cornerIndex = 0; cornerIndex < 8U; ++cornerIndex) {
    const iggy3d::Vec3 corner{
        (cornerIndex & 1U) != 0U ? bounds.max.x : bounds.min.x,
        (cornerIndex & 2U) != 0U ? bounds.max.y : bounds.min.y,
        (cornerIndex & 4U) != 0U ? bounds.max.z : bounds.min.z,
    };
    const iggy3d::Vec3 rotated =
        rotateAroundPivot(corner, pivot, axis, radians);
    output.min.x = std::min(output.min.x, rotated.x);
    output.min.y = std::min(output.min.y, rotated.y);
    output.min.z = std::min(output.min.z, rotated.z);
    output.max.x = std::max(output.max.x, rotated.x);
    output.max.y = std::max(output.max.y, rotated.y);
    output.max.z = std::max(output.max.z, rotated.z);
  }
  return output;
}

[[nodiscard]] VisualBounds rotateObjectVisualBounds(
    const cr::CreativeObject& object,
    iggy3d::Vec3 pivot,
    cr::CreativeAxis3 axis,
    double radians) noexcept {
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeObjectBounds(object);
  if (!resolved.valid) {
    return rotateVisualBounds(visualBoundsForObject(object), pivot, axis,
                              radians);
  }
  VisualBounds output;
  output.min = {std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  output.max = {std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()};
  for (cr::CreativeVec3 corner : resolved.corners) {
    const cr::CreativeCoreVec3Conversion core =
        cr::creativeVec3ToCoreChecked(corner);
    if (!core.converted) {
      return rotateVisualBounds(visualBoundsForObject(object), pivot, axis,
                                radians);
    }
    const iggy3d::Vec3 rotated =
        rotateAroundPivot(core.value, pivot, axis, radians);
    output.min.x = std::min(output.min.x, rotated.x);
    output.min.y = std::min(output.min.y, rotated.y);
    output.min.z = std::min(output.min.z, rotated.z);
    output.max.x = std::max(output.max.x, rotated.x);
    output.max.y = std::max(output.max.y, rotated.y);
    output.max.z = std::max(output.max.z, rotated.z);
  }
  return output;
}

constexpr float kPatternCollisionEpsilonMeters = 1.0e-5F;

struct PatternPreviewCollisionBox {
  iggy3d::OrientedBox oriented{};
  iggy3d::Aabb3 worldAabb{};
  bool valid = false;
  bool blocksPlacement = false;
};

[[nodiscard]] cr::CreativeBounds creativeBounds(VisualBounds bounds) noexcept {
  return {{static_cast<double>(bounds.min.x),
           static_cast<double>(bounds.min.y),
           static_cast<double>(bounds.min.z)},
          {static_cast<double>(bounds.max.x),
           static_cast<double>(bounds.max.y),
           static_cast<double>(bounds.max.z)}};
}

void includePatternBounds(CreativeEditorPatternPreviewReceipt& receipt,
                          cr::CreativeBounds bounds) noexcept {
  const cr::CreativeBoundsMetrics metrics = cr::measureCreativeBounds(bounds);
  if (!metrics.valid) {
    return;
  }
  if (!receipt.hasFinalBounds) {
    receipt.finalBounds = bounds;
    receipt.hasFinalBounds = true;
    return;
  }
  receipt.finalBounds.min.x =
      std::min(receipt.finalBounds.min.x, bounds.min.x);
  receipt.finalBounds.min.y =
      std::min(receipt.finalBounds.min.y, bounds.min.y);
  receipt.finalBounds.min.z =
      std::min(receipt.finalBounds.min.z, bounds.min.z);
  receipt.finalBounds.max.x =
      std::max(receipt.finalBounds.max.x, bounds.max.x);
  receipt.finalBounds.max.y =
      std::max(receipt.finalBounds.max.y, bounds.max.y);
  receipt.finalBounds.max.z =
      std::max(receipt.finalBounds.max.z, bounds.max.z);
}

[[nodiscard]] PatternPreviewCollisionBox patternCollisionBox(
    cr::CreativeVec3 center,
    cr::CreativeVec3 size,
    cr::CreativeVec3 rotation,
    bool blocksPlacement) noexcept {
  PatternPreviewCollisionBox result;
  result.blocksPlacement = blocksPlacement;
  if (!blocksPlacement) {
    return result;
  }
  const cr::CreativeCoreVec3Conversion coreCenter =
      cr::creativeVec3ToCoreChecked(center);
  const cr::CreativeCoreVec3Conversion coreSize =
      cr::creativeVec3ToCoreChecked(size);
  const cr::CreativeCoreVec3Conversion coreRotation =
      cr::creativeVec3ToCoreChecked(rotation);
  if (!coreCenter.converted || !coreSize.converted ||
      !coreRotation.converted || !cr::isPositiveCreativeVec3(size)) {
    return result;
  }
  const iggy3d::Vec3 half = coreSize.value * 0.5F;
  result.oriented = iggy3d::makeOrientedBox(
      {coreCenter.value, coreRotation.value, {1.0F, 1.0F, 1.0F}},
      iggy3d::makeAabb3(half * -1.0F, half));
  result.worldAabb = iggy3d::orientedBoxWorldAabb(result.oriented);
  result.valid = iggy3d::isFinite(result.oriented.transform) &&
                 iggy3d::isValid(result.oriented.localBounds) &&
                 iggy3d::isValid(result.worldAabb);
  return result;
}

[[nodiscard]] PatternPreviewCollisionBox objectCollisionBox(
    const cr::CreativeObject& object) noexcept {
  const bool blocks = creativeObjectBlocksPlacementClearance(object);
  if (!blocks) {
    PatternPreviewCollisionBox result;
    result.blocksPlacement = false;
    return result;
  }
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeObjectBounds(object);
  if (!resolved.valid) {
    PatternPreviewCollisionBox result;
    result.blocksPlacement = true;
    return result;
  }
  return patternCollisionBox(resolved.center, resolved.size,
                             resolved.rotationEulerRadians, true);
}

[[nodiscard]] cr::CreativeVec3 add(cr::CreativeVec3 lhs,
                                   cr::CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] PatternPreviewCollisionBox transformedCollisionBox(
    const cr::CreativeObject& object,
    cr::CreativePatternRecipeKind kind,
    cr::CreativeVec3 linearOffset,
    cr::CreativeVec3 pivot,
    cr::CreativeAxis3 radialAxis,
    double radialAngle) noexcept {
  const bool blocks = creativeObjectBlocksPlacementClearance(object);
  if (!blocks) {
    PatternPreviewCollisionBox result;
    result.blocksPlacement = false;
    return result;
  }
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeObjectBounds(object);
  if (!resolved.valid) {
    PatternPreviewCollisionBox result;
    result.blocksPlacement = true;
    return result;
  }
  if (kind == cr::CreativePatternRecipeKind::LinearArray) {
    return patternCollisionBox(add(resolved.center, linearOffset),
                               resolved.size,
                               resolved.rotationEulerRadians, true);
  }
  if (kind != cr::CreativePatternRecipeKind::RadialArray) {
    return {};
  }
  if (!cr::objectHasTransform(object.kind)) {
    const cr::CreativeBounds rotated = creativeBounds(
        rotateObjectVisualBounds(object,
                                 cr::creativeVec3ToCoreChecked(pivot).value,
                                 radialAxis, radialAngle));
    const cr::CreativeBoundsMetrics metrics =
        cr::measureCreativeBounds(rotated);
    return metrics.valid
               ? patternCollisionBox(metrics.center, metrics.size, {}, true)
               : PatternPreviewCollisionBox{};
  }
  const cr::CreativeVec3 center = add(
      pivot, cr::rotateCreativeVectorAxisAngle(
                 subtract(resolved.center, pivot), radialAxis, radialAngle));
  const cr::CreativeVec3 rotation = cr::composeCreativeWorldAxisRotation(
      resolved.rotationEulerRadians, radialAxis, radialAngle);
  return patternCollisionBox(center, resolved.size, rotation, true);
}

[[nodiscard]] bool previewBoxesOverlap(
    const PatternPreviewCollisionBox& lhs,
    const PatternPreviewCollisionBox& rhs) noexcept {
  return lhs.valid && rhs.valid && lhs.blocksPlacement &&
         rhs.blocksPlacement &&
         iggy3d::intersects(lhs.worldAabb, rhs.worldAabb) &&
         iggy3d::strictlyOverlaps(lhs.oriented, rhs.oriented,
                                  kPatternCollisionEpsilonMeters);
}

[[nodiscard]] bool ignoredPatternObject(
    std::span<const cr::CreativeObjectId> sortedIds,
    cr::CreativeObjectId objectId) noexcept {
  return std::binary_search(sortedIds.begin(), sortedIds.end(), objectId);
}

[[nodiscard]] bool evaluatePatternPreviewCollisions(
    CreativeEditorPatternPreviewReceipt& receipt,
    std::span<const PatternPreviewCollisionBox> collisionBoxes,
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> ignoredObjectIds,
    const CreativePlacementClearanceCache* cache) {
  const bool useCache = cache != nullptr && cache->valid && cache->complete &&
                        cache->documentId == document.id() &&
                        cache->documentRevision == document.revision();
  bool valid = true;
  const auto testDocumentObject =
      [&](std::size_t candidateIndex,
          cr::CreativeObjectId objectId) {
        if (ignoredPatternObject(ignoredObjectIds, objectId)) {
          return;
        }
        const cr::CreativeObject* object = document.findObject(objectId);
        if (object == nullptr) {
          valid = false;
          return;
        }
        if (!cr::creativeObjectEffectivelyVisible(document, objectId) ||
            !creativeObjectBlocksPlacementClearance(*object)) {
          return;
        }
        ++receipt.testedDocumentObjectCount;
        const PatternPreviewCollisionBox obstacle =
            objectCollisionBox(*object);
        if (!obstacle.valid) {
          valid = false;
          return;
        }
        if (!previewBoxesOverlap(collisionBoxes[candidateIndex], obstacle)) {
          return;
        }
        receipt.objects[candidateIndex].colliding = true;
        if (receipt.blockingObjectId == cr::kInvalidObjectId) {
          receipt.blockingObjectId = objectId;
        }
      };

  for (std::size_t index = 0; index < receipt.objectCount; ++index) {
    const PatternPreviewCollisionBox& candidate = collisionBoxes[index];
    if (!candidate.blocksPlacement) {
      continue;
    }
    if (!candidate.valid) {
      return false;
    }
    bool queried = false;
    if (useCache) {
      try {
        const iggy3d::AabbGridQueryResult query =
            cache->authoredObstacleIndex.queryChecked(candidate.worldAabb);
        if (query.queried()) {
          queried = true;
          for (std::uint64_t objectId : query.candidates) {
            testDocumentObject(index,
                               static_cast<cr::CreativeObjectId>(objectId));
          }
        }
      } catch (...) {
        queried = false;
      }
    }
    if (!queried) {
      for (const cr::CreativeObject& object : document.objects()) {
        testDocumentObject(index, object.id);
      }
    }
    if (!valid) {
      return false;
    }
  }

  for (std::size_t lhs = 0; lhs < receipt.objectCount; ++lhs) {
    for (std::size_t rhs = lhs + 1U; rhs < receipt.objectCount; ++rhs) {
      if (receipt.objects[lhs].instanceOrdinal ==
          receipt.objects[rhs].instanceOrdinal) {
        continue;
      }
      ++receipt.testedGeneratedPairCount;
      if (previewBoxesOverlap(collisionBoxes[lhs], collisionBoxes[rhs])) {
        receipt.objects[lhs].colliding = true;
        receipt.objects[rhs].colliding = true;
      }
    }
  }
  receipt.collidingObjectCount = static_cast<std::uint64_t>(std::count_if(
      receipt.objects.begin(), receipt.objects.begin() + receipt.objectCount,
      [](const CreativeEditorPatternPreviewObject& object) {
        return object.colliding;
      }));
  return true;
}

[[nodiscard]] bool patternOptionsDraftActive(
    const CreativeEditorState& editor) noexcept {
  return editor.toolOptions.open &&
         editor.toolOptions.targetEntry.kind ==
             cr::CreativeHeldItemKind::LinearArray;
}

[[nodiscard]] const cr::CreativeToolSettings& patternPreviewSettings(
    const CreativeEditorState& editor) noexcept {
  return patternOptionsDraftActive(editor) ? editor.toolOptions.draft
                                           : editor.toolSettings;
}

[[nodiscard]] double patternPreviewCellSize(
    const CreativeEditorState& editor) noexcept {
  return patternOptionsDraftActive(editor)
             ? editor.toolOptions.placeCellSizeDraft
             : editor.placeCellSize;
}

[[nodiscard]] CreativeEditorPatternPreviewReceipt buildPatternPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    cr::CreativePatternRecipeKind kind,
    const CreativePlacementClearanceCache* clearanceCache) {
  CreativeEditorPatternPreviewReceipt receipt;
  receipt.requested = true;
  receipt.kind = kind;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::LinearArray) {
    receipt.status = CreativeEditorPatternPreviewStatus::ToolInactive;
    receipt.reasonCode = "creative_pattern_preview_tool_inactive";
    return receipt;
  }

  const cr::CreativePatternRecipe* selectedRecipe =
      creativeEditorSelectedPatternRecipe(appState);
  const cr::CreativePatternRecipe* editedRecipe =
      selectedRecipe != nullptr && selectedRecipe->kind == kind
          ? selectedRecipe
          : nullptr;
  const std::vector<cr::CreativeObjectId> sources =
      patternSourceObjectIds(appState, kind);
  receipt.sourceObjectCount = sources.size();
  if (sources.empty()) {
    receipt.status = CreativeEditorPatternPreviewStatus::EmptySource;
    receipt.reasonCode = "creative_pattern_preview_source_empty";
    return receipt;
  }
  for (cr::CreativeObjectId sourceId : sources) {
    const cr::CreativeObject* source = appState.facade.findObject(sourceId);
    if (source == nullptr) {
      receipt.status = CreativeEditorPatternPreviewStatus::MissingSource;
      receipt.reasonCode = "creative_pattern_preview_source_missing";
      return receipt;
    }
    includePatternBounds(receipt,
                         creativeBounds(visualBoundsForObject(*source)));
  }

  std::array<PatternPreviewCollisionBox,
             cr::kCreativeLinearArrayGeneratedObjectCapacity>
      collisionBoxes{};
  const auto appendCandidate =
      [&](const cr::CreativeObject& source, std::uint32_t ordinal,
          VisualBounds worldBounds, PatternPreviewCollisionBox collision) {
        if (receipt.objectCount >= receipt.objects.size()) {
          return false;
        }
        CreativeEditorPatternPreviewObject& output =
            receipt.objects[receipt.objectCount];
        output.sourceObjectId = source.id;
        output.instanceOrdinal = ordinal;
        output.worldBounds = creativeBounds(worldBounds);
        collisionBoxes[receipt.objectCount] = collision;
        includePatternBounds(receipt, output.worldBounds);
        ++receipt.objectCount;
        return true;
      };

  bool degeneratePivot = false;
  const cr::CreativeToolSettings& settings = patternPreviewSettings(editor);
  if (kind == cr::CreativePatternRecipeKind::LinearArray) {
    receipt.linear = creativeEditorLinearArrayRequest(
        settings, patternPreviewCellSize(editor));
    cr::CreativeLinearArrayPlanRequest request;
    request.sourceObjectCount = sources.size();
    request.direction = receipt.linear.direction;
    request.copyCount = receipt.linear.copyCount;
    request.spacing = receipt.linear.spacing;
    request.cellSize = receipt.linear.cellSize;
    request.maxGeneratedObjects = receipt.linear.maxGeneratedObjects;
    const cr::CreativeLinearArrayPlanReceipt plan =
        cr::planCreativeLinearArray(request);
    if (!plan.accepted) {
      receipt.status = CreativeEditorPatternPreviewStatus::PlanRejected;
      receipt.reasonCode = plan.reasonCode;
      return receipt;
    }
    receipt.generatedObjectCount = plan.generatedObjectCount;
    for (const cr::CreativeLinearArrayInstance& instance :
         plan.plannedInstances()) {
      const cr::CreativeCoreVec3Conversion coreOffset =
          cr::creativeVec3ToCoreChecked(instance.offset);
      if (!coreOffset.converted) {
        receipt.status = CreativeEditorPatternPreviewStatus::InvalidGeometry;
        receipt.reasonCode = "creative_pattern_preview_offset_invalid";
        return receipt;
      }
      for (cr::CreativeObjectId sourceId : sources) {
        const cr::CreativeObject& source = *appState.facade.findObject(sourceId);
        VisualBounds worldBounds = visualBoundsForObject(source);
        worldBounds.min = worldBounds.min + coreOffset.value;
        worldBounds.max = worldBounds.max + coreOffset.value;
        if (!appendCandidate(
                source, instance.ordinal, worldBounds,
                transformedCollisionBox(
                    source, kind, instance.offset, {}, cr::CreativeAxis3::Y,
                    0.0))) {
          receipt.status = CreativeEditorPatternPreviewStatus::PlanRejected;
          receipt.reasonCode = "creative_pattern_preview_capacity_exceeded";
          return receipt;
        }
      }
    }
  } else if (kind == cr::CreativePatternRecipeKind::RadialArray) {
    const bool editing = editedRecipe != nullptr;
    if (!editing && !editor.interaction.target.grid.valid) {
      receipt.status = CreativeEditorPatternPreviewStatus::PivotUnavailable;
      receipt.reasonCode = "creative_pattern_preview_pivot_unavailable";
      return receipt;
    }
    const cr::CreativeVec3 pivot =
        editing ? editedRecipe->radial.pivot
                : editor.interaction.target.grid.placementAnchor;
    receipt.radial = creativeEditorRadialArrayRequest(settings, pivot);
    cr::CreativeRadialArrayPlanRequest request;
    request.sourceObjectCount = sources.size();
    request.pivot = receipt.radial.pivot;
    request.axis = receipt.radial.axis;
    request.instanceCount = receipt.radial.instanceCount;
    request.sweep = receipt.radial.sweep;
    request.maxGeneratedObjects = receipt.radial.maxGeneratedObjects;
    const cr::CreativeRadialArrayPlanReceipt plan =
        cr::planCreativeRadialArray(request);
    if (!plan.accepted) {
      receipt.status = CreativeEditorPatternPreviewStatus::PlanRejected;
      receipt.reasonCode = plan.reasonCode;
      return receipt;
    }
    receipt.generatedObjectCount = plan.generatedObjectCount;
    const cr::CreativeBoundsMetrics sourceBounds =
        cr::measureCreativeBounds(receipt.finalBounds);
    const cr::CreativeVec3 sourceAnchor{
        sourceBounds.center.x, receipt.finalBounds.min.y,
        sourceBounds.center.z};
    degeneratePivot =
        cr::creativeSquaredDistanceFromAxis(sourceAnchor, pivot,
                                            receipt.radial.axis) <= 1.0e-12;
    const cr::CreativeCoreVec3Conversion corePivot =
        cr::creativeVec3ToCoreChecked(pivot);
    if (!corePivot.converted) {
      receipt.status = CreativeEditorPatternPreviewStatus::InvalidGeometry;
      receipt.reasonCode = "creative_pattern_preview_pivot_invalid";
      return receipt;
    }
    for (const cr::CreativeRadialArrayInstance& instance :
         plan.plannedInstances()) {
      for (cr::CreativeObjectId sourceId : sources) {
        const cr::CreativeObject& source = *appState.facade.findObject(sourceId);
        const VisualBounds worldBounds = rotateObjectVisualBounds(
            source, corePivot.value, receipt.radial.axis,
            instance.angleRadians);
        if (!appendCandidate(
                source, instance.ordinal, worldBounds,
                transformedCollisionBox(
                    source, kind, {}, pivot, receipt.radial.axis,
                    instance.angleRadians))) {
          receipt.status = CreativeEditorPatternPreviewStatus::PlanRejected;
          receipt.reasonCode = "creative_pattern_preview_capacity_exceeded";
          return receipt;
        }
      }
    }
  } else {
    receipt.status = CreativeEditorPatternPreviewStatus::PlanRejected;
    receipt.reasonCode = "creative_pattern_preview_kind_invalid";
    return receipt;
  }

  std::vector<cr::CreativeObjectId> ignoredObjectIds;
  if (editedRecipe != nullptr) {
    ignoredObjectIds = editedRecipe->generatedObjectIds;
    std::sort(ignoredObjectIds.begin(), ignoredObjectIds.end());
  }
  if (degeneratePivot) {
    for (std::size_t index = 0; index < receipt.objectCount; ++index) {
      receipt.objects[index].colliding = true;
    }
    receipt.collidingObjectCount = receipt.objectCount;
    receipt.status = CreativeEditorPatternPreviewStatus::DegeneratePivot;
    receipt.reasonCode = "creative_pattern_preview_radius_degenerate";
    return receipt;
  }
  if (!evaluatePatternPreviewCollisions(
          receipt,
          std::span<const PatternPreviewCollisionBox>{collisionBoxes.data(),
                                                      receipt.objectCount},
          appState.facade.document(), ignoredObjectIds, clearanceCache)) {
    receipt.status = CreativeEditorPatternPreviewStatus::InvalidGeometry;
    receipt.reasonCode = "creative_pattern_preview_geometry_invalid";
    return receipt;
  }
  receipt.accepted = true;
  receipt.collisionFree = receipt.collidingObjectCount == 0U;
  receipt.status = receipt.collisionFree
                       ? CreativeEditorPatternPreviewStatus::Ready
                       : CreativeEditorPatternPreviewStatus::Collision;
  receipt.reasonCode = receipt.collisionFree
                           ? "creative_pattern_preview_ready"
                           : "creative_pattern_preview_collision";
  return receipt;
}

std::size_t appendPatternPreview(
    const CreativeEditorPatternPreviewReceipt& preview,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const std::size_t before = wireLines.size();
  for (const CreativeEditorPatternPreviewObject& object :
       preview.generatedObjects()) {
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(object.worldBounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(object.worldBounds.max);
    if (!minimum.converted || !maximum.converted) {
      continue;
    }
    const iggy3d::RenderLineColor color =
        object.colliding
            ? iggy3d::RenderLineColor{1.0F, 0.18F, 0.14F, 0.95F}
            : iggy3d::RenderLineColor{0.22F, 0.88F, 1.0F, 0.90F};
    appendStandaloneWireframeBoxEdges(
        wireLines, minimum.value, maximum.value, color,
        std::max(0.025F, wireThickness * 0.8F));
  }
  if (preview.kind == cr::CreativePatternRecipeKind::RadialArray &&
      cr::isFiniteCreativeVec3(preview.radial.pivot)) {
    const cr::CreativeCoreVec3Conversion pivot =
        cr::creativeVec3ToCoreChecked(preview.radial.pivot);
    if (pivot.converted) {
      const float halfExtent = std::max(0.06F, wireThickness * 1.5F);
      appendStandaloneWireframeBoxEdges(
          wireLines,
          pivot.value - iggy3d::Vec3{halfExtent, halfExtent, halfExtent},
          pivot.value + iggy3d::Vec3{halfExtent, halfExtent, halfExtent},
          {0.96F, 0.74F, 0.18F, 1.0F},
          std::max(0.03F, wireThickness));
    }
  }
  return wireLines.size() - before;
}

}  // namespace

const cr::CreativePatternRecipe* creativeEditorSelectedPatternRecipe(
    const cr::CreativeAppState& appState) noexcept {
  const cr::TargetRef selected =
      appState.facade.selectionState().selectedTarget;
  if (selected.value == cr::kInvalidId) {
    return nullptr;
  }
  const cr::CreativeSemanticSelectionResolution resolved =
      cr::resolveCreativeSemanticSelection(
          appState.facade.document(),
          static_cast<cr::CreativeObjectId>(selected.value));
  return resolved.accepted &&
                 resolved.patternRecipeId !=
                     cr::kInvalidCreativePatternRecipeId
             ? cr::findCreativePatternRecipe(
                   appState.facade.document().patternRecipeStore(),
                   resolved.patternRecipeId)
             : nullptr;
}

bool loadCreativeEditorPatternRecipeSettings(
    const cr::CreativePatternRecipe& recipe,
    cr::CreativeToolSettings& settings,
    double& cellSize) noexcept {
  switch (recipe.kind) {
    case cr::CreativePatternRecipeKind::LinearArray:
      settings.arrayMode = cr::CreativeArrayMode::Linear;
      settings.arrayDirection = recipe.linear.direction;
      settings.arrayCopyCount = recipe.linear.copyCount;
      settings.arraySpacing = recipe.linear.spacing;
      cellSize = recipe.linear.cellSize;
      break;
    case cr::CreativePatternRecipeKind::RadialArray:
      settings.arrayMode = cr::CreativeArrayMode::Radial;
      settings.radialArrayAxis = recipe.radial.axis;
      settings.radialArrayInstanceCount = recipe.radial.instanceCount;
      settings.radialArraySweep = recipe.radial.sweep;
      break;
    case cr::CreativePatternRecipeKind::AssetScatter:
      return false;
    case cr::CreativePatternRecipeKind::Count:
      return false;
  }
  return cr::isValidCreativeToolSettings(settings) &&
         std::isfinite(cellSize) && cellSize > 0.0;
}

cr::CreativeLinearArrayRequest creativeEditorLinearArrayRequest(
    const cr::CreativeToolSettings& settings,
    double cellSize) noexcept {
  cr::CreativeLinearArrayRequest request;
  request.direction = settings.arrayDirection;
  request.copyCount = settings.arrayCopyCount;
  request.spacing = settings.arraySpacing;
  request.cellSize = cellSize;
  return request;
}

cr::CreativeRadialArrayRequest creativeEditorRadialArrayRequest(
    const cr::CreativeToolSettings& settings,
    cr::CreativeVec3 pivot) noexcept {
  cr::CreativeRadialArrayRequest request;
  request.pivot = pivot;
  request.axis = settings.radialArrayAxis;
  request.instanceCount = settings.radialArrayInstanceCount;
  request.sweep = settings.radialArraySweep;
  return request;
}

cr::CreativeLinearArrayReceipt applyCreativeEditorLinearArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    double cellSize,
    std::string_view source) {
  const cr::CreativePatternRecipe* recipe =
      creativeEditorSelectedPatternRecipe(appState);
  const cr::CreativeLinearArrayRequest request =
      creativeEditorLinearArrayRequest(settings, cellSize);
  const bool updating =
      recipe != nullptr &&
      recipe->kind == cr::CreativePatternRecipeKind::LinearArray;
  cr::CreativePatternRecipe prospective = prospectivePatternRecipe(
      appState, cr::CreativePatternRecipeKind::LinearArray);
  prospective.linear = request;
  std::optional<cr::CreativeAuthoringOperationRecord> operation =
      patternOperationRecord(prospective,
                             updating
                                 ? cr::CreativeAuthoringOperationKind::Reconcile
                                 : cr::CreativeAuthoringOperationKind::Apply,
                             updating ? "LinearArray.Update"
                                      : "LinearArray.Create");
  if (!operation.has_value()) {
    state.lastReceipt = {};
    state.lastReceipt.requested = true;
    state.lastReceipt.status = cr::CreativeLinearArrayStatus::InvalidRequest;
    state.lastReceipt.message =
        "creative_linear_array_operation_record_invalid";
    return state.lastReceipt;
  }
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source, std::move(*operation));
  state.lastReceipt =
      updating
          ? appState.facade.updateLinearArrayRecipe(recipe->id, request)
          : appState.facade.createLinearArrayFromSelection(request);
  if (transaction.operation.has_value()) {
    transaction.operation->affectedMemberCount =
        state.lastReceipt.generatedObjectCount +
        state.lastReceipt.replacedGeneratedObjectCount;
  }
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastReceipt.accepted && state.lastReceipt.changed,
      state.lastReceipt.message));

  SDL_Log("iggy3d_creative: LINEAR_ARRAY status='%s' accepted=%d changed=%d "
          "sourceObjects=%llu generatedObjects=%llu direction='%s' copies='%s' "
          "spacing='%s' reasonCode='%s'",
          std::string(cr::toString(state.lastReceipt.status)).c_str(),
          state.lastReceipt.accepted ? 1 : 0,
          state.lastReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(state.lastReceipt.sourceObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.generatedObjectCount),
          std::string(cr::toString(settings.arrayDirection)).c_str(),
          std::string(cr::toString(settings.arrayCopyCount)).c_str(),
          std::string(cr::toString(settings.arraySpacing)).c_str(),
          state.lastReceipt.message.c_str());
  return state.lastReceipt;
}

cr::CreativeRadialArrayReceipt applyCreativeEditorRadialArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    cr::CreativeVec3 pivot,
    std::string_view source) {
  const cr::CreativePatternRecipe* recipe =
      creativeEditorSelectedPatternRecipe(appState);
  const bool updating =
      recipe != nullptr &&
      recipe->kind == cr::CreativePatternRecipeKind::RadialArray;
  const cr::CreativeRadialArrayRequest request =
      creativeEditorRadialArrayRequest(
          settings,
          updating ? recipe->radial.pivot : pivot);
  cr::CreativePatternRecipe prospective = prospectivePatternRecipe(
      appState, cr::CreativePatternRecipeKind::RadialArray);
  prospective.radial = request;
  std::optional<cr::CreativeAuthoringOperationRecord> operation =
      patternOperationRecord(prospective,
                             updating
                                 ? cr::CreativeAuthoringOperationKind::Reconcile
                                 : cr::CreativeAuthoringOperationKind::Apply,
                             updating ? "RadialArray.Update"
                                      : "RadialArray.Create");
  if (!operation.has_value()) {
    state.lastRadialReceipt = {};
    state.lastRadialReceipt.requested = true;
    state.lastRadialReceipt.status =
        cr::CreativeRadialArrayStatus::InvalidRequest;
    state.lastRadialReceipt.message =
        "creative_radial_array_operation_record_invalid";
    return state.lastRadialReceipt;
  }
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source, std::move(*operation));
  state.lastRadialReceipt =
      updating
          ? appState.facade.updateRadialArrayRecipe(recipe->id, request)
          : appState.facade.createRadialArrayFromSelection(request);
  if (transaction.operation.has_value()) {
    transaction.operation->affectedMemberCount =
        state.lastRadialReceipt.generatedObjectCount +
        state.lastRadialReceipt.replacedGeneratedObjectCount;
  }
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastRadialReceipt.accepted && state.lastRadialReceipt.changed,
      state.lastRadialReceipt.message));

  SDL_Log("iggy3d_creative: RADIAL_ARRAY status='%s' accepted=%d changed=%d "
          "sourceObjects=%llu generatedObjects=%llu axis='%s' instances='%s' "
          "sweep='%s' pivot=(%.3f,%.3f,%.3f) reasonCode='%s'",
          std::string(cr::toString(state.lastRadialReceipt.status)).c_str(),
          state.lastRadialReceipt.accepted ? 1 : 0,
          state.lastRadialReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(
              state.lastRadialReceipt.sourceObjectCount),
          static_cast<unsigned long long>(
              state.lastRadialReceipt.generatedObjectCount),
          std::string(cr::toString(settings.radialArrayAxis)).c_str(),
          std::string(cr::toString(settings.radialArrayInstanceCount)).c_str(),
          std::string(cr::toString(settings.radialArraySweep)).c_str(), pivot.x,
          pivot.y, pivot.z, state.lastRadialReceipt.message.c_str());
  return state.lastRadialReceipt;
}

bool applyCreativeEditorArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    double cellSize,
    bool pivotValid,
    cr::CreativeVec3 pivot,
    std::string_view source) {
  switch (settings.arrayMode) {
    case cr::CreativeArrayMode::Linear:
      return applyCreativeEditorLinearArrayWithHistory(
                 appState, state, settings, cellSize, source)
          .accepted;
    case cr::CreativeArrayMode::Radial:
      if ((!pivotValid || !cr::isFiniteCreativeVec3(pivot)) &&
          (creativeEditorSelectedPatternRecipe(appState) == nullptr ||
           creativeEditorSelectedPatternRecipe(appState)->kind !=
               cr::CreativePatternRecipeKind::RadialArray)) {
        state.lastRadialReceipt = {};
        state.lastRadialReceipt.requested = true;
        state.lastRadialReceipt.status =
            cr::CreativeRadialArrayStatus::InvalidRequest;
        state.lastRadialReceipt.message =
            "creative_radial_array_pivot_unavailable";
        return false;
      }
      return applyCreativeEditorRadialArrayWithHistory(
                 appState, state, settings, pivot, source)
          .accepted;
    case cr::CreativeArrayMode::Count:
      return false;
  }
  return false;
}

cr::CreativePatternRecipeMutationReceipt
detachCreativeEditorPatternRecipeWithHistory(
    cr::CreativeAppState& appState,
    cr::CreativePatternRecipeId recipeId,
    std::string_view source) {
  const cr::CreativePatternRecipe* recipe = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), recipeId);
  std::optional<cr::CreativeAuthoringOperationRecord> operation =
      recipe != nullptr
          ? patternOperationRecord(
                *recipe,
                cr::CreativeAuthoringOperationKind::Destructive,
                recipe->kind == cr::CreativePatternRecipeKind::AssetScatter
                    ? "AssetScatter.Detach"
                    : "Pattern.Detach",
                recipe->kind == cr::CreativePatternRecipeKind::AssetScatter
                    ? cr::CreativeAuthoringFamily::AssetScatter
                    : cr::CreativeAuthoringFamily::Pattern)
          : std::nullopt;
  if (!operation.has_value()) {
    cr::CreativePatternRecipeMutationReceipt receipt;
    receipt.requested = true;
    receipt.kind = cr::CreativePatternRecipeMutationKind::Detach;
    receipt.status = cr::CreativePatternRecipeMutationStatus::InvalidRequest;
    receipt.recipeId = recipeId;
    receipt.reasonCode = "creative_pattern_operation_record_invalid";
    return receipt;
  }
  operation->affectedMemberCount = recipe->generatedObjectIds.size();
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source, std::move(*operation));
  cr::CreativePatternRecipeMutationReceipt receipt =
      appState.facade.detachPatternRecipe(recipeId);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  return receipt;
}

CreativeEditorPatternPreviewReceipt evaluateCreativeEditorArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    const CreativePlacementClearanceCache* clearanceCache) {
  switch (patternPreviewSettings(editor).arrayMode) {
    case cr::CreativeArrayMode::Linear:
      return buildPatternPreview(
          appState, editor, cr::CreativePatternRecipeKind::LinearArray,
          clearanceCache);
    case cr::CreativeArrayMode::Radial:
      return buildPatternPreview(
          appState, editor, cr::CreativePatternRecipeKind::RadialArray,
          clearanceCache);
    case cr::CreativeArrayMode::Count:
      break;
  }
  CreativeEditorPatternPreviewReceipt receipt;
  receipt.requested = true;
  receipt.status = CreativeEditorPatternPreviewStatus::PlanRejected;
  receipt.reasonCode = "creative_pattern_preview_mode_invalid";
  return receipt;
}

std::size_t appendCreativeEditorLinearArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    const CreativePlacementClearanceCache* clearanceCache) {
  return appendPatternPreview(
      buildPatternPreview(appState, editor,
                          cr::CreativePatternRecipeKind::LinearArray,
                          clearanceCache),
      wireThickness, wireLines);
}

std::size_t appendCreativeEditorRadialArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    const CreativePlacementClearanceCache* clearanceCache) {
  return appendPatternPreview(
      buildPatternPreview(appState, editor,
                          cr::CreativePatternRecipeKind::RadialArray,
                          clearanceCache),
      wireThickness, wireLines);
}

std::size_t appendCreativeEditorArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    const CreativePlacementClearanceCache* clearanceCache) {
  return appendPatternPreview(
      evaluateCreativeEditorArrayPreview(appState, editor, clearanceCache),
      wireThickness, wireLines);
}

}  // namespace iggy3d_creative_app
