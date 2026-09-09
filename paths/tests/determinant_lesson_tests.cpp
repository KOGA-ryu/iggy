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
std::size_t assertions=0;
void require(bool condition,const char* reason){++assertions;if(!condition)throw std::runtime_error(reason);}
void near(double a,double b,double tolerance,const char* reason){require(std::isfinite(a)&&std::abs(a-b)<=tolerance,reason);}
void act(ObjectLesson& model,ObjectLessonAction a){const auto result=model.dispatch(a);if(!result.accepted)throw std::runtime_error(std::string(result.reason));}
void set(ObjectLesson& model,MathParameter p,double v,bool practice=false){act(model,{ObjectLessonActionKind::SetParameter,0,p,v,practice});}
double metric(const MathObjectSnapshot& snapshot,std::string_view name){for(unsigned i=0;i<snapshot.metricCount;++i)if(snapshot.metrics[i].label==name)return snapshot.metrics[i].value;throw std::runtime_error("Missing metric");}
void expected(const MathObjectSnapshot& state,double k,double s){
  near(metric(state,"Signed determinant"),s,1e-12,"Diagonal-product determinant certificate failed");
  near(metric(state,"Volume"),std::abs(s),1e-12,"Geometric volume must be absolute determinant");
  near(metric(state,"Rank"),s==0?2:3,0,"Independent-column rank certificate failed");
  require(state.matrixCount==1,"Missing current matrix");
  const std::array<double,9> a{1,k,0,0,s,0,0,0,1};
  for(unsigned i=0;i<9;++i)near(state.matrices[0].values[i],a[i],1e-12,"Published map disagrees with the stated family");
  const std::array<iggy3d::Vec3,3> basis{{{1,0,0},{static_cast<float>(k),static_cast<float>(s),0},{0,0,1}}};
  const std::array<std::string_view,3> names{"A e1","A e2","A e3"};
  for(unsigned axis=0;axis<3;++axis){
    unsigned tips=0;
    for(unsigned i=0;i<state.partCount;++i){const auto& part=state.parts[i];
      if(part.role==names[axis]&&part.shape==MathShape::Cone){
        const auto end=part.center+part.y*.5f;
        near(end.x,basis[axis].x,1e-6,"Basis arrow x coordinate wrong");near(end.y,basis[axis].y,1e-6,"Basis arrow y coordinate wrong");near(end.z,basis[axis].z,1e-6,"Basis arrow z coordinate wrong");++tips;
      }
    }
    require(tips==(axis==1&&k==0&&s==0?0u:1u),"Zero or nonzero basis arrow misrepresented");
  }
}
void reject(ObjectLesson& model,ObjectLessonAction action){
  const auto e=model.snapshot().revision,p=model.snapshot(true).revision;
  const auto example=model.example();const auto custom=model.customized();
  require(!model.dispatch(action).accepted,"Invalid object lesson action accepted");
  require(model.snapshot().revision==e&&model.snapshot(true).revision==p&&model.example()==example&&model.customized()==custom,"Rejected action partly changed a lesson");
}
}
int main(){try{
  ObjectLesson model(determinantVolumeSpec());expected(model.snapshot(),0,1);expected(model.snapshot(true),0,1);
  const auto badSpec=[](const ObjectLessonSpec& spec){bool failed=false;try{ObjectLesson invalid(spec);}catch(const std::invalid_argument&){failed=true;}require(failed,"Invalid authored lesson specification was accepted");};
  auto invalid=determinantVolumeSpec();invalid.metrics={"Missing measurement"};badSpec(invalid);
  invalid=determinantVolumeSpec();invalid.matrices={"Missing matrix"};badSpec(invalid);
  invalid=determinantVolumeSpec();invalid.controls.push_back(invalid.controls.front());badSpec(invalid);
  invalid=determinantVolumeSpec();invalid.examples[1].values.push_back({MathParameter::Scale,5});badSpec(invalid);
  const std::array<std::array<double,2>,5> cases{{{0,1},{1,1},{0,2},{.6,-1},{.6,0}}};
  require(model.spec().examples.size()==cases.size(),"The pilot's five comparisons are missing");
  for(unsigned i=0;i<cases.size();++i){act(model,{ObjectLessonActionKind::SelectExample,i});expected(model.snapshot(),cases[i][0],cases[i][1]);expected(model.snapshot(true),0,1);}
  require(model.snapshot(true).feedback==MathFeedback::None,"Browsing a solved example checked practice");
  reject(model,{ObjectLessonActionKind::Check});
  act(model,{ObjectLessonActionKind::Check,0,MathParameter::X,0,true});require(model.snapshot(true).feedback==MathFeedback::TryAgain,"An unchanged practice cube passed");
  set(model,MathParameter::Scale,0,true);act(model,{ObjectLessonActionKind::Check,0,MathParameter::X,0,true});
  require(model.snapshot(true).feedback==MathFeedback::Solved,"Collapsed practice did not pass");
  act(model,{ObjectLessonActionKind::Reset});expected(model.snapshot(),0,1);
  require(model.snapshot(true).feedback==MathFeedback::Solved,"Exploration reset erased practice feedback");
  act(model,{ObjectLessonActionKind::Reset,0,MathParameter::X,0,true});expected(model.snapshot(true),0,1);require(model.snapshot(true).feedback==MathFeedback::None,"Practice reset retained a stale result");
  reject(model,{ObjectLessonActionKind::SelectExample,999});reject(model,{ObjectLessonActionKind::SelectExample,4,MathParameter::X,0,true});
  for(double value:{-2.01,2.01,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})reject(model,{ObjectLessonActionKind::SetParameter,0,MathParameter::Scale,value});
  reject(model,{ObjectLessonActionKind::SetParameter,0,MathParameter::Shear,1.21});
  reject(model,{ObjectLessonActionKind::SetParameter,0,MathParameter::A00,0});
  reject(model,{ObjectLessonActionKind::SetParameter,0,MathParameter::Count,0});
  reject(model,{static_cast<ObjectLessonActionKind>(999)});
  set(model,MathParameter::Scale,.049);near(model.parameter(MathParameter::Scale),0,0,"Quantization missed zero");
  set(model,MathParameter::Scale,.051);near(model.parameter(MathParameter::Scale),.1,1e-12,"Quantization missed nearest tenth");

  MathObjectScene scene;std::size_t states=0,maxVertices=0,maxIndices=0;
  for(int kt=-12;kt<=12;++kt)for(int st=-20;st<=20;++st){
    const double k=kt/10.,s=st/10.;set(model,MathParameter::Shear,k);set(model,MathParameter::Scale,s);expected(model.snapshot(),k,s);
    const auto& frame=scene.publish(model.snapshot(),{0,0,800,600});++states;
    require(!frame.vertices.empty()&&!frame.indices.empty(),"A supported state lost its geometry");
    require(frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"Geometry capacity exceeded");
    maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
    require(iggy3d::isFinite(frame.clipFromWorld),"Camera matrix is not finite");
    for(const auto& vertex:frame.vertices)for(float value:vertex.position)require(std::isfinite(value)&&std::abs(value)<8,"Mesh coordinate is nonfinite or outside the bounded example");
    for(auto index:frame.indices)require(index<frame.vertices.size(),"Mesh index outside its buffer");
    act(model,{ObjectLessonActionKind::Check,0,MathParameter::X,0,true}); // Still the independent unsolved unit cube.
    require(model.snapshot(true).feedback==MathFeedback::TryAgain,"Teaching sweep affected practice");
  }
  require(states==1025,"Incomplete input lattice sweep");

  Textbook book;const auto sections=matrixChapter();unsigned section=0;
  for(;section<sections.size();++section)if(std::string_view(sections[section].id)=="matrix.determinant-volume")break;
  require(section<sections.size()&&book.dispatch({BookActionKind::OpenSection,section}).accepted,"Determinant section is unreachable");
  require(!book.hasBoard()&&book.exerciseKind()==BookExerciseKind::Object,"Object exercise borrowed a matrix board");
  bool refusedBoard=false;try{static_cast<void>(book.board());}catch(const std::logic_error&){refusedBoard=true;}require(refusedBoard,"Object section silently exposes a source board");
  const auto e=book.objectLesson().snapshot().revision,p=book.objectLesson().snapshot(true).revision;
  std::set<std::string> ids;unsigned written=0;
  for(const auto& block:sections[section].lesson){require(ids.insert(block.id).second,"Duplicate lesson block id");
    if(block.kind==BookBlockKind::Exercise){++written;for(unsigned h=1;h<4;++h)require(!block.help[h].empty(),"Written check lacks independent help");}
  }
  require(written==3,"Expected three written checks");
  for(const auto& block:sections[section].lesson)for(const auto& ref:block.references)require(ids.contains(ref.target),"Broken lesson reference");
  for(const auto& term:sections[section].terms)require(ids.contains(term.blockId),"Broken index reference");
  for(const auto& block:book.lessonView())for(const auto& help:block.help)require(!help.open&&help.passages.empty(),"Unrequested written answer was disclosed");
  require(book.dispatch({BookActionKind::ToggleHelp,section,0,"determinant.practice.volume",BookHelp::Hint}).accepted,"Cannot open hint");
  for(const auto& block:book.lessonView())if(std::string_view(block.id)=="determinant.practice.volume")require(block.help[1].open&&!block.help[2].open&&!block.help[3].open,"Hint leaked answer or solution");
  for(const auto action:{BookAction{BookActionKind::Exercise},BookAction{BookActionKind::Read},BookAction{BookActionKind::Contents},BookAction{BookActionKind::Index},BookAction{BookActionKind::Resume}})require(book.dispatch(action).accepted,"View navigation failed");
  require(book.objectLesson().snapshot().revision==e&&book.objectLesson().snapshot(true).revision==p,"Reading or navigation changed mathematical state");
  require(book.objectLesson().snapshot(true).feedback==MathFeedback::None,"Navigation performed a check");
  set(book.objectLesson(),MathParameter::Scale,0,true);act(book.objectLesson(),{ObjectLessonActionKind::Check,0,MathParameter::X,0,true});
  const auto saved=book.bookmark();require(book.restoreBookmark(saved).accepted,"Object reading bookmark failed");
  require(book.objectLesson().snapshot(true).feedback==MathFeedback::Solved,"Restoring reading erased existing object practice feedback");

  // A second existing model proves that controls and checking are not Linear-specific.
  const ObjectLessonSpec circle{MathObjectKind::Trig,0,"cos^2 + sin^2 = 1","Unit circle","Reach the existing trigonometry target.",{{MathParameter::Angle,"Angle"}},{{"Start","Positive x axis",{{MathParameter::Angle,0}}},{"Second quadrant","Angle 150 degrees",{{MathParameter::Angle,150}}}}};
  ObjectLesson reusable(circle);act(reusable,{ObjectLessonActionKind::SelectExample,1});
  near(reusable.snapshot().metrics[0].value,-std::sqrt(3.)/2,1e-12,"Generic adapter changed the second model's cosine");
  near(reusable.snapshot().metrics[1].value,.5,1e-12,"Generic adapter changed the second model's sine");
  set(reusable,MathParameter::Angle,150,true);act(reusable,{ObjectLessonActionKind::Check,0,MathParameter::X,0,true});require(reusable.snapshot(true).feedback==MathFeedback::Solved,"Generic adapter did not use the second model's checker");
  std::printf("determinant lesson: %zu assertions; 5 exact examples; %zu input/mesh states; max %zu vertices / %zu indices; separate practice, redacted help, navigation and reusable second-model binding passed.\n",assertions,states,maxVertices,maxIndices);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"determinant lesson test: %s\n",e.what());return 1;}}
