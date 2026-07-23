#pragma once

#include "app/iggy3d/creative/play/NpcSpawn.hpp"
#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"
#include "app/iggy3d/creative/validation/MapValidation.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativePlayPreparationStatus : std::uint8_t {
  NotRequested,
  MissingDocument,
  InvalidDocument,
  InvalidDocumentIdentity,
  ValidationFailed,
  PlayerSpawnUnavailable,
  InteractableCatalogInvalid,
  Prepared,
};

struct CreativePlayPreparationRequest {
  const CreativeDocument* document = nullptr;
  const StaticMeshAssetCatalog* staticMeshAssetCatalog = nullptr;
  std::string roomId = "creative_play";
  std::string playerSpawnGroup =
      std::string(kCreativeDefaultPlayerSpawnGroup);
  float reachabilityCellSizeMeters = 1.0F;
};

// Treat this as an immutable activation snapshot. Runtime owners may consume
// the room, but they must not reinterpret it against a newer document revision.
struct CreativePlayActivationPayload {
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t documentRevision = 0;
  std::string roomId;
  CreativeObjectId playerSpawnObjectId = kInvalidObjectId;
  RoomAnchorAsset playerSpawn;
  CreativePlayerSpawnSettings playerSpawnSettings{};
  float playerSpawnYawRadians = 0.0F;
  Vec3 playerSpawnCameraPositionMeters;
  std::vector<CreativeNpcSpawnPlan> npcSpawns;
  std::vector<CreativeNpcPatrolRoutePlan> npcPatrolRoutes;
  std::vector<CreativeRuntimeInteractableDefinition> interactables;
  std::vector<CreativeRuntimeLogicLink> logicLinks;
  RoomAsset room;
};

struct CreativePlayPreparationResult {
  bool requested = false;
  bool accepted = false;
  CreativePlayPreparationStatus status =
      CreativePlayPreparationStatus::NotRequested;
  std::string_view reasonCode = "creative_play_not_requested";
  CreativeMapValidationResult validation;
  CreativeNpcSpawnPlanResult npcSpawns;
  std::optional<CreativePlayActivationPayload> payload;
};

[[nodiscard]] std::string_view toString(
    CreativePlayPreparationStatus status) noexcept;

[[nodiscard]] CreativePlayPreparationResult prepareCreativePlay(
    const CreativePlayPreparationRequest& request);

[[nodiscard]] bool creativePlayActivationIsCurrent(
    const CreativePlayActivationPayload& payload,
    const CreativeDocument& document) noexcept;

}  // namespace iggy3d::creative
