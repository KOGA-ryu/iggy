#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "core/math/Vec3.hpp"

#include "EditorPersistence.hpp"
#include "EditorEdits.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

enum class StandaloneCaptureProofRole {
  None,
  Delete,
  Move,
  CreateUndo,
  Point,
  Line,
  Path,
};

struct StandaloneCapturePlacement {
  std::uint64_t frame = 0;
  double worldX = 0.0;
  double worldZ = 0.0;
  cr::CreativeObjectKind kind = cr::CreativeObjectKind::Unknown;
  StandaloneCaptureProofRole proofRole = StandaloneCaptureProofRole::None;
};

constexpr bool capturePlacementHasProofRole(
    const StandaloneCapturePlacement& placement,
    StandaloneCaptureProofRole role) noexcept {
  return placement.proofRole == role;
}

struct StandaloneCaptureScript {
  static constexpr std::uint64_t kDeleteNoSelectionFrame = 2U;
  static constexpr std::uint64_t kCreateUndoFrame = 7U;
  static constexpr std::uint64_t kDeleteFrame = 8U;
  static constexpr std::uint64_t kUndoFrame = 9U;
  static constexpr std::uint64_t kMoveBeginFrame = 10U;
  static constexpr std::uint64_t kMovePreviewFrame = 11U;
  static constexpr std::uint64_t kMoveCommitFrame = 12U;
  static constexpr std::uint64_t kMoveUndoFrame = 13U;
  static constexpr std::uint64_t kPointMoveBeginFrame = 15U;
  static constexpr std::uint64_t kPointMoveCommitFrame = 16U;
  static constexpr std::uint64_t kPointMoveUndoFrame = 17U;
  static constexpr std::uint64_t kLineMoveBeginFrame = 19U;
  static constexpr std::uint64_t kLineMoveCommitFrame = 20U;
  static constexpr std::uint64_t kLineMoveUndoFrame = 21U;
  static constexpr std::uint64_t kPathMoveFrame = 23U;
  static constexpr std::uint64_t kPathMoveUndoFrame = 24U;
  static constexpr std::uint64_t kPathPointMoveFrame = 25U;
  static constexpr std::uint64_t kPathPointMoveUndoFrame = 26U;
  static constexpr std::uint64_t kSaveFrame = 27U;
  static constexpr std::uint64_t kClearFrame = 28U;
  static constexpr std::uint64_t kLoadFrame = 29U;

  std::array<StandaloneCapturePlacement, 7> placements{{
      {3U, 2.0, 2.0, cr::CreativeObjectKind::Crate,
       StandaloneCaptureProofRole::Move},
      {4U, 4.0, 2.0, cr::CreativeObjectKind::Crate,
       StandaloneCaptureProofRole::Delete},
      {5U, -2.0, 2.0, cr::CreativeObjectKind::Wall,
       StandaloneCaptureProofRole::None},
      {6U, 6.0, 2.0, cr::CreativeObjectKind::Crate,
       StandaloneCaptureProofRole::CreateUndo},
      {14U, 6.0, 2.0, cr::CreativeObjectKind::PointLight,
       StandaloneCaptureProofRole::Point},
      {18U, 9.0, 4.0, cr::CreativeObjectKind::Beam,
       StandaloneCaptureProofRole::Line},
      {22U, 10.0, 1.0, cr::CreativeObjectKind::PatrolRoute,
       StandaloneCaptureProofRole::Path},
  }};

  std::vector<ObjectSnapshotEntry> roundtripBefore;
  std::size_t roundtripCountAfterClear = 0;
  bool roundtripSaved = false;
  bool roundtripCleared = false;
  bool roundtripLoaded = false;
  bool deleteNoSelectionAttempted = false;
  bool createUndoAttempted = false;
  bool deleteAttempted = false;
  bool undoAttempted = false;
  bool moveBeginAttempted = false;
  bool movePreviewAttempted = false;
  bool moveCommitAttempted = false;
  bool moveUndoAttempted = false;
  bool pointMoveBeginAttempted = false;
  bool pointMoveCommitAttempted = false;
  bool pointMoveUndoAttempted = false;
  bool pointHitProxyLogged = false;
  bool lineMoveBeginAttempted = false;
  bool lineMoveCommitAttempted = false;
  bool lineMoveUndoAttempted = false;
  bool lineHitProxyLogged = false;
  bool pathMoveAttempted = false;
  bool pathMoveUndoAttempted = false;
  bool pathHitProxyLogged = false;
  bool pathPointMoveAttempted = false;
  bool pathPointMoveUndoAttempted = false;
  bool pathPointHandleLogged = false;

  cr::CreativeObjectId createUndoTargetId = cr::kInvalidObjectId;
  cr::CreativeObjectId deleteTargetId = cr::kInvalidObjectId;
  cr::CreativeObjectId moveTargetId = cr::kInvalidObjectId;
  cr::CreativeObjectId pointTargetId = cr::kInvalidObjectId;
  cr::CreativeObjectId lineTargetId = cr::kInvalidObjectId;
  cr::CreativeObjectId pathTargetId = cr::kInvalidObjectId;
  cr::CreativeToolWorldPoint moveDestination{};
  cr::CreativeToolWorldPoint pointMoveDestination{};
  cr::CreativeToolWorldPoint lineMoveDestination{};
};

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

struct CreativeEditorState;

void runCreativeEditorCaptureScenarioFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    bool captureMode);

}  // namespace iggy3d_creative_app
