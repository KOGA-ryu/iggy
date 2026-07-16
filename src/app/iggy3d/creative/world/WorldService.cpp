#include "app/iggy3d/creative/world/WorldService.hpp"

#include <chrono>
#include <limits>
#include <string_view>
#include <utility>

#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "runtime/save/SaveFileStore.hpp"

namespace iggy3d {
namespace {

constexpr std::string_view kEmptyTemplateId = "empty";

bool isBlank(std::string_view value) noexcept {
  for (const char character : value) {
    if (character != ' ' && character != '\t' && character != '\n' &&
        character != '\r') {
      return false;
    }
  }
  return true;
}

// One status setter for all three world-result types (create/open/save): each
// carries {status, reasonCode} and set them identically. A new result type gets
// this for free.
template <typename ResultT>
void setResultStatus(ResultT& result, std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

void considerCreativeDocumentId(const ProductSaveCatalog& catalog,
                                creative::CreativeDocumentId& maxId) {
  for (const ProductSaveCatalogEntry& entry : catalog.entries) {
    if (!entry.creativeDocumentPresent) {
      continue;
    }
    if (entry.creativeDocumentId > maxId) {
      maxId = entry.creativeDocumentId;
    }
  }
}

struct CreativeDocumentIdMintResult {
  creative::CreativeDocumentId id = creative::kInvalidDocumentId;
  bool scanMeasured = false;
  std::uint64_t scanMicroseconds = 0;
  std::uint64_t scanEntryCount = 0;
  std::string_view scanStatus = "creative_document_id_scan_not_requested";
};

CreativeDocumentIdMintResult nextCreativeDocumentId(
    const std::filesystem::path& saveRoot) {
  CreativeDocumentIdMintResult result;
  const auto started = std::chrono::steady_clock::now();
  creative::CreativeDocumentId maxId = creative::kInvalidDocumentId;
  const ProductSaveBridgeResult active = scanProductSaves(saveRoot, "", "");
  const ProductSaveBridgeResult deleted =
      scanDeletedProductSaves(saveRoot, "", "");
  considerCreativeDocumentId(active.catalog.catalog, maxId);
  considerCreativeDocumentId(deleted.catalog.catalog, maxId);
  result.scanMeasured = true;
  result.scanMicroseconds = elapsedMicroseconds(started);
  result.scanEntryCount = active.scanEntryCount + deleted.scanEntryCount;
  result.scanStatus = "creative_document_id_scan_ready";
  if (maxId == std::numeric_limits<creative::CreativeDocumentId>::max()) {
    return result;
  }
  result.id = maxId + 1U;
  return result;
}

void mirrorOpenLoad(CreativeWorldOpenResult& result,
                    const ProductCreativeSaveLoadResult& load) {
  result.load = load;
  result.packageId = load.packageId;
  result.scenarioId = load.scenarioId;
  result.worldId = load.worldId;
  result.worldTitle = load.worldTitle;
  result.saveTitle = load.saveTitle;
  result.saveType = load.saveType;
  result.createdAtUtc = load.createdAtUtc;
  result.savedAtUtc = load.savedAtUtc;
  result.documentId = load.documentId;
  result.objectCount = load.creativeObjectCount;
  result.nextObjectId = load.creativeNextObjectId;
  result.worldLayoutPresent = load.creativeWorldLayoutPresent;
}

void mirrorSaveDocumentState(CreativeWorldSaveResult& result,
                             const creative::CreativeDocument* document) {
  if (document == nullptr) {
    return;
  }
  result.documentId = document->id();
  result.objectCount = document->objectCount();
  result.nextObjectId = document->nextObjectId();
  result.dirtyFlagsBefore = document->dirtyFlags();
  result.dirtyFlagsAfter = result.dirtyFlagsBefore;
}

void mirrorSaveWrite(CreativeWorldSaveResult& result,
                     const ProductCreativeSaveWriteResult& saveWrite) {
  result.saveWrite = saveWrite;
  result.packageId = saveWrite.packageId;
  result.scenarioId = saveWrite.scenarioId;
  result.worldId = saveWrite.worldId;
  result.worldTitle = saveWrite.worldTitle;
  result.saveTitle = saveWrite.saveTitle;
  result.saveType = saveWrite.saveType;
  result.createdAtUtc = saveWrite.createdAtUtc;
  result.savedAtUtc = saveWrite.savedAtUtc;
  result.documentId = saveWrite.documentId;
  result.objectCount = saveWrite.creativeObjectCount;
  result.nextObjectId = saveWrite.creativeNextObjectId;
  result.worldLayoutPresent = saveWrite.creativeWorldLayoutPresent;
}

bool encodeWorldLayoutForSave(
    const creative::CreativeWorldLayout* layout,
    std::string& encoded,
    creative::CreativeWorldLayoutCodecStatus& status,
    std::string& reasonCode) {
  if (layout == nullptr) {
    status = creative::CreativeWorldLayoutCodecStatus::NotRequested;
    return true;
  }
  const creative::CreativeWorldLayoutEncodeResult result =
      creative::encodeCreativeWorldLayout(*layout);
  status = result.status;
  if (!result.accepted) {
    reasonCode = result.reasonCode;
    return false;
  }
  encoded = result.encodedText;
  return true;
}

}  // namespace

CreativeWorldCreateResult createCreativeWorld(
    const CreativeWorldCreateRequest& request) {
  CreativeWorldCreateResult result;
  result.title = request.title;
  result.templateId = request.templateId;

  if (request.saveRoot.empty()) {
    setResultStatus(result, "creative_world_save_root_missing");
    return result;
  }
  if (isBlank(request.title)) {
    setResultStatus(result, "creative_world_title_missing");
    return result;
  }
  if (isBlank(request.requestedAtUtc)) {
    setResultStatus(result, "creative_world_timestamp_missing");
    return result;
  }
  if (request.templateId != kEmptyTemplateId) {
    setResultStatus(result, "creative_world_template_unknown");
    return result;
  }

  const ProductWorldIdMintResult worldId =
      nextProductWorldIdMeasured(request.saveRoot);
  result.worldId = worldId.worldId;
  result.worldIdScanMeasured = worldId.scanMeasured;
  result.worldIdScanMicroseconds = worldId.scanMicroseconds;
  result.worldIdScanEntryCount = worldId.scanEntryCount;
  result.worldIdScanStatus = std::string{worldId.scanStatus};

  const CreativeDocumentIdMintResult documentId =
      nextCreativeDocumentId(request.saveRoot);
  result.documentId = documentId.id;
  result.documentIdScanMeasured = documentId.scanMeasured;
  result.documentIdScanMicroseconds = documentId.scanMicroseconds;
  result.documentIdScanEntryCount = documentId.scanEntryCount;
  result.documentIdScanStatus = std::string{documentId.scanStatus};
  if (result.documentId == creative::kInvalidDocumentId) {
    setResultStatus(result, "creative_world_document_id_unavailable");
    return result;
  }

  result.document = creative::CreativeDocument::create(request.title);
  result.documentCreated = result.document.assignId(result.documentId);
  if (!result.documentCreated) {
    setResultStatus(result, "creative_world_document_create_failed");
    return result;
  }

  ProductCreativeSaveWriteRequest saveRequest;
  saveRequest.saveRoot = request.saveRoot;
  saveRequest.attemptToken = request.attemptToken;
  saveRequest.document = &result.document;
  std::string encodedWorldLayout;
  if (!encodeWorldLayoutForSave(
          request.worldLayout, encodedWorldLayout,
          result.worldLayoutCodecStatus, result.reasonCode)) {
    result.status = result.reasonCode;
    return result;
  }
  if (request.worldLayout != nullptr) {
    saveRequest.creativeWorldLayoutEncoded = &encodedWorldLayout;
    saveRequest.creativeWorldLayoutVersion =
        creative::kCreativeWorldLayoutCodecVersion;
    result.worldLayoutPresent = true;
  }
  saveRequest.packageId = request.packageId;
  saveRequest.scenarioId = request.scenarioId;
  saveRequest.worldId = result.worldId;
  saveRequest.worldTitle = request.title;
  saveRequest.saveTitle = request.title;
  saveRequest.saveType = "creative";
  saveRequest.createdAtUtc = request.requestedAtUtc;
  saveRequest.savedAtUtc = request.requestedAtUtc;

  result.saveWrite = writeCreativeDocumentSaveDurably(saveRequest);
  result.saveId = result.saveWrite.record.id.empty() ? "none"
                                                     : result.saveWrite.record.id;
  result.path = result.saveWrite.record.path;
  if (!result.saveWrite.ok) {
    setResultStatus(result, result.saveWrite.reasonCode);
    return result;
  }

  result.accepted = true;
  result.initialSaveWritten = true;
  setResultStatus(result, "creative_world_created");
  return result;
}

CreativeWorldOpenResult openCreativeWorld(
    const CreativeWorldOpenRequest& request) {
  CreativeWorldOpenResult result;
  result.saveId = request.saveId.empty() ? "none" : request.saveId;

  if (request.saveRoot.empty()) {
    setResultStatus(result, "creative_world_save_root_missing");
    return result;
  }
  if (isBlank(request.saveId)) {
    setResultStatus(result, "creative_world_save_id_missing");
    return result;
  }
  if (!isValidSaveFileId(request.saveId)) {
    setResultStatus(result, "creative_world_save_id_invalid");
    return result;
  }

  result.path = saveFilePathForId(request.saveRoot, request.saveId);
  const ProductCreativeSaveLoadResult load =
      loadCreativeDocumentSave({result.path});
  mirrorOpenLoad(result, load);
  if (!load.ok) {
    setResultStatus(result, load.reasonCode);
    return result;
  }

  if (load.creativeWorldLayoutPresent) {
    if (load.creativeWorldLayoutVersion == 0U ||
        load.creativeWorldLayoutVersion >
            creative::kCreativeWorldLayoutCodecVersion) {
      result.worldLayoutCodecStatus =
          creative::CreativeWorldLayoutCodecStatus::UnsupportedVersion;
      setResultStatus(result, "creative_world_layout_section_unsupported");
      return result;
    }
    creative::CreativeWorldLayoutDecodeResult decodedLayout =
        creative::decodeCreativeWorldLayout(
            load.creativeWorldLayoutEncoded);
    result.worldLayoutCodecStatus = decodedLayout.status;
    if (!decodedLayout.accepted) {
      setResultStatus(result, decodedLayout.reasonCode);
      return result;
    }
    result.worldLayout = std::move(decodedLayout.layout);
    result.worldLayoutPresent = true;
  }

  result.document = load.document;
  result.accepted = true;
  setResultStatus(result, "creative_world_opened");
  return result;
}

CreativeWorldSaveResult saveCreativeWorld(
    const CreativeWorldSaveRequest& request) {
  CreativeWorldSaveResult result;
  result.saveId = request.saveId.empty() ? "none" : request.saveId;
  mirrorSaveDocumentState(result, request.document);

  if (request.document == nullptr) {
    setResultStatus(result, "creative_world_save_document_missing");
    return result;
  }
  if (request.saveRoot.empty()) {
    setResultStatus(result, "creative_world_save_root_missing");
    return result;
  }
  if (isBlank(request.saveId)) {
    setResultStatus(result, "creative_world_save_id_missing");
    return result;
  }
  if (!isValidSaveFileId(request.saveId)) {
    setResultStatus(result, "creative_world_save_id_invalid");
    return result;
  }
  if (request.document->id() == creative::kInvalidDocumentId) {
    setResultStatus(result, "invalid_document_id");
    return result;
  }

  result.path = saveFilePathForId(request.saveRoot, request.saveId);

  ProductCreativeSaveWriteRequest saveRequest;
  saveRequest.saveRoot = request.saveRoot;
  saveRequest.saveIdHint = request.saveId;
  saveRequest.attemptToken = request.attemptToken;
  saveRequest.document = request.document;
  std::string encodedWorldLayout;
  if (!encodeWorldLayoutForSave(
          request.worldLayout, encodedWorldLayout,
          result.worldLayoutCodecStatus, result.reasonCode)) {
    result.status = result.reasonCode;
    return result;
  }
  if (request.worldLayout != nullptr) {
    saveRequest.creativeWorldLayoutEncoded = &encodedWorldLayout;
    saveRequest.creativeWorldLayoutVersion =
        creative::kCreativeWorldLayoutCodecVersion;
    result.worldLayoutPresent = true;
  }
  saveRequest.packageId = request.packageId;
  saveRequest.scenarioId = request.scenarioId;
  saveRequest.worldId = request.worldId;
  saveRequest.worldTitle = request.worldTitle;
  saveRequest.saveTitle = request.saveTitle;
  saveRequest.saveType = request.saveType;
  saveRequest.createdAtUtc = request.createdAtUtc;
  saveRequest.savedAtUtc = request.savedAtUtc;

  const ProductCreativeSaveWriteResult saveWrite =
      writeCreativeDocumentSaveDurably(saveRequest);
  mirrorSaveWrite(result, saveWrite);
  result.saveId = saveWrite.record.id.empty() ? result.saveId
                                              : saveWrite.record.id;
  result.path = saveWrite.record.path.empty() ? result.path
                                              : saveWrite.record.path;
  if (!saveWrite.ok) {
    setResultStatus(result, saveWrite.reasonCode);
    result.dirtyFlagsAfter = request.document->dirtyFlags();
    return result;
  }

  result.dirtyFlagsDrained = request.document->drainDirtyFlags();
  result.dirtyFlagsAfter = request.document->dirtyFlags();
  result.accepted = true;
  result.saved = true;
  setResultStatus(result, "creative_world_saved");
  return result;
}

}  // namespace iggy3d
