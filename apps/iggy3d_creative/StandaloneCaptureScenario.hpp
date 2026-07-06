#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "core/math/Vec3.hpp"

#include "StandaloneCaptureScript.hpp"
#include "StandaloneUndo.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

using StandaloneCaptureDeleteSelectedFn =
    std::function<cr::CreativeDocumentRemoveReceipt(std::string_view)>;

struct StandaloneCaptureScenarioStepRequest {
  bool enabled = false;
  std::uint64_t frameIndex = 0;
  cr::CreativeAppState* appState = nullptr;
  StandaloneUndoStack* undoStack = nullptr;
  StandaloneCaptureScript* captureScript = nullptr;
  cr::CreativeObjectKind* placeBrush = nullptr;
  bool* placeMode = nullptr;
  std::uint64_t* placedCount = nullptr;
  double placeCellSize = 1.0;
  const std::filesystem::path* saveRoot = nullptr;
  const std::string* saveId = nullptr;
  cr::CreativeToolMoveHeldAxis moveHeldAxisForX =
      cr::CreativeToolMoveHeldAxis::Y;
  cr::CreativeToolMoveHeldAxis moveHeldAxisForZ =
      cr::CreativeToolMoveHeldAxis::X;
  StandaloneCaptureDeleteSelectedFn deleteSelected;
};

void runStandaloneCaptureScenarioStep(
    const StandaloneCaptureScenarioStepRequest& request);

}  // namespace iggy3d_creative_app
