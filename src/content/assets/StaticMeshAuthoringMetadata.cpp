#include "content/assets/StaticMeshAuthoringMetadata.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "cgltf/cgltf.h"

namespace iggy3d {

std::string_view toString(StaticMeshCollisionMode mode) noexcept {
  switch (mode) {
    case StaticMeshCollisionMode::Bounds: return "bounds";
    case StaticMeshCollisionMode::None: return "none";
    case StaticMeshCollisionMode::Convex: return "convex";
    case StaticMeshCollisionMode::Mesh: return "mesh";
    case StaticMeshCollisionMode::Invalid: return "invalid";
  }
  return "invalid";
}

std::string_view toString(StaticMeshAuthoringMetadataStatus status) noexcept {
  switch (status) {
    case StaticMeshAuthoringMetadataStatus::DefaultsApplied:
      return "defaults_applied";
    case StaticMeshAuthoringMetadataStatus::Authored: return "authored";
    case StaticMeshAuthoringMetadataStatus::UnsupportedCollision:
      return "unsupported_collision";
    case StaticMeshAuthoringMetadataStatus::Invalid: return "invalid";
  }
  return "invalid";
}

namespace detail {
namespace {

constexpr std::size_t kMaxExtrasBytes = 4096U;
constexpr std::size_t kMaxExtrasObjectCount = 256U;
constexpr std::size_t kMaxTotalExtrasBytes = 64U * 1024U;
constexpr std::size_t kMaxStringBytes = 64U;
constexpr std::size_t kMaxJsonDepth = 16U;

[[nodiscard]] bool asciiHexDigit(unsigned char value) noexcept {
  return (value >= '0' && value <= '9') ||
         (value >= 'a' && value <= 'f') ||
         (value >= 'A' && value <= 'F');
}

[[nodiscard]] bool asciiIdentifierByte(unsigned char value) noexcept {
  return (value >= 'a' && value <= 'z') ||
         (value >= 'A' && value <= 'Z') ||
         (value >= '0' && value <= '9') || value == '_' || value == '-';
}

class ExtrasReader {
public:
  explicit ExtrasReader(std::string_view source) noexcept : source_(source) {}

  [[nodiscard]] bool parseObject(StaticMeshAuthoringMetadata& metadata,
                                 bool& anyAuthored) noexcept {
    skipWhitespace();
    if (!take('{')) {
      return false;
    }
    bool sawCollision = false;
    bool sawWalkable = false;
    bool sawCategory = false;
    skipWhitespace();
    if (take('}')) {
      return atEnd();
    }

    while (true) {
      std::string key;
      bool keyOverflow = false;
      if (!parseString(&key, keyOverflow)) {
        return false;
      }
      skipWhitespace();
      if (!take(':')) {
        return false;
      }
      skipWhitespace();

      if (keyOverflow) {
        if (!skipValue(0U)) {
          return false;
        }
      } else if (key == "iggy_collision") {
        if (sawCollision || !parseCollision(metadata)) {
          return false;
        }
        sawCollision = true;
        anyAuthored = true;
      } else if (key == "iggy_walkable") {
        if (sawWalkable || !parseWalkable(metadata)) {
          return false;
        }
        sawWalkable = true;
        anyAuthored = true;
      } else if (key == "iggy_category") {
        if (sawCategory || !parseCategory(metadata)) {
          return false;
        }
        sawCategory = true;
        anyAuthored = true;
      } else if (!skipValue(0U)) {
        return false;
      }

      skipWhitespace();
      if (take('}')) {
        return atEnd();
      }
      if (!take(',')) {
        return false;
      }
      skipWhitespace();
    }
  }

private:
  [[nodiscard]] bool atEnd() noexcept {
    skipWhitespace();
    return cursor_ == source_.size();
  }

  void skipWhitespace() noexcept {
    while (cursor_ < source_.size()) {
      const char value = source_[cursor_];
      if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
        break;
      }
      ++cursor_;
    }
  }

  [[nodiscard]] bool take(char expected) noexcept {
    if (cursor_ >= source_.size() || source_[cursor_] != expected) {
      return false;
    }
    ++cursor_;
    return true;
  }

  [[nodiscard]] bool parseString(std::string* output,
                                 bool& overflow) noexcept {
    if (!take('"')) {
      return false;
    }
    while (cursor_ < source_.size()) {
      const unsigned char value =
          static_cast<unsigned char>(source_[cursor_++]);
      if (value == '"') {
        return true;
      }
      if (value < 0x20U) {
        return false;
      }
      char decoded = static_cast<char>(value);
      if (value == '\\') {
        if (cursor_ >= source_.size()) {
          return false;
        }
        const char escape = source_[cursor_++];
        switch (escape) {
          case '"': decoded = '"'; break;
          case '\\': decoded = '\\'; break;
          case '/': decoded = '/'; break;
          case 'b': decoded = '\b'; break;
          case 'f': decoded = '\f'; break;
          case 'n': decoded = '\n'; break;
          case 'r': decoded = '\r'; break;
          case 't': decoded = '\t'; break;
          case 'u':
            if (!skipHexCodepoint()) {
              return false;
            }
            decoded = '?';
            break;
          default: return false;
        }
      }
      if (output != nullptr) {
        if (output->size() == kMaxStringBytes) {
          overflow = true;
        } else if (!overflow) {
          output->push_back(decoded);
        }
      }
    }
    return false;
  }

  [[nodiscard]] bool skipHexCodepoint() noexcept {
    if (source_.size() - cursor_ < 4U) {
      return false;
    }
    for (std::size_t index = 0; index < 4U; ++index) {
      const unsigned char value =
          static_cast<unsigned char>(source_[cursor_++]);
      if (!asciiHexDigit(value)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] bool parseCollision(
      StaticMeshAuthoringMetadata& metadata) noexcept {
    std::string value;
    bool overflow = false;
    if (!parseString(&value, overflow) || overflow) {
      return false;
    }
    StaticMeshCollisionMode mode = StaticMeshCollisionMode::Invalid;
    if (value == "bounds") {
      mode = StaticMeshCollisionMode::Bounds;
    } else if (value == "none") {
      mode = StaticMeshCollisionMode::None;
    } else if (value == "convex") {
      mode = StaticMeshCollisionMode::Convex;
    } else if (value == "mesh") {
      mode = StaticMeshCollisionMode::Mesh;
    } else {
      return false;
    }
    if (metadata.collisionSpecified && metadata.collisionMode != mode) {
      return false;
    }
    metadata.collisionMode = mode;
    metadata.collisionSpecified = true;
    return true;
  }

  [[nodiscard]] bool parseWalkable(
      StaticMeshAuthoringMetadata& metadata) noexcept {
    bool value = false;
    if (source_.substr(cursor_, 4U) == "true") {
      cursor_ += 4U;
      value = true;
    } else if (source_.substr(cursor_, 5U) == "false") {
      cursor_ += 5U;
    } else {
      return false;
    }
    if (metadata.walkableSpecified && metadata.walkable != value) {
      return false;
    }
    metadata.walkable = value;
    metadata.walkableSpecified = true;
    return true;
  }

  [[nodiscard]] bool parseCategory(
      StaticMeshAuthoringMetadata& metadata) noexcept {
    std::string value;
    bool overflow = false;
    if (!parseString(&value, overflow) || overflow || value.empty()) {
      return false;
    }
    for (char character : value) {
      const unsigned char byte = static_cast<unsigned char>(character);
      if (!asciiIdentifierByte(byte)) {
        return false;
      }
    }
    if (!metadata.categoryId.empty() && metadata.categoryId != value) {
      return false;
    }
    metadata.categoryId = std::move(value);
    return true;
  }

  [[nodiscard]] bool skipValue(std::size_t depth) noexcept {
    if (depth >= kMaxJsonDepth || cursor_ >= source_.size()) {
      return false;
    }
    if (source_[cursor_] == '"') {
      bool overflow = false;
      return parseString(nullptr, overflow);
    }
    if (source_[cursor_] == '{') {
      ++cursor_;
      skipWhitespace();
      if (take('}')) {
        return true;
      }
      while (true) {
        bool overflow = false;
        if (!parseString(nullptr, overflow)) {
          return false;
        }
        skipWhitespace();
        if (!take(':')) {
          return false;
        }
        skipWhitespace();
        if (!skipValue(depth + 1U)) {
          return false;
        }
        skipWhitespace();
        if (take('}')) {
          return true;
        }
        if (!take(',')) {
          return false;
        }
        skipWhitespace();
      }
    }
    if (source_[cursor_] == '[') {
      ++cursor_;
      skipWhitespace();
      if (take(']')) {
        return true;
      }
      while (true) {
        if (!skipValue(depth + 1U)) {
          return false;
        }
        skipWhitespace();
        if (take(']')) {
          return true;
        }
        if (!take(',')) {
          return false;
        }
        skipWhitespace();
      }
    }

    if (source_.substr(cursor_, 4U) == "true" ||
        source_.substr(cursor_, 4U) == "null") {
      cursor_ += 4U;
      return true;
    }
    if (source_.substr(cursor_, 5U) == "false") {
      cursor_ += 5U;
      return true;
    }
    return skipNumber();
  }

  [[nodiscard]] bool skipNumber() noexcept {
    if (take('-') && cursor_ == source_.size()) {
      return false;
    }
    if (take('0')) {
      if (cursor_ < source_.size() && source_[cursor_] >= '0' &&
          source_[cursor_] <= '9') {
        return false;
      }
    } else {
      const std::size_t integerStart = cursor_;
      while (cursor_ < source_.size() && source_[cursor_] >= '0' &&
             source_[cursor_] <= '9') {
        ++cursor_;
      }
      if (cursor_ == integerStart) {
        return false;
      }
    }
    if (take('.')) {
      const std::size_t fractionStart = cursor_;
      while (cursor_ < source_.size() && source_[cursor_] >= '0' &&
             source_[cursor_] <= '9') {
        ++cursor_;
      }
      if (cursor_ == fractionStart) {
        return false;
      }
    }
    if (cursor_ < source_.size() &&
        (source_[cursor_] == 'e' || source_[cursor_] == 'E')) {
      ++cursor_;
      if (cursor_ < source_.size() &&
          (source_[cursor_] == '+' || source_[cursor_] == '-')) {
        ++cursor_;
      }
      const std::size_t exponentStart = cursor_;
      while (cursor_ < source_.size() && source_[cursor_] >= '0' &&
             source_[cursor_] <= '9') {
        ++cursor_;
      }
      if (cursor_ == exponentStart) {
        return false;
      }
    }
    return true;
  }

  std::string_view source_;
  std::size_t cursor_ = 0U;
};

[[nodiscard]] StaticMeshAuthoringMetadata invalidMetadata() {
  StaticMeshAuthoringMetadata metadata;
  metadata.collisionMode = StaticMeshCollisionMode::Invalid;
  metadata.status = StaticMeshAuthoringMetadataStatus::Invalid;
  metadata.reasonCode = "static_mesh_authoring_metadata_invalid";
  return metadata;
}

[[nodiscard]] bool appendExtras(std::vector<std::string_view>& output,
                                const cgltf_extras& extras) {
  if (extras.data != nullptr && extras.data[0] != '\0') {
    if (output.size() == kMaxExtrasObjectCount) {
      return false;
    }
    output.emplace_back(extras.data);
  }
  return true;
}

}  // namespace

StaticMeshAuthoringMetadata parseStaticMeshAuthoringMetadata(
    std::span<const std::string_view> extrasObjects) noexcept {
  StaticMeshAuthoringMetadata metadata;
  bool anyAuthored = false;
  std::size_t totalBytes = 0U;
  if (extrasObjects.size() > kMaxExtrasObjectCount) {
    return invalidMetadata();
  }
  for (std::string_view extras : extrasObjects) {
    if (extras.size() > kMaxExtrasBytes ||
        totalBytes > kMaxTotalExtrasBytes - extras.size()) {
      return invalidMetadata();
    }
    totalBytes += extras.size();
    ExtrasReader reader(extras);
    if (!reader.parseObject(metadata, anyAuthored)) {
      return invalidMetadata();
    }
  }

  if (metadata.walkable &&
      metadata.collisionMode == StaticMeshCollisionMode::None) {
    return invalidMetadata();
  }
  if (metadata.collisionMode == StaticMeshCollisionMode::Convex ||
      metadata.collisionMode == StaticMeshCollisionMode::Mesh) {
    metadata.status =
        StaticMeshAuthoringMetadataStatus::UnsupportedCollision;
    metadata.reasonCode = "static_mesh_collision_mode_unsupported";
    return metadata;
  }
  if (anyAuthored) {
    metadata.status = StaticMeshAuthoringMetadataStatus::Authored;
    metadata.reasonCode = "static_mesh_authoring_metadata_authored";
  }
  return metadata;
}

StaticMeshAuthoringMetadata importStaticMeshAuthoringMetadata(
    const cgltf_data& data) {
  std::vector<std::string_view> extras;
  extras.reserve(32U);
  if (!appendExtras(extras, data.extras) ||
      !appendExtras(extras, data.asset.extras)) {
    return invalidMetadata();
  }
  for (cgltf_size index = 0; index < data.nodes_count; ++index) {
    if (!appendExtras(extras, data.nodes[index].extras)) {
      return invalidMetadata();
    }
  }
  for (cgltf_size index = 0; index < data.meshes_count; ++index) {
    if (!appendExtras(extras, data.meshes[index].extras)) {
      return invalidMetadata();
    }
  }
  return parseStaticMeshAuthoringMetadata(extras);
}

}  // namespace detail
}  // namespace iggy3d
