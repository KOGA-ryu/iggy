#pragma once

#include "runtime/first_move/MathNotation.hpp"
#include <array>

namespace paths {
struct NotationBounds {float x=0,y=0,width=0,height=0;bool available=false;};
enum class NotationControl : std::size_t { Close, Previous, Next, Mode, Up, Down, Count };
struct MathNotationUiState {
  std::string question;
  std::uint32_t run=0;
  bool open=false, practice=false, top=false, follow=false;
  std::size_t lesson=0, token=0;
  iggy3d::first_move::NotationVerdict verdict=iggy3d::first_move::NotationVerdict::Unavailable;
  float scroll=0, scrollMax=0, pageHeight=0;
  NotationBounds toggle, panel, body, detail;
  std::array<NotationBounds,static_cast<std::size_t>(NotationControl::Count)> controls{};
  std::array<NotationBounds,iggy3d::first_move::kNotationTokenCapacity> tokens{};
};
// Call in any equation workspace; switching working steps does not reset a lesson.
void syncMathNotation(MathNotationUiState&,std::string_view question,std::uint32_t run);
void drawMathNotationToggle(MathNotationUiState&,bool available,bool blocked);
// Fills its caller's existing panel. No equation parser, scene, or game dependency.
void drawMathNotation(MathNotationUiState&,std::span<const iggy3d::first_move::NotationLesson>,bool blocked,bool paused);
} // namespace paths
