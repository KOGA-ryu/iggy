#include "app/iggy3d/creative/Tools.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool samePointer(const CreativeToolPointerPacket& lhs,
                               const CreativeToolPointerPacket& rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.button == rhs.button &&
         lhs.modifiers == rhs.modifiers && lhs.target.value == rhs.target.value;
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
        case Tool::Move:
          // Move selects like Select until the drag slice (TV1-F/G) lands.
          emitIntent(receipt,
                     CreativeToolIntentKind::SelectObjectCandidate,
                     state.activeTool,
                     input.pointer);
          receipt.message = "select_object_candidate";
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
      } else {
        receipt.message = "no_intent";
      }
      break;

    case CreativeToolInputKind::Cancel:
      receipt.accepted = true;
      if (state.measurementActive) {
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
