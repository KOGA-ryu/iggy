#include "app/iggy3d/ProductWorldCreation.hpp"

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

iggy3d::ProductWorldCreationInput inputFor(
    iggy3d::WorldSetupCreateRequest setupRequest,
    iggy3d::ProductWorldTemplate worldTemplate,
    std::string_view worldId = "world_0001",
    std::string_view requestedAtUtc = "2026-06-23T12:00:00Z") {
  return iggy3d::makeProductWorldCreationInput(
      setupRequest,
      worldTemplate,
      std::filesystem::path("/tmp/iggy3d_product_world_creation_tests"),
      std::string(requestedAtUtc),
      std::string(worldId));
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
         expect(result.initialSavePlan.saveType == "manual",
                "initial save type") &&
         expect(!result.initialSavePlan.userTitlePresent,
                "user title absent") &&
         expect(result.initialSavePlan.autoTitle == "New World - Beginning",
                "auto title") &&
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

}  // namespace

int main() {
  const bool ok = validDefaultRequestPreparesWorldCreation() &&
                  devOverrideTemplateFactsArePreservedWithoutIo() &&
                  notRequestedIsRejected() && invalidWorldIdsAreRejected() &&
                  missingTemplateAndTimestampFieldsReject() &&
                  pathSafeWorldIdsAreAllowed();
  return ok ? 0 : 1;
}
