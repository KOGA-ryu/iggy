#pragma once
#include "runtime/math_objects/MathObjects.hpp"
#include "runtime/matrix_board/MatrixBoard.hpp"
#include <string>

namespace paths {
enum class RowPlaneParameter : unsigned { Extent, Density, FocusRow, Original, Normals, Solution, Labels, ProbeS, ProbeT, ProbeU, Count };
struct RowPlaneParameterSpec { RowPlaneParameter id; const char* label; double minimum,maximum,initial; bool integer; };
std::span<const RowPlaneParameterSpec> rowPlaneParameters();
iggy3d::Vec3 rowPlaneColour(unsigned row);
enum class RowPlaneActionKind { Set, Reset };
struct RowPlaneAction { RowPlaneActionKind kind; RowPlaneParameter parameter=RowPlaneParameter::Extent; double value=0; };
using PlaneVector=std::array<double,3>;
struct EquationPlane { PlaneVector normal{},u{},v{}; bool zero=true; double offset=0; bool contradiction=false; };
struct EquationSpace {
  bool available=false,consistent=true;
  unsigned rank=0,dimension=0,augmentedRank=0;
  std::array<EquationPlane,3> planes{};
  std::array<PlaneVector,3> rows{},null{};
  PlaneVector particular{};
};
EquationSpace equationSpace(const BoardMatrix&,const BoardMatrix& rhs={});
struct RowPlaneView {
  bool available=false,working=false,agrees=false,consistent=true;
  unsigned rows=0,rank=0,dimension=0,example=0,steps=0,augmentedRank=0;
  std::array<EquationPlane,3> planes{},originalPlanes{};
  std::array<PlaneVector,3> basis{};
  PlaneVector probe{},particular{};
  std::array<double,9> coefficients{};
  std::array<double,3> rhs{};
  double displayScale=1;
  double agreement=0,probeResidual=0;
  std::string reason;
};
// A read-only geometric projection of the existing board, plus exploration settings.
// It never edits givens or creates a check/score. An explicit rhs produces affine planes.
class RowPlaneFigure {
public:
  RowPlaneFigure();
  BoardResult dispatch(RowPlaneAction);
  double parameter(RowPlaneParameter)const;
  const RowPlaneView& publish(const MatrixBoardView&);
  const RowPlaneView& view()const{return view_;}
  const MathObjectSnapshot& geometry()const{return geometry_;}
private:
  std::array<double,static_cast<unsigned>(RowPlaneParameter::Count)> parameters_{};
  BoardMatrix given_,current_,givenRhs_,currentRhs_;
  unsigned example_=~0u,steps_=~0u;
  bool dirty_=true;
  RowPlaneView view_;
  MathObjectSnapshot geometry_;
  void rebuild(const MatrixBoardView&);
};
} // namespace paths
