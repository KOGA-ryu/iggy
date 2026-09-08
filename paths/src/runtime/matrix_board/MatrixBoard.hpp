#pragma once
#include <complex>
#include <string>
#include <vector>
#include <array>

namespace paths {
using MatrixScalar = std::complex<double>;
struct BoardMatrix {
  unsigned rows=0, cols=0;
  std::vector<MatrixScalar> values;
  BoardMatrix()=default;
  BoardMatrix(unsigned r,unsigned c):rows(r),cols(c),values(r*c){}
  MatrixScalar& at(unsigned r,unsigned c){return values.at(r*cols+c);}
  MatrixScalar at(unsigned r,unsigned c)const{return values.at(r*cols+c);}
};
struct MatrixCardSpec { unsigned id; const char* title; unsigned cases; const char* provenance; const char* task; };
const std::array<MatrixCardSpec,6>& matrixCards();
enum class BoardActionKind { Select, Configure, Step, SwapRows, ScaleRow, AddRow, Undo, Reset, Check };
struct BoardAction {
  BoardActionKind kind; unsigned first=0,second=0,third=0; MatrixScalar value=1.;
};
struct BoardResult { bool accepted; std::string reason; };
struct MatrixBoardView {
  unsigned card=1, example=0, size=6, bandwidth=1, partition=2, steps=0;
  bool working=false, complete=false, blocked=false, checked=false, passed=false;
  BoardMatrix given, rhs, current, currentRhs, lower, upper, permutation, schurBlock;
  bool hasLeadingDeterminant=false;
  MatrixScalar leadingDeterminant=0.;
  // One flag per given entry: a zero prescribed by the authored band condition.
  std::vector<bool> structuralZero;
  double residual=0, tolerance=1e-10;
  std::string status, checkKind, evidence;
};
class MatrixBoard {
public:
  static constexpr unsigned maxSize=32, maxHistory=128;
  MatrixBoard();
  BoardResult dispatch(const BoardAction& action);
  MatrixBoardView view()const;
private:
  struct State {
    BoardMatrix a,b,u,y,l,p,e;
    unsigned pivotRow=0,pivotCol=0;
    bool working=false,complete=false,blocked=false;
    std::string status;
  } state_;
  unsigned card_=1,example_=0,size_=6,band_=1,partition_=2;
  std::vector<State> history_;
  bool checked_=false,passed_=false;
  double residual_=0;
  std::string evidence_;
  void initialize();
  BoardResult mutate(const BoardAction&);
  void step();
  void check();
};
} // namespace paths
