#include "app/iggy3d/ReceiptBuilder.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/receipt/ReceiptFields.hpp"

namespace iggy3d {
namespace {

void setPhysicsMovementPlannerProof(ProductAppWindowState& window,
                                    std::string status,
                                    bool requested,
                                    bool used) {
  window.physicsMovementPlanner.requested = requested;
  window.physicsMovementPlanner.used = used;
  window.physicsMovementPlanner.status = std::move(status);
  window.physicsMovementPlanner.reasonCode = window.physicsMovementPlanner.status;
}

}  // namespace

void recordProductPhysicsMovementPlannerTickProof(
    ProductAppWindowState& window,
    bool requested,
    bool collisionSurfacesAvailable,
    bool movementPhysicsStatsAvailable) {
  // branch-gate: BG-1114
  if (!requested) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_disabled", false, false);
    return;
  }
  // branch-gate: BG-1114
  if (!collisionSurfacesAvailable) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_no_collision_surfaces", true, false);
    return;
  }
  // branch-gate: BG-1114
  if (movementPhysicsStatsAvailable) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_used", true, true);
    return;
  }
  setPhysicsMovementPlannerProof(
      window, "physics_movement_planner_not_used", true, false);
}

}  // namespace iggy3d
