#include "app/iggy3d/save/Flow.hpp"

#include <array>

#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/menu/Transitions.hpp"

namespace iggy3d {
namespace {

struct PauseSaveFlowConfig {
  ProductPauseSaveFlowKind kind;
  std::string_view source;
  std::string_view successFrontendStatus;
  std::string_view failureFrontendStatus;
  std::string_view successLaunchStatus;
  std::string_view creativeSuccessFrontendStatus;
  std::string_view creativeFailureFrontendStatus;
  bool returnToTitleOnSuccess;
};

using SessionApply = void (*)(std::optional<Session>&);

void keepSession(std::optional<Session>&) {}

void resetSession(std::optional<Session>& activeSession) {
  activeSession.reset();
}

constexpr std::array<SessionApply, 2U> kPauseSaveSessionAppliers{
    keepSession,
    resetSession,
};

constexpr std::array kPauseSaveFlowConfigs{
    PauseSaveFlowConfig{
        ProductPauseSaveFlowKind::Save,
        "pause_save",
        "pause_save_written",
        "pause_save_failed",
        "pause_save_written",
        "pause_creative_save_written",
        "pause_creative_save_failed",
        false,
    },
    PauseSaveFlowConfig{
        ProductPauseSaveFlowKind::SaveAndExit,
        "pause_save_and_exit",
        "pause_save_and_exit_written",
        "pause_save_and_exit_failed",
        "pause_save_and_exit_written",
        "pause_creative_save_and_exit_written",
        "pause_creative_save_and_exit_failed",
        true,
    },
};

const PauseSaveFlowConfig& pauseSaveFlowConfigFor(
    ProductPauseSaveFlowKind kind) {
  return kPauseSaveFlowConfigs[static_cast<std::size_t>(kind)];
}

void clearPauseFlowActiveCreativeIdentity(ProductAppWindowState& window) {
  window.activeCreativeSaveId = "none";
  window.activeCreativeSavePath = "none";
  window.activeCreativeWorldId = "none";
  window.activeCreativeDocumentId = creative::kInvalidDocumentId;
  window.activeCreativeObjectCount = 0;
  window.activeCreativeNextObjectId = creative::kInvalidObjectId;
  window.activeCreativeSaveStatus = "creative_world_save_not_requested";
  window.activeCreativeSaveReasonCode = "creative_world_save_not_requested";
  window.activeCreativeSaveDirtyFlagsBefore = 0;
  window.activeCreativeSaveDirtyFlagsDrained = 0;
  window.activeCreativeSaveDirtyFlagsAfter = 0;
  window.activeCreativeSaveSavedAtUtc = "none";
}

void recordPauseCreativeFacadeMissing(ProductPauseSaveFlowResult& result,
                                      ProductAppWindowState& window) {
  constexpr std::string_view reason = "product_creative_save_facade_missing";
  result.creativeSaveRequested = true;
  result.creativeSave.status = std::string{reason};
  result.creativeSave.reasonCode = std::string{reason};
  result.creativeSave.saveId = window.activeCreativeSaveId;
  result.creativeSave.worldId = window.activeCreativeWorldId;
  result.creativeSave.documentId = window.activeCreativeDocumentId;
  result.creativeSave.objectCount = window.activeCreativeObjectCount;
  result.creativeSave.nextObjectId = window.activeCreativeNextObjectId;
  result.launchStatus = std::string{reason};
  window.launchStatus = std::string{reason};
  window.activeCreativeSaveStatus = std::string{reason};
  window.activeCreativeSaveReasonCode = std::string{reason};
}

ProductPauseSaveFlowResult executeCreativePauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    FrontendSettings* settings,
    creative::Facade* creativeFacade) {
  const PauseSaveFlowConfig& config = pauseSaveFlowConfigFor(kind);
  ProductPauseSaveFlowResult result;
  result.kind = kind;
  result.creativeSaveRequested = true;

  if (creativeFacade == nullptr) {
    recordPauseCreativeFacadeMissing(result, window);
  } else {
    result.creativeSave = saveProductCurrentCreativeWorld(
        options, *creativeFacade, config.source, window);
    result.creativeSaveAccepted = result.creativeSave.accepted;
    result.creativeSaveSaved = result.creativeSave.saved;
    result.launchStatus = result.creativeSave.reasonCode;
    window.launchStatus = result.launchStatus;
  }

  const bool ok = result.creativeSaveAccepted && result.creativeSaveSaved;
  const std::string_view frontendStatus =
      ok ? config.creativeSuccessFrontendStatus
         : config.creativeFailureFrontendStatus;
  result.frontendStatus = std::string{frontendStatus};
  result.returnedToTitle = ok && config.returnToTitleOnSuccess;
  result.sessionReset = result.returnedToTitle;
  frontend.status = frontendStatus;

  if (result.returnedToTitle) {  // branch-gate: BG-1017
    if (settings == nullptr) {
      returnProductToTitleTransition(frontend, window);
    } else {
      returnProductToTitleTransition(frontend, window, *settings);
    }
    clearPauseFlowActiveCreativeIdentity(window);
  }
  kPauseSaveSessionAppliers[result.sessionReset](activeSession);
  return result;
}

}  // namespace

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window) {
  const PauseSaveFlowConfig& config = pauseSaveFlowConfigFor(kind);
  ProductPauseSaveFlowResult result;
  result.kind = kind;
  result.write =
      writeProductCurrentSessionSave(options, activeSession, config.source, window);

  const std::array<std::string_view, 2U> frontendStatuses{
      config.failureFrontendStatus,
      config.successFrontendStatus,
  };
  const std::array<std::string, 2U> launchStatuses{
      result.write.reasonCode,
      std::string{config.successLaunchStatus},
  };
  result.frontendStatus = std::string{frontendStatuses[result.write.ok]};
  result.launchStatus = launchStatuses[result.write.ok];
  result.returnedToTitle = result.write.ok && config.returnToTitleOnSuccess;
  result.sessionReset = result.returnedToTitle;

  frontend.status = frontendStatuses[result.write.ok];
  window.launchStatus = result.launchStatus;
  if (result.returnedToTitle) {  // branch-gate: BG-1017
    returnProductToTitleTransition(frontend, window);
  }
  kPauseSaveSessionAppliers[result.sessionReset](activeSession);
  return result;
}

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::Facade* creativeFacade) {
  if (window.interactionMode == ProductInteractionMode::Creative) {
    return executeCreativePauseSaveFlow(kind,
                                        options,
                                        frontend,
                                        activeSession,
                                        window,
                                        nullptr,
                                        creativeFacade);
  }
  return executeProductPauseSaveFlow(
      kind, options, frontend, activeSession, window);
}

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    FrontendSettings& settings) {
  ProductPauseSaveFlowResult result =
      executeProductPauseSaveFlow(kind, options, frontend, activeSession, window);
  if (result.returnedToTitle) {  // branch-gate: BG-1017
    clearProductGameplayOnlyModes(window, settings);
  }
  return result;
}

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    FrontendSettings& settings,
    creative::Facade* creativeFacade) {
  if (window.interactionMode == ProductInteractionMode::Creative) {
    return executeCreativePauseSaveFlow(kind,
                                        options,
                                        frontend,
                                        activeSession,
                                        window,
                                        &settings,
                                        creativeFacade);
  }

  ProductPauseSaveFlowResult result =
      executeProductPauseSaveFlow(kind, options, frontend, activeSession, window);
  if (result.returnedToTitle) {  // branch-gate: BG-1017
    clearProductGameplayOnlyModes(window, settings);
  }
  return result;
}

}  // namespace iggy3d
