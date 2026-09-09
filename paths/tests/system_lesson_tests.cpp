#include "runtime/textbook/Textbook.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>

using namespace paths;
namespace {
unsigned assertions=0,systems=0,frames=0;std::size_t maxVertices=0,maxIndices=0;
void require(bool v,const char* why){++assertions;if(!v)throw std::runtime_error(why);}
void near(double a,double b,double eps,const char* why){require(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=eps,why);}
void act(SystemLesson& s,SystemAction a){const auto r=s.dispatch(a);if(!r.accepted)throw std::runtime_error(r.reason);}
void row(SystemLesson& s,BoardAction a){act(s,{SystemActionKind::RowOperation,0,0,0,a});}
double dot(PlaneVector a,PlaneVector b){double n=0;for(unsigned i=0;i<3;++i)n+=a[i]*b[i];return n;}
// Exact integer minors, independent of the production orthogonalization and row kernel.
unsigned exactRank(const std::array<long long,12>& a,unsigned cols){
  auto at=[&](unsigned r,unsigned c){return a[r*4+c];};unsigned rank=0;
  for(unsigned r=0;r<3;++r)for(unsigned c=0;c<cols;++c)if(at(r,c))rank=1;
  for(unsigned r=0;r<3;++r)for(unsigned s=r+1;s<3;++s)for(unsigned c=0;c<cols;++c)for(unsigned d=c+1;d<cols;++d)
    if(at(r,c)*at(s,d)-at(r,d)*at(s,c))rank=2;
  for(unsigned i=0;i<cols;++i)for(unsigned j=i+1;j<cols;++j)for(unsigned k=j+1;k<cols;++k){
    const auto det=at(0,i)*(at(1,j)*at(2,k)-at(1,k)*at(2,j))-at(0,j)*(at(1,i)*at(2,k)-at(1,k)*at(2,i))+at(0,k)*(at(1,i)*at(2,j)-at(1,j)*at(2,i));
    if(det)return 3;
  }return rank;
}
void certificate(const EquationSpace& v,const BoardMatrix& a,const BoardMatrix& b){
  require(v.available&&v.rank+v.dimension==3,"Invalid solution-space dimensions");
  for(unsigned k=0;k<v.dimension;++k)for(unsigned r=0;r<a.rows;++r){double sum=0,scale=1;for(unsigned c=0;c<3;++c){const auto t=a.at(r,c).real()*v.null[k][c];sum+=t;scale+=std::abs(t);}near(sum/scale,0,1e-9,"Null-space direction fails original equation");}
  if(v.consistent)for(unsigned r=0;r<a.rows;++r){double sum=0,scale=std::max(1.,std::abs(b.at(r,0).real()));for(unsigned c=0;c<3;++c){const double t=a.at(r,c).real()*v.particular[c];sum+=t;scale+=std::abs(t);}near((sum-b.at(r,0).real())/scale,0,1e-9,"Particular solution fails original equation");}
}
void mesh(RowPlaneFigure& f,MathObjectScene& scene){
  const auto& v=f.view();const auto& g=f.geometry();require(v.available,"Missing live figure");
  bool gold=false;for(unsigned i=0;i<g.partCount;++i)if(g.parts[i].role.starts_with("solution_"))gold=true;
  require(gold==v.consistent,"Empty solution set has gold geometry, or consistent set lost it");
  const auto& frame=scene.publish(g,{0,0,700,450});++frames;
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"Geometry capacity exceeded");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  for(const auto& vertex:frame.vertices)for(float x:vertex.position)require(std::isfinite(x),"Nonfinite geometry");
  for(auto index:frame.indices)require(index<frame.vertices.size(),"Invalid index");
  for(unsigned r=0;r<v.rows;++r)if(!v.planes[r].zero){const auto& p=v.planes[r];
    near(dot(p.normal,p.u),0,1e-10,"Plane grid axis is not tangent");near(dot(p.normal,p.v),0,1e-10,"Second grid axis is not tangent");
    if(v.consistent)near(dot(p.normal,v.probe),p.offset,1e-8*std::max(1.,std::abs(p.offset)),"Probe left affine solution set");
  }
}
void rejected(SystemLesson& lesson,SystemAction a){
  const auto before=lesson.view(),practice=lesson.view(true);require(!lesson.dispatch(a).accepted,"Invalid lesson action accepted");const auto after=lesson.view(),p=lesson.view(true);
  require(before.board.given.values==after.board.given.values&&before.board.rhs.values==after.board.rhs.values&&before.board.current.values==after.board.current.values&&before.board.steps==after.board.steps,"Rejected action changed exploration");
  require(practice.board.given.values==p.board.given.values&&practice.board.steps==p.board.steps&&practice.revealed==p.revealed&&practice.attempts.size()==p.attempts.size()&&practice.point==p.point,"Rejected action changed practice");
}
}
int main(){try{
  SystemLesson lesson;RowPlaneFigure figure;MathObjectScene scene;
  const std::array<unsigned,5> ranks{3,2,2,1,2};const std::array<bool,5> consistency{true,true,false,true,false};
  for(unsigned n=0;n<5;++n){
    act(lesson,{SystemActionKind::SelectExample,n});const auto initial=lesson.view();const auto solved=*initial.solution;++systems;
    require(solved.rank==ranks[n]&&solved.consistent==consistency[n],"Wrong authored classification");certificate(solved,initial.board.given,initial.board.rhs);
    for(double original:{0.,1.})for(double normals:{0.,1.})for(double density:{2.,4.}){
      require(figure.dispatch({RowPlaneActionKind::Set,RowPlaneParameter::Original,original}).accepted,"Cannot set original planes");
      require(figure.dispatch({RowPlaneActionKind::Set,RowPlaneParameter::Normals,normals}).accepted,"Cannot set normals");
      require(figure.dispatch({RowPlaneActionKind::Set,RowPlaneParameter::Density,density}).accepted,"Cannot set density");
      figure.publish(lesson.board().view());mesh(figure,scene);
    }
    const auto probe=figure.view().probe;
    for(const auto action:std::array<BoardAction,3>{{{BoardActionKind::AddRow,0,1,0,2.},{BoardActionKind::ScaleRow,0,0,0,-3.},{BoardActionKind::SwapRows,0,2}}}){
      row(lesson,action);figure.publish(lesson.board().view());require(figure.view().agrees,"Row operation changed the affine solution set");if(solved.consistent)for(unsigned c=0;c<3;++c)near(figure.view().probe[c],probe[c],1e-9,"Row operation made the solution probe jump");mesh(figure,scene);
    }
    for(unsigned i=0;i<3;++i)row(lesson,{BoardActionKind::Undo});require(lesson.board().view().rhs.values==initial.board.rhs.values&&!lesson.board().view().working,"Undo lost explicit givens");
    for(unsigned i=0;i<6&&!lesson.board().view().complete;++i){row(lesson,{BoardActionKind::Step});figure.publish(lesson.board().view());require(figure.view().agrees,"Guided reduction changed solution set");mesh(figure,scene);}
    require(lesson.board().view().complete,"Guided reduction did not terminate");
    require(lesson.board().dispatch({BoardActionKind::Check}).accepted&&lesson.board().view().passed,"Augmented transformation invariant failed");
    if(n==4){const auto v=lesson.board().view();require(std::abs(v.currentRhs.at(2,0))>.5,"Contradictory rhs disappeared");for(unsigned c=0;c<3;++c)near(std::abs(v.current.at(2,c)),0,1e-12,"Contradictory coefficient row not zero");}
    figure.publish(lesson.board().view());const auto revision=figure.geometry().revision;figure.publish(lesson.board().view());require(revision==figure.geometry().revision,"Unchanged affine view rebuilds");
  }
  // Editing only b must invalidate geometry and clear the old reduction.
  act(lesson,{SystemActionKind::SelectExample,1});figure.publish(lesson.board().view());const auto revision=figure.geometry().revision;
  row(lesson,{BoardActionKind::Step});act(lesson,{SystemActionKind::SetRightHandSide,2,0,3});figure.publish(lesson.board().view());
  require(!figure.view().consistent&&figure.geometry().revision>revision&&lesson.board().view().steps==0,"Rhs-only edit retained old solution/working");mesh(figure,scene);
  act(lesson,{SystemActionKind::SetRightHandSide,2,0,2});figure.publish(lesson.board().view());require(figure.view().consistent&&figure.view().dimension==1,"Restoring rhs failed to restore solution line");
  act(lesson,{SystemActionKind::SetCoefficient,2,2,2});require(lesson.view().solution->rank==3,"Coefficient editor did not change rank");
  // Exact minors check all rank/consistency outcomes across bounded integer systems.
  unsigned seed=991;auto random=[&](){seed=1664525u*seed+1013904223u;return static_cast<int>((seed>>16)%5)-2;};
  for(unsigned sample=0;sample<480;++sample){
    std::array<long long,12> exact{};BoardMatrix a(3,3),b(3,1);
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)exact[r*4+c]=random();
    if(sample%4==0)for(unsigned c=0;c<4;++c)exact[8+c]=exact[c]+exact[4+c];
    if(sample%5==0)for(unsigned c=0;c<3;++c)exact[8+c]=exact[c]+exact[4+c];
    if(sample%7==0)for(unsigned c=0;c<4;++c)exact[4+c]=2*exact[c];
    if(sample%11==0)for(unsigned c=0;c<3;++c)exact[c]=exact[4+c]=exact[8+c]=0;
    for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)a.at(r,c)=static_cast<double>(exact[r*4+c]);b.at(r,0)=static_cast<double>(exact[r*4+3]);}
    const auto result=equationSpace(a,b);const auto rank=exactRank(exact,3),augmented=exactRank(exact,4);++systems;
    require(result.rank==rank&&result.augmentedRank==augmented&&result.consistent==(rank==augmented),"Exact minor oracle disagrees with solution classification");certificate(result,a,b);
    if(sample<120){
      MatrixBoard board;require(board.loadSystem(a,b).accepted,"Cannot load integer system");figure.publish(board.view());mesh(figure,scene);
      for(unsigned step=0;step<5&&!board.view().complete;++step){
        require(board.dispatch({BoardActionKind::Step}).accepted,"Integer system reduction rejected");figure.publish(board.view());
        if(!figure.view().agrees||figure.view().rank!=rank||figure.view().consistent!=(rank==augmented)){
          std::fprintf(stderr,"sample=%u step=%u ranks=%u/%u expected=%u/%u consistent=%d\n",sample,step,figure.view().rank,figure.view().augmentedRank,rank,augmented,figure.view().consistent);
          throw std::runtime_error("Integer reduction changed the plane classification");
        }
        mesh(figure,scene);
      }
    }
  }
  // Cleanup must distinguish a scaled nonzero row from cancellation noise.
  act(lesson,{SystemActionKind::SelectExample,0});row(lesson,{BoardActionKind::ScaleRow,0,0,0,1e-9});
  figure.publish(lesson.board().view());require(figure.view().agrees&&figure.view().rank==3,"Roundoff cleanup discarded a deliberately small equation");
  require(std::abs(lesson.board().view().currentRhs.at(0,0))>0,"Scaled rhs disappeared");
  const auto before=lesson.board().view();row(lesson,{BoardActionKind::Undo});require(!lesson.board().view().working&&before.steps==1,"Cleanup broke Undo");
  BoardMatrix zero(3,3),rhs(3,1);const auto all=equationSpace(zero,rhs);require(all.consistent&&all.dimension==3,"Zero system should allow all of space");
  rhs.at(2,0)=1e-200;const auto impossible=equationSpace(zero,rhs);require(!impossible.consistent&&impossible.augmentedRank==1&&impossible.planes[2].contradiction,"Tiny nonzero contradiction was discarded");
  auto invalid=zero;invalid.at(0,0)={1,1};require(!equationSpace(invalid,rhs).available,"Complex coefficients projected into real space");
  invalid=zero;invalid.at(0,0)=std::numeric_limits<double>::infinity();require(!equationSpace(invalid,rhs).available,"Nonfinite coefficient accepted");
  MatrixBoard board;require(!board.loadSystem(invalid,rhs).accepted&&board.view().card==1,"Invalid custom-system load changed source board");
  require(!equationSpace(zero,BoardMatrix(2,1)).available,"Mismatched rhs shape accepted");
  // Malformed actions must preserve both retained boards and all practice evidence.
  for(const auto a:std::array<SystemAction,9>{{{SystemActionKind::SelectExample,99},{SystemActionKind::SetCoefficient,3,0,1},{SystemActionKind::SetCoefficient,0,0,.1},{SystemActionKind::SetRightHandSide,0,0,std::numeric_limits<double>::quiet_NaN()},{SystemActionKind::SelectChallenge,3},{SystemActionKind::Predict,3},{SystemActionKind::SetPoint,0,0,1},{SystemActionKind::RowOperation,0,0,0,{BoardActionKind::Step},true},{static_cast<SystemActionKind>(999)}}})rejected(lesson,a);
  auto hidden=lesson.view(true);require(!hidden.solution&&!hidden.revealed&&!hidden.hasPrediction&&hidden.attempts.empty(),"Fresh practice disclosed an answer");
  const auto explore=lesson.view();
  const std::array<SystemOutcome,3> outcomes{SystemOutcome::None,SystemOutcome::One,SystemOutcome::Infinite};
  const std::array<PlaneVector,3> points{{{0,0,0},{.5,.5,.5},{0,1,0}}};
  for(unsigned challenge=0;challenge<3;++challenge){
    act(lesson,{SystemActionKind::SelectChallenge,challenge});require(!lesson.view(true).solution,"Challenge selector exposed its solution");
    act(lesson,{SystemActionKind::Predict,static_cast<unsigned>(outcomes[challenge]),challenge});
    auto v=lesson.view(true);require(v.predictionCorrect&&v.solution&&v.attempts.back().correct&&!v.attempts.back().assisted,"Correct prediction not recorded independently");
    if(challenge){
      for(unsigned c=0;c<3;++c)act(lesson,{SystemActionKind::SetPoint,c,0,points[challenge][c]});
      act(lesson,{SystemActionKind::CheckPoint});require(lesson.view(true).pointCorrect,"Known solution point rejected");
      act(lesson,{SystemActionKind::SetPoint,0,0,3});require(!lesson.view(true).pointChecked,"Editing point retained old success");act(lesson,{SystemActionKind::CheckPoint});require(!lesson.view(true).pointCorrect,"Non-solution point accepted");
    }else rejected(lesson,{SystemActionKind::CheckPoint});
  }
  require(lesson.view().board.given.values==explore.board.given.values&&lesson.view().board.rhs.values==explore.board.rhs.values&&lesson.view().board.steps==explore.board.steps,"Practice modified exploration");
  act(lesson,{SystemActionKind::SelectChallenge,0});act(lesson,{SystemActionKind::Predict,0,0});require(lesson.view(true).attempts.back().assisted,"Restart erased previous answer exposure");
  act(lesson,{SystemActionKind::Predict,0,2});require(!lesson.view(true).predictionCorrect,"Correct outcome with wrong reason passed");
  act(lesson,{SystemActionKind::Reveal});const auto count=lesson.view(true).attempts.size();require(count==5,"Reveal fabricated an attempted prediction");
  // New reading references and navigation preserve both maths and redacted help.
  Textbook book;require(book.dispatch({BookActionKind::OpenSection,2}).accepted,"Cannot open systems section");
  require(matrixChapter()[2].figure.kind==BookFigureKind::AffinePlanes&&book.exerciseIndex()==6,"Wrong systems binding");
  std::set<std::string> ids;for(const auto& block:book.lessonView()){require(ids.insert(block.id).second&&!block.body.empty(),"Invalid systems reading block");for(const auto& h:block.help)require(!h.open&&h.passages.empty(),"Reading exposed unrequested help");}
  for(const auto& block:matrixChapter()[2].lesson)for(const auto& ref:block.references)require(ids.contains(ref.target),"Broken systems reference");
  for(const auto& term:matrixChapter()[2].terms)require(ids.contains(term.blockId),"Broken systems index reference");
  const auto bookmark=book.bookmark();require(book.dispatch({BookActionKind::Exercise}).accepted&&!book.systems().view(true).solution,"Exercise navigation reveals answer");
  require(book.dispatch({BookActionKind::Read}).accepted&&book.bookmark()==bookmark,"Mode change changed bookmark or givens");
  std::printf("systems lesson: %u assertions, %u independently checked systems, %u CPU mesh states; max %zu vertices / %zu indices. Explicit rhs, affine invariants, all outcomes, prediction disclosure and retained state passed. No images, fonts, native host or rasterization.\n",assertions,systems,frames,maxVertices,maxIndices);
  return 0;
}catch(const std::exception& e){std::fprintf(stderr,"systems lesson test: %s\n",e.what());return 1;}}
