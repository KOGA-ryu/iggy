#include "app/iggy3d/save/SaveBridge.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "app/iggy3d/save/SaveBridgeInternal.hpp"

namespace iggy3d {
namespace {

void mirrorCreativeMetadata(ProductCreativeSaveWriteResult& result,
                            const ProductCreativeSaveWriteRequest& request,
                            const save_bridge_internal::ExistingSaveIdentity& existing) {
  result.packageId = request.packageId;
  result.scenarioId = request.scenarioId;
  result.worldId = save_bridge_internal::preferNonEmpty(request.worldId, existing.worldId);
  result.worldTitle = save_bridge_internal::preferNonEmpty(request.worldTitle, existing.worldTitle);
  result.saveTitle = save_bridge_internal::preferNonEmpty(request.saveTitle, existing.saveTitle);
  result.saveType = save_bridge_internal::preferNonEmpty(request.saveType, existing.saveType);
  result.createdAtUtc =
      save_bridge_internal::preferNonEmpty(request.createdAtUtc, existing.createdAtUtc);
  result.savedAtUtc = save_bridge_internal::preferNonEmpty(request.savedAtUtc, existing.savedAtUtc);
}

void mirrorCreativeMetadata(ProductCreativeSaveLoadResult& result,
                            const SaveEnvelope& envelope) {
  result.packageId = envelope.metadata.packageId;
  result.scenarioId = envelope.metadata.scenarioId;
  result.worldId = envelope.metadata.worldId;
  result.worldTitle = envelope.metadata.worldTitle;
  result.saveTitle = envelope.metadata.saveTitle;
  result.saveType = envelope.metadata.saveType;
  result.createdAtUtc = envelope.metadata.createdAtUtc;
  result.savedAtUtc = envelope.metadata.savedAtUtc;
}

void mirrorCreativeSectionReceipt(
    ProductCreativeSaveWriteResult& result,
    const ProductCreativeDocumentSectionReceipt& receipt) {
  result.sectionReceipt = receipt;
  result.documentId = receipt.documentId;
  result.creativeObjectCount = receipt.objectCount;
  result.creativeNextObjectId = receipt.nextObjectId;
}

void mirrorCreativeSectionReceipt(
    ProductCreativeSaveLoadResult& result,
    const ProductCreativeDocumentSectionReceipt& receipt) {
  result.sectionReceipt = receipt;
  result.documentId = receipt.documentId;
  result.creativeObjectCount = receipt.objectCount;
  result.creativeNextObjectId = receipt.nextObjectId;
}

}  // namespace

ProductCreativeSaveWriteResult writeCreativeDocumentSaveDurably(
    const ProductCreativeSaveWriteRequest& request) {
  ProductCreativeSaveWriteResult result;
  result.packageId = request.packageId;
  result.scenarioId = request.scenarioId;
  result.worldId = request.worldId;
  result.worldTitle = request.worldTitle;
  result.saveTitle = request.saveTitle;
  result.saveType = request.saveType;
  result.createdAtUtc = request.createdAtUtc;
  result.savedAtUtc = request.savedAtUtc;

  if (request.document == nullptr) {
    result.status = "creative_save_document_missing";
    result.reasonCode = "creative_save_document_missing";
    result.durableReason = "creative_save_document_missing";
    return result;
  }

  const ProductCreativeDocumentSectionBuildResult section =
      buildSaveCreativeDocumentSection(*request.document);
  mirrorCreativeSectionReceipt(result, section.receipt);
  if (!section.receipt.accepted) {
    const std::string reason{section.receipt.reasonCode};
    result.status = reason;
    result.reasonCode = reason;
    result.durableReason = reason;
    return result;
  }

  const save_bridge_internal::ExistingSaveIdentity existing =
      save_bridge_internal::readExistingSaveIdentity(request.saveRoot,
                                                     request.saveIdHint);
  mirrorCreativeMetadata(result, request, existing);

  SaveEnvelope envelope;
  envelope.metadata.packageId = result.packageId;
  envelope.metadata.scenarioId = result.scenarioId;
  envelope.metadata.worldId = result.worldId;
  envelope.metadata.worldTitle = result.worldTitle;
  envelope.metadata.saveTitle = result.saveTitle;
  envelope.metadata.saveType = result.saveType;
  envelope.metadata.createdAtUtc = result.createdAtUtc;
  envelope.metadata.savedAtUtc = result.savedAtUtc;
  envelope.metadata.savedStateHash = 0;
  envelope.metadata.savedStateHashHex = "0000000000000000";
  envelope.creativeDocument = section.section;
  if (request.creativeWorldLayoutEncoded != nullptr) {
    if (request.creativeWorldLayoutEncoded->empty()) {
      result.status = "creative_world_layout_encoded_empty";
      result.reasonCode = result.status;
      result.durableReason = result.status;
      return result;
    }
    constexpr std::size_t kMaxCreativeWorldLayoutEncodedBytes =
        8U * 1024U * 1024U;
    if (request.creativeWorldLayoutEncoded->size() >
        kMaxCreativeWorldLayoutEncodedBytes) {
      result.status = "creative_world_layout_encoded_size_exceeded";
      result.reasonCode = result.status;
      result.durableReason = result.status;
      return result;
    }
    envelope.creativeWorldLayout.present = true;
    envelope.creativeWorldLayout.version =
        request.creativeWorldLayoutVersion;
    envelope.creativeWorldLayout.encodedText =
        *request.creativeWorldLayoutEncoded;
    result.creativeWorldLayoutPresent = true;
    result.creativeWorldLayoutVersion = request.creativeWorldLayoutVersion;
    result.creativeWorldLayoutEncodedBytes =
        request.creativeWorldLayoutEncoded->size();
  }

  SaveFileEnvelopeDurableWriteRequest durableRequest;
  durableRequest.root = request.saveRoot;
  durableRequest.idHint = request.saveIdHint;
  durableRequest.attemptToken = request.attemptToken;
  durableRequest.envelope = std::move(envelope);

  result.durableWriteRequested = true;
  const SaveFileDurableWriteResult durable =
      writeSaveEnvelopeFileDurably(durableRequest);
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
  result.status = "creative_save_written";
  result.reasonCode = "creative_save_written";
  return result;
}

ProductCreativeSaveLoadResult loadCreativeDocumentSave(
    const ProductCreativeSaveLoadRequest& request) {
  ProductCreativeSaveLoadResult result;
  if (request.path.empty()) {
    result.status = "creative_save_load_path_missing";
    result.reasonCode = "creative_save_load_path_missing";
    return result;
  }

  const SaveFileReadResult read = readSaveFile(request.path);
  result.codecStatus = read.codecStatus;
  if (!read.ok) {
    const bool decodeFailed = read.reason == "save_file_decode_failed";
    result.status =
        decodeFailed ? "creative_save_decode_failed" : "creative_save_read_failed";
    result.reasonCode = result.status;
    return result;
  }

  result.fileRead = true;
  result.record = read.record;
  result.decoded = true;
  mirrorCreativeMetadata(result, read.envelope);
  result.creativeWorldLayoutPresent =
      read.envelope.creativeWorldLayout.present;
  result.creativeWorldLayoutVersion =
      read.envelope.creativeWorldLayout.version;
  result.creativeWorldLayoutEncoded =
      read.envelope.creativeWorldLayout.encodedText;

  const ProductCreativeDocumentSectionRestoreResult restored =
      restoreCreativeDocumentFromSaveSection(read.envelope.creativeDocument);
  mirrorCreativeSectionReceipt(result, restored.receipt);
  if (!restored.receipt.accepted) {
    const std::string reason{restored.receipt.reasonCode};
    result.status = reason;
    result.reasonCode = reason;
    return result;
  }

  result.document = restored.document;
  result.sectionRestored = true;
  result.documentId = result.document.id();
  result.creativeObjectCount = result.document.objectCount();
  result.creativeNextObjectId = result.document.nextObjectId();
  result.ok = true;
  result.status = "creative_save_loaded";
  result.reasonCode = "creative_save_loaded";
  return result;
}

}  // namespace iggy3d
