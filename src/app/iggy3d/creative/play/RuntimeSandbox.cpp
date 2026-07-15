#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "runtime/ai/NpcBehaviorProfile.hpp"

namespace iggy3d::creative {
namespace {

constexpr std::uint32_t kPlayerFaction = 1U;
constexpr std::uint32_t kMonsterFaction = 2U;
constexpr Aabb3 kPlayerBounds{{-0.35F, 0.0F, -0.35F},
                              {0.35F, 1.8F, 0.35F}};
constexpr Aabb3 kNpcBounds{{-0.30F, 0.0F, -0.30F},
                           {0.30F, 1.8F, 0.30F}};

enum class RuntimeAnchorActorKind : std::uint8_t {
  Npc,
  Monster,
};

struct RuntimeAnchorActorPolicy {
  std::string_view anchorKind;
  RuntimeAnchorActorKind actorKind;
};

constexpr auto kRuntimeAnchorActorPolicies =
    std::to_array<RuntimeAnchorActorPolicy>({
        {"npc", RuntimeAnchorActorKind::Npc},
        {"monster", RuntimeAnchorActorKind::Monster},
    });

const RuntimeAnchorActorPolicy* actorPolicyFor(
    std::string_view anchorKind) noexcept {
  const auto found = std::find_if(
      kRuntimeAnchorActorPolicies.begin(), kRuntimeAnchorActorPolicies.end(),
      [anchorKind](const RuntimeAnchorActorPolicy& policy) {
        return policy.anchorKind == anchorKind;
      });
  return found == kRuntimeAnchorActorPolicies.end() ? nullptr : &*found;
}

bool samePosition(Vec3 lhs, Vec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameAnchor(const RoomAnchorAsset& lhs,
                const RoomAnchorAsset& rhs) noexcept {
  return lhs.id == rhs.id && lhs.kind == rhs.kind &&
         lhs.runtimeStableName == rhs.runtimeStableName &&
         samePosition(lhs.positionMeters, rhs.positionMeters);
}

bool anchorIsValid(const RoomAnchorAsset& anchor) noexcept {
  return !anchor.id.empty() && !anchor.kind.empty() &&
         !anchor.runtimeStableName.empty() && isFinite(anchor.positionMeters);
}

bool anchorsHaveUniqueIdentity(
    const std::vector<RoomAnchorAsset>& anchors) noexcept {
  for (std::size_t index = 0; index < anchors.size(); ++index) {
    for (std::size_t other = index + 1U; other < anchors.size(); ++other) {
      if (anchors[index].id == anchors[other].id ||
          anchors[index].runtimeStableName ==
              anchors[other].runtimeStableName) {
        return false;
      }
    }
  }
  return true;
}

bool surfacesHaveUniqueIdentity(
    const std::vector<RoomSpatialSurface>& surfaces) noexcept {
  for (std::size_t index = 0; index < surfaces.size(); ++index) {
    if (surfaces[index].id.empty()) {
      return false;
    }
    for (std::size_t other = index + 1U; other < surfaces.size(); ++other) {
      if (surfaces[index].id == surfaces[other].id) {
        return false;
      }
    }
  }
  return true;
}

bool payloadShapeIsValid(
    const CreativePlayActivationPayload& payload) noexcept {
  if (payload.documentId == kInvalidDocumentId || payload.roomId.empty() ||
      payload.room.id != payload.roomId ||
      payload.playerSpawn.kind != "spawn" ||
      !anchorIsValid(payload.playerSpawn) || payload.room.anchors.empty()) {
    return false;
  }

  std::size_t spawnCount = 0U;
  bool matchedPreparedSpawn = false;
  for (const RoomAnchorAsset& anchor : payload.room.anchors) {
    if (!anchorIsValid(anchor)) {
      return false;
    }
    if (anchor.kind == "spawn") {
      ++spawnCount;
      matchedPreparedSpawn = matchedPreparedSpawn ||
                             sameAnchor(anchor, payload.playerSpawn);
    }
  }
  return spawnCount == 1U && matchedPreparedSpawn &&
         anchorsHaveUniqueIdentity(payload.room.anchors);
}

bool sandboxConfigIsValid(const CreativeRuntimeSandboxConfig& config) {
  if (validateRuntimeConfig(config.runtimeConfig) != RuntimeConfigStatus::Ok ||
      config.packageId.empty() || config.playerHitPoints <= 0 ||
      config.npcHitPoints <= 0 || config.monsterHitPoints <= 0) {
    return false;
  }

  const NpcBehaviorProfileCatalog catalog =
      makeBuiltInNpcBehaviorProfileCatalog();
  return resolveNpcBehaviorProfile(
             {&catalog, config.npcBehaviorProfileId})
             .ok &&
         resolveNpcBehaviorProfile(
             {&catalog, config.monsterBehaviorProfileId})
             .ok;
}

ScenarioEntitySeed makeCombatantEntity(const RoomAnchorAsset& anchor,
                                       ScenarioEntityKind kind,
                                       Aabb3 bounds,
                                       std::uint32_t factionId,
                                       std::int32_t hitPoints) {
  ScenarioEntitySeed entity;
  entity.stableName = anchor.runtimeStableName;
  entity.kind = kind;
  entity.transform = identityTransform3();
  entity.transform.position = anchor.positionMeters;
  entity.localBounds = bounds;
  entity.active = true;
  entity.persistent = true;
  entity.targeting.targetable = true;
  entity.targeting.actions = {ScenarioTargetAction::Attack,
                              ScenarioTargetAction::Interact,
                              ScenarioTargetAction::Inspect};
  entity.interaction.kind = ScenarioInteractionKind::Inspect;
  entity.interaction.repeatable = true;
  entity.combatantEnabled = true;
  entity.combatant.factionId = factionId;
  entity.combatant.hitPoints = hitPoints;
  entity.combatant.maxHitPoints = hitPoints;
  return entity;
}

ScenarioObjectiveSeed makeSandboxObjective() {
  ScenarioObjectiveSeed objective;
  objective.id = "creative_sandbox_active";
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0U;
  objective.completeStatus = ObjectiveStatusSeed::Complete;
  return objective;
}

void setActivationStatus(CreativeRuntimeSandboxActivationReceipt& receipt,
                         CreativeRuntimeSandboxActivationStatus status,
                         std::string reasonCode,
                         bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.accepted = accepted;
}

}  // namespace

std::string_view toString(CreativeRuntimeScenarioSeedStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeScenarioSeedStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeScenarioSeedStatus::InvalidConfig:
      return "invalid_config";
    case CreativeRuntimeScenarioSeedStatus::InvalidPayload:
      return "invalid_payload";
    case CreativeRuntimeScenarioSeedStatus::Built:
      return "built";
  }
  return "not_requested";
}

CreativeRuntimeScenarioSeedResult buildCreativeRuntimeScenarioSeed(
    const CreativePlayActivationPayload& payload,
    const CreativeRuntimeSandboxConfig& config) {
  CreativeRuntimeScenarioSeedResult result;
  result.summary.sourceAnchorCount = payload.room.anchors.size();
  if (!sandboxConfigIsValid(config)) {
    result.status = CreativeRuntimeScenarioSeedStatus::InvalidConfig;
    result.reasonCode = "creative_runtime_seed_config_invalid";
    return result;
  }
  if (!payloadShapeIsValid(payload)) {
    result.status = CreativeRuntimeScenarioSeedStatus::InvalidPayload;
    result.reasonCode = "creative_runtime_seed_payload_invalid";
    return result;
  }

  FixtureScenarioSeed seed;
  seed.scenarioId = payload.roomId + ".creative_play";
  seed.config = config.runtimeConfig;
  seed.initialClockMode = ScenarioClockMode::Normal;
  seed.defaultRealtimeCamera = ScenarioCameraMode::FirstPerson;
  seed.defaultTacticalCamera = ScenarioCameraMode::TacticalOverhead;
  seed.players.push_back(
      {0U, ScenarioPlayerSlotKind::Local,
       payload.playerSpawn.runtimeStableName});
  seed.entities.push_back(makeCombatantEntity(
      payload.playerSpawn, ScenarioEntityKind::Player, kPlayerBounds,
      kPlayerFaction, config.playerHitPoints));
  seed.objectives.push_back(makeSandboxObjective());
  result.summary.playerEntityCount = 1U;

  for (const RoomAnchorAsset& anchor : payload.room.anchors) {
    const RuntimeAnchorActorPolicy* policy = actorPolicyFor(anchor.kind);
    if (policy == nullptr) {
      if (anchor.kind != "spawn") {
        ++result.summary.ignoredAnchorCount;
      }
      continue;
    }

    const bool monster =
        policy->actorKind == RuntimeAnchorActorKind::Monster;
    const std::string& profileId =
        monster ? config.monsterBehaviorProfileId
                : config.npcBehaviorProfileId;
    const std::uint32_t factionId =
        monster ? kMonsterFaction : kPlayerFaction;
    const std::int32_t hitPoints =
        monster ? config.monsterHitPoints : config.npcHitPoints;
    seed.entities.push_back(makeCombatantEntity(
        anchor, ScenarioEntityKind::Npc, kNpcBounds, factionId, hitPoints));
    seed.aiActors.push_back({anchor.runtimeStableName, profileId});
    if (monster) {
      ++result.summary.monsterEntityCount;
    } else {
      ++result.summary.npcEntityCount;
    }
  }

  result.accepted = true;
  result.status = CreativeRuntimeScenarioSeedStatus::Built;
  result.reasonCode = "creative_runtime_seed_built";
  result.seed = std::move(seed);
  return result;
}

std::string_view toString(
    CreativeRuntimeSandboxActivationStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeSandboxActivationStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeSandboxActivationStatus::MissingDocument:
      return "missing_document";
    case CreativeRuntimeSandboxActivationStatus::StalePayload:
      return "stale_payload";
    case CreativeRuntimeSandboxActivationStatus::InvalidConfig:
      return "invalid_config";
    case CreativeRuntimeSandboxActivationStatus::InvalidPayload:
      return "invalid_payload";
    case CreativeRuntimeSandboxActivationStatus::InvalidCollisionSurfaces:
      return "invalid_collision_surfaces";
    case CreativeRuntimeSandboxActivationStatus::SessionCreationFailed:
      return "session_creation_failed";
    case CreativeRuntimeSandboxActivationStatus::Activated:
      return "activated";
  }
  return "not_requested";
}

CreativeRuntimeSandboxActivationResult activateCreativeRuntimeSandbox(
    CreativeRuntimeSandboxActivationRequest request) {
  CreativeRuntimeSandboxActivationResult result;
  CreativeRuntimeSandboxActivationReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentId = request.payload.documentId;
  receipt.documentRevision = request.payload.documentRevision;
  receipt.roomId = request.payload.roomId;

  if (request.sourceDocument == nullptr) {
    setActivationStatus(
        receipt, CreativeRuntimeSandboxActivationStatus::MissingDocument,
        "creative_runtime_sandbox_document_missing");
    return result;
  }
  if (!creativePlayActivationIsCurrent(request.payload,
                                       *request.sourceDocument)) {
    setActivationStatus(receipt,
                        CreativeRuntimeSandboxActivationStatus::StalePayload,
                        "creative_runtime_sandbox_payload_stale");
    return result;
  }

  CreativeRuntimeScenarioSeedResult seed =
      buildCreativeRuntimeScenarioSeed(request.payload, request.config);
  receipt.scenario = seed.summary;
  if (!seed.accepted) {
    const CreativeRuntimeSandboxActivationStatus status =
        seed.status == CreativeRuntimeScenarioSeedStatus::InvalidConfig
            ? CreativeRuntimeSandboxActivationStatus::InvalidConfig
            : CreativeRuntimeSandboxActivationStatus::InvalidPayload;
    setActivationStatus(receipt, status, std::string(seed.reasonCode));
    return result;
  }

  SpatialSurfaceSet collisionSurfaces =
      buildSpatialSurfaceSet(request.payload.room);
  receipt.collisionSurfaceCount = collisionSurfaces.size();
  if (request.payload.room.spatialSurfaces.empty() ||
      collisionSurfaces.size() !=
          request.payload.room.spatialSurfaces.size() ||
      !surfacesHaveUniqueIdentity(request.payload.room.spatialSurfaces)) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::InvalidCollisionSurfaces,
        "creative_runtime_sandbox_collision_surfaces_invalid");
    return result;
  }

  ReasoningGraph reasoningGraph =
      buildReasoningGraph(request.payload.room, {});
  receipt.reasoningGraph = summarizeReasoningGraph(reasoningGraph);

  SessionCreateRequest sessionRequest;
  sessionRequest.packageId = request.config.packageId;
  sessionRequest.config = request.config.runtimeConfig;
  sessionRequest.seed = std::move(seed.seed);
  Result<Session> created = Session::create(sessionRequest);
  if (created.status != ResultStatus::Ok) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::SessionCreationFailed,
        created.error.code);
    return result;
  }
  created.value.setReasoningGraph(std::move(reasoningGraph));
  receipt.initialStateHash = created.value.stateHash();

  CreativeRuntimeSandbox sandbox;
  sandbox.sourceDocumentId = request.payload.documentId;
  sandbox.sourceDocumentRevision = request.payload.documentRevision;
  sandbox.roomId = request.payload.roomId;
  sandbox.scenario = receipt.scenario;
  sandbox.reasoningGraph = receipt.reasoningGraph;
  sandbox.room = std::move(request.payload.room);
  sandbox.collisionSurfaces = std::move(collisionSurfaces);
  sandbox.session = std::move(created.value);
  result.sandbox.emplace(std::move(sandbox));
  setActivationStatus(receipt,
                      CreativeRuntimeSandboxActivationStatus::Activated,
                      "creative_runtime_sandbox_activated", true);
  return result;
}

bool creativeRuntimeSandboxIsCurrent(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeDocument& document) noexcept {
  return document.isValid() &&
         sandbox.sourceDocumentId != kInvalidDocumentId &&
         sandbox.sourceDocumentId == document.id() &&
         sandbox.sourceDocumentRevision == document.revision();
}

std::string_view toString(CreativeRuntimeSandboxStopStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeSandboxStopStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeSandboxStopStatus::NoActiveSandbox:
      return "no_active_sandbox";
    case CreativeRuntimeSandboxStopStatus::Stopped:
      return "stopped";
  }
  return "not_requested";
}

CreativeRuntimeSandboxStopReceipt stopCreativeRuntimeSandbox(
    std::optional<CreativeRuntimeSandbox>& sandbox) noexcept {
  CreativeRuntimeSandboxStopReceipt receipt;
  receipt.requested = true;
  if (!sandbox.has_value()) {
    receipt.status = CreativeRuntimeSandboxStopStatus::NoActiveSandbox;
    receipt.reasonCode = "creative_runtime_sandbox_not_active";
    return receipt;
  }

  receipt.documentId = sandbox->sourceDocumentId;
  receipt.documentRevision = sandbox->sourceDocumentRevision;
  receipt.finalStateHash = sandbox->session.stateHash();
  sandbox.reset();
  receipt.stopped = true;
  receipt.status = CreativeRuntimeSandboxStopStatus::Stopped;
  receipt.reasonCode = "creative_runtime_sandbox_stopped";
  return receipt;
}

}  // namespace iggy3d::creative
