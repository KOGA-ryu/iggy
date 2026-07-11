#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocumentCreateRequest roomRequest(std::string_view name) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = std::string{name};
  return request;
}

bool roomObjectCarriesCommonCreativeShape() {
  cr::CreativeDocumentCreateRequest request = roomRequest("Atrium");
  request.bounds.min = {-5.0, 0.0, -6.0};
  request.bounds.max = {5.0, 4.0, 6.0};
  request.hasBoundsOverride = true;
  request.layerId = 7;
  request.hasLayerOverride = true;
  request.visible = false;
  request.hasVisibleOverride = true;
  request.locked = true;
  request.hasLockedOverride = true;
  request.tags = {"blockout", "entry"};

  cr::Facade facade;
  const cr::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(request);
  const cr::CreativeObject* room = facade.findObject(receipt.objectId);

  return expect(receipt.accepted && receipt.objectCreated,
                "room create accepted") &&
         expect(receipt.objectId != cr::kInvalidObjectId,
                "room id is stable nonzero") &&
         expect(room != nullptr, "room is findable") &&
         expect(room->id == receipt.objectId, "room id stored") &&
         expect(room->kind == cr::CreativeObjectKind::Room, "room kind") &&
         expect(room->name == "Atrium", "room name") &&
         expect(room->transform.position.x == 0.0,
                "room transform stays at descriptor default") &&
         expect(room->bounds.min.x == -5.0, "room bounds min x") &&
         expect(room->bounds.max.z == 6.0, "room bounds max z") &&
         expect(room->layerId == 7U, "room layer") &&
         expect(!room->visible, "room visible flag") &&
         expect(room->locked, "room locked flag") &&
         expect(room->tags.size() == 2U, "room tag count") &&
         expect(room->tags[0] == "blockout", "room first tag") &&
         expect(room->tags[1] == "entry", "room second tag") &&
         expect(!room->parentId.has_value(),
                "room has no parent by default");
}

bool documentRoomCreateRenameRemoveRevisionsAreStable() {
  cr::CreativeDocument document;

  const std::uint64_t initialRevision = document.revision();
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(roomRequest("Room A"));
  const cr::CreativeObjectId id = created.objectId;
  const cr::CreativeObject* room = document.findObject(id);

  const bool createdOk =
      expect(created.accepted && created.objectCreated,
             "document create room accepted") &&
      expect(id != cr::kInvalidObjectId, "document create room id") &&
      expect(room != nullptr, "document stores room") &&
      expect(document.objectCount() == 1U, "document room count") &&
      expect(document.revision() == initialRevision + 1U,
             "create increments revision");

  const std::uint64_t afterCreateRevision = document.revision();
  const cr::CreativeDocumentMutationReceipt sameNameRename =
      cr::renameDocumentObject(document, id, "Room A");
  const bool unchangedRename =
      expect(sameNameRename.status ==
                 cr::CreativeDocumentMutationStatus::NoChange,
             "same name rename reports no change") &&
      expect(document.revision() == afterCreateRevision,
             "same name rename does not increment revision");

  const cr::CreativeDocumentMutationReceipt changedRename =
      cr::renameDocumentObject(document, id, "Room B");
  const bool renamed =
      expect(changedRename.status ==
                 cr::CreativeDocumentMutationStatus::Applied,
             "changed rename succeeds") &&
      expect(document.revision() == afterCreateRevision + 1U,
             "changed rename increments revision") &&
      expect(document.findObject(id)->name == "Room B", "renamed object name");

  const std::uint64_t afterRenameRevision = document.revision();
  const cr::CreativeDocumentRemoveReceipt missingRemove =
      document.removeDocumentObject(9999);
  const bool missingRemoveStable =
      expect(!missingRemove.objectRemoved, "missing remove reports false") &&
      expect(document.revision() == afterRenameRevision,
             "missing remove does not increment revision");

  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(id);
  const bool removedState =
      expect(removed.objectRemoved, "existing remove succeeds") &&
      expect(document.findObject(id) == nullptr, "removed object gone") &&
      expect(document.objectCount() == 0U, "document empty after remove") &&
      expect(document.revision() == afterRenameRevision + 1U,
             "remove increments revision");

  return createdOk && unchangedRename && renamed && missingRemoveStable &&
         removedState;
}

bool facadeRoomCommandsCountMetrics() {
  cr::Facade facade;
  const cr::CreativeDocumentCreateReceipt created =
      facade.createDocumentObject(roomRequest("Metrics Room"));
  const cr::CreativeObjectId id = created.objectId;
  const cr::CreativeDocumentRemoveReceipt removeMissing =
      facade.removeDocumentObject(9999);
  const cr::CreativeDocumentRemoveReceipt removeOk =
      facade.removeDocumentObject(id);
  const cr::Stats& stats = facade.stats();

  return expect(id != cr::kInvalidObjectId, "facade created room") &&
         expect(created.accepted, "facade create success") &&
         expect(!removeMissing.objectRemoved,
                "facade remove missing failure") &&
         expect(removeOk.objectRemoved, "facade remove success") &&
         expect(facade.findObject(id) == nullptr, "facade removed room") &&
         expect(stats.commandAttempts == 3U, "command attempts") &&
         expect(stats.commandSuccesses == 2U, "command successes") &&
         expect(stats.commandFailures == 1U, "command failures") &&
         expect(stats.objectsCreated == 1U, "objects created") &&
         expect(stats.roomsCreated == 1U, "rooms created");
}

bool objectVocabularyKeepsRoomClassified() {
  return expect(cr::toString(cr::CreativeObjectKind::Room) == "Room",
                "room string") &&
         expect(cr::objectUsesCategory(cr::CreativeObjectKind::Room,
                                       cr::CreativeObjectCategory::Structural),
                "room structural descriptor classification") &&
         expect(!cr::objectUsesCategory(cr::CreativeObjectKind::Room,
                                        cr::CreativeObjectCategory::Gameplay),
                "room not gameplay descriptor classification");
}

}  // namespace

int main() {
  const bool ok = roomObjectCarriesCommonCreativeShape() &&
                  documentRoomCreateRenameRemoveRevisionsAreStable() &&
                  facadeRoomCommandsCountMetrics() &&
                  objectVocabularyKeepsRoomClassified();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
