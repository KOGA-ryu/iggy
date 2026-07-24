#include "EditorTransform.hpp"
#include "EditorTransformInternal.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "EditorWorldLayoutState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/tools/RecipeTransform.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace detail {

[[nodiscard]] bool moveSourceAvailable(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard) noexcept {
  const cr::CreativeDocument& document = appState.facade.document();
  if (clipboard.sourceDocumentId != document.id() ||
      clipboard.sourceRevision != document.revision() ||
      cr::creativeClipboardEmpty(clipboard)) {
    return false;
  }
  return std::all_of(
      clipboard.objects.begin(), clipboard.objects.end(),
      [&document](const cr::CreativeObject& object) {
        return document.containsObject(object.id);
      });
}

[[nodiscard]] const cr::CreativeObject* transformSourceObject(
    const CreativeEditorSelectionTransformState& state,
    cr::CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      state.sourceClipboard.objects.begin(), state.sourceClipboard.objects.end(),
      [objectId](const cr::CreativeObject& object) {
        return object.id == objectId;
      });
  return found == state.sourceClipboard.objects.end() ? nullptr : &*found;
}

}  // namespace detail

namespace {

[[nodiscard]] std::vector<cr::CreativeObjectId> clipboardObjectIds(
    const cr::CreativeClipboard& clipboard) {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(clipboard.objects.size());
  for (const cr::CreativeObject& object : clipboard.objects) {
    ids.push_back(object.id);
  }
  return ids;
}

[[nodiscard]] CreativeEditorTransformPreflight inspectTransformSource(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    cr::CreativeSelectionPlacementMode mode,
    const CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorTransformPreflight result;
  result.requested = true;
  if (cr::creativeClipboardEmpty(clipboard) || !clipboard.hasPlacementAnchor ||
      !cr::isFiniteCreativeVec3(clipboard.placementAnchor)) {
    result.reasonCode = "editor_transform_source_invalid";
    return result;
  }

  result.capabilities = cr::resolveCreativeSelectionPlacementCapabilities(
      clipboard.objects);
  if (!result.capabilities.resolved) {
    result.reasonCode = "editor_transform_capabilities_unresolved";
    return result;
  }
  const bool anyOperation =
      result.capabilities.translate || result.capabilities.mirror ||
      result.capabilities.rotation != cr::CreativeObjectRotationSupport::None ||
      result.capabilities.scale != cr::CreativeObjectScaleSupport::None;
  if (!anyOperation) {
    result.reasonCode = "editor_transform_source_not_transformable";
    return result;
  }

  const cr::CreativeDocument& document = appState.facade.document();
  if (mode == cr::CreativeSelectionPlacementMode::Move &&
      clipboard.sourceDocumentId == document.id() &&
      !clipboard.patternRecipes.empty()) {
    const cr::CreativeSemanticObjectActionPolicy transformPolicy =
        cr::resolveCreativeSemanticObjectAction(
            cr::CreativeSemanticSelectionOwner::PatternRecipe,
            cr::CreativeSemanticObjectAction::TransformSelection);
    if (!transformPolicy.allowed ||
        transformPolicy.route !=
            cr::CreativeSemanticObjectActionRoute::PatternRecipe) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.reasonCode = std::string{transformPolicy.reasonCode};
      return result;
    }
    if (clipboard.sourceRevision != document.revision() ||
        clipboard.patternRecipes.size() != 1U) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.reasonCode =
          "editor_transform_pattern_recipe_dependency_conflict";
      return result;
    }
    const cr::CreativePatternRecipeId recipeId =
        clipboard.patternRecipes.front().id;
    const cr::CreativePatternRecipeTranslationPlan patternPlan =
        cr::planCreativePatternRecipeTranslation(document, recipeId, {});
    if (!patternPlan.accepted) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.failedObjectId = patternPlan.failedObjectId;
      result.reasonCode = std::string{patternPlan.reasonCode};
      return result;
    }
    std::unordered_set<cr::CreativeObjectId> clipboardIds;
    clipboardIds.reserve(clipboard.objects.size());
    for (const cr::CreativeObject& object : clipboard.objects) {
      clipboardIds.insert(object.id);
    }
    if (clipboardIds.size() != patternPlan.memberObjectIds.size() ||
        !std::all_of(patternPlan.memberObjectIds.begin(),
                     patternPlan.memberObjectIds.end(),
                     [&clipboardIds](cr::CreativeObjectId objectId) {
                       return clipboardIds.contains(objectId);
                     })) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.reasonCode =
          "editor_transform_pattern_recipe_scope_incomplete";
      return result;
    }
    if (worldLayout != nullptr) {
      const cr::CreativePatternRecipe* liveRecipe =
          cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                        recipeId);
      if (liveRecipe == nullptr) {
        result.reasonCode = "editor_transform_pattern_recipe_missing";
        return result;
      }
      for (cr::CreativeObjectId sourceObjectId :
           liveRecipe->sourceObjectIds) {
        const cr::CreativeSemanticSelectionResolution semantic =
            cr::resolveCreativeSemanticSelection(
                document, sourceObjectId, &worldLayout->source);
        if (semantic.accepted &&
            semantic.worldLayoutSource.owned) {
          result.blockedOwner =
              cr::CreativeSemanticSelectionOwner::WorldLayoutSource;
          result.failedObjectId = sourceObjectId;
          result.reasonCode =
              "editor_transform_pattern_recipe_world_layout_source_owned";
          return result;
        }
      }
    }
    result.ownershipRoute =
        CreativeEditorTransformOwnershipRoute::PatternRecipe;
    result.patternRecipeId = recipeId;
    result.sourceDocumentRevision = document.revision();
    result.capabilities = {true, patternPlan.memberObjectIds.size(), true,
                           cr::CreativeObjectRotationSupport::None,
                           cr::CreativeObjectScaleSupport::None, false};
    result.accepted = true;
    result.reasonCode = "editor_transform_pattern_recipe_ready";
    return result;
  }

  std::unordered_set<cr::CreativeObjectId> sourceIds;
  sourceIds.reserve(clipboard.objects.size());
  for (const cr::CreativeObject& object : clipboard.objects) {
    sourceIds.insert(object.id);
    if (mode == cr::CreativeSelectionPlacementMode::Move) {
      const cr::CreativeObjectHierarchyState hierarchyState =
          cr::resolveCreativeObjectHierarchyState(document, object.id);
      if (!hierarchyState.resolved || hierarchyState.effectivelyLocked) {
        result.failedObjectId =
            hierarchyState.lockedByObjectId != cr::kInvalidObjectId
                ? hierarchyState.lockedByObjectId
                : object.id;
        result.reasonCode = "editor_transform_object_locked";
        return result;
      }
    }
  }
  if (mode == cr::CreativeSelectionPlacementMode::Move) {
    for (const cr::CreativeObject& object : clipboard.objects) {
      if (object.parentId.has_value() &&
          !sourceIds.contains(*object.parentId)) {
        result.failedObjectId = object.id;
        result.reasonCode = "editor_transform_external_parent";
        return result;
      }
    }
  }

  if (clipboard.sourceDocumentId == document.id()) {
    const std::vector<cr::CreativeObjectId> objectIds =
        clipboardObjectIds(clipboard);
    const cr::CreativeSemanticSelectionSetResolution semanticSet =
        cr::resolveCreativeSemanticSelectionSet(
            document, objectIds,
            objectIds.empty() ? cr::kInvalidObjectId : objectIds.front(),
            worldLayout != nullptr ? &worldLayout->source : nullptr);
    if (!semanticSet.accepted) {
      result.reasonCode = semanticSet.reasonCode;
      return result;
    }
    const bool onlyWorldLayoutOwned =
        semanticSet.worldLayoutOwnerCount > 0U &&
        semanticSet.authoredOwnerCount == 0U &&
        semanticSet.patternOwnerCount == 0U;
    if (onlyWorldLayoutOwned) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::WorldLayoutSource;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.worldLayoutSource = semanticSet.commonWorldLayoutSource;
      if (worldLayout == nullptr ||
          worldLayout->generatedRevision != worldLayout->revision) {
        result.reasonCode =
            "editor_transform_world_layout_source_unsynchronized";
        return result;
      }
      const cr::CreativeWorldLayoutSourceRef buildingSource =
          cr::resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
              document, objectIds, worldLayout->source);
      if (buildingSource.table ==
          cr::CreativeWorldLayoutTable::Building) {
        result.worldLayoutSource = buildingSource;
      }
      const cr::CreativeSemanticObjectActionPolicy transformPolicy =
          cr::resolveCreativeSemanticObjectAction(
              cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
              cr::CreativeSemanticObjectAction::TransformSelection,
              result.worldLayoutSource.table);
      if (!transformPolicy.allowed ||
          transformPolicy.route !=
              cr::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
        result.reasonCode = std::string{transformPolicy.reasonCode};
        return result;
      }
      if (result.worldLayoutSource.index >=
          worldLayout->source.buildings.size()) {
        result.reasonCode =
            "editor_transform_world_layout_building_scope_required";
        return result;
      }
      result.ownershipRoute =
          CreativeEditorTransformOwnershipRoute::WorldLayoutBuilding;
      result.capabilities = {true, clipboard.objects.size(), true,
                             cr::CreativeObjectRotationSupport::QuarterTurns,
                             cr::CreativeObjectScaleSupport::None, true};
      result.worldLayoutRevision = worldLayout->revision;
      result.worldLayoutSourceEpoch = worldLayout->sourceEpoch;
      result.accepted = true;
      result.reasonCode = "editor_transform_world_layout_building_ready";
      return result;
    }

    const cr::CreativeSemanticObjectActionPolicy transformPolicy =
        cr::resolveCreativeSemanticObjectAction(
            semanticSet,
            cr::CreativeSemanticObjectAction::TransformSelection);
    if (!transformPolicy.allowed) {
      result.blockedOwner = semanticSet.primaryOwner;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.reasonCode = std::string{transformPolicy.reasonCode};
      return result;
    }
    if (transformPolicy.route ==
        cr::CreativeSemanticObjectActionRoute::PatternRecipe) {
      result.blockedOwner =
          cr::CreativeSemanticSelectionOwner::PatternRecipe;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.reasonCode = "editor_transform_pattern_recipe_owned";
      return result;
    }
    if (transformPolicy.route !=
        cr::CreativeSemanticObjectActionRoute::Document) {
      result.blockedOwner = semanticSet.primaryOwner;
      result.failedObjectId = semanticSet.primaryObjectId;
      result.reasonCode = std::string{transformPolicy.reasonCode};
      return result;
    }
  }

  result.accepted = true;
  result.reasonCode = "editor_transform_preflight_ready";
  return result;
}

[[nodiscard]] bool beginTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorTransformSource transformSource,
    cr::CreativeSelectionPlacementMode mode,
    CreativeEditorTransformAnchorPolicy anchorPolicy,
    cr::CreativeObjectId activeObjectId,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    const CreativeEditorWorldLayoutState* worldLayout) {
  state = {};
  state.preflight =
      inspectTransformSource(appState, clipboard, mode, worldLayout);
  if (!state.preflight.accepted) {
    SDL_Log("iggy3d_creative: TRANSFORM preview rejected source='%s' "
            "objectCount=%zu reasonCode='%s' failedObject=%llu",
            std::string(source).c_str(), clipboard.objects.size(),
            state.preflight.reasonCode.c_str(),
            static_cast<unsigned long long>(state.preflight.failedObjectId));
    return false;
  }
  state.active = true;
  state.source = transformSource;
  state.anchorPolicy = anchorPolicy;
  state.mode = mode;
  state.sourceClipboard = clipboard;
  state.sourceObjectIds.reserve(clipboard.objects.size());
  for (const cr::CreativeObject& object : clipboard.objects) {
    state.sourceObjectIds.push_back(object.id);
  }
  if (detail::worldLayoutBuildingRoute(state) && worldLayout != nullptr) {
    state.sourceWorldLayout = worldLayout->source;
    state.sourceWorldLayoutNextStableOrdinal =
        worldLayout->nextStableOrdinal;
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      state.snapStepMeters = grid.cellSizeMeters;
    }
  }
  state.transformMode = detail::firstAvailableTransformMode(state);
  const cr::CreativeObject* activeSource =
      detail::transformSourceObject(state, activeObjectId);
  state.activeObjectId =
      activeSource != nullptr ? activeSource->id : clipboard.objects.front().id;
  state.request.mode = mode;
  state.request.sourceAnchor = clipboard.placementAnchor;
  if (activeSource != nullptr &&
      activeSource->kind == cr::CreativeObjectKind::Group) {
    cr::CreativeVec3 groupPivot{};
    if (cr::resolveCreativeSelectionPlacementObjectOrigin(*activeSource,
                                                          groupPivot)) {
      state.pivot = CreativeEditorTransformPivot::ActiveObjectOrigin;
      state.request.sourceAnchor = groupPivot;
    }
  }
  state.moveAvailable = detail::moveSourceAvailable(appState, clipboard);
  if (mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state.active = false;
    state.preflight.accepted = false;
    state.preflight.reasonCode = "editor_transform_move_source_stale";
    return false;
  }
  if (anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = state.request.sourceAnchor;
    detail::refreshResolvedTarget(appState, state);
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview begun source='%s' mode='%s' "
          "objectCount=%zu anchor=(%.3f, %.3f, %.3f)",
          std::string(source).c_str(),
          std::string(cr::toString(mode)).c_str(), clipboard.objects.size(),
          clipboard.placementAnchor.x, clipboard.placementAnchor.y,
          clipboard.placementAnchor.z);
  return true;
}

}  // namespace

bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    const CreativeEditorWorldLayoutState* worldLayout) {
  return beginTransformPreview(
      appState, clipboard, CreativeEditorTransformSource::Clipboard,
      cr::CreativeSelectionPlacementMode::Copy,
      CreativeEditorTransformAnchorPolicy::FollowAim,
      clipboard.objects.empty() ? cr::kInvalidObjectId
                                : clipboard.objects.front().id,
      state, source, worldLayout);
}

bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    CreativeEditorTransformAnchorPolicy anchorPolicy,
    const CreativeEditorWorldLayoutState* worldLayout) {
  cr::CreativeClipboard selection;
  const cr::CreativeClipboardCopyReceipt copied =
      appState.facade.copySelectedObjectsToClipboard(selection);
  if (!copied.accepted) {
    return false;
  }
  cr::CreativeObjectId activeObjectId = cr::kInvalidObjectId;
  const cr::TargetRef primary = appState.facade.selectionState().selectedTarget;
  if (primary.value != cr::kInvalidId) {
    activeObjectId = static_cast<cr::CreativeObjectId>(primary.value);
  }
  return beginTransformPreview(
      appState, selection, CreativeEditorTransformSource::Selection,
      cr::CreativeSelectionPlacementMode::Move, anchorPolicy, activeObjectId,
      state, source, worldLayout);
}

bool beginCreativeEditorTerrainOperationTransformPreview(
    const cr::CreativeAppState& appState,
    cr::CreativeTerrainOperationId operationId,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  state = {};
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       operationId);
  cr::CreativeTerrainHeightFieldBounds sourceBounds{};
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (operation == nullptr ||
      operation->owner != cr::CreativeTerrainOperationOwner::Manual ||
      operation->kind == cr::CreativeTerrainOperationKind::GeneratedTerrain ||
      !cr::creativeTerrainOperationSpatialBounds(*operation, sourceBounds) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    state.preflight.requested = true;
    state.preflight.terrainOperationId = operationId;
    state.preflight.reasonCode =
        "editor_transform_terrain_operation_source_invalid";
    return false;
  }

  const double minimumX =
      static_cast<double>(sourceBounds.minimum.x) * grid.cellSizeMeters;
  const double minimumZ =
      static_cast<double>(sourceBounds.minimum.z) * grid.cellSizeMeters;
  const double maximumX =
      static_cast<double>(static_cast<std::int64_t>(sourceBounds.minimum.x) +
                          sourceBounds.widthCells) *
      grid.cellSizeMeters;
  const double maximumZ =
      static_cast<double>(static_cast<std::int64_t>(sourceBounds.minimum.z) +
                          sourceBounds.depthCells) *
      grid.cellSizeMeters;
  const cr::CreativeBounds worldBounds{
      {minimumX, 0.0, minimumZ},
      {maximumX, grid.cellSizeMeters, maximumZ}};
  if (!cr::measureCreativeBounds(worldBounds).valid) {
    state.preflight.requested = true;
    state.preflight.terrainOperationId = operationId;
    state.preflight.reasonCode =
        "editor_transform_terrain_operation_bounds_invalid";
    return false;
  }

  cr::CreativeObject proxy;
  proxy.id = std::numeric_limits<cr::CreativeObjectId>::max();
  proxy.kind = cr::CreativeObjectKind::Room;
  proxy.name = "Terrain operation transform preview";
  proxy.bounds = worldBounds;
  cr::CreativeClipboard clipboard;
  clipboard.sourceDocumentId = document.id();
  clipboard.sourceRevision = document.revision();
  clipboard.hasPlacementAnchor = true;
  clipboard.placementAnchor = worldBounds.min;
  clipboard.objects.push_back(std::move(proxy));

  state.active = true;
  state.source = CreativeEditorTransformSource::Selection;
  state.transformMode = CreativeEditorTransformMode::Move;
  state.anchorPolicy = CreativeEditorTransformAnchorPolicy::FixedSource;
  state.pivot = CreativeEditorTransformPivot::SelectionAnchor;
  state.mode = cr::CreativeSelectionPlacementMode::Move;
  state.constraint = cr::CreativeSelectionPlacementAxis::Free;
  state.activeObjectId = clipboard.objects.front().id;
  state.snapStepMeters = grid.cellSizeMeters;
  state.sourceClipboard = std::move(clipboard);
  state.request.mode = state.mode;
  state.request.sourceAnchor = worldBounds.min;
  state.preflight.requested = true;
  state.preflight.accepted = true;
  state.preflight.ownershipRoute =
      CreativeEditorTransformOwnershipRoute::TerrainOperation;
  state.preflight.terrainOperationId = operationId;
  state.preflight.sourceDocumentRevision = document.revision();
  state.preflight.capabilities = {
      true, 1U, true, cr::CreativeObjectRotationSupport::None,
      cr::CreativeObjectScaleSupport::None, false};
  state.preflight.reasonCode =
      "editor_transform_terrain_operation_ready";
  state.moveAvailable = true;
  state.aimTargetPositionable = true;
  state.aimTargetAnchor = state.request.sourceAnchor;
  detail::refreshResolvedTarget(appState, state);
  SDL_Log("iggy3d_creative: TERRAIN_OPERATION transform begun source='%s' "
          "operation=%llu kind='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(operationId),
          std::string(cr::toString(operation->kind)).c_str());
  return state.plan.accepted;
}

}  // namespace iggy3d_creative_app
