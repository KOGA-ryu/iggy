#include "EditorDesktopCommands.hpp"

#include "EditorDesktopCommandsInternal.hpp"
#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <cstddef>
#include <utility>
#include <variant>

#include "app/iggy3d/creative/play/PlaySession.hpp"

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

[[nodiscard]] bool commandAllowedDuringPlay(
    CreativeDesktopCommandId id) noexcept {
  return id == CreativeDesktopCommandId::None ||
         id == CreativeDesktopCommandId::Play ||
         id == CreativeDesktopCommandId::SelectObjects ||
         id == CreativeDesktopCommandId::ClearSelection ||
         id == CreativeDesktopCommandId::WorldLayoutFocusSource ||
         id == CreativeDesktopCommandId::WorldLayoutFocusObjectSource;
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

  if (context.playMode != nullptr &&
      creativePlaySessionActive(*context.playMode) &&
      !commandAllowedDuringPlay(command.id)) {
    result.message = "stop play before editing";
    return;
  }

  if (dispatchCreativeDesktopDocumentCommand(command, context, result) ||
      dispatchCreativeDesktopObjectCommand(command, context, result) ||
      dispatchCreativeDesktopAssetCommand(command, context, result) ||
      dispatchCreativeDesktopTerrainCommand(command, context, result) ||
      dispatchCreativeDesktopWorldLayoutSourceCommand(command, context,
                                                       result) ||
      dispatchCreativeDesktopWorldLayoutPropertyCommand(command, context,
                                                         result) ||
      dispatchCreativeDesktopWorldLayoutLevelCommand(command, context,
                                                      result) ||
      dispatchCreativeDesktopWorldLayoutBuildingCommand(command, context,
                                                         result) ||
      dispatchCreativeDesktopWorldLayoutPlanCommand(command, context,
                                                     result) ||
      dispatchCreativeDesktopWorldLayoutElementCommand(command, context,
                                                        result) ||
      dispatchCreativeDesktopWorldLayoutWallOpeningCommand(command, context,
                                                            result) ||
      dispatchCreativeDesktopWorldLayoutLifecycleCommand(command, context,
                                                          result) ||
      dispatchCreativeDesktopPlayCommand(command, context, result)) {
    return;
  }

  switch (command.id) {
    case CreativeDesktopCommandId::None:
    case CreativeDesktopCommandId::Count:
    default:
      break;
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
