#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"

#include <array>

namespace iggy3d_creative_app {
namespace {

constexpr std::array<std::string_view, 5U> kKnownKinds{
    kPlaytestEventKindSessionStarted,
    kPlaytestEventKindHeartbeat,
    kPlaytestEventKindRuntimeEvent,
    kPlaytestEventKindSessionEnded,
    kPlaytestEventKindCommandAck,
};

[[nodiscard]] std::string sanitizeToken(std::string_view raw) {
  std::string token;
  token.reserve(raw.size());
  for (const char value : raw) {
    const bool forbidden = value == ' ' || value == '\n' || value == '\r' ||
                           value == '\t' || value == '=';
    token.push_back(forbidden ? '_' : value);
  }
  return token;
}

[[nodiscard]] bool validToken(std::string_view token) noexcept {
  if (token.empty()) {
    return false;
  }
  for (const char value : token) {
    if (value == ' ' || value == '\n' || value == '\r' || value == '\t') {
      return false;
    }
  }
  return true;
}

}  // namespace

bool isKnownPlaytestEventKind(std::string_view kind) noexcept {
  for (const std::string_view known : kKnownKinds) {
    if (kind == known) {
      return true;
    }
  }
  return false;
}

std::string_view PlaytestEvent::field(std::string_view key,
                                      std::string_view fallback) const {
  for (const auto& [candidate, value] : fields) {
    if (candidate == key) {
      return value;
    }
  }
  return fallback;
}

namespace {

[[nodiscard]] std::string formatLineWithMagic(std::string_view magic,
                                              const PlaytestEvent& event);
[[nodiscard]] PlaytestEventParseResult parseLineWithMagic(
    std::string_view magic, std::string_view line);

}  // namespace

std::string formatPlaytestEventLine(const PlaytestEvent& event) {
  return formatLineWithMagic(kPlaytestEventMagic, event);
}

std::string formatPlaytestCommandLine(const PlaytestEvent& command) {
  return formatLineWithMagic(kPlaytestCommandMagic, command);
}

PlaytestEventParseResult parsePlaytestEventLine(std::string_view line) {
  return parseLineWithMagic(kPlaytestEventMagic, line);
}

PlaytestEventParseResult parsePlaytestCommandLine(std::string_view line) {
  return parseLineWithMagic(kPlaytestCommandMagic, line);
}

namespace {

std::string formatLineWithMagic(std::string_view magic,
                                const PlaytestEvent& event) {
  std::string line{magic};
  line.push_back(' ');
  line += sanitizeToken(event.kind.empty() ? "unknown" : event.kind);
  for (const auto& [key, value] : event.fields) {
    line.push_back(' ');
    line += sanitizeToken(key.empty() ? "_" : key);
    line.push_back('=');
    line += sanitizeToken(value);
  }
  return line;
}

PlaytestEventParseResult parseLineWithMagic(std::string_view magic,
                                            std::string_view line) {
  PlaytestEventParseResult result;
  // Tolerate a trailing carriage return (Windows-side children later).
  while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
    line.remove_suffix(1);
  }
  if (line.substr(0, magic.size()) != magic) {
    result.status = PlaytestEventParseStatus::NotProtocol;
    return result;
  }
  std::string_view rest = line.substr(magic.size());
  if (rest.empty() || rest.front() != ' ') {
    result.status = PlaytestEventParseStatus::Malformed;
    return result;
  }
  rest.remove_prefix(1);

  const auto nextToken = [&rest]() -> std::string_view {
    const std::size_t space = rest.find(' ');
    const std::string_view token =
        space == std::string_view::npos ? rest : rest.substr(0, space);
    rest = space == std::string_view::npos ? std::string_view{}
                                           : rest.substr(space + 1);
    return token;
  };

  const std::string_view kind = nextToken();
  if (!validToken(kind) || kind.find('=') != std::string_view::npos) {
    result.status = PlaytestEventParseStatus::Malformed;
    return result;
  }
  result.event.kind = std::string(kind);
  while (!rest.empty()) {
    const std::string_view pair = nextToken();
    if (pair.empty()) {
      continue;  // collapse duplicate spaces
    }
    const std::size_t equals = pair.find('=');
    if (equals == std::string_view::npos || equals == 0U) {
      result.status = PlaytestEventParseStatus::Malformed;
      return result;
    }
    result.event.fields.emplace_back(std::string(pair.substr(0, equals)),
                                     std::string(pair.substr(equals + 1)));
  }
  result.status = PlaytestEventParseStatus::Parsed;
  return result;
}

}  // namespace

PlaytestEventStreamParser::FeedStats PlaytestEventStreamParser::consumeLine(
    std::string_view line, std::vector<PlaytestEvent>& out) {
  FeedStats stats;
  if (line.empty()) {
    return stats;
  }
  PlaytestEventParseResult parsed = parseLineWithMagic(magic_, line);
  switch (parsed.status) {
    case PlaytestEventParseStatus::Parsed:
      ++stats.parsedCount;
      ++totalParsed_;
      out.push_back(std::move(parsed.event));
      break;
    case PlaytestEventParseStatus::Malformed:
      ++stats.malformedCount;
      ++totalMalformed_;
      break;
    case PlaytestEventParseStatus::NotProtocol:
      ++stats.nonProtocolCount;
      ++totalNonProtocol_;
      break;
  }
  return stats;
}

PlaytestEventStreamParser::FeedStats PlaytestEventStreamParser::feed(
    std::string_view chunk, std::vector<PlaytestEvent>& out) {
  FeedStats stats;
  partial_ += chunk;
  std::size_t start = 0U;
  while (true) {
    const std::size_t newline = partial_.find('\n', start);
    if (newline == std::string::npos) {
      break;
    }
    const FeedStats lineStats = consumeLine(
        std::string_view(partial_).substr(start, newline - start), out);
    stats.parsedCount += lineStats.parsedCount;
    stats.malformedCount += lineStats.malformedCount;
    stats.nonProtocolCount += lineStats.nonProtocolCount;
    start = newline + 1U;
  }
  partial_.erase(0, start);
  return stats;
}

PlaytestEventStreamParser::FeedStats PlaytestEventStreamParser::finish(
    std::vector<PlaytestEvent>& out) {
  FeedStats stats;
  if (!partial_.empty()) {
    stats = consumeLine(partial_, out);
    partial_.clear();
  }
  return stats;
}

}  // namespace iggy3d_creative_app
