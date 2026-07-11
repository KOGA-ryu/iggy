#include "app/iggy3d/creative/world/WorldService.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <utility>

#include "runtime/save/SaveFileStore.hpp"

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_creative_world_service_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

iggy3d::CreativeWorldCreateRequest createRequest(
    const std::filesystem::path& root,
    std::string_view title = "Creative Alpha",
    std::string_view requestedAtUtc = "2026-07-03T10:00:00Z") {
  iggy3d::CreativeWorldCreateRequest request;
  request.saveRoot = root;
  request.title = std::string{title};
  request.requestedAtUtc = std::string{requestedAtUtc};
  return request;
}

iggy3d::CreativeWorldSaveRequest saveRequest(
    const std::filesystem::path& root,
    std::string saveId,
    cr::CreativeDocument& document) {
  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = root;
  request.saveId = std::move(saveId);
  request.document = &document;
  return request;
}

bool createEmptyCreativeWorldWritesDurableSaveAndScansAsCreative() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(createRequest(root));
  const iggy3d::ProductSaveBridgeResult scanned =
      iggy3d::scanProductSaves(root, "iggy3d.creative", "creative.document");
  const bool hasEntry = scanned.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry entry =
      hasEntry ? scanned.catalog.catalog.entries.front()
               : iggy3d::ProductSaveCatalogEntry{};
  const iggy3d::ProductContinueSelectionResult selected =
      iggy3d::selectProductContinueSave(scanned.catalog.catalog);

  return expect(created.accepted, "create accepted") &&
         expect(created.status == "creative_world_created",
                "create status") &&
         expect(created.reasonCode == "creative_world_created",
                "create reason") &&
         expect(created.title == "Creative Alpha", "create title mirror") &&
         expect(created.templateId == "empty", "create template mirror") &&
         expect(created.worldId == "world_0001", "create world id") &&
         expect(created.documentId == 1U, "create document id") &&
         expect(created.worldIdScanMeasured, "create world id scan measured") &&
         expect(created.worldIdScanStatus == "product_world_id_scan_ready",
                "create world id scan status") &&
         expect(created.worldIdScanEntryCount == 0U,
                "create world id scan empty count") &&
         expect(created.documentIdScanMeasured,
                "create document id scan measured") &&
         expect(created.documentIdScanStatus ==
                    "creative_document_id_scan_ready",
                "create document id scan status") &&
         expect(created.documentIdScanEntryCount == 0U,
                "create document id scan empty count") &&
         expect(created.documentCreated, "create document created") &&
         expect(created.initialSaveWritten, "create initial save written") &&
         expect(created.saveId == "save_001", "create save id") &&
         expect(created.path == root / "save_001.iggy3d.save",
                "create save path") &&
         expect(std::filesystem::exists(created.path), "create file exists") &&
         expect(created.saveWrite.ok, "create save write ok") &&
         expect(created.saveWrite.worldId == "world_0001",
                "create save world id") &&
         expect(created.saveWrite.worldTitle == "Creative Alpha",
                "create save world title") &&
         expect(created.saveWrite.saveTitle == "Creative Alpha",
                "create save title") &&
         expect(created.saveWrite.saveType == "creative",
                "create save type") &&
         expect(created.saveWrite.createdAtUtc == "2026-07-03T10:00:00Z",
                "create created time") &&
         expect(created.saveWrite.savedAtUtc == "2026-07-03T10:00:00Z",
                "create saved time") &&
         expect(created.document.id() == created.documentId,
                "create document id stored") &&
         expect(created.document.name() == "Creative Alpha",
                "create document name") &&
         expect(created.document.objectCount() == 0U,
                "create document empty") &&
         expect(created.document.revision() == 0U,
                "create document clean revision") &&
         expect(created.document.dirtyFlags() == 0U,
                "create document clean dirty") &&
         expect(hasEntry, "create scan one entry") &&
         expect(entry.contentKind ==
                    iggy3d::ProductSaveContentKind::CreativeDocument,
                "create scan creative content kind") &&
         expect(entry.creativeDocumentPresent,
                "create scan creative present") &&
         expect(entry.creativeDocumentId == created.documentId,
                "create scan document id") &&
         expect(entry.creativeObjectCount == 0U,
                "create scan object count") &&
         expect(entry.creativeNextObjectId == created.document.nextObjectId(),
                "create scan next id") &&
         expect(iggy3d::canOpenCreativeWorld(entry),
                "create scan openable") &&
         expect(!iggy3d::canLoadProductSave(entry),
                "create scan not product loadable") &&
         expect(entry.disabledReason == "creative_save_not_product_loadable",
                "create scan disabled reason") &&
         expect(!selected.selected, "continue does not select creative") &&
         expect(selected.status == "continue_no_compatible_saves",
                "continue no compatible product save") &&
         expect(selected.consideredCount == 1U,
                "continue considered creative active row") &&
         expect(selected.compatibleCount == 0U,
                "continue compatible product count zero");
}

bool openCreativeWorldRestoresCreatedDocument() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(createRequest(root, "Open Me"));
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, created.saveId});

  return expect(created.accepted, "open setup create accepted") &&
         expect(opened.accepted, "open accepted") &&
         expect(opened.status == "creative_world_opened", "open status") &&
         expect(opened.reasonCode == "creative_world_opened",
                "open reason") &&
         expect(opened.saveId == created.saveId, "open save id") &&
         expect(opened.path == created.path, "open path") &&
         expect(opened.load.ok, "open load ok") &&
         expect(opened.documentId == created.documentId,
                "open document id") &&
         expect(opened.objectCount == 0U, "open object count") &&
         expect(opened.document.id() == created.documentId,
                "open document id stored") &&
         expect(opened.document.name() == "Open Me", "open name") &&
         expect(opened.document.revision() == 0U,
                "open revision clean") &&
         expect(opened.document.dirtyFlags() == 0U,
                "open dirty clean") &&
         expect(opened.worldId == created.worldId, "open world id") &&
         expect(opened.worldTitle == "Open Me", "open world title") &&
         expect(opened.saveTitle == "Open Me", "open save title") &&
         expect(opened.saveType == "creative", "open save type");
}

bool saveDirtyCreativeWorldDrainsDirtyFlagsAfterDurableWrite() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(createRequest(root, "Carry Forward"));
  cr::CreativeDocument document = created.document;
  const bool renamed = document.rename("Renamed Creative Document");
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();
  const std::uint64_t revisionBeforeSave = document.revision();

  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(saveRequest(root, created.saveId, document));
  const iggy3d::CreativeWorldOpenResult reopened =
      iggy3d::openCreativeWorld({root, created.saveId});
  const iggy3d::ProductSaveBridgeResult scanned =
      iggy3d::scanProductSaves(root, "iggy3d.creative", "creative.document");
  const bool hasEntry = scanned.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry entry =
      hasEntry ? scanned.catalog.catalog.entries.front()
               : iggy3d::ProductSaveCatalogEntry{};

  return expect(created.accepted, "save dirty setup create accepted") &&
         expect(renamed, "save dirty document renamed") &&
         expect(dirtyBefore != 0U, "save dirty has dirty flags") &&
         expect(saved.accepted, "save dirty accepted") &&
         expect(saved.saved, "save dirty saved") &&
         expect(saved.status == "creative_world_saved",
                "save dirty status") &&
         expect(saved.reasonCode == "creative_world_saved",
                "save dirty reason") &&
         expect(saved.saveId == created.saveId, "save dirty same id") &&
         expect(saved.path == created.path, "save dirty same path") &&
         expect(saved.saveWrite.ok, "save dirty write ok") &&
         expect(saved.saveWrite.previousExisted,
                "save dirty overwrote existing save") &&
         expect(saved.dirtyFlagsBefore == dirtyBefore,
                "save dirty before mirrored") &&
         expect(saved.dirtyFlagsDrained == dirtyBefore,
                "save dirty drained before flags") &&
         expect(saved.dirtyFlagsAfter == 0U, "save dirty after zero") &&
         expect(document.dirtyFlags() == 0U, "save dirty document drained") &&
         expect(document.revision() == revisionBeforeSave,
                "save dirty revision preserved") &&
         expect(reopened.accepted, "save dirty reopen accepted") &&
         expect(reopened.document.name() == "Renamed Creative Document",
                "save dirty reopened renamed document") &&
         expect(reopened.document.revision() == 0U,
                "save dirty reopened revision clean") &&
         expect(reopened.document.dirtyFlags() == 0U,
                "save dirty reopened dirty clean") &&
         expect(saved.worldId == "world_0001",
                "save dirty carried world id") &&
         expect(saved.worldTitle == "Carry Forward",
                "save dirty carried world title") &&
         expect(saved.saveTitle == "Carry Forward",
                "save dirty carried save title") &&
         expect(saved.saveType == "creative",
                "save dirty carried save type") &&
         expect(saved.createdAtUtc == "2026-07-03T10:00:00Z",
                "save dirty carried created time") &&
         expect(saved.savedAtUtc == "2026-07-03T10:00:00Z",
                "save dirty carried saved time") &&
         expect(hasEntry, "save dirty scan entry") &&
         expect(entry.worldId == "world_0001",
                "save dirty catalog world id carried") &&
         expect(entry.worldTitle == "Carry Forward",
                "save dirty catalog world title carried") &&
         expect(entry.saveTitle == "Carry Forward",
                "save dirty catalog save title carried");
}

bool saveCleanCreativeWorldSucceedsAndDrainsZero() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(createRequest(root, "Clean"));
  cr::CreativeDocument document = created.document;

  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(saveRequest(root, created.saveId, document));

  return expect(created.accepted, "save clean setup create accepted") &&
         expect(document.dirtyFlags() == 0U, "save clean starts clean") &&
         expect(saved.accepted, "save clean accepted") &&
         expect(saved.saved, "save clean saved") &&
         expect(saved.dirtyFlagsBefore == 0U, "save clean before zero") &&
         expect(saved.dirtyFlagsDrained == 0U, "save clean drained zero") &&
         expect(saved.dirtyFlagsAfter == 0U, "save clean after zero") &&
         expect(document.dirtyFlags() == 0U, "save clean document remains clean");
}

bool saveInvalidAttemptTokenPreservesDirtyFlags() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(createRequest(root, "Dirty Failure"));
  cr::CreativeDocument document = created.document;
  static_cast<void>(document.rename("Still Dirty"));
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();
  iggy3d::CreativeWorldSaveRequest request =
      saveRequest(root, created.saveId, document);
  request.attemptToken = "attempt token with spaces";

  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(request);

  return expect(created.accepted, "save invalid setup create accepted") &&
         expect(dirtyBefore != 0U, "save invalid setup dirty") &&
         expect(!saved.accepted, "save invalid rejected") &&
         expect(!saved.saved, "save invalid not saved") &&
         expect(saved.status == "durable_save_invalid_attempt_token",
                "save invalid status") &&
         expect(saved.dirtyFlagsBefore == dirtyBefore,
                "save invalid before mirrored") &&
         expect(saved.dirtyFlagsDrained == 0U,
                "save invalid did not drain") &&
         expect(saved.dirtyFlagsAfter == dirtyBefore,
                "save invalid after preserved") &&
         expect(document.dirtyFlags() == dirtyBefore,
                "save invalid document dirty preserved") &&
         expect(!saved.saveWrite.tempWritten,
                "save invalid no temp write") &&
         expect(!saved.saveWrite.committed,
                "save invalid not committed");
}

bool saveValidationRejectsBeforeDurableWriteAndPreservesDirtyFlags() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(createRequest(root, "Validation"));
  cr::CreativeDocument document = created.document;
  static_cast<void>(document.rename("Validation Dirty"));
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();

  iggy3d::CreativeWorldSaveRequest missingRoot =
      saveRequest(root, created.saveId, document);
  missingRoot.saveRoot.clear();
  iggy3d::CreativeWorldSaveRequest missingId =
      saveRequest(root, " ", document);
  iggy3d::CreativeWorldSaveRequest invalidId =
      saveRequest(root, "save/001", document);
  iggy3d::CreativeWorldSaveRequest nullDocument;
  nullDocument.saveRoot = root;
  nullDocument.saveId = created.saveId;

  const iggy3d::CreativeWorldSaveResult missingRootResult =
      iggy3d::saveCreativeWorld(missingRoot);
  const iggy3d::CreativeWorldSaveResult missingIdResult =
      iggy3d::saveCreativeWorld(missingId);
  const iggy3d::CreativeWorldSaveResult invalidIdResult =
      iggy3d::saveCreativeWorld(invalidId);
  const iggy3d::CreativeWorldSaveResult nullDocumentResult =
      iggy3d::saveCreativeWorld(nullDocument);

  return expect(created.accepted, "save validation setup create accepted") &&
         expect(!missingRootResult.accepted, "save missing root rejected") &&
         expect(missingRootResult.status == "creative_world_save_root_missing",
                "save missing root status") &&
         expect(missingRootResult.dirtyFlagsBefore == dirtyBefore,
                "save missing root dirty before") &&
         expect(missingRootResult.dirtyFlagsAfter == dirtyBefore,
                "save missing root dirty after") &&
         expect(!missingIdResult.accepted, "save missing id rejected") &&
         expect(missingIdResult.status == "creative_world_save_id_missing",
                "save missing id status") &&
         expect(missingIdResult.dirtyFlagsAfter == dirtyBefore,
                "save missing id dirty after") &&
         expect(!invalidIdResult.accepted, "save invalid id rejected") &&
         expect(invalidIdResult.status == "creative_world_save_id_invalid",
                "save invalid id status") &&
         expect(invalidIdResult.dirtyFlagsAfter == dirtyBefore,
                "save invalid id dirty after") &&
         expect(!nullDocumentResult.accepted, "save null document rejected") &&
         expect(nullDocumentResult.status ==
                    "creative_world_save_document_missing",
                "save null document status") &&
         expect(!missingRootResult.saveWrite.durableWriteRequested,
                "save missing root no durable") &&
         expect(!missingIdResult.saveWrite.durableWriteRequested,
                "save missing id no durable") &&
         expect(!invalidIdResult.saveWrite.durableWriteRequested,
                "save invalid id no durable") &&
         expect(document.dirtyFlags() == dirtyBefore,
                "save validation document dirty preserved");
}

bool saveInvalidDocumentIdRejectsAndPreservesDirtyFlags() {
  const std::filesystem::path root = testRoot();
  cr::CreativeDocument document = cr::CreativeDocument::create("No Id");
  static_cast<void>(document.rename("No Id Dirty"));
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();

  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(saveRequest(root, "save_001", document));

  return expect(!saved.accepted, "save invalid document rejected") &&
         expect(saved.status == "invalid_document_id",
                "save invalid document status") &&
         expect(saved.documentId == cr::kInvalidDocumentId,
                "save invalid document id mirrored") &&
         expect(saved.dirtyFlagsBefore == dirtyBefore,
                "save invalid document dirty before") &&
         expect(saved.dirtyFlagsAfter == dirtyBefore,
                "save invalid document dirty after") &&
         expect(saved.dirtyFlagsDrained == 0U,
                "save invalid document did not drain") &&
         expect(document.dirtyFlags() == dirtyBefore,
                "save invalid document dirty preserved") &&
         expect(!saved.saveWrite.durableWriteRequested,
                "save invalid document no durable") &&
         expect(iggy3d::listSaveFilePaths(root).empty(),
                "save invalid document no committed save");
}

bool validationRejectsBeforeDurableWrite() {
  const std::filesystem::path root = testRoot();
  iggy3d::CreativeWorldCreateRequest blankTitle = createRequest(root, "   ");
  iggy3d::CreativeWorldCreateRequest blankTime =
      createRequest(root, "Creative", " ");
  iggy3d::CreativeWorldCreateRequest unknownTemplate =
      createRequest(root, "Creative", "2026-07-03T10:00:00Z");
  unknownTemplate.templateId = "starter_room";
  iggy3d::CreativeWorldCreateRequest missingRoot =
      createRequest(root, "Creative", "2026-07-03T10:00:00Z");
  missingRoot.saveRoot.clear();

  const iggy3d::CreativeWorldCreateResult titleResult =
      iggy3d::createCreativeWorld(blankTitle);
  const iggy3d::CreativeWorldCreateResult timeResult =
      iggy3d::createCreativeWorld(blankTime);
  const iggy3d::CreativeWorldCreateResult templateResult =
      iggy3d::createCreativeWorld(unknownTemplate);
  const iggy3d::CreativeWorldCreateResult rootResult =
      iggy3d::createCreativeWorld(missingRoot);

  return expect(!titleResult.accepted, "blank title rejected") &&
         expect(titleResult.status == "creative_world_title_missing",
                "blank title status") &&
         expect(!timeResult.accepted, "blank timestamp rejected") &&
         expect(timeResult.status == "creative_world_timestamp_missing",
                "blank timestamp status") &&
         expect(!templateResult.accepted, "unknown template rejected") &&
         expect(templateResult.status == "creative_world_template_unknown",
                "unknown template status") &&
         expect(!rootResult.accepted, "missing root rejected") &&
         expect(rootResult.status == "creative_world_save_root_missing",
                "missing root status") &&
         expect(iggy3d::listSaveFilePaths(root).empty(),
                "validation wrote no saves");
}

bool invalidAttemptTokenFailsAfterDocumentCreationWithoutCommittedSave() {
  const std::filesystem::path root = testRoot();
  iggy3d::CreativeWorldCreateRequest request = createRequest(root);
  request.attemptToken = "attempt token with spaces";

  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(request);

  return expect(!created.accepted, "invalid attempt rejected") &&
         expect(created.status == "durable_save_invalid_attempt_token",
                "invalid attempt status") &&
         expect(created.reasonCode == "durable_save_invalid_attempt_token",
                "invalid attempt reason") &&
         expect(created.documentCreated,
                "invalid attempt still created local document") &&
         expect(!created.initialSaveWritten,
                "invalid attempt no initial save") &&
         expect(created.saveWrite.durableWriteRequested,
                "invalid attempt durable requested") &&
         expect(!created.saveWrite.tempWritten,
                "invalid attempt no temp write") &&
         expect(!created.saveWrite.committed,
                "invalid attempt not committed") &&
         expect(iggy3d::listSaveFilePaths(root).empty(),
                "invalid attempt no committed save");
}

bool documentIdMintUsesActiveAndDeletedCreativeSaves() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldCreateResult first =
      iggy3d::createCreativeWorld(createRequest(root, "First"));
  const iggy3d::CreativeWorldCreateResult second =
      iggy3d::createCreativeWorld(createRequest(root, "Second"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, first.saveId});
  const iggy3d::CreativeWorldCreateResult third =
      iggy3d::createCreativeWorld(createRequest(root, "Third"));

  return expect(first.accepted, "id first accepted") &&
         expect(second.accepted, "id second accepted") &&
         expect(deleted.ok, "id first deleted") &&
         expect(third.accepted, "id third accepted") &&
         expect(first.documentId == 1U, "id first one") &&
         expect(second.documentId == 2U, "id second two") &&
         expect(third.documentId == 3U,
                "id third considers active and deleted") &&
         expect(third.worldIdScanMeasured, "id third world scan measured") &&
         expect(third.worldIdScanEntryCount == 2U,
                "id third world scan entry count") &&
         expect(third.documentIdScanMeasured,
                "id third document scan measured") &&
         expect(third.documentIdScanEntryCount == 2U,
                "id third document scan entry count") &&
         expect(first.worldId == "world_0001", "world first one") &&
         expect(second.worldId == "world_0002", "world second two") &&
         expect(third.worldId == "world_0003",
                "world third considers active and deleted");
}

bool openMissingAndInvalidSaveIdsRejectBeforeLoad() {
  const std::filesystem::path root = testRoot();
  const iggy3d::CreativeWorldOpenResult missing =
      iggy3d::openCreativeWorld({root, " "});
  const iggy3d::CreativeWorldOpenResult invalid =
      iggy3d::openCreativeWorld({root, "save/001"});

  return expect(!missing.accepted, "open missing id rejected") &&
         expect(missing.status == "creative_world_save_id_missing",
                "open missing id status") &&
         expect(!missing.load.fileRead, "open missing id no read") &&
         expect(!invalid.accepted, "open invalid id rejected") &&
         expect(invalid.status == "creative_world_save_id_invalid",
                "open invalid id status") &&
         expect(!invalid.load.fileRead, "open invalid id no read");
}

bool openProductSessionSaveRejectsMissingCreativeSection() {
  const std::filesystem::path root = testRoot();
  iggy3d::SaveEnvelope envelope;
  envelope.metadata.packageId = "iggy3d.product.default";
  envelope.metadata.scenarioId = "training_ground";
  envelope.metadata.worldId = "world_product_0001";
  envelope.metadata.worldTitle = "Product World";
  envelope.metadata.saveTitle = "Product Save";
  envelope.metadata.saveType = "manual";
  envelope.metadata.createdAtUtc = "2026-07-03T10:00:00Z";
  envelope.metadata.savedAtUtc = "2026-07-03T10:01:00Z";

  iggy3d::SaveFileEnvelopeDurableWriteRequest write;
  write.root = root;
  write.idHint = "product_session";
  write.attemptToken = "attempt_001";
  write.envelope = envelope;
  const iggy3d::SaveFileDurableWriteResult written =
      iggy3d::writeSaveEnvelopeFileDurably(write);
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "product_session"});

  return expect(written.ok, "product section setup written") &&
         expect(!opened.accepted, "product section rejected") &&
         expect(opened.status == "missing_creative_document_section",
                "product section status") &&
         expect(opened.load.fileRead, "product section read") &&
         expect(opened.load.decoded, "product section decoded") &&
         expect(!opened.load.sectionRestored,
                "product section not restored") &&
         expect(opened.load.sectionReceipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        MissingSection,
                "product section receipt status");
}

}  // namespace

int main() {
  const bool ok =
      createEmptyCreativeWorldWritesDurableSaveAndScansAsCreative() &&
      openCreativeWorldRestoresCreatedDocument() &&
      saveDirtyCreativeWorldDrainsDirtyFlagsAfterDurableWrite() &&
      saveCleanCreativeWorldSucceedsAndDrainsZero() &&
      saveInvalidAttemptTokenPreservesDirtyFlags() &&
      saveValidationRejectsBeforeDurableWriteAndPreservesDirtyFlags() &&
      saveInvalidDocumentIdRejectsAndPreservesDirtyFlags() &&
      validationRejectsBeforeDurableWrite() &&
      invalidAttemptTokenFailsAfterDocumentCreationWithoutCommittedSave() &&
      documentIdMintUsesActiveAndDeletedCreativeSaves() &&
      openMissingAndInvalidSaveIdsRejectBeforeLoad() &&
      openProductSessionSaveRejectsMissingCreativeSection();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
