#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

namespace creative {
struct CreativeAppState;
}  // namespace creative

enum class ProductPauseSaveFlowKind : std::uint8_t {
  Save,
  SaveAndExit,
};

struct ProductPauseSaveFlowResult {
  ProductPauseSaveFlowKind kind = ProductPauseSaveFlowKind::Save;
  ProductSaveWriteResult write;
  bool creativeSaveRequested = false;
  bool creativeSaveAccepted = false;
  bool creativeSaveSaved = false;
  ProductCreativeCurrentWorldSaveResult creativeSave;
  std::string frontendStatus;
  std::string launchStatus;
  bool returnedToTitle = false;
  bool sessionReset = false;
};

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window);

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState* creativeApp);

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    FrontendSettings& settings);

ProductPauseSaveFlowResult executeProductPauseSaveFlow(
    ProductPauseSaveFlowKind kind,
    const ProductAppOptions& options,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    FrontendSettings& settings,
    creative::CreativeAppState* creativeApp);

}  // namespace iggy3d
