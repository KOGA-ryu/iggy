#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
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
  if (!window.activeRoom.loaded) {
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
  result.observedRoomRevision = window.activeRoomRevision;
  result.observedSessionHash = effectiveSessionHash(session);

  ProductActiveRoomCollisionState& existing = window.activeRoomCollision;
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
          ? buildProductActiveRoomCollision(window.activeRoom, session->state())
          : buildProductActiveRoomCollision(window.activeRoom);
  collision.bakedFromRoomRevision = result.observedRoomRevision;
  collision.bakedFromSessionHash = result.observedSessionHash;

  result.rebaked = true;
  result.reasonCode =
      rebakeReasonCode(window, collision, roomMismatch, sessionMismatch);
  window.activeRoomCollision = std::move(collision);
  return result;
}

}  // namespace iggy3d
