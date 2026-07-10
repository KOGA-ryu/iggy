#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/spatial/Ghost.hpp"
#include "app/iggy3d/creative/tools/Measure.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/spatial/Snap.hpp"
#include "app/iggy3d/creative/State.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/ui/Ui.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace iggy3d::creative {

// Move-tool drag lifecycle stages, receipt-visible (TL-6): a drag is fully
// observable — begin, preview target, and the commit outcome.
enum class CreativeFacadeMoveDragStage : std::uint8_t {
  None,
  Begin,
  Preview,
  Commit,
  Cancelled,
};

enum class CreativeFacadeMoveDragOutcome : std::uint8_t {
  None,
  Begun,
  Previewing,
  Applied,
  NoChange,
  RejectedLocked,
  Rejected,
  Cancelled,
  NoTarget,
};

// Full receipt for one Move-drag lifecycle event. Anchors are corner anchors
// (TD-2): bounds.min for bounds-only kinds, transform.position otherwise. The
// destination Y equals the start anchor Y (TD-7: XZ-only move in v1).
struct CreativeFacadeMoveDragReceipt {
  CreativeFacadeMoveDragStage stage = CreativeFacadeMoveDragStage::None;
  CreativeFacadeMoveDragOutcome outcome = CreativeFacadeMoveDragOutcome::None;
  bool requested = false;
  bool accepted = false;
  bool committed = false;
  bool changed = false;
  bool locked = false;
  TargetRef target;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  bool hasStartAnchor = false;
  CreativeVec3 startAnchor;
  bool hasDestinationAnchor = false;
  CreativeVec3 requestedAnchor;
  CreativeVec3 snappedAnchor;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  CreativeDocumentMutationStatus documentStatus =
      CreativeDocumentMutationStatus::Unknown;
  std::string message = "move_drag_not_requested";
};

struct CreativeFacadeToolDispatchReceipt {
  CreativeToolInputKind inputKind = CreativeToolInputKind::Unknown;
  Tool activeToolBefore = Tool::Select;
  Tool activeToolAfter = Tool::Select;
  std::size_t emittedIntentCount = 0;
  bool toolAccepted = false;
  bool selectionChanged = false;
  bool measurementChanged = false;
  bool ghostChanged = false;
  bool moveDragChanged = false;
  CreativeFacadeMoveDragReceipt moveDrag;
  bool accepted = false;
  bool changed = false;
  std::string_view message = "tool_input_not_dispatched";
};

enum class CreativeFacadeMutationStatus : std::uint8_t {
  Unknown,
  NoSelection,
  MissingObject,
  Applied,
  NoChange,
  Rejected,
};

struct CreativeFacadeMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool hadSelection = false;
  TargetRef target;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  bool visibleBefore = false;
  bool visibleAfter = false;
  bool lockedBefore = false;
  bool lockedAfter = false;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  CreativeFacadeMutationStatus status = CreativeFacadeMutationStatus::Unknown;
  CreativeDocumentMutationStatus documentStatus =
      CreativeDocumentMutationStatus::Unknown;
  CreativeMutationKind mutationKind = CreativeMutationKind::Unknown;
  std::string message;
};

struct CreativeFacadeDocumentInstallReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool hadPreviousDocument = false;
  CreativeDocumentId previousDocumentId = kInvalidDocumentId;
  CreativeDocumentId nextDocumentId = kInvalidDocumentId;
  std::uint64_t previousObjectCount = 0;
  std::uint64_t nextObjectCount = 0;
  CreativeObjectDirtyFlags previousDirtyFlags = 0;
  CreativeObjectDirtyFlags nextDirtyFlags = 0;
  bool selectionCleared = false;
  bool measurementCleared = false;
  bool ghostCleared = false;
  bool toolPointerCleared = false;
  Tool activeToolBefore = Tool::Select;
  Tool activeToolAfter = Tool::Select;
  std::string_view status = "creative_facade_document_not_requested";
  std::string_view reasonCode = "creative_facade_document_not_requested";
  std::string_view message = "creative_facade_document_not_requested";
};

enum class CreativeFacadeDocumentBatchCreateStatus : std::uint8_t {
  Unknown,
  Empty,
  CreateRejected,
  InstallRejected,
  Applied,
};

struct CreativeFacadeDocumentBatchCreateReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeFacadeDocumentBatchCreateStatus status =
      CreativeFacadeDocumentBatchCreateStatus::Unknown;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t attemptedCreateCount = 0;
  std::uint64_t appliedCreateCount = 0;
  bool hasFailedCreate = false;
  std::uint64_t firstFailedCreateIndex = 0;
  CreativeDocumentCreateStatus firstFailedCreateStatus =
      CreativeDocumentCreateStatus::Unknown;
  std::string_view firstFailedCreateReasonCode =
      "document_create_not_requested";
  std::string_view firstFailedCreateMessage =
      "document_create_not_requested";
  bool installAttempted = false;
  CreativeFacadeDocumentInstallReceipt installReceipt;
  std::string_view reasonCode =
      "creative_facade_batch_create_not_requested";
  std::string_view message =
      "creative_facade_batch_create_not_requested";
};

struct Stats {
  std::uint64_t commandAttempts = 0;
  std::uint64_t commandSuccesses = 0;
  std::uint64_t commandFailures = 0;
  std::uint64_t objectsCreated = 0;
  std::uint64_t roomsCreated = 0;
};

[[nodiscard]] std::string_view toString(
    CreativeFacadeMutationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeFacadeDocumentBatchCreateStatus status) noexcept;

class Facade {
 public:
  void reset() noexcept;
  void beginFrame(const FramePacket& packet) noexcept;
  void handle(const Packet& packet) noexcept;

  [[nodiscard]] const State& state() const noexcept;
  [[nodiscard]] const CreativeToolState& toolState() const noexcept;
  [[nodiscard]] const CreativeSelectionState& selectionState() const noexcept;
  [[nodiscard]] const CreativeMeasurementState& measurementState() const noexcept;
  [[nodiscard]] const CreativeSnapSettings& snapSettings() const noexcept;
  [[nodiscard]] const CreativeGhostState& ghostState() const noexcept;
  [[nodiscard]] const CreativeFacadeMoveDragReceipt& moveDragReceipt()
      const noexcept;
  [[nodiscard]] bool setActiveTool(Tool tool) noexcept;
  void setSnapSettings(CreativeSnapSettings settings) noexcept;
  [[nodiscard]] CreativeFacadeToolDispatchReceipt dispatchToolInput(
      const CreativeToolInputPacket& input);
  [[nodiscard]] CreativeUiBuildReceipt buildUiModel() const;
  [[nodiscard]] CreativeUiBuildReceipt buildUiModel(
      CreativeUiBuildOptions options) const;
  [[nodiscard]] CreativeFacadeMutationReceipt
  toggleSelectedObjectVisibility();
  [[nodiscard]] CreativeFacadeMutationReceipt toggleSelectedObjectLocked();

  [[nodiscard]] CreativeDocumentCreateReceipt createDocumentObject(
      const CreativeDocumentCreateRequest& request);
  [[nodiscard]] CreativeDocumentCreateReceipt createDocumentObject(
      CreativeObjectKind kind);
  [[nodiscard]] CreativeDocumentRemoveReceipt removeDocumentObject(
      const CreativeDocumentRemoveRequest& request);
  [[nodiscard]] CreativeDocumentRemoveReceipt removeDocumentObject(
      CreativeObjectId id);
  [[nodiscard]] CreativeFacadeDocumentInstallReceipt installDocument(
      CreativeDocument document);
  [[nodiscard]] CreativeFacadeDocumentBatchCreateReceipt
  createDocumentObjectsAtomically(
      std::span<const CreativeDocumentCreateRequest> requests);
  [[nodiscard]] const CreativeObject* findObject(
      CreativeObjectId id) const noexcept;
  [[nodiscard]] const CreativeDocument& document() const noexcept;
  [[nodiscard]] CreativeDocument& documentForPersistence() noexcept;
  [[nodiscard]] const Stats& stats() const noexcept;

 private:
  // Applies one Move-drag lifecycle intent (BeginMove/PreviewMove/CommitMove/
  // CancelMove) against the drag-tracking members. Returns the stage receipt.
  CreativeFacadeMoveDragReceipt applyMoveDragIntent(
      const CreativeToolIntent& intent);

  State state_;
  CreativeDocument document_;
  Stats stats_;
  CreativeToolState toolState_;
  CreativeSelectionState selectionState_;
  CreativeMeasurementState measurementState_;
  CreativeSnapSettings snapSettings_;
  CreativeGhostState ghostState_;
  // Move-drag tracking (TV1-G): the resolved drag target + its start corner
  // anchor, recorded on BeginMove and consumed on Preview/Commit. Y is held
  // fixed to startAnchor.y for the whole drag (TD-7).
  bool moveDragActive_ = false;
  TargetRef moveDragTarget_;
  CreativeObjectId moveDragObjectId_ = kInvalidObjectId;
  CreativeVec3 moveDragStartAnchor_;
  CreativeFacadeMoveDragReceipt moveDragReceipt_;
};

}  // namespace iggy3d::creative
