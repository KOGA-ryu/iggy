#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/SaveBridge.hpp"

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
  bool asciiRoomRequested = false;
  std::string asciiRoomId;
  std::string asciiRoomSourceName;
  std::filesystem::path saveRoot;
  std::string requestedAtUtc;
};

struct ProductInitialSavePlan {
  bool requested = false;
  std::string saveType = "initial";
  bool userTitlePresent = false;
  std::string worldTitle;
  std::string worldId;
  std::string createdAtUtc;
  std::string savedAtUtc;
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

struct ProductWorldInitialSaveRequest {
  ProductWorldCreationResult creation;
  const SessionState* state = nullptr;
  const SaveAuthoredRoomSection* authoredRoom = nullptr;
  std::string attemptToken;
};

struct ProductWorldInitialSaveResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  ProductWorldCreationResult creation;
  ProductSaveWriteResult saveWrite;
};

ProductWorldCreationInput makeProductWorldCreationInput(
    const WorldSetupCreateRequest& setupRequest,
    const ProductWorldTemplate& worldTemplate,
    std::filesystem::path saveRoot,
    std::string requestedAtUtc,
    std::string generatedWorldId);

ProductWorldCreationResult prepareProductWorldCreation(
    const ProductWorldCreationInput& input);
ProductWorldInitialSaveResult writeProductWorldInitialSaveDurably(
    const ProductWorldInitialSaveRequest& request);

}  // namespace iggy3d
