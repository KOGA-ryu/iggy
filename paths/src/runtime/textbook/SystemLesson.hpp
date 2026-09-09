#pragma once
#include "RowPlaneFigure.hpp"
#include <optional>

namespace paths {
struct SystemExample {
  const char* id;
  const char* title;
  const char* prompt;
  std::array<double,9> coefficients;
  std::array<double,3> rhs;
};
std::span<const SystemExample> systemExamples();
enum class SystemOutcome : unsigned { None, One, Infinite };
enum class SystemReason : unsigned { Contradiction, FullPivots, FreeVariables };
const char* systemOutcomeName(SystemOutcome);
SystemOutcome systemOutcome(const EquationSpace&);
enum class SystemActionKind { SelectExample, SetCoefficient, SetRightHandSide, RowOperation, ResetExploration, SelectChallenge, Predict, Reveal, SetPoint, CheckPoint };
struct SystemAction {
  SystemActionKind kind;
  unsigned first=0,second=0;
  double value=0;
  BoardAction row{BoardActionKind::Reset};
  bool practice=false;
};
struct SystemAttempt { unsigned challenge;SystemOutcome prediction;SystemReason reason;bool correct,assisted; };
struct SystemLessonView {
  unsigned example=0,challenge=0;
  bool practice=false,revealed=false,assisted=false,customized=false;
  bool hasPrediction=false,predictionCorrect=false,pointChecked=false,pointCorrect=false;
  PlaneVector point{};
  double pointResidual=0;
  MatrixBoardView board;
  std::optional<EquationSpace> solution;
  std::vector<SystemAttempt> attempts;
};
// Authored teaching examples and practice have separate retained boards. Both use
// MatrixBoard's row kernel. Only this owner checks practice predictions/points.
class SystemLesson {
public:
  SystemLesson();
  BoardResult dispatch(const SystemAction&);
  SystemLessonView view(bool practice=false)const;
  MatrixBoard& board(bool practice=false){return practice?practice_:explore_;}
  const MatrixBoard& board(bool practice=false)const{return practice?practice_:explore_;}
private:
  MatrixBoard explore_,practice_;
  unsigned example_=0,challenge_=0;
  bool revealed_=false,assisted_=false,customized_=false,hasPrediction_=false,predictionCorrect_=false,pointChecked_=false,pointCorrect_=false;
  std::array<bool,3> exposed_{};
  PlaneVector point_{};
  double pointResidual_=0;
  std::vector<SystemAttempt> attempts_;
  void startChallenge(unsigned);
};
}
