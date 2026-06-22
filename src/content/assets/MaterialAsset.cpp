#include "content/assets/MaterialAsset.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <sstream>

namespace iggy3d {
namespace {

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

}  // namespace

MaterialAssetParseResult parseMaterialAssetText(const std::string& text) {
  MaterialAssetParseResult result;
  MaterialAssetRecord* current = nullptr;
  std::istringstream input(text);
  std::string rawLine;
  while (std::getline(input, rawLine)) {
    const std::string line = stripComment(rawLine);
    if (line.empty()) {
      continue;
    }
    if (line == "[[materials]]") {
      result.library.materials.push_back({});
      current = &result.library.materials.back();
      continue;
    }
    if (line.starts_with("[")) {
      result.reason = "material_unsupported_table";
      return result;
    }
    if (current == nullptr) {
      result.reason = "material_key_outside_table";
      return result;
    }
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      result.reason = "material_parse_failed";
      return result;
    }
    const std::string key(trim(std::string_view(line).substr(0, equals)));
    const std::string_view value = trim(std::string_view(line).substr(equals + 1U));
    bool ok = false;
    if (key == "id") {
      ok = parseString(value, current->id);
    } else if (key == "kind") {
      ok = parseString(value, current->kind);
    } else if (key == "color") {
      ok = parseVec3(value, current->color);
    } else {
      result.reason = "material_unsupported_key";
      return result;
    }
    if (!ok) {
      result.reason = "material_invalid_value";
      return result;
    }
  }
  if (result.library.materials.empty()) {
    result.reason = "material_empty";
    return result;
  }
  for (const MaterialAssetRecord& material : result.library.materials) {
    if (material.id.empty() || material.kind != "vertex_color") {
      result.reason = "material_invalid_record";
      return result;
    }
  }
  result.ok = true;
  result.reason = "material_asset_ok";
  return result;
}

}  // namespace iggy3d
