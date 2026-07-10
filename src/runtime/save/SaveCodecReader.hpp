#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveCodecEnumTables.hpp"

namespace iggy3d::save_codec_detail {

// branch-gate-relocation: BG-1236 from=src/runtime/save/SaveCodec.cpp
template <typename T>
bool parseUnsigned(std::string_view value, T& out) {
  if (value.empty() || value.front() == '+') {
    return false;
  }
  std::uint64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
    return false;
  }
  out = static_cast<T>(parsed);
  return true;
}

class Reader {
public:
  explicit Reader(std::string_view bytes);

  SaveDecodeResult decode();

private:
  SaveDecodeResult fail(SaveCodecStatus status,
                        const std::string& key,
                        std::size_t line,
                        std::string diagnostic);
  std::string keyAt(std::size_t lineIndex) const;
  bool nextKeyIs(const std::string& expectedKey) const;
  bool nextValue(const std::string& expectedKey, std::string_view& value);
  bool readString(const std::string& key, std::string& out);
  bool readOptionalString(const std::string& key, std::string& out);

  template <typename T>
  bool readUnsigned(const std::string& key,
                    T& out,
                    SaveCodecStatus status = SaveCodecStatus::InvalidNumber) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseUnsigned(value, out)) {
      result_ = fail(status, key, index_, "invalid number");
      return false;
    }
    return true;
  }

  bool readI32(const std::string& key, std::int32_t& out);
  bool readOptionalI32(const std::string& key, std::int32_t& out);
  bool readFloat(const std::string& key, float& out);
  bool readDouble(const std::string& key, double& out);
  bool readBool(const std::string& key, bool& out);
  bool readVec3(const std::string& key, Vec3& out);
  bool readCreativeVec3(const std::string& key,
                        SaveCreativeDocumentVec3Record& out);

  template <typename Enum>
  bool readEnum(const std::string& key, Enum& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseEnum(value, out)) {
      result_ = fail(SaveCodecStatus::InvalidEnum, key, index_, "invalid enum");
      return false;
    }
    return true;
  }

  bool readEntityId(const std::string& key, EntityId& out);
  void readMetadata();
  void readSession();
  void readWorld();
  void readStringVector(const std::string& countKey,
                        const std::string& itemPrefix,
                        std::vector<std::string>& out);
  void readOptionalCreativeVec3Vector(
      const std::string& countKey,
      const std::string& itemPrefix,
      const std::string& itemSuffix,
      std::vector<SaveCreativeDocumentVec3Record>& out);
  void readAuthoredRoomSemantics(const std::string& prefix,
                                 SaveAuthoredRoomSemanticsRecord& semantics);
  void readAuthoredRoom();
  void readCreativeDocumentObject(const std::string& prefix,
                                  SaveCreativeDocumentObjectRecord& object);
  void readCreativeDocument();
  void readPlayers();
  void readClock();
  void readCamera();
  void readAbilities();
  void readCommandLog();
  void readInventory();
  void readCombat();
  void readAi();
  void readObjectives();

  SaveEnvelope envelope_;
  SaveDecodeResult result_;
  std::vector<std::string> lines_;
  std::set<std::string> seen_;
  std::size_t index_ = 0;
};

}  // namespace iggy3d::save_codec_detail
