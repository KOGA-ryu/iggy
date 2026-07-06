#include "runtime/ai/ReconIntel.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace iggy3d {

ReconIntel captureReconIntel(std::span<const GuardReconObservation> observations) {
  ReconIntel intel;
  intel.guards.assign(observations.begin(), observations.end());
  intel.guardCount = static_cast<std::uint32_t>(intel.guards.size());
  for (const GuardReconObservation& guard : intel.guards) {
    if (guard.alertLevel >= kReconAlertedLevel) {
      ++intel.alertedGuardCount;
    }
    if (guard.hasLastKnownTarget) {
      intel.anyGuardHasLastKnownTarget = true;
    }
  }
  return intel;
}

StableHashValue hashReconIntel(const ReconIntel& intel) {
  StableHasher hasher;
  hasher.addU64(intel.guardCount);
  hasher.addU64(intel.alertedGuardCount);
  hasher.addBool(intel.anyGuardHasLastKnownTarget);
  // Order-sensitive fold: swapping two guards changes the digest (no set-semantics).
  for (const GuardReconObservation& g : intel.guards) {
    hasher.addU64(g.guard.value);
    addVec3Quantized(hasher, g.position);
    addVec3Quantized(hasher, g.facing);
    hasher.addFloatQuantized(g.alertLevel);
    hasher.addString(g.behavior);
    hasher.addBool(g.patrols);
    hasher.addU64(g.patrolWaypointCount);
    hasher.addU64(g.patrolTargetIndex);
    hasher.addU64(static_cast<std::uint64_t>(g.patrolMode));
    addVec3Quantized(hasher, g.patrolTarget);
    hasher.addBool(g.hasLastKnownTarget);
    addVec3Quantized(hasher, g.lastKnownTargetPosition);
    hasher.addBool(g.hasWatchedNode);
    hasher.addU64(static_cast<std::uint64_t>(g.watchedNodeKind));
    addVec3Quantized(hasher, g.watchedNodePosition);
  }
  return hasher.value();
}

namespace {

// Shortest round-trip float text (std::to_chars) -- lossless, the SaveCodec discipline.
std::string floatText(float value) {
  std::array<char, 32> buffer{};
  const std::to_chars_result result =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  return std::string(buffer.data(), result.ptr);
}

std::string vec3Text(Vec3 value) {
  return floatText(value.x) + "," + floatText(value.y) + "," + floatText(value.z);
}

void appendLine(std::string& out, std::string_view key, std::string_view value) {
  out += key;
  out += '=';
  out += value;
  out += '\n';
}

float parseFloat(std::string_view value) {
  const std::string text(value);
  return std::strtof(text.c_str(), nullptr);
}

Vec3 parseVec3(std::string_view value) {
  Vec3 out;
  const std::size_t first = value.find(',');
  if (first == std::string_view::npos) {
    return out;
  }
  const std::size_t second = value.find(',', first + 1U);
  if (second == std::string_view::npos) {
    return out;
  }
  out.x = parseFloat(value.substr(0, first));
  out.y = parseFloat(value.substr(first + 1U, second - first - 1U));
  out.z = parseFloat(value.substr(second + 1U));
  return out;
}

// Re-resolve a decoded behavior string to its STABLE static name (the string_view must point at the
// aiBehaviorKindName table, never a temporary). Append-only enum -- keep this list in sync.
std::string_view resolveBehaviorName(std::string_view name) {
  constexpr AiBehaviorKind kAll[] = {
      AiBehaviorKind::None,      AiBehaviorKind::Idle,      AiBehaviorKind::Alert,
      AiBehaviorKind::Chasing,   AiBehaviorKind::Attacking, AiBehaviorKind::Defeated,
      AiBehaviorKind::Returning, AiBehaviorKind::Observant, AiBehaviorKind::Suspicious,
      AiBehaviorKind::Searching};
  for (const AiBehaviorKind kind : kAll) {
    if (aiBehaviorKindName(kind) == name) {
      return aiBehaviorKindName(kind);
    }
  }
  return {};
}

}  // namespace

std::string serializeReconIntel(const ReconIntel& intel) {
  std::string out = "recon_intel.v1\n";
  appendLine(out, "guards", std::to_string(intel.guards.size()));
  for (std::size_t i = 0; i < intel.guards.size(); ++i) {
    const GuardReconObservation& g = intel.guards[i];
    const std::string p = "g" + std::to_string(i) + ".";
    appendLine(out, p + "id", std::to_string(g.guard.value));
    appendLine(out, p + "pos", vec3Text(g.position));
    appendLine(out, p + "facing", vec3Text(g.facing));
    appendLine(out, p + "alert", floatText(g.alertLevel));
    appendLine(out, p + "behavior", g.behavior);
    appendLine(out, p + "patrols", g.patrols ? "1" : "0");
    appendLine(out, p + "wp_count", std::to_string(g.patrolWaypointCount));
    appendLine(out, p + "wp_index", std::to_string(g.patrolTargetIndex));
    appendLine(out, p + "patrol_mode",
               std::to_string(static_cast<std::uint32_t>(g.patrolMode)));
    appendLine(out, p + "patrol_target", vec3Text(g.patrolTarget));
    appendLine(out, p + "has_last_known", g.hasLastKnownTarget ? "1" : "0");
    appendLine(out, p + "last_known", vec3Text(g.lastKnownTargetPosition));
    appendLine(out, p + "has_watched", g.hasWatchedNode ? "1" : "0");
    appendLine(out, p + "watched_kind",
               std::to_string(static_cast<std::uint32_t>(g.watchedNodeKind)));
    appendLine(out, p + "watched_pos", vec3Text(g.watchedNodePosition));
  }
  return out;
}

ReconIntelDecodeResult deserializeReconIntel(std::string_view text) {
  ReconIntelDecodeResult result;

  std::size_t pos = 0;
  const auto nextLine = [&]() -> std::string_view {
    const std::size_t nl = text.find('\n', pos);
    if (nl == std::string_view::npos) {
      const std::string_view line = text.substr(pos);
      pos = text.size();
      return line;
    }
    const std::string_view line = text.substr(pos, nl - pos);
    pos = nl + 1U;
    return line;
  };

  if (nextLine() != "recon_intel.v1") {
    result.reasonCode = "recon_intel_bad_header";
    return result;
  }

  std::unordered_map<std::string, std::string> fields;
  while (pos < text.size()) {
    const std::string_view line = nextLine();
    const std::size_t eq = line.find('=');
    if (eq == std::string_view::npos) {
      continue;
    }
    fields.insert_or_assign(std::string(line.substr(0, eq)),
                            std::string(line.substr(eq + 1U)));
  }

  const auto field = [&](const std::string& key) -> std::string_view {
    const auto it = fields.find(key);
    return it == fields.end() ? std::string_view{} : std::string_view(it->second);
  };
  const auto readU32 = [&](const std::string& key) -> std::uint32_t {
    const std::string_view v = field(key);
    std::uint32_t out = 0;
    std::from_chars(v.data(), v.data() + v.size(), out);
    return out;
  };
  const auto readU64 = [&](const std::string& key) -> std::uint64_t {
    const std::string_view v = field(key);
    std::uint64_t out = 0;
    std::from_chars(v.data(), v.data() + v.size(), out);
    return out;
  };
  const auto readFlag = [&](const std::string& key) -> bool {
    return field(key) == "1";
  };

  const std::size_t count = readU64("guards");
  std::vector<GuardReconObservation> guards;
  guards.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    const std::string p = "g" + std::to_string(i) + ".";
    GuardReconObservation g;
    g.guard = EntityId{readU64(p + "id")};
    g.position = parseVec3(field(p + "pos"));
    g.facing = parseVec3(field(p + "facing"));
    g.alertLevel = parseFloat(field(p + "alert"));
    g.behavior = resolveBehaviorName(field(p + "behavior"));
    g.patrols = readFlag(p + "patrols");
    g.patrolWaypointCount = readU32(p + "wp_count");
    g.patrolTargetIndex = readU32(p + "wp_index");
    g.patrolMode = static_cast<PatrolMode>(readU32(p + "patrol_mode"));
    g.patrolTarget = parseVec3(field(p + "patrol_target"));
    g.hasLastKnownTarget = readFlag(p + "has_last_known");
    g.lastKnownTargetPosition = parseVec3(field(p + "last_known"));
    g.hasWatchedNode = readFlag(p + "has_watched");
    g.watchedNodeKind = static_cast<ReasoningNodeKind>(readU32(p + "watched_kind"));
    g.watchedNodePosition = parseVec3(field(p + "watched_pos"));
    guards.push_back(g);
  }

  // Rebuild the packet (derived summary) from the restored guards -- one source of truth.
  result.intel = captureReconIntel(guards);
  result.ok = true;
  result.reasonCode = "recon_intel_ok";
  return result;
}

}  // namespace iggy3d
