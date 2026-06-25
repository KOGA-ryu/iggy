#include "app/frontend/WorldSetupModel.hpp"

#include <cctype>
#include <cstddef>

namespace iggy3d {
namespace {

constexpr std::size_t kMaxWorldNameLength = 64U;
constexpr std::string_view kDefaultSeedText = "new_world_seed";

std::string_view trimAsciiWhitespace(std::string_view value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front())) != 0) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.remove_suffix(1);
  }
  return value;
}

bool supportedDifficulty(WorldSetupDifficulty difficulty) {
  switch (difficulty) {
    case WorldSetupDifficulty::Standard:
      return true;
  }
  return false;
}

bool supportedScenario(WorldSetupScenario scenario) {
  switch (scenario) {
    case WorldSetupScenario::TrainingGround:
      return true;
  }
  return false;
}

WorldSetupRouteResult ignoredRoute(FrontendAction action,
                                   std::string_view status,
                                   std::string_view reason) {
  WorldSetupRouteResult result;
  result.selectedAction = action;
  result.status = status;
  result.reasonCode = reason;
  return result;
}

WorldSetupCreateRequest createRequestFromDraft(const WorldSetupDraft& draft) {
  WorldSetupCreateRequest request;
  request.requested = true;
  request.worldName = std::string(trimAsciiWhitespace(draft.worldName));
  request.seedText = std::string(trimAsciiWhitespace(draft.seedText));
  request.resolvedSeed = resolveWorldSetupSeed(request.seedText);
  request.difficulty = draft.difficulty;
  request.startingScenario = draft.startingScenario;
  request.asciiRoomRequested = draft.asciiRoomEnabled;
  request.asciiRoomText = draft.asciiRoomText;
  request.asciiRoomId = std::string(trimAsciiWhitespace(draft.asciiRoomId));
  request.asciiRoomSourceName =
      std::string(trimAsciiWhitespace(draft.asciiRoomSourceName));
  return request;
}

}  // namespace

std::string_view worldSetupFieldName(WorldSetupField field) {
  switch (field) {
    case WorldSetupField::None:
      return "none";
    case WorldSetupField::WorldName:
      return "world_name";
    case WorldSetupField::Seed:
      return "seed";
    case WorldSetupField::Difficulty:
      return "difficulty";
    case WorldSetupField::StartingScenario:
      return "starting_scenario";
    case WorldSetupField::AsciiRoom:
      return "ascii_room";
    case WorldSetupField::Create:
      return "create";
    case WorldSetupField::Back:
      return "back";
  }
  return "unknown";
}

std::string_view worldSetupDifficultyName(WorldSetupDifficulty difficulty) {
  switch (difficulty) {
    case WorldSetupDifficulty::Standard:
      return "standard";
  }
  return "unknown";
}

std::string_view worldSetupScenarioName(WorldSetupScenario scenario) {
  switch (scenario) {
    case WorldSetupScenario::TrainingGround:
      return "training_ground";
  }
  return "unknown";
}

std::uint64_t resolveWorldSetupSeed(std::string_view seedText) {
  // FNV-1a gives a small deterministic text-to-u64 resolver without state.
  std::uint64_t hash = 14695981039346656037ULL;
  for (const char ch : seedText) {
    hash ^= static_cast<unsigned char>(ch);
    hash *= 1099511628211ULL;
  }
  return hash;
}

WorldSetupDraft makeDefaultWorldSetupDraft(std::string_view generatedSeedText) {
  WorldSetupDraft draft;
  draft.seedText = generatedSeedText.empty() ? std::string(kDefaultSeedText)
                                             : std::string(generatedSeedText);
  draft.resolvedSeed = resolveWorldSetupSeed(draft.seedText);
  return draft;
}

WorldSetupValidation validateWorldSetupDraft(const WorldSetupDraft& draft) {
  const std::string_view trimmedName = trimAsciiWhitespace(draft.worldName);
  if (trimmedName.empty() || trimmedName.size() > kMaxWorldNameLength) {
    return WorldSetupValidation{false,
                                "invalid_world_name",
                                WorldSetupField::WorldName};
  }

  const std::string_view trimmedSeed = trimAsciiWhitespace(draft.seedText);
  if (trimmedSeed.empty()) {
    return WorldSetupValidation{false, "invalid_seed", WorldSetupField::Seed};
  }

  if (!supportedDifficulty(draft.difficulty)) {
    return WorldSetupValidation{false,
                                "unsupported_difficulty",
                                WorldSetupField::Difficulty};
  }

  if (!supportedScenario(draft.startingScenario)) {
    return WorldSetupValidation{false,
                                "unsupported_scenario",
                                WorldSetupField::StartingScenario};
  }

  if (draft.asciiRoomEnabled) {
    if (trimAsciiWhitespace(draft.asciiRoomText).empty()) {
      return WorldSetupValidation{false,
                                  "invalid_ascii_room_text",
                                  WorldSetupField::AsciiRoom};
    }
    if (trimAsciiWhitespace(draft.asciiRoomId).empty()) {
      return WorldSetupValidation{false,
                                  "invalid_ascii_room_id",
                                  WorldSetupField::AsciiRoom};
    }
    if (trimAsciiWhitespace(draft.asciiRoomSourceName).empty()) {
      return WorldSetupValidation{false,
                                  "invalid_ascii_room_source_name",
                                  WorldSetupField::AsciiRoom};
    }
  }

  return WorldSetupValidation{true, "ok", WorldSetupField::None};
}

WorldSetupRouteResult routeWorldSetupAction(const WorldSetupDraft& draft,
                                            FrontendAction action) {
  switch (action) {
    case FrontendAction::Back: {
      WorldSetupRouteResult result;
      result.accepted = true;
      result.selectedAction = FrontendAction::Back;
      result.status = "world_setup_back";
      result.reasonCode = "world_setup_back";
      result.draftDiscarded = true;
      return result;
    }
    case FrontendAction::CreateAndEnter: {
      const WorldSetupValidation validation = validateWorldSetupDraft(draft);
      if (!validation.valid) {
        return ignoredRoute(FrontendAction::CreateAndEnter,
                            "world_setup_invalid",
                            validation.reasonCode);
      }

      WorldSetupRouteResult result;
      result.accepted = true;
      result.selectedAction = FrontendAction::CreateAndEnter;
      result.status = "world_setup_create_requested";
      result.reasonCode = "world_setup_create_requested";
      result.createRequested = true;
      result.createRequest = createRequestFromDraft(draft);
      return result;
    }
    case FrontendAction::None:
    case FrontendAction::Continue:
    case FrontendAction::NewWorld:
    case FrontendAction::LoadSave:
    case FrontendAction::Settings:
    case FrontendAction::DevTools:
    case FrontendAction::Exit:
    case FrontendAction::Load:
    case FrontendAction::Delete:
    case FrontendAction::Apply:
    case FrontendAction::RestoreDefaults:
    case FrontendAction::Resume:
    case FrontendAction::Save:
    case FrontendAction::SaveAndExit:
    case FrontendAction::ReturnToTitle:
    case FrontendAction::ExitGame:
      return ignoredRoute(action, "not_world_setup_action", "not_world_setup_action");
  }

  return ignoredRoute(action, "not_world_setup_action", "not_world_setup_action");
}

}  // namespace iggy3d
