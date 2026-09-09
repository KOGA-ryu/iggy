#pragma once
#include "runtime/textbook/Textbook.hpp"
#include "runtime/textbook/RowPlaneFigure.hpp"
#include "runtime/textbook/LessonSpread.hpp"
#include "scene/MathObjectScene.hpp"
#include "MatrixBoardUi.hpp"
#include "NativeMath.hpp"

namespace paths {
struct TextbookFigureUiState {
  RowPlaneFigure model;
  MathObjectScene scene;
  bool hasPending=false,resetCamera=false;
  RowPlaneAction pending{RowPlaneActionKind::Reset};
  bool hasSystemPending=false;
  SystemAction systemPending{SystemActionKind::ResetExploration};
  int prediction=-1,reason=-1;
  bool originalEquations=false;
  int target=0,other=1;
  double multiplier=-1;
  std::string message;
  struct Equation { std::string latex;float pixels=0;NativeMath::Equation layout; };
  std::array<Equation,3> equations;
};
// A topic plugs its model and controls into these shared title/viewport/control regions.
// Returns true only when this frame actually supplies a scene to the native host.
void applySystemLessonPending(SystemLesson&,TextbookFigureUiState&);
void drawSystemExercise(SystemLesson&,TextbookFigureUiState&,MatrixBoardUiState&,NativeMath&);
bool drawTextbookFigure(const BookFigureSpec&,Textbook&,MatrixBoardUiState&,
                       TextbookFigureUiState&,NativeMath&,SceneViewport,float textScale);
} // namespace paths
