#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace iggy3d {



// Owned selected-product-save state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSelectedProductSaveState {
  std::string id = "none";
  bool enabled = false;
  std::string status = "none";
};

// Owned save-flow state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSaveFlowState {
  std::string operation = "none";
  std::string sourceSurface = "none";
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string affectedSlotId = "none";
  std::uint64_t activeCountBefore = 0;
  std::uint64_t activeCountAfter = 0;
  std::uint64_t deletedCountAfter = 0;
  std::string selectedSlotAfter = "none";
};

// Owned save-delete state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSaveDeleteState {
  bool confirmationOpen = false;
  std::string candidateId = "none";
  bool candidateEnabled = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string type = "none";
  bool recoverable = false;
  bool executed = false;
};

// Owned save-recover state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSaveRecoverState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  bool executed = false;
  std::string saveId = "none";
  bool snapshotRecovered = false;
  bool snapshotMissing = false;
};
struct SaveSessionStore {
  std::string productSaveStatus = "not_requested";
  std::string productSaveReasonCode = "not_requested";
  std::string productSaveDurableReason = "not_requested";
  std::string productSaveSource = "none";
  std::string productSaveSaveId = "none";
  bool productSaveSessionSaved = false;
  std::string activeProductSaveId = "none";
  ProductSaveLoadResult productSaveLoadResult;
  std::string productSaveLoadSource = "none";
  std::string productSaveLoadSelectedId = "none";
  bool productSaveLoadSelectedEnabled = false;
  ProductSavedRoomMarkerBindingResult savedMarkerBind;
  ProductSelectedProductSaveState selectedProductSave;
  std::string saveSlotBrowserMode = "load";
  std::uint64_t saveSlotRingCount = 0;
  std::uint64_t saveSlotRingSelectedIndex = 0;
  std::string saveSlotRingSelectedId = "none";
  std::string saveSlotRingSelectedStatus = "empty";
  std::string saveSlotActionCommand = "none";
  bool saveSlotActionEnabled = false;
  bool saveSlotActionConfirmationRequired = false;
  std::string saveSlotActionStatus = "not_requested";
  ProductSaveFlowState saveFlow;
  ProductSaveDeleteState saveDelete;
  bool deletedSaveBrowserOpen = false;
  std::uint64_t deletedSaveCount = 0;
  std::uint64_t deletedCompatibleSaveCount = 0;
  std::string deletedSelectedSaveId = "none";
  bool deletedSelectedSaveEnabled = false;
  std::string deletedSelectedSaveStatus = "none";
  ProductSaveRecoverState saveRecover;
};

}  // namespace iggy3d
