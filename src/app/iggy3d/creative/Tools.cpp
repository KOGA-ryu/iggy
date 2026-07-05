#include "app/iggy3d/creative/Tools.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool samePointer(const CreativeToolPointerPacket& lhs,
                               const CreativeToolPointerPacket& rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.button == rhs.button &&
         lhs.modifiers == rhs.modifiers && lhs.target.value == rhs.target.value &&
         lhs.hasWorldDestination == rhs.hasWorldDestination &&
         lhs.worldDestination.x == rhs.worldDestination.x &&
         lhs.worldDestination.y == rhs.worldDestination.y &&
         lhs.worldDestination.z == rhs.worldDestination.z;
}

void updatePointer(CreativeToolState& state,
                   const CreativeToolPointerPacket& pointer,
                   bool& changedState) noexcept {
  if (!samePointer(state.pointer, pointer)) {
    state.pointer = pointer;
    changedState = true;
  }
}

[[nodiscard]] CreativeToolDispatchReceipt makeReceipt(
    Tool activeToolBefore,
    CreativeToolInputKind inputKind) {
  CreativeToolDispatchReceipt receipt;
  receipt.activeToolBefore = activeToolBefore;
  receipt.activeToolAfter = activeToolBefore;
  receipt.inputKind = inputKind;
  return receipt;
}

void emitIntent(CreativeToolDispatchReceipt& receipt,
                CreativeToolIntentKind kind,
                Tool tool,
                const CreativeToolPointerPacket& pointer) {
  receipt.intents.push_back(CreativeToolIntent{kind, tool, pointer});
  receipt.emittedIntentCount = receipt.intents.size();
}

}  // namespace

CreativeToolState makeDefaultCreativeToolState() noexcept {
  return {};
}

bool setActiveTool(CreativeToolState& state, Tool tool) noexcept {
  if (state.activeTool == tool) {
    return false;
  }

  state.activeTool = tool;
  state.measurementActive = false;
  // A tool switch abandons any Move drag in flight so a stranded drag cannot
  // commit into the newly-selected tool (mirrors the pointer-lifecycle reset).
  state.moveDragActive = false;
  state.moveDragTarget = {};
  return true;
}

CreativeToolDispatchReceipt dispatchToolInput(
    CreativeToolState& state,
    const CreativeToolInputPacket& input) {
  const Tool activeToolBefore = state.activeTool;
  CreativeToolDispatchReceipt receipt =
      makeReceipt(activeToolBefore, input.kind);

  switch (input.kind) {
    case CreativeToolInputKind::PointerMove:
      receipt.accepted = true;
      if (state.activeTool == Tool::Navigate) {
        // Navigate never touches the document; the camera arrives in TV1-H.
        receipt.message = "navigate_pointer_inert";
        break;
      }
      updatePointer(state, input.pointer, receipt.changedState);
      if (state.activeTool == Tool::Measure) {
        emitIntent(receipt,
                   CreativeToolIntentKind::UpdateMeasurement,
                   state.activeTool,
                   input.pointer);
        receipt.message = "measurement_update";
      } else if (state.activeTool == Tool::Move && state.moveDragActive) {
        // TD-6: a held Move drag previews only — no mutation until Release.
        emitIntent(receipt,
                   CreativeToolIntentKind::PreviewMove,
                   state.activeTool,
                   input.pointer);
        receipt.message = "move_preview";
      } else {
        emitIntent(receipt,
                   CreativeToolIntentKind::PreviewPointer,
                   state.activeTool,
                   input.pointer);
        receipt.message = "preview_pointer";
      }
      break;

    case CreativeToolInputKind::PointerPress:
      receipt.accepted = true;
      if (state.activeTool == Tool::Navigate) {
        // Navigate never touches the document; the camera arrives in TV1-H.
        receipt.message = "navigate_pointer_inert";
        break;
      }
      updatePointer(state, input.pointer, receipt.changedState);
      switch (state.activeTool) {
        case Tool::Select:
          emitIntent(receipt,
                     CreativeToolIntentKind::SelectObjectCandidate,
                     state.activeTool,
                     input.pointer);
          receipt.message = "select_object_candidate";
          break;
        case Tool::Move:
          // Move press selects (TV1-C) AND begins a drag (TV1-G). The picked
          // target seeds the drag; the facade falls back to the current
          // selection when the press missed a specific object.
          emitIntent(receipt,
                     CreativeToolIntentKind::SelectObjectCandidate,
                     state.activeTool,
                     input.pointer);
          state.moveDragActive = true;
          state.moveDragTarget = input.pointer.target;
          receipt.changedState = true;
          emitIntent(receipt,
                     CreativeToolIntentKind::BeginMove,
                     state.activeTool,
                     input.pointer);
          receipt.message = "move_drag_begin";
          break;
        case Tool::Measure:
          if (!state.measurementActive) {
            state.measurementActive = true;
            receipt.changedState = true;
          }
          emitIntent(receipt,
                     CreativeToolIntentKind::BeginMeasurement,
                     state.activeTool,
                     input.pointer);
          receipt.message = "begin_measurement";
          break;
        case Tool::Navigate:
          break;
      }
      break;

    case CreativeToolInputKind::PointerRelease:
      receipt.accepted = true;
      if (state.activeTool == Tool::Navigate) {
        // Navigate never touches the document; the camera arrives in TV1-H.
        receipt.message = "navigate_pointer_inert";
        break;
      }
      updatePointer(state, input.pointer, receipt.changedState);
      if (state.activeTool == Tool::Measure && state.measurementActive) {
        state.measurementActive = false;
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::EndMeasurement,
                   state.activeTool,
                   input.pointer);
        receipt.message = "end_measurement";
      } else if (state.activeTool == Tool::Move && state.moveDragActive) {
        // TD-6: release ends the drag and commits exactly one snapped Move.
        // (TV1-F entry req ii: a Release with no active drag falls through to
        // no_intent below — a harmless no-op, never a spurious move.)
        state.moveDragActive = false;
        state.moveDragTarget = {};
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::CommitMove,
                   state.activeTool,
                   input.pointer);
        receipt.message = "move_drag_commit";
      } else {
        receipt.message = "no_intent";
      }
      break;

    case CreativeToolInputKind::Cancel:
      receipt.accepted = true;
      if (state.moveDragActive) {
        // TD-6: Esc/Cancel mid-drag discards the drag with NO mutation.
        state.moveDragActive = false;
        state.moveDragTarget = {};
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::CancelMove,
                   state.activeTool,
                   input.pointer);
        receipt.message = "move_drag_cancel";
      } else if (state.measurementActive) {
        state.measurementActive = false;
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::CancelToolAction,
                   state.activeTool,
                   input.pointer);
        receipt.message = "cancel_tool_action";
      } else {
        receipt.message = "no_intent";
      }
      break;

    case CreativeToolInputKind::Unknown:
      receipt.message = "unsupported_input";
      break;
  }

  receipt.activeToolAfter = state.activeTool;
  receipt.emittedIntentCount = receipt.intents.size();
  return receipt;
}

}  // namespace iggy3d::creative
