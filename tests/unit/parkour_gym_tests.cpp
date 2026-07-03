// M-LAB s2 -- the parkour gym: two demo packages (earth + giant) on BYTE-IDENTICAL geometry, the
// stations deriving the traversal affordances the designer will feel against. Content-only: these
// tests load the authored packages and assert derivation + the earth/giant profile contrast + the
// geometry-parity contract.

#include "content/PackageLoader.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"
#include "runtime/session/Session.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

constexpr std::string_view kEarthPackage =
    "fixtures/demos/parkour_gym_earth/package.iggy3d.toml";
constexpr std::string_view kEarthRoom =
    "fixtures/demos/parkour_gym_earth/assets/rooms/parkour_gym.room.iggy3d.toml";

std::size_t slotKindCount(const iggy3d::MovementTraversalSlotRegistry& registry,
                          iggy3d::MovementTraversalSlotKind kind) {
  std::size_t count = 0;
  for (const iggy3d::MovementTraversalSlot& slot : registry.slots) {
    if (slot.kind == kind) {
      ++count;
    }
  }
  return count;
}

const iggy3d::MovementTraversalSlot* findSlotBySource(
    const iggy3d::MovementTraversalSlotRegistry& registry, std::string_view sourceMeshId) {
  for (const iggy3d::MovementTraversalSlot& slot : registry.slots) {
    if (slot.sourceStaticMeshId == sourceMeshId) {
      return &slot;
    }
  }
  return nullptr;
}

std::string readFile(std::string_view path) {
  std::ifstream file{std::string(path), std::ios::binary};
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Load a gym package + build its slot registry from the (single) authored room.
bool loadGym(std::string_view packagePath, iggy3d::PackageLoadResult& packageOut,
             iggy3d::MovementTraversalSlotRegistry& registryOut) {
  packageOut = iggy3d::loadPackage({std::string(packagePath)});
  if (packageOut.status != iggy3d::PackageLoadStatus::Ok || packageOut.rooms.empty()) {
    return false;
  }
  registryOut = iggy3d::buildMovementTraversalSlotRegistry(packageOut.rooms.front(), {});
  return true;
}

bool earthGymLoadsCreatesAndActivates() {
  const iggy3d::PackageLoadResult package = iggy3d::loadPackage({std::string(kEarthPackage)});
  bool ok = expect(package.status == iggy3d::PackageLoadStatus::Ok, "earth gym package loads") &&
            expect(!package.rooms.empty(), "earth gym ships a room");

  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  ok = ok && expect(created.status == iggy3d::ResultStatus::Ok, "earth gym session creates");
  if (created.status == iggy3d::ResultStatus::Ok) {
    // A tick proves the created session is live (the activation smoke).
    ok = ok && expect(created.value.tick().status == iggy3d::ResultStatus::Ok,
                      "earth gym session ticks");
  }
  return ok;
}

bool earthGymDerivesStationSlots() {
  iggy3d::PackageLoadResult package;
  iggy3d::MovementTraversalSlotRegistry registry;
  if (!expect(loadGym(kEarthPackage, package, registry), "earth gym derives slots")) {
    return false;
  }

  bool ok =
      expect(slotKindCount(registry, iggy3d::MovementTraversalSlotKind::Clamber) == 5U,
             "clamber ladder derives 5 clamber slots") &&
      expect(slotKindCount(registry, iggy3d::MovementTraversalSlotKind::Vault) == 4U,
             "vault rails derive 4 vault slots") &&
      expect(slotKindCount(registry, iggy3d::MovementTraversalSlotKind::WireWalk) == 3U,
             "wire run derives 3 wire slots");

  // The top PASSING rung (5.75 ft = 1.753 m) is clamber_high (off the 1.80 m boundary).
  const iggy3d::MovementTraversalSlot* highRung = findSlotBySource(registry, "clamber_rung_high");
  ok = ok && expect(highRung != nullptr && highRung->heightBand == "clamber_high",
                    "the 1.75 m rung is clamber_high (passes, off boundary)");

  // The over-max wall (7.5 ft = 2.286 m) derives a slot but sits in the blocked_high band that a
  // clamber attempt never selects -- feeling the ceiling.
  const iggy3d::MovementTraversalSlot* blocked = findSlotBySource(registry, "clamber_rung_blocked");
  ok = ok && expect(blocked != nullptr && blocked->heightBand == "blocked_high",
                    "the 2.2 m over-max wall is blocked_high");
  return ok;
}

bool earthGymResolvesEarthProfile() {
  const iggy3d::PackageLoadResult package = iggy3d::loadPackage({std::string(kEarthPackage)});
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "earth gym loads for profile check")) {
    return false;
  }
  return expect(package.scenario.movementProfile.id == "earth_standard",
                "earth gym resolves earth_standard") &&
         expect(package.scenario.config.movementDistanceMeters == 3.0F,
                "earth gym admission limit is 3.0 (from the profile)");
}

}  // namespace

int main() {
  const bool ok = earthGymLoadsCreatesAndActivates() && earthGymDerivesStationSlots() &&
                  earthGymResolvesEarthProfile();
  // Referenced so GATE 2 (giant + parity) can drop in without an unused-warning churn.
  (void)&readFile;
  (void)&kEarthRoom;
  return ok ? 0 : 1;
}
