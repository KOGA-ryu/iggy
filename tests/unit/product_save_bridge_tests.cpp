#include "app/iggy3d/save/SaveBridge.hpp"
#include "content/PackageLoader.hpp"
#include "runtime/save/SaveCodec.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_product_save_bridge_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

iggy3d::Session makeFixtureSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({"fixtures/demos/movement_playground/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request).value;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::Session makeChangedFixtureSession() {
  iggy3d::Session session = makeFixtureSession();
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  (void)session.tick();
  return session;
}

iggy3d::ProductSaveWriteRequest productSaveRequest(
    const std::filesystem::path& root,
    const iggy3d::Session& session,
    std::string_view attemptToken,
    std::string_view idHint = "") {
  iggy3d::ProductSaveWriteRequest request;
  request.saveRoot = root;
  request.saveIdHint = std::string(idHint);
  request.attemptToken = std::string(attemptToken);
  request.state = &session.state();
  request.worldId = "world_0001";
  request.worldTitle = "Training World";
  request.saveTitle = "Manual Save";
  request.saveType = "manual";
  request.createdAtUtc = "2026-06-24T00:00:00Z";
  request.savedAtUtc = "2026-06-24T01:02:03Z";
  return request;
}

iggy3d::SaveAuthoredRoomSection authoredRoomFixture() {
  iggy3d::SaveAuthoredRoomSection authoredRoom;
  authoredRoom.present = true;
  authoredRoom.id = "product_bridge_room";
  iggy3d::SaveAuthoredRoomFloorRecord floor;
  floor.id = "bridge_floor_1";
  floor.locked = true;
  floor.hidden = true;
  floor.semantics.traversalTags = {"walkable"};
  authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomMarkerRecord marker;
  marker.id = "marker_product_bridge_treasure";
  marker.tag = "treasure";
  marker.glyph = "$";
  marker.row = 2;
  marker.column = 3;
  marker.positionMeters = {2.0F, 0.05F, 3.0F};
  marker.sourceLine = 3;
  marker.sourceColumn = 4;
  authoredRoom.markers.push_back(marker);
  return authoredRoom;
}

bool sameCreativeVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameCreativeBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return sameCreativeVec3(lhs.min, rhs.min) &&
         sameCreativeVec3(lhs.max, rhs.max);
}

bool sameCreativeTransform(cr::CreativeTransform lhs,
                           cr::CreativeTransform rhs) {
  return sameCreativeVec3(lhs.position, rhs.position) &&
         sameCreativeVec3(lhs.rotation, rhs.rotation) &&
         sameCreativeVec3(lhs.scale, rhs.scale);
}

bool sameCreativeGridSettings(cr::CreativeGridSettings lhs,
                              cr::CreativeGridSettings rhs) {
  return sameCreativeVec3(lhs.origin, rhs.origin) &&
         lhs.cellSizeMeters == rhs.cellSizeMeters &&
         lhs.size.width == rhs.size.width && lhs.size.height == rhs.size.height &&
         lhs.size.depth == rhs.size.depth;
}

bool sameCreativeSnapSettings(cr::CreativeDocumentSnapSettings lhs,
                              cr::CreativeDocumentSnapSettings rhs) {
  return lhs.mode == rhs.mode && lhs.axes == rhs.axes &&
         lhs.stepX == rhs.stepX && lhs.stepY == rhs.stepY &&
         lhs.stepZ == rhs.stepZ && lhs.originX == rhs.originX &&
         lhs.originY == rhs.originY && lhs.originZ == rhs.originZ;
}

cr::CreativeGridSettings creativeGridFixture() {
  cr::CreativeGridSettings settings;
  settings.origin = {1.0 / 3.0, 0.125, -4.5};
  settings.cellSizeMeters = 0.5;
  settings.size = {64, 32, 8};
  return settings;
}

cr::CreativeDocumentSnapSettings creativeSnapFixture() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.mode = cr::CreativeDocumentSnapMode::Grid;
  settings.axes = cr::kCreativeDocumentSnapAxisXZ;
  settings.stepX = 0.25;
  settings.stepZ = 2.0;
  settings.originX = -1.0;
  settings.originZ = 4.0;
  return settings;
}

cr::CreativeBounds creativeWorldBoundsFixture() {
  return {{-8.0, -1.0, -4.0}, {64.0, 32.0, 8.0}};
}

cr::CreativeObject creativeGroupObjectFixture() {
  cr::CreativeObject object;
  object.id = 2;
  object.kind = cr::CreativeObjectKind::Group;
  object.name = "Creative Group";
  object.layerId = 4;
  object.tags = {"container"};
  return object;
}

cr::CreativeObject creativeWallObjectFixture() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Wall;
  object.name = "Creative Wall";
  object.transform.position = {1.0 / 3.0, 2.0, 0.125};
  object.transform.rotation = {0.0, 0.5, 0.0};
  object.transform.scale = {1.0, 2.0, 3.0};
  object.bounds = {{0.0, -0.25, 0.0}, {10.5, 4.25, 7.75}};
  object.layerId = 9;
  object.visible = false;
  object.locked = true;
  object.parentId = 2;
  object.tags = {"wall", "saved"};
  return object;
}

cr::CreativeDocumentRestoreRequest creativeRestoreFixture() {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9001;
  request.name = "Creative Save Document";
  request.units = cr::CreativeUnits::Meters;
  request.gridSettings = creativeGridFixture();
  request.snapSettings = creativeSnapFixture();
  request.worldBounds = creativeWorldBoundsFixture();
  request.nextObjectId = 100;
  request.objects = {creativeGroupObjectFixture(), creativeWallObjectFixture()};
  return request;
}

cr::CreativeDocument creativeDocumentFixture() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Before");
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(creativeRestoreFixture());
  if (!restored.accepted) {
    std::cerr << "creative document fixture restore failed\n";
  }
  return document;
}

bool sameCreativeObject(const cr::CreativeObject& lhs,
                        const cr::CreativeObject& rhs) {
  return lhs.id == rhs.id && lhs.kind == rhs.kind && lhs.name == rhs.name &&
         sameCreativeTransform(lhs.transform, rhs.transform) &&
         sameCreativeBounds(lhs.bounds, rhs.bounds) &&
         lhs.layerId == rhs.layerId && lhs.visible == rhs.visible &&
         lhs.locked == rhs.locked && lhs.parentId == rhs.parentId &&
         lhs.tags == rhs.tags;
}

bool sameCreativeDocumentContent(const cr::CreativeDocument& lhs,
                                 const cr::CreativeDocument& rhs) {
  if (lhs.id() != rhs.id() || lhs.name() != rhs.name() ||
      lhs.units() != rhs.units() ||
      !sameCreativeGridSettings(lhs.gridSettings(), rhs.gridSettings()) ||
      !sameCreativeSnapSettings(lhs.documentSnapSettings(),
                                rhs.documentSnapSettings()) ||
      !sameCreativeBounds(lhs.worldBounds(), rhs.worldBounds()) ||
      lhs.nextObjectId() != rhs.nextObjectId() ||
      lhs.objectCount() != rhs.objectCount()) {
    return false;
  }
  const std::span<const cr::CreativeObject> lhsObjects = lhs.objects();
  const std::span<const cr::CreativeObject> rhsObjects = rhs.objects();
  for (std::size_t index = 0; index < lhsObjects.size(); ++index) {
    if (!sameCreativeObject(lhsObjects[index], rhsObjects[index])) {
      return false;
    }
  }
  return true;
}

iggy3d::ProductCreativeSaveWriteRequest creativeSaveRequest(
    const std::filesystem::path& root,
    const cr::CreativeDocument& document,
    std::string_view attemptToken,
    std::string_view idHint = "") {
  iggy3d::ProductCreativeSaveWriteRequest request;
  request.saveRoot = root;
  request.saveIdHint = std::string{idHint};
  request.attemptToken = std::string{attemptToken};
  request.document = &document;
  request.packageId = "iggy3d.creative";
  request.scenarioId = "creative.document";
  request.worldId = "world_creative_0001";
  request.worldTitle = "Creative World";
  request.saveTitle = "Creative Manual Save";
  request.createdAtUtc = "2026-07-03T00:00:00Z";
  request.savedAtUtc = "2026-07-03T00:05:00Z";
  return request;
}

bool productDurableSaveWritesFinalAndScans() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001"));
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path)
                 : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");

  return expect(written.ok, "product durable write ok") &&
         expect(written.status == "product_save_written", "product status") &&
         expect(written.reasonCode == "product_save_written", "product reason") &&
         expect(written.durableReason == "durable_save_file_written",
                "product durable reason") &&
         expect(written.durableWriteRequested, "product durable requested") &&
         expect(written.record.id == "save_001", "product generated id") &&
         expect(written.record.path == written.paths.finalPath,
                "product record path") &&
         expect(written.record.savedStateHash == session.stateHash(),
                "product record hash") &&
         expect(written.tempWritten, "product temp written") &&
         expect(written.tempValidated, "product temp validated") &&
         expect(written.committed, "product committed") &&
         expect(written.finalValidated, "product final validated") &&
         expect(!written.previousExisted, "product no previous") &&
         expect(written.previousPreserved, "product previous preserved") &&
         expect(written.encodedBytes > 0U, "product encoded bytes") &&
         expect(written.worldId == "world_0001", "product world proof") &&
         expect(written.worldTitle == "Training World",
                "product world title proof") &&
         expect(written.saveTitle == "Manual Save",
                "product save title proof") &&
         expect(written.saveType == "manual", "product save type proof") &&
         expect(written.createdAtUtc == "2026-06-24T00:00:00Z",
                "product created utc proof") &&
         expect(written.savedAtUtc == "2026-06-24T01:02:03Z",
                "product saved utc proof") &&
         expect(read.ok, "product final read") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "product final decoded") &&
         expect(decoded.envelope.metadata.saveId == "save_001",
                "product metadata save id") &&
         expect(decoded.envelope.metadata.saveId == written.record.id,
                "product metadata id matches record") &&
         expect(decoded.envelope.metadata.worldId == "world_0001",
                "product metadata world id") &&
         expect(decoded.envelope.metadata.worldTitle == "Training World",
                "product metadata world title") &&
         expect(decoded.envelope.metadata.saveTitle == "Manual Save",
                "product metadata save title") &&
         expect(decoded.envelope.metadata.saveType == "manual",
                "product metadata save type") &&
         expect(decoded.envelope.metadata.createdAtUtc == "2026-06-24T00:00:00Z",
                "product metadata created utc") &&
         expect(decoded.envelope.metadata.savedAtUtc == "2026-06-24T01:02:03Z",
                "product metadata saved utc") &&
         expect(std::filesystem::exists(written.paths.finalPath),
                "product final exists") &&
         expect(!std::filesystem::exists(written.paths.tempPath),
                "product temp consumed") &&
         expect(scanned.status == "save_bridge_ready", "scan status") &&
         expect(scanned.scanMeasured, "scan measured") &&
         expect(scanned.scanStatus == "save_catalog_scan_ready",
                "scan measured status") &&
         expect(scanned.scanEntryCount == 1U, "scan measured count") &&
         expect(scanned.saveRoot == root, "scan root") &&
         expect(scanned.slots.slots.size() == 1U, "scan one slot") &&
         expect(scanned.slots.compatibleCount == 1U, "scan compatible") &&
         expect(scanned.slots.slots.front().id == "save_001", "scan id") &&
         expect(scanned.slots.slots.front().enabled, "scan enabled") &&
         expect(scanned.slots.slots.front().savedStateHashHex ==
                    written.record.savedStateHashHex,
                "scan hash hex");
}

bool worldIdMintMeasurementCountsActiveAndDeletedSaves() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult first =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_first"));
  const iggy3d::ProductSaveWriteResult second =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_second"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_first"});
  const iggy3d::ProductWorldIdMintResult minted =
      iggy3d::nextProductWorldIdMeasured(root);

  return expect(first.ok, "world id first write") &&
         expect(second.ok, "world id second write") &&
         expect(deleted.ok, "world id soft delete") &&
         expect(minted.worldId == "world_0002", "world id next keeps max") &&
         expect(minted.scanMeasured, "world id scan measured") &&
         expect(minted.scanStatus == "product_world_id_scan_ready",
                "world id scan status") &&
         expect(minted.scanEntryCount == 2U, "world id scan entry count");
}

bool productDurableSaveHonorsValidIdHint() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "manual_save_01"));
  return expect(written.ok, "product id hint ok") &&
         expect(written.record.id == "manual_save_01", "product id hint id") &&
         expect(written.paths.finalPath == root / "manual_save_01.iggy3d.save",
                "product id hint final");
}

bool productDurableSaveRejectsMissingState() {
  const std::filesystem::path root = testRoot();
  iggy3d::ProductSaveWriteRequest request;
  request.saveRoot = root;
  request.attemptToken = "attempt_001";
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(request);
  return expect(!written.ok, "product missing state rejected") &&
         expect(written.status == "save_state_missing",
                "product missing state status") &&
         expect(written.reasonCode == "save_state_missing",
                "product missing state reason") &&
         expect(written.durableReason == "save_state_missing",
                "product missing durable reason") &&
         expect(written.durableWriteRequested,
                "product missing durable requested") &&
         expect(!written.tempWritten, "product missing no temp") &&
         expect(!written.committed, "product missing not committed") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product missing no slots");
}

bool productDurableSaveRejectsInvalidAttemptToken() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt 001"));
  return expect(!written.ok, "product invalid attempt rejected") &&
         expect(written.status == "durable_save_invalid_attempt_token",
                "product invalid attempt status") &&
         expect(written.reasonCode == "durable_save_invalid_attempt_token",
                "product invalid attempt reason") &&
         expect(written.durableReason == "durable_save_invalid_attempt_token",
                "product invalid durable reason") &&
         expect(!written.tempWritten, "product invalid attempt no temp") &&
         expect(!written.committed, "product invalid attempt not committed") &&
         expect(written.paths.finalPath.empty(),
                "product invalid attempt no final path") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product invalid attempt no slots");
}

bool productDurableSavePersistsAuthoredRoom() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveAuthoredRoomSection authoredRoom = authoredRoomFixture();
  iggy3d::ProductSaveWriteRequest request =
      productSaveRequest(root, session, "attempt_001", "save_010");
  request.authoredRoom = &authoredRoom;
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(request);
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path)
                 : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(written.ok, "product authored write ok") &&
         expect(read.ok, "product authored read ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "product authored decoded") &&
         expect(decoded.envelope.authoredRoom.present,
                "product authored present") &&
         expect(decoded.envelope.authoredRoom.id == "product_bridge_room",
                "product authored id") &&
         expect(decoded.envelope.authoredRoom.floors.size() == 1U,
                "product authored floor") &&
         expect(decoded.envelope.authoredRoom.floors[0].locked,
                "product authored floor locked") &&
         expect(decoded.envelope.authoredRoom.floors[0].hidden,
                "product authored floor hidden") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.traversalTags[0] ==
                    "walkable",
                "product authored traversal") &&
         expect(decoded.envelope.authoredRoom.markers.size() == 1U,
                "product authored marker") &&
         expect(decoded.envelope.authoredRoom.markers[0].tag == "treasure",
                "product authored marker tag") &&
         expect(scanned.slots.slots.size() == 1U, "product authored scan slot") &&
         expect(scanned.slots.slots.front().authoredFloorCount == 1U,
                "product authored scan floor count") &&
         expect(scanned.slots.slots.front().authoredMarkerCount == 1U,
                "product authored scan marker count");
}

bool creativeDurableSaveWritesFinalAndLoads() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_001"));
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path)
                 : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};
  const iggy3d::ProductCreativeSaveLoadResult loaded =
      written.ok ? iggy3d::loadCreativeDocumentSave({written.record.path})
                 : iggy3d::ProductCreativeSaveLoadResult{};

  return expect(written.ok, "creative write ok") &&
         expect(written.status == "creative_save_written",
                "creative write status") &&
         expect(written.reasonCode == "creative_save_written",
                "creative write reason") &&
         expect(written.durableReason == "durable_save_file_written",
                "creative durable reason") &&
         expect(written.durableWriteRequested,
                "creative durable requested") &&
         expect(written.sectionReceipt.accepted,
                "creative section accepted") &&
         expect(written.sectionReceipt.reasonCode ==
                    "creative_document_section_converted",
                "creative section reason") &&
         expect(written.record.id == "save_001",
                "creative generated id") &&
         expect(written.record.path == written.paths.finalPath,
                "creative record path") &&
         expect(written.tempWritten, "creative temp written") &&
         expect(written.tempValidated, "creative temp validated") &&
         expect(written.committed, "creative committed") &&
         expect(written.finalValidated, "creative final validated") &&
         expect(!written.previousExisted, "creative no previous") &&
         expect(written.previousPreserved, "creative previous preserved") &&
         expect(written.encodedBytes > 0U, "creative encoded bytes") &&
         expect(written.packageId == "iggy3d.creative",
                "creative package mirror") &&
         expect(written.scenarioId == "creative.document",
                "creative scenario mirror") &&
         expect(written.worldId == "world_creative_0001",
                "creative world id mirror") &&
         expect(written.saveType == "creative",
                "creative save type mirror") &&
         expect(written.documentId == document.id(),
                "creative document id mirror") &&
         expect(written.creativeObjectCount == document.objectCount(),
                "creative object count mirror") &&
         expect(written.creativeNextObjectId == document.nextObjectId(),
                "creative next id mirror") &&
         expect(std::filesystem::exists(written.paths.finalPath),
                "creative final exists") &&
         expect(!std::filesystem::exists(written.paths.tempPath),
                "creative temp consumed") &&
         expect(read.ok, "creative read ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "creative decoded") &&
         expect(decoded.envelope.metadata.saveId == "save_001",
                "creative metadata save id") &&
         expect(decoded.envelope.metadata.worldId == "world_creative_0001",
                "creative metadata world id") &&
         expect(decoded.envelope.metadata.saveTitle == "Creative Manual Save",
                "creative metadata save title") &&
         expect(decoded.envelope.metadata.savedStateHash == 0U,
                "creative metadata zero hash") &&
         expect(decoded.envelope.metadata.savedStateHashHex ==
                    "0000000000000000",
                "creative metadata zero hash hex") &&
         expect(decoded.envelope.creativeDocument.present,
                "creative section present") &&
         expect(decoded.envelope.creativeDocument.documentId == document.id(),
                "creative section document id") &&
         expect(decoded.envelope.creativeDocument.objects.size() == 2U,
                "creative section object count") &&
         expect(decoded.envelope.creativeDocument.objects[1].kind == "Wall",
                "creative section kind string") &&
         expect(decoded.envelope.creativeDocument.objects[1].hasParent,
                "creative section parent flag") &&
         expect(decoded.envelope.creativeDocument.objects[1].parentId == 2U,
                "creative section parent id") &&
         expect(loaded.ok, "creative load ok") &&
         expect(loaded.status == "creative_save_loaded",
                "creative load status") &&
         expect(loaded.fileRead, "creative load read") &&
         expect(loaded.decoded, "creative load decoded") &&
         expect(loaded.sectionRestored, "creative load section restored") &&
         expect(loaded.sectionReceipt.accepted,
                "creative load section accepted") &&
         expect(loaded.record.id == "save_001", "creative load record id") &&
         expect(loaded.packageId == "iggy3d.creative",
                "creative load package mirror") &&
         expect(loaded.worldTitle == "Creative World",
                "creative load world title mirror") &&
         expect(loaded.documentId == document.id(),
                "creative load document id mirror") &&
         expect(loaded.creativeObjectCount == document.objectCount(),
                "creative load object count mirror") &&
         expect(loaded.creativeNextObjectId == document.nextObjectId(),
                "creative load next id mirror") &&
         expect(loaded.document.revision() == 0U,
                "creative loaded revision zero") &&
         expect(loaded.document.dirtyFlags() == 0U,
                "creative loaded dirty zero") &&
         expect(sameCreativeDocumentContent(loaded.document, document),
                "creative loaded document content");
}

bool creativeDurableSaveScansAsCreativeAndNotProductLoadable() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_001", "creative_save"));
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.creative", "creative.document");

  const bool hasEntry = scanned.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry entry =
      hasEntry ? scanned.catalog.catalog.entries.front()
               : iggy3d::ProductSaveCatalogEntry{};
  const bool hasSlot = scanned.slots.slots.size() == 1U;
  const iggy3d::SaveSlotPreview slot =
      hasSlot ? scanned.slots.slots.front() : iggy3d::SaveSlotPreview{};

  return expect(written.ok, "creative scan setup write ok") &&
         expect(hasEntry, "creative scan one catalog entry") &&
         expect(entry.contentKind ==
                    iggy3d::ProductSaveContentKind::CreativeDocument,
                "creative scan content kind") &&
         expect(iggy3d::productSaveContentKindName(entry.contentKind) ==
                    "creative_document",
                "creative scan content kind name") &&
         expect(entry.creativeDocumentPresent,
                "creative scan document present mirror") &&
         expect(entry.creativeDocumentId == document.id(),
                "creative scan document id mirror") &&
         expect(entry.creativeObjectCount == document.objectCount(),
                "creative scan object count mirror") &&
         expect(entry.creativeNextObjectId == document.nextObjectId(),
                "creative scan next id mirror") &&
         expect(entry.compatible, "creative scan compatible") &&
         expect(!entry.loadable, "creative scan not product loadable") &&
         expect(!iggy3d::canLoadProductSave(entry),
                "creative scan canLoad false") &&
         expect(iggy3d::canOpenCreativeWorld(entry),
                "creative scan canOpen true") &&
         expect(entry.disabledReason == "creative_save_not_product_loadable",
                "creative scan disabled reason") &&
         expect(scanned.catalog.compatibleActiveCount == 0U,
                "creative scan no product-compatible active count") &&
         expect(hasSlot, "creative scan one slot") &&
         expect(!slot.enabled, "creative active slot disabled") &&
         expect(slot.compatibility == iggy3d::SaveSlotCompatibility::Compatible,
                "creative active slot compatible") &&
         expect(slot.reason == "creative_save_not_product_loadable",
                "creative active slot reason") &&
         expect(scanned.slots.compatibleCount == 0U,
                "creative active slot not counted enabled");
}

bool productContinueIgnoresCreativeSaveWhenNewestInScan() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult product =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult creative =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_002", "save_999"));
  const iggy3d::ProductSaveBridgeResult scanned =
      iggy3d::scanProductSaves(root, "", "");
  const iggy3d::ProductContinueSelectionResult selected =
      iggy3d::selectProductContinueSave(scanned.catalog.catalog);

  return expect(product.ok, "mixed scan product write ok") &&
         expect(creative.ok, "mixed scan creative write ok") &&
         expect(scanned.catalog.catalog.entries.size() == 2U,
                "mixed scan two entries") &&
         expect(scanned.catalog.compatibleActiveCount == 1U,
                "mixed scan one product-loadable entry") &&
         expect(selected.selected, "mixed scan selected product continue") &&
         expect(selected.selectedSaveId == "save_001",
                "mixed scan product continue ignores creative newest") &&
         expect(selected.consideredCount == 2U,
                "mixed scan continue considered active rows") &&
         expect(selected.compatibleCount == 1U,
                "mixed scan continue compatible product count");
}

bool creativeDurableSaveHonorsValidIdHint() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_001", "creative_manual"));
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path)
                 : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};

  return expect(written.ok, "creative id hint ok") &&
         expect(written.record.id == "creative_manual",
                "creative id hint record") &&
         expect(written.paths.finalPath == root / "creative_manual.iggy3d.save",
                "creative id hint path") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "creative id hint decoded") &&
         expect(decoded.envelope.metadata.saveId == "creative_manual",
                "creative id hint metadata");
}

bool creativeDurableSaveCarriesExistingIdentityOnOverwrite() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult initial =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_001", "creative_manual"));

  cr::CreativeDocument replacement = creativeDocumentFixture();
  cr::CreativeDocumentCreateRequest createRequest;
  createRequest.kind = cr::CreativeObjectKind::Room;
  const cr::CreativeDocumentCreateReceipt created =
      replacement.createObject(createRequest);
  iggy3d::ProductCreativeSaveWriteRequest request =
      creativeSaveRequest(root, replacement, "attempt_002", "creative_manual");
  request.packageId = "iggy3d.creative.replacement";
  request.scenarioId = "creative.replacement";
  request.worldId.clear();
  request.worldTitle.clear();
  request.saveTitle.clear();
  request.saveType.clear();
  request.createdAtUtc.clear();
  request.savedAtUtc.clear();

  const iggy3d::ProductCreativeSaveWriteResult overwritten =
      iggy3d::writeCreativeDocumentSaveDurably(request);
  const iggy3d::SaveFileReadResult read =
      overwritten.ok ? iggy3d::readSaveFile(overwritten.record.path)
                     : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};

  return expect(initial.ok, "creative overwrite setup ok") &&
         expect(created.accepted, "creative overwrite changed document") &&
         expect(overwritten.ok, "creative overwrite ok") &&
         expect(overwritten.previousExisted,
                "creative overwrite previous existed") &&
         expect(overwritten.packageId == "iggy3d.creative.replacement",
                "creative overwrite package from request") &&
         expect(overwritten.scenarioId == "creative.replacement",
                "creative overwrite scenario from request") &&
         expect(overwritten.worldId == "world_creative_0001",
                "creative overwrite carries world id") &&
         expect(overwritten.worldTitle == "Creative World",
                "creative overwrite carries world title") &&
         expect(overwritten.saveTitle == "Creative Manual Save",
                "creative overwrite carries save title") &&
         expect(overwritten.saveType == "creative",
                "creative overwrite carries save type") &&
         expect(overwritten.createdAtUtc == "2026-07-03T00:00:00Z",
                "creative overwrite carries created utc") &&
         expect(overwritten.savedAtUtc == "2026-07-03T00:05:00Z",
                "creative overwrite carries saved utc") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "creative overwrite decoded") &&
         expect(decoded.envelope.metadata.packageId ==
                    "iggy3d.creative.replacement",
                "creative overwrite encoded package") &&
         expect(decoded.envelope.metadata.scenarioId == "creative.replacement",
                "creative overwrite encoded scenario") &&
         expect(decoded.envelope.metadata.worldId == "world_creative_0001",
                "creative overwrite encoded world id") &&
         expect(decoded.envelope.metadata.saveTitle == "Creative Manual Save",
                "creative overwrite encoded save title") &&
         expect(decoded.envelope.creativeDocument.objects.size() ==
                    replacement.objectCount(),
                "creative overwrite encoded replacement document");
}

bool creativeDurableSaveRejectsNullDocument() {
  const std::filesystem::path root = testRoot();
  iggy3d::ProductCreativeSaveWriteRequest request;
  request.saveRoot = root;
  request.attemptToken = "attempt_001";
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(request);

  return expect(!written.ok, "creative null document rejected") &&
         expect(written.status == "creative_save_document_missing",
                "creative null document status") &&
         expect(written.reasonCode == "creative_save_document_missing",
                "creative null document reason") &&
         expect(written.durableReason == "creative_save_document_missing",
                "creative null durable reason") &&
         expect(!written.durableWriteRequested,
                "creative null no durable") &&
         expect(!written.tempWritten, "creative null no temp") &&
         expect(!written.committed, "creative null no commit") &&
         expect(iggy3d::listSaveFiles(root).empty(),
                "creative null no save");
}

bool creativeDurableSaveRejectsInvalidDocumentId() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document =
      cr::CreativeDocument::create("Missing Id");
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_001"));

  return expect(!written.ok, "creative invalid document id rejected") &&
         expect(written.status == "invalid_document_id",
                "creative invalid document id status") &&
         expect(written.reasonCode == "invalid_document_id",
                "creative invalid document id reason") &&
         expect(written.sectionReceipt.requested,
                "creative invalid section requested") &&
         expect(written.sectionReceipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        InvalidDocumentId,
                "creative invalid section status") &&
         expect(!written.durableWriteRequested,
                "creative invalid no durable") &&
         expect(iggy3d::listSaveFiles(root).empty(),
                "creative invalid no save");
}

bool creativeDurableSaveRejectsInvalidAttemptToken() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt 001"));

  return expect(!written.ok, "creative invalid attempt rejected") &&
         expect(written.status == "durable_save_invalid_attempt_token",
                "creative invalid attempt status") &&
         expect(written.reasonCode == "durable_save_invalid_attempt_token",
                "creative invalid attempt reason") &&
         expect(written.durableReason == "durable_save_invalid_attempt_token",
                "creative invalid durable reason") &&
         expect(written.sectionReceipt.accepted,
                "creative invalid attempt section accepted") &&
         expect(written.durableWriteRequested,
                "creative invalid attempt durable requested") &&
         expect(!written.tempWritten,
                "creative invalid attempt no temp") &&
         expect(!written.committed,
                "creative invalid attempt no commit") &&
         expect(written.paths.finalPath.empty(),
                "creative invalid attempt no final") &&
         expect(iggy3d::listSaveFiles(root).empty(),
                "creative invalid attempt no save");
}

bool creativeLoadRejectsSessionSaveWithoutCreativeSection() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductCreativeSaveLoadResult loaded =
      written.ok ? iggy3d::loadCreativeDocumentSave({written.record.path})
                 : iggy3d::ProductCreativeSaveLoadResult{};

  return expect(written.ok, "creative missing setup session write") &&
         expect(!loaded.ok, "creative missing section rejected") &&
         expect(loaded.status == "missing_creative_document_section",
                "creative missing section status") &&
         expect(loaded.reasonCode == "missing_creative_document_section",
                "creative missing section reason") &&
         expect(loaded.fileRead, "creative missing section read") &&
         expect(loaded.decoded, "creative missing section decoded") &&
         expect(!loaded.sectionRestored,
                "creative missing section not restored") &&
         expect(loaded.sectionReceipt.status ==
                    iggy3d::ProductCreativeDocumentSectionStatus::
                        MissingSection,
                "creative missing section receipt") &&
         expect(loaded.document.objectCount() == 0U,
                "creative missing section empty document");
}

bool productSoftDeleteMovesSaveAndRemovesFromScan() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(written.ok, "product soft delete setup write ok") &&
         expect(deleted.ok, "product soft delete ok") &&
         expect(deleted.status == "product_save_soft_deleted",
                "product soft delete status") &&
         expect(deleted.reasonCode == "product_save_soft_deleted",
                "product soft delete reason") &&
         expect(deleted.softDeleteReason == "soft_delete_moved",
                "product soft delete runtime reason") &&
         expect(deleted.saveId == "save_001", "product soft delete id") &&
         expect(deleted.saveMoved, "product soft delete save moved") &&
         expect(deleted.snapshotMissing, "product soft delete snapshot missing") &&
         expect(!deleted.snapshotMoved, "product soft delete snapshot not moved") &&
         expect(!std::filesystem::exists(deleted.paths.activeSavePath),
                "product soft delete active gone") &&
         expect(std::filesystem::exists(deleted.paths.deletedSavePath),
                "product soft delete deleted exists") &&
         expect(scanned.slots.slots.empty(), "product soft delete not scanned");
}

bool productSoftDeleteMovesSnapshotSidecarWhenPresent() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  {
    std::ofstream snapshot(root / "save_001.snapshot.png");
    snapshot << "snapshot bytes";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  return expect(written.ok, "product soft delete snapshot setup write ok") &&
         expect(deleted.ok, "product soft delete snapshot ok") &&
         expect(deleted.snapshotMoved, "product soft delete snapshot moved") &&
         expect(!deleted.snapshotMissing,
                "product soft delete snapshot not missing") &&
         expect(!std::filesystem::exists(deleted.paths.activeSnapshotPath),
                "product soft delete active snapshot gone") &&
         expect(std::filesystem::exists(deleted.paths.deletedSnapshotPath),
                "product soft delete deleted snapshot exists");
}

bool productSoftDeleteRejectsMissingIdBeforeIo() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, ""});
  return expect(!deleted.ok, "product soft delete missing id rejected") &&
         expect(deleted.status == "product_save_delete_id_missing",
                "product soft delete missing id status") &&
         expect(deleted.reasonCode == "product_save_delete_id_missing",
                "product soft delete missing id reason") &&
         expect(deleted.softDeleteReason == "not_requested",
                "product soft delete missing id no runtime") &&
         expect(deleted.saveId == "none", "product soft delete missing id none") &&
         expect(!deleted.saveMoved, "product soft delete missing id not moved") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product soft delete missing id no scan");
}

bool productSoftDeleteRejectsInvalidId() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save/001"});
  return expect(!deleted.ok, "product soft delete invalid id rejected") &&
         expect(deleted.status == "soft_delete_invalid_id",
                "product soft delete invalid id status") &&
         expect(deleted.reasonCode == "soft_delete_invalid_id",
                "product soft delete invalid id reason") &&
         expect(deleted.softDeleteReason == "soft_delete_invalid_id",
                "product soft delete invalid runtime") &&
         expect(deleted.saveId == "save/001", "product soft delete invalid id proof") &&
         expect(deleted.paths.activeSavePath.empty(),
                "product soft delete invalid no active path");
}

bool productSoftDeleteForwardsMissingSource() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  return expect(!deleted.ok, "product soft delete missing source rejected") &&
         expect(deleted.status == "soft_delete_source_missing",
                "product soft delete missing source status") &&
         expect(deleted.reasonCode == "soft_delete_source_missing",
                "product soft delete missing source reason") &&
         expect(deleted.softDeleteReason == "soft_delete_source_missing",
                "product soft delete missing source runtime") &&
         expect(!deleted.saveMoved, "product soft delete missing source not moved") &&
         expect(!std::filesystem::exists(deleted.paths.deletedSavePath),
                "product soft delete missing source no deleted file");
}

bool productSoftDeleteRejectsExistingDeletedTarget() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::SaveFileSoftDeletePlan plan =
      iggy3d::planSoftDeleteSaveFile(root, "save_001");
  std::filesystem::create_directories(plan.paths.deletedSavePath.parent_path());
  {
    std::ofstream existing(plan.paths.deletedSavePath);
    existing << "existing deleted target";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  return expect(written.ok, "product soft delete collision setup write ok") &&
         expect(!deleted.ok, "product soft delete collision rejected") &&
         expect(deleted.status == "soft_delete_target_exists",
                "product soft delete collision status") &&
         expect(deleted.reasonCode == "soft_delete_target_exists",
                "product soft delete collision reason") &&
         expect(deleted.softDeleteReason == "soft_delete_target_exists",
                "product soft delete collision runtime") &&
         expect(deleted.targetExisted, "product soft delete collision flag") &&
         expect(!deleted.saveMoved, "product soft delete collision not moved") &&
         expect(std::filesystem::exists(plan.paths.activeSavePath),
                "product soft delete collision active preserved") &&
         expect(std::filesystem::exists(plan.paths.deletedSavePath),
                "product soft delete collision deleted preserved");
}

bool productRecoverRestoresSoftDeletedSaveAndScans() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(written.ok, "product recover setup write ok") &&
         expect(deleted.ok, "product recover setup soft delete ok") &&
         expect(recovered.ok, "product recover ok") &&
         expect(recovered.status == "product_save_recovered",
                "product recover status") &&
         expect(recovered.reasonCode == "product_save_recovered",
                "product recover reason") &&
         expect(recovered.recoverReason == "recover_save_moved",
                "product recover runtime reason") &&
         expect(recovered.saveId == "save_001", "product recover id") &&
         expect(recovered.saveRecovered, "product recover save recovered") &&
         expect(recovered.snapshotMissing, "product recover snapshot missing") &&
         expect(!recovered.snapshotRecovered,
                "product recover snapshot not recovered") &&
         expect(std::filesystem::exists(recovered.paths.activeSavePath),
                "product recover active exists") &&
         expect(!std::filesystem::exists(recovered.paths.deletedSavePath),
                "product recover deleted gone") &&
         expect(scanned.slots.slots.size() == 1U, "product recover scanned") &&
         expect(scanned.slots.compatibleCount == 1U,
                "product recover compatible") &&
         expect(scanned.slots.slots.front().id == "save_001",
                "product recover scan id");
}

bool productRecoverMovesSnapshotSidecarWhenPresent() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  {
    std::ofstream snapshot(root / "save_001.snapshot.png");
    snapshot << "snapshot bytes";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  return expect(written.ok, "product recover snapshot setup write ok") &&
         expect(deleted.ok, "product recover snapshot setup delete ok") &&
         expect(recovered.ok, "product recover snapshot ok") &&
         expect(recovered.snapshotRecovered,
                "product recover snapshot recovered") &&
         expect(!recovered.snapshotMissing,
                "product recover snapshot not missing") &&
         expect(std::filesystem::exists(recovered.paths.activeSnapshotPath),
                "product recover active snapshot exists") &&
         expect(!std::filesystem::exists(recovered.paths.deletedSnapshotPath),
                "product recover deleted snapshot gone");
}

bool productRecoverRejectsMissingIdBeforeIo() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, ""});
  return expect(!recovered.ok, "product recover missing id rejected") &&
         expect(recovered.status == "product_save_recover_id_missing",
                "product recover missing id status") &&
         expect(recovered.reasonCode == "product_save_recover_id_missing",
                "product recover missing id reason") &&
         expect(recovered.recoverReason == "not_requested",
                "product recover missing id no runtime") &&
         expect(recovered.saveId == "none", "product recover missing id none") &&
         expect(!recovered.saveRecovered,
                "product recover missing id not recovered") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product recover missing id no scan");
}

bool productRecoverRejectsInvalidId() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save/001"});
  return expect(!recovered.ok, "product recover invalid id rejected") &&
         expect(recovered.status == "recover_save_invalid_id",
                "product recover invalid id status") &&
         expect(recovered.reasonCode == "recover_save_invalid_id",
                "product recover invalid id reason") &&
         expect(recovered.recoverReason == "recover_save_invalid_id",
                "product recover invalid runtime") &&
         expect(recovered.saveId == "save/001",
                "product recover invalid id proof") &&
         expect(recovered.paths.activeSavePath.empty(),
                "product recover invalid no active path");
}

bool productRecoverForwardsMissingSource() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  return expect(!recovered.ok, "product recover missing source rejected") &&
         expect(recovered.status == "recover_save_source_missing",
                "product recover missing source status") &&
         expect(recovered.reasonCode == "recover_save_source_missing",
                "product recover missing source reason") &&
         expect(recovered.recoverReason == "recover_save_source_missing",
                "product recover missing source runtime") &&
         expect(!recovered.saveRecovered,
                "product recover missing source not recovered") &&
         expect(!std::filesystem::exists(recovered.paths.activeSavePath),
                "product recover missing source no active file");
}

bool productRecoverRejectsExistingActiveTarget() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::SaveFileRecoverPlan plan =
      iggy3d::planRecoverDeletedSaveFile(root, "save_001");
  std::filesystem::create_directories(plan.paths.deletedSavePath.parent_path());
  {
    std::ofstream deleted(plan.paths.deletedSavePath);
    deleted << "deleted target";
  }
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  return expect(written.ok, "product recover collision setup write ok") &&
         expect(!recovered.ok, "product recover collision rejected") &&
         expect(recovered.status == "recover_save_target_exists",
                "product recover collision status") &&
         expect(recovered.reasonCode == "recover_save_target_exists",
                "product recover collision reason") &&
         expect(recovered.recoverReason == "recover_save_target_exists",
                "product recover collision runtime") &&
         expect(recovered.targetExisted, "product recover collision flag") &&
         expect(!recovered.saveRecovered,
                "product recover collision not recovered") &&
         expect(std::filesystem::exists(plan.paths.activeSavePath),
                "product recover collision active preserved") &&
         expect(std::filesystem::exists(plan.paths.deletedSavePath),
                "product recover collision deleted preserved");
}

bool productDeletedScanShowsSoftDeletedSaveThenClearsOnRecover() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult activeAfterDelete =
      iggy3d::scanProductSaves(root, "iggy3d.movement_playground",
                               "movement_playground.runtime_loop");
  const iggy3d::ProductSaveBridgeResult deletedAfterDelete =
      iggy3d::scanDeletedProductSaves(root, "iggy3d.movement_playground",
                                      "movement_playground.runtime_loop");
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult activeAfterRecover =
      iggy3d::scanProductSaves(root, "iggy3d.movement_playground",
                               "movement_playground.runtime_loop");
  const iggy3d::ProductSaveBridgeResult deletedAfterRecover =
      iggy3d::scanDeletedProductSaves(root, "iggy3d.movement_playground",
                                      "movement_playground.runtime_loop");
  return expect(written.ok, "product deleted scan setup write ok") &&
         expect(deleted.ok, "product deleted scan setup delete ok") &&
         expect(activeAfterDelete.status == "save_bridge_ready",
                "product deleted scan active status") &&
         expect(activeAfterDelete.slots.slots.empty(),
                "product deleted scan active empty") &&
         expect(deletedAfterDelete.status == "deleted_save_bridge_ready",
                "product deleted scan status") &&
         expect(deletedAfterDelete.saveRoot == root / "deleted",
                "product deleted scan root") &&
         expect(deletedAfterDelete.slots.slots.size() == 1U,
                "product deleted scan one slot") &&
         expect(deletedAfterDelete.slots.compatibleCount == 1U,
                "product deleted scan compatible") &&
         expect(deletedAfterDelete.slots.slots.front().id == "save_001",
                "product deleted scan id") &&
         expect(deletedAfterDelete.slots.slots.front().enabled,
                "product deleted scan enabled") &&
         expect(recovered.ok, "product deleted scan recover ok") &&
         expect(deletedAfterRecover.slots.slots.empty(),
                "product deleted scan empty after recover") &&
         expect(activeAfterRecover.slots.slots.size() == 1U,
                "product active scan restored") &&
         expect(activeAfterRecover.slots.slots.front().id == "save_001",
                "product active scan restored id");
}

bool productDeletedScanUsesMovedSnapshotSidecar() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  {
    std::ofstream snapshot(root / "save_001.snapshot.png");
    snapshot << "snapshot bytes";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult deletedScan =
      iggy3d::scanDeletedProductSaves(root, "iggy3d.movement_playground",
                                      "movement_playground.runtime_loop");
  return expect(written.ok, "product deleted snapshot setup write ok") &&
         expect(deleted.ok, "product deleted snapshot soft delete ok") &&
         expect(deleted.snapshotMoved, "product deleted snapshot moved") &&
         expect(deletedScan.slots.slots.size() == 1U,
                "product deleted snapshot scanned") &&
         expect(deletedScan.slots.slots.front().snapshotPath ==
                    root / "deleted" / "save_001.snapshot.png",
                "product deleted snapshot path") &&
         expect(deletedScan.slots.slots.front().snapshotAvailable,
                "product deleted snapshot available") &&
         expect(!deletedScan.slots.slots.front().snapshotFallback,
                "product deleted snapshot no fallback") &&
         expect(deletedScan.slots.slots.front().snapshotStatus == "available",
                "product deleted snapshot status");
}

bool deletedCreativeSaveRemainsRecoverableButNotProductLoadable() {
  const std::filesystem::path root = testRoot();
  const cr::CreativeDocument document = creativeDocumentFixture();
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(
          creativeSaveRequest(root, document, "attempt_001", "creative_save"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "creative_save"});
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanDeletedProductSaves(
      root, "iggy3d.creative", "creative.document");

  const bool hasEntry = scanned.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry entry =
      hasEntry ? scanned.catalog.catalog.entries.front()
               : iggy3d::ProductSaveCatalogEntry{};
  const bool hasSlot = scanned.slots.slots.size() == 1U;
  const iggy3d::SaveSlotPreview slot =
      hasSlot ? scanned.slots.slots.front() : iggy3d::SaveSlotPreview{};

  return expect(written.ok, "deleted creative setup write ok") &&
         expect(deleted.ok, "deleted creative soft delete ok") &&
         expect(hasEntry, "deleted creative one catalog entry") &&
         expect(entry.contentKind ==
                    iggy3d::ProductSaveContentKind::CreativeDocument,
                "deleted creative content kind") &&
         expect(entry.deleted, "deleted creative entry deleted") &&
         expect(!entry.loadable, "deleted creative not product loadable") &&
         expect(entry.recoverable, "deleted creative recoverable") &&
         expect(!iggy3d::canLoadProductSave(entry),
                "deleted creative canLoad false") &&
         expect(!iggy3d::canOpenCreativeWorld(entry),
                "deleted creative canOpen false while deleted") &&
         expect(entry.disabledReason == "none",
                "deleted creative no disabled reason") &&
         expect(hasSlot, "deleted creative one slot") &&
         expect(slot.enabled, "deleted creative slot enabled for recovery") &&
         expect(slot.reason == "compatible",
                "deleted creative slot recovery reason");
}

bool corruptCatalogEntryKeepsUnknownContentKind() {
  const std::filesystem::path root = testRoot();
  {
    std::ofstream output(root / "save_corrupt.iggy3d.save");
    output << "not an iggy3d save\n";
  }
  const iggy3d::ProductSaveBridgeResult scanned =
      iggy3d::scanProductSaves(root, "", "");
  const bool hasEntry = scanned.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry entry =
      hasEntry ? scanned.catalog.catalog.entries.front()
               : iggy3d::ProductSaveCatalogEntry{};

  return expect(hasEntry, "corrupt catalog one entry") &&
         expect(entry.corrupt, "corrupt catalog marked corrupt") &&
         expect(entry.contentKind == iggy3d::ProductSaveContentKind::Unknown,
                "corrupt catalog content kind unknown") &&
         expect(!entry.creativeDocumentPresent,
                "corrupt catalog creative mirror absent") &&
         expect(!iggy3d::canLoadProductSave(entry),
                "corrupt catalog canLoad false") &&
         expect(!iggy3d::canOpenCreativeWorld(entry),
                "corrupt catalog canOpen false");
}

bool productLoadSaveLoadsCompatibleSession() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, savedSession, "attempt_001", "save_001"));
  iggy3d::Session destination = makeFixtureSession();
  const std::uint64_t previousHash = destination.stateHash();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product load setup write ok") &&
         expect(loaded.ok, "product load ok") &&
         expect(loaded.status == "product_save_loaded", "product load status") &&
         expect(loaded.reasonCode == "product_save_loaded",
                "product load reason") &&
         expect(loaded.fileRead, "product load file read") &&
         expect(loaded.decoded, "product load decoded") &&
         expect(loaded.compatibilityChecked, "product load compatibility") &&
         expect(loaded.sessionLoaded, "product load session loaded") &&
         expect(loaded.record.id == "save_001", "product load record id") &&
         expect(loaded.record.path == written.record.path,
                "product load record path") &&
         expect(loaded.previousHash == previousHash,
                "product load previous hash") &&
         expect(loaded.loadedHash == written.record.savedStateHash,
                "product load loaded hash") &&
         expect(destination.stateHash() == written.record.savedStateHash,
                "product load destination hash") &&
         expect(destination.stateHash() != previousHash,
                "product load mutated destination") &&
         expect(loaded.codecStatus == iggy3d::SaveCodecStatus::Ok,
                "product load codec ok") &&
         expect(loaded.loadStatus == iggy3d::SaveLoadStatus::Ok,
                "product load status ok") &&
         expect(loaded.compatibilityStatus ==
                    iggy3d::SaveCompatibilityStatus::Compatible,
                "product load compatible") &&
         expect(loaded.sessionLoadStatus == iggy3d::SessionLoadStatus::Ok,
                "product load session status");
}

bool productLoadSaveExposesAuthoredRoomSection() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  iggy3d::SaveAuthoredRoomSection authoredRoom = authoredRoomFixture();
  iggy3d::ProductSaveWriteRequest writeRequest =
      productSaveRequest(root, savedSession, "attempt_001", "save_001");
  writeRequest.authoredRoom = &authoredRoom;
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(writeRequest);
  iggy3d::Session destination = makeFixtureSession();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product authored load setup write ok") &&
         expect(loaded.ok, "product authored load ok") &&
         expect(loaded.authoredRoomPresent,
                "product authored load room present") &&
         expect(loaded.authoredRoomId == "product_bridge_room",
                "product authored load room id") &&
         expect(loaded.authoredFloorCount == 1U,
                "product authored load floor count") &&
         expect(loaded.authoredWallCount == 0U,
                "product authored load wall count") &&
         expect(loaded.authoredMarkerCount == 1U,
                "product authored load marker count") &&
         expect(loaded.authoredRoom.present,
                "product authored load copied room present") &&
         expect(loaded.authoredRoom.floors.size() == 1U,
                "product authored load copied floor") &&
         expect(loaded.authoredRoom.markers.size() == 1U,
                "product authored load copied marker") &&
         expect(loaded.sessionLoaded,
                "product authored load session loaded");
}

bool productLoadSaveRejectsMissingSessionBeforeIo() {
  iggy3d::ProductSaveLoadRequest request;
  request.path = "/tmp/iggy3d_product_save_bridge_tests_missing_session.iggy3d.save";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product load missing session rejected") &&
         expect(loaded.status == "product_save_load_session_missing",
                "product load missing session status") &&
         expect(loaded.reasonCode == "product_save_load_session_missing",
                "product load missing session reason") &&
         expect(!loaded.fileRead, "product load missing session no read") &&
         expect(!loaded.sessionLoaded,
                "product load missing session not loaded");
}

bool productLoadSaveRejectsMissingPathBeforeIo() {
  iggy3d::Session session = makeFixtureSession();
  const std::uint64_t previousHash = session.stateHash();
  iggy3d::ProductSaveLoadRequest request;
  request.session = &session;
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product load missing path rejected") &&
         expect(loaded.status == "product_save_load_path_missing",
                "product load missing path status") &&
         expect(loaded.reasonCode == "product_save_load_path_missing",
                "product load missing path reason") &&
         expect(!loaded.fileRead, "product load missing path no read") &&
         expect(session.stateHash() == previousHash,
                "product load missing path no mutation");
}

bool productLoadSaveMissingFilePreservesSession() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const std::uint64_t previousHash = session.stateHash();
  iggy3d::ProductSaveLoadRequest request;
  request.path = root / "missing.iggy3d.save";
  request.session = &session;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product load missing file rejected") &&
         expect(loaded.status == "save_file_read_failed",
                "product load missing file status") &&
         expect(loaded.reasonCode == "save_file_read_failed",
                "product load missing file reason") &&
         expect(!loaded.fileRead, "product load missing file no read") &&
         expect(!loaded.sessionLoaded,
                "product load missing file not loaded") &&
         expect(session.stateHash() == previousHash,
                "product load missing file no mutation");
}

bool productLoadSaveRejectsIncompatiblePackage() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, savedSession, "attempt_001", "save_001"));
  iggy3d::Session destination = makeFixtureSession();
  const std::uint64_t previousHash = destination.stateHash();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "wrong.package";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product incompatible setup write ok") &&
         expect(!loaded.ok, "product incompatible rejected") &&
         expect(loaded.status == "product_save_load_compatibility_failed",
                "product incompatible status") &&
         expect(loaded.reasonCode == "product_save_load_compatibility_failed",
                "product incompatible reason") &&
         expect(loaded.fileRead, "product incompatible read") &&
         expect(loaded.decoded, "product incompatible decoded") &&
         expect(loaded.compatibilityChecked,
                "product incompatible compatibility checked") &&
         expect(loaded.compatibilityStatus ==
                    iggy3d::SaveCompatibilityStatus::PackageMismatch,
                "product incompatible package status") &&
         expect(!loaded.sessionLoaded, "product incompatible not loaded") &&
         expect(destination.stateHash() == previousHash,
                "product incompatible no mutation");
}

bool productLoadSaveRejectsIncompatibleScenario() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, savedSession, "attempt_001", "save_001"));
  iggy3d::Session destination = makeFixtureSession();
  const std::uint64_t previousHash = destination.stateHash();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "wrong.scenario";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product scenario setup write ok") &&
         expect(!loaded.ok, "product scenario rejected") &&
         expect(loaded.status == "product_save_load_compatibility_failed",
                "product scenario status") &&
         expect(loaded.compatibilityStatus ==
                    iggy3d::SaveCompatibilityStatus::ScenarioMismatch,
                "product scenario status enum") &&
         expect(!loaded.sessionLoaded, "product scenario not loaded") &&
         expect(destination.stateHash() == previousHash,
                "product scenario no mutation");
}

bool productLoadSaveRejectsCorruptFile() {
  const std::filesystem::path root = testRoot();
  const std::filesystem::path path = root / "corrupt.iggy3d.save";
  {
    std::ofstream output(path);
    output << "not an iggy3d save\n";
  }
  iggy3d::Session session = makeFixtureSession();
  const std::uint64_t previousHash = session.stateHash();
  iggy3d::ProductSaveLoadRequest request;
  request.path = path;
  request.session = &session;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product corrupt rejected") &&
         expect(loaded.status == "save_file_decode_failed",
                "product corrupt status") &&
         expect(loaded.reasonCode == "save_file_decode_failed",
                "product corrupt reason") &&
         expect(!loaded.fileRead, "product corrupt file not accepted") &&
         expect(!loaded.sessionLoaded, "product corrupt not loaded") &&
         expect(session.stateHash() == previousHash,
                "product corrupt no mutation");
}

}  // namespace

int main() {
  const bool ok = productDurableSaveWritesFinalAndScans() &&
                  worldIdMintMeasurementCountsActiveAndDeletedSaves() &&
                  productDurableSaveHonorsValidIdHint() &&
                  productDurableSaveRejectsMissingState() &&
                  productDurableSaveRejectsInvalidAttemptToken() &&
                  productDurableSavePersistsAuthoredRoom() &&
                  creativeDurableSaveWritesFinalAndLoads() &&
                  creativeDurableSaveScansAsCreativeAndNotProductLoadable() &&
                  productContinueIgnoresCreativeSaveWhenNewestInScan() &&
                  creativeDurableSaveHonorsValidIdHint() &&
                  creativeDurableSaveCarriesExistingIdentityOnOverwrite() &&
                  creativeDurableSaveRejectsNullDocument() &&
                  creativeDurableSaveRejectsInvalidDocumentId() &&
                  creativeDurableSaveRejectsInvalidAttemptToken() &&
                  creativeLoadRejectsSessionSaveWithoutCreativeSection() &&
                  deletedCreativeSaveRemainsRecoverableButNotProductLoadable() &&
                  corruptCatalogEntryKeepsUnknownContentKind() &&
                  productSoftDeleteMovesSaveAndRemovesFromScan() &&
                  productSoftDeleteMovesSnapshotSidecarWhenPresent() &&
                  productSoftDeleteRejectsMissingIdBeforeIo() &&
                  productSoftDeleteRejectsInvalidId() &&
                  productSoftDeleteForwardsMissingSource() &&
                  productSoftDeleteRejectsExistingDeletedTarget() &&
                  productRecoverRestoresSoftDeletedSaveAndScans() &&
                  productRecoverMovesSnapshotSidecarWhenPresent() &&
                  productRecoverRejectsMissingIdBeforeIo() &&
                  productRecoverRejectsInvalidId() &&
                  productRecoverForwardsMissingSource() &&
                  productRecoverRejectsExistingActiveTarget() &&
                  productDeletedScanShowsSoftDeletedSaveThenClearsOnRecover() &&
                  productDeletedScanUsesMovedSnapshotSidecar() &&
                  productLoadSaveLoadsCompatibleSession() &&
                  productLoadSaveExposesAuthoredRoomSection() &&
                  productLoadSaveRejectsMissingSessionBeforeIo() &&
                  productLoadSaveRejectsMissingPathBeforeIo() &&
                  productLoadSaveMissingFilePreservesSession() &&
                  productLoadSaveRejectsIncompatiblePackage() &&
                  productLoadSaveRejectsIncompatibleScenario() &&
                  productLoadSaveRejectsCorruptFile();
  return ok ? 0 : 1;
}
