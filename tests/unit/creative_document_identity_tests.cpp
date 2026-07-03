#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/DocumentMutation.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocumentCreateReceipt createRoom(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  return document.createObject(request);
}

bool defaultAndCreatedDocumentsStartWithInvalidIdentity() {
  const cr::CreativeDocument defaultDocument;
  const cr::CreativeDocument namedDocument =
      cr::CreativeDocument::create("Document");

  return expect(defaultDocument.isValid(), "default document valid") &&
         expect(defaultDocument.id() == cr::kInvalidDocumentId,
                "default id invalid") &&
         expect(defaultDocument.revision() == 0U, "default revision zero") &&
         expect(defaultDocument.dirtyFlags() == 0U,
                "default dirty flags zero") &&
         expect(namedDocument.isValid(), "created document valid") &&
         expect(namedDocument.id() == cr::kInvalidDocumentId,
                "created document id invalid") &&
         expect(namedDocument.name() == "Document", "created name") &&
         expect(namedDocument.revision() == 0U, "created revision zero") &&
         expect(namedDocument.dirtyFlags() == 0U,
                "created dirty flags zero");
}

bool assigningNonzeroIdentityDoesNotDirtyOrRevise() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");

  const bool assigned = document.assignId(42);

  return expect(assigned, "assign nonzero accepted") &&
         expect(document.id() == 42U, "assigned id stored") &&
         expect(document.revision() == 0U, "assign revision stable") &&
         expect(document.dirtyFlags() == 0U, "assign dirty zero") &&
         expect(document.drainDirtyFlags() == 0U, "assign drain zero") &&
         expect(document.revision() == 0U, "assign drain revision stable");
}

bool sameIdentityAndDifferentIdentityAreNoChangeAfterAssignment() {
  cr::CreativeDocument document;
  const bool first = document.assignId(7);
  const std::uint64_t revisionAfterFirst = document.revision();
  const cr::CreativeObjectDirtyFlags dirtyAfterFirst = document.dirtyFlags();

  const bool same = document.assignId(7);
  const bool different = document.assignId(8);

  return expect(first, "first id assignment accepted") &&
         expect(!same, "same id assignment no-change false") &&
         expect(!different, "different id reassignment rejected") &&
         expect(document.id() == 7U, "id remains first value") &&
         expect(document.revision() == revisionAfterFirst,
                "reassign revision stable") &&
         expect(document.dirtyFlags() == dirtyAfterFirst,
                "reassign dirty stable");
}

bool invalidIdentityAssignmentIsRejectedAndDoesNotClear() {
  cr::CreativeDocument document;

  const bool invalidInitial = document.assignId(cr::kInvalidDocumentId);
  const bool assigned = document.assignId(99);
  const bool invalidAfterAssigned = document.assignId(cr::kInvalidDocumentId);

  return expect(!invalidInitial, "initial invalid id rejected") &&
         expect(document.id() == 99U, "valid id assigned") &&
         expect(assigned, "valid assignment accepted") &&
         expect(!invalidAfterAssigned, "invalid id does not clear") &&
         expect(document.id() == 99U, "id remains assigned") &&
         expect(document.revision() == 0U, "invalid assign revision stable") &&
         expect(document.dirtyFlags() == 0U, "invalid assign dirty stable");
}

bool resetClearsDocumentIdentity() {
  cr::CreativeDocument document;
  const bool assigned = document.assignId(123);
  const cr::CreativeDocumentCreateReceipt created = createRoom(document);

  document.reset();

  return expect(assigned, "reset id assignment accepted") &&
         expect(created.accepted, "reset setup create accepted") &&
         expect(document.id() == cr::kInvalidDocumentId,
                "reset clears id") &&
         expect(document.revision() == 0U, "reset revision zero") &&
         expect(document.dirtyFlags() == 0U, "reset dirty zero") &&
         expect(document.objectCount() == 0U, "reset object count zero");
}

bool createRemoveAndMutationPreserveAssignedIdentity() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  const bool assigned = document.assignId(55);

  const cr::CreativeDocumentCreateReceipt created = createRoom(document);
  const cr::CreativeDocumentMutationReceipt mutated =
      cr::setDocumentObjectVisible(document, created.objectId, false);
  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(created.objectId);

  return expect(assigned, "preserve setup id assigned") &&
         expect(created.accepted, "preserve create accepted") &&
         expect(document.id() == 55U, "id after create") &&
         expect(mutated.status == cr::CreativeDocumentMutationStatus::Applied,
                "preserve mutation applied") &&
         expect(document.id() == 55U, "id after mutation") &&
         expect(removed.accepted, "preserve remove accepted") &&
         expect(document.id() == 55U, "id after remove");
}

bool facadeResetLeavesDocumentIdentityInvalid() {
  cr::Facade facade;
  const cr::CreativeDocumentCreateReceipt created =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);

  facade.reset();

  return expect(created.accepted, "facade identity setup create accepted") &&
         expect(facade.document().isValid(), "facade document valid") &&
         expect(facade.document().id() == cr::kInvalidDocumentId,
                "facade reset invalid id") &&
         expect(facade.document().revision() == 0U,
                "facade reset revision zero") &&
         expect(facade.document().dirtyFlags() == 0U,
                "facade reset dirty zero");
}

}  // namespace

int main() {
  const bool ok = defaultAndCreatedDocumentsStartWithInvalidIdentity() &&
                  assigningNonzeroIdentityDoesNotDirtyOrRevise() &&
                  sameIdentityAndDifferentIdentityAreNoChangeAfterAssignment() &&
                  invalidIdentityAssignmentIsRejectedAndDoesNotClear() &&
                  resetClearsDocumentIdentity() &&
                  createRemoveAndMutationPreserveAssignedIdentity() &&
                  facadeResetLeavesDocumentIdentityInvalid();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
