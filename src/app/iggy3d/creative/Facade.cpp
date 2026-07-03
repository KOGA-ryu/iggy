#include "app/iggy3d/creative/Facade.hpp"

#include <limits>
#include <span>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool targetRefToObjectId(TargetRef target,
                                       CreativeObjectId& objectId) noexcept {
  if (target.value == kInvalidId) {
    return false;
  }

  if constexpr (std::numeric_limits<Id>::max() >
                std::numeric_limits<CreativeObjectId>::max()) {
    if (target.value > std::numeric_limits<CreativeObjectId>::max()) {
      return false;
    }
  }

  objectId = static_cast<CreativeObjectId>(target.value);
  return objectId != kInvalidObjectId;
}

[[nodiscard]] TargetRef objectIdToTargetRef(CreativeObjectId objectId) noexcept {
  if (objectId == kInvalidObjectId ||
      objectId > std::numeric_limits<Id>::max()) {
    return {};
  }

  return TargetRef{static_cast<Id>(objectId)};
}

}  // namespace

std::string_view toString(CreativeFacadeMutationStatus status) noexcept {
  switch (status) {
    case CreativeFacadeMutationStatus::Unknown:
      return "Unknown";
    case CreativeFacadeMutationStatus::NoSelection:
      return "NoSelection";
    case CreativeFacadeMutationStatus::MissingObject:
      return "MissingObject";
    case CreativeFacadeMutationStatus::Applied:
      return "Applied";
    case CreativeFacadeMutationStatus::NoChange:
      return "NoChange";
    case CreativeFacadeMutationStatus::Rejected:
      return "Rejected";
  }
  return "Unknown";
}

void Facade::reset() noexcept {
  state_ = State{};
  document_.reset();
  resetStats(stats_);
  toolState_ = makeDefaultCreativeToolState();
  selectionState_ = makeDefaultCreativeSelectionState();
  inspectionState_ = makeDefaultCreativeInspectionState();
  measurementState_ = makeDefaultCreativeMeasurementState();
  snapSettings_ = makeDefaultCreativeSnapSettings();
  ghostState_ = makeDefaultCreativeGhostState();
}

void Facade::beginFrame(const FramePacket& packet) noexcept {
  state_.frame = packet.frame;
  state_.flags = packet.flags;
}

void Facade::handle(const Packet& packet) noexcept {
  state_.frame = packet.frame;
  state_.hovered = packet.target;
  state_.flags = packet.flags;
}

const State& Facade::state() const noexcept {
  return state_;
}

const CreativeToolState& Facade::toolState() const noexcept {
  return toolState_;
}

const CreativeSelectionState& Facade::selectionState() const noexcept {
  return selectionState_;
}

const CreativeInspectionState& Facade::inspectionState() const noexcept {
  return inspectionState_;
}

const CreativeMeasurementState& Facade::measurementState() const noexcept {
  return measurementState_;
}

const CreativeSnapSettings& Facade::snapSettings() const noexcept {
  return snapSettings_;
}

const CreativeGhostState& Facade::ghostState() const noexcept {
  return ghostState_;
}

bool Facade::setActiveTool(Tool tool) noexcept {
  const Tool activeToolBefore = toolState_.activeTool;
  const bool changed = iggy3d::creative::setActiveTool(toolState_, tool);
  if (changed && activeToolBefore == Tool::Measure &&
      toolState_.activeTool != Tool::Measure && measurementState_.active) {
    static_cast<void>(cancelMeasurement(measurementState_));
  }
  state_.tool = toolState_.activeTool;
  return changed;
}

void Facade::setSnapSettings(CreativeSnapSettings settings) noexcept {
  snapSettings_ = settings;
}

CreativeFacadeToolDispatchReceipt Facade::dispatchToolInput(
    const CreativeToolInputPacket& input) {
  CreativeFacadeToolDispatchReceipt receipt;
  receipt.inputKind = input.kind;
  receipt.activeToolBefore = toolState_.activeTool;

  const CreativeToolDispatchReceipt toolReceipt =
      iggy3d::creative::dispatchToolInput(toolState_, input);
  receipt.activeToolAfter = toolState_.activeTool;
  receipt.emittedIntentCount = toolReceipt.emittedIntentCount;
  receipt.toolAccepted = toolReceipt.accepted;
  receipt.accepted = toolReceipt.accepted;
  receipt.changed = toolReceipt.changedState;
  receipt.message = toolReceipt.message;

  for (const CreativeToolIntent& intent : toolReceipt.intents) {
    const CreativeSelectionReceipt selectionReceipt =
        applySelectionToolIntent(selectionState_, intent);
    const CreativeInspectionReceipt inspectionReceipt =
        applyInspectionToolIntent(inspectionState_, intent);
    const CreativeMeasurementReceipt measurementReceipt =
        applyMeasurementToolIntent(measurementState_, intent);
    const CreativeGhostReceipt ghostReceipt =
        applyGhostToolIntent(ghostState_, intent, snapSettings_);

    receipt.selectionChanged = receipt.selectionChanged ||
                               selectionReceipt.changed;
    receipt.inspectionChanged = receipt.inspectionChanged ||
                                inspectionReceipt.changed;
    receipt.measurementChanged = receipt.measurementChanged ||
                                 measurementReceipt.changed;
    receipt.ghostChanged = receipt.ghostChanged || ghostReceipt.changed;
  }

  receipt.changed = receipt.changed || receipt.selectionChanged ||
                    receipt.inspectionChanged || receipt.measurementChanged ||
                    receipt.ghostChanged;
  state_.tool = toolState_.activeTool;
  state_.selected = selectionState_.selectedTarget;
  state_.hovered = toolState_.pointer.target;
  return receipt;
}

CreativeUiBuildReceipt Facade::buildUiModel() const {
  CreativeUiBuildRequest request;
  request.toolState = toolState_;
  request.selectionState = selectionState_;
  request.inspectionState = inspectionState_;
  request.measurementState = measurementState_;
  request.snapSettings = snapSettings_;
  request.ghostState = ghostState_;
  const std::span<const CreativeObject> objects = document_.objects();
  request.objectSummaries.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    const TargetRef target = objectIdToTargetRef(object.id);
    if (target.value == kInvalidId) {
      continue;
    }

    CreativeUiObjectSummary summary;
    summary.target = target;
    summary.objectKind = object.kind;
    summary.exists = true;
    summary.visible = object.visible;
    request.objectSummaries.push_back(summary);
  }
  return buildCreativeUiModel(request);
}

CreativeFacadeMutationReceipt Facade::toggleSelectedObjectVisibility() {
  CreativeFacadeMutationReceipt receipt;
  receipt.requested = true;
  receipt.target = selectionState_.selectedTarget;
  receipt.revisionBefore = document_.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  if (receipt.target.value == kInvalidId) {
    receipt.status = CreativeFacadeMutationStatus::NoSelection;
    receipt.message = "no_selection";
    return receipt;
  }

  receipt.hadSelection = true;
  receipt.mutationKind = CreativeMutationKind::SetVisible;

  CreativeObjectId objectId = kInvalidObjectId;
  if (!targetRefToObjectId(receipt.target, objectId)) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }
  receipt.objectId = objectId;

  const CreativeObject* object = document_.findObject(objectId);
  if (object == nullptr) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }

  receipt.objectKind = object->kind;
  receipt.visibleBefore = object->visible;

  const CreativeDocumentMutationReceipt documentReceipt =
      setDocumentObjectVisible(document_, objectId, !receipt.visibleBefore);

  receipt.accepted = documentMutationSucceeded(documentReceipt.status);
  receipt.changed = documentReceipt.changed &&
                    documentReceipt.revisionAfter !=
                        documentReceipt.revisionBefore;
  receipt.documentStatus = documentReceipt.status;
  receipt.mutationKind = documentReceipt.mutationKind;
  receipt.revisionBefore = documentReceipt.revisionBefore;
  receipt.revisionAfter = documentReceipt.revisionAfter;
  receipt.message = documentReceipt.message;

  const CreativeObject* objectAfter = document_.findObject(objectId);
  if (objectAfter != nullptr) {
    receipt.objectKind = objectAfter->kind;
    receipt.visibleAfter = objectAfter->visible;
  } else {
    receipt.visibleAfter = receipt.visibleBefore;
  }

  if (documentReceipt.status == CreativeDocumentMutationStatus::Applied &&
      receipt.changed) {
    receipt.status = CreativeFacadeMutationStatus::Applied;
  } else if (documentReceipt.status == CreativeDocumentMutationStatus::NoChange) {
    receipt.status = CreativeFacadeMutationStatus::NoChange;
  } else {
    receipt.status = CreativeFacadeMutationStatus::Rejected;
  }

  return receipt;
}

CreativeDocumentCreateReceipt Facade::createDocumentObject(
    const CreativeDocumentCreateRequest& request) {
  recordCommandAttempt(stats_);
  CreativeDocumentCreateReceipt receipt = document_.createObject(request);
  if (!receipt.accepted || !receipt.objectCreated) {
    recordCommandFailure(stats_);
    return receipt;
  }

  recordCommandSuccess(stats_);
  recordObjectCreated(stats_);
  if (receipt.objectKind == CreativeObjectKind::Room) {
    recordRoomCreated(stats_);
  }
  return receipt;
}

CreativeDocumentCreateReceipt Facade::createDocumentObject(
    CreativeObjectKind kind) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  return createDocumentObject(request);
}

CreativeObjectId Facade::createRoom(const CreateRoomCommand& command) {
  recordCommandAttempt(stats_);

  const CreativeObjectId id = document_.createRoom(command.name,
                                                   command.transform,
                                                   command.bounds,
                                                   command.layerId,
                                                   command.visible,
                                                   command.locked,
                                                   command.tags,
                                                   command.parentId);
  if (id == kInvalidObjectId) {
    recordCommandFailure(stats_);
    return kInvalidObjectId;
  }

  recordCommandSuccess(stats_);
  recordObjectCreated(stats_);
  recordRoomCreated(stats_);
  return id;
}

CreativeObjectId Facade::createRoom(std::string name) {
  return createRoom(makeCreateRoomCommand(std::move(name)));
}

bool Facade::renameObject(const RenameObjectCommand& command) {
  recordCommandAttempt(stats_);
  const bool renamed = document_.renameObject(command.id, command.name);
  if (renamed) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return renamed;
}

bool Facade::renameObject(CreativeObjectId id, std::string nextName) {
  RenameObjectCommand command;
  command.id = id;
  command.name = std::move(nextName);
  return renameObject(command);
}

bool Facade::removeObject(const RemoveObjectCommand& command) {
  recordCommandAttempt(stats_);
  const bool removed = document_.removeObject(command.id);
  if (removed) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return removed;
}

bool Facade::removeObject(CreativeObjectId id) {
  RemoveObjectCommand command;
  command.id = id;
  return removeObject(command);
}

const CreativeObject* Facade::findObject(CreativeObjectId id) const noexcept {
  return document_.findObject(id);
}

const CreativeDocument& Facade::document() const noexcept {
  return document_;
}

const Stats& Facade::stats() const noexcept {
  return stats_;
}

}  // namespace iggy3d::creative
