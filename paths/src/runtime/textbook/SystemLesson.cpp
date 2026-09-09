#include "SystemLesson.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
constexpr std::array<SystemExample,5> examples{{
  {"point","One solution","Move a right-hand side and follow the shared point.",{1,1,0,0,1,1,1,0,1},{1,1,1}},
  {"line","Infinitely many / a line","The third equation repeats information from the first two.",{1,1,0,0,1,1,1,2,1},{1,1,2}},
  {"parallel","No solution / parallel planes","Move the second plane until it coincides with the first.",{1,0,0,1,0,0,0,1,0},{0,1,0}},
  {"plane","Infinitely many / a plane","All three equations describe the same plane.",{1,1,1,2,2,2,-1,-1,-1},{1,2,-1}},
  {"triangle","No solution / no common intersection","Each pair meets, but there is no point shared by all three.",{1,0,0,0,1,0,1,1,0},{0,0,1}}
}};
// Neutral challenge names and a different order avoid an answer-bearing selector.
constexpr std::array<unsigned,3> challenges{4,0,1};
void load(MatrixBoard& board,const SystemExample& e){
  BoardMatrix a(3,3),b(3,1);for(unsigned i=0;i<9;++i)a.values[i]=e.coefficients[i];for(unsigned i=0;i<3;++i)b.values[i]=e.rhs[i];
  const auto result=board.loadSystem(a,b);if(!result.accepted)throw std::logic_error(result.reason);
}
EquationSpace answer(const MatrixBoard& board){const auto v=board.view();return equationSpace(v.given,v.rhs);}
bool quarter(double x){return std::isfinite(x)&&x>=-3&&x<=3&&std::abs(x*4-std::round(x*4))<1e-10;}
}
std::span<const SystemExample> systemExamples(){return examples;}
SystemOutcome systemOutcome(const EquationSpace& s){return !s.consistent?SystemOutcome::None:s.dimension?SystemOutcome::Infinite:SystemOutcome::One;}
const char* systemOutcomeName(SystemOutcome o){switch(o){case SystemOutcome::None:return "No solution";case SystemOutcome::One:return "One solution";case SystemOutcome::Infinite:return "Infinitely many solutions";}return "Unknown";}
SystemLesson::SystemLesson(){load(explore_,examples[0]);startChallenge(0);}
void SystemLesson::startChallenge(unsigned n){
  challenge_=n;load(practice_,examples[challenges[n]]);revealed_=assisted_=hasPrediction_=predictionCorrect_=pointChecked_=pointCorrect_=false;point_={};pointResidual_=0;assisted_=exposed_[n];
}
BoardResult SystemLesson::dispatch(const SystemAction& a){
  switch(a.kind){
    case SystemActionKind::SelectExample:
      if(a.first>=examples.size())return {false,"Unknown teaching example."};
      example_=a.first;load(explore_,examples[example_]);customized_=false;return {true,{}};
    case SystemActionKind::SetCoefficient:
    case SystemActionKind::SetRightHandSide:{
      if(a.practice||a.first>=3||(a.kind==SystemActionKind::SetCoefficient&&a.second>=3)||!quarter(a.value))return {false,"Use quarter-step real coefficients and right-hand sides from -3 to 3."};
      auto v=explore_.view();auto& entry=a.kind==SystemActionKind::SetCoefficient?v.given.at(a.first,a.second):v.rhs.at(a.first,0);
      if(entry==a.value)return {true,{}};entry=a.value;
      const auto result=explore_.loadSystem(v.given,v.rhs);if(result.accepted)customized_=true;return result;
    }
    case SystemActionKind::ResetExploration:load(explore_,examples[example_]);customized_=false;return {true,{}};
    case SystemActionKind::RowOperation:
      if(a.row.value.imag()!=0)return {false,"This geometric lesson uses real row multipliers."};
      if(a.practice&&!revealed_)return {false,"Submit a prediction or reveal the explanation before reducing the practice system."};
      switch(a.row.kind){case BoardActionKind::Step:case BoardActionKind::Undo:case BoardActionKind::Reset:case BoardActionKind::SwapRows:case BoardActionKind::ScaleRow:case BoardActionKind::AddRow:break;default:return {false,"Use a row operation in this lesson."};}
      return board(a.practice).dispatch(a.row);
    case SystemActionKind::SelectChallenge:
      if(a.first>=challenges.size())return {false,"Unknown practice system."};startChallenge(a.first);return {true,{}};
    case SystemActionKind::Predict:{
      if(a.first>2||a.second>2)return {false,"Choose a prediction and its reason."};
      if(attempts_.size()>=128)return {false,"This session's prediction history is full."};
      const auto s=answer(practice_);const auto expected=systemOutcome(s);
      const auto reason=!s.consistent?SystemReason::Contradiction:s.dimension?SystemReason::FreeVariables:SystemReason::FullPivots;
      const bool correct=static_cast<SystemOutcome>(a.first)==expected&&static_cast<SystemReason>(a.second)==reason;
      attempts_.push_back({challenge_,static_cast<SystemOutcome>(a.first),static_cast<SystemReason>(a.second),correct,assisted_||revealed_});
      exposed_[challenge_]=true;revealed_=true;hasPrediction_=true;predictionCorrect_=correct;return {true,{}};
    }
    case SystemActionKind::Reveal:exposed_[challenge_]=true;revealed_=true;assisted_=true;return {true,{}};
    case SystemActionKind::SetPoint:
      if(!revealed_||a.first>=3||!std::isfinite(a.value)||std::abs(a.value)>8)return {false,"Reveal the practice result and use a coordinate between -8 and 8."};
      point_[a.first]=a.value;pointChecked_=pointCorrect_=false;pointResidual_=0;return {true,{}};
    case SystemActionKind::CheckPoint:{
      if(!revealed_)return {false,"Submit a prediction first."};
      const auto s=answer(practice_);if(!s.consistent)return {false,"An inconsistent system has no solution point to check."};
      const auto v=practice_.view();double error=0;
      for(unsigned r=0;r<3;++r){double sum=0,scale=std::max(1.,std::abs(v.rhs.at(r,0).real()));for(unsigned c=0;c<3;++c){const double term=v.given.at(r,c).real()*point_[c];sum+=term;scale+=std::abs(term);}error=std::max(error,std::abs(sum-v.rhs.at(r,0).real())/scale);}
      pointResidual_=error;pointChecked_=true;pointCorrect_=error<1e-8;return {true,{}};
    }
    default:return {false,"Unknown systems lesson action."};
  }
}
SystemLessonView SystemLesson::view(bool practice)const{
  SystemLessonView v;v.example=example_;v.challenge=challenge_;v.practice=practice;v.customized=customized_;v.board=board(practice).view();
  v.revealed=!practice||revealed_;if(v.revealed)v.solution=equationSpace(v.board.given,v.board.rhs);
  if(practice){v.assisted=assisted_;v.hasPrediction=hasPrediction_;v.predictionCorrect=predictionCorrect_;v.point=point_;v.pointChecked=pointChecked_;v.pointCorrect=pointCorrect_;v.pointResidual=pointResidual_;v.attempts=attempts_;}
  return v;
}
}
