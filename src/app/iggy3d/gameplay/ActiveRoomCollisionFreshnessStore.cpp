#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

ProductActiveRoomCollisionFreshnessResult ensureActiveRoomCollisionFresh(
    ProductAppWindowState& window,
    const Session* session) {
  ProductActiveRoomCollisionFreshnessResult result;
  result.observedRoomRevision = window.activeRoomRevision;
  result.observedSessionHash = session != nullptr ? session->state().currentStateHash : 0U;
  result.reasonCode = "g2_stub_no_rebake";
  return result;
}

}  // namespace iggy3d
