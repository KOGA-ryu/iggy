#include "app/iggy3d/SaveBridge.hpp"
#include "content/PackageLoader.hpp"
#include "runtime/save/SaveCodec.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

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
  request.saveType = "manual";
  request.autoTitle = "New World - Beginning";
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
  return authoredRoom;
}

bool productDurableSaveWritesFinalAndScans() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001"));
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
         expect(written.saveType == "manual", "product save type proof") &&
         expect(written.autoTitle == "New World - Beginning",
                "product title proof") &&
         expect(std::filesystem::exists(written.paths.finalPath),
                "product final exists") &&
         expect(!std::filesystem::exists(written.paths.tempPath),
                "product temp consumed") &&
         expect(scanned.status == "save_bridge_ready", "scan status") &&
         expect(scanned.saveRoot == root, "scan root") &&
         expect(scanned.slots.slots.size() == 1U, "scan one slot") &&
         expect(scanned.slots.compatibleCount == 1U, "scan compatible") &&
         expect(scanned.slots.slots.front().id == "save_001", "scan id") &&
         expect(scanned.slots.slots.front().enabled, "scan enabled") &&
         expect(scanned.slots.slots.front().savedStateHashHex ==
                    written.record.savedStateHashHex,
                "scan hash hex");
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
         expect(scanned.slots.slots.size() == 1U, "product authored scan slot") &&
         expect(scanned.slots.slots.front().authoredFloorCount == 1U,
                "product authored scan floor count");
}

}  // namespace

int main() {
  const bool ok = productDurableSaveWritesFinalAndScans() &&
                  productDurableSaveHonorsValidIdHint() &&
                  productDurableSaveRejectsMissingState() &&
                  productDurableSaveRejectsInvalidAttemptToken() &&
                  productDurableSavePersistsAuthoredRoom();
  return ok ? 0 : 1;
}
