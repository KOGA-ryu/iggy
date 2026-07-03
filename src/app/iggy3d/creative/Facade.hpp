#pragma once

#include "app/iggy3d/creative/Commands.hpp"
#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/DocumentMutation.hpp"
#include "app/iggy3d/creative/Ghost.hpp"
#include "app/iggy3d/creative/Inspect.hpp"
#include "app/iggy3d/creative/Measure.hpp"
#include "app/iggy3d/creative/Metrics.hpp"
#include "app/iggy3d/creative/Object.hpp"
#include "app/iggy3d/creative/Select.hpp"
#include "app/iggy3d/creative/Snap.hpp"
#include "app/iggy3d/creative/State.hpp"
#include "app/iggy3d/creative/Tools.hpp"
#include "app/iggy3d/creative/Ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

struct CreativeFacadeToolDispatchReceipt {
  CreativeToolInputKind inputKind = CreativeToolInputKind::Unknown;
  Tool activeToolBefore = Tool::Select;
  Tool activeToolAfter = Tool::Select;
  std::size_t emittedIntentCount = 0;
  bool toolAccepted = false;
  bool selectionChanged = false;
  bool inspectionChanged = false;
  bool measurementChanged = false;
  bool ghostChanged = false;
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
  bool inspectionCleared = false;
  bool measurementCleared = false;
  bool ghostCleared = false;
  bool toolPointerCleared = false;
  Tool activeToolBefore = Tool::Select;
  Tool activeToolAfter = Tool::Select;
  std::string_view status = "creative_facade_document_not_requested";
  std::string_view reasonCode = "creative_facade_document_not_requested";
  std::string_view message = "creative_facade_document_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeFacadeMutationStatus status) noexcept;

class Facade {
 public:
  void reset() noexcept;
  void beginFrame(const FramePacket& packet) noexcept;
  void handle(const Packet& packet) noexcept;

  [[nodiscard]] const State& state() const noexcept;
  [[nodiscard]] const CreativeToolState& toolState() const noexcept;
  [[nodiscard]] const CreativeSelectionState& selectionState() const noexcept;
  [[nodiscard]] const CreativeInspectionState& inspectionState() const noexcept;
  [[nodiscard]] const CreativeMeasurementState& measurementState() const noexcept;
  [[nodiscard]] const CreativeSnapSettings& snapSettings() const noexcept;
  [[nodiscard]] const CreativeGhostState& ghostState() const noexcept;
  [[nodiscard]] bool setActiveTool(Tool tool) noexcept;
  void setSnapSettings(CreativeSnapSettings settings) noexcept;
  [[nodiscard]] CreativeFacadeToolDispatchReceipt dispatchToolInput(
      const CreativeToolInputPacket& input);
  [[nodiscard]] CreativeUiBuildReceipt buildUiModel() const;
  [[nodiscard]] CreativeFacadeMutationReceipt
  toggleSelectedObjectVisibility();

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
  [[nodiscard]] CreativeObjectId createRoom(const CreateRoomCommand& command);
  [[nodiscard]] CreativeObjectId createRoom(std::string name);
  [[nodiscard]] bool renameObject(const RenameObjectCommand& command);
  [[nodiscard]] bool renameObject(CreativeObjectId id, std::string nextName);
  [[nodiscard]] bool removeObject(const RemoveObjectCommand& command);
  [[nodiscard]] bool removeObject(CreativeObjectId id);
  [[nodiscard]] const CreativeObject* findObject(
      CreativeObjectId id) const noexcept;
  [[nodiscard]] const CreativeDocument& document() const noexcept;
  [[nodiscard]] CreativeDocument& documentForPersistence() noexcept;
  [[nodiscard]] const Stats& stats() const noexcept;

 private:
  State state_;
  CreativeDocument document_;
  Stats stats_;
  CreativeToolState toolState_;
  CreativeSelectionState selectionState_;
  CreativeInspectionState inspectionState_;
  CreativeMeasurementState measurementState_;
  CreativeSnapSettings snapSettings_;
  CreativeGhostState ghostState_;
};

}  // namespace iggy3d::creative
