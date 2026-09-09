#include "runtime/textbook/RowPlaneFigure.hpp"
#include "runtime/textbook/LessonSpread.hpp"
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
unsigned checks=0,frames=0,layouts=0;std::size_t maxVertices=0,maxIndices=0;
void require(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void near(double a,double b,double eps,const char* message){require(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=eps,message);}
double dot(PlaneVector a,PlaneVector b){double sum=0;for(unsigned i=0;i<3;++i)sum+=a[i]*b[i];return sum;}
MatrixBoardView example(unsigned rows,std::initializer_list<double> values){MatrixBoardView v;v.card=4;v.given=BoardMatrix(rows,3);unsigned i=0;for(double x:values)v.given.values.at(i++)=x;return v;}
void set(RowPlaneFigure& f,RowPlaneParameter p,double v){require(f.dispatch({RowPlaneActionKind::Set,p,v}).accepted,"Valid setting rejected");}
void step(MatrixBoard& b,BoardAction a){require(b.dispatch(a).accepted,"Valid row operation rejected");}
void math(const RowPlaneView& v){
  require(v.available&&v.dimension+v.rank==3,"Rank/nullity does not fill the three variable directions");
  for(unsigned r=0;r<v.rows;++r){const auto& p=v.planes[r];if(p.zero)continue;
    near(dot(p.normal,p.normal),1,1e-12,"Row normal is not unit length");near(dot(p.u,p.u),1,1e-12,"Plane basis not unit");near(dot(p.v,p.v),1,1e-12,"Plane basis not unit");
    near(dot(p.normal,p.u),0,1e-12,"Grid leaves its equation plane");near(dot(p.normal,p.v),0,1e-12,"Grid leaves its equation plane");near(dot(p.u,p.v),0,1e-12,"Grid axes are not perpendicular");
    for(unsigned j=0;j<v.dimension;++j)near(dot(p.normal,v.basis[j]),0,2e-8,"Solution basis violates a row equation");
    near(dot(p.normal,v.probe),0,1e-7,"Probe leaves the common solution set");
  }
  for(unsigned i=0;i<v.dimension;++i)for(unsigned j=0;j<v.dimension;++j)near(dot(v.basis[i],v.basis[j]),i==j?1:0,1e-12,"Null-space basis is not orthonormal");
}
void mesh(RowPlaneFigure& f,MathObjectScene& scene){
  const auto& g=f.geometry();const auto& frame=scene.publish(g,{20,30,700,450});++frames;
  require(g.partCount>0&&g.partCount<=g.parts.size(),"Invalid primitive count");require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"Figure exceeds native mesh capacity");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  std::set<std::uint32_t> ids;std::size_t end=0;
  for(const auto& d:frame.draws){require(ids.insert(d.objectId.value).second,"Duplicate scene ID");require(d.firstIndex==end&&d.indexCount%3==0,"Invalid draw range");end+=d.indexCount;}
  require(end==frame.indices.size(),"Unowned geometry");
  for(const auto& p:frame.vertices){for(float x:p.position)require(std::isfinite(x),"Nonfinite figure vertex");for(float c:p.color)require(std::isfinite(c)&&c>=0&&c<=1,"Invalid figure colour");}
  for(auto i:frame.indices)require(i<frame.vertices.size(),"Triangle index out of bounds");
  require(iggy3d::isFinite(frame.clipFromWorld),"Invalid figure camera");
}
bool inside(SceneViewport a,SceneViewport b){return a.x>=b.x-.01f&&a.y>=b.y-.01f&&a.width>0&&a.height>0&&a.x+a.width<=b.x+b.width+.01f&&a.y+a.height<=b.y+b.height+.01f;}
bool overlaps(SceneViewport a,SceneViewport b){return a.x<b.x+b.width-.01f&&b.x<a.x+a.width-.01f&&a.y<b.y+b.height-.01f&&b.y<a.y+a.height-.01f;}
}
int main(){try{
  Textbook book;require(matrixChapter()[1].figure.kind==BookFigureKind::RowPlanes,"RREF has no live figure binding");
  require(book.dispatch({BookActionKind::OpenSection,1}).accepted,"Cannot open RREF");
  auto& board=book.board();RowPlaneFigure figure;MathObjectScene scene;
  const auto original=board.view();const auto& first=figure.publish(original);math(first);
  require(first.rank==2&&first.dimension==1&&first.agrees&&!first.working,"Printed part (a) should have a solution line before work");
  const double length=std::sqrt(94.);for(unsigned i=0;i<3;++i)near(first.basis[0][i],std::array<double,3>{3,7,6}[i]/length,1e-12,"Wrong null direction for printed part (a)");
  const auto direction=first.basis[0];mesh(figure,scene);
  const auto revision=figure.geometry().revision;figure.publish(board.view());require(figure.geometry().revision==revision,"Unchanged figure rebuilds each frame");
  for(const auto action:std::array<BoardAction,3>{{{BoardActionKind::AddRow,0,1,0,2.},{BoardActionKind::ScaleRow,0,0,0,-3.},{BoardActionKind::SwapRows,0,1}}}){
    step(board,action);const auto& v=figure.publish(board.view());math(v);require(v.agrees&&v.rank==2,"Legal row operation changed the solution space");
    for(unsigned i=0;i<3;++i)near(v.basis[0][i],direction[i],1e-12,"Equivalent rows made the solution probe jump");
    for(unsigned r=0;r<v.rows;++r)for(unsigned c=0;c<3;++c)near(v.coefficients[r*3+c],board.view().current.at(r,c).real(),0,"Geometry and matrix use different states");mesh(figure,scene);
  }
  for(unsigned i=0;i<3;++i)step(board,{BoardActionKind::Undo});require(!board.view().working,"Undo did not restore givens");figure.publish(board.view());
  for(unsigned i=0;i<10&&!board.view().complete;++i)step(board,{BoardActionKind::Step});
  require(board.view().complete,"Guided reduction did not terminate");step(board,{BoardActionKind::Check});require(board.view().passed,"Printed reduction failed");
  const auto checked=board.view();const auto bookmark=book.bookmark();
  for(const auto& p:rowPlaneParameters())set(figure,p.id,p.maximum);
  math(figure.publish(board.view()));mesh(figure,scene);
  require(book.bookmark()==bookmark&&board.view().current.values==checked.current.values&&board.view().passed&&board.view().steps==checked.steps,"Figure settings changed reading or exercise evidence");
  using Nav=iggy3d::ProductCreativeViewportNavigationOperation;
  require(scene.navigate(Nav::Orbit,20,-12)&&scene.navigate(Nav::Pan,4,3)&&scene.navigate(Nav::Dolly,0,.5f),"Figure camera does not accept the shared navigation actions");
  mesh(figure,scene);require(board.view().passed&&board.view().steps==checked.steps,"Camera changed checked mathematics");
  require(book.dispatch({BookActionKind::OpenSection,4}).accepted&&book.dispatch({BookActionKind::OpenSection,1}).accepted,"Section navigation failed");require(book.board().view().passed,"Figure navigation lost checked work");
  for(auto bad:std::array<RowPlaneAction,5>{{{RowPlaneActionKind::Set,RowPlaneParameter::Extent,std::numeric_limits<double>::quiet_NaN()},{RowPlaneActionKind::Set,RowPlaneParameter::Density,2.5},{RowPlaneActionKind::Set,RowPlaneParameter::ProbeS,3},{RowPlaneActionKind::Set,static_cast<RowPlaneParameter>(999),0},{static_cast<RowPlaneActionKind>(999)}}}){
    const auto before=figure.geometry().revision;const auto extent=figure.parameter(RowPlaneParameter::Extent);require(!figure.dispatch(bad).accepted,"Bad figure setting accepted");figure.publish(board.view());require(figure.geometry().revision==before&&figure.parameter(RowPlaneParameter::Extent)==extent,"Rejected setting changed figure");
  }
  require(figure.dispatch({RowPlaneActionKind::Reset}).accepted,"Figure reset failed");figure.publish(board.view());require(board.view().passed,"Reset settings reset exercise");
  const std::array<MatrixBoardView,4> spaces{
    example(3,{0,0,0,0,0,0,0,0,0}),example(3,{1,2,3,2,4,6,0,0,0}),
    example(2,{1,2,0,0,1,3}),example(3,{1,0,0,0,1,0,0,0,1})};
  for(unsigned rank=0;rank<spaces.size();++rank){
    const auto& v=figure.publish(spaces[rank]);math(v);require(v.rank==rank&&v.dimension==3-rank,"Wrong solution dimension");
    for(double extent:{1.,1.6,3.})for(double density:{2.,3.,4.})for(double originalPlanes:{0.,1.})for(double normals:{0.,1.}){
      set(figure,RowPlaneParameter::Extent,extent);set(figure,RowPlaneParameter::Density,density);set(figure,RowPlaneParameter::Original,originalPlanes);set(figure,RowPlaneParameter::Normals,normals);
      for(unsigned p=0;p<3;++p)set(figure,static_cast<RowPlaneParameter>(static_cast<unsigned>(RowPlaneParameter::ProbeS)+p),p%2?2:-2);
      math(figure.publish(spaces[rank]));mesh(figure,scene);
    }
  }
  auto extreme=example(3,{1e-310,0,0,0,1e300,0,0,0,-1e-200});require(figure.publish(extreme).rank==3,"Row scaling changed the geometric rank");math(figure.view());mesh(figure,scene);
  auto changed=spaces[2];changed.working=true;changed.current=BoardMatrix(2,3);changed.current.at(0,0)=1;changed.current.at(1,0)=1;
  require(!figure.publish(changed).agrees,"A changed null space was labelled invariant");math(figure.view());
  auto zeroRow=spaces[1];figure.publish(zeroRow);require(figure.view().planes[2].zero,"Zero row became a spurious plane");
  for(unsigned part=0;part<6;++part){MatrixBoard printed;step(printed,{BoardActionKind::Select,4,part});const auto& v=figure.publish(printed.view());require(v.available==(part==0||part==3),"Unsupported dimensions silently became 3D");if(v.available){math(v);mesh(figure,scene);}else require(figure.geometry().partCount==0,"Unsupported matrix retained stale geometry");}
  auto invalid=spaces[2];invalid.given.at(0,0)={1,1e-20};require(!figure.publish(invalid).available,"Complex coefficient discarded");
  invalid=spaces[2];invalid.given.at(0,0)=std::numeric_limits<double>::infinity();require(!figure.publish(invalid).available,"Nonfinite coefficient accepted");
  for(float width:{320.f,800.f,1024.f,1200.f,1440.f,1920.f,2560.f})for(float height:{320.f,600.f,900.f,1200.f})for(bool rail:{false,true})for(bool has:{false,true})for(auto mode:{LessonPresentation::Together,LessonPresentation::Reading,LessonPresentation::Figure})for(float fraction:{.3f,.46f,.7f}){
    const auto l=planLessonSpread({width,height,rail,has,mode,fraction});const SceneViewport screen{0,0,width,height};++layouts;
    require(inside(l.header,screen)&&inside(l.footer,screen),"Shell header or footer leaves the window");
    if(l.showContents)require(inside(l.contents,screen),"Contents leaves the window");
    if(l.showReading){require(inside(l.reading,screen),"Reading leaves the window");require(!overlaps(l.reading,l.header)&&!overlaps(l.reading,l.footer),"Reader overlaps navigation");}
    if(l.showFigure){
      require(inside(l.figure,screen),"Figure leaves the window");const auto regions=planLessonFigureRegions(l.figure);
      require(inside(regions.title,l.figure)&&inside(regions.viewport,l.figure)&&inside(regions.controls,l.figure),"Figure regions leave their panel");
      require(!overlaps(regions.title,regions.viewport)&&!overlaps(regions.viewport,regions.controls),"Controls cover the 3D viewport");
    }
    if(l.split)require(l.showReading&&l.showFigure&&!overlaps(l.reading,l.figure)&&l.reading.width>=400&&l.figure.width>=440,"Split panes overlap or become too narrow");
    if(!has)require(l.showReading&&!l.showFigure,"Section without provider lost reading");
  }
  bool rejected=false;try{static_cast<void>(planLessonSpread({800,std::numeric_limits<float>::quiet_NaN()}));}catch(const std::invalid_argument&){rejected=true;}require(rejected,"Nonfinite layout accepted");
  std::printf("textbook figure: %u assertions, %u CPU mesh states, %u layouts; max %zu vertices / %zu indices. Row-space invariants, all nullities, retained checks, real-three-column limits and responsive regions passed. No native host, fonts, rasterization or images.\n",checks,frames,layouts,maxVertices,maxIndices);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"textbook figure test: %s\n",e.what());return 1;}}
