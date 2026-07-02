#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"

namespace iggy3d {

struct AsciiRoomCanvas {
  std::size_t width = 0;
  std::size_t height = 0;
  std::vector<char> tiles;
};

AsciiRoomCanvas makeAsciiRoomCanvas(std::size_t width,
                                    std::size_t height,
                                    char fillTile = ' ');
bool asciiRoomCanvasInBounds(const AsciiRoomCanvas& canvas,
                             std::int32_t x,
                             std::int32_t y);
std::size_t asciiRoomCanvasIndex(const AsciiRoomCanvas& canvas,
                                 std::size_t x,
                                 std::size_t y);
char asciiRoomCanvasGet(const AsciiRoomCanvas& canvas,
                        std::int32_t x,
                        std::int32_t y,
                        char outOfBoundsTile = '\0');
bool asciiRoomCanvasSet(AsciiRoomCanvas& canvas,
                        std::int32_t x,
                        std::int32_t y,
                        char tile);
void fillAsciiRoomRect(AsciiRoomCanvas& canvas,
                       std::int32_t x,
                       std::int32_t y,
                       std::int32_t width,
                       std::int32_t height,
                       char tile);
void drawAsciiRoomBorder(AsciiRoomCanvas& canvas,
                         std::int32_t x,
                         std::int32_t y,
                         std::int32_t width,
                         std::int32_t height,
                         char wallTile = '#');
void drawAsciiRoom(AsciiRoomCanvas& canvas,
                   std::int32_t x,
                   std::int32_t y,
                   std::int32_t width,
                   std::int32_t height,
                   char floorTile = '.',
                   char wallTile = '#');
bool drawAsciiRoomDoor(AsciiRoomCanvas& canvas,
                       std::int32_t x,
                       std::int32_t y,
                       char doorTile = '+');
std::string asciiRoomCanvasToText(const AsciiRoomCanvas& canvas);
AsciiRoomSource parseAsciiRoomCanvas(const AsciiRoomCanvas& canvas,
                                     std::string sourceName = {});

}  // namespace iggy3d
