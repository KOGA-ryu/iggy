#include "EditorDesktopCommands.hpp"

#include "EditorDesktopCommandsInternal.hpp"
#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <cassert>
#include <cstddef>
#include <utility>
#include <variant>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].payload = std::monostate{};
  ++count;
}

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       std::string saveId) {
  push(id, CreativeDesktopCommandPayload{
               CreativeDesktopSaveAsPayload{std::move(saveId)}});
}

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       CreativeDesktopCommandPayload payload) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].payload = std::move(payload);
  ++count;
}

void CreativeDesktopCommandFrame::clear() noexcept {
  count = 0U;
  overflowed = false;
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

void mergeCommandResult(CreativeDesktopCommandResult& aggregate,
                        CreativeDesktopCommandResult commandResult) {
  const CreativeDesktopCommandImpactFlags cumulativeImpacts =
      aggregate.impacts | commandResult.impacts;
  aggregate = std::move(commandResult);
  aggregate.impacts = cumulativeImpacts;
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
    mergeCommandResult(result, std::move(commandResult));
  }
  return result;
}

}  // namespace iggy3d_creative_app
