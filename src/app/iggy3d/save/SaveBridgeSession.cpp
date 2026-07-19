#include "app/iggy3d/save/SaveBridge.hpp"

#include "app/iggy3d/save/SaveBridgeInternal.hpp"
#include "runtime/save/SaveCodec.hpp"

namespace iggy3d {

ProductSaveWriteResult writeProductSessionSaveDurably(
    const ProductSaveWriteRequest& request) {
  // Identity carry-forward: when overwriting an existing save, any identity
  // field the caller leaves empty inherits the value already on disk. This is
  // what stops a pause/progress save (which only knows the save id) from
  // blanking the worldId/worldTitle/saveType/createdAtUtc that world creation
  // wrote. A caller that supplies a field (e.g. saveType="manual" on a manual
  // save, or a future rename) still overrides it. New saves (no existing file)
  // keep the request values verbatim.
  const save_bridge_internal::ExistingSaveIdentity existing =
      save_bridge_internal::readExistingSaveIdentity(request.saveRoot,
                                                     request.saveIdHint);
  const std::string worldId = save_bridge_internal::preferNonEmpty(request.worldId, existing.worldId);
  const std::string worldTitle =
      save_bridge_internal::preferNonEmpty(request.worldTitle, existing.worldTitle);
  const std::string saveTitle =
      save_bridge_internal::preferNonEmpty(request.saveTitle, existing.saveTitle);
  const std::string saveType =
      save_bridge_internal::preferNonEmpty(request.saveType, existing.saveType);
  const std::string createdAtUtc =
      save_bridge_internal::preferNonEmpty(request.createdAtUtc, existing.createdAtUtc);
  const std::string savedAtUtc =
      save_bridge_internal::preferNonEmpty(request.savedAtUtc, existing.savedAtUtc);

  ProductSaveWriteResult result;
  result.durableWriteRequested = true;
  result.worldId = worldId;
  result.worldTitle = worldTitle;
  result.saveTitle = saveTitle;
  result.saveType = saveType;
  result.createdAtUtc = createdAtUtc;
  result.savedAtUtc = savedAtUtc;

  SaveFileDurableWriteRequest durableRequest;
  durableRequest.root = request.saveRoot;
  durableRequest.idHint = request.saveIdHint;
  durableRequest.attemptToken = request.attemptToken;
  durableRequest.state = request.state;
  durableRequest.authoredRoom = request.authoredRoom;
  durableRequest.productMetadata.worldId = worldId;
  durableRequest.productMetadata.worldTitle = worldTitle;
  durableRequest.productMetadata.saveTitle = saveTitle;
  durableRequest.productMetadata.saveType = saveType;
  durableRequest.productMetadata.createdAtUtc = createdAtUtc;
  durableRequest.productMetadata.savedAtUtc = savedAtUtc;

  const SaveFileDurableWriteResult durable =
      writeSessionSaveFileDurably(durableRequest);
  result.durableReason = durable.reason;
  result.paths = durable.paths;
  result.record = durable.record;
  result.encodedBytes = durable.encodedBytes;
  result.tempWritten = durable.tempWritten;
  result.tempValidated = durable.tempValidated;
  result.committed = durable.committed;
  result.finalValidated = durable.finalValidated;
  result.previousExisted = durable.previousExisted;
  result.previousPreserved = durable.previousPreserved;

  if (!durable.ok) {
    result.status = durable.reason;
    result.reasonCode = durable.reason;
    return result;
  }

  result.ok = true;
  result.status = "product_save_written";
  result.reasonCode = "product_save_written";
  return result;
}

ProductSaveLoadResult loadProductSessionSave(
    const ProductSaveLoadRequest& request) {
  ProductSaveLoadResult result;
  if (request.session == nullptr) {
    result.status = "product_save_load_session_missing";
    result.reasonCode = "product_save_load_session_missing";
    return result;
  }
  if (request.path.empty()) {
    result.status = "product_save_load_path_missing";
    result.reasonCode = "product_save_load_path_missing";
    return result;
  }

  result.previousHash = request.session->stateHash();
  const SaveFileReadResult read = readSaveFile(request.path);
  result.codecStatus = read.codecStatus;
  if (!read.ok) {
    result.status = read.reason;
    result.reasonCode = read.reason;
    return result;
  }
  result.fileRead = true;
  result.decoded = true;
  result.record = read.record;
  const SaveDecodeResult decoded = decodeSaveEnvelope(read.encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status == SaveCodecStatus::Ok &&
      decoded.envelope.authoredRoom.present) {
    result.authoredRoomPresent = true;
    result.authoredRoom = decoded.envelope.authoredRoom;
    result.authoredRoomId =
        result.authoredRoom.id.empty() ? "none" : result.authoredRoom.id;
    result.authoredFloorCount =
        static_cast<std::uint64_t>(result.authoredRoom.floors.size());
    result.authoredWallCount =
        static_cast<std::uint64_t>(result.authoredRoom.walls.size());
    result.authoredObjectCount =
        static_cast<std::uint64_t>(result.authoredRoom.objects.size());
    result.authoredMarkerCount =
        static_cast<std::uint64_t>(result.authoredRoom.markers.size());
  }

  SaveEnvelope compatibilityEnvelope;
  const SaveCompatibilityRequest compatibilityRequest{
      compatibilityEnvelope,
      request.expectedPackageId,
      request.expectedScenarioId,
  };
  const LoadStateResult loaded =
      loadEncodedSaveIntoSession(*request.session, read.encodedText,
                                 compatibilityRequest);
  result.loadStatus = loaded.status;
  result.codecStatus = loaded.codecStatus;
  result.compatibilityStatus = loaded.compatibilityStatus;
  result.sessionLoadStatus = loaded.sessionLoadStatus;
  result.previousHash = loaded.previousHash;
  result.loadedHash = loaded.loadedHash;
  result.compatibilityChecked = loaded.status != SaveLoadStatus::DecodeFailed;

  if (loaded.status == SaveLoadStatus::CompatibilityFailed) {
    result.status = "product_save_load_compatibility_failed";
    result.reasonCode = "product_save_load_compatibility_failed";
    return result;
  }
  if (loaded.status == SaveLoadStatus::DecodeFailed) {
    result.status = "product_save_load_decode_failed";
    result.reasonCode = "product_save_load_decode_failed";
    return result;
  }
  if (loaded.status != SaveLoadStatus::Ok) {
    result.status = "product_save_load_failed";
    result.reasonCode = "product_save_load_failed";
    return result;
  }

  result.ok = true;
  result.status = "product_save_loaded";
  result.reasonCode = "product_save_loaded";
  result.sessionLoaded = true;
  return result;
}

}  // namespace iggy3d
