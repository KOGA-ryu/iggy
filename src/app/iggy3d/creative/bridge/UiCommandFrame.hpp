#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/UiCommandCatalog.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

struct ProductCreativeUiCommandFrameRequest {
  creative::CreativeAppState* creative = nullptr;
  ProductCreativeUiInputFrameReceipt inputReceipt;
};

struct ProductCreativeUiCommandCreateOutcome {
  creative::CreativeDocumentCreateReceipt document;
  bool placementOffsetApplied = false;
  double placementOffsetX = 0.0;
};

struct ProductCreativeUiCommandRemoveOutcome {
  creative::CreativeDocumentRemoveReceipt document;
  bool noSelection = false;
};

struct ProductCreativeUiCommandFrameReceipt {
  bool requested = false;
  bool facadeAvailable = false;
  bool inputClickPresent = false;
  bool inputConsumed = false;
  bool inputEnabled = false;
  bool accepted = false;
  bool changed = false;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  creative::Tool commandTool = creative::Tool::Select;
  creative::CreativeObjectKind commandObjectKind =
      creative::CreativeObjectKind::Unknown;
  creative::Tool toolBefore = creative::Tool::Select;
  creative::Tool toolAfter = creative::Tool::Select;
  creative::CreativeFacadeMutationReceipt mutation;
  ProductCreativeUiCommandCreateOutcome create;
  ProductCreativeUiCommandRemoveOutcome remove;
  creative::CreativeDocumentUndoApplyReceipt undo;
  bool shellRequested = false;
  bool shellAccepted = false;
  bool shellChanged = false;
  creative::CreativeObjectId shellRoomObjectId = creative::kInvalidObjectId;
  std::uint64_t shellGeneratedObjectCount = 0;
  std::uint64_t shellRemovedObjectCount = 0;
  std::uint64_t shellFloorCount = 0;
  std::uint64_t shellWallCount = 0;
  std::uint64_t shellRevisionBefore = 0;
  std::uint64_t shellRevisionAfter = 0;
  std::string shellStatus = "creative_room_shell_not_requested";
  std::string shellReasonCode = "creative_room_shell_not_requested";
  std::string shellMessage = "creative_room_shell_not_requested";
  std::string semanticId;
  std::string status = "product_creative_ui_command_not_requested";
  std::string reasonCode = "product_creative_ui_command_not_requested";
};

[[nodiscard]] ProductCreativeUiCommandFrameReceipt
routeProductCreativeUiCommandFrame(
    const ProductCreativeUiCommandFrameRequest& request);

[[nodiscard]] bool productCreativeUiCommandKindHasHandler(
    ProductCreativeUiCommandKind commandKind) noexcept;

}  // namespace iggy3d
