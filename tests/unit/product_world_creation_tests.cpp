#include "app/iggy3d/world/ProductWorldCreation.hpp"
#include "content/PackageLoader.hpp"
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

iggy3d::WorldSetupCreateRequest defaultSetupRequest() {
  auto draft = iggy3d::makeDefaultWorldSetupDraft("seed_42");
  draft.worldName = "  New World  ";
  return iggy3d::routeWorldSetupAction(
             draft,
             iggy3d::FrontendAction::CreateAndEnter)
      .createRequest;
}

iggy3d::WorldSetupCreateRequest asciiSetupRequest() {
  auto draft = iggy3d::makeDefaultWorldSetupDraft("seed_ascii");
  draft.worldName = "  ASCII Chapter  ";
  draft.asciiRoomEnabled = true;
  draft.asciiRoomText =
      "#####\n"
      "#P$E#\n"
      "#####\n";
  draft.asciiRoomId = "ascii_chapter_room";
  draft.asciiRoomSourceName = "worlds/ascii_chapter.iggyroom.txt";
  return iggy3d::routeWorldSetupAction(
             draft,
             iggy3d::FrontendAction::CreateAndEnter)
      .createRequest;
}

iggy3d::ProductWorldCreationInput inputFor(
    iggy3d::WorldSetupCreateRequest setupRequest,
    iggy3d::ProductWorldTemplate worldTemplate,
    std::string_view worldId = "world_0001",
    std::string_view requestedAtUtc = "2026-06-23T12:00:00Z",
    std::filesystem::path saveRoot =
        std::filesystem::path("/tmp/iggy3d_product_world_creation_tests")) {
  return iggy3d::makeProductWorldCreationInput(
      setupRequest,
      worldTemplate,
      std::move(saveRoot),
      std::string(requestedAtUtc),
      std::string(worldId));
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_product_world_creation_tests";
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

iggy3d::ProductWorldTemplate movementPlaygroundTemplate() {
  iggy3d::ProductWorldTemplate worldTemplate =
      iggy3d::defaultProductWorldTemplate();
  worldTemplate.packageId = "iggy3d.movement_playground";
  worldTemplate.scenarioId = "movement_playground.runtime_loop";
  worldTemplate.displayName = "Movement Playground";
  worldTemplate.source = "unit_fixture_override";
  return worldTemplate;
}

bool validDefaultRequestPreparesWorldCreation() {
  const iggy3d::WorldSetupCreateRequest setup = defaultSetupRequest();
  const iggy3d::ProductWorldTemplate worldTemplate =
      iggy3d::defaultProductWorldTemplate();
  const iggy3d::ProductWorldCreationResult result =
      iggy3d::prepareProductWorldCreation(
          inputFor(setup, worldTemplate, "world_0001"));

  return expect(result.accepted, "default accepted") &&
         expect(result.status == "world_creation_request_ready",
                "default status") &&
         expect(result.reasonCode == "world_creation_request_ready",
                "default reason") &&
         expect(result.request.requested, "request requested") &&
         expect(result.request.worldId == "world_0001", "world id") &&
         expect(result.request.worldName == "New World", "world name") &&
         expect(result.request.seedText == "seed_42", "seed text") &&
         expect(result.request.resolvedSeed ==
                    iggy3d::resolveWorldSetupSeed("seed_42"),
                "resolved seed") &&
         expect(result.request.difficulty ==
                    iggy3d::WorldSetupDifficulty::Standard,
                "difficulty") &&
         expect(result.request.startingScenario ==
                    iggy3d::WorldSetupScenario::TrainingGround,
                "scenario") &&
         expect(result.request.packageId == "iggy3d.default_world",
                "package id") &&
         expect(result.request.scenarioId == "default", "scenario id") &&
         expect(result.request.templateSource == "builtin_default",
                "template source") &&
         expect(result.request.saveRoot ==
                    std::filesystem::path(
                        "/tmp/iggy3d_product_world_creation_tests"),
                "save root") &&
         expect(result.request.requestedAtUtc == "2026-06-23T12:00:00Z",
                "timestamp") &&
         expect(result.initialSavePlan.requested, "initial save requested") &&
         expect(result.initialSavePlan.saveType == "initial",
                "initial save type") &&
         expect(!result.initialSavePlan.userTitlePresent,
                "user title absent") &&
         expect(result.initialSavePlan.worldTitle == "New World",
                "initial save world title") &&
         expect(result.initialSavePlan.createdAtUtc == "2026-06-23T12:00:00Z",
                "initial save created utc") &&
         expect(result.initialSavePlan.savedAtUtc == "2026-06-23T12:00:00Z",
                "initial save saved utc") &&
         expect(result.initialSavePlan.worldId == "world_0001",
                "initial save world id") &&
         expect(!result.initialSavePlan.written, "initial save not written") &&
         expect(result.initialSavePlan.saveId.empty(), "save id empty") &&
         expect(!result.sessionCreated, "session not created") &&
         expect(!result.initialSaveWritten, "initial save not written flag") &&
         expect(result.routeAfterCreate == "world_setup", "route after create");
}

bool devOverrideTemplateFactsArePreservedWithoutIo() {
  const iggy3d::ProductWorldTemplate worldTemplate =
      iggy3d::devOverrideProductWorldTemplate(
          "fixtures/demos/movement_playground/package.iggy3d.toml",
          "movement_playground");
  const iggy3d::ProductWorldCreationResult result =
      iggy3d::prepareProductWorldCreation(
          inputFor(defaultSetupRequest(), worldTemplate, "world_alpha_01"));

  return expect(result.accepted, "dev accepted") &&
         expect(result.request.worldId == "world_alpha_01", "dev world id") &&
         expect(result.request.packageId ==
                    "fixtures/demos/movement_playground/package.iggy3d.toml",
                "dev package") &&
         expect(result.request.scenarioId == "movement_playground",
                "dev scenario") &&
         expect(result.request.templateSource == "dev_package_override",
                "dev source") &&
         expect(result.request.templateDisplayName == "Dev Override World",
                "dev display");
}

bool asciiRoomSelectionFactsArePreservedWithoutIo() {
  const iggy3d::ProductWorldCreationResult result =
      iggy3d::prepareProductWorldCreation(inputFor(
          asciiSetupRequest(), iggy3d::defaultProductWorldTemplate(),
          "world_ascii_01"));

  return expect(result.accepted, "ascii accepted") &&
         expect(result.request.worldName == "ASCII Chapter",
                "ascii world title") &&
         expect(result.request.asciiRoomRequested,
                "ascii room requested") &&
         expect(result.request.asciiRoomId == "ascii_chapter_room",
                "ascii room id") &&
         expect(result.request.asciiRoomSourceName ==
                    "worlds/ascii_chapter.iggyroom.txt",
                "ascii room source") &&
         expect(result.initialSavePlan.worldTitle == "ASCII Chapter",
                "ascii initial save title");
}

bool notRequestedIsRejected() {
  iggy3d::WorldSetupCreateRequest setup;
  setup.requested = false;
  const iggy3d::ProductWorldCreationResult result =
      iggy3d::prepareProductWorldCreation(
          inputFor(setup, iggy3d::defaultProductWorldTemplate()));
  return expect(!result.accepted, "not requested rejected") &&
         expect(result.status == "world_creation_not_requested",
                "not requested status") &&
         expect(result.reasonCode == "world_creation_not_requested",
                "not requested reason") &&
         expect(!result.request.requested, "not requested no request") &&
         expect(!result.initialSavePlan.requested,
                "not requested no initial save");
}

bool invalidWorldIdsAreRejected() {
  const iggy3d::WorldSetupCreateRequest setup = defaultSetupRequest();
  const iggy3d::ProductWorldTemplate worldTemplate =
      iggy3d::defaultProductWorldTemplate();

  const auto empty = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, ""));
  const auto whitespace = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "   "));
  const auto slash = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "world/0001"));
  const auto backslash = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "world\\0001"));
  const auto traversal = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "world..0001"));

  return expect(!empty.accepted, "empty id rejected") &&
         expect(empty.status == "world_creation_missing_world_id",
                "empty id status") &&
         expect(!whitespace.accepted, "whitespace id rejected") &&
         expect(whitespace.reasonCode == "world_creation_missing_world_id",
                "whitespace id reason") &&
         expect(!slash.accepted, "slash id rejected") &&
         expect(slash.status == "world_creation_invalid_world_id",
                "slash id status") &&
         expect(!backslash.accepted, "backslash id rejected") &&
         expect(backslash.reasonCode == "world_creation_invalid_world_id",
                "backslash id reason") &&
         expect(!traversal.accepted, "traversal id rejected") &&
         expect(traversal.status == "world_creation_invalid_world_id",
                "traversal id status");
}

bool missingTemplateAndTimestampFieldsReject() {
  const iggy3d::WorldSetupCreateRequest setup = defaultSetupRequest();

  iggy3d::ProductWorldTemplate missingPackage =
      iggy3d::defaultProductWorldTemplate();
  missingPackage.packageId = " ";
  const auto packageResult = iggy3d::prepareProductWorldCreation(
      inputFor(setup, missingPackage));

  iggy3d::ProductWorldTemplate missingScenario =
      iggy3d::defaultProductWorldTemplate();
  missingScenario.scenarioId.clear();
  const auto scenarioResult = iggy3d::prepareProductWorldCreation(
      inputFor(setup, missingScenario));

  const auto timestampResult = iggy3d::prepareProductWorldCreation(
      inputFor(setup, iggy3d::defaultProductWorldTemplate(), "world_0001", " "));

  return expect(!packageResult.accepted, "missing package rejected") &&
         expect(packageResult.status == "world_creation_missing_package_id",
                "missing package status") &&
         expect(!scenarioResult.accepted, "missing scenario rejected") &&
         expect(scenarioResult.status == "world_creation_missing_scenario_id",
                "missing scenario status") &&
         expect(!timestampResult.accepted, "missing timestamp rejected") &&
         expect(timestampResult.status == "world_creation_missing_timestamp",
                "missing timestamp status");
}

bool pathSafeWorldIdsAreAllowed() {
  const iggy3d::WorldSetupCreateRequest setup = defaultSetupRequest();
  const iggy3d::ProductWorldTemplate worldTemplate =
      iggy3d::defaultProductWorldTemplate();
  const auto underscore = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "world_0001"));
  const auto hyphen = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "world-0001"));
  const auto alpha = iggy3d::prepareProductWorldCreation(
      inputFor(setup, worldTemplate, "world_alpha_01"));

  return expect(underscore.accepted, "underscore id accepted") &&
         expect(hyphen.accepted, "hyphen id accepted") &&
         expect(alpha.accepted, "alpha id accepted");
}

bool validCreationWritesInitialSaveDurably() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductWorldCreationResult creation =
      iggy3d::prepareProductWorldCreation(inputFor(
          defaultSetupRequest(), movementPlaygroundTemplate(), "world_0001",
          "2026-06-23T12:00:00Z", root));

  iggy3d::ProductWorldInitialSaveRequest request;
  request.creation = creation;
  request.state = &session.state();
  request.attemptToken = "attempt_001";
  const iggy3d::ProductWorldInitialSaveResult result =
      iggy3d::writeProductWorldInitialSaveDurably(request);
  const iggy3d::SaveFileReadResult read =
      result.saveWrite.ok ? iggy3d::readSaveFile(result.saveWrite.record.path)
                          : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, creation.request.packageId, creation.request.scenarioId);

  return expect(result.ok, "initial save ok") &&
         expect(result.status == "world_creation_initial_save_written",
                "initial save status") &&
         expect(result.reasonCode == "world_creation_initial_save_written",
                "initial save reason") &&
         expect(result.creation.accepted, "initial save accepted") &&
         expect(result.creation.sessionCreated, "initial save session supplied") &&
         expect(result.creation.initialSaveWritten,
                "initial save written flag") &&
         expect(result.creation.initialSavePlan.written,
                "initial save plan written") &&
         expect(result.creation.initialSavePlan.saveId == "save_001",
                "initial save id") &&
         expect(result.creation.routeAfterCreate == "gameplay",
                "initial save route gameplay") &&
         expect(result.saveWrite.ok, "initial save bridge ok") &&
         expect(result.saveWrite.status == "product_save_written",
                "initial save bridge status") &&
         expect(result.saveWrite.durableReason == "durable_save_file_written",
                "initial save durable reason") &&
         expect(result.saveWrite.worldId == "world_0001",
                "initial save world proof") &&
         expect(result.saveWrite.worldTitle == "New World",
                "initial save world title proof") &&
         expect(result.saveWrite.saveTitle == "New World",
                "initial save save title proof") &&
         expect(result.saveWrite.saveType == "initial",
                "initial save type proof") &&
         expect(result.saveWrite.createdAtUtc == "2026-06-23T12:00:00Z",
                "initial save created utc proof") &&
         expect(result.saveWrite.savedAtUtc == "2026-06-23T12:00:00Z",
                "initial save saved utc proof") &&
         expect(read.ok, "initial save final read") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "initial save final decoded") &&
         expect(decoded.envelope.metadata.saveId == "save_001",
                "initial save metadata save id") &&
         expect(decoded.envelope.metadata.worldId == "world_0001",
                "initial save metadata world id") &&
         expect(decoded.envelope.metadata.worldTitle == "New World",
                "initial save metadata world title") &&
         expect(decoded.envelope.metadata.saveTitle == "New World",
                "initial save metadata save title") &&
         expect(decoded.envelope.metadata.saveType == "initial",
                "initial save metadata save type") &&
         expect(decoded.envelope.metadata.createdAtUtc == "2026-06-23T12:00:00Z",
                "initial save metadata created utc") &&
         expect(decoded.envelope.metadata.savedAtUtc == "2026-06-23T12:00:00Z",
                "initial save metadata saved utc") &&
         expect(result.saveWrite.finalValidated,
                "initial save final validated") &&
         expect(std::filesystem::exists(result.saveWrite.paths.finalPath),
                "initial save final exists") &&
         expect(!std::filesystem::exists(result.saveWrite.paths.tempPath),
                "initial save temp consumed") &&
         expect(scanned.slots.slots.size() == 1U, "initial save scan slot") &&
         expect(scanned.slots.compatibleCount == 1U,
                "initial save scan compatible") &&
         expect(scanned.slots.slots.front().id == "save_001",
                "initial save scan id");
}

bool unpreparedCreationDoesNotWriteInitialSave() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::ProductWorldInitialSaveRequest request;
  request.creation = iggy3d::ProductWorldCreationResult{};
  request.state = &session.state();
  request.attemptToken = "attempt_001";
  const iggy3d::ProductWorldInitialSaveResult result =
      iggy3d::writeProductWorldInitialSaveDurably(request);
  return expect(!result.ok, "unprepared initial save rejected") &&
         expect(result.status == "world_creation_not_ready",
                "unprepared status") &&
         expect(result.reasonCode == "world_creation_not_ready",
                "unprepared reason") &&
         expect(!result.saveWrite.durableWriteRequested,
                "unprepared no bridge write") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "unprepared no saves");
}

bool missingSessionDoesNotWriteInitialSave() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductWorldCreationResult creation =
      iggy3d::prepareProductWorldCreation(inputFor(
          defaultSetupRequest(), movementPlaygroundTemplate(), "world_0001",
          "2026-06-23T12:00:00Z", root));
  iggy3d::ProductWorldInitialSaveRequest request;
  request.creation = creation;
  request.attemptToken = "attempt_001";
  const iggy3d::ProductWorldInitialSaveResult result =
      iggy3d::writeProductWorldInitialSaveDurably(request);
  return expect(!result.ok, "missing session rejected") &&
         expect(result.status == "world_creation_session_missing",
                "missing session status") &&
         expect(result.reasonCode == "world_creation_session_missing",
                "missing session reason") &&
         expect(!result.saveWrite.durableWriteRequested,
                "missing session no bridge write") &&
         expect(result.creation.routeAfterCreate == "world_setup",
                "missing session stays setup") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "missing session no saves");
}

bool invalidAttemptTokenPreservesDurableReason() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductWorldCreationResult creation =
      iggy3d::prepareProductWorldCreation(inputFor(
          defaultSetupRequest(), movementPlaygroundTemplate(), "world_0001",
          "2026-06-23T12:00:00Z", root));
  iggy3d::ProductWorldInitialSaveRequest request;
  request.creation = creation;
  request.state = &session.state();
  request.attemptToken = "attempt 001";
  const iggy3d::ProductWorldInitialSaveResult result =
      iggy3d::writeProductWorldInitialSaveDurably(request);
  return expect(!result.ok, "invalid attempt rejected") &&
         expect(result.status == "durable_save_invalid_attempt_token",
                "invalid attempt status") &&
         expect(result.reasonCode == "durable_save_invalid_attempt_token",
                "invalid attempt reason") &&
         expect(result.saveWrite.durableWriteRequested,
                "invalid attempt bridge requested") &&
         expect(result.saveWrite.durableReason ==
                    "durable_save_invalid_attempt_token",
                "invalid attempt durable reason") &&
         expect(!result.creation.initialSaveWritten,
                "invalid attempt not written") &&
         expect(!result.creation.initialSavePlan.written,
                "invalid attempt plan not written") &&
         expect(result.creation.routeAfterCreate == "world_setup",
                "invalid attempt stays setup") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "invalid attempt no saves");
}

}  // namespace

int main() {
  const bool ok = validDefaultRequestPreparesWorldCreation() &&
                  devOverrideTemplateFactsArePreservedWithoutIo() &&
                  asciiRoomSelectionFactsArePreservedWithoutIo() &&
                  notRequestedIsRejected() && invalidWorldIdsAreRejected() &&
                  missingTemplateAndTimestampFieldsReject() &&
                  pathSafeWorldIdsAreAllowed() &&
                  validCreationWritesInitialSaveDurably() &&
                  unpreparedCreationDoesNotWriteInitialSave() &&
                  missingSessionDoesNotWriteInitialSave() &&
                  invalidAttemptTokenPreservesDurableReason();
  return ok ? 0 : 1;
}
