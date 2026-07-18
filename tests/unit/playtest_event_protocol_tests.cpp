// IGGY3DP1 protocol proof: pure formatter/parser round-trip, unknown-kind
// tolerance (the append-only wire-law), partial-line reassembly across
// arbitrary read boundaries, malformed-line counting (never fatal).

#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

namespace app = iggy3d_creative_app;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool roundTrip() {
  app::PlaytestEvent event;
  event.kind = std::string(app::kPlaytestEventKindSessionStarted);
  event.fields = {{"doc", "7"}, {"rev", "42"}, {"room", "creative_editor_play"}};
  const std::string line = app::formatPlaytestEventLine(event);
  const app::PlaytestEventParseResult parsed =
      app::parsePlaytestEventLine(line);
  return expect(line == "IGGY3DP1 session_started doc=7 rev=42 "
                        "room=creative_editor_play",
                "format produces the exact wire line") &&
         expect(parsed.status == app::PlaytestEventParseStatus::Parsed,
                "round-trip parses") &&
         expect(parsed.event.kind == event.kind &&
                    parsed.event.fields == event.fields,
                "round-trip preserves kind and fields") &&
         expect(parsed.event.field("rev") == "42", "field lookup") &&
         expect(parsed.event.field("absent", "x") == "x",
                "field fallback");
}

bool sanitization() {
  app::PlaytestEvent event;
  event.kind = "weird kind";
  event.fields = {{"k ey", "v=al ue\n"}};
  const std::string line = app::formatPlaytestEventLine(event);
  const app::PlaytestEventParseResult parsed =
      app::parsePlaytestEventLine(line);
  return expect(line == "IGGY3DP1 weird_kind k_ey=v_al_ue_",
                "spaces, '=', newlines sanitized on format") &&
         expect(parsed.status == app::PlaytestEventParseStatus::Parsed,
                "sanitized line stays parseable");
}

bool unknownKindTolerance() {
  // The wire-law: a future child may emit kinds this editor cannot name.
  const app::PlaytestEventParseResult parsed = app::parsePlaytestEventLine(
      "IGGY3DP1 teleport_used from=a to=b");
  return expect(parsed.status == app::PlaytestEventParseStatus::Parsed,
                "unknown kind still parses") &&
         expect(!app::isKnownPlaytestEventKind(parsed.event.kind),
                "unknown kind is reported unknown") &&
         expect(app::isKnownPlaytestEventKind(
                    app::kPlaytestEventKindHeartbeat),
                "known kind is reported known");
}

bool malformedAndForeignLines() {
  return expect(app::parsePlaytestEventLine("hello world").status ==
                    app::PlaytestEventParseStatus::NotProtocol,
                "foreign chatter is not-protocol") &&
         expect(app::parsePlaytestEventLine("IGGY3DP1").status ==
                    app::PlaytestEventParseStatus::Malformed,
                "magic with no kind is malformed") &&
         expect(app::parsePlaytestEventLine("IGGY3DP1 kind bad-pair").status ==
                    app::PlaytestEventParseStatus::Malformed,
                "pair without '=' is malformed") &&
         expect(app::parsePlaytestEventLine("IGGY3DP1 kind =v").status ==
                    app::PlaytestEventParseStatus::Malformed,
                "empty key is malformed") &&
         expect(app::parsePlaytestEventLine(
                    "IGGY3DP1 heartbeat tick=9\r").status ==
                    app::PlaytestEventParseStatus::Parsed,
                "trailing CR tolerated");
}

bool streamingReassembly() {
  app::PlaytestEventStreamParser parser;
  std::vector<app::PlaytestEvent> events;
  // Two events + foreign line + malformed line, fed in awkward chunks that
  // split lines mid-token.
  parser.feed("IGGY3DP1 heartbeat ti", events);
  parser.feed("ck=60\nnoise line\nIGGY3DP1 session_en", events);
  if (!expect(events.size() == 1U && events[0].kind == "heartbeat" &&
                  events[0].field("tick") == "60",
              "partial line completed across feeds")) {
    return false;
  }
  parser.feed("ded reason=escape_pressed frames=9\nIGGY3DP1 broken pair\n",
              events);
  const bool sequenceOk =
      expect(events.size() == 2U && events[1].kind == "session_ended" &&
                 events[1].field("reason") == "escape_pressed",
             "second event completed") &&
      expect(parser.totalNonProtocol() == 1U, "foreign line counted") &&
      expect(parser.totalMalformed() == 1U, "malformed line counted");
  // Unterminated trailing line is recovered by finish() (child died
  // mid-write).
  parser.feed("IGGY3DP1 heartbeat tick=120", events);
  const app::PlaytestEventStreamParser::FeedStats finishStats =
      parser.finish(events);
  return sequenceOk &&
         expect(finishStats.parsedCount == 1U && events.size() == 3U &&
                    events[2].field("tick") == "120",
                "finish flushes the unterminated tail") &&
         expect(parser.totalParsed() == 3U, "cumulative parse count");
}

}  // namespace

int main() {
  const bool ok = roundTrip() && sanitization() && unknownKindTolerance() &&
                  malformedAndForeignLines() && streamingReassembly();
  if (ok) {
    std::cout << "playtest_event_protocol_tests passed\n";
  }
  return ok ? 0 : 1;
}
