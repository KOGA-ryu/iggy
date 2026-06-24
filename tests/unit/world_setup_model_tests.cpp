#include "app/frontend/WorldSetupModel.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool defaultDraftAndNamesAreStable() {
  const iggy3d::WorldSetupDraft draft =
      iggy3d::makeDefaultWorldSetupDraft("fixed_seed");
  return expect(iggy3d::worldSetupFieldName(iggy3d::WorldSetupField::WorldName) ==
                    "world_name",
                "world name field") &&
         expect(iggy3d::worldSetupFieldName(iggy3d::WorldSetupField::Seed) ==
                    "seed",
                "seed field") &&
         expect(iggy3d::worldSetupFieldName(iggy3d::WorldSetupField::Create) ==
                    "create",
                "create field") &&
         expect(iggy3d::worldSetupDifficultyName(
                    iggy3d::WorldSetupDifficulty::Standard) == "standard",
                "standard difficulty") &&
         expect(iggy3d::worldSetupScenarioName(
                    iggy3d::WorldSetupScenario::TrainingGround) ==
                    "training_ground",
                "training ground scenario") &&
         expect(draft.worldName == "New World", "default world name") &&
         expect(draft.seedText == "fixed_seed", "default seed text") &&
         expect(draft.resolvedSeed == iggy3d::resolveWorldSetupSeed("fixed_seed"),
                "default resolved seed") &&
         expect(draft.seedGenerated, "default seed generated") &&
         expect(draft.difficulty == iggy3d::WorldSetupDifficulty::Standard,
                "default difficulty") &&
         expect(draft.startingScenario ==
                    iggy3d::WorldSetupScenario::TrainingGround,
                "default scenario") &&
         expect(draft.selectedField == iggy3d::WorldSetupField::WorldName,
                "default selected field");
}

bool validationAcceptsDefaultDraft() {
  const iggy3d::WorldSetupDraft draft = iggy3d::makeDefaultWorldSetupDraft();
  const iggy3d::WorldSetupValidation validation =
      iggy3d::validateWorldSetupDraft(draft);
  return expect(validation.valid, "default valid") &&
         expect(validation.reasonCode == "ok", "default reason") &&
         expect(validation.field == iggy3d::WorldSetupField::None,
                "default invalid field none");
}

bool validationRejectsInvalidWorldNames() {
  iggy3d::WorldSetupDraft empty = iggy3d::makeDefaultWorldSetupDraft();
  empty.worldName = "   \t ";
  const iggy3d::WorldSetupValidation emptyValidation =
      iggy3d::validateWorldSetupDraft(empty);

  iggy3d::WorldSetupDraft longName = iggy3d::makeDefaultWorldSetupDraft();
  longName.worldName = std::string(65U, 'A');
  const iggy3d::WorldSetupValidation longValidation =
      iggy3d::validateWorldSetupDraft(longName);

  return expect(!emptyValidation.valid, "empty name invalid") &&
         expect(emptyValidation.reasonCode == "invalid_world_name",
                "empty name reason") &&
         expect(emptyValidation.field == iggy3d::WorldSetupField::WorldName,
                "empty name field") &&
         expect(!longValidation.valid, "long name invalid") &&
         expect(longValidation.reasonCode == "invalid_world_name",
                "long name reason") &&
         expect(longValidation.field == iggy3d::WorldSetupField::WorldName,
                "long name field");
}

bool validationRejectsInvalidSeedAndUnsupportedEnums() {
  iggy3d::WorldSetupDraft emptySeed = iggy3d::makeDefaultWorldSetupDraft();
  emptySeed.seedText = " ";
  const iggy3d::WorldSetupValidation seedValidation =
      iggy3d::validateWorldSetupDraft(emptySeed);

  iggy3d::WorldSetupDraft unsupportedDifficulty =
      iggy3d::makeDefaultWorldSetupDraft();
  unsupportedDifficulty.difficulty =
      static_cast<iggy3d::WorldSetupDifficulty>(255U);
  const iggy3d::WorldSetupValidation difficultyValidation =
      iggy3d::validateWorldSetupDraft(unsupportedDifficulty);

  iggy3d::WorldSetupDraft unsupportedScenario =
      iggy3d::makeDefaultWorldSetupDraft();
  unsupportedScenario.startingScenario =
      static_cast<iggy3d::WorldSetupScenario>(255U);
  const iggy3d::WorldSetupValidation scenarioValidation =
      iggy3d::validateWorldSetupDraft(unsupportedScenario);

  return expect(!seedValidation.valid, "empty seed invalid") &&
         expect(seedValidation.reasonCode == "invalid_seed",
                "empty seed reason") &&
         expect(seedValidation.field == iggy3d::WorldSetupField::Seed,
                "empty seed field") &&
         expect(!difficultyValidation.valid, "difficulty invalid") &&
         expect(difficultyValidation.reasonCode == "unsupported_difficulty",
                "difficulty reason") &&
         expect(difficultyValidation.field == iggy3d::WorldSetupField::Difficulty,
                "difficulty field") &&
         expect(!scenarioValidation.valid, "scenario invalid") &&
         expect(scenarioValidation.reasonCode == "unsupported_scenario",
                "scenario reason") &&
         expect(scenarioValidation.field ==
                    iggy3d::WorldSetupField::StartingScenario,
                "scenario field");
}

bool backRouteDiscardsDraftOnly() {
  const iggy3d::WorldSetupDraft draft = iggy3d::makeDefaultWorldSetupDraft();
  const iggy3d::WorldSetupRouteResult route =
      iggy3d::routeWorldSetupAction(draft, iggy3d::FrontendAction::Back);
  return expect(route.accepted, "back accepted") &&
         expect(route.selectedAction == iggy3d::FrontendAction::Back,
                "back selected") &&
         expect(route.status == "world_setup_back", "back status") &&
         expect(route.reasonCode == "world_setup_back", "back reason") &&
         expect(route.draftDiscarded, "back discards") &&
         expect(!route.createRequested, "back no create") &&
         expect(!route.createRequest.requested, "back no request");
}

bool validCreateReturnsRequest() {
  iggy3d::WorldSetupDraft draft = iggy3d::makeDefaultWorldSetupDraft("seed_42");
  draft.worldName = "  My World  ";
  const iggy3d::WorldSetupRouteResult route =
      iggy3d::routeWorldSetupAction(draft,
                                    iggy3d::FrontendAction::CreateAndEnter);
  return expect(route.accepted, "create accepted") &&
         expect(route.selectedAction == iggy3d::FrontendAction::CreateAndEnter,
                "create selected") &&
         expect(route.status == "world_setup_create_requested",
                "create status") &&
         expect(route.reasonCode == "world_setup_create_requested",
                "create reason") &&
         expect(!route.draftDiscarded, "create preserves draft") &&
         expect(route.createRequested, "create requested") &&
         expect(route.createRequest.requested, "create request requested") &&
         expect(route.createRequest.worldName == "My World",
                "create trims world name") &&
         expect(route.createRequest.seedText == "seed_42", "create seed text") &&
         expect(route.createRequest.resolvedSeed ==
                    iggy3d::resolveWorldSetupSeed("seed_42"),
                "create resolved seed") &&
         expect(route.createRequest.difficulty ==
                    iggy3d::WorldSetupDifficulty::Standard,
                "create difficulty") &&
         expect(route.createRequest.startingScenario ==
                    iggy3d::WorldSetupScenario::TrainingGround,
                "create scenario");
}

bool invalidCreateAndUnsupportedActionAreRejected() {
  iggy3d::WorldSetupDraft draft = iggy3d::makeDefaultWorldSetupDraft();
  draft.worldName = "";
  const iggy3d::WorldSetupRouteResult invalid =
      iggy3d::routeWorldSetupAction(draft,
                                    iggy3d::FrontendAction::CreateAndEnter);
  const iggy3d::WorldSetupRouteResult unsupported =
      iggy3d::routeWorldSetupAction(draft, iggy3d::FrontendAction::Apply);

  return expect(!invalid.accepted, "invalid create rejected") &&
         expect(invalid.selectedAction == iggy3d::FrontendAction::CreateAndEnter,
                "invalid create selected") &&
         expect(invalid.status == "world_setup_invalid",
                "invalid create status") &&
         expect(invalid.reasonCode == "invalid_world_name",
                "invalid create reason") &&
         expect(!invalid.createRequested, "invalid create no request") &&
         expect(!unsupported.accepted, "unsupported rejected") &&
         expect(unsupported.selectedAction == iggy3d::FrontendAction::Apply,
                "unsupported selected") &&
         expect(unsupported.status == "not_world_setup_action",
                "unsupported status") &&
         expect(unsupported.reasonCode == "not_world_setup_action",
                "unsupported reason");
}

}  // namespace

int main() {
  const bool ok = defaultDraftAndNamesAreStable() &&
                  validationAcceptsDefaultDraft() &&
                  validationRejectsInvalidWorldNames() &&
                  validationRejectsInvalidSeedAndUnsupportedEnums() &&
                  backRouteDiscardsDraftOnly() && validCreateReturnsRequest() &&
                  invalidCreateAndUnsupportedActionAreRejected();
  return ok ? 0 : 1;
}
