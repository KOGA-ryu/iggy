#include "EditorDesktopCommandsInternal.hpp"

#include "app/iggy3d/creative/history/History.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopMeasurementCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  if (command.id != CreativeDesktopCommandId::SaveMeasurementAnnotation &&
      command.id != CreativeDesktopCommandId::RemoveMeasurementAnnotation) {
    return false;
  }

  const auto* payload =
      payloadAs<CreativeDesktopMeasurementAnnotationPayload>(command);
  if (payload == nullptr) {
    result.message = "measurement annotation: payload mismatch";
    return true;
  }

  creative::CreativeAppState& appState = context.appState;
  creative::CreativeDocumentHistoryTransaction transaction =
      creative::beginCreativeHistoryTransaction(
          appState.facade,
          command.id == CreativeDesktopCommandId::SaveMeasurementAnnotation
              ? "desktop_save_measurement_annotation"
              : "desktop_remove_measurement_annotation");
  if (command.id == CreativeDesktopCommandId::SaveMeasurementAnnotation) {
    const creative::CreativeFacadeMeasurementAnnotationSaveReceipt receipt =
        appState.facade.saveMeasurementAnnotation(payload->name);
    result.accepted = receipt.accepted;
    result.changed = receipt.changed;
    result.message = std::string{receipt.reasonCode};
  } else {
    const creative::CreativeMeasurementAnnotationMutationReceipt receipt =
        appState.facade.removeMeasurementAnnotation(payload->annotationId);
    result.accepted = receipt.accepted;
    result.changed = receipt.changed;
    result.message = std::string{receipt.reasonCode};
  }

  if (result.changed) {
    const creative::CreativeHistoryRecordReceipt history =
        creative::commitCreativeHistoryTransaction(
            appState.history, std::move(transaction), appState.facade);
    if (!history.recorded) {
      result.message += "; history not recorded: ";
      result.message += history.reasonCode;
    }
  } else {
    creative::cancelCreativeHistoryTransaction(transaction);
  }
  result.sceneChanged = result.changed;
  result.affectedObjectCount = result.changed ? 1U : 0U;
  return true;
}

}  // namespace iggy3d_creative_app
