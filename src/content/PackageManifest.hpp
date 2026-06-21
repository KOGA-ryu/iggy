#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d {

struct PackageAssetRef {
  std::string id;
  std::string path;
};

struct PackageManifest {
  std::string packageId;
  std::uint32_t schemaVersion = 1;
  std::uint32_t requiredRuntimeSchema = 1;
  std::string scenarioPath;
  std::vector<PackageAssetRef> assets;
};

}  // namespace iggy3d
