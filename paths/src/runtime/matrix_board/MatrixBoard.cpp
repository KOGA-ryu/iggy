#include "MatrixBoard.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace paths {
namespace {
constexpr double eps=1e-12;
BoardMatrix identity(unsigned n){BoardMatrix a(n,n);for(unsigned i=0;i<n;++i)a.at(i,i)=1.;return a;}
BoardMatrix matrix(unsigned r,unsigned c,std::initializer_list<double> v){BoardMatrix a(r,c);unsigned i=0;for(double x:v)a.values[i++]=x;return a;}
void swapRows(BoardMatrix& a,unsigned r,unsigned s){for(unsigned j=0;j<a.cols;++j)std::swap(a.at(r,j),a.at(s,j));}
void scaleRow(BoardMatrix& a,unsigned r,MatrixScalar x){for(unsigned j=0;j<a.cols;++j)a.at(r,j)*=x;}
void addRow(BoardMatrix& a,unsigned r,unsigned s,MatrixScalar x){for(unsigned j=0;j<a.cols;++j)a.at(r,j)+=x*a.at(s,j);}
BoardMatrix multiply(const BoardMatrix& a,const BoardMatrix& b){
  if(a.cols!=b.rows)throw std::logic_error("Matrix product shape mismatch");
  BoardMatrix c(a.rows,b.cols);for(unsigned i=0;i<a.rows;++i)for(unsigned k=0;k<a.cols;++k)for(unsigned j=0;j<b.cols;++j)c.at(i,j)+=a.at(i,k)*b.at(k,j);return c;
}
double difference(const BoardMatrix& a,const BoardMatrix& b){
  if(a.rows!=b.rows||a.cols!=b.cols)return std::numeric_limits<double>::infinity();
  double d=0,scale=1;for(unsigned i=0;i<a.values.size();++i){d=std::max(d,std::abs(a.values[i]-b.values[i]));scale=std::max(scale,std::abs(b.values[i]));}return d/scale;
}
bool finite(const BoardMatrix& a){return std::all_of(a.values.begin(),a.values.end(),[](auto x){return std::isfinite(x.real())&&std::isfinite(x.imag())&&std::abs(x)<1e100;});}
MatrixScalar leadingDeterminant(const BoardMatrix& a,unsigned n){
  BoardMatrix t(n,n);for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j)t.at(i,j)=a.at(i,j);
  MatrixScalar determinant=1.;
  for(unsigned k=0;k<n;++k){unsigned p=k;for(unsigned i=k+1;i<n;++i)if(std::abs(t.at(i,k))>std::abs(t.at(p,k)))p=i;
    if(std::abs(t.at(p,k))<=eps)return 0.;if(p!=k){swapRows(t,p,k);determinant=-determinant;}
    determinant*=t.at(k,k);for(unsigned i=k+1;i<n;++i)addRow(t,i,k,-t.at(i,k)/t.at(k,k));
  }return determinant;
}
bool rref(const BoardMatrix& a){
  unsigned last=0;bool first=true,zeroSeen=false;
  for(unsigned i=0;i<a.rows;++i){unsigned p=0;while(p<a.cols&&std::abs(a.at(i,p))<1e-10)++p;
    if(p==a.cols){zeroSeen=true;continue;}if(zeroSeen||(!first&&p<=last)||std::abs(a.at(i,p)-1.)>1e-10)return false;
    for(unsigned r=0;r<a.rows;++r)if(r!=i&&std::abs(a.at(r,p))>1e-10)return false;first=false;last=p;
  }return true;
}
// Independent block calculation: solve A11 X=A12 with pivoted Gauss-Jordan,
// then form A22-A21 X. No explicit inverse and no scalar-LU state is reused.
BoardMatrix schur(const BoardMatrix& a,unsigned n){
  BoardMatrix t(n,a.cols);for(unsigned i=0;i<n;++i)for(unsigned j=0;j<a.cols;++j)t.at(i,j)=a.at(i,j);
  for(unsigned k=0;k<n;++k){unsigned p=k;for(unsigned i=k+1;i<n;++i)if(std::abs(t.at(i,k))>std::abs(t.at(p,k)))p=i;
    if(std::abs(t.at(p,k))<=eps)throw std::runtime_error("Singular leading block");swapRows(t,p,k);scaleRow(t,k,1./t.at(k,k));for(unsigned i=0;i<n;++i)if(i!=k)addRow(t,i,k,-t.at(i,k));}
  BoardMatrix s(a.rows-n,a.cols-n);for(unsigned i=n;i<a.rows;++i)for(unsigned j=n;j<a.cols;++j){s.at(i-n,j-n)=a.at(i,j);for(unsigned k=0;k<n;++k)s.at(i-n,j-n)-=a.at(i,k)*t.at(k,j);}return s;
}
}
const std::array<MatrixCardSpec,6>& matrixCards(){
  static const std::array<MatrixCardSpec,6> cards{{
    {1,"001 / Banded LU",1,"Chosen complex, strictly diagonally dominant example; not a printed matrix.","Explore unpivoted LU under the leading-block hypothesis. A finite example is not a proof of the sparsity claim."},
    {4,"004 / Six RREF matrices",6,"Exact printed matrices (a)-(f). Every column belongs to A; none is an augmented right-hand side.","Use row operations to produce reduced row echelon form."},
    {18,"018 / Leading blocks",2,"Chosen examples: dominant complex matrix; invertible matrix with a zero first pivot.","Inspect nested leading blocks and unpivoted elimination. Numerical evidence does not prove existence or uniqueness in general."},
    {31,"031 / Partial pivoting",1,"Chosen invertible real system with planted x=(1,...,1); b=A x. Not printed source data.","Step through partial pivoting, pivot-row normalization and downward elimination. The source calls this Forward-Substitution."},
    {44,"044 / Pivoted LU",2,"Exact printed matrices (a)-(b). Current source convention: PA=LU, with row permutations.","Build unit-lower L, upper U and permutation P. Record earlier L entries when later rows swap."},
    {59,"059 / Schur complement",1,"Chosen complex, strictly diagonally dominant example. All leading blocks are nonsingular.","Choose a block cut n. Compare n scalar elimination steps with A22-A21 solve(A11,A12). Numerical agreement is not a general proof."}
  }};return cards;
}
MatrixBoard::MatrixBoard(){initialize();}
BoardResult MatrixBoard::loadSystem(const BoardMatrix& a,const BoardMatrix& b){
  if(a.rows<1||a.rows>3||a.cols!=3||a.values.size()!=a.rows*3||b.rows!=a.rows||b.cols!=1||b.values.size()!=b.rows)
    return {false,"A lesson system requires one to three real equations in three variables and an explicit right-hand side."};
  for(const auto* m:{&a,&b})for(const auto z:m->values)
    if(!std::isfinite(z.real())||z.imag()!=0||std::abs(z.real())>100)return {false,"Lesson givens must be finite real numbers with magnitude at most 100."};
  customSystem_=true;card_=0;example_=0;systemA_=a;systemB_=b;initialize();return {true,{}};
}
void MatrixBoard::initialize(){
  state_=State{};history_.clear();checked_=passed_=false;residual_=0;evidence_.clear();
  auto& s=state_;
  switch(card_){
    case 0:s.a=systemA_;s.b=systemB_;break;
    case 4: {
      static const std::array<BoardMatrix,6> a{{matrix(2,3,{-2,0,1,1,3,-4}),matrix(3,5,{2,3,4,-1,1,1,0,-1,0,3,2,2,2,-1,2}),matrix(3,2,{-2,1,0,3,1,-4}),matrix(3,3,{1,2,-3,4,-5,6,7,-8,9}),matrix(4,4,{1,1,1,1,1,2,2,2,1,2,3,3,1,2,3,4}),matrix(4,5,{-1,2,1,0,1,2,3,1,-1,1,-1,-2,0,2,-1,2,2,1,0,1})}};
      s.a=a[example_];break;
    }
    case 44:s.a=example_==0?matrix(2,2,{0,2,-1,4}):matrix(3,3,{1,2,-2,3,6,-4,1,5,-5});break;
    case 1:case 18:case 31:case 59:{
      s.a=BoardMatrix(size_,size_);
      for(unsigned i=0;i<size_;++i){double sum=0;for(unsigned j=0;j<size_;++j){if(i==j)continue;if(card_==1&&static_cast<unsigned>(std::abs(static_cast<int>(i)-static_cast<int>(j)))>band_)continue;
        const MatrixScalar z{.15*(1+(i+2*j)%3),card_==31?0.:.1*(static_cast<int>((i+j)%3)-1)};s.a.at(i,j)=z;sum+=std::abs(z);}s.a.at(i,i)=sum+2.;}
      if((card_==18&&example_==1)||card_==31){s.a.at(0,0)=0;s.a.at(0,1)=1;s.a.at(1,0)=1;for(unsigned j=2;j<size_;++j){s.a.at(0,j)=0;s.a.at(1,j)=0;}for(unsigned i=2;i<size_;++i){s.a.at(i,0)=0;s.a.at(i,1)=0;}}
      if(card_==31){s.b=BoardMatrix(size_,1);for(unsigned i=0;i<size_;++i)for(unsigned j=0;j<size_;++j)s.b.at(i,0)+=s.a.at(i,j);}
      break;
    }
    default:throw std::logic_error("Unknown authored card");
  }
  s.u=s.a;s.y=s.b;s.l=s.p=s.e=identity(s.a.rows);s.status="Setup: givens only. Choose a step or a row operation to begin.";
}
BoardResult MatrixBoard::dispatch(const BoardAction& action){
  // Commit only a fully valid candidate, including all state/history/feedback.
  MatrixBoard candidate=*this;const auto result=candidate.mutate(action);if(!result.accepted)return result;
  if(candidate.customSystem_)switch(action.kind){
    case BoardActionKind::Step:case BoardActionKind::SwapRows:case BoardActionKind::ScaleRow:case BoardActionKind::AddRow:candidate.cleanSystemRoundoff();break;
    default:break;
  }
  for(const auto* a:{&candidate.state_.u,&candidate.state_.y,&candidate.state_.l,&candidate.state_.p,&candidate.state_.e})if(!finite(*a))return {false,"Operation exceeds the finite numeric range; no state changed."};
  *this=std::move(candidate);return result;
}
void MatrixBoard::cleanSystemRoundoff(){
  // The recorded row transform supplies a scale for cancellation in each entry.
  // A small but deliberately scaled equation survives: its bound scales with E.
  // This bounded lesson policy never changes printed source-card matrices.
  auto& s=state_;bool cleaned=false;
  const double factor=8*static_cast<double>(history_.size()+1)*std::numeric_limits<double>::epsilon();
  const auto clean=[&](BoardMatrix& current,const BoardMatrix& given){
    for(unsigned r=0;r<current.rows;++r)for(unsigned c=0;c<current.cols;++c){
      double magnitude=0;for(unsigned k=0;k<given.rows;++k)magnitude+=std::abs(s.e.at(r,k))*std::abs(given.at(k,c));
      if(current.at(r,c)!=MatrixScalar{}&&std::isfinite(magnitude)&&std::abs(current.at(r,c))<=factor*magnitude){current.at(r,c)=0.;cleaned=true;}
    }
  };
  clean(s.u,s.a);clean(s.y,s.b);
  if(cleaned)s.status+=" Tiny cancellation residues were treated as zero using the recorded row-transform scale.";
}
BoardResult MatrixBoard::mutate(const BoardAction& a){
  switch(a.kind){
    case BoardActionKind::Select:{auto it=std::find_if(matrixCards().begin(),matrixCards().end(),[&](auto c){return c.id==a.first;});if(it==matrixCards().end()||a.second>=it->cases)return {false,"Unknown card or example."};customSystem_=false;card_=a.first;example_=a.second;initialize();return {true,{}};}
    case BoardActionKind::Configure:
      if(customSystem_||card_==4||card_==44)return {false,"This example has fixed matrix dimensions."};
      if(a.first<2||a.first>maxSize||a.second>=a.first||a.third<1||(card_==18?a.third>a.first:a.third>=a.first))return {false,"Require 2<=size<=32, 0<=band<size, 1<=cut<size (leading-block inspection also permits cut=size)."};
      size_=a.first;band_=a.second;partition_=a.third;initialize();return {true,{}};
    case BoardActionKind::Reset:initialize();return {true,{}};
    case BoardActionKind::Undo:if(history_.empty())return {false,"No operation to undo."};state_=std::move(history_.back());history_.pop_back();checked_=passed_=false;evidence_.clear();residual_=0;return {true,{}};
    case BoardActionKind::Check:if(!state_.working)return {false,"Begin working before checking."};check();return {true,{}};
    case BoardActionKind::Step:if(state_.complete||state_.blocked)return {false,"This trace is complete or blocked. Undo or reset to continue."};break;
    case BoardActionKind::SwapRows:case BoardActionKind::ScaleRow:case BoardActionKind::AddRow:
      if(!customSystem_&&card_!=4&&card_!=31)return {false,"Manual row operations are available on cards 004 and 031 and authored systems."};
      if(a.first>=state_.u.rows||a.second>=state_.u.rows)return {false,"Row index out of bounds."};
      if(!std::isfinite(a.value.real())||!std::isfinite(a.value.imag())||std::abs(a.value)>1e6)return {false,"Multiplier must be finite with magnitude <= 1e6."};
      if(a.kind==BoardActionKind::ScaleRow&&std::abs(a.value)<=eps)return {false,"A row scale must be nonzero (magnitude > 1e-12)."};
      if(a.kind!=BoardActionKind::ScaleRow&&a.first==a.second)return {false,"Choose two distinct rows."};break;
    default:return {false,"Unknown matrix-board action."};
  }
  if(history_.size()>=maxHistory)return {false,"Operation history is full; undo or reset."};
  history_.push_back(state_);checked_=passed_=false;evidence_.clear();residual_=0;state_.working=true;
  if(a.kind==BoardActionKind::Step){step();return {true,{}};}
  for(auto* m:{&state_.u,&state_.y,&state_.e}){if(m->rows==0)continue;switch(a.kind){
    case BoardActionKind::SwapRows:swapRows(*m,a.first,a.second);break;
    case BoardActionKind::ScaleRow:scaleRow(*m,a.first,a.value);break;
    case BoardActionKind::AddRow:addRow(*m,a.first,a.second,a.value);break;
    default:break;
  }}
  // Restart a later guided sweep from the learner's current matrix, not stale pivots.
  state_.pivotRow=state_.pivotCol=0;state_.complete=state_.blocked=false;state_.status="Applied row operation to the matrix, transformation record, and any right-hand side.";return {true,{}};
}
void MatrixBoard::step(){
  auto& s=state_;unsigned r=s.pivotRow,c=s.pivotCol;
  const bool reduction=customSystem_||card_==4,normalized=card_==31,pivoting=reduction||normalized||card_==44;
  if(reduction){while(c<s.u.cols){unsigned p=r;for(unsigned i=r;i<s.u.rows;++i)if(std::abs(s.u.at(i,c))>std::abs(s.u.at(p,c)))p=i;if(std::abs(s.u.at(p,c))>eps)break;++c;}}
  if(r>=s.u.rows||c>=s.u.cols){s.complete=true;s.status="Elimination trace complete.";return;}
  unsigned p=r;if(pivoting)for(unsigned i=r+1;i<s.u.rows;++i)if(std::abs(s.u.at(i,c))>std::abs(s.u.at(p,c)))p=i;
  if(std::abs(s.u.at(p,c))<=eps){s.blocked=true;s.status="Pivot magnitude <= 1e-12. This unpivoted trace cannot proceed; no division performed.";return;}
  if(p!=r){swapRows(s.u,p,r);swapRows(s.e,p,r);swapRows(s.p,p,r);if(s.y.rows)swapRows(s.y,p,r);for(unsigned j=0;j<r;++j)std::swap(s.l.at(p,j),s.l.at(r,j));}
  if(reduction||normalized){const auto scale=1./s.u.at(r,c);scaleRow(s.u,r,scale);scaleRow(s.e,r,scale);if(s.y.rows)scaleRow(s.y,r,scale);}
  for(unsigned i=reduction?0:r+1;i<s.u.rows;++i){if(i==r)continue;const auto factor=s.u.at(i,c)/s.u.at(r,c);addRow(s.u,i,r,-factor);addRow(s.e,i,r,-factor);if(s.y.rows)addRow(s.y,i,r,-factor);if(!reduction&&!normalized)s.l.at(i,r)=factor;}
  s.pivotRow=r+1;s.pivotCol=c+1;s.complete=s.pivotRow>=s.u.rows||s.pivotCol>=s.u.cols||(card_==59&&s.pivotRow>=partition_);
  s.status="Pivot column "+std::to_string(c+1)+": "+(p!=r?"swapped rows "+std::to_string(r+1)+" and "+std::to_string(p+1)+"; ":"")+(reduction?"normalized and eliminated above/below.":normalized?"normalized and eliminated below.":"eliminated below without scaling the pivot row.");
}
void MatrixBoard::check(){
  const auto& s=state_;checked_=true;residual_=difference(multiply(s.e,s.a),s.u);bool goal=false;
  switch(card_){
    case 0:residual_=std::max(residual_,difference(multiply(s.e,s.b),s.y));goal=rref(s.u);evidence_="E*A equals the reduced coefficients and E*b equals the working right-hand side. Coefficient pivots are reduced; a zero coefficient row with nonzero rhs is a contradiction.";break;
    case 4:goal=rref(s.u);evidence_="Invariant check: E*A equals the working matrix; independently test the RREF definition. This is not a source-page solve record.";break;
    case 31:{
      residual_=std::max(residual_,difference(multiply(s.e,s.b),s.y));double shape=0,plant=0;
      for(unsigned i=0;i<s.u.rows;++i){MatrixScalar sum=0;for(unsigned j=0;j<s.u.cols;++j){sum+=s.u.at(i,j);if(i>j)shape=std::max(shape,std::abs(s.u.at(i,j)));}plant=std::max(plant,std::abs(sum-s.y.at(i,0)));shape=std::max(shape,std::abs(s.u.at(i,i)-1.));}
      residual_=std::max(residual_,plant);goal=shape<1e-10;evidence_="Planted + invariant: U*(1,...,1)=y, E*A=U, E*b=y; normalized upper-triangular form. One chosen system.";break;
    }
    case 1:case 18:case 44:{
      const auto expected=card_==44?multiply(s.p,s.a):s.a;residual_=std::max(residual_,difference(multiply(s.l,s.u),expected));goal=s.complete&&!s.blocked;
      if(card_==1){double outside=0;for(unsigned i=0;i<s.u.rows;++i)for(unsigned j=0;j<s.u.cols;++j)if(static_cast<unsigned>(std::abs(static_cast<int>(i)-static_cast<int>(j)))>band_)outside=std::max({outside,std::abs(s.l.at(i,j)),std::abs(s.u.at(i,j))});residual_=std::max(residual_,outside);}
      evidence_=card_==44?"Invariant: PA=LU, with unit-lower L and upper U. Recorded row permutations include earlier L columns.":"Invariant: A=LU for this chosen example; no general existence, uniqueness or sparsity proof is graded.";
      if(s.blocked)evidence_="Observed unpivoted breakdown in this chosen example. A zero leading pivot does not imply that A is singular.";break;
    }
    case 59:{
      BoardMatrix trailing(s.a.rows-partition_,s.a.cols-partition_);for(unsigned i=partition_;i<s.u.rows;++i)for(unsigned j=partition_;j<s.u.cols;++j)trailing.at(i-partition_,j-partition_)=s.u.at(i,j);
      residual_=std::max(residual_,difference(trailing,schur(s.a,partition_)));goal=s.complete&&!s.blocked;evidence_="Two methods: scalar elimination versus a separate pivoted block solve and product. Compare only the trailing block; the top-left blocks need not match. Numeric example, not proof.";break;
    }
  }
  passed_=goal&&std::isfinite(residual_)&&residual_<=1e-10;
}
MatrixBoardView MatrixBoard::view()const{
  MatrixBoardView v;v.card=card_;v.example=example_;v.size=size_;v.bandwidth=band_;v.partition=partition_;v.steps=static_cast<unsigned>(history_.size());v.given=state_.a;v.rhs=state_.b;v.working=state_.working;v.complete=state_.complete;v.blocked=state_.blocked;v.checked=checked_;v.passed=passed_;v.status=state_.status;
  v.structuralZero.resize(v.given.values.size());if(card_==1)for(unsigned i=0;i<v.given.rows;++i)for(unsigned j=0;j<v.given.cols;++j)v.structuralZero[i*v.given.cols+j]=static_cast<unsigned>(std::abs(static_cast<int>(i)-static_cast<int>(j)))>band_;
  if(v.working){v.current=state_.u;v.currentRhs=state_.y;if(v.complete&&(card_==1||card_==18||card_==44)){v.lower=state_.l;v.upper=state_.u;if(card_==44)v.permutation=state_.p;}}
  if(v.complete&&card_==59){v.schurBlock=BoardMatrix(state_.u.rows-partition_,state_.u.cols-partition_);for(unsigned i=partition_;i<state_.u.rows;++i)for(unsigned j=partition_;j<state_.u.cols;++j)v.schurBlock.at(i-partition_,j-partition_)=state_.u.at(i,j);}
  if(v.checked&&card_==18){v.hasLeadingDeterminant=true;v.leadingDeterminant=leadingDeterminant(state_.a,partition_);}
  if(v.checked){v.residual=residual_;v.evidence=evidence_;switch(card_){case 31:v.checkKind="planted + invariant";break;case 59:v.checkKind="two methods";break;default:v.checkKind="invariant";break;}}
  return v;
}
} // namespace paths
