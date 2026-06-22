#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct RoomStaticMeshAsset {
  std::string id;
  std::string meshId;
  std::string materialId;
  std::string role;
  Vec3 positionMeters;
  Vec3 sizeMeters;
};

struct RoomAnchorAsset {
  std::string id;
  std::string kind;
  std::string runtimeStableName;
  Vec3 positionMeters;
};

struct RoomOpeningAsset {
  std::string id;
  std::string edge;
  std::string kind;
  float offsetMeters = 0.0F;
  float widthMeters = 0.0F;
};

struct RoomAsset {
  std::string id;
  std::uint32_t version = 1;
  std::string units;
  std::string source;
  std::string sourceFile;
  std::string sourceSubset;
  std::vector<RoomStaticMeshAsset> staticMeshes;
  std::vector<RoomAnchorAsset> anchors;
  std::vector<RoomOpeningAsset> openings;
};

struct RoomAssetParseResult {
  bool ok = false;
  std::string reason = "room_parse_failed";
  RoomAsset room;
};

RoomAssetParseResult parseRoomAssetText(const std::string& text);

}  // namespace iggy3d
