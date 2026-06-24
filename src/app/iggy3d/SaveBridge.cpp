#include "app/iggy3d/SaveBridge.hpp"

namespace iggy3d {

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId) {
  ProductSaveBridgeResult result;
  result.saveRoot = saveRoot;
  result.slots = buildSaveSlotList(saveRoot, packageId, scenarioId);
  return result;
}

ProductSaveWriteResult writeProductSessionSaveDurably(
    const ProductSaveWriteRequest& request) {
  ProductSaveWriteResult result;
  result.durableWriteRequested = true;
  result.worldId = request.worldId;
  result.saveType = request.saveType;
  result.autoTitle = request.autoTitle;

  SaveFileDurableWriteRequest durableRequest;
  durableRequest.root = request.saveRoot;
  durableRequest.idHint = request.saveIdHint;
  durableRequest.attemptToken = request.attemptToken;
  durableRequest.state = request.state;
  durableRequest.authoredRoom = request.authoredRoom;

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
