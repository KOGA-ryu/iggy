#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

enum class ProductPauseSaveFlowKind : std::uint8_t {
  Save,
  SaveAndExit,
};

struct ProductPauseSaveFlowResult {
  ProductPauseSaveFlowKind kind = ProductPauseSaveFlowKind::Save;
  ProductSaveWriteResult write;
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

std::string_view productPauseSaveFlowKindName(ProductPauseSaveFlowKind kind);

}  // namespace iggy3d
