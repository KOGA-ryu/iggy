#pragma once

#include <string>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct MaterialAssetRecord {
  std::string id;
  std::string kind;
  Vec3 color;
};

struct MaterialAssetLibrary {
  std::vector<MaterialAssetRecord> materials;
};

struct MaterialAssetParseResult {
  bool ok = false;
  std::string reason = "material_parse_failed";
  MaterialAssetLibrary library;
};

MaterialAssetParseResult parseMaterialAssetText(const std::string& text);

}  // namespace iggy3d
