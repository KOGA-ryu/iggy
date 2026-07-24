#include "EditorDesktopCommandsInternal.hpp"

#include "EditorPlaytestProcess.hpp"
#include "app/iggy3d/creative/play/PlaySession.hpp"
#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {

bool dispatchCreativeDesktopPlayCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  switch (command.id) {
    case CreativeDesktopCommandId::PlaytestPause:
    case CreativeDesktopCommandId::PlaytestResume: {
      // Pure decision + owner executes: the hang guard inside the owner is
      // the gatekeeper (no child / stalled -> rejected, nothing queued).
      if (context.playtestControl == nullptr) {
        result.message = "playtest command rejected: no child";
        return true;
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
      return true;
    }
    case CreativeDesktopCommandId::Play:
      if (context.playMode == nullptr) {
        result.message = "play owner is unavailable";
        return true;
      }
      if (creativePlaySessionActive(*context.playMode)) {
        const iggy3d::creative::CreativeRuntimeSandboxStopReceipt stopped =
            stopCreativePlaySession(*context.playMode);
        result.accepted = stopped.stopped;
        result.changed = stopped.stopped;
        result.message = stopped.stopped ? "play stopped"
                                         : "play stop failed";
        return true;
      }
      if (context.editor.terrainGeneration.previewActive) {
        result.message = "apply or cancel the terrain preview before play";
        return true;
      }
      {
        CreativePlayStartRequest request;
        request.document = &context.appState.facade.document();
        request.staticMeshAssetCatalog = context.staticMeshAssetCatalog;
        const CreativePlayStartReceipt started =
            startCreativePlaySession(*context.playMode, std::move(request));
        result.accepted = started.accepted;
        result.changed = started.accepted;
        result.message = started.accepted
                             ? "play started"
                             : "play failed: " + started.reasonCode;
      }
      return true;
    case CreativeDesktopCommandId::None:
    case CreativeDesktopCommandId::Count:
    default:
      return false;
  }
}

}  // namespace iggy3d_creative_app
