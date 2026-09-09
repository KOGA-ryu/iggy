#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace paths {
inline constexpr std::size_t motionChapterCount=5, motionAttemptCapacity=256, motionUndoCapacity=64;
struct MotionPlan {
  std::array<double,2> values{};
  bool operator==(const MotionPlan&) const = default;
};
enum class MotionProfile { Position, Constant, Sections, Ramp };
struct MotionParameter { std::string_view label, symbol; double minimum,maximum,step; };
enum class MotionMeasure { Position, Distance, Velocity };
struct MotionGoal { MotionMeasure measure; double time,target; std::string_view label; };
struct MotionChoice { MotionPlan plan; std::string_view latex; };
struct MotionChapter {
  std::string_view id,title,prompt,given,relationship,explanation;
  MotionProfile profile;
  double start,duration;
  MotionPlan initial;
  std::span<const MotionParameter> parameters;
  std::span<const MotionGoal> goals;
  std::array<MotionChoice,4> choices;
};
[[nodiscard]] std::span<const MotionChapter> motionChapters();
struct MotionSegment { double begin,end,firstVelocity,lastVelocity; };
struct MotionJourney { std::array<MotionSegment,2> segments{}; std::size_t count=0; };
struct MotionSample {
  double time=0,position=0,velocity=0,displacement=0,distance=0;
  std::optional<double> acceleration;
};
struct MotionCheck { MotionGoal goal; double measured=0; bool passed=false; };
struct MotionEvaluation { std::vector<MotionCheck> checks; bool passed=false; };
[[nodiscard]] bool validMotionPlan(std::size_t chapter,const MotionPlan&);
[[nodiscard]] MotionJourney motionJourney(std::size_t chapter,const MotionPlan&);
[[nodiscard]] MotionSample sampleMotion(std::size_t chapter,const MotionPlan&,double time);
[[nodiscard]] MotionEvaluation evaluateMotion(std::size_t chapter,const MotionPlan&);
[[nodiscard]] std::vector<std::string> motionWorking(std::size_t chapter,const MotionPlan&);

struct MotionAttempt { MotionPlan plan; bool passed=false; };
struct MotionRun {
  MotionPlan plan;
  std::vector<MotionPlan> undo;
  std::vector<MotionAttempt> attempts;
  bool started=false,solved=false;
};
struct MotionProgress {
  std::array<MotionRun,motionChapterCount> runs;
  std::size_t selected=0;
  double time=0,speed=1;
  std::optional<std::size_t> inspected;
  bool recording=false;
};
enum class MotionActionKind {
  Select, SetParameter, BeginEdit, EndEdit, ChoosePlan, Undo,
  Run, Pause, Tick, Rewind, Scrub, Speed, Review, ReturnToPlan, Again, Next
};
struct MotionAction { MotionActionKind kind; std::size_t index=0; double value=0; };

// Sole kinematics, goal and attempt owner. Playback samples analytic segments;
// neither frame rate nor scene geometry participates in correctness.
class MotionLesson {
public:
  MotionLesson();
  [[nodiscard]] bool dispatch(MotionAction);
  [[nodiscard]] const MotionProgress& progress() const { return progress_; }
  [[nodiscard]] const MotionRun& run() const { return progress_.runs[progress_.selected]; }
  [[nodiscard]] const MotionChapter& chapter() const { return motionChapters()[progress_.selected]; }
  [[nodiscard]] const MotionPlan& shownPlan() const;
  [[nodiscard]] const MotionAttempt* ghost() const;
  [[nodiscard]] MotionSample sample() const { return sampleMotion(progress_.selected,shownPlan(),progress_.time); }
  [[nodiscard]] bool playing() const { return playing_; }
  [[nodiscard]] std::size_t revision() const { return revision_; }
  [[nodiscard]] std::string_view message() const { return message_; }
  // Validates the complete candidate before replacing live state. No I/O here.
  [[nodiscard]] bool restore(MotionProgress);
private:
  void halt();
  void remember(const MotionPlan&);
  bool changePlan(MotionPlan);
  MotionProgress progress_;
  std::optional<MotionPlan> editBefore_;
  bool playing_=false;
  std::size_t revision_=0;
  std::string message_;
};
} // namespace paths
