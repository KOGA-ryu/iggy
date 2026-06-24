#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"

namespace iggy3d {

struct ProductWorldCreationInput {
  WorldSetupCreateRequest setupRequest;
  ProductWorldTemplate worldTemplate;
  std::filesystem::path saveRoot;
  std::string requestedAtUtc;
  std::string generatedWorldId;
};

struct ProductWorldCreationRequest {
  bool requested = false;
  std::string worldId;
  std::string worldName;
  std::string seedText;
  std::uint64_t resolvedSeed = 0;
  WorldSetupDifficulty difficulty = WorldSetupDifficulty::Standard;
  WorldSetupScenario startingScenario = WorldSetupScenario::TrainingGround;
  std::string packageId;
  std::string scenarioId;
  std::string templateDisplayName;
  std::string templateSource;
  std::filesystem::path saveRoot;
  std::string requestedAtUtc;
};

struct ProductInitialSavePlan {
  bool requested = false;
  std::string saveType = "manual";
  bool userTitlePresent = false;
  std::string autoTitle;
  std::string worldId;
  bool written = false;
  std::string saveId;
};

struct ProductWorldCreationResult {
  bool accepted = false;
  std::string_view status = "world_creation_not_requested";
  std::string_view reasonCode = "world_creation_not_requested";
  ProductWorldCreationRequest request;
  ProductInitialSavePlan initialSavePlan;
  bool sessionCreated = false;
  bool initialSaveWritten = false;
  std::string_view routeAfterCreate = "world_setup";
};

ProductWorldCreationInput makeProductWorldCreationInput(
    const WorldSetupCreateRequest& setupRequest,
    const ProductWorldTemplate& worldTemplate,
    std::filesystem::path saveRoot,
    std::string requestedAtUtc,
    std::string generatedWorldId);

ProductWorldCreationResult prepareProductWorldCreation(
    const ProductWorldCreationInput& input);

}  // namespace iggy3d
