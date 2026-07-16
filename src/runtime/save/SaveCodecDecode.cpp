#include "runtime/save/SaveCodecReader.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

#include "runtime/save/SaveCodecFormat.hpp"

namespace iggy3d {

// branch-gate-relocation: BG-1237 from=src/runtime/save/SaveCodec.cpp
namespace {

std::string sectionForKey(const std::string& key) {
  const std::size_t dot = key.find('.');
  return dot == std::string::npos ? key : key.substr(0, dot);
}

bool hexDigit(char c, int& value) {
  if (c >= '0' && c <= '9') {
    value = c - '0';
    return true;
  }
  if (c >= 'A' && c <= 'F') {
    value = 10 + c - 'A';
    return true;
  }
  return false;
}

bool unescapeString(std::string_view value, std::string& out) {
  out.clear();
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] != '%') {
      out.push_back(value[index]);
      continue;
    }
    if (index + 2U >= value.size()) {
      return false;
    }
    int high = 0;
    int low = 0;
    if (!hexDigit(value[index + 1U], high) || !hexDigit(value[index + 2U], low)) {
      return false;
    }
    const int byte = high * 16 + low;
    if (byte == 0x25) {
      out.push_back('%');
    } else if (byte == 0x0A) {
      out.push_back('\n');
    } else if (byte == 0x0D) {
      out.push_back('\r');
    } else if (byte == 0x3D) {
      out.push_back('=');
    } else {
      return false;
    }
    index += 2U;
  }
  return true;
}

bool parseFloat(std::string_view value, float& out) {
  std::string text(value);
  char* end = nullptr;
  out = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && std::isfinite(out);
}

bool parseDouble(std::string_view value, double& out) {
  std::string text(value);
  char* end = nullptr;
  out = std::strtod(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && std::isfinite(out);
}

bool parseVec3(std::string_view value, Vec3& out) {
  const std::size_t first = value.find(',');
  const std::size_t second = first == std::string_view::npos ? std::string_view::npos
                                                             : value.find(',', first + 1U);
  if (first == std::string_view::npos || second == std::string_view::npos ||
      value.find(',', second + 1U) != std::string_view::npos) {
    return false;
  }
  return parseFloat(value.substr(0, first), out.x) &&
         parseFloat(value.substr(first + 1U, second - first - 1U), out.y) &&
         parseFloat(value.substr(second + 1U), out.z);
}

bool parseCreativeVec3(std::string_view value, SaveCreativeDocumentVec3Record& out) {
  const std::size_t first = value.find(',');
  const std::size_t second = first == std::string_view::npos ? std::string_view::npos
                                                             : value.find(',', first + 1U);
  if (first == std::string_view::npos || second == std::string_view::npos ||
      value.find(',', second + 1U) != std::string_view::npos) {
    return false;
  }
  return parseDouble(value.substr(0, first), out.x) &&
         parseDouble(value.substr(first + 1U, second - first - 1U), out.y) &&
         parseDouble(value.substr(second + 1U), out.z);
}

bool parseI32(std::string_view value, std::int32_t& out) {
  std::int64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed < std::numeric_limits<std::int32_t>::min() ||
      parsed > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  out = static_cast<std::int32_t>(parsed);
  return true;
}

bool parseBool(std::string_view value, bool& out) {
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

}  // namespace

namespace save_codec_detail {

Reader::Reader(std::string_view bytes) {
  std::string current;
  for (char c : bytes) {
    if (c == '\n') {
      lines_.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  if (!current.empty()) {
    lines_.push_back(current);
  }
}

SaveDecodeResult Reader::decode() {
  // branch-gate: BG-1239
  if (lines_.empty() || lines_[0] != kEnvelopeHeader) {
    return fail(SaveCodecStatus::UnsupportedVersion, "header", 1, "invalid header");
  }
  index_ = 1;
  readMetadata();
  readSession();
  readWorld();
  readAuthoredRoom();
  readCreativeDocument();
  readCreativeWorldLayout();
  readPlayers();
  readClock();
  readCamera();
  readAbilities();
  readCommandLog();
  readInventory();
  readCombat();
  readAi();
  readObjectives();
  if (result_.status != SaveCodecStatus::Ok) {
    return result_;
  }
  if (index_ != lines_.size()) {
    const std::string key = keyAt(index_);
    return fail(SaveCodecStatus::UnknownKey, key, index_ + 1U, "unknown key");
  }
  result_.envelope = envelope_;
  return result_;
}

SaveDecodeResult Reader::fail(SaveCodecStatus status,
                              const std::string& key,
                              std::size_t line,
                              std::string diagnostic) {
  if (result_.status == SaveCodecStatus::Ok) {
    result_.status = status;
    result_.diagnosticSection = sectionForKey(key);
    result_.diagnosticKey = key;
    result_.diagnosticLine = static_cast<std::uint32_t>(line);
    result_.diagnostic = std::move(diagnostic);
  }
  return result_;
}

std::string Reader::keyAt(std::size_t lineIndex) const {
  if (lineIndex >= lines_.size()) {
    return {};
  }
  const std::size_t equals = lines_[lineIndex].find('=');
  return equals == std::string::npos ? lines_[lineIndex] : lines_[lineIndex].substr(0, equals);
}

bool Reader::nextKeyIs(const std::string& expectedKey) const {
  return index_ < lines_.size() && keyAt(index_) == expectedKey;
}

bool Reader::nextValue(const std::string& expectedKey, std::string_view& value) {
  if (result_.status != SaveCodecStatus::Ok) {
    return false;
  }
  if (index_ >= lines_.size()) {
    result_ = fail(SaveCodecStatus::MissingField, expectedKey, index_ + 1U, "missing field");
    return false;
  }
  const std::string& line = lines_[index_];
  const std::size_t equals = line.find('=');
  if (equals == std::string::npos) {
    result_ = fail(SaveCodecStatus::DecodeFailed, expectedKey, index_ + 1U, "expected field");
    return false;
  }
  const std::string key = line.substr(0, equals);
  if (seen_.contains(key)) {
    result_ = fail(SaveCodecStatus::DuplicateKey, key, index_ + 1U, "duplicate key");
    return false;
  }
  if (key != expectedKey) {
    result_ = fail(sectionForKey(key) == sectionForKey(expectedKey)
                       ? SaveCodecStatus::UnknownKey
                       : SaveCodecStatus::InvalidSectionOrder,
                   key, index_ + 1U, "unexpected key");
    return false;
  }
  seen_.insert(key);
  value = std::string_view(line).substr(equals + 1U);
  ++index_;
  return true;
}

bool Reader::readString(const std::string& key, std::string& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!unescapeString(value, out)) {
    result_ = fail(SaveCodecStatus::DecodeFailed, key, index_, "invalid escape");
    return false;
  }
  return true;
}

bool Reader::readOptionalString(const std::string& key, std::string& out) {
  if (!nextKeyIs(key)) {
    return true;
  }
  return readString(key, out);
}

bool Reader::readI32(const std::string& key, std::int32_t& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!parseI32(value, out)) {
    result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid integer");
    return false;
  }
  return true;
}

bool Reader::readOptionalI32(const std::string& key, std::int32_t& out) {
  if (!nextKeyIs(key)) {
    return true;
  }
  return readI32(key, out);
}

bool Reader::readFloat(const std::string& key, float& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!parseFloat(value, out)) {
    result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid float");
    return false;
  }
  return true;
}

bool Reader::readDouble(const std::string& key, double& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!parseDouble(value, out)) {
    result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid double");
    return false;
  }
  return true;
}

bool Reader::readBool(const std::string& key, bool& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!parseBool(value, out)) {
    result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid bool");
    return false;
  }
  return true;
}

bool Reader::readVec3(const std::string& key, Vec3& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!parseVec3(value, out)) {
    result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid vector");
    return false;
  }
  return true;
}

bool Reader::readCreativeVec3(const std::string& key,
                              SaveCreativeDocumentVec3Record& out) {
  std::string_view value;
  if (!nextValue(key, value)) {
    return false;
  }
  if (!parseCreativeVec3(value, out)) {
    result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid vector");
    return false;
  }
  return true;
}

bool Reader::readEntityId(const std::string& key, EntityId& out) {
  std::uint64_t value = 0;
  if (!readUnsigned(key, value, SaveCodecStatus::InvalidId)) {
    return false;
  }
  out = EntityId{value};
  return true;
}

}  // namespace save_codec_detail

SaveDecodeResult decodeSaveEnvelope(std::string_view bytes) {
  // branch-gate: BG-1239
  if (bytes.size() > save_codec_detail::kMaxSaveBytes) {
    SaveDecodeResult result;
    result.status = SaveCodecStatus::SaveTooLarge;
    result.diagnostic = "save too large";
    return result;
  }
  return save_codec_detail::Reader(bytes).decode();
}

}  // namespace iggy3d
