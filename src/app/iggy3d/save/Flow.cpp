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
  bool returnToTitleOnSuccess;
};

using TransitionApply = void (*)(FrontendState&, ProductAppWindowState&);
using SessionApply = void (*)(std::optional<Session>&);

void keepFrontend(FrontendState&, ProductAppWindowState&) {}

void returnFrontendToTitle(FrontendState& frontend, ProductAppWindowState& window) {
  returnProductToTitleTransition(frontend, window);
}

void keepSession(std::optional<Session>&) {}

void resetSession(std::optional<Session>& activeSession) {
  activeSession.reset();
}

}  // namespace

std::string_view productPauseSaveFlowKindName(ProductPauseSaveFlowKind kind) {
  static constexpr std::array names{
      std::string_view{"pause_save"},
      std::string_view{"pause_save_and_exit"},
  };
  return names[static_cast<std::size_t>(kind)];
}

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window) {
  static constexpr std::array configs{
      PauseSaveFlowConfig{
          ProductPauseSaveFlowKind::Save,
          "pause_save",
          "pause_save_written",
          "pause_save_failed",
          "pause_save_written",
          false,
      },
      PauseSaveFlowConfig{
          ProductPauseSaveFlowKind::SaveAndExit,
          "pause_save_and_exit",
          "pause_save_and_exit_written",
          "pause_save_and_exit_failed",
          "pause_save_and_exit_written",
          true,
      },
  };
  static constexpr std::array<TransitionApply, 2U> transitionAppliers{
      keepFrontend,
      returnFrontendToTitle,
  };
  static constexpr std::array<SessionApply, 2U> sessionAppliers{
      keepSession,
      resetSession,
  };

  const PauseSaveFlowConfig& config = configs[static_cast<std::size_t>(kind)];
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

  frontend.status = result.frontendStatus;
  window.launchStatus = result.launchStatus;
  transitionAppliers[result.returnedToTitle](frontend, window);
  sessionAppliers[result.sessionReset](activeSession);
  return result;
}

}  // namespace iggy3d
