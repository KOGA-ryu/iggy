#pragma once

// IGGY3DP1 — the one-way playtest event protocol (i3dp stdout -> i3dc).
// One event per line: `IGGY3DP1 <kind> key=value key=value...`
//   - fixed magic+version token first;
//   - kind token second;
//   - space-separated key=value pairs, keys and values sanitized to
//     [no spaces, no newlines, no '='] on format.
// The vocabulary is APPEND-ONLY (the wire-law): parsers must IGNORE unknown
// kinds but COUNT them, so old editors keep working against newer children.
// Pure functions and a small streaming reassembler -- no iostream state.

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {

inline constexpr std::string_view kPlaytestEventMagic = "IGGY3DP1";
// The editor->child command channel magic (child stdin). Same grammar, same
// wire-law, opposite direction: `IGGY3DC1 <verb> seq=<n> k=v...`.
inline constexpr std::string_view kPlaytestCommandMagic = "IGGY3DC1";

// v1 vocabulary. Appending is allowed; renaming or removing is not.
inline constexpr std::string_view kPlaytestEventKindSessionStarted =
    "session_started";
inline constexpr std::string_view kPlaytestEventKindHeartbeat = "heartbeat";
inline constexpr std::string_view kPlaytestEventKindRuntimeEvent =
    "runtime_event";
inline constexpr std::string_view kPlaytestEventKindSessionEnded =
    "session_ended";
// Appended (command-channel slice): the child acks EVERY stdin command.
inline constexpr std::string_view kPlaytestEventKindCommandAck = "command_ack";

// v1 command verbs (append-only).
inline constexpr std::string_view kPlaytestCommandVerbPause = "pause";
inline constexpr std::string_view kPlaytestCommandVerbResume = "resume";

[[nodiscard]] bool isKnownPlaytestEventKind(std::string_view kind) noexcept;

struct PlaytestEvent {
  std::string kind;
  std::vector<std::pair<std::string, std::string>> fields;

  [[nodiscard]] std::string_view field(std::string_view key,
                                       std::string_view fallback = {}) const;
};

// Formats WITHOUT the trailing newline (the writer owns framing). Keys,
// values, and the kind are sanitized: spaces/newlines/'=' become '_'.
[[nodiscard]] std::string formatPlaytestEventLine(const PlaytestEvent& event);
// Same grammar under the IGGY3DC1 magic (kind = the command verb).
[[nodiscard]] std::string formatPlaytestCommandLine(const PlaytestEvent& command);

enum class PlaytestEventParseStatus : std::uint8_t {
  Parsed,       // well-formed protocol line (kind may still be unknown)
  NotProtocol,  // no IGGY3DP1 magic -- foreign chatter, never fatal
  Malformed,    // magic present but the line violates the grammar
};

struct PlaytestEventParseResult {
  PlaytestEventParseStatus status = PlaytestEventParseStatus::NotProtocol;
  PlaytestEvent event;
};

[[nodiscard]] PlaytestEventParseResult parsePlaytestEventLine(
    std::string_view line);
[[nodiscard]] PlaytestEventParseResult parsePlaytestCommandLine(
    std::string_view line);

// Streaming reassembler: feed arbitrary chunk boundaries; complete lines are
// parsed, the trailing partial line is carried to the next feed. Counters
// are cumulative and never fatal.
class PlaytestEventStreamParser {
 public:
  // Defaults to the event magic (IGGY3DP1); the child's stdin drain reuses
  // the same reassembly discipline with the command magic (IGGY3DC1).
  PlaytestEventStreamParser() = default;
  explicit PlaytestEventStreamParser(std::string_view magic)
      : magic_(magic) {}
  struct FeedStats {
    std::size_t parsedCount = 0;
    std::size_t malformedCount = 0;
    std::size_t nonProtocolCount = 0;
  };
  FeedStats feed(std::string_view chunk, std::vector<PlaytestEvent>& out);
  // Flush a trailing unterminated line (child exited mid-write).
  FeedStats finish(std::vector<PlaytestEvent>& out);

  [[nodiscard]] std::size_t totalParsed() const { return totalParsed_; }
  [[nodiscard]] std::size_t totalMalformed() const { return totalMalformed_; }
  [[nodiscard]] std::size_t totalNonProtocol() const {
    return totalNonProtocol_;
  }

 private:
  FeedStats consumeLine(std::string_view line, std::vector<PlaytestEvent>& out);
  std::string_view magic_ = kPlaytestEventMagic;
  std::string partial_;
  std::size_t totalParsed_ = 0;
  std::size_t totalMalformed_ = 0;
  std::size_t totalNonProtocol_ = 0;
};

}  // namespace iggy3d_creative_app
