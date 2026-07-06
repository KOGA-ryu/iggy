#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

enum class ProductCreativeUiCommandKind : std::uint8_t {
  None,
  ToggleSelectedObjectVisibility,
  ToggleSelectedObjectLocked,
  SetActiveTool,
  CreateObject,
  RebuildRoom,
  UndoLastDocumentChange,
  DeleteSelectedObject,
};

struct ProductCreativeUiCommandFrameRequest {
  creative::CreativeAppState* creative = nullptr;
  ProductCreativeUiInputFrameReceipt inputReceipt;
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
  bool lockedBefore = false;
  bool lockedAfter = false;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string mutationMessage;
  bool createRequested = false;
  bool createAccepted = false;
  bool createChanged = false;
  creative::CreativeDocumentCreateStatus createStatus =
      creative::CreativeDocumentCreateStatus::Unknown;
  creative::CreativeObjectId createObjectId = creative::kInvalidObjectId;
  creative::CreativeObjectKind createObjectKind =
      creative::CreativeObjectKind::Unknown;
  std::string createObjectName;
  std::uint64_t createRevisionBefore = 0;
  std::uint64_t createRevisionAfter = 0;
  creative::CreativeObjectDirtyFlags createDirtyFlags = 0;
  std::string createMessage;
  std::string createReasonCode;
  bool deleteRequested = false;
  bool deleteAccepted = false;
  bool deleteChanged = false;
  bool deleteRemoved = false;
  creative::CreativeObjectId deleteObjectId = creative::kInvalidObjectId;
  creative::CreativeObjectKind deleteObjectKind =
      creative::CreativeObjectKind::Unknown;
  std::string deleteObjectName;
  std::uint64_t deleteRevisionBefore = 0;
  std::uint64_t deleteRevisionAfter = 0;
  creative::CreativeObjectDirtyFlags deleteDirtyFlags = 0;
  std::string deleteStatus = "Unknown";
  std::string deleteMessage;
  std::string deleteReasonCode;
  bool undoRequested = false;
  bool undoAccepted = false;
  bool undoChanged = false;
  bool undoHadSnapshot = false;
  creative::CreativeDocumentId undoDocumentId = creative::kInvalidDocumentId;
  std::uint64_t undoRevisionBefore = 0;
  std::uint64_t undoRevisionAfter = 0;
  std::uint64_t undoObjectCountBefore = 0;
  std::uint64_t undoObjectCountAfter = 0;
  std::uint64_t undoDepthBefore = 0;
  std::uint64_t undoDepthAfter = 0;
  std::string undoStatus = "creative_undo_not_requested";
  std::string undoReasonCode = "creative_undo_not_requested";
  std::string undoMessage = "creative_undo_not_requested";
  std::string semanticId;
  std::string status = "product_creative_ui_command_not_requested";
  std::string reasonCode = "product_creative_ui_command_not_requested";
};

[[nodiscard]] ProductCreativeUiCommandFrameReceipt
routeProductCreativeUiCommandFrame(
    const ProductCreativeUiCommandFrameRequest& request);

}  // namespace iggy3d
