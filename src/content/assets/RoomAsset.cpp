#include "content/assets/RoomAsset.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
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

bool parseVec3Feet(std::string_view value, Vec3& out) {
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
  out = {parsed[0] * kFeetToMeters, parsed[1] * kFeetToMeters, parsed[2] * kFeetToMeters};
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

}  // namespace

RoomAssetParseResult parseRoomAssetText(const std::string& text) {
  RoomAssetParseResult result;
  RoomTable table = RoomTable::None;
  RoomStaticMeshAsset* staticMesh = nullptr;
  RoomAnchorAsset* anchor = nullptr;
  RoomOpeningAsset* opening = nullptr;

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
      continue;
    }
    if (line == "[conversion]") {
      table = RoomTable::Conversion;
      staticMesh = nullptr;
      anchor = nullptr;
      opening = nullptr;
      continue;
    }
    if (line == "[[static_meshes]]") {
      table = RoomTable::StaticMesh;
      result.room.staticMeshes.push_back({});
      staticMesh = &result.room.staticMeshes.back();
      anchor = nullptr;
      opening = nullptr;
      continue;
    }
    if (line == "[[anchors]]") {
      table = RoomTable::Anchor;
      result.room.anchors.push_back({});
      anchor = &result.room.anchors.back();
      staticMesh = nullptr;
      opening = nullptr;
      continue;
    }
    if (line == "[[openings]]") {
      table = RoomTable::Opening;
      result.room.openings.push_back({});
      opening = &result.room.openings.back();
      staticMesh = nullptr;
      anchor = nullptr;
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
    } else {
      result.reason = "room_key_outside_table";
      return result;
    }
    if (!ok) {
      result.reason = "room_invalid_value";
      return result;
    }
  }

  if (result.room.id.empty() || result.room.sourceFile.empty() ||
      result.room.sourceSubset != "spawn_room_corridor_stub" ||
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
  result.ok = true;
  result.reason = "room_asset_ok";
  return result;
}

}  // namespace iggy3d
