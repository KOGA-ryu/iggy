#pragma once

#include <vector>

#include "content/FixtureScenarioLoader.hpp"
#include "content/PackageManifest.hpp"
#include "core/diagnostics/Diagnostic.hpp"

namespace iggy3d {

enum class PackageValidationStatus : std::uint8_t {
  Ok,
  MissingPackageId,
  WrongPackageId,
  UnsupportedSchemaVersion,
  InvalidScenarioPath,
  MissingScenarioId,
  WrongScenarioId,
  MissingStableName,
  DuplicateStableName,
  MissingPlayerBinding,
  MissingPlayerEntity,
  NonFiniteTransform,
  InvalidBounds,
  MissingGoldKeyInteraction,
  InvalidObjectiveCondition,
  MissingAiActorEntity,
  NonNpcAiActor,
  DuplicateAiActorBinding,
  InvalidAiActorProfileId,
  MissingGuardActor,
  GuardActorNotFound,
  NonNpcGuardActor,
  DuplicateGuardActor,
  MissingGuardAnchor,
  GuardAnchorNotFound,
  NonMarkerGuardAnchor,
  InvalidGuardLeashRadius,
  InvalidGuardReturnRadius,
  InvalidGuardHomeTolerance,
  GuardReturnExceedsLeash,
  OldIggyDependency,
};

struct PackageValidationRequest {
  PackageManifest manifest;
  FixtureScenarioSeed scenario;
};

struct PackageValidationResult {
  PackageValidationStatus status = PackageValidationStatus::Ok;
  std::vector<Diagnostic> diagnostics;
};

PackageValidationResult validatePackage(const PackageValidationRequest& request);

}  // namespace iggy3d
