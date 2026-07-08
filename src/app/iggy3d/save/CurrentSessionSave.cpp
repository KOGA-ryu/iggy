#include "app/iggy3d/save/CurrentSessionSave.hpp"

#include <string>

#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"

namespace iggy3d {
namespace {

void recordProductSaveWriteResult(std::string_view source,
                                  const ProductSaveWriteResult& written,
                                  ProductAppWindowState& window) {
  window.saveSession.productSaveStatus = written.status;
  window.saveSession.productSaveReasonCode = written.reasonCode;
  window.saveSession.productSaveDurableReason = written.durableReason;
  window.saveSession.productSaveSource = std::string(source);
  window.saveSession.productSaveSaveId = written.record.id.empty() ? "none" : written.record.id;
  window.saveSession.productSaveSessionSaved = written.ok;
  if (written.ok && !written.record.id.empty()) {
    window.saveSession.activeProductSaveId = written.record.id;
  }
}

}  // namespace

ProductSaveWriteResult writeProductCurrentSessionSave(
    const ProductAppOptions& options,
    const std::optional<Session>& activeSession,
    std::string_view source,
    ProductAppWindowState& window) {
  if (!activeSession.has_value()) {
    ProductSaveWriteResult missing;
    missing.status = "product_save_session_missing";
    missing.reasonCode = "product_save_session_missing";
    missing.durableReason = "not_requested";
    recordProductSaveWriteResult(source, missing, window);
    return missing;
  }

  ProductSaveWriteRequest request;
  request.saveRoot = options.saveRoot;
  request.saveIdHint = window.saveSession.activeProductSaveId == "none"
                           ? std::string{}
                           : window.saveSession.activeProductSaveId;
  request.attemptToken = "attempt_002";
  request.state = &activeSession->state();
  if (activeRoom(window).hasAuthoredRoom) {
    request.authoredRoom = &activeRoom(window).authoredRoom;
  }
  // A pause/progress save is a manual save and advances the save time. The
  // remaining identity (worldId/worldTitle/saveTitle/createdAtUtc) is left
  // empty on purpose: writeProductSessionSaveDurably carries it forward from
  // the existing save on disk so re-saving never wipes the world identity.
  request.saveType = "manual";
  request.savedAtUtc = productSaveTimestampNowUtc();
  const ProductSaveWriteResult written = writeProductSessionSaveDurably(request);
  recordProductSaveWriteResult(source, written, window);
  return written;
}

}  // namespace iggy3d
