#include "app/iggy3d/ascii_room/AsciiRoomCanvas.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"

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

bool filledCanvasSerializesExpectedRows() {
  const iggy3d::AsciiRoomCanvas canvas =
      iggy3d::makeAsciiRoomCanvas(3, 2, 'x');
  return expect(iggy3d::asciiRoomCanvasToText(canvas) == "xxx\nxxx\n",
                "filled canvas text");
}

bool drawRoomCreatesBorderedRoom() {
  iggy3d::AsciiRoomCanvas canvas =
      iggy3d::makeAsciiRoomCanvas(5, 4, ' ');
  iggy3d::drawAsciiRoom(canvas, 0, 0, 5, 4);
  return expect(iggy3d::asciiRoomCanvasToText(canvas) ==
                    "#####\n"
                    "#...#\n"
                    "#...#\n"
                    "#####\n",
                "room border and floor");
}

bool doorOverwritesWall() {
  iggy3d::AsciiRoomCanvas canvas =
      iggy3d::makeAsciiRoomCanvas(5, 4, ' ');
  iggy3d::drawAsciiRoom(canvas, 0, 0, 5, 4);
  const bool placed = iggy3d::drawAsciiRoomDoor(canvas, 2, 0);
  return expect(placed, "door placement") &&
         expect(iggy3d::asciiRoomCanvasGet(canvas, 2, 0) == '+',
                "door overwrites wall") &&
         expect(iggy3d::asciiRoomCanvasToText(canvas).substr(0, 5) ==
                    "##+##",
                "door text");
}

bool outOfBoundsSetLeavesCanvasUnchanged() {
  iggy3d::AsciiRoomCanvas canvas =
      iggy3d::makeAsciiRoomCanvas(2, 2, '.');
  const std::string before = iggy3d::asciiRoomCanvasToText(canvas);
  const bool writeNegative = iggy3d::asciiRoomCanvasSet(canvas, -1, 0, '#');
  const bool writePastEnd = iggy3d::asciiRoomCanvasSet(canvas, 2, 1, '#');
  return expect(!writeNegative, "negative write rejected") &&
         expect(!writePastEnd, "past end write rejected") &&
         expect(iggy3d::asciiRoomCanvasToText(canvas) == before,
                "canvas unchanged");
}

bool clippedRectangleOnlyWritesValidCells() {
  iggy3d::AsciiRoomCanvas canvas =
      iggy3d::makeAsciiRoomCanvas(4, 3, '.');
  iggy3d::fillAsciiRoomRect(canvas, -1, 1, 6, 3, 'x');
  return expect(iggy3d::asciiRoomCanvasToText(canvas) ==
                    "....\n"
                    "xxxx\n"
                    "xxxx\n",
                "clipped rect text");
}

bool generatedCanvasBuildsAsciiRoomGrid() {
  iggy3d::AsciiRoomCanvas canvas =
      iggy3d::makeAsciiRoomCanvas(7, 5, ' ');
  iggy3d::drawAsciiRoom(canvas, 0, 0, 7, 5);
  (void)iggy3d::asciiRoomCanvasSet(canvas, 1, 1, 'P');
  (void)iggy3d::asciiRoomCanvasSet(canvas, 5, 3, 'E');

  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomCanvas(canvas, "canvas_room");
  const iggy3d::AsciiRoomGridBuildResult result =
      iggy3d::buildAsciiRoomGrid(source);

  return expect(source.status == "ascii_room_ok", "canvas source ok") &&
         expect(result.ok, "canvas grid ok") &&
         expect(result.grid.width == 7U, "canvas grid width") &&
         expect(result.grid.height == 5U, "canvas grid height") &&
         expect(result.grid.playerSpawnCount == 1U,
                "canvas player spawn") &&
         expect(result.grid.markerCount == 2U, "canvas markers");
}

}  // namespace

int main() {
  bool ok = true;
  ok = filledCanvasSerializesExpectedRows() && ok;
  ok = drawRoomCreatesBorderedRoom() && ok;
  ok = doorOverwritesWall() && ok;
  ok = outOfBoundsSetLeavesCanvasUnchanged() && ok;
  ok = clippedRectangleOnlyWritesValidCells() && ok;
  ok = generatedCanvasBuildsAsciiRoomGrid() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
