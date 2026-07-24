#include "EditorEdits.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeAppState makeApp(cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Edit Transaction");
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool changedDocumentCannotLoseItsUndoRecord() {
  cr::CreativeAppState appState = makeApp(9810U);
  app::StandaloneEditTransaction transaction =
      app::beginEditTransaction(appState.facade, "mismatched-change");
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(request);

  const cr::CreativeHistoryRecordReceipt recorded =
      app::completeEditTransaction(
          appState.history, std::move(transaction), appState.facade, false,
          "caller_reported_no_change");
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(created.accepted && created.changed,
                "transaction fixture changes the document") &&
         expect(recorded.accepted && recorded.recorded,
                "revision mismatch records history despite a false flag") &&
         expect(undone.accepted && undone.changed &&
                    appState.facade.document().objectCount() == 0U,
                "recovered history record remains undoable");
}

bool unchangedTransactionCancels() {
  cr::CreativeAppState appState = makeApp(9811U);
  app::StandaloneEditTransaction transaction =
      app::beginEditTransaction(appState.facade, "unchanged");
  const cr::CreativeHistoryRecordReceipt recorded =
      app::completeEditTransaction(
          appState.history, std::move(transaction), appState.facade, false,
          "no_change");

  return expect(!recorded.requested && !recorded.recorded,
                "unchanged transaction cancels before the history kernel") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "unchanged transaction adds no undo entry");
}

bool explicitCancelClearsTransaction() {
  cr::CreativeAppState appState = makeApp(9812U);
  cr::CreativeHistorySidecar sidecar;
  sidecar.type = "test";
  sidecar.version = 1U;
  sidecar.payload = "{}";
  app::StandaloneEditTransaction transaction = app::beginEditTransaction(
      appState.facade, "cancel", std::move(sidecar));
  const bool startedWithSidecar =
      transaction.active && transaction.beforeSidecar.has_value();
  app::cancelEditTransaction(transaction);

  return expect(startedWithSidecar,
                "sidecar overload preserves transaction metadata") &&
         expect(!transaction.active && !transaction.beforeSidecar.has_value(),
                "editor cancel boundary clears transaction state");
}

}  // namespace

int main() {
  return changedDocumentCannotLoseItsUndoRecord() &&
                 unchangedTransactionCancels() &&
                 explicitCancelClearsTransaction()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
