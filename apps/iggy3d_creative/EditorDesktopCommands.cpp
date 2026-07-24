#include "EditorDesktopCommands.hpp"

#include "EditorDesktopCommandsInternal.hpp"
#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string_view>
#include <utility>
#include <variant>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

CreativeDesktopCommandEnqueueResult CreativeDesktopCommandFrame::push(
    CreativeDesktopCommandId id) {
  return push(id, CreativeDesktopCommandPayload{std::monostate{}});
}

CreativeDesktopCommandEnqueueResult CreativeDesktopCommandFrame::push(
    CreativeDesktopCommandId id, std::string saveId) {
  return push(id, CreativeDesktopCommandPayload{
                      CreativeDesktopSaveAsPayload{std::move(saveId)}});
}

CreativeDesktopCommandEnqueueResult CreativeDesktopCommandFrame::push(
    CreativeDesktopCommandId id, CreativeDesktopCommandPayload payload) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    if (rejectedCommandCount == 0U) {
      firstRejectedCommand = id;
    }
    ++rejectedCommandCount;
    return CreativeDesktopCommandEnqueueResult::CapacityExceeded;
  }
  commands[count].id = id;
  commands[count].payload = std::move(payload);
  ++count;
  return CreativeDesktopCommandEnqueueResult::Enqueued;
}

void CreativeDesktopCommandFrame::clear() noexcept {
  count = 0U;
  overflowed = false;
  rejectedCommandCount = 0U;
  firstRejectedCommand = CreativeDesktopCommandId::None;
}

namespace {

void observeCreativeDesktopEnqueue(
    const CreativeDesktopCommandFrame& frame,
    CreativeDesktopCommandEnqueueResult enqueueResult) {
  switch (enqueueResult) {
    case CreativeDesktopCommandEnqueueResult::Enqueued:
      return;
    case CreativeDesktopCommandEnqueueResult::CapacityExceeded:
      if (!frame.overflowed || frame.rejectedCommandCount == 0U) {
        assert(false && "desktop command refusal was not recorded");
      }
      return;
  }
}

}  // namespace

void CreativeDesktopCommandFrame::enqueue(CreativeDesktopCommandId id) {
  observeCreativeDesktopEnqueue(*this, push(id));
}

void CreativeDesktopCommandFrame::enqueue(CreativeDesktopCommandId id,
                                          std::string saveId) {
  observeCreativeDesktopEnqueue(*this, push(id, std::move(saveId)));
}

void CreativeDesktopCommandFrame::enqueue(
    CreativeDesktopCommandId id, CreativeDesktopCommandPayload payload) {
  observeCreativeDesktopEnqueue(*this, push(id, std::move(payload)));
}

namespace {

struct CreativeDesktopDocumentKey {
  creative::CreativeDocumentId id = creative::kInvalidDocumentId;
  std::uint64_t revision = 0U;

  [[nodiscard]] friend bool operator==(
      const CreativeDesktopDocumentKey&,
      const CreativeDesktopDocumentKey&) noexcept = default;
};

struct CreativeDesktopDocumentSnapshot {
  CreativeDesktopDocumentKey root;
  CreativeDesktopDocumentKey active;
  bool assetEditActive = false;
};

[[nodiscard]] CreativeDesktopDocumentKey creativeDesktopDocumentKey(
    const creative::CreativeDocument& document) noexcept {
  return {document.id(), document.revision()};
}

[[nodiscard]] CreativeDesktopDocumentSnapshot creativeDesktopDocumentSnapshot(
    const CreativeDesktopCommandContext& context) noexcept {
  const creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(context.editor, context.appState);
  return {
      creativeDesktopDocumentKey(context.appState.facade.document()),
      creativeDesktopDocumentKey(activeAppState.facade.document()),
      context.editor.assetEdit.active,
  };
}

void addCommandImpact(CreativeDesktopCommandResult& result,
                      CreativeDesktopCommandImpact impact) noexcept {
  result.impacts |= creativeDesktopCommandImpactFlag(impact);
}

void resolveCommandImpacts(
    CreativeDesktopCommandResult& result,
    const CreativeDesktopDocumentSnapshot& before,
    const CreativeDesktopDocumentSnapshot& after) noexcept {
  const bool documentChanged =
      before.root != after.root || before.active != after.active ||
      before.assetEditActive != after.assetEditActive;
  if (documentChanged) {
    addCommandImpact(result, CreativeDesktopCommandImpact::DocumentChanged);
  }
  if (result.documentReplaced || before.root.id != after.root.id) {
    addCommandImpact(result, CreativeDesktopCommandImpact::DocumentReplaced);
  }
  if (result.sceneChanged) {
    addCommandImpact(result, CreativeDesktopCommandImpact::SceneChanged);
  }
  if (result.worldLayoutChanged) {
    addCommandImpact(result, CreativeDesktopCommandImpact::WorldLayoutChanged);
  }
}

void appendCommandReceipt(
    CreativeDesktopCommandResult& aggregate,
    const CreativeDesktopCommandResult& commandResult) noexcept {
  if (aggregate.commandReceiptCount >= kCreativeDesktopCommandCapacity) {
    assert(false && "desktop command receipt capacity exceeded");
    return;
  }
  CreativeDesktopCommandDispatchReceipt& receipt =
      aggregate.commandReceipts[aggregate.commandReceiptCount++];
  receipt.command = commandResult.lastCommand;
  receipt.owner = creativeDesktopCommandOwner(commandResult.lastCommand);
  receipt.accepted = commandResult.accepted;
  receipt.changed = commandResult.changed;
  receipt.impacts = commandResult.impacts;
  receipt.affectedObjectCount = commandResult.affectedObjectCount;
  const std::size_t copiedSize =
      std::min(commandResult.message.size(),
               kCreativeDesktopCommandReceiptMessageCapacity);
  std::copy_n(commandResult.message.data(), copiedSize, receipt.message.data());
  receipt.messageLength = static_cast<std::uint16_t>(copiedSize);
  receipt.messageTruncated = copiedSize < commandResult.message.size();
}

void mergeCommandResult(CreativeDesktopCommandResult& aggregate,
                        CreativeDesktopCommandResult commandResult) {
  const CreativeDesktopCommandImpactFlags cumulativeImpacts =
      aggregate.impacts | commandResult.impacts;
  aggregate.lastCommand = commandResult.lastCommand;
  aggregate.objectAction = std::move(commandResult.objectAction);
  aggregate.accepted = commandResult.accepted;
  aggregate.changed = commandResult.changed;
  aggregate.impacts = cumulativeImpacts;
  aggregate.affectedObjectCount = commandResult.affectedObjectCount;
  aggregate.message = std::move(commandResult.message);
  aggregate.documentReplaced = creativeDesktopCommandHasImpact(
      aggregate, CreativeDesktopCommandImpact::DocumentReplaced);
  aggregate.sceneChanged = creativeDesktopCommandHasImpact(
      aggregate, CreativeDesktopCommandImpact::SceneChanged);
  aggregate.worldLayoutChanged = creativeDesktopCommandHasImpact(
      aggregate, CreativeDesktopCommandImpact::WorldLayoutChanged);
}

void dispatchOne(const CreativeDesktopCommand& command,
                 const CreativeDesktopCommandContext& context,
                 CreativeDesktopCommandResult& result) {
  result.lastCommand = command.id;
  result.objectAction = {};
  result.accepted = false;
  result.changed = false;
  result.impacts = 0U;
  result.documentReplaced = false;
  result.sceneChanged = false;
  result.worldLayoutChanged = false;
  result.affectedObjectCount = 0U;
  result.message.clear();

  bool handled = false;
  switch (creativeDesktopCommandOwner(command.id)) {
    case CreativeDesktopCommandOwner::None:
    case CreativeDesktopCommandOwner::Invalid:
      return;
    case CreativeDesktopCommandOwner::Document:
      handled =
          dispatchCreativeDesktopDocumentCommand(command, context, result);
      break;
    case CreativeDesktopCommandOwner::Object:
      handled = dispatchCreativeDesktopObjectCommand(command, context, result);
      break;
    case CreativeDesktopCommandOwner::Terrain:
      handled = dispatchCreativeDesktopTerrainCommand(command, context, result);
      break;
    case CreativeDesktopCommandOwner::Play:
      handled = dispatchCreativeDesktopPlayCommand(command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutSource:
      handled = dispatchCreativeDesktopWorldLayoutSourceCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutProperty:
      handled = dispatchCreativeDesktopWorldLayoutPropertyCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutLevel:
      handled = dispatchCreativeDesktopWorldLayoutLevelCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutBuilding:
      handled = dispatchCreativeDesktopWorldLayoutBuildingCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutPlan:
      handled = dispatchCreativeDesktopWorldLayoutPlanCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutElement:
      handled = dispatchCreativeDesktopWorldLayoutElementCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutWallOpening:
      handled = dispatchCreativeDesktopWorldLayoutWallOpeningCommand(
          command, context, result);
      break;
    case CreativeDesktopCommandOwner::WorldLayoutLifecycle:
      handled = dispatchCreativeDesktopWorldLayoutLifecycleCommand(
          command, context, result);
      break;
  }
  if (!handled) {
    assert(false && "desktop command owner rejected its mapped command");
  }
}

}  // namespace

CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context) {
  CreativeDesktopCommandResult result;
  const std::size_t count =
      frame.count < kCreativeDesktopCommandCapacity ? frame.count
                                                    : kCreativeDesktopCommandCapacity;
  for (std::size_t index = 0U; index < count; ++index) {
    const CreativeDesktopDocumentSnapshot before =
        creativeDesktopDocumentSnapshot(context);
    CreativeDesktopCommandResult commandResult;
    dispatchOne(frame.commands[index], context, commandResult);
    const CreativeDesktopDocumentSnapshot after =
        creativeDesktopDocumentSnapshot(context);
    resolveCommandImpacts(commandResult, before, after);
    appendCommandReceipt(result, commandResult);
    mergeCommandResult(result, std::move(commandResult));
  }
  result.overflowed = frame.overflowed;
  result.rejectedCommandCount = frame.rejectedCommandCount;
  result.firstRejectedCommand = frame.firstRejectedCommand;
  if (result.overflowed) {
    result.accepted = false;
    result.message = "desktop command frame capacity exceeded";
  }
  return result;
}

}  // namespace iggy3d_creative_app
