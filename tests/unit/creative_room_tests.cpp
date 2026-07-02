#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Object.hpp"

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

bool roomObjectCarriesCommonCreativeShape() {
  cr::CreateRoomCommand command = cr::makeCreateRoomCommand("Atrium");
  command.transform.position = {1.0, 2.0, 3.0};
  command.transform.rotation = {0.0, 90.0, 0.0};
  command.transform.scale = {2.0, 3.0, 4.0};
  command.bounds.min = {-5.0, 0.0, -6.0};
  command.bounds.max = {5.0, 4.0, 6.0};
  command.layerId = 7;
  command.visible = false;
  command.locked = true;
  command.tags = {"blockout", "entry"};
  command.parentId = 42;

  cr::Facade facade;
  const cr::CreativeObjectId id = facade.createRoom(command);
  const cr::CreativeObject* room = facade.findObject(id);

  return expect(id != cr::kInvalidObjectId, "room id is stable nonzero") &&
         expect(room != nullptr, "room is findable") &&
         expect(room->id == id, "room id stored") &&
         expect(room->kind == cr::CreativeObjectKind::Room, "room kind") &&
         expect(room->name == "Atrium", "room name") &&
         expect(room->transform.position.x == 1.0, "room position x") &&
         expect(room->transform.rotation.y == 90.0, "room rotation y") &&
         expect(room->transform.scale.z == 4.0, "room scale z") &&
         expect(room->bounds.min.x == -5.0, "room bounds min x") &&
         expect(room->bounds.max.z == 6.0, "room bounds max z") &&
         expect(room->layerId == 7U, "room layer") &&
         expect(!room->visible, "room visible flag") &&
         expect(room->locked, "room locked flag") &&
         expect(room->tags.size() == 2U, "room tag count") &&
         expect(room->tags[0] == "blockout", "room first tag") &&
         expect(room->tags[1] == "entry", "room second tag") &&
         expect(room->parentId.has_value(), "room parent present") &&
         expect(*room->parentId == 42U, "room parent id");
}

bool documentRoomCreateRenameRemoveRevisionsAreStable() {
  cr::CreativeDocument document;

  const std::uint64_t initialRevision = document.revision();
  const cr::CreativeObjectId id = document.createRoom("Room A");
  const cr::CreativeObject* room = document.findObject(id);

  const bool created =
      expect(id != cr::kInvalidObjectId, "document create room id") &&
      expect(room != nullptr, "document stores room") &&
      expect(document.objectCount() == 1U, "document room count") &&
      expect(document.revision() == initialRevision + 1U,
             "create increments revision");

  const std::uint64_t afterCreateRevision = document.revision();
  const bool sameNameRename = document.renameObject(id, "Room A");
  const bool unchangedRename =
      expect(!sameNameRename, "same name rename reports no change") &&
      expect(document.revision() == afterCreateRevision,
             "same name rename does not increment revision");

  const bool changedRename = document.renameObject(id, "Room B");
  const bool renamed =
      expect(changedRename, "changed rename succeeds") &&
      expect(document.revision() == afterCreateRevision + 1U,
             "changed rename increments revision") &&
      expect(document.findObject(id)->name == "Room B", "renamed object name");

  const std::uint64_t afterRenameRevision = document.revision();
  const bool missingRemove = document.removeObject(9999);
  const bool missingRemoveStable =
      expect(!missingRemove, "missing remove reports false") &&
      expect(document.revision() == afterRenameRevision,
             "missing remove does not increment revision");

  const bool removed = document.removeObject(id);
  const bool removedState =
      expect(removed, "existing remove succeeds") &&
      expect(document.findObject(id) == nullptr, "removed object gone") &&
      expect(document.objectCount() == 0U, "document empty after remove") &&
      expect(document.revision() == afterRenameRevision + 1U,
             "remove increments revision");

  return created && unchangedRename && renamed && missingRemoveStable &&
         removedState;
}

bool facadeRoomCommandsCountMetrics() {
  cr::Facade facade;
  const cr::CreativeObjectId id = facade.createRoom("Metrics Room");
  const bool renameOk = facade.renameObject(id, "Metrics Room Renamed");
  const bool renameNoop = facade.renameObject(id, "Metrics Room Renamed");
  const bool removeMissing = facade.removeObject(9999);
  const bool removeOk = facade.removeObject(id);
  const cr::Stats& stats = facade.stats();

  return expect(id != cr::kInvalidObjectId, "facade created room") &&
         expect(renameOk, "facade rename success") &&
         expect(!renameNoop, "facade rename no-op failure") &&
         expect(!removeMissing, "facade remove missing failure") &&
         expect(removeOk, "facade remove success") &&
         expect(facade.findObject(id) == nullptr, "facade removed room") &&
         expect(stats.commandAttempts == 5U, "command attempts") &&
         expect(stats.commandSuccesses == 3U, "command successes") &&
         expect(stats.commandFailures == 2U, "command failures") &&
         expect(stats.objectsCreated == 1U, "objects created") &&
         expect(stats.roomsCreated == 1U, "rooms created");
}

bool objectVocabularyKeepsRoomClassified() {
  return expect(cr::toString(cr::CreativeObjectKind::Room) == "Room",
                "room string") &&
         expect(cr::isStructuralObject(cr::CreativeObjectKind::Room),
                "room structural classification") &&
         expect(!cr::isGameplayObject(cr::CreativeObjectKind::Room),
                "room not gameplay classification");
}

}  // namespace

int main() {
  const bool ok = roomObjectCarriesCommonCreativeShape() &&
                  documentRoomCreateRenameRemoveRevisionsAreStable() &&
                  facadeRoomCommandsCountMetrics() &&
                  objectVocabularyKeepsRoomClassified();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
