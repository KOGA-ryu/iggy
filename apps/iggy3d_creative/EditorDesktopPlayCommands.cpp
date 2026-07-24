#include "EditorDesktopCommandsInternal.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include "EditorPlaytestProcess.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlaytestLaunch.hpp"
#include "EditorPlaytestNames.hpp"
#include "app/iggy3d/creative/play/PlayPreparation.hpp"
#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"

#include <string>
#include <string_view>

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
    case CreativeDesktopCommandId::Play: {
      // Play always targets the latest authored state. Validation happens
      // before replacing a running child so an invalid edit cannot kill the
      // last usable playtest.
      finalizeCreativeEditorContinuousGestures(
          context.appState, context.editor,
          "creative_continuous_gesture_playtest_launch");
      iggy3d::creative::CreativePlayPreparationRequest validationRequest;
      validationRequest.document = &context.appState.facade.document();
      validationRequest.staticMeshAssetCatalog =
          context.staticMeshAssetCatalog;
      const iggy3d::creative::CreativePlayPreparationResult validation =
          iggy3d::creative::prepareCreativePlay(validationRequest);
      if (!validation.accepted) {
        result.message = std::string("playtest refused: playtest_refused_") +
                         std::string(toString(validation.status));
        return true;
      }

      if (context.playtestControl != nullptr &&
          decidePlaytestLaunchAction(context.playtestControl->running()) ==
              PlaytestLaunchAction::ReplaceRunning) {
        context.playtestControl->stopRunning();
      }

      const char* basePath = SDL_GetBasePath();
      const std::filesystem::path appBasePath =
          basePath == nullptr ? std::filesystem::path{}
                              : std::filesystem::path{basePath};
      PlaytestLaunchPreparation preparation = preparePlaytestLaunch(
          context.appState.facade.document(), context.staticMeshAssetCatalog,
          context.saveRoot, appBasePath);
      if (preparation.accepted) {
        preparation.plan = buildPlaytestLaunchPlan(
            appBasePath,
            context.saveRoot / std::string(kPlaytestSnapshotDirName),
            std::string(kPlaytestSnapshotSaveId),
            &context.editor.playtestWindowPreferences);
      }
      if (!preparation.accepted) {
        result.message = "playtest refused: " + preparation.reasonCode;
        return true;
      }

      std::string spawnReason;
      const bool spawned =
          context.playtestControl != nullptr
              ? context.playtestControl->launch(preparation.plan, spawnReason)
              : spawnPlaytestProcess(preparation.plan, spawnReason);
      if (spawned && context.playtestControl != nullptr &&
          validation.payload.has_value()) {
        context.playtestControl->setSnapshotEntityNames(buildPlaytestNameMap(
            *validation.payload, context.appState.facade.document()));
      }
      result.accepted = spawned;
      result.message = spawned ? "playtest launched"
                               : "playtest launch failed: " + spawnReason;
      return true;
    }
    case CreativeDesktopCommandId::None:
    case CreativeDesktopCommandId::Count:
    default:
      return false;
  }
}

}  // namespace iggy3d_creative_app
