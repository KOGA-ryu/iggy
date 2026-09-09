#include "runtime/motion/MotionLesson.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace paths {
namespace {
constexpr MotionParameter position[]={{"Finish position (m)","x_1",-2,14,.5}};
constexpr MotionParameter velocity[]={{"Velocity (m/s)","v",-4,6,.5}};
constexpr MotionParameter sections[]={{"First 2 s (m/s)","v_1",-4,6,.5},{"Last 2 s (m/s)","v_2",-4,6,.5}};
constexpr MotionParameter ramp[]={{"Peak velocity (m/s)","v_p",0,8,.5},{"Peak time (s)","t_p",1,3,.5}};
constexpr MotionGoal positionGoals[]={{MotionMeasure::Position,1,8,"Finish at 8 m"}};
constexpr MotionGoal constantGoals[]={{MotionMeasure::Position,4,8,"Reach 8 m at 4 s"}};
constexpr MotionGoal sectionGoals[]={{MotionMeasure::Position,2,2,"Pass 2 m at 2 s"},{MotionMeasure::Position,4,8,"Reach 8 m at 4 s"}};
constexpr MotionGoal returnGoals[]={{MotionMeasure::Position,2,6,"Reach 6 m at 2 s"},{MotionMeasure::Position,4,0,"Return to 0 m at 4 s"},{MotionMeasure::Distance,4,12,"Travel 12 m in total"}};
constexpr MotionGoal rampGoals[]={{MotionMeasure::Position,4,12,"Reach 12 m at 4 s"},{MotionMeasure::Velocity,0,0,"Start at rest"},{MotionMeasure::Velocity,4,0,"Finish at rest"},{MotionMeasure::Distance,4,12,"Travel 12 m in total"}};
constexpr MotionChoice choice(double a,double b,std::string_view latex) { return {{{a,b}},latex}; }
const std::array<MotionChapter,motionChapterCount> chapters{{
  {"position-v1","Where am I?","Move from 2 m to 8 m. Set the finish position, then run the journey.",
   R"(x_0=2\,\mathrm{m},\quad x_1=8\,\mathrm{m})",R"(\Delta x=x_1-x_0)",
   "Position is a coordinate; displacement is the change in position. The one-second animation shows the move, not a required speed.",
   MotionProfile::Position,2,1,{{4,0}},position,positionGoals,{{choice(4,0,R"(x_1=4)"),choice(8,0,R"(x_1=8)"),choice(-2,0,R"(x_1=-2)"),choice(6,0,R"(x_1=6)")}}},
  {"constant-v1","Moving at a steady pace","Reach 8 m in 4 s from 0 m. Choose a constant velocity.",
   R"(x_0=0,\quad x(4)=8\,\mathrm{m})",R"(\Delta x=v\,\Delta t)",
   "Velocity has a sign: positive goes right, negative goes left. A rectangle under the velocity graph gives displacement.",
   MotionProfile::Constant,0,4,{{1,0}},velocity,constantGoals,{{choice(1,0,R"(v=1)"),choice(-2,0,R"(v=-2)"),choice(2,0,R"(v=2)"),choice(4,0,R"(v=4)")}}},
  {"sections-v1","Building a journey","Pass 2 m at 2 s, then reach 8 m at 4 s. Set both velocities.",
   R"(x(2)=2,\quad x(4)=8\,\mathrm{m})",R"(\Delta x=v_1\Delta t_1+v_2\Delta t_2)",
   "Each section contributes its own signed area. Meeting the final target alone is not enough: the halfway checkpoint also matters.",
   MotionProfile::Sections,0,4,{{1,1}},sections,sectionGoals,{{choice(2,2,R"((v_1,v_2)=(2,2))"),choice(3,1,R"((v_1,v_2)=(3,1))"),choice(1,1,R"((v_1,v_2)=(1,1))"),choice(1,3,R"((v_1,v_2)=(1,3))")}}},
  {"return-v1","Going out and coming back","Reach 6 m at 2 s, then return to 0 m at 4 s. Travel 12 m in total.",
   R"(x(2)=6,\quad x(4)=0,\quad d=12\,\mathrm{m})",R"(\Delta x=\int v\,dt,\quad d=\int |v|\,dt)",
   "Area below zero subtracts from displacement. Distance counts movement in both directions positively. Instant velocity changes are idealised here.",
   MotionProfile::Sections,0,4,{{2,1}},sections,returnGoals,{{choice(3,3,R"((v_1,v_2)=(3,3))"),choice(3,-3,R"((v_1,v_2)=(3,-3))"),choice(0,0,R"((v_1,v_2)=(0,0))"),choice(-3,3,R"((v_1,v_2)=(-3,3))")}}},
  {"stopping-v1","Accelerating and stopping","Travel 12 m in 4 s. Start and finish at rest. Shape the velocity ramp.",
   R"(d=12\,\mathrm{m},\quad T=4\,\mathrm{s},\quad v(0)=v(4)=0)",R"(\Delta x=\tfrac12 T v_p,\quad a=\frac{\Delta v}{\Delta t})",
   "Triangle area gives displacement; slope gives acceleration. Moving the peak changes how strongly you accelerate and brake, while the area can stay the same.",
   MotionProfile::Ramp,0,4,{{3,2}},ramp,rampGoals,{{choice(3,2,R"((v_p,t_p)=(3,2))"),choice(8,2,R"((v_p,t_p)=(8,2))"),choice(6,2,R"((v_p,t_p)=(6,2))"),choice(6,1,R"((v_p,t_p)=(6,1))")}}}
}};
bool close(double a,double b){return std::abs(a-b)<=1e-8;}
std::string number(double value) {
  if(std::abs(value)<1e-10)value=0;
  std::ostringstream out;out<<std::setprecision(5)<<value;return out.str();
}
double unsignedArea(double first,double last,double duration) {
  if(first*last>=0)return .5*(std::abs(first)+std::abs(last))*duration;
  // Split exactly at the zero crossing instead of taking abs(signed area).
  const double before=duration*std::abs(first)/(std::abs(first)+std::abs(last));
  return .5*std::abs(first)*before+.5*std::abs(last)*(duration-before);
}
}
std::span<const MotionChapter> motionChapters(){return chapters;}
bool validMotionPlan(std::size_t chapter,const MotionPlan& p) {
  if(chapter>=chapters.size())return false;
  const auto specs=chapters[chapter].parameters;
  for(std::size_t i=0;i<p.values.size();++i) {
    const auto v=p.values[i];if(!std::isfinite(v))return false;
    if(i>=specs.size()){if(v!=0)return false;continue;}
    const auto& s=specs[i];if(v<s.minimum || v>s.maximum || !close((v-s.minimum)/s.step,std::round((v-s.minimum)/s.step)))return false;
  }
  return true;
}
MotionJourney motionJourney(std::size_t index,const MotionPlan& p) {
  if(!validMotionPlan(index,p))throw std::invalid_argument("Invalid motion plan");
  const auto& c=chapters[index];const auto v=p.values[0];
  switch(c.profile) {
    case MotionProfile::Position:return {{{{0,c.duration,(v-c.start)/c.duration,(v-c.start)/c.duration},{}}},1};
    case MotionProfile::Constant:return {{{{0,c.duration,v,v},{}}},1};
    case MotionProfile::Sections:return {{{{0,2,v,v},{2,c.duration,p.values[1],p.values[1]}}},2};
    case MotionProfile::Ramp:return {{{{0,p.values[1],0,v},{p.values[1],c.duration,v,0}}},2};
  }
  throw std::invalid_argument("Unknown motion profile");
}
MotionSample sampleMotion(std::size_t index,const MotionPlan& p,double time) {
  const auto journey=motionJourney(index,p);const auto& chapter=chapters[index];
  if(!std::isfinite(time))throw std::invalid_argument("Invalid motion time");
  MotionSample out;out.time=std::clamp(time,0.0,chapter.duration);out.position=chapter.start;
  for(std::size_t i=0;i<journey.count;++i) {
    const auto& s=journey.segments[i];const double duration=s.end-s.begin;
    const double elapsed=std::clamp(out.time-s.begin,0.0,duration),a=(s.lastVelocity-s.firstVelocity)/duration;
    const double v=s.firstVelocity+a*elapsed;
    out.displacement+=.5*(s.firstVelocity+v)*elapsed;
    out.distance+=unsignedArea(s.firstVelocity,v,elapsed);
    if(out.time>=s.begin) {out.velocity=v;out.acceleration=a;}
  }
  out.position+=out.displacement;
  if(journey.count==2 && close(out.time,journey.segments[1].begin)) {
    const auto& a=journey.segments[0];const auto& b=journey.segments[1];
    if(!close(a.lastVelocity,b.firstVelocity) || !close((a.lastVelocity-a.firstVelocity)/(a.end-a.begin),(b.lastVelocity-b.firstVelocity)/(b.end-b.begin)))out.acceleration.reset();
  }
  return out;
}
MotionEvaluation evaluateMotion(std::size_t index,const MotionPlan& p) {
  MotionEvaluation result;result.passed=true;
  for(const auto& goal:chapters.at(index).goals) {
    const auto sample=sampleMotion(index,p,goal.time);double measured=0;
    switch(goal.measure) {
      case MotionMeasure::Position:measured=sample.position;break;
      case MotionMeasure::Distance:measured=sample.distance;break;
      case MotionMeasure::Velocity:measured=sample.velocity;break;
    }
    const bool passed=close(measured,goal.target);result.checks.push_back({goal,measured,passed});result.passed=result.passed && passed;
  }
  return result;
}
std::vector<std::string> motionWorking(std::size_t index,const MotionPlan& plan) {
  const auto& c=chapters.at(index);const auto journey=motionJourney(index,plan);
  std::vector<std::string> out{std::string(c.relationship)};
  if(c.profile==MotionProfile::Position)out.push_back("\\Delta x="+number(plan.values[0])+"-("+number(c.start)+")="+number(plan.values[0]-c.start)+"\\,\\mathrm{m}");
  else for(std::size_t i=0;i<journey.count;++i) {
    const auto& s=journey.segments[i];const double area=.5*(s.firstVelocity+s.lastVelocity)*(s.end-s.begin);
    out.push_back("A_"+std::to_string(i+1)+"=\\tfrac12("+number(s.firstVelocity)+"+("+number(s.lastVelocity)+"))("+number(s.end-s.begin)+")="+number(area)+"\\,\\mathrm{m}");
  }
  const auto end=sampleMotion(index,plan,c.duration);
  out.push_back("\\Delta x="+number(end.displacement)+"\\,\\mathrm{m},\\quad d="+number(end.distance)+"\\,\\mathrm{m}");
  if(c.profile==MotionProfile::Ramp) {
    out.push_back("a_1="+number(plan.values[0]/plan.values[1])+"\\,\\mathrm{m/s^2},\\quad a_2="+number(-plan.values[0]/(c.duration-plan.values[1]))+"\\,\\mathrm{m/s^2}");
  }
  return out;
}
MotionLesson::MotionLesson() {
  for(std::size_t i=0;i<chapters.size();++i)progress_.runs[i].plan=chapters[i].initial;
}
const MotionPlan& MotionLesson::shownPlan() const {return progress_.inspected?run().attempts[*progress_.inspected].plan:run().plan;}
const MotionAttempt* MotionLesson::ghost() const {
  if(progress_.inspected)return *progress_.inspected?&run().attempts[*progress_.inspected-1]:nullptr;
  for(auto i=run().attempts.rbegin();i!=run().attempts.rend();++i)if(i->plan!=run().plan)return &*i;
  return nullptr;
}
void MotionLesson::halt(){playing_=false;progress_.recording=false;}
void MotionLesson::remember(const MotionPlan& p) {
  auto& history=progress_.runs[progress_.selected].undo;
  if(history.size()==motionUndoCapacity)history.erase(history.begin());
  history.push_back(p);
}
bool MotionLesson::changePlan(MotionPlan p) {
  auto& run=progress_.runs[progress_.selected];
  if(run.solved || progress_.inspected || !validMotionPlan(progress_.selected,p))return false;
  if(run.plan==p)return true;
  if(!editBefore_)remember(run.plan);
  run.plan=p;run.started=true;halt();progress_.time=0;message_.clear();++revision_;return true;
}
bool MotionLesson::dispatch(MotionAction action) {
  auto& run=progress_.runs[progress_.selected];
  if(!std::isfinite(action.value))return false;
  switch(action.kind) {
    case MotionActionKind::Select:
      if(action.index>=chapters.size())return false;
      (void)dispatch({MotionActionKind::EndEdit});halt();progress_.selected=action.index;progress_.time=0;progress_.inspected.reset();message_.clear();++revision_;return true;
    case MotionActionKind::BeginEdit:
      if(run.solved || progress_.inspected)return false;
      if(!editBefore_)editBefore_=run.plan;return true;
    case MotionActionKind::EndEdit:
      if(editBefore_ && *editBefore_!=run.plan){remember(*editBefore_);++revision_;}
      editBefore_.reset();return true;
    case MotionActionKind::SetParameter: {
      if(action.index>=chapter().parameters.size())return false;
      const auto& spec=chapter().parameters[action.index];auto p=run.plan;
      p.values[action.index]=spec.minimum+std::round((std::clamp(action.value,spec.minimum,spec.maximum)-spec.minimum)/spec.step)*spec.step;
      return changePlan(p);
    }
    case MotionActionKind::ChoosePlan:
      if(action.index>=chapter().choices.size())return false;
      (void)dispatch({MotionActionKind::EndEdit});return changePlan(chapter().choices[action.index].plan);
    case MotionActionKind::Undo:
      (void)dispatch({MotionActionKind::EndEdit});
      if(run.solved || progress_.inspected || run.undo.empty())return false;
      run.plan=run.undo.back();run.undo.pop_back();halt();progress_.time=0;message_.clear();++revision_;return true;
    case MotionActionKind::Run:
      (void)dispatch({MotionActionKind::EndEdit});
      if(!run.solved && !progress_.inspected && run.attempts.size()==motionAttemptCapacity){message_="This chapter has reached its 256-run history limit.";return false;}
      progress_.time=0;playing_=true;progress_.recording=!run.solved && !progress_.inspected;run.started=true;message_.clear();++revision_;return true;
    case MotionActionKind::Pause:
      if(playing_)playing_=false;
      else if(progress_.time<chapter().duration)playing_=true;
      ++revision_;return true;
    case MotionActionKind::Tick:
      if(action.value<0 || action.value>.25)return false;
      if(!playing_)return true;
      progress_.time=std::min(chapter().duration,progress_.time+action.value*progress_.speed);
      if(progress_.time>=chapter().duration-1e-10) {
        progress_.time=chapter().duration;playing_=false;
        if(progress_.recording) {
          const bool passed=evaluateMotion(progress_.selected,run.plan).passed;
          run.attempts.push_back({run.plan,passed});run.solved=passed;progress_.recording=false;
          message_=passed?"Complete. Inspect your working, or choose Next.":"Checkpoint missed. Adjust the cyan plan and run again.";
        }
        ++revision_;
      }
      return true;
    case MotionActionKind::Rewind:halt();progress_.time=0;++revision_;return true;
    case MotionActionKind::Scrub:halt();progress_.time=std::clamp(action.value,0.0,chapter().duration);message_="Timeline preview. Run records a complete journey.";++revision_;return true;
    case MotionActionKind::Speed:
      if(action.value!=.25 && action.value!=.5 && action.value!=1 && action.value!=2)return false;
      progress_.speed=action.value;++revision_;return true;
    case MotionActionKind::Review:
      if(action.index>=run.attempts.size())return false;
      (void)dispatch({MotionActionKind::EndEdit});halt();progress_.inspected=action.index;progress_.time=chapter().duration;++revision_;return true;
    case MotionActionKind::ReturnToPlan:halt();progress_.inspected.reset();progress_.time=0;++revision_;return true;
    case MotionActionKind::Again:
      if(!run.solved || progress_.inspected)return false;
      halt();run.solved=false;run.undo.clear();run.plan=chapter().initial;progress_.time=0;message_.clear();++revision_;return true;
    case MotionActionKind::Next:
      if(!run.solved || progress_.selected+1>=chapters.size())return false;
      return dispatch({MotionActionKind::Select,progress_.selected+1});
  }
  return false;
}
bool MotionLesson::restore(MotionProgress p) {
  if(p.selected>=chapters.size() || !std::isfinite(p.time) || p.time<0 || p.time>chapters[p.selected].duration ||
      (p.speed!=.25 && p.speed!=.5 && p.speed!=1 && p.speed!=2))return false;
  for(std::size_t i=0;i<chapters.size();++i) {
    auto& r=p.runs[i];if(!validMotionPlan(i,r.plan) || r.undo.size()>motionUndoCapacity || r.attempts.size()>motionAttemptCapacity)return false;
    for(const auto& plan:r.undo)if(!validMotionPlan(i,plan))return false;
    for(auto& attempt:r.attempts) {
      if(!validMotionPlan(i,attempt.plan))return false;
      attempt.passed=evaluateMotion(i,attempt.plan).passed;
    }
    if(r.solved && (r.attempts.empty() || !r.attempts.back().passed || r.attempts.back().plan!=r.plan))return false;
    if(!r.started && (!r.attempts.empty() || r.solved || !r.undo.empty() || r.plan!=chapters[i].initial))return false;
  }
  if(p.inspected && *p.inspected>=p.runs[p.selected].attempts.size())return false;
  const auto& active=p.runs[p.selected];
  if(p.recording && (p.inspected || active.solved || !active.started || p.time>=chapters[p.selected].duration || active.attempts.size()>=motionAttemptCapacity))return false;
  progress_=std::move(p);playing_=false;editBefore_.reset();message_="Motion progress restored. Playback is paused.";++revision_;return true;
}
} // namespace paths
