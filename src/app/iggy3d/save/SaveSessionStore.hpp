#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/save/SaveDeleteState.hpp"
#include "app/iggy3d/save/SaveFlowState.hpp"
#include "app/iggy3d/save/SaveRecoverState.hpp"
#include "app/iggy3d/save/SelectedProductSaveState.hpp"

namespace iggy3d {

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
