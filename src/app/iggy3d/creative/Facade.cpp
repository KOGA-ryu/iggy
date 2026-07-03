#include "app/iggy3d/creative/Facade.hpp"

#include <utility>

namespace iggy3d::creative {

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
  const bool changed = iggy3d::creative::setActiveTool(toolState_, tool);
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
  return buildCreativeUiModel(request);
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
