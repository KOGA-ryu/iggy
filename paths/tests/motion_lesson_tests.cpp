#include "content/MotionProgressIO.hpp"
#include "scene/MotionScene.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>

using namespace paths;
namespace {
void expect(bool yes,const char* reason){if(!yes)throw std::runtime_error(reason);}
void near(double a,double b,const char* reason,double tolerance=1e-8){expect(std::abs(a-b)<=tolerance,reason);}
void apply(MotionLesson& m,MotionAction a){expect(m.dispatch(a),"Semantic motion action accepted");}
void finish(MotionLesson& m,double tick=.25) {
  for(unsigned n=0;n<10000 && m.playing();++n)apply(m,{MotionActionKind::Tick,0,tick});
  expect(!m.playing(),"Playback bounded by chapter duration");
}
constexpr std::array<unsigned,5> solutions{1,2,3,1,2};
void routes() {
  MotionLesson m;
  for(std::size_t i=0;i<motionChapterCount;++i) {
    expect(m.progress().selected==i,"Only explicit Next advances chapters");
    const auto initial=m.run().plan;
    apply(m,{MotionActionKind::Run});apply(m,{MotionActionKind::Tick,0,.25});apply(m,{MotionActionKind::Pause});
    const auto held=m.sample();apply(m,{MotionActionKind::Tick,0,.25});near(m.sample().position,held.position,"Pause holds exact coordinate");
    apply(m,{MotionActionKind::Pause});finish(m);
    expect(m.run().attempts.size()==1 && !m.run().attempts[0].passed && !m.run().solved,"Initial wrong plan records one failed attempt");
    expect(m.run().plan==initial,"Wrong run retains the player's plan");
    apply(m,{MotionActionKind::ChoosePlan,solutions[i]});const auto correct=m.run().plan;
    expect(m.ghost() && m.ghost()->plan==initial,"Previous different plan becomes ghost");
    apply(m,{MotionActionKind::Undo});expect(m.run().plan==initial && m.run().attempts.size()==1,"Undo preserves attempt evidence");
    apply(m,{MotionActionKind::ChoosePlan,solutions[i]});
    apply(m,{MotionActionKind::Run});apply(m,{MotionActionKind::Scrub,0,m.chapter().duration});
    expect(!m.run().solved && m.run().attempts.size()==1,"Scrubbing to the finish cannot manufacture a run");
    apply(m,{MotionActionKind::Run});finish(m);
    expect(m.run().solved && m.run().attempts.size()==2,"Full correct run passes goals once");
    expect(!m.dispatch({MotionActionKind::SetParameter,0,0}),"Completed working is locked until Again");
    apply(m,{MotionActionKind::Run});finish(m);expect(m.run().attempts.size()==2,"Replay adds no duplicate evidence");
    apply(m,{MotionActionKind::Review,0});expect(m.shownPlan()==initial && m.run().plan==correct,"Blueprint inspection retains current working");
    apply(m,{MotionActionKind::ReturnToPlan});expect(m.run().solved,"Inspection does not unfinish a chapter");
    if(i+1<motionChapterCount)apply(m,{MotionActionKind::Next});else expect(!m.dispatch({MotionActionKind::Next}),"Final chapter remains visible");
  }
  apply(m,{MotionActionKind::Again});expect(!m.run().solved && m.run().attempts.size()==2,"Again retains completed and failed runs");
  apply(m,{MotionActionKind::ChoosePlan,3});apply(m,{MotionActionKind::Run});finish(m);
  expect(m.run().solved,"A different peak time is a valid solution");
  MotionLesson edits;const auto initial=edits.run().plan;
  apply(edits,{MotionActionKind::BeginEdit});
  for(int value=0;value<=10;++value)apply(edits,{MotionActionKind::SetParameter,0,static_cast<double>(value)});
  apply(edits,{MotionActionKind::EndEdit});expect(edits.run().undo.size()==1,"A complete drag is one Undo action");
  apply(edits,{MotionActionKind::Undo});expect(edits.run().plan==initial,"Undo restores the beginning of a drag");
  for(double invalid:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})expect(!edits.dispatch({MotionActionKind::SetParameter,0,invalid}),"Nonfinite input rejected");
  expect(!edits.dispatch({MotionActionKind::Tick,0,-1}) && !edits.dispatch({MotionActionKind::Tick,0,1}),"Unbounded clock steps rejected");
  expect(!edits.dispatch({MotionActionKind::Select,5}) && !edits.dispatch({MotionActionKind::SetParameter,2,2}),"Unknown chapters and controls rejected");
  for(double speed:{.25,.5,1.0,2.0})for(double dt:{.25,1.0/60,1.0/144}) {
    MotionLesson playback;apply(playback,{MotionActionKind::Select,4});apply(playback,{MotionActionKind::ChoosePlan,2});
    apply(playback,{MotionActionKind::Speed,0,speed});apply(playback,{MotionActionKind::Run});finish(playback,dt);
    near(playback.sample().position,12,"Same endpoint at all playback speeds and frame rates");expect(playback.run().solved,"Playback rate cannot change correctness");
  }
}
std::size_t arithmetic() {
  // Independent midpoint integration uses the authored functional forms, never
  // motionJourney or sampleMotion to produce the reference values.
  std::size_t plans=0;
  for(std::size_t chapter=0;chapter<motionChapterCount;++chapter) {
    const auto& c=motionChapters()[chapter];const auto& first=c.parameters[0];
    for(double a=first.minimum;a<=first.maximum;a+=first.step) {
      const double minB=c.parameters.size()==2?c.parameters[1].minimum:0,maxB=c.parameters.size()==2?c.parameters[1].maximum:0,stepB=c.parameters.size()==2?c.parameters[1].step:1;
      for(double b=minB;b<=maxB;b+=stepB) {
        MotionPlan p{{a,b}};++plans;expect(validMotionPlan(chapter,p),"Authored parameter domain is valid");
        double displacement=0,distance=0;constexpr int steps=4000;const double dt=c.duration/steps;
        for(int n=0;n<steps;++n) {
          const double t=(n+.5)*dt;double v=0;
          switch(chapter) {
            case 0:v=a-2;break;
            case 1:v=a;break;
            case 2:case 3:v=t<2?a:b;break;
            case 4:v=t<b?a*t/b:a*(4-t)/(4-b);break;
          }
          displacement+=v*dt;distance+=std::abs(v)*dt;
        }
        const auto s=sampleMotion(chapter,p,c.duration);
        near(s.displacement,displacement,"Analytic displacement agrees with independent quadrature",1e-6);
        near(s.distance,distance,"Analytic distance agrees with independent absolute-velocity quadrature",1e-6);
        near(s.position,c.start+displacement,"Position includes starting coordinate",1e-6);
      }
    }
  }
  const MotionPlan back{{3,-3}};near(sampleMotion(3,back,4).distance,12,"Out-and-back distance is twelve");near(sampleMotion(3,back,4).displacement,0,"Out-and-back displacement is zero");
  expect(!sampleMotion(3,back,2).acceleration,"Velocity jump has no finite acceleration");
  const MotionPlan triangle{{6,2}};near(sampleMotion(4,triangle,1).position,1.5,"Accelerating position is quadratic");near(sampleMotion(4,triangle,3).position,10.5,"Braking position is quadratic");
  expect(!sampleMotion(4,triangle,2).acceleration,"Ramp corner has no single acceleration");
  near(*sampleMotion(4,triangle,1).acceleration,3,"Acceleration is positive slope");near(*sampleMotion(4,triangle,3).acceleration,-3,"Braking is negative slope");
  expect(!evaluateMotion(2,{{2,2}}).passed,"Correct endpoint with missed checkpoint fails");
  expect(!evaluateMotion(3,{{0,0}}).passed,"Staying still does not solve out-and-back");
  for(double peakTime:{1.0,1.5,2.0,2.5,3.0})expect(evaluateMotion(4,{{6,peakTime}}).passed,"Every supported peak time accepts equal area");
  return plans;
}
std::string read(const std::filesystem::path& path){std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
void write(const std::filesystem::path& path,const std::string& text){std::ofstream out(path);out<<text;}
void persistence(const std::filesystem::path& root) {
  const auto path=root/"motion.json";MotionLesson m;MotionProgressFile save(path);save.load(m);
  apply(m,{MotionActionKind::Run});finish(m);apply(m,{MotionActionKind::ChoosePlan,1});apply(m,{MotionActionKind::Run});finish(m);
  apply(m,{MotionActionKind::Next});apply(m,{MotionActionKind::SetParameter,0,-2});apply(m,{MotionActionKind::Scrub,0,1.5});save.save(m,true);
  expect(!save.failed(),"Motion save succeeds");
  MotionLesson restored;MotionProgressFile load(path);load.load(restored);
  expect(!load.failed() && restored.progress().selected==1,"Stable chapter identity restored");
  expect(restored.progress().runs[0].solved && restored.progress().runs[0].attempts.size()==2,"Completion and both attempts restored");
  expect(restored.run().plan==m.run().plan && restored.run().undo==m.run().undo,"Unfinished plan and Undo restored");near(restored.progress().time,1.5,"Scrub position restored");expect(!restored.playing(),"Reopen is paused");
  auto reordered=nlohmann::json::parse(read(path));std::reverse(reordered["chapters"].begin(),reordered["chapters"].end());write(path,reordered.dump());
  MotionLesson reorderedLesson;MotionProgressFile reorderedFile(path);reorderedFile.load(reorderedLesson);expect(!reorderedFile.failed() && reorderedLesson.progress().runs[0].solved,"Row reordering preserves saved identities");
  const auto original=read(path);write(path,original+" ");apply(reorderedLesson,{MotionActionKind::SetParameter,0,3});reorderedFile.save(reorderedLesson);
  expect(reorderedFile.failed() && read(path)==original+" ","External edit is preserved");
  for(int corruption=0;corruption<4;++corruption) {
    auto damaged=reordered;
    switch(corruption) {
      case 0:damaged["version"]=2;break;
      case 1:damaged["chapters"][0]["id"]="stopping-v2";break;
      case 2:damaged["chapters"][0]["plan"][0]=100000;break;
      case 3:damaged["chapters"][0]["solved"]=true;break;
    }
    const auto text=damaged.dump();write(path,text);MotionLesson fresh;MotionProgressFile invalid(path);invalid.load(fresh);invalid.save(fresh,true);
    expect(invalid.failed() && read(path)==text && fresh.progress().selected==0,"Malformed save remains intact; restore is atomic");
  }
  const auto flightPath=root/"in-flight.json";MotionLesson flight;MotionProgressFile flightSave(flightPath);flightSave.load(flight);
  apply(flight,{MotionActionKind::Select,4});apply(flight,{MotionActionKind::ChoosePlan,2});apply(flight,{MotionActionKind::Run});
  apply(flight,{MotionActionKind::Tick,0,.25});apply(flight,{MotionActionKind::Tick,0,.25});flightSave.save(flight,true);
  MotionLesson resumed;MotionProgressFile resumeFile(flightPath);resumeFile.load(resumed);
  expect(!resumeFile.failed() && !resumed.playing() && resumed.progress().recording,"Reopen preserves a paused in-flight attempt");
  apply(resumed,{MotionActionKind::Pause});finish(resumed);expect(resumed.run().solved && resumed.run().attempts.size()==1,"Resuming a saved journey records exactly one completed attempt");
  const auto dragPath=root/"drag.json";MotionLesson drag;MotionProgressFile dragSave(dragPath);dragSave.load(drag);
  apply(drag,{MotionActionKind::BeginEdit});apply(drag,{MotionActionKind::SetParameter,0,8});dragSave.save(drag);
  apply(drag,{MotionActionKind::EndEdit});dragSave.save(drag);
  MotionLesson dragRestored;MotionProgressFile dragRead(dragPath);dragRead.load(dragRestored);
  expect(!dragRead.failed() && dragRestored.run().undo.size()==1,"Releasing a drag saves its Undo boundary");
}
void geometry() {
  MotionLesson m;MotionScene scene;
  for(std::size_t chapter=0;chapter<motionChapterCount;++chapter)for(const auto viewport:{SceneViewport{0,0,920,300},SceneViewport{0,0,510,190},SceneViewport{0,0,360,74}}) {
    apply(m,{MotionActionKind::Select,chapter});apply(m,{MotionActionKind::ChoosePlan,solutions[chapter]});
    for(int sample=0;sample<=8;++sample) {
      apply(m,{MotionActionKind::Scrub,0,m.chapter().duration*sample/8});const auto& frame=scene.publish(m,viewport);
      expect(frame.vertices.size()<kSceneVertexCapacity && frame.indices.size()<kSceneIndexCapacity,"Motion scene fits existing GPU buffers");
      for(auto i:frame.indices)expect(i<frame.vertices.size(),"Every mesh index is valid");
      for(const auto& v:frame.vertices)for(float coordinate:v.position)expect(std::isfinite(coordinate),"All scene coordinates finite");
      const auto cart=std::find_if(frame.draws.begin(),frame.draws.end(),[](const auto& d){return d.objectId.value==100;});
      expect(cart!=frame.draws.end(),"Native scene contains actual cart geometry");near((cart->bounds.min.x+cart->bounds.max.x)/2,m.sample().position,"Rendered cart uses canonical coordinate",1e-5);
      const auto p=scene.project({static_cast<float>(m.sample().position),.5F,0});
      expect(p.x>=0 && p.x<=viewport.width && p.y>=0 && p.y<=viewport.height,"Cart projects inside the requested viewport");
    }
  }
}
}
int main() {
  const auto root=std::filesystem::temp_directory_path()/("paths-motion-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(root);routes();const auto plans=arithmetic();persistence(root);geometry();std::filesystem::remove_all(root);
    std::cout<<"MOTION {\"chapters\":5,\"complete_routes\":6,\"independent_plans\":"<<plans<<",\"playback_combinations\":12,\"scene_samples\":135,\"windows\":0,\"captures\":0}\n";
  } catch(const std::exception& e){std::filesystem::remove_all(root);std::cerr<<e.what()<<'\n';return 1;}
}
