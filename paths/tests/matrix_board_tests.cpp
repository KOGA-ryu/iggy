#include "runtime/matrix_board/MatrixBoard.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
using namespace paths;
namespace {
unsigned checks=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void act(MatrixBoard& b,BoardAction a){auto r=b.dispatch(a);if(!r.accepted)throw std::runtime_error(r.reason);}
void finish(MatrixBoard& b){for(unsigned i=0;i<65&&!b.view().complete&&!b.view().blocked;++i)act(b,{BoardActionKind::Step});act(b,{BoardActionKind::Check});}
void equal(const BoardMatrix& a,const BoardMatrix& b){require(a.rows==b.rows&&a.cols==b.cols,"shape changed");require(a.values==b.values,"matrix changed");}
void same(const MatrixBoardView& a,const MatrixBoardView& b){equal(a.given,b.given);equal(a.rhs,b.rhs);equal(a.current,b.current);equal(a.currentRhs,b.currentRhs);equal(a.lower,b.lower);equal(a.upper,b.upper);equal(a.permutation,b.permutation);equal(a.schurBlock,b.schurBlock);require(a.hasLeadingDeterminant==b.hasLeadingDeterminant&&a.leadingDeterminant==b.leadingDeterminant,"measurement changed");require(a.steps==b.steps&&a.card==b.card&&a.example==b.example&&a.size==b.size&&a.bandwidth==b.bandwidth&&a.partition==b.partition,"identity/history changed");require(a.checked==b.checked&&a.passed==b.passed&&a.working==b.working&&a.complete==b.complete&&a.blocked==b.blocked&&a.status==b.status&&a.evidence==b.evidence&&a.residual==b.residual,"feedback changed");}
void reject(MatrixBoard& b,BoardAction a){const auto before=b.view();require(!b.dispatch(a).accepted,"invalid action accepted");same(before,b.view());}
void factors(const MatrixBoardView& v){
  const auto n=v.given.rows;require(v.lower.rows==n&&v.upper.rows==n,"missing factors");
  for(unsigned i=0;i<n;++i){require(std::abs(v.lower.at(i,i)-1.)<1e-10,"L diagonal");for(unsigned j=0;j<n;++j){if(i<j)require(std::abs(v.lower.at(i,j))<1e-10,"L shape");if(i>j)require(std::abs(v.upper.at(i,j))<1e-10,"U shape");MatrixScalar lhs=0,rhs=0;for(unsigned k=0;k<n;++k){lhs+=v.lower.at(i,k)*v.upper.at(k,j);if(v.permutation.rows)rhs+=v.permutation.at(i,k)*v.given.at(k,j);}if(!v.permutation.rows)rhs=v.given.at(i,j);require(std::abs(lhs-rhs)<1e-8,"factor reconstruction");}}
}
}
int main(){try{
  MatrixBoard b;
  for(const auto card:matrixCards())for(unsigned part=0;part<card.cases;++part){
    act(b,{BoardActionKind::Select,card.id,part});const auto setup=b.view();require(!setup.working&&!setup.checked&&setup.current.values.empty()&&setup.lower.values.empty()&&setup.upper.values.empty()&&setup.permutation.values.empty()&&setup.schurBlock.values.empty()&&!setup.hasLeadingDeterminant&&setup.evidence.empty(),"setup leaked derived entries");reject(b,{BoardActionKind::Check});
    act(b,{BoardActionKind::Step});act(b,{BoardActionKind::Undo});same(setup,b.view());
    finish(b);const auto v=b.view();if(card.id==18&&part==1){require(v.blocked&&!v.passed,"zero first pivot did not block");require(v.given.at(0,1)==MatrixScalar(1)&&v.given.at(1,0)==MatrixScalar(1),"counterexample is not invertible block");}else{require(v.passed,"card check failed");if(card.id==1||card.id==18||card.id==44)factors(v);}
    reject(b,{static_cast<BoardActionKind>(999)});reject(b,{BoardActionKind::Select,95});reject(b,{BoardActionKind::Select,card.id,99});reject(b,{BoardActionKind::Configure,33,1,1});reject(b,{BoardActionKind::Configure,3,3,1});reject(b,{BoardActionKind::Configure,3,1,4});
  }
  act(b,{BoardActionKind::Select,18,1});act(b,{BoardActionKind::Configure,5,1,1});finish(b);require(b.view().hasLeadingDeterminant&&b.view().leadingDeterminant==MatrixScalar(0),"zero leading determinant");
  act(b,{BoardActionKind::Configure,5,1,2});finish(b);require(std::abs(b.view().leadingDeterminant+1.)<1e-12,"invertible two by two corner determinant");
  act(b,{BoardActionKind::Configure,5,1,5});finish(b);require(std::abs(b.view().leadingDeterminant)>1,"full determinant of invertible breakdown example");
  // Expected RREFs independently generated with Python Fraction arithmetic.
  const std::array<std::vector<double>,6> expected{{
    {1,0,-.5,0,1,-7./6},{1,0,-1,0,3,0,1,2,0,-1,0,0,0,1,2},{1,0,0,1,0,0},
    {1,0,0,0,1,0,0,0,1},{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,-1,0,0,0,1,2,0,0,0,0,0,1}}};
  for(unsigned p=0;p<6;++p){act(b,{BoardActionKind::Select,4,p});finish(b);const auto v=b.view();require(v.current.values.size()==expected[p].size(),"RREF fixture shape");for(unsigned i=0;i<expected[p].size();++i)require(std::abs(v.current.values[i]-expected[p][i])<1e-9,"independent exact RREF mismatch");}
  for(unsigned card:{1U,18U,31U,59U})for(unsigned n:{2U,5U,12U,32U})for(unsigned band:{0U,1U,n-1})for(unsigned cut:{1U,n/2,n-1}){
    act(b,{BoardActionKind::Select,card});act(b,{BoardActionKind::Configure,n,band,cut});const auto setup=b.view();require(setup.given.rows==n&&setup.given.cols==n,"size clamped or projected");
    if(card==1)for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){bool zero=static_cast<unsigned>(std::abs(static_cast<int>(i)-static_cast<int>(j)))>band;require(setup.structuralZero[i*n+j]==zero,"band mask wrong");if(zero)require(setup.given.at(i,j)==MatrixScalar(0),"band input nonzero");}
    finish(b);require(b.view().passed,"parameterized check failed");if(card==1||card==18)factors(b.view());
  }
  for(unsigned card:{4U,31U}){
    act(b,{BoardActionKind::Select,card});auto before=b.view();act(b,{BoardActionKind::SwapRows,0,1});auto after=b.view();for(unsigned j=0;j<before.given.cols;++j)require(after.current.at(0,j)==before.given.at(1,j),"swap mismatch");if(card==31)require(after.currentRhs.at(0,0)==before.rhs.at(1,0),"rhs did not swap");
    act(b,{BoardActionKind::Undo});same(before,b.view());act(b,{BoardActionKind::ScaleRow,0,0,0,{2,1}});after=b.view();for(unsigned j=0;j<before.given.cols;++j)require(after.current.at(0,j)==MatrixScalar(2,1)*before.given.at(0,j),"complex scaling");if(card==31)require(after.currentRhs.at(0,0)==MatrixScalar(2,1)*before.rhs.at(0,0),"rhs scaling");
    before=after;act(b,{BoardActionKind::AddRow,1,0,0,{-1,.5}});after=b.view();if(card==31)require(after.currentRhs.at(1,0)==before.currentRhs.at(1,0)+MatrixScalar(-1,.5)*before.currentRhs.at(0,0),"rhs addition");finish(b);require(b.view().passed,"manual followed by guided sweep");
    reject(b,{BoardActionKind::SwapRows,99,1});reject(b,{BoardActionKind::AddRow,0,0});reject(b,{BoardActionKind::ScaleRow,0,0,0,0.});reject(b,{BoardActionKind::ScaleRow,0,0,0,{std::numeric_limits<double>::quiet_NaN(),0}});
  }
  act(b,{BoardActionKind::Select,4});for(unsigned k=0;k<MatrixBoard::maxHistory;++k)act(b,{BoardActionKind::ScaleRow,0,0,0,1.});reject(b,{BoardActionKind::ScaleRow,0,0,0,1.});
  act(b,{BoardActionKind::Reset});for(unsigned k=0;k<16;++k)act(b,{BoardActionKind::ScaleRow,0,0,0,1e6});reject(b,{BoardActionKind::ScaleRow,0,0,0,1e6});
  std::printf("matrix_board: %u assertions passed; six adapters, all printed parts, four sizes, band/cut sweeps, exact RREF, complex row operations, atomic failures, disclosure and undo.\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"matrix_board test: %s\n",e.what());return 1;}}
