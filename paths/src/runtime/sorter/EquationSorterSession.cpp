#include "runtime/sorter/EquationSorterSession.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace paths {
namespace {
constexpr std::array<std::string_view, sorterBucketCount> bucketNames{"A", "B", "C", "D", "E", "Dump"};
bool validBucket(SorterBucket bucket) {
  return static_cast<unsigned>(bucket) < bucketNames.size();
}
}
std::string_view sorterBucketName(SorterBucket bucket) {
  return validBucket(bucket) ? bucketNames[static_cast<unsigned>(bucket)] : "?";
}

std::optional<SorterContentError> validateSorterContent(std::span<const SorterEquation> content) {
  if (content.size() != sorterEquationCount)
    return SorterContentError{"/equations", "expected exactly 100 equations"};
  for (std::size_t i = 0; i < content.size(); ++i) {
    const auto& e = content[i];
    const auto field = "/equations/" + std::to_string(i);
    if (!e.id) return SorterContentError{field + "/id", "expected a positive ID"};
    if (e.homeIndex >= sorterEquationCount)
      return SorterContentError{field + "/home_index", "expected 0 through 99"};
    if (e.text.empty() || e.text.size() > 96 || e.text.find_first_not_of(' ') == std::string::npos ||
        std::any_of(e.text.begin(), e.text.end(), [](unsigned char c) { return c < 32 || c > 126; }))
      return SorterContentError{field + "/text", "expected 1 through 96 printable ASCII bytes"};
    if (e.subject && static_cast<unsigned>(*e.subject) >= sorterSubjects.size())
      return SorterContentError{field + "/subject", "unknown maths subject"};
    if (e.hint.size() > 240 || std::any_of(e.hint.begin(), e.hint.end(), [](unsigned char c) { return c < 32 || c > 126; }))
      return SorterContentError{field + "/hint", "expected at most 240 printable ASCII bytes"};
    if(e.study) {
      if(!e.subject) return SorterContentError{field+"/study","study classification requires a subject"};
      for(const auto* text:{&e.study->chapter,&e.study->type,&e.study->form})
        if(text->empty() || text->size()>80 || text->find_first_not_of(' ')==std::string::npos ||
            std::any_of(text->begin(),text->end(),[](unsigned char c){return c<32 || c>126;}))
          return SorterContentError{field+"/study","expected nonempty chapter, type and form titles of at most 80 printable ASCII bytes"};
      for(std::size_t j=0;j<i;++j)if(content[j].study && content[j].subject==e.subject &&
          content[j].study->chapter==e.study->chapter && content[j].study->type==e.study->type && content[j].study->form!=e.study->form)
        return SorterContentError{field+"/study/form","one problem type must have one form within a chapter"};
    }
    if (e.solution) {
      namespace fm=iggy3d::first_move;
      if (!fm::validateQuestion(*e.solution,fm::QuestionInteraction::ArcadeCollect).valid() || e.solution->equation!=e.text)
        return SorterContentError{field + "/solve_pack", "solution must be valid and match the displayed equation"};
      for (const auto& step : e.solution->steps)
        if (step.hint.empty() || step.nextMove.empty() ||
            (step.semantics.purpose!=fm::StepPurpose::OperationChoice && step.semantics.purpose!=fm::StepPurpose::Calculation &&
             step.semantics.purpose!=fm::StepPurpose::GraphChoice))
          return SorterContentError{field + "/solve_pack", "solving steps need a hint, next_move, and operation_choice, calculation or graph_choice purpose"};
      if (e.solution->steps.back().explanation.empty())
        return SorterContentError{field + "/solve_pack", "solution needs a final explanation for checking the result"};
    }
    for (std::size_t j = 0; j < i; ++j) {
      if (e.id == content[j].id) return SorterContentError{field + "/id", "duplicate ID"};
      if (e.homeIndex == content[j].homeIndex)
        return SorterContentError{field + "/home_index", "duplicate home index"};
      if (e.text == content[j].text) return SorterContentError{field + "/text", "duplicate equation text"};
    }
  }
  return std::nullopt;
}

EquationSorterSession::EquationSorterSession(std::vector<SorterEquation> content)
    : content_(std::move(content)) {
  if (auto error = validateSorterContent(content_))
    throw std::invalid_argument(error->field + ": " + error->reason);
  std::sort(content_.begin(), content_.end(), [](const auto& a, const auto& b) {
    return a.homeIndex < b.homeIndex;
  });
  for(const auto& e:content_)if(e.solution && e.study) {
    auto type=std::find_if(studyTypes_.begin(),studyTypes_.end(),[&](const auto& t){
      return t.subject==*e.subject && t.chapter==e.study->chapter && t.title==e.study->type;
    });
    if(type==studyTypes_.end()) {
      const auto chapter=std::find_if(studyTypes_.begin(),studyTypes_.end(),[&](const auto& t){
        return t.subject==*e.subject && t.chapter==e.study->chapter;
      });
      const auto chapterId=chapter==studyTypes_.end()?studyTypes_.size():chapter->chapterId;
      studyTypes_.push_back({*e.subject,chapterId,e.study->chapter,e.study->type,e.study->form,{e.homeIndex}});
    } else type->homes.push_back(e.homeIndex);
  }
  if(!studyTypes_.empty())includedTypes_[0]=true;
  refreshStudySelection();
}
SorterView EquationSorterSession::view() const {
  SorterView v;
  v.owners = owners_;
  for (const auto owner : owners_) ++v.counts[owner ? static_cast<unsigned>(*owner) + 1 : 0];
  v.activeBucket = activeBucket_;
  v.inventory = inventory_;
  v.inspected = inspected_;
  v.pendingEmpty = pendingEmpty_;
  v.inventorySlots = slots_;
  v.slotCount = slotCount_;
  v.undoDepth = history_.size();
  v.revision = revision_;
  v.hintVisible = hintVisible_;
  for (const auto& e : content_) if (!owners_[e.homeIndex]) {
    if (e.subject) ++v.autoSortCount;
    else ++v.unclassifiedCount;
  }
  describeNextStep(v);
  v.solving = solving_;
  v.studying=studying_;v.studyRun=studyRun_;
  v.study={studyTypes_,includedTypes_,studyAvailable_,studySelected_,studyMode_,
      static_cast<std::size_t>(std::count(studyAvailable_.begin(),studyAvailable_.end(),true)),
      static_cast<std::size_t>(std::count(studySelected_.begin(),studySelected_.end(),true)),studyRandomCount_,!studyQueue_.empty()};
  for (const auto& e : content_) if (e.solution) {
    ++v.solveCount;
    if (solving_ && e.homeIndex==solveHome_) v.solveNumber=v.solveCount;
  }
  if(studyRun_) {v.solveCount=studyQueue_.size();v.solveNumber=studyPosition_+1;}
  if (const auto next=nextSolveHome()) v.nextSolve=content_[*next].id;
  auto candidate = std::find_if(content_.begin(),content_.end(),[&](const auto& e) { return e.id==inspected_ && e.solution; });
  if (!inspected_ && candidate==content_.end()) candidate=std::find_if(content_.begin(),content_.end(),[&](const auto& e) {
    return e.solution && (!savedSolve() || e.homeIndex==solveHome_);
  });
  if (candidate!=content_.end()) {
    v.solveCandidate=candidate->id;
    v.solveLabel=(solves_[candidate->homeIndex] ? "Resume: " : "Solve: ") + candidate->text;
  }
  return v;
}
void EquationSorterSession::describeNextStep(SorterView& v) const {
  const std::array<std::pair<bool, std::string>, 8> steps{{
      {pendingEmpty_, "Confirm to return this group, or Cancel to keep it."},
      {inventory_ && inspected_.has_value(), "Click the inspected card again to return it."},
      {inventory_ && !v.counts[static_cast<unsigned>(activeBucket_.value_or(SorterBucket::A)) + 1], "Empty group: go Back to grid, or Undo a move."},
      {inventory_, "Inspect a card, or go Back to grid."},
      {!v.counts[0], "All grouped. Open a group to review, or Undo."},
      {!activeBucket_, "Choose a group, or use Auto sort."},
      {inspected_.has_value(), "Click the inspected card again to store it in " + std::string(sorterBucketName(activeBucket_.value_or(SorterBucket::A))) + "."},
      {true, "Click a card to inspect. Hint explains its subject."}}};
  v.nextStep = std::find_if(steps.begin(), steps.end(), [](const auto& step) { return step.first; })->second;
  v.hint = v.nextStep;
  const auto card = std::find_if(content_.begin(), content_.end(), [&](const auto& e) {
    return inspected_ ? e.id == *inspected_ : inventory_ ? owners_[e.homeIndex] == activeBucket_ : !owners_[e.homeIndex];
  });
  if (!pendingEmpty_ && card != content_.end()) {
    v.hint += "\n\n" + std::string(inspected_ ? "Inspected: " : "Example available here: ") + card->text;
    if (card->subject) {
      const auto& subject = sorterSubjects[static_cast<unsigned>(*card->subject)];
      v.hint += "\n\nSuggested group: " + std::string(sorterBucketName(subject.bucket)) + " - " + std::string(subject.name) + ".";
    } else v.hint += "\n\nNo subject is supplied for this card. Choose its group yourself.";
    if (!card->hint.empty()) v.hint += "\n" + card->hint;
  }
  v.hint += "\n\nAuto sort groups remaining cards by subject. Previous moves stay in place. One Undo reverses the automatic move.";
  if (v.unclassifiedCount) v.hint += "\n" + std::to_string(v.unclassifiedCount) + " remaining cards have no subject and need manual grouping.";
  v.hint += "\n\nYou can choose any group. On the grid, choose a group then click a card twice to inspect and store it. To change a stored card's group, return it to the grid first.";
}
SorterResult EquationSorterSession::accept(bool changed) {
  if (changed) ++revision_;
  return {true, changed, {}};
}
std::optional<std::size_t> EquationSorterSession::home(SorterEquationId id) const {
  for (const auto& e : content_) if (e.id == id) return e.homeIndex;
  return std::nullopt;
}
void EquationSorterSession::reconcileInventorySlots() {
  if (!inventory_) return;
  for (const auto& e : content_) {
    if (owners_[e.homeIndex] == activeBucket_ &&
        std::find(slots_.begin(), slots_.begin() + slotCount_, e.id) == slots_.begin() + slotCount_)
      slots_[slotCount_++] = e.id;
  }
}
void EquationSorterSession::openInventory() {
  inventory_ = true;
  inspected_.reset();
  pendingEmpty_ = false;
  slotCount_ = 0;
  reconcileInventorySlots();
}
SorterResult EquationSorterSession::commitTransaction(Transaction changes) {
  if (changes.empty()) return accept(false);
  std::array<bool, sorterEquationCount> seen{};
  for (const auto& change : changes) {
    const auto index = home(change.equation);
    if (!index || seen[*index] ||
        owners_[*index] != change.from || change.from == change.to ||
        (change.to && !validBucket(*change.to)))
      return {false, false, "invalid transaction"};
    seen[*index] = true;
  }
  // Complete potentially throwing allocation before changing membership.
  history_.push_back(std::move(changes));
  for (const auto& change : history_.back()) owners_[*home(change.equation)] = change.to;
  inspected_.reset();
  pendingEmpty_ = false;
  reconcileInventorySlots();
  return accept(true);
}
SorterResult EquationSorterSession::undo() {
  if (history_.empty()) {
    const bool changed = inspected_.has_value() || pendingEmpty_;
    inspected_.reset();
    pendingEmpty_ = false;
    return accept(changed);
  }
  for (const auto& change : history_.back())
    if (owners_[*home(change.equation)] != change.to) return {false, false, "stale Undo"};
  for (const auto& change : history_.back()) owners_[*home(change.equation)] = change.from;
  history_.pop_back();
  inspected_.reset();
  pendingEmpty_ = false;
  reconcileInventorySlots();
  return accept(true);
}
SorterResult EquationSorterSession::activateEquation(SorterEquationId id) {
  const auto index = home(id);
  if (!index || (inventory_ ? owners_[*index] != activeBucket_ : owners_[*index].has_value()))
    return {false, false, "equation unavailable in this view"};
  if (pendingEmpty_ || inspected_ != id) {
    pendingEmpty_ = false;
    inspected_ = id;
    return accept(true);
  }
  if (!inventory_ && !activeBucket_) return accept(false);
  return commitTransaction({{id, owners_[*index], inventory_ ? SorterOwner{} : activeBucket_}});
}
SorterResult EquationSorterSession::dispatch(const SorterAction& action) {
  switch(action.kind) {
  case SorterActionKind::OpenStudy:case SorterActionKind::CloseStudy:
  case SorterActionKind::ToggleStudySubject:case SorterActionKind::ToggleStudyChapter:case SorterActionKind::ToggleStudyType:
  case SorterActionKind::SetStudyMode:case SorterActionKind::SetStudyCount:case SorterActionKind::ShuffleStudy:
  case SorterActionKind::ToggleStudyQuestion:case SorterActionKind::StartStudy:case SorterActionKind::ResumeStudy:
  case SorterActionKind::ReturnToStudy:return dispatchStudy(action);
  default:break;
  }
  if (solving_ && action.kind!=SorterActionKind::ReturnToSorter && action.kind!=SorterActionKind::NextSolve)
    return {false,false,"return to the sorter before grouping"};
  if ((action.kind == SorterActionKind::ActivateEquation || action.kind == SorterActionKind::ConfirmEmpty ||
       action.kind == SorterActionKind::AutoSort || action.kind == SorterActionKind::OpenSolve ||
       action.kind == SorterActionKind::NextSolve) &&
      action.revision != revision_) return {false, false, "stale interaction"};
  switch (action.kind) {
  case SorterActionKind::SelectBucket:
    if (!validBucket(action.bucket)) return {false, false, "invalid bucket"};
    if (inventory_ && activeBucket_ == action.bucket) return accept(false);
    if (inventory_ || activeBucket_ == action.bucket || std::all_of(owners_.begin(), owners_.end(), [](const auto& owner) { return owner.has_value(); })) {
      activeBucket_ = action.bucket;
      openInventory();
    } else {
      activeBucket_ = action.bucket;
      inspected_.reset();
      pendingEmpty_ = false;
    }
    return accept(true);
  case SorterActionKind::BackToGrid: {
    const bool changed = inventory_ || inspected_.has_value() || pendingEmpty_;
    inventory_ = false;
    inspected_.reset();
    pendingEmpty_ = false;
    return accept(changed);
  }
  case SorterActionKind::ActivateEquation: return activateEquation(action.equation);
  case SorterActionKind::ClearInspection: {
    const bool changed = inspected_.has_value();
    inspected_.reset();
    return accept(changed);
  }
  case SorterActionKind::RequestEmpty:
    if (!inventory_) return {false, false, "open an inventory first"};
    if (std::find(owners_.begin(), owners_.end(), activeBucket_) == owners_.end() || pendingEmpty_)
      return accept(false);
    inspected_.reset();
    pendingEmpty_ = true;
    return accept(true);
  case SorterActionKind::ConfirmEmpty: {
    if (!inventory_ || !pendingEmpty_) return {false, false, "no pending empty confirmation"};
    Transaction changes;
    for (const auto& e : content_)
      if (owners_[e.homeIndex] == activeBucket_) changes.push_back({e.id, activeBucket_, {}});
    return commitTransaction(std::move(changes));
  }
  case SorterActionKind::CancelEmpty: {
    const bool changed = pendingEmpty_;
    pendingEmpty_ = false;
    return accept(changed);
  }
  case SorterActionKind::Undo: return undo();
  case SorterActionKind::AutoSort: {
    Transaction changes;
    for (const auto& e : content_)
      if (!owners_[e.homeIndex] && e.subject)
        changes.push_back({e.id, {}, sorterSubjects[static_cast<unsigned>(*e.subject)].bucket});
    return commitTransaction(std::move(changes));
  }
  case SorterActionKind::ShowHint: {
    const bool changed = !hintVisible_;
    hintVisible_ = true;
    return accept(changed);
  }
  case SorterActionKind::CloseHint: {
    const bool changed = hintVisible_;
    hintVisible_ = false;
    return accept(changed);
  }
  case SorterActionKind::OpenSolve: {
    const auto index=home(action.equation);
    if (!index || !content_[*index].solution) return {false,false,"no prepared solution for this card"};
    const auto result=openSolve(*index);studyRun_=false;return result;
  }
  case SorterActionKind::NextSolve: {
    const auto next=nextSolveHome();
    if (!next) return {false,false,"no next prepared question or current question unfinished"};
    const auto result=openSolve(*next);if(studyRun_)++studyPosition_;return result;
  }
  case SorterActionKind::ReturnToSorter:
    if (!solving_) return accept(false);
    (void)solves_[solveHome_]->dispatch(GalleryPause{true});
    solving_=false;
    return accept(true);
  default:break;
  }
  return {false, false, "invalid action"};
}
std::optional<std::size_t> EquationSorterSession::nextSolveHome() const {
  if (!solving_ || !savedSolve()->question().currentRun().completed) return std::nullopt;
  if(studyRun_)return studyPosition_+1<studyQueue_.size()?std::optional{studyQueue_[studyPosition_+1]}:std::nullopt;
  for (std::size_t i=solveHome_+1;i<content_.size();++i)
    if (content_[i].solution) return i;
  return std::nullopt;
}
std::unique_ptr<GallerySession> EquationSorterSession::freshSolve(std::size_t index,
    std::span<const iggy3d::first_move::LayeredQuestionCommand> savedProgress) const {
  std::unique_ptr<GallerySession> game;
  if(solves_[index]) {
    game=std::make_unique<GallerySession>(*solves_[index]);
    const auto result=game->dispatch(ReplayQuestion{true});
    if(!result.accepted)throw std::logic_error(std::string(result.reason));
  } else {
    GalleryConfig config; config.variation=GalleryVariation::EquationChain;
    config.motion=RouteKind::Horizontal; config.pace=.4F; config.stopAfterQuestion=true;
    config.mathematicalMoves=content_[index].solution->supportsMathMoves;
    game=std::make_unique<GallerySession>(config,
        std::vector<iggy3d::first_move::LayeredQuestionContent>{*content_[index].solution},std::vector<std::size_t>{0},savedProgress);
  }
  (void)game->dispatch(GalleryPause{true});return game;
}
SorterResult EquationSorterSession::openSolve(std::size_t index) {
  // Entry and Next share preparation; existing runs resume through their owner.
  if(!solves_[index])solves_[index]=freshSolve(index);
  if (solving_) {
    (void)solves_[solveHome_]->dispatch(GalleryPause{true});
    inspected_.reset();
  }
  solveHome_=index;
  (void)solves_[solveHome_]->dispatch(GalleryPause{false});
  solving_=true;studying_=false;hintVisible_=false;pendingEmpty_=false;
  return accept(true);
}
void EquationSorterSession::refreshStudySelection() {
  studyAvailable_.fill(false);
  for(std::size_t i=0;i<studyTypes_.size();++i)if(includedTypes_[i])
    for(const auto index:studyTypes_[i].homes)studyAvailable_[index]=true;
  if(studyMode_==StudyMode::Specific) {
    for(std::size_t i=0;i<content_.size();++i)studySelected_[i]=studySelected_[i] && studyAvailable_[i];
    return;
  }
  studySelected_=studyAvailable_;
  if(studyMode_==StudyMode::Random) {
    std::vector<std::size_t> pool;
    for(std::size_t i=0;i<content_.size();++i)if(studyAvailable_[i])pool.push_back(i);
    std::shuffle(pool.begin(),pool.end(),studyRandom_);
    studySelected_.fill(false);
    for(std::size_t i=0;i<std::min(studyRandomCount_,pool.size());++i)studySelected_[pool[i]]=true;
  }
}
SorterResult EquationSorterSession::dispatchStudy(const SorterAction& action) {
  if(action.revision!=revision_)return {false,false,"stale interaction"};
  if(solving_ && action.kind!=SorterActionKind::ReturnToStudy)return {false,false,"return to contents before changing the selection"};
  if(!studying_ && action.kind!=SorterActionKind::OpenStudy && action.kind!=SorterActionKind::ReturnToStudy)
    return {false,false,"open table of contents first"};
  switch(action.kind) {
  case SorterActionKind::OpenStudy: {
    const bool changed=!studying_;studying_=true;hintVisible_=false;return accept(changed);
  }
  case SorterActionKind::CloseStudy:studying_=false;return accept(true);
  case SorterActionKind::ReturnToStudy:
    if(!solving_)return {false,false,"no active question"};
    (void)solves_[solveHome_]->dispatch(GalleryPause{true});
    solving_=false;studying_=true;return accept(true);
  case SorterActionKind::ToggleStudySubject:case SorterActionKind::ToggleStudyChapter:case SorterActionKind::ToggleStudyType: {
    std::vector<std::size_t> matches;
    for(std::size_t i=0;i<studyTypes_.size();++i) {
      const auto& t=studyTypes_[i];
      const bool match=action.kind==SorterActionKind::ToggleStudySubject?static_cast<unsigned>(t.subject)==action.value:
          action.kind==SorterActionKind::ToggleStudyChapter?t.chapterId==action.value:i==action.value;
      if(match)matches.push_back(i);
    }
    if(matches.empty())return {false,false,"no prepared problems in this title"};
    const bool enable=!std::all_of(matches.begin(),matches.end(),[&](auto i){return includedTypes_[i];});
    for(const auto i:matches)includedTypes_[i]=enable;
    refreshStudySelection();return accept(true);
  }
  case SorterActionKind::SetStudyMode:
    if(action.value>static_cast<unsigned>(StudyMode::Specific))return {false,false,"invalid selection mode"};
    if(studyMode_==static_cast<StudyMode>(action.value))return accept(false);
    studyMode_=static_cast<StudyMode>(action.value);refreshStudySelection();return accept(true);
  case SorterActionKind::SetStudyCount:
    if(!action.value || action.value>sorterEquationCount)return {false,false,"random count must be 1 through 100"};
    if(studyRandomCount_==action.value)return accept(false);
    studyRandomCount_=action.value;if(studyMode_==StudyMode::Random)refreshStudySelection();return accept(true);
  case SorterActionKind::ShuffleStudy:
    if(studyMode_!=StudyMode::Random)return {false,false,"choose random selection first"};
    refreshStudySelection();return accept(true);
  case SorterActionKind::ToggleStudyQuestion: {
    const auto index=home(action.equation);
    if(!index || !studyAvailable_[*index])return {false,false,"question is outside the chosen titles"};
    studyMode_=StudyMode::Specific;studySelected_[*index]=!studySelected_[*index];return accept(true);
  }
  case SorterActionKind::StartStudy: {
    std::vector<std::size_t> queue;
    std::vector<std::unique_ptr<GallerySession>> prepared;
    for(std::size_t i=0;i<content_.size();++i)if(studySelected_[i]) {
      queue.push_back(i);prepared.push_back(freshSolve(i));
    }
    if(queue.empty())return {false,false,"select at least one problem"};
    // All allocation and replay preparation precede replacing any saved owner.
    for(std::size_t i=0;i<queue.size();++i)solves_[queue[i]]=std::move(prepared[i]);
    studyQueue_=std::move(queue);studyPosition_=0;studyRun_=true;
    return openSolve(studyQueue_[0]);
  }
  case SorterActionKind::ResumeStudy:
    if(studyQueue_.empty())return {false,false,"no study session to resume"};
    studyRun_=true;return openSolve(studyQueue_[studyPosition_]);
  default:return {false,false,"invalid study action"};
  }
}
StudyProgress EquationSorterSession::studyProgress() const {
  StudyProgress saved;
  saved.mode=studyMode_;saved.randomCount=studyRandomCount_;saved.position=studyPosition_;
  for(std::size_t i=0;i<studyTypes_.size();++i)if(includedTypes_[i]) {
    const auto& t=studyTypes_[i];saved.titles.push_back({t.subject,t.chapter,t.title});
  }
  for(const auto index:studyQueue_)saved.queue.push_back(content_[index].id);
  for(std::size_t i=0;i<content_.size();++i) {
    if(studySelected_[i])saved.selected.push_back(content_[i].id);
    if(solves_[i]) {
      const auto& question=solves_[i]->question();
      const auto commands=question.journal();
      saved.questions.push_back({content_[i].id,question.content().id,question.content().version,{commands.begin(),commands.end()}});
    }
  }
  return saved;
}
std::uint64_t EquationSorterSession::progressRevision() const {
  auto revision=revision_;
  for(const auto& game:solves_)if(game)revision+=game->question().journal().size();
  return revision;
}
void EquationSorterSession::restoreStudyProgress(const StudyProgress& saved) {
  const auto require=[](bool valid,const char* reason) {if(!valid)throw std::invalid_argument(reason);};
  EquationSorterSession staged(content_);
  staged.includedTypes_.fill(false);
  require(static_cast<unsigned>(saved.mode)<=static_cast<unsigned>(StudyMode::Specific),"invalid saved selection mode");
  require(saved.randomCount>0 && saved.randomCount<=sorterEquationCount,"invalid saved random count");
  for(const auto& key:saved.titles) {
    const auto found=std::find_if(studyTypes_.begin(),studyTypes_.end(),[&](const auto& t) {
      return t.subject==key.subject && t.chapter==key.chapter && t.title==key.type;
    });
    require(found!=studyTypes_.end(),"saved title is no longer available");
    const auto index=static_cast<std::size_t>(found-studyTypes_.begin());
    require(!staged.includedTypes_[index],"duplicate saved title");staged.includedTypes_[index]=true;
  }
  staged.studyMode_=saved.mode;staged.studyRandomCount_=saved.randomCount;
  staged.refreshStudySelection();staged.studySelected_.fill(false);
  for(const auto id:saved.selected) {
    const auto index=home(id);
    require(index && staged.studyAvailable_[*index],"saved question is outside the selected titles");
    require(!staged.studySelected_[*index],"duplicate saved selection");staged.studySelected_[*index]=true;
  }
  const auto available=static_cast<std::size_t>(std::count(staged.studyAvailable_.begin(),staged.studyAvailable_.end(),true));
  if(saved.mode==StudyMode::All)require(saved.selected.size()==available,"saved all selection has changed");
  if(saved.mode==StudyMode::Random)require(saved.selected.size()==std::min(available,saved.randomCount),"saved random selection has changed");
  for(const auto& question:saved.questions) {
    const auto index=home(question.equation);
    require(index && content_[*index].solution,"saved question is no longer available");
    require(!staged.solves_[*index],"duplicate saved question");
    const auto& content=*content_[*index].solution;
    require(content.id==question.questionId && content.version==question.contentVersion,"saved question version has changed");
    require(!question.commands.empty(),"saved question has no opening command");
    staged.solves_[*index]=staged.freshSolve(*index,question.commands);
  }
  for(const auto id:saved.queue) {
    const auto index=home(id);
    require(index && content_[*index].study && staged.solves_[*index],"saved practice queue has a missing question");
    require(std::find(staged.studyQueue_.begin(),staged.studyQueue_.end(),*index)==staged.studyQueue_.end(),"duplicate saved queue question");
    staged.studyQueue_.push_back(*index);
  }
  require(saved.queue.empty()?saved.position==0:saved.position<saved.queue.size(),"invalid saved queue position");
  staged.studyPosition_=saved.position;staged.studyRun_=!saved.queue.empty();staged.studying_=!studyTypes_.empty();
  if(staged.studyRun_)staged.solveHome_=staged.studyQueue_[saved.position];
  staged.revision_=revision_+1;
  *this=std::move(staged);
}
} // namespace paths
