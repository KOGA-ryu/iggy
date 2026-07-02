#include "app/iggy3d/ascii_room/AsciiRoomCanvas.hpp"

#include <utility>

namespace iggy3d {

AsciiRoomCanvas makeAsciiRoomCanvas(std::size_t width,
                                    std::size_t height,
                                    char fillTile) {
  AsciiRoomCanvas canvas;
  canvas.width = width;
  canvas.height = height;
  canvas.tiles.assign(width * height, fillTile);
  return canvas;
}

bool asciiRoomCanvasInBounds(const AsciiRoomCanvas& canvas,
                             std::int32_t x,
                             std::int32_t y) {
  return x >= 0 && y >= 0 &&
         static_cast<std::size_t>(x) < canvas.width &&
         static_cast<std::size_t>(y) < canvas.height;
}

std::size_t asciiRoomCanvasIndex(const AsciiRoomCanvas& canvas,
                                 std::size_t x,
                                 std::size_t y) {
  return y * canvas.width + x;
}

char asciiRoomCanvasGet(const AsciiRoomCanvas& canvas,
                        std::int32_t x,
                        std::int32_t y,
                        char outOfBoundsTile) {
  if (!asciiRoomCanvasInBounds(canvas, x, y)) {
    return outOfBoundsTile;
  }
  return canvas.tiles[asciiRoomCanvasIndex(canvas,
                                           static_cast<std::size_t>(x),
                                           static_cast<std::size_t>(y))];
}

bool asciiRoomCanvasSet(AsciiRoomCanvas& canvas,
                        std::int32_t x,
                        std::int32_t y,
                        char tile) {
  if (!asciiRoomCanvasInBounds(canvas, x, y)) {
    return false;
  }
  canvas.tiles[asciiRoomCanvasIndex(canvas,
                                    static_cast<std::size_t>(x),
                                    static_cast<std::size_t>(y))] = tile;
  return true;
}

void fillAsciiRoomRect(AsciiRoomCanvas& canvas,
                       std::int32_t x,
                       std::int32_t y,
                       std::int32_t width,
                       std::int32_t height,
                       char tile) {
  if (width <= 0 || height <= 0) {
    return;
  }
  for (std::int32_t row = 0; row < height; ++row) {
    for (std::int32_t column = 0; column < width; ++column) {
      (void)asciiRoomCanvasSet(canvas, x + column, y + row, tile);
    }
  }
}

void drawAsciiRoomBorder(AsciiRoomCanvas& canvas,
                         std::int32_t x,
                         std::int32_t y,
                         std::int32_t width,
                         std::int32_t height,
                         char wallTile) {
  if (width <= 0 || height <= 0) {
    return;
  }
  for (std::int32_t column = 0; column < width; ++column) {
    (void)asciiRoomCanvasSet(canvas, x + column, y, wallTile);
    (void)asciiRoomCanvasSet(canvas, x + column, y + height - 1, wallTile);
  }
  for (std::int32_t row = 0; row < height; ++row) {
    (void)asciiRoomCanvasSet(canvas, x, y + row, wallTile);
    (void)asciiRoomCanvasSet(canvas, x + width - 1, y + row, wallTile);
  }
}

void drawAsciiRoom(AsciiRoomCanvas& canvas,
                   std::int32_t x,
                   std::int32_t y,
                   std::int32_t width,
                   std::int32_t height,
                   char floorTile,
                   char wallTile) {
  fillAsciiRoomRect(canvas, x, y, width, height, floorTile);
  drawAsciiRoomBorder(canvas, x, y, width, height, wallTile);
}

bool drawAsciiRoomDoor(AsciiRoomCanvas& canvas,
                       std::int32_t x,
                       std::int32_t y,
                       char doorTile) {
  return asciiRoomCanvasSet(canvas, x, y, doorTile);
}

std::string asciiRoomCanvasToText(const AsciiRoomCanvas& canvas) {
  std::string text;
  text.reserve(canvas.height * (canvas.width + 1U));
  for (std::size_t y = 0; y < canvas.height; ++y) {
    for (std::size_t x = 0; x < canvas.width; ++x) {
      text.push_back(canvas.tiles[asciiRoomCanvasIndex(canvas, x, y)]);
    }
    text.push_back('\n');
  }
  return text;
}

AsciiRoomSource parseAsciiRoomCanvas(const AsciiRoomCanvas& canvas,
                                     std::string sourceName) {
  return parseAsciiRoomSource(asciiRoomCanvasToText(canvas),
                              std::move(sourceName));
}

}  // namespace iggy3d
