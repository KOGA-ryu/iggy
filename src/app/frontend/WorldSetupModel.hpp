#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

enum class WorldSetupField : std::uint8_t {
  None,
  WorldName,
  Seed,
  Difficulty,
  StartingScenario,
  AsciiRoom,
  Create,
  Back,
};

enum class WorldSetupDifficulty : std::uint8_t {
  Standard,
};

enum class WorldSetupScenario : std::uint8_t {
  TrainingGround,
};

struct WorldSetupDraft {
  std::string worldName = "New World";
  std::string seedText = "new_world_seed";
  std::uint64_t resolvedSeed = 0;
  bool seedGenerated = true;
  WorldSetupDifficulty difficulty = WorldSetupDifficulty::Standard;
  WorldSetupScenario startingScenario = WorldSetupScenario::TrainingGround;
  bool asciiRoomEnabled = false;
  std::string asciiRoomText;
  std::string asciiRoomId = "world_setup_room";
  std::string asciiRoomSourceName = "world_setup_ascii_room.iggyroom.txt";
  WorldSetupField selectedField = WorldSetupField::WorldName;
};

struct WorldSetupValidation {
  bool valid = false;
  std::string_view reasonCode = "world_setup_unvalidated";
  WorldSetupField field = WorldSetupField::None;
};

struct WorldSetupCreateRequest {
  bool requested = false;
  std::string worldName;
  std::string seedText;
  std::uint64_t resolvedSeed = 0;
  WorldSetupDifficulty difficulty = WorldSetupDifficulty::Standard;
  WorldSetupScenario startingScenario = WorldSetupScenario::TrainingGround;
  bool asciiRoomRequested = false;
  std::string asciiRoomText;
  std::string asciiRoomId;
  std::string asciiRoomSourceName;
};

struct WorldSetupRouteResult {
  bool accepted = false;
  FrontendAction selectedAction = FrontendAction::None;
  std::string_view status = "not_world_setup_action";
  std::string_view reasonCode = "not_world_setup_action";
  bool draftDiscarded = false;
  bool createRequested = false;
  WorldSetupCreateRequest createRequest;
};

std::string_view worldSetupFieldName(WorldSetupField field);
std::string_view worldSetupDifficultyName(WorldSetupDifficulty difficulty);
std::string_view worldSetupScenarioName(WorldSetupScenario scenario);

std::uint64_t resolveWorldSetupSeed(std::string_view seedText);
WorldSetupDraft makeDefaultWorldSetupDraft(
    std::string_view generatedSeedText = "new_world_seed");
WorldSetupValidation validateWorldSetupDraft(const WorldSetupDraft& draft);
WorldSetupRouteResult routeWorldSetupAction(const WorldSetupDraft& draft,
                                            FrontendAction action);

}  // namespace iggy3d
