#include "content/assets/RoomAsset.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <sstream>

namespace iggy3d {
namespace {

constexpr float kFeetToMeters = 0.3048F;

enum class RoomTable {
  None,
  Room,
  Conversion,
  StaticMesh,
  Anchor,
  Opening,
  SpatialSurface,
};

std::string_view trim(std::string_view value) {
  while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

std::string stripComment(std::string_view value) {
  bool inString = false;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '"') {
      inString = !inString;
    } else if (value[index] == '#' && !inString) {
      return std::string(trim(value.substr(0, index)));
    }
  }
  return std::string(trim(value));
}

bool parseString(std::string_view value, std::string& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '"' || value.back() != '"') {
    return false;
  }
  out = std::string(value.substr(1U, value.size() - 2U));
  return true;
}

bool parseU32(std::string_view value, std::uint32_t& out) {
  value = trim(value);
  if (value.empty() || value.front() == '-') {
    return false;
  }
  std::uint64_t parsed = 0;
  for (const char c : value) {
    if (c < '0' || c > '9') {
      return false;
    }
    parsed = parsed * 10U + static_cast<std::uint64_t>(c - '0');
    if (parsed > UINT32_MAX) {
      return false;
    }
  }
  out = static_cast<std::uint32_t>(parsed);
  return true;
}

bool parseFloat(std::string_view value, float& out) {
  value = trim(value);
  std::string text(value);
  char* end = nullptr;
  out = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && std::isfinite(out);
}

bool parseVec3(std::string_view value, Vec3& out) {
  value = trim(value);
  if (value.size() < 5U || value.front() != '[' || value.back() != ']') {
    return false;
  }
  value.remove_prefix(1);
  value.remove_suffix(1);
  float parsed[3]{};
  for (std::uint32_t index = 0; index < 3U; ++index) {
    const std::size_t comma = index == 2U ? std::string_view::npos : value.find(',');
    const std::string_view token = comma == std::string_view::npos ? value : value.substr(0, comma);
    if (!parseFloat(token, parsed[index])) {
      return false;
    }
    if (comma != std::string_view::npos) {
      value.remove_prefix(comma + 1U);
    }
  }
  if (value.find(',') != std::string_view::npos) {
    return false;
  }
  out = {parsed[0], parsed[1], parsed[2]};
  return true;
}

bool parseVec3Feet(std::string_view value, Vec3& out) {
  Vec3 feet;
  if (!parseVec3(value, feet)) {
    return false;
  }
  out = {feet.x * kFeetToMeters, feet.y * kFeetToMeters, feet.z * kFeetToMeters};
  return true;
}

bool parseFeet(std::string_view value, float& outMeters) {
  float feet = 0.0F;
  if (!parseFloat(value, feet)) {
    return false;
  }
  outMeters = feet * kFeetToMeters;
  return true;
}

bool parseBool(std::string_view value, bool& out) {
  value = trim(value);
  if (value == "true") {
    out = true;
    return true;
  }
  if (value == "false") {
    out = false;
    return true;
  }
  return false;
}

bool parseStringArray(std::string_view value, std::vector<std::string>& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '[' || value.back() != ']') {
    return false;
  }
  value.remove_prefix(1);
  value.remove_suffix(1);
  out.clear();
  while (!trim(value).empty()) {
    value = trim(value);
    std::string item;
    const std::size_t endQuote = value.find('"', 1U);
    if (endQuote == std::string_view::npos ||
        !parseString(value.substr(0, endQuote + 1U), item)) {
      return false;
    }
    out.push_back(item);
    value.remove_prefix(endQuote + 1U);
    value = trim(value);
    if (value.empty()) {
      break;
    }
    if (value.front() != ',') {
      return false;
    }
    value.remove_prefix(1);
  }
  return true;
}

bool parsePointsFeet(std::string_view value, std::vector<Vec3>& out) {
  value = trim(value);
  if (value.size() < 7U || value.front() != '[' || value.back() != ']') {
    return false;
  }
  value.remove_prefix(1);
  value.remove_suffix(1);
  out.clear();
  while (!trim(value).empty()) {
    value = trim(value);
    if (value.front() != '[') {
      return false;
    }
    const std::size_t pointEnd = value.find(']');
    if (pointEnd == std::string_view::npos) {
      return false;
    }
    Vec3 point;
    if (!parseVec3Feet(value.substr(0, pointEnd + 1U), point)) {
      return false;
    }
    out.push_back(point);
    value.remove_prefix(pointEnd + 1U);
    value = trim(value);
    if (value.empty()) {
      break;
    }
    if (value.front() != ',') {
      return false;
    }
    value.remove_prefix(1);
  }
  return out.size() == 3U || out.size() == 4U;
}

bool parseShape(std::string_view value, RoomSpatialSurfaceShape& out) {
  std::string text;
  if (!parseString(value, text)) {
    return false;
  }
  if (text == "box") {
    out = RoomSpatialSurfaceShape::Box;
    return true;
  }
  if (text == "plane") {
    out = RoomSpatialSurfaceShape::Plane;
    return true;
  }
  if (text == "opening") {
    out = RoomSpatialSurfaceShape::Opening;
    return true;
  }
  return false;
}

bool parseRole(std::string_view value, RoomSpatialSurfaceRole& out) {
  std::string text;
  if (!parseString(value, text)) {
    return false;
  }
  if (text == "walkable") {
    out = RoomSpatialSurfaceRole::Walkable;
    return true;
  }
  if (text == "blocker") {
    out = RoomSpatialSurfaceRole::Blocker;
    return true;
  }
  if (text == "projectile_blocker") {
    out = RoomSpatialSurfaceRole::ProjectileBlocker;
    return true;
  }
  if (text == "opening") {
    out = RoomSpatialSurfaceRole::Opening;
    return true;
  }
  return false;
}

bool containsString(const std::vector<std::string>& values, std::string_view expected) {
  for (const std::string& value : values) {
    if (value == expected) {
      return true;
    }
  }
  return false;
}

bool hasDuplicateStrings(const std::vector<std::string>& values) {
  std::set<std::string> seen;
  for (const std::string& value : values) {
    if (!seen.insert(value).second) {
      return true;
    }
  }
  return false;
}

bool validTraversalTag(std::string_view tag) {
  return tag == "walkable" || tag == "blocker" || tag == "projectile_blocker" ||
         tag == "opening" || tag == "clamber" || tag == "no_player" ||
         tag == "debug_only";
}

bool validCollisionMask(std::string_view mask) {
  return mask == "actor" || mask == "projectile" || mask == "sight";
}

bool hasStaticMesh(const RoomAsset& room, std::string_view id) {
  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.id == id) {
      return true;
    }
  }
  return false;
}

bool hasOpening(const RoomAsset& room, std::string_view id) {
  for (const RoomOpeningAsset& opening : room.openings) {
    if (opening.id == id) {
      return true;
    }
  }
  return false;
}

}  // namespace

RoomAssetParseResult parseRoomAssetText(const std::string& text) {
  RoomAssetParseResult result;
  RoomTable table = RoomTable::None;
  RoomStaticMeshAsset* staticMesh = nullptr;
  RoomAnchorAsset* anchor = nullptr;
  RoomOpeningAsset* opening = nullptr;
  RoomSpatialSurface* spatialSurface = nullptr;

  std::istringstream input(text);
  std::string rawLine;
  while (std::getline(input, rawLine)) {
    const std::string line = stripComment(rawLine);
    if (line.empty()) {
      continue;
    }
    if (line == "[room]") {
      table = RoomTable::Room;
      staticMesh = nullptr;
      anchor = nullptr;
      opening = nullptr;
      spatialSurface = nullptr;
      continue;
    }
    if (line == "[conversion]") {
      table = RoomTable::Conversion;
      staticMesh = nullptr;
      anchor = nullptr;
      opening = nullptr;
      spatialSurface = nullptr;
      continue;
    }
    if (line == "[[static_meshes]]") {
      table = RoomTable::StaticMesh;
      result.room.staticMeshes.push_back({});
      staticMesh = &result.room.staticMeshes.back();
      anchor = nullptr;
      opening = nullptr;
      spatialSurface = nullptr;
      continue;
    }
    if (line == "[[anchors]]") {
      table = RoomTable::Anchor;
      result.room.anchors.push_back({});
      anchor = &result.room.anchors.back();
      staticMesh = nullptr;
      opening = nullptr;
      spatialSurface = nullptr;
      continue;
    }
    if (line == "[[openings]]") {
      table = RoomTable::Opening;
      result.room.openings.push_back({});
      opening = &result.room.openings.back();
      staticMesh = nullptr;
      anchor = nullptr;
      spatialSurface = nullptr;
      continue;
    }
    if (line == "[[spatial_surfaces]]") {
      table = RoomTable::SpatialSurface;
      result.room.spatialSurfaces.push_back({});
      spatialSurface = &result.room.spatialSurfaces.back();
      staticMesh = nullptr;
      anchor = nullptr;
      opening = nullptr;
      continue;
    }
    if (line.starts_with("[")) {
      result.reason = "room_unsupported_table";
      return result;
    }

    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      result.reason = "room_parse_failed";
      return result;
    }
    const std::string key(trim(std::string_view(line).substr(0, equals)));
    const std::string_view value = trim(std::string_view(line).substr(equals + 1U));
    bool ok = true;
    if (table == RoomTable::Room) {
      if (key == "id") {
        ok = parseString(value, result.room.id);
      } else if (key == "version") {
        ok = parseU32(value, result.room.version);
      } else if (key == "units") {
        ok = parseString(value, result.room.units);
      } else if (key == "source") {
        ok = parseString(value, result.room.source);
      } else if (key == "source_file") {
        ok = parseString(value, result.room.sourceFile);
      } else if (key == "source_subset") {
        ok = parseString(value, result.room.sourceSubset);
      } else {
        result.reason = "room_unsupported_key";
        return result;
      }
    } else if (table == RoomTable::Conversion) {
      continue;
    } else if (table == RoomTable::StaticMesh && staticMesh != nullptr) {
      if (key == "id") {
        ok = parseString(value, staticMesh->id);
      } else if (key == "mesh") {
        ok = parseString(value, staticMesh->meshId);
      } else if (key == "material") {
        ok = parseString(value, staticMesh->materialId);
      } else if (key == "role") {
        ok = parseString(value, staticMesh->role);
      } else if (key == "position_ft") {
        ok = parseVec3Feet(value, staticMesh->positionMeters);
      } else if (key == "size_ft") {
        ok = parseVec3Feet(value, staticMesh->sizeMeters);
      } else {
        result.reason = "room_unsupported_key";
        return result;
      }
    } else if (table == RoomTable::Anchor && anchor != nullptr) {
      if (key == "id") {
        ok = parseString(value, anchor->id);
      } else if (key == "kind") {
        ok = parseString(value, anchor->kind);
      } else if (key == "runtime_stable_name") {
        ok = parseString(value, anchor->runtimeStableName);
      } else if (key == "position_ft") {
        ok = parseVec3Feet(value, anchor->positionMeters);
      } else {
        result.reason = "room_unsupported_key";
        return result;
      }
    } else if (table == RoomTable::Opening && opening != nullptr) {
      if (key == "id") {
        ok = parseString(value, opening->id);
      } else if (key == "edge") {
        ok = parseString(value, opening->edge);
      } else if (key == "kind") {
        ok = parseString(value, opening->kind);
      } else if (key == "offset_ft") {
        ok = parseFeet(value, opening->offsetMeters);
      } else if (key == "width_ft") {
        ok = parseFeet(value, opening->widthMeters);
      } else {
        result.reason = "room_unsupported_key";
        return result;
      }
    } else if (table == RoomTable::SpatialSurface && spatialSurface != nullptr) {
      if (key == "id") {
        ok = parseString(value, spatialSurface->id);
      } else if (key == "source_static_mesh") {
        ok = parseString(value, spatialSurface->sourceStaticMeshId);
      } else if (key == "shape") {
        ok = parseShape(value, spatialSurface->shape);
      } else if (key == "role") {
        ok = parseRole(value, spatialSurface->role);
      } else if (key == "points_ft") {
        ok = parsePointsFeet(value, spatialSurface->pointsMeters);
        if (!ok) {
          result.reason = "room_invalid_spatial_surface_point";
          return result;
        }
      } else if (key == "normal") {
        ok = parseVec3(value, spatialSurface->normal);
      } else if (key == "traversal_tags") {
        ok = parseStringArray(value, spatialSurface->traversalTags);
      } else if (key == "collision_mask") {
        ok = parseStringArray(value, spatialSurface->collisionMask);
      } else if (key == "blocks_actor") {
        ok = parseBool(value, spatialSurface->blocksActor);
      } else if (key == "blocks_projectile") {
        ok = parseBool(value, spatialSurface->blocksProjectile);
      } else if (key == "opening_id") {
        ok = parseString(value, spatialSurface->openingId);
      } else {
        result.reason = "room_unsupported_key";
        return result;
      }
    } else {
      result.reason = "room_key_outside_table";
      return result;
    }
    if (!ok) {
      result.reason = "room_invalid_value";
      return result;
    }
  }

  if (result.room.id.empty() || result.room.sourceFile.empty() || result.room.sourceSubset.empty() ||
      result.room.staticMeshes.empty() || result.room.anchors.empty()) {
    result.reason = "room_missing_required";
    return result;
  }
  for (const RoomStaticMeshAsset& mesh : result.room.staticMeshes) {
    if (mesh.id.empty() || mesh.meshId.empty() || mesh.materialId.empty() ||
        mesh.role.empty() || mesh.sizeMeters.x <= 0.0F || mesh.sizeMeters.y <= 0.0F ||
        mesh.sizeMeters.z <= 0.0F) {
      result.reason = "room_invalid_static_mesh";
      return result;
    }
  }
  for (const RoomAnchorAsset& item : result.room.anchors) {
    if (item.id.empty() || item.kind.empty()) {
      result.reason = "room_invalid_anchor";
      return result;
    }
  }
  for (std::size_t index = 0; index < result.room.spatialSurfaces.size(); ++index) {
    RoomSpatialSurface& surface = result.room.spatialSurfaces[index];
    if (surface.id.empty()) {
      result.reason = "room_invalid_spatial_surface";
      return result;
    }
    for (std::size_t other = 0; other < index; ++other) {
      if (result.room.spatialSurfaces[other].id == surface.id) {
        result.reason = "room_duplicate_spatial_surface_id";
        return result;
      }
    }
    if (surface.sourceStaticMeshId.empty() ||
        !hasStaticMesh(result.room, surface.sourceStaticMeshId)) {
      result.reason = "room_unknown_spatial_surface_mesh";
      return result;
    }
    if (surface.pointsMeters.size() != 3U && surface.pointsMeters.size() != 4U) {
      result.reason = "room_invalid_spatial_surface_point";
      return result;
    }
    for (Vec3 point : surface.pointsMeters) {
      if (!isFinite(point)) {
        result.reason = "room_invalid_spatial_surface_point";
        return result;
      }
    }
    const float normalLengthSquared = lengthSquared(surface.normal);
    if (!isFinite(surface.normal) || normalLengthSquared <= 0.000001F) {
      result.reason = "room_invalid_spatial_surface_normal";
      return result;
    }
    const float normalLength = std::sqrt(normalLengthSquared);
    surface.normal = surface.normal / normalLength;
    if (hasDuplicateStrings(surface.traversalTags) || hasDuplicateStrings(surface.collisionMask)) {
      result.reason = "room_unknown_traversal_tag";
      return result;
    }
    for (const std::string& tag : surface.traversalTags) {
      if (!validTraversalTag(tag)) {
        result.reason = "room_unknown_traversal_tag";
        return result;
      }
    }
    for (const std::string& mask : surface.collisionMask) {
      if (!validCollisionMask(mask)) {
        result.reason = "room_unknown_traversal_tag";
        return result;
      }
    }
    if (surface.role == RoomSpatialSurfaceRole::Walkable &&
        (!containsString(surface.traversalTags, "walkable") || surface.blocksActor ||
         surface.normal.y <= 0.0F)) {
      result.reason = "room_invalid_spatial_surface";
      return result;
    }
    if (surface.role == RoomSpatialSurfaceRole::Blocker &&
        (!containsString(surface.traversalTags, "blocker") || !surface.blocksActor)) {
      result.reason = "room_invalid_spatial_surface";
      return result;
    }
    if (surface.role == RoomSpatialSurfaceRole::ProjectileBlocker &&
        (!containsString(surface.traversalTags, "projectile_blocker") ||
         !surface.blocksProjectile)) {
      result.reason = "room_invalid_spatial_surface";
      return result;
    }
    if (surface.role == RoomSpatialSurfaceRole::Opening) {
      if (!containsString(surface.traversalTags, "opening") || surface.blocksActor ||
          surface.blocksProjectile) {
        result.reason = "room_invalid_opening_surface";
        return result;
      }
      if (surface.openingId.empty() || !hasOpening(result.room, surface.openingId)) {
        result.reason = "room_unknown_spatial_surface_opening";
        return result;
      }
    } else if (!surface.openingId.empty()) {
      result.reason = "room_invalid_opening_surface";
      return result;
    }
  }
  result.ok = true;
  result.reason = "room_asset_ok";
  return result;
}

}  // namespace iggy3d
