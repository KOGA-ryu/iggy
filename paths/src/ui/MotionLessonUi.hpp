#pragma once
#include "scene/MotionScene.hpp"
#include "ui/MathNotationUi.hpp"
#include "ui/NativeMath.hpp"

namespace paths {
enum class MotionControl : std::size_t { Back,Chapter,Run,Pause,Rewind,Speed,Undo,Next,Again,Timeline,Symbols,ReturnToPlan,Count };
struct MotionLessonUiState {
  MotionLesson* lesson=nullptr;
  NativeMath* math=nullptr;
  MotionScene scene;
  bool open=false,presented=false,symbols=false;
  std::string progressMessage;
  bool progressFailed=false;
  std::array<NotationBounds,static_cast<std::size_t>(MotionControl::Count)> controls{};
  std::array<NotationBounds,motionChapterCount> chapters{};
  std::array<NotationBounds,2> parameters{},handles{};
  std::array<NotationBounds,4> choices{};
  std::vector<NotationBounds> attempts;
  NotationBounds question,graph,working,track;
  std::size_t formulaErrors=0;
};
void drawMotionLesson(MotionLessonUiState&);
} // namespace paths
