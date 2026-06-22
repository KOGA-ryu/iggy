#include "content/assets/MeshAsset.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <sstream>

namespace iggy3d {
namespace {

constexpr float kFeetToMeters = 0.3048F;

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

}  // namespace

MeshAssetParseResult parseMeshAssetText(const std::string& text) {
  MeshAssetParseResult result;
  MeshAssetPrimitive* current = nullptr;
  std::istringstream input(text);
  std::string rawLine;
  while (std::getline(input, rawLine)) {
    const std::string line = stripComment(rawLine);
    if (line.empty()) {
      continue;
    }
    if (line == "[[primitive_meshes]]") {
      result.library.primitives.push_back({});
      current = &result.library.primitives.back();
      continue;
    }
    if (line.starts_with("[")) {
      result.reason = "mesh_unsupported_table";
      return result;
    }
    if (current == nullptr) {
      result.reason = "mesh_key_outside_table";
      return result;
    }
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      result.reason = "mesh_parse_failed";
      return result;
    }
    const std::string key(trim(std::string_view(line).substr(0, equals)));
    const std::string_view value = trim(std::string_view(line).substr(equals + 1U));
    bool ok = false;
    if (key == "id") {
      ok = parseString(value, current->id);
    } else if (key == "kind") {
      ok = parseString(value, current->kind);
    } else if (key == "size_ft") {
      ok = parseVec3Feet(value, current->sizeMeters);
    } else if (key == "material") {
      ok = parseString(value, current->materialId);
    } else {
      result.reason = "mesh_unsupported_key";
      return result;
    }
    if (!ok) {
      result.reason = "mesh_invalid_value";
      return result;
    }
  }
  if (result.library.primitives.empty()) {
    result.reason = "mesh_empty";
    return result;
  }
  for (const MeshAssetPrimitive& primitive : result.library.primitives) {
    if (primitive.id.empty() || primitive.kind != "box" || primitive.materialId.empty() ||
        primitive.sizeMeters.x <= 0.0F || primitive.sizeMeters.y <= 0.0F ||
        primitive.sizeMeters.z <= 0.0F) {
      result.reason = "mesh_invalid_record";
      return result;
    }
  }
  result.ok = true;
  result.reason = "mesh_asset_ok";
  return result;
}

}  // namespace iggy3d
