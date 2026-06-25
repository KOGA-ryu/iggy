#include "app/iggy3d/ProductWorldCreation.hpp"

#include <cctype>
#include <utility>

namespace iggy3d {
namespace {

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

bool containsWhitespaceOnly(std::string_view value) {
  return trimAsciiWhitespace(value).empty();
}

bool containsInvalidWorldIdToken(std::string_view value) {
  return value.find('/') != std::string_view::npos ||
         value.find('\\') != std::string_view::npos ||
         value.find("..") != std::string_view::npos;
}

ProductWorldCreationResult rejected(std::string_view reason) {
  ProductWorldCreationResult result;
  result.status = reason;
  result.reasonCode = reason;
  return result;
}

ProductWorldInitialSaveResult initialSaveRejected(
    const ProductWorldInitialSaveRequest& request,
    std::string reason) {
  ProductWorldInitialSaveResult result;
  result.status = reason;
  result.reasonCode = std::move(reason);
  result.creation = request.creation;
  result.creation.routeAfterCreate = "world_setup";
  return result;
}

ProductInitialSavePlan makeInitialSavePlan(std::string_view worldId,
                                           std::string_view worldTitle,
                                           std::string_view requestedAtUtc) {
  ProductInitialSavePlan plan;
  plan.requested = true;
  plan.saveType = "initial";
  plan.userTitlePresent = false;
  plan.worldTitle = std::string(worldTitle);
  plan.worldId = std::string(worldId);
  plan.createdAtUtc = std::string(requestedAtUtc);
  plan.savedAtUtc = std::string(requestedAtUtc);
  plan.written = false;
  return plan;
}

}  // namespace

ProductWorldCreationInput makeProductWorldCreationInput(
    const WorldSetupCreateRequest& setupRequest,
    const ProductWorldTemplate& worldTemplate,
    std::filesystem::path saveRoot,
    std::string requestedAtUtc,
    std::string generatedWorldId) {
  ProductWorldCreationInput input;
  input.setupRequest = setupRequest;
  input.worldTemplate = worldTemplate;
  input.saveRoot = std::move(saveRoot);
  input.requestedAtUtc = std::move(requestedAtUtc);
  input.generatedWorldId = std::move(generatedWorldId);
  return input;
}

ProductWorldCreationResult prepareProductWorldCreation(
    const ProductWorldCreationInput& input) {
  if (!input.setupRequest.requested) {
    return rejected("world_creation_not_requested");
  }

  const std::string_view trimmedWorldId = trimAsciiWhitespace(input.generatedWorldId);
  if (trimmedWorldId.empty()) {
    return rejected("world_creation_missing_world_id");
  }
  if (containsInvalidWorldIdToken(trimmedWorldId)) {
    return rejected("world_creation_invalid_world_id");
  }

  if (containsWhitespaceOnly(input.worldTemplate.packageId)) {
    return rejected("world_creation_missing_package_id");
  }

  if (containsWhitespaceOnly(input.worldTemplate.scenarioId)) {
    return rejected("world_creation_missing_scenario_id");
  }

  if (containsWhitespaceOnly(input.requestedAtUtc)) {
    return rejected("world_creation_missing_timestamp");
  }

  const std::string_view trimmedWorldName =
      trimAsciiWhitespace(input.setupRequest.worldName);
  ProductWorldCreationResult result;
  result.accepted = true;
  result.status = "world_creation_request_ready";
  result.reasonCode = "world_creation_request_ready";
  result.request.requested = true;
  result.request.worldId = std::string(trimmedWorldId);
  result.request.worldName = std::string(trimmedWorldName);
  result.request.seedText = std::string(trimAsciiWhitespace(input.setupRequest.seedText));
  result.request.resolvedSeed = input.setupRequest.resolvedSeed;
  result.request.difficulty = input.setupRequest.difficulty;
  result.request.startingScenario = input.setupRequest.startingScenario;
  result.request.packageId = input.worldTemplate.packageId;
  result.request.scenarioId = input.worldTemplate.scenarioId;
  result.request.templateDisplayName = input.worldTemplate.displayName;
  result.request.templateSource = input.worldTemplate.source;
  result.request.asciiRoomRequested = input.setupRequest.asciiRoomRequested;
  result.request.asciiRoomId = input.setupRequest.asciiRoomId;
  result.request.asciiRoomSourceName = input.setupRequest.asciiRoomSourceName;
  result.request.saveRoot = input.saveRoot;
  result.request.requestedAtUtc = std::string(trimAsciiWhitespace(input.requestedAtUtc));
  result.initialSavePlan = makeInitialSavePlan(
      result.request.worldId, result.request.worldName, result.request.requestedAtUtc);
  result.sessionCreated = false;
  result.initialSaveWritten = false;
  result.routeAfterCreate = "world_setup";
  return result;
}

ProductWorldInitialSaveResult writeProductWorldInitialSaveDurably(
    const ProductWorldInitialSaveRequest& request) {
  if (!request.creation.accepted) {
    return initialSaveRejected(request, "world_creation_not_ready");
  }
  if (!request.creation.initialSavePlan.requested) {
    return initialSaveRejected(request,
                               "world_creation_initial_save_not_requested");
  }
  if (request.state == nullptr) {
    return initialSaveRejected(request, "world_creation_session_missing");
  }

  ProductSaveWriteRequest saveRequest;
  saveRequest.saveRoot = request.creation.request.saveRoot;
  saveRequest.saveIdHint = request.creation.initialSavePlan.saveId;
  saveRequest.attemptToken = request.attemptToken;
  saveRequest.state = request.state;
  saveRequest.authoredRoom = request.authoredRoom;
  saveRequest.worldId = request.creation.request.worldId;
  saveRequest.worldTitle = request.creation.initialSavePlan.worldTitle;
  saveRequest.saveTitle = request.creation.initialSavePlan.worldTitle;
  saveRequest.saveType = request.creation.initialSavePlan.saveType;
  saveRequest.createdAtUtc = request.creation.initialSavePlan.createdAtUtc;
  saveRequest.savedAtUtc = request.creation.initialSavePlan.savedAtUtc;

  ProductWorldInitialSaveResult result;
  result.creation = request.creation;
  result.saveWrite = writeProductSessionSaveDurably(saveRequest);
  if (!result.saveWrite.ok) {
    result.status = result.saveWrite.reasonCode;
    result.reasonCode = result.saveWrite.reasonCode;
    result.creation.routeAfterCreate = "world_setup";
    result.creation.initialSaveWritten = false;
    result.creation.initialSavePlan.written = false;
    return result;
  }

  result.ok = true;
  result.status = "world_creation_initial_save_written";
  result.reasonCode = "world_creation_initial_save_written";
  result.creation.status = "world_creation_initial_save_written";
  result.creation.reasonCode = "world_creation_initial_save_written";
  result.creation.accepted = true;
  result.creation.sessionCreated = true;
  result.creation.initialSaveWritten = true;
  result.creation.initialSavePlan.written = true;
  result.creation.initialSavePlan.saveId = result.saveWrite.record.id;
  result.creation.routeAfterCreate = "gameplay";
  return result;
}

}  // namespace iggy3d
