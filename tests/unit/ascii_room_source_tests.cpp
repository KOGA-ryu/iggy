#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool emptySourceRejects() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource("");
  return expect(source.status == "ascii_room_empty", "empty source status") &&
         expect(source.reasonCode == "ascii_room_empty", "empty source reason") &&
         expect(!source.diagnostics.empty(), "empty source diagnostic") &&
         expect(source.diagnostics.front().severity == "error", "empty severity");
}

bool trailingNewlineAccepted() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource("###\n#P#\n###\n");
  return expect(source.status == "ascii_room_ok", "trailing newline ok") &&
         expect(source.width == 3U, "trailing newline width") &&
         expect(source.height == 3U, "trailing newline height") &&
         expect(source.rows.size() == 3U, "trailing newline ignored");
}

bool crlfIsNormalized() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource("###\r\n#P#\r\n###\r\n");
  return expect(source.status == "ascii_room_ok", "crlf ok") &&
         expect(source.rawText == "###\n#P#\n###\n", "crlf normalized") &&
         expect(source.height == 3U, "crlf height");
}

bool unknownGlyphRejectsWithPosition() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource("###\n#@#\n#P#");
  return expect(source.status == "ascii_room_unknown_glyph", "unknown status") &&
         expect(!source.diagnostics.empty(), "unknown diagnostic") &&
         expect(source.diagnostics.front().row == 1U, "unknown row") &&
         expect(source.diagnostics.front().column == 1U, "unknown column") &&
         expect(source.diagnostics.front().glyph == '@', "unknown glyph") &&
         expect(source.diagnostics.front().reasonCode == "ascii_room_unknown_glyph",
                "unknown reason");
}

bool raggedRowsReject() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource("####\n#P#\n####");
  return expect(source.status == "ascii_room_ragged_rows", "ragged status") &&
         expect(!source.diagnostics.empty(), "ragged diagnostic") &&
         expect(source.diagnostics.front().row == 1U, "ragged row") &&
         expect(source.diagnostics.front().reasonCode == "ascii_room_ragged_rows",
                "ragged reason");
}

bool sourceOffsetIsZeroBasedAndNormalized() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource("###\r\n#P#\r\n###\r\n");
  return expect(iggy3d::asciiRoomSourceOffset(source, 1, 1) == 5U,
                "source offset row column");
}

bool scaleDirectiveIsParsedAndStrippedFromRows() {
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource("*5\n###\n#P#\n###\n");
  return expect(source.status == "ascii_room_ok", "scale source ok") &&
         expect(source.hasTileScaleDirective, "scale directive present") &&
         expect(source.tileScaleMeters == 5.0F, "scale value") &&
         expect(source.width == 3U, "scale width") &&
         expect(source.height == 3U, "scale height") &&
         expect(source.rows.front() == "###", "scale row stripped") &&
         expect(iggy3d::asciiRoomSourceOffset(source, 1, 1) == 8U,
                "scale source offset");
}

bool invalidScaleDirectiveRejects() {
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource("*0\n###\n#P#\n###\n");
  return expect(source.status == "ascii_room_invalid_scale",
                "invalid scale status") &&
         expect(source.reasonCode == "ascii_room_invalid_scale",
                "invalid scale reason") &&
         expect(!source.diagnostics.empty(), "invalid scale diagnostic");
}

bool floorLayersParseWithFourMeterSpacing() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource(
      "floor1\n"
      "P...\n"
      "....\n"
      "floor2\n"
      "..  \n"
      "....\n");
  return expect(source.status == "ascii_room_ok", "layer source ok") &&
         expect(source.hasLayerDirectives, "layer directives present") &&
         expect(source.layerFloorSpacingMeters == 4.0F, "layer spacing") &&
         expect(source.width == 4U, "layer width") &&
         expect(source.height == 2U, "layer height") &&
         expect(source.layers.size() == 2U, "layer count") &&
         expect(source.layers[0].name == "floor1", "floor1 name") &&
         expect(source.layers[0].storyIndex == 0, "floor1 story") &&
         expect(source.layers[1].name == "floor2", "floor2 name") &&
         expect(source.layers[1].storyIndex == 1, "floor2 story") &&
         expect(source.layers[1].rows[0] == "..  ", "layer spaces preserved");
}

bool floorLayerSizeMismatchRejects() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource(
      "floor1\n"
      "P...\n"
      "....\n"
      "floor2\n"
      "...\n"
      "....\n");
  return expect(source.status == "ascii_room_layer_size_mismatch",
                "layer size mismatch status") &&
         expect(!source.diagnostics.empty(), "layer mismatch diagnostic");
}

}  // namespace

int main() {
  bool ok = true;
  ok = emptySourceRejects() && ok;
  ok = trailingNewlineAccepted() && ok;
  ok = crlfIsNormalized() && ok;
  ok = unknownGlyphRejectsWithPosition() && ok;
  ok = raggedRowsReject() && ok;
  ok = sourceOffsetIsZeroBasedAndNormalized() && ok;
  ok = scaleDirectiveIsParsedAndStrippedFromRows() && ok;
  ok = invalidScaleDirectiveRejects() && ok;
  ok = floorLayersParseWithFourMeterSpacing() && ok;
  ok = floorLayerSizeMismatchRejects() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
