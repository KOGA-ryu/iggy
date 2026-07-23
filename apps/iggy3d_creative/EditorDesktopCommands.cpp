#include "EditorDesktopCommands.hpp"

#include "EditorDesktopCommandsInternal.hpp"
#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <variant>

#include "app/iggy3d/creative/play/PlaySession.hpp"
#include "EditorPlaytestProcess.hpp"
#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"

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
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  result.lastCommand = command.id;
  result.accepted = false;
  result.changed = false;
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
      dispatchCreativeDesktopMeasurementCommand(command, context, result) ||
      dispatchCreativeDesktopObjectCommand(command, context, result) ||
      dispatchCreativeDesktopAssetCommand(command, context, result) ||
      dispatchCreativeDesktopTerrainCommand(command, context, result) ||
      dispatchCreativeDesktopWorldLayoutSourceCommand(command, context,
                                                       result) ||
      dispatchCreativeDesktopWorldLayoutBuildingCommand(command, context,
                                                         result) ||
      dispatchCreativeDesktopWorldLayoutStructureCommand(command, context,
                                                          result) ||
      dispatchCreativeDesktopWorldLayoutLifecycleCommand(command, context,
                                                          result)) {
    return;
  }

  switch (command.id) {
    case CreativeDesktopCommandId::PlaytestPause:
    case CreativeDesktopCommandId::PlaytestResume: {
      // Pure decision + owner executes: the hang guard inside the owner is
      // the gatekeeper (no child / stalled -> rejected, nothing queued).
      if (context.playtestControl == nullptr) {
        result.message = "playtest command rejected: no child";
        break;
      }
      const std::string_view verb =
          command.id == CreativeDesktopCommandId::PlaytestPause
              ? kPlaytestCommandVerbPause
              : kPlaytestCommandVerbResume;
      std::string sendReason;
      const bool sent = context.playtestControl->sendPlaytestCommand(
          verb, {}, sendReason);
      result.accepted = sent;
      result.message = sent ? std::string(verb) + " sent"
                            : "playtest command rejected: " + sendReason;
      break;
    }
    case CreativeDesktopCommandId::Play:
      if (context.playMode == nullptr) {
        result.message = "play owner is unavailable";
        break;
      }
      if (creativePlaySessionActive(*context.playMode)) {
        const creative::CreativeRuntimeSandboxStopReceipt stopped =
            stopCreativePlaySession(*context.playMode);
        result.accepted = stopped.stopped;
        result.changed = stopped.stopped;
        result.message = stopped.stopped ? "play stopped"
                                         : "play stop failed";
        break;
      }
      if (editor.terrainGeneration.previewActive) {
        result.message = "apply or cancel the terrain preview before play";
        break;
      }
      {
        CreativePlayStartRequest request;
        request.document = &appState.facade.document();
        request.staticMeshAssetCatalog = context.staticMeshAssetCatalog;
        const CreativePlayStartReceipt started =
            startCreativePlaySession(*context.playMode, std::move(request));
        result.accepted = started.accepted;
        result.changed = started.accepted;
        result.message = started.accepted
                             ? "play started"
                             : "play failed: " + started.reasonCode;
      }
      break;
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
    dispatchOne(frame.commands[index], context, result);
  }
  return result;
}

}  // namespace iggy3d_creative_app
