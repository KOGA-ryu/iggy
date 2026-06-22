#pragma once

#include <string>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct MeshAssetPrimitive {
  std::string id;
  std::string kind;
  Vec3 sizeMeters;
  std::string materialId;
};

struct MeshAssetLibrary {
  std::vector<MeshAssetPrimitive> primitives;
};

struct MeshAssetParseResult {
  bool ok = false;
  std::string reason = "mesh_parse_failed";
  MeshAssetLibrary library;
};

MeshAssetParseResult parseMeshAssetText(const std::string& text);

}  // namespace iggy3d
