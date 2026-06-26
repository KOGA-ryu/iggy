#include "app/iggy3d/ProductBuiltinDungeon.hpp"

#include <string>

namespace iggy3d {
namespace {

constexpr std::string_view kWorldTitle = "Loop Keep";
constexpr std::string_view kRoomId = "loop_keep_ascii";
constexpr std::string_view kSourceName = "fixtures/rooms/ascii/loop_keep.iggyroom.txt";
constexpr std::string_view kAsciiRoomText =
    "#################\n"
    "#P.....#.......E#\n"
    "#.###..#..###...#\n"
    "#...K..+..$.....#\n"
    "#.###..#..###...#\n"
    "#.......#.......#\n"
    "#################\n";

}  // namespace

std::string_view productBuiltinDungeonWorldTitle() {
  return kWorldTitle;
}

std::string_view productBuiltinDungeonRoomId() {
  return kRoomId;
}

std::string_view productBuiltinDungeonSourceName() {
  return kSourceName;
}

std::string_view productBuiltinDungeonAsciiRoomText() {
  return kAsciiRoomText;
}

WorldSetupDraft makeProductDefaultWorldSetupDraft(
    std::string_view generatedSeedText) {
  WorldSetupDraft draft = makeDefaultWorldSetupDraft(generatedSeedText);
  draft.worldName = std::string(kWorldTitle);
  draft.asciiRoomEnabled = true;
  draft.asciiRoomText = std::string(kAsciiRoomText);
  draft.asciiRoomId = std::string(kRoomId);
  draft.asciiRoomSourceName = std::string(kSourceName);
  draft.selectedField = WorldSetupField::Create;
  return draft;
}

}  // namespace iggy3d
