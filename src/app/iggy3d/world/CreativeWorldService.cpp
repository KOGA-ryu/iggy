#include "app/iggy3d/world/CreativeWorldService.hpp"

#include <limits>
#include <string_view>
#include <utility>

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

void setCreateStatus(CreativeWorldCreateResult& result,
                     std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

void setOpenStatus(CreativeWorldOpenResult& result, std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
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

creative::CreativeDocumentId nextCreativeDocumentId(
    const std::filesystem::path& saveRoot) {
  creative::CreativeDocumentId maxId = creative::kInvalidDocumentId;
  considerCreativeDocumentId(
      scanProductSaves(saveRoot, "", "").catalog.catalog, maxId);
  considerCreativeDocumentId(
      scanDeletedProductSaves(saveRoot, "", "").catalog.catalog, maxId);
  if (maxId == std::numeric_limits<creative::CreativeDocumentId>::max()) {
    return creative::kInvalidDocumentId;
  }
  return maxId + 1U;
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
}

}  // namespace

CreativeWorldCreateResult createCreativeWorld(
    const CreativeWorldCreateRequest& request) {
  CreativeWorldCreateResult result;
  result.title = request.title;
  result.templateId = request.templateId;

  if (request.saveRoot.empty()) {
    setCreateStatus(result, "creative_world_save_root_missing");
    return result;
  }
  if (isBlank(request.title)) {
    setCreateStatus(result, "creative_world_title_missing");
    return result;
  }
  if (isBlank(request.requestedAtUtc)) {
    setCreateStatus(result, "creative_world_timestamp_missing");
    return result;
  }
  if (request.templateId != kEmptyTemplateId) {
    setCreateStatus(result, "creative_world_template_unknown");
    return result;
  }

  result.worldId = nextProductWorldId(request.saveRoot);
  result.documentId = nextCreativeDocumentId(request.saveRoot);
  if (result.documentId == creative::kInvalidDocumentId) {
    setCreateStatus(result, "creative_world_document_id_unavailable");
    return result;
  }

  result.document = creative::CreativeDocument::create(request.title);
  result.documentCreated = result.document.assignId(result.documentId);
  if (!result.documentCreated) {
    setCreateStatus(result, "creative_world_document_create_failed");
    return result;
  }

  ProductCreativeSaveWriteRequest saveRequest;
  saveRequest.saveRoot = request.saveRoot;
  saveRequest.attemptToken = request.attemptToken;
  saveRequest.document = &result.document;
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
    setCreateStatus(result, result.saveWrite.reasonCode);
    return result;
  }

  result.accepted = true;
  result.initialSaveWritten = true;
  setCreateStatus(result, "creative_world_created");
  return result;
}

CreativeWorldOpenResult openCreativeWorld(
    const CreativeWorldOpenRequest& request) {
  CreativeWorldOpenResult result;
  result.saveId = request.saveId.empty() ? "none" : request.saveId;

  if (request.saveRoot.empty()) {
    setOpenStatus(result, "creative_world_save_root_missing");
    return result;
  }
  if (isBlank(request.saveId)) {
    setOpenStatus(result, "creative_world_save_id_missing");
    return result;
  }
  if (!isValidSaveFileId(request.saveId)) {
    setOpenStatus(result, "creative_world_save_id_invalid");
    return result;
  }

  result.path = saveFilePathForId(request.saveRoot, request.saveId);
  const ProductCreativeSaveLoadResult load =
      loadCreativeDocumentSave({result.path});
  mirrorOpenLoad(result, load);
  if (!load.ok) {
    setOpenStatus(result, load.reasonCode);
    return result;
  }

  result.document = load.document;
  result.accepted = true;
  setOpenStatus(result, "creative_world_opened");
  return result;
}

}  // namespace iggy3d
