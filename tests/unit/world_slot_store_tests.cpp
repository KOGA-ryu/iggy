#include "content/authoring/WorldSlotStore.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_world_slot_store_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

bool createListAndDeleteRoundTrips() {
  const std::filesystem::path root = testRoot();
  iggy3d::WorldSlotCreateRequest request;
  request.root = root;
  request.name = "Movement Test World";
  request.packagePath = "fixtures/demos/movement_playground/package.iggy3d.toml";
  const iggy3d::WorldSlotCreateResult created = iggy3d::createWorldSlot(request);
  const std::vector<iggy3d::WorldSlotRecord> listed = iggy3d::listWorldSlots(root);
  const bool createdFileExists = created.ok && std::filesystem::exists(created.slot.path);
  const bool deleted =
      created.ok && !listed.empty() && iggy3d::deleteWorldSlotFile(listed.front().path);
  const std::vector<iggy3d::WorldSlotRecord> afterDelete = iggy3d::listWorldSlots(root);
  return expect(created.ok, "slot created") &&
         expect(created.reason == "world_slot_created", "create reason") &&
         expect(createdFileExists, "slot file exists") &&
         expect(listed.size() == 1U, "one slot listed") &&
         expect(listed.front().id == "world_001", "slot id") &&
         expect(listed.front().name == "Movement Test World", "slot name") &&
         expect(listed.front().packagePath == request.packagePath, "slot package") &&
         expect(deleted, "slot deleted") &&
         expect(afterDelete.empty(), "slots empty after delete");
}

bool missingPackageIsRejected() {
  iggy3d::WorldSlotCreateRequest request;
  request.root = testRoot();
  const iggy3d::WorldSlotCreateResult result = iggy3d::createWorldSlot(request);
  return expect(!result.ok, "missing package rejected") &&
         expect(result.reason == "world_package_missing", "missing package reason");
}

}  // namespace

int main() {
  const bool ok = createListAndDeleteRoundTrips() && missingPackageIsRejected();
  return ok ? 0 : 1;
}
