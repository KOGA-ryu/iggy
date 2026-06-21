#include "content/PackageValidator.hpp"

#include <algorithm>

namespace iggy3d {

namespace {

std::string forbiddenLegacyPrefix() {
  return std::string{"/Users/kogaryu/"} + "iggy";
}

bool hasOldDependency(std::string_view value) {
  return value.find(forbiddenLegacyPrefix()) != std::string_view::npos ||
         value.find("old_iggy_dependency") != std::string_view::npos;
}

Diagnostic diagnostic(std::string code, std::string message, std::uint32_t line = 0,
                      std::uint32_t column = 0) {
  return makeDiagnostic(DiagnosticDomain::Content, DiagnosticSeverity::Error, std::move(code),
                        std::move(message), DiagnosticLocation{"", line, column});
}

PackageValidationResult fail(PackageValidationStatus status, std::string code, std::string message,
                             std::uint32_t line = 0, std::uint32_t column = 0) {
  PackageValidationResult result;
  result.status = status;
  result.diagnostics.push_back(diagnostic(std::move(code), std::move(message), line, column));
  return result;
}

bool scenarioPathValid(const std::string& path) {
  return path == "scenario.iggy3d.toml";
}

const ScenarioEntitySeed* findEntity(const FixtureScenarioSeed& seed, std::string_view stableName) {
  for (const ScenarioEntitySeed& entity : seed.entities) {
    if (entity.stableName == stableName) {
      return &entity;
    }
  }
  return nullptr;
}

const ScenarioObjectiveSeed* findObjective(const FixtureScenarioSeed& seed, std::string_view id) {
  for (const ScenarioObjectiveSeed& objective : seed.objectives) {
    if (objective.id == id) {
      return &objective;
    }
  }
  return nullptr;
}

bool hasAction(const ScenarioEntitySeed& entity, TargetAction action) {
  return isTargetActionSupported(entity.targeting, action);
}

}  // namespace

PackageValidationResult validatePackage(const PackageValidationRequest& request) {
  if (request.manifest.packageId.empty()) {
    return fail(PackageValidationStatus::MissingPackageId, "package.missing_id", "missing package id");
  }
  if (request.manifest.packageId != "iggy3d.first_room") {
    return fail(PackageValidationStatus::WrongPackageId, "package.wrong_id", "wrong package id");
  }
  if (request.manifest.schemaVersion != 1U || request.manifest.requiredRuntimeSchema != 1U) {
    return fail(PackageValidationStatus::UnsupportedSchemaVersion, "package.unsupported_schema",
                "unsupported schema");
  }
  if (!scenarioPathValid(request.manifest.scenarioPath)) {
    return fail(PackageValidationStatus::InvalidScenarioPath, "package.invalid_scenario_path",
                "invalid scenario path");
  }
  if (hasOldDependency(request.manifest.packageId) || hasOldDependency(request.manifest.scenarioPath)) {
    return fail(PackageValidationStatus::OldIggyDependency, "package.old_iggy_dependency",
                "old dependency");
  }
  for (const PackageAssetRef& asset : request.manifest.assets) {
    if (hasOldDependency(asset.id) || hasOldDependency(asset.path)) {
      return fail(PackageValidationStatus::OldIggyDependency, "package.old_iggy_dependency",
                  "old dependency");
    }
  }
  if (request.scenario.scenarioId.empty()) {
    return fail(PackageValidationStatus::MissingScenarioId, "scenario.missing_id",
                "missing scenario id");
  }
  if (request.scenario.scenarioId != "first_room.runtime_loop") {
    return fail(PackageValidationStatus::WrongScenarioId, "scenario.wrong_id", "wrong scenario id");
  }
  for (std::size_t i = 0; i < request.scenario.entities.size(); ++i) {
    const ScenarioEntitySeed& entity = request.scenario.entities[i];
    const std::uint32_t line = static_cast<std::uint32_t>(i + 1U);
    if (entity.stableName.empty()) {
      return fail(PackageValidationStatus::MissingStableName, "scenario.missing_stable_name",
                  "missing stable name", line, 1);
    }
    for (std::size_t j = 0; j < i; ++j) {
      if (request.scenario.entities[j].stableName == entity.stableName) {
        return fail(PackageValidationStatus::DuplicateStableName,
                    "scenario.duplicate_stable_name", "duplicate stable name " + entity.stableName, line, 1);
      }
    }
  }
  bool foundSlot0 = false;
  for (const ScenarioPlayerSeed& player : request.scenario.players) {
    if (player.slot == 0U && player.kind == PlayerSlotKind::Local &&
        player.actorStableName == "player") {
      foundSlot0 = true;
    }
  }
  if (!foundSlot0) {
    return fail(PackageValidationStatus::MissingPlayerBinding, "scenario.missing_player_binding",
                "missing player binding");
  }
  const ScenarioEntitySeed* player = findEntity(request.scenario, "player");
  if (player == nullptr) {
    return fail(PackageValidationStatus::MissingPlayerEntity, "scenario.missing_player_entity",
                "missing player entity");
  }
  for (const ScenarioEntitySeed& entity : request.scenario.entities) {
    if (!isFinite(entity.transform) || !hasPositiveFiniteScale(entity.transform)) {
      return fail(PackageValidationStatus::NonFiniteTransform, "scenario.non_finite_transform",
                  "non finite transform");
    }
    if (!isValid(entity.localBounds)) {
      return fail(PackageValidationStatus::InvalidBounds, "scenario.invalid_bounds", "invalid bounds");
    }
  }
  const ScenarioEntitySeed* goldKey = findEntity(request.scenario, "gold_key");
  if (goldKey == nullptr || goldKey->kind != EntityKind::Pickup ||
      !nearlyEqual(goldKey->transform.position, Vec3{3.0F, 0.0F, 0.0F}) ||
      !hasAction(*goldKey, TargetAction::Interact) || !hasAction(*goldKey, TargetAction::Inspect) ||
      goldKey->interaction.kind != InteractionKind::Pickup ||
      goldKey->interaction.primaryEffect != InteractionEffectKind::AddItemToInventory ||
      goldKey->interaction.itemId != "gold_key" || goldKey->interaction.itemCount != 1U ||
      goldKey->interaction.objectiveId != "collect_gold_key" || goldKey->interaction.repeatable ||
      !goldKey->interaction.deactivateTargetOnSuccess) {
    return fail(PackageValidationStatus::MissingGoldKeyInteraction,
                "scenario.missing_gold_key_interaction", "missing gold key interaction");
  }
  const ScenarioObjectiveSeed* objective = findObjective(request.scenario, "collect_gold_key");
  if (objective == nullptr || objective->initialStatus != ObjectiveStatusSeed::Active ||
      objective->condition != "InventoryContains" || objective->playerSlot != 0U ||
      objective->itemId != "gold_key" || objective->itemCount != 1U ||
      objective->completeStatus != ObjectiveStatusSeed::Complete) {
    return fail(PackageValidationStatus::InvalidObjectiveCondition,
                "scenario.invalid_objective_condition", "invalid objective condition");
  }
  return {};
}

}  // namespace iggy3d
