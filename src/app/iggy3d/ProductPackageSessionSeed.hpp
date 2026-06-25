#pragma once

#include <cstddef>
#include <string>

#include "content/FixtureScenarioLoader.hpp"
#include "content/PackageLoader.hpp"

namespace iggy3d {

struct ProductPackageSessionSeedResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  FixtureScenarioSeed seed;
  bool synthesizedFromRoomAnchors = false;
  std::string sourceRoomId;
  std::size_t roomCount = 0;
  std::size_t anchorCount = 0;
  std::size_t playerCount = 0;
  std::size_t entityCount = 0;
  std::size_t objectiveCount = 0;
};

ProductPackageSessionSeedResult buildProductPackageSessionSeed(
    const PackageLoadResult& package);

}  // namespace iggy3d
