#include "EditorEdits.hpp"
#include "EditorEditsInternal.hpp"

#include "EditorAttachmentPlacement.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;
CreativeEditorSemanticEditReceipt
setCreativeEditorMovingPlatformSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeMovingPlatformSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetMovingPlatformSettings,
      creative::makeMovingPlatformSettingsPayload(settings),
      "SET MOVING PLATFORM SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorPlayerSpawnSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativePlayerSpawnSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetPlayerSpawnSettings,
      creative::makePlayerSpawnSettingsPayload(std::move(settings)),
      "SET PLAYER SPAWN SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorNpcSpawnSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeNpcSpawnSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetNpcSpawnSettings,
      creative::makeNpcSpawnSettingsPayload(std::move(settings)),
      "SET NPC SPAWN SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorLootPointSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeLootPointSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetLootPointSettings,
      creative::makeLootPointSettingsPayload(std::move(settings)),
      "SET LOOT POINT SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorExitPointSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeExitPointSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetExitPointSettings,
      creative::makeExitPointSettingsPayload(std::move(settings)),
      "SET EXIT POINT SETTINGS", source, worldLayout);
}

}  // namespace iggy3d_creative_app
