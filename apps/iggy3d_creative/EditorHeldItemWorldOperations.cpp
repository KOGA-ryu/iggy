#include "EditorInteraction.hpp"
#include "EditorHeldItemWorldOperationsInternal.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void dispatchHeldItemWorldOperation(
    cr::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context) {
  switch (creativeHeldItemWorldOperationOwner(operation)) {
    case CreativeHeldItemWorldOperationOwner::None:
    case CreativeHeldItemWorldOperationOwner::Invalid:
      return;
    case CreativeHeldItemWorldOperationOwner::SelectionTransform:
      executeCreativeHeldItemSelectionTransformOperation(
          operation, context);
      return;
    case CreativeHeldItemWorldOperationOwner::VolumeSurface:
      executeCreativeHeldItemVolumeSurfaceOperation(operation, context);
      return;
    case CreativeHeldItemWorldOperationOwner::Terrain:
      executeCreativeHeldItemTerrainOperation(operation, context);
      return;
    case CreativeHeldItemWorldOperationOwner::Relationships:
      executeCreativeHeldItemRelationshipOperation(operation, context);
      return;
  }
}

}  // namespace

void dispatchCreativeEditorHeldItemWorldOperation(
    cr::CreativeHeldItemWorldOperation operation,
    const CreativeEditorWorldInteractionFrameRequest& request,
    const cr::CreativeHotbarEntry& held) {
  CreativeHeldItemWorldOperationContext context{request, held};
  dispatchHeldItemWorldOperation(operation, context);
}

void processCreativeEditorMoveInteraction(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  processCreativeEditorMoveInteractionInternal(request);
}

}  // namespace iggy3d_creative_app
