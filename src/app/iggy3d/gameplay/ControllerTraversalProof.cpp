#include "app/iggy3d/gameplay/ControllerTraversalProof.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "runtime/collision/CollisionTypes.hpp"
#include "runtime/movement/MovementTraversal.hpp"

namespace iggy3d {

void clearProductTraversalProof(ProductAppWindowState& window) {
  window.gameplay.gameplayTraversal.requested = false;
  window.gameplay.gameplayTraversal.consumed = false;
  window.gameplay.gameplayTraversal.accepted = false;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = false;
  window.gameplay.gameplayTraversal.status = "not_requested";
  window.gameplay.gameplayTraversal.reasonCode = "not_requested";
  window.gameplay.gameplayTraversal.mechanic = "none";
  window.gameplay.gameplayTraversal.slotId = "none";
  window.gameplay.gameplayTraversal.targetId = "none";
  window.gameplay.gameplayTraversal.landingSurfaceId = "none";
  window.gameplay.gameplayTraversal.startX = 0.0F;
  window.gameplay.gameplayTraversal.startY = 0.0F;
  window.gameplay.gameplayTraversal.startZ = 0.0F;
  window.gameplay.gameplayTraversal.finalX = 0.0F;
  window.gameplay.gameplayTraversal.finalY = 0.0F;
  window.gameplay.gameplayTraversal.finalZ = 0.0F;
}

void recordProductTraversalProof(ProductAppWindowState& window,
                                 const TraversalIntentResult& result) {
  window.gameplay.gameplayTraversal.requested = result.requested;
  window.gameplay.gameplayTraversal.consumed = result.consumedInput;
  window.gameplay.gameplayTraversal.accepted = result.accepted;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = result.fallbackJumpAllowed;
  window.gameplay.gameplayTraversal.status = traversalIntentStatusName(result.status);
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.reasonCode =
      result.reasonCode == nullptr ? "unknown" : result.reasonCode;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.mechanic =
      result.traversalAttempted ? traversalMechanicName(result.selectedMechanic)
                                : "none";
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.slotId =
      result.traversal.slotId.empty() ? "none" : result.traversal.slotId;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.targetId =
      result.traversal.targetId.empty() ? "none" : result.traversal.targetId;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.landingSurfaceId =
      result.traversal.landingSurfaceId.empty() ? "none"
                                                : result.traversal.landingSurfaceId;
  window.gameplay.gameplayTraversal.startX = result.traversal.start.x;
  window.gameplay.gameplayTraversal.startY = result.traversal.start.y;
  window.gameplay.gameplayTraversal.startZ = result.traversal.start.z;
  window.gameplay.gameplayTraversal.finalX = result.traversal.finalPosition.x;
  window.gameplay.gameplayTraversal.finalY = result.traversal.finalPosition.y;
  window.gameplay.gameplayTraversal.finalZ = result.traversal.finalPosition.z;
}

void recordProductWallJumpTraversalProof(ProductAppWindowState& window,
                                         const CollisionSurfaceView& surface,
                                         Vec3 start,
                                         Vec3 finalPosition) {
  window.gameplay.gameplayTraversal.requested = true;
  window.gameplay.gameplayTraversal.consumed = true;
  window.gameplay.gameplayTraversal.accepted = true;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = false;
  window.gameplay.gameplayTraversal.status = "traversal_intent_applied";
  window.gameplay.gameplayTraversal.reasonCode = "traversal_intent_applied";
  window.gameplay.gameplayTraversal.mechanic = "wall_jump";
  window.gameplay.gameplayTraversal.slotId = "wall_jump";
  window.gameplay.gameplayTraversal.landingSurfaceId = "wall_jump_surface";
  // branch-gate: BG-1157
  if (!surface.id.empty()) {
    window.gameplay.gameplayTraversal.slotId = surface.id;
    window.gameplay.gameplayTraversal.landingSurfaceId = surface.id;
  }
  window.gameplay.gameplayTraversal.targetId = window.gameplay.gameplayTraversal.slotId;
  // branch-gate: BG-1157
  if (!surface.runtimeOwnerStableName.empty()) {
    window.gameplay.gameplayTraversal.targetId = surface.runtimeOwnerStableName;
  }
  window.gameplay.gameplayTraversal.startX = start.x;
  window.gameplay.gameplayTraversal.startY = start.y;
  window.gameplay.gameplayTraversal.startZ = start.z;
  window.gameplay.gameplayTraversal.finalX = finalPosition.x;
  window.gameplay.gameplayTraversal.finalY = finalPosition.y;
  window.gameplay.gameplayTraversal.finalZ = finalPosition.z;
}

}  // namespace iggy3d
