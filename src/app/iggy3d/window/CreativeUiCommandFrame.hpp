#pragma once

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/window/CreativeUiInputFrame.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

enum class ProductCreativeUiCommandKind : std::uint8_t {
  None,
  CycleNextTool,
  ToggleSelectedObjectVisibility,
};

struct ProductCreativeUiCommandFrameRequest {
  creative::Facade* facade = nullptr;
  ProductCreativeUiInputFrameReceipt inputReceipt;
};

struct ProductCreativeUiCommandFrameReceipt {
  bool requested = false;
  bool facadeAvailable = false;
  bool inputConsumed = false;
  bool inputEnabled = false;
  bool accepted = false;
  bool changed = false;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  creative::Tool toolBefore = creative::Tool::Select;
  creative::Tool toolAfter = creative::Tool::Select;
  bool mutationRequested = false;
  bool mutationAccepted = false;
  bool mutationChanged = false;
  creative::CreativeFacadeMutationStatus mutationStatus =
      creative::CreativeFacadeMutationStatus::Unknown;
  creative::CreativeDocumentMutationStatus documentMutationStatus =
      creative::CreativeDocumentMutationStatus::Unknown;
  creative::CreativeMutationKind mutationKind =
      creative::CreativeMutationKind::Unknown;
  creative::TargetRef mutationTarget;
  creative::CreativeObjectId mutationObjectId = creative::kInvalidObjectId;
  creative::CreativeObjectKind mutationObjectKind =
      creative::CreativeObjectKind::Unknown;
  bool visibleBefore = false;
  bool visibleAfter = false;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string mutationMessage;
  std::string semanticId;
  std::string status = "product_creative_ui_command_not_requested";
  std::string reasonCode = "product_creative_ui_command_not_requested";
};

[[nodiscard]] ProductCreativeUiCommandFrameReceipt
routeProductCreativeUiCommandFrame(
    const ProductCreativeUiCommandFrameRequest& request);

}  // namespace iggy3d
