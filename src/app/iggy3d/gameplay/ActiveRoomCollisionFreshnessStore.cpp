// Architecture receipt: ActiveRoomCollisionFreshnessStore
//
// Why: activeRoomCollision is derived truth from the active room plus optional
// live session door state. This store is the single frame/read-seam guard that
// proves the collision blob is stamped for the current room revision and
// current session hash before gameplay reads it.
//
// Owns: the freshness predicate, the room-revision/session-hash provenance
// stamps on activeRoomCollision, the choice of session-aware vs sessionless
// collision bake, and the observable rebake/skip reason.
//
// Does not own: room geometry mutation, session ticking, door activation
// commands, collision math, SpatialSurfaceSet construction policy, persistence,
// or any gameplay movement decision that consumes the collision surfaces.
//
// Thread rules: runs synchronously on the product/window main path. It borrows
// ProductAppWindowState and an optional Session for the duration of the call
// only; no pointers, references, jobs, or snapshots outlive the call.
//
// Shutdown rules: nullptr Session is a first-class input. Dropping/resetting the
// active session restamps the collision blob with session hash 0 and a
// sessionless bake rather than reading destroyed session state.
//
// First consumer: Product window input calls ensureActiveRoomCollisionFresh at
// the frame boundary before applying gameplay actions and handing collision
// surfaces to gameplay.
//
// Known limitations: the session half uses the coarse SessionState
// currentStateHash, so unrelated hashed session changes conservatively rebake.
// The I7 precondition that door-active writes flow through the hashed session
// command path is audited but not structurally enforced here.
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "runtime/session/Session.hpp"

#include <utility>

namespace iggy3d {
namespace {

std::uint64_t effectiveSessionHash(const Session* session) {
  return session != nullptr ? session->state().currentStateHash : 0U;
}

std::string rebakeReasonCode(const ProductAppWindowState& window,
                             const ProductActiveRoomCollisionState& collision,
                             bool roomMismatch,
                             bool sessionMismatch) {
  if (!activeRoom(window).loaded) {
    return "rebaked_unloaded";
  }
  if (!collision.ready || collision.querySurfaceCount == 0U) {
    return "rebaked_empty";
  }
  if (roomMismatch && sessionMismatch) {
    return "rebaked_both";
  }
  if (roomMismatch) {
    return "rebaked_room";
  }
  if (sessionMismatch) {
    return "rebaked_session";
  }
  return "rebaked_both";
}

}  // namespace

ProductActiveRoomCollisionFreshnessResult ensureActiveRoomCollisionFresh(
    ProductAppWindowState& window,
    const Session* session) {
  ProductActiveRoomCollisionFreshnessResult result;
  result.observedRoomRevision = activeRoomRevision(window);
  result.observedSessionHash = effectiveSessionHash(session);

  ProductActiveRoomCollisionState& existing = activeRoomCollision(window);
  const bool roomMismatch =
      existing.bakedFromRoomRevision != result.observedRoomRevision;
  const bool sessionMismatch =
      existing.bakedFromSessionHash != result.observedSessionHash;
  if (!roomMismatch && !sessionMismatch) {
    result.reasonCode = "skipped_fresh";
    return result;
  }

  ProductActiveRoomCollisionState collision =
      session != nullptr
          ? buildProductActiveRoomCollision(activeRoom(window), session->state())
          : buildProductActiveRoomCollision(activeRoom(window));
  collision.bakedFromRoomRevision = result.observedRoomRevision;
  collision.bakedFromSessionHash = result.observedSessionHash;

  result.rebaked = true;
  result.reasonCode =
      rebakeReasonCode(window, collision, roomMismatch, sessionMismatch);
  activeRoomCollision(window) = std::move(collision);
  return result;
}

}  // namespace iggy3d
