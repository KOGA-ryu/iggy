#include "app/iggy3d/world/CreativeWorldService.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

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
      validationRejectsBeforeDurableWrite() &&
      invalidAttemptTokenFailsAfterDocumentCreationWithoutCommittedSave() &&
      documentIdMintUsesActiveAndDeletedCreativeSaves() &&
      openMissingAndInvalidSaveIdsRejectBeforeLoad() &&
      openProductSessionSaveRejectsMissingCreativeSection();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
