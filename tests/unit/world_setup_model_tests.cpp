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
         expect(iggy3d::worldSetupFieldName(iggy3d::WorldSetupField::AsciiRoom) ==
                    "ascii_room",
                "ascii room field") &&
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
         expect(!draft.asciiRoomEnabled, "default ascii room disabled") &&
         expect(draft.asciiRoomText.empty(), "default ascii room text empty") &&
         expect(draft.asciiRoomId == "world_setup_room",
                "default ascii room id") &&
         expect(draft.asciiRoomSourceName ==
                    "world_setup_ascii_room.iggyroom.txt",
                "default ascii room source name") &&
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

bool validationRejectsInvalidAsciiRoomDraft() {
  iggy3d::WorldSetupDraft missingText = iggy3d::makeDefaultWorldSetupDraft();
  missingText.asciiRoomEnabled = true;
  missingText.asciiRoomText = "   ";
  const iggy3d::WorldSetupValidation textValidation =
      iggy3d::validateWorldSetupDraft(missingText);

  iggy3d::WorldSetupDraft missingId = iggy3d::makeDefaultWorldSetupDraft();
  missingId.asciiRoomEnabled = true;
  missingId.asciiRoomText = "###\n#P#\n###\n";
  missingId.asciiRoomId = " ";
  const iggy3d::WorldSetupValidation idValidation =
      iggy3d::validateWorldSetupDraft(missingId);

  iggy3d::WorldSetupDraft missingSource =
      iggy3d::makeDefaultWorldSetupDraft();
  missingSource.asciiRoomEnabled = true;
  missingSource.asciiRoomText = "###\n#P#\n###\n";
  missingSource.asciiRoomSourceName = "\t";
  const iggy3d::WorldSetupValidation sourceValidation =
      iggy3d::validateWorldSetupDraft(missingSource);

  return expect(!textValidation.valid, "empty ascii text invalid") &&
         expect(textValidation.reasonCode == "invalid_ascii_room_text",
                "empty ascii text reason") &&
         expect(textValidation.field == iggy3d::WorldSetupField::AsciiRoom,
                "empty ascii text field") &&
         expect(!idValidation.valid, "empty ascii id invalid") &&
         expect(idValidation.reasonCode == "invalid_ascii_room_id",
                "empty ascii id reason") &&
         expect(idValidation.field == iggy3d::WorldSetupField::AsciiRoom,
                "empty ascii id field") &&
         expect(!sourceValidation.valid, "empty ascii source invalid") &&
         expect(sourceValidation.reasonCode ==
                    "invalid_ascii_room_source_name",
                "empty ascii source reason") &&
         expect(sourceValidation.field == iggy3d::WorldSetupField::AsciiRoom,
                "empty ascii source field");
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
  draft.asciiRoomEnabled = true;
  draft.asciiRoomText = "###\n#P#\n###\n";
  draft.asciiRoomId = "  my_world_room  ";
  draft.asciiRoomSourceName = "  worlds/my_world.iggyroom.txt  ";
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
                "create scenario") &&
         expect(route.createRequest.asciiRoomRequested,
                "create ascii room requested") &&
         expect(route.createRequest.asciiRoomText == "###\n#P#\n###\n",
                "create ascii room text") &&
         expect(route.createRequest.asciiRoomId == "my_world_room",
                "create ascii room id") &&
         expect(route.createRequest.asciiRoomSourceName ==
                    "worlds/my_world.iggyroom.txt",
                "create ascii room source name");
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
                  validationRejectsInvalidAsciiRoomDraft() &&
                  backRouteDiscardsDraftOnly() && validCreateReturnsRequest() &&
                  invalidCreateAndUnsupportedActionAreRejected();
  return ok ? 0 : 1;
}
