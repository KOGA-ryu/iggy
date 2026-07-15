#include "app/iggy3d/creative/play/PlayPreparation.hpp"

#include <algorithm>
#include <utility>

namespace iggy3d::creative {
namespace {

void setStatus(CreativePlayPreparationResult& result,
               CreativePlayPreparationStatus status,
               std::string_view reasonCode,
               bool accepted = false) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.accepted = accepted;
}

void setValidationFailureStatus(CreativePlayPreparationResult& result) {
  switch (result.validation.status) {
    case CreativeMapValidationStatus::MissingDocument:
      setStatus(result,
                CreativePlayPreparationStatus::MissingDocument,
                "creative_play_document_missing");
      return;
    case CreativeMapValidationStatus::InvalidDocument:
      setStatus(result,
                CreativePlayPreparationStatus::InvalidDocument,
                "creative_play_document_invalid");
      return;
    case CreativeMapValidationStatus::NotRequested:
    case CreativeMapValidationStatus::Validated:
      setStatus(result,
                CreativePlayPreparationStatus::ValidationFailed,
                "creative_play_validation_failed");
      return;
  }
}

}  // namespace

std::string_view toString(CreativePlayPreparationStatus status) noexcept {
  switch (status) {
    case CreativePlayPreparationStatus::NotRequested:
      return "not_requested";
    case CreativePlayPreparationStatus::MissingDocument:
      return "missing_document";
    case CreativePlayPreparationStatus::InvalidDocument:
      return "invalid_document";
    case CreativePlayPreparationStatus::InvalidDocumentIdentity:
      return "invalid_document_identity";
    case CreativePlayPreparationStatus::ValidationFailed:
      return "validation_failed";
    case CreativePlayPreparationStatus::PlayerSpawnUnavailable:
      return "player_spawn_unavailable";
    case CreativePlayPreparationStatus::InteractableCatalogInvalid:
      return "interactable_catalog_invalid";
    case CreativePlayPreparationStatus::Prepared:
      return "prepared";
  }
  return "not_requested";
}

CreativePlayPreparationResult prepareCreativePlay(
    const CreativePlayPreparationRequest& request) {
  CreativePlayPreparationResult result;
  result.requested = true;

  CreativeMapValidationRequest validationRequest;
  validationRequest.document = request.document;
  validationRequest.staticMeshAssetCatalog =
      request.staticMeshAssetCatalog;
  validationRequest.roomId = request.roomId;
  validationRequest.reachabilityCellSizeMeters =
      request.reachabilityCellSizeMeters;
  CreativeMapEvaluationResult evaluation =
      evaluateCreativeMap(validationRequest);
  result.validation = std::move(evaluation.validation);

  if (!result.validation.accepted) {
    setValidationFailureStatus(result);
    return result;
  }
  if (!result.validation.passed) {
    setStatus(result,
              CreativePlayPreparationStatus::ValidationFailed,
              "creative_play_validation_failed");
    return result;
  }
  if (request.document == nullptr ||
      request.document->id() == kInvalidDocumentId) {
    setStatus(result,
              CreativePlayPreparationStatus::InvalidDocumentIdentity,
              "creative_play_document_identity_invalid");
    return result;
  }

  const auto spawn = std::find_if(
      evaluation.roomBake.room.anchors.begin(),
      evaluation.roomBake.room.anchors.end(),
      [](const RoomAnchorAsset& anchor) { return anchor.kind == "spawn"; });
  if (spawn == evaluation.roomBake.room.anchors.end()) {
    setStatus(result,
              CreativePlayPreparationStatus::PlayerSpawnUnavailable,
              "creative_play_player_spawn_unavailable");
    return result;
  }

  CreativeRuntimeInteractableCatalog interactables =
      buildCreativeRuntimeInteractableCatalog(*request.document,
                                              evaluation.roomBake.room);
  if (!interactables.ok) {
    setStatus(result,
              CreativePlayPreparationStatus::InteractableCatalogInvalid,
              interactables.reasonCode);
    return result;
  }

  CreativePlayActivationPayload payload;
  payload.documentId = request.document->id();
  payload.documentRevision = request.document->revision();
  payload.roomId = evaluation.roomBake.room.id;
  payload.playerSpawn = *spawn;
  payload.interactables = std::move(interactables.definitions);
  payload.logicLinks = std::move(interactables.logicLinks);
  payload.room = std::move(evaluation.roomBake.room);
  result.payload.emplace(std::move(payload));
  setStatus(result,
            CreativePlayPreparationStatus::Prepared,
            "creative_play_prepared",
            true);
  return result;
}

bool creativePlayActivationIsCurrent(
    const CreativePlayActivationPayload& payload,
    const CreativeDocument& document) noexcept {
  return document.isValid() &&
         payload.documentId != kInvalidDocumentId &&
         payload.documentId == document.id() &&
         payload.documentRevision == document.revision();
}

}  // namespace iggy3d::creative
