#include "content/EquationSorterContentIO.hpp"
#include "content/QuestionContentIO.hpp"

#include <array>
#include <set>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using namespace paths;
namespace fm=iggy3d::first_move;
void expect(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
fm::LinearEquation equation(std::string_view text) {
  auto result=fm::parseLinearEquation(text);
  if(!result.equation)throw std::runtime_error(std::string(result.error)+": "+std::string(text));
  return *result.equation;
}
MathematicalMove command(const GallerySession& game,fm::MathOperation operation,std::string operand,std::string entry,
                        fm::MathMoveKind kind=fm::MathMoveKind::Submit) {
  const auto& run=game.question().currentRun();
  return {game.view().challenge,{kind,operation,std::move(operand),std::move(entry),run.runNumber,run.math->revision,run.questionId,run.contentVersion}};
}
void move(GallerySession& game,fm::MathOperation operation,std::string operand,std::string entry,bool correct=true) {
  const auto result=game.dispatch(command(game,operation,std::move(operand),std::move(entry)));
  if(!result.accepted)throw std::runtime_error(std::string(result.reason));
  const auto& event=game.question().currentRun().math->events.back();
  if(event.correct!=correct)throw std::runtime_error("unexpected verdict for "+event.entry+": "+event.feedback);
}
void action(EquationSorterSession& session,SorterActionKind kind,std::uint32_t id=0,std::uint32_t value=0) {
  const auto result=session.dispatch({kind,SorterBucket::A,id,session.view().revision,value});
  if(!result.accepted)throw std::runtime_error(std::string(result.reason));
}
struct Example {
  SorterEquationId id;
  const char *factor,*shift,*divided,*expanded,*cleared,*answer;
  fm::MathOperation shiftOperation;
  const char* expandedShift;
};
const std::array examples{
  Example{1012,"-2","1","x+1=-11","-2x-2=22","-2x=24","x=-12",fm::MathOperation::Subtract,"-2"},
  Example{3001,"3","2","x+2=7","3x+6=21","3x=15","x=5",fm::MathOperation::Subtract,"6"},
  Example{3002,"4","3","x-3=2","4x-12=8","4x=20","x=5",fm::MathOperation::Add,"12"},
  Example{3003,"5","3","x+3=3","5x+15=15","5x=0","x=0",fm::MathOperation::Subtract,"15"},
  Example{3004,"4","1","x+1=5/2","4x+4=10","4x=6","x=3/2",fm::MathOperation::Subtract,"4"},
  Example{3005,"-6","2","x+2=-3/2","-6x-12=9","-6x=21","x=-7/2",fm::MathOperation::Subtract,"-12"},
};
void exactRules() {
  using Op=fm::MathOperation;
  const auto original=equation("3(x + 2) = 18");
  const auto divide=fm::checkMathMove(original,Op::Divide,"3","x + 2 = 18/3");
  expect(divide.result && divide.result->right.constant==fm::ExactNumber{6,1},"division keeps exact values");
  auto simple=fm::checkMathMove(*divide.result,Op::Simplify,"","x+2=6");
  expect(simple.result.has_value(),"simplification preserves each side");
  auto solved=fm::checkMathMove(*simple.result,Op::Subtract,"2","x=4");
  expect(solved.result && !fm::verifyMathSolution(original,*solved.result).empty(),"independent original substitution establishes completion");
  expect(fm::checkMathMove(original,Op::Expand,"","3x+6=18").result.has_value(),"expansion is also valid");
  for(const auto& input:{"x=4","3x+2=18","3(x+2)=18"})
    expect(!fm::checkMathMove(original,Op::Expand,"",input).result,"expand cannot skip to the answer, omit a term, or leave its bracket");
  expect(!fm::checkMathMove(original,Op::Divide,"3","x+2=18").result,"one-sided division fails");
  expect(!fm::checkMathMove(original,Op::Divide,"3","x+2=5").result,"incorrect arithmetic fails");
  for(const auto op:{Op::Multiply,Op::Divide})for(const auto operand:{"0","x","1/(x-x+1)","1/0"})
    expect(!fm::checkMathMove(original,op,operand,"x=4").result,"zero and variable divisors/multipliers are refused");
  const auto fraction=equation("4(x+1)=10");
  const auto rational=fm::checkMathMove(fraction,Op::Multiply,"1/4","x+1=2.5");
  expect(rational.result && rational.result->right.constant==fm::ExactNumber{5,2},"decimal entry and fractional multiplier are exact");
  expect(fm::checkMathMove(*rational.result,Op::Add,"-1","x=6/4").result->isolated,"equivalent fraction answers are accepted");
  expect(equation(".5*x+1/3=5/6").left.coefficient==fm::ExactNumber{1,2},"decimal coefficient parsed exactly");
  const auto negative=equation("-(x+2)=-6");
  expect(fm::checkMathMove(negative,Op::Expand,"","-x-2=-6").result.has_value(),"negative bracket distributes");
  for(const auto input:{"x*x=4","x^2=4","x/(x+1)=2","sin(x)=0","1 2=x","x=1/0","x=2=3","(x+2=4","x=nan","x=1e3","x=0.0000001","x=1000000001","x+\n1=2","x=∞"})
    expect(!fm::parseLinearEquation(input).equation,"unsupported or ambiguous input fails without evaluating code");
  expect(!fm::parseLinearEquation(std::string(18,'(')+"x"+std::string(18,')')+"=1").equation,"nesting is bounded");
  expect(!fm::checkMathMove(equation("1000000000x=1"),Op::Multiply,"1000000000","x=1").result,"overflow is refused");
}
void branchesAndAllExamples() {
  using Op=fm::MathOperation;
  EquationSorterSession session(loadSorterContent(SORTER_STUDY_FIXTURE));
  action(session,SorterActionKind::AutoSort);const auto owners=session.view().owners;
  std::array<GallerySession*,6> games{};
  for(std::size_t i=0;i<examples.size();++i) {
    const auto& e=examples[i];action(session,SorterActionKind::OpenSolve,e.id);
    auto& game=*session.activeSolve();games[i]=&game;
    expect(game.question().currentRun().math.has_value() && game.question().currentRun().steps.empty(),"default bracket content starts the mathematical-move owner");
    expect(game.view().choiceCount==0 && game.scene().objects().empty(),"prepared answers and sphere targets are absent in this mode");
    auto wrongIdentity=command(game,Op::Expand,"",e.expanded);wrongIdentity.move.questionId="another_question";
    expect(!game.dispatch(wrongIdentity).accepted && game.question().currentRun().math->events.empty(),"cross-question input is rejected without recording an attempt");
    expect(!game.dispatch(ChooseAnswer{game.view().challenge,{101}}).accepted &&
        !game.dispatch(GalleryHelp{game.view().challenge,GalleryHelpKind::DoStep}).accepted,"prepared routes cannot bypass mathematical entry");
    move(game,Op::Divide,e.factor,"x=999",false);
    const auto stale=command(game,Op::Divide,e.factor,e.divided);
    expect(game.dispatch(stale).accepted,"a fresh move is judged");
    expect(!game.dispatch(stale).accepted,"repeating the same move does not add evidence");
    const auto revision=game.question().currentRun().math->revision;
    action(session,SorterActionKind::ReturnToSorter);
    expect(game.view().paused,"return pauses the same owner");
    expect(!game.dispatch(command(game,Op::Subtract,"1","x=0")).accepted,"paused mathematical actions are refused");
    action(session,SorterActionKind::OpenSolve,e.id);
    expect(session.activeSolve()==&game && game.question().currentRun().math->revision==revision,"resume preserves working, revision and evidence");
    move(game,e.shiftOperation,e.shift,e.answer);
    expect(game.view().completed && !game.view().verification.empty(),"division-first route completes every signed, zero and fraction case");
    const auto summary=fm::summarizeLayeredQuestionRun(game.question().currentRun());
    expect(summary.completed && summary.correctedAfterRetry==1 && summary.correctOnFirstTry==1 && summary.incorrectCheckedAttempts==1 && !summary.assisted,"math evidence reports retries without prepared or invented answers");
    const auto before=game.question().currentRun().math->events.size();
    for(int j=0;j<240;++j)(void)game.dispatch(GalleryTick{.25F});
    expect(game.view().completed && game.question().currentRun().math->events.size()==before,"completion waits indefinitely for an explicit action");
    for(int j=0;j<2;++j)expect(game.dispatch(command(game,Op::Expand,"","",fm::MathMoveKind::Undo)).accepted,"Undo returns to the parent even after completion");
    expect(!game.view().completed && game.question().currentRun().math->nodes.size()==3 && game.question().currentRun().math->active==0,"Undo retains the first solution branch");
    move(game,Op::Expand,"",e.expanded);
    // For a negative outside factor, subtracting its negative constant is equivalent to adding its magnitude.
    move(game,e.shiftOperation,e.expandedShift,e.cleared);
    move(game,Op::Divide,e.factor,e.answer);
    const auto& math=*game.question().currentRun().math;
    expect(game.view().completed && math.nodes.size()==6 && math.nodes[3].parent==0 && math.nodes[4].parent==3,"expansion creates a second checked branch from the original problem");
    expect(game.question().review()->math==&math && game.question().review()->steps.empty(),"review exposes the same move evidence and no prepared working");
    const auto old=command(game,Op::Divide,e.factor,e.answer);
    expect(game.dispatch(ReplayQuestion{}).accepted,"explicit replay starts another attempt");
    expect(!game.dispatch(old).accepted && game.question().archivedRuns().size()==1 &&
        game.question().archivedRuns()[0].math->nodes.size()==6 && game.question().currentRun().math->nodes.size()==1,"replay archives both branches and blocks commands from the previous run");
    move(game,Op::Divide,e.factor,e.divided);move(game,e.shiftOperation,e.shift,e.answer);
    action(session,SorterActionKind::ReturnToSorter);
  }
  expect(session.view().owners==owners,"solving never changes grouping ownership");
  for(const auto* game:games)expect(game->view().completed && game->view().paused,"all six independent sessions retain their solutions");
  action(session,SorterActionKind::OpenStudy);
  action(session,SorterActionKind::StartStudy);
  expect(session.activeSolve()->question().archivedRuns().size()==2 && !session.activeSolve()->view().completed,"new study set archives finished move histories");
  for(std::size_t i=0;i<examples.size();++i) {
    const auto& e=examples[i];auto& game=*session.activeSolve();
    expect(!session.dispatch({SorterActionKind::NextSolve,SorterBucket::A,0,session.view().revision}).accepted,"Next cannot skip incomplete work");
    move(game,Op::Divide,e.factor,e.divided);move(game,e.shiftOperation,e.shift,e.answer);
    if(i+1<examples.size())action(session,SorterActionKind::NextSolve);
  }
  expect(!session.view().nextSolve,"selected bracket chapter ends without wrapping");
}
void visualChoices() {
  using Op=fm::MathOperation;
  EquationSorterSession session(loadSorterContent(SORTER_STUDY_FIXTURE));
  std::set<std::size_t> answerSlots;
  const auto choose=[&](GallerySession& game,Op operation,std::string_view operand,const char* expected) {
    const auto before=game.question().currentRun().math->active;
    const auto choices=game.question().mathMoveChoices();
    expect(!choices.empty() && choices.size()<=fm::kMathMoveChoiceCapacity,"visual move list is nonempty and bounded");
    const auto found=std::find_if(choices.begin(),choices.end(),[&](const auto& c){return c.operation==operation && c.operand==operand;});
    expect(found!=choices.end(),"known solving move is available without a typed number");
    const auto expectedEquation=equation(expected);
    const auto correct=std::find(found->results.begin(),found->results.end(),expectedEquation.display);
    expect(correct!=found->results.end(),"independently expected equation is a result choice");
    const auto slot=static_cast<std::size_t>(correct-found->results.begin());answerSlots.insert(slot);
    std::set<std::string> unique(found->results.begin(),found->results.end());
    expect(unique.size()==fm::kMathResultChoiceCount,"all result tiles are distinct");
    for(std::size_t i=0;i<found->results.size();++i)if(i!=slot) {
      const auto wrong=equation(found->results[i]);
      expect(wrong.left!=expectedEquation.left || wrong.right!=expectedEquation.right,"distractors are mathematically distinct from the known correct result");
    }
    move(game,operation,found->operand,found->results[(slot+1)%found->results.size()],false);
    expect(game.question().currentRun().math->active==before,"wrong tile retains active working");
    const auto retry=game.question().mathMoveChoices();
    for(std::size_t i=0;i<choices.size();++i)expect(choices[i].results==retry[i].results,"retry cannot shuffle the choices");
    move(game,operation,found->operand,*correct);
    expect(game.view().working==expectedEquation.display,"visual submission reaches independently expected working");
  };
  for(const auto& e:examples) {
    action(session,SorterActionKind::OpenSolve,e.id);auto& game=*session.activeSolve();
    choose(game,Op::Divide,e.factor,e.divided);choose(game,e.shiftOperation,e.shift,e.answer);
    expect(game.view().completed && game.question().mathMoveChoices().empty(),"every bracket question finishes by visual choices");
    for(int i=0;i<2;++i)expect(game.dispatch(command(game,Op::Expand,"","",fm::MathMoveKind::Undo)).accepted,"Undo restores visual choices");
    choose(game,Op::Expand,"",e.expanded);
    const auto shift=std::string(e.expandedShift);
    const bool negative=shift.starts_with('-');
    choose(game,negative?Op::Add:e.shiftOperation,negative?shift.substr(1):shift,e.cleared);
    choose(game,Op::Divide,e.factor,e.answer);
    expect(game.view().completed && game.question().currentRun().math->nodes.size()==6,"both visual routes retain their branches");
    action(session,SorterActionKind::ReturnToSorter);
  }
  expect(answerSlots.size()>1,"the correct result does not occupy a fixed slot");
  const auto expansion=fm::availableMathMoves(equation("3(x+2)=21")).front();
  expect(expansion.operation==Op::Expand &&
      std::find(expansion.results.begin(),expansion.results.end(),"3x+2 = 21")!=expansion.results.end() &&
      std::find(expansion.results.begin(),expansion.results.end(),"x+6 = 21")!=expansion.results.end(),
      "expansion tests distribution on both terms, rather than only changing the right side");
  action(session,SorterActionKind::OpenSolve,3001);
  auto& game=*session.activeSolve();expect(game.dispatch(ReplayQuestion{}).accepted,"start fresh visual route");
  choose(game,Op::Multiply,"1/3","x+2=7");
  choose(game,Op::Add,"2","x+4=9");
  choose(game,Op::Subtract,"4","x=5");
  expect(game.view().completed,"reciprocal multiplication and a valid detour remain playable");
  for(const auto input:{"-x+1=2","1/3x+2/5=7/9","1000000000x+999999999=1","x+1/1000000000=1/1000000000"}) {
    const auto before=equation(input);const auto choices=fm::availableMathMoves(before);
    expect(!choices.empty(),"bounded signed and fractional equations still have visual moves");
    for(const auto& choice:choices) {
      std::size_t valid=0;
      for(const auto& result:choice.results)if(fm::checkMathMove(before,choice.operation,choice.operand,result).result)++valid;
      expect(valid==1,"each offered move has exactly one checkable result at numeric boundaries");
    }
  }
}
fm::AugmentedMatrix matrix(std::string_view text) {
  const auto parsed=fm::parseAugmentedMatrix(text);
  if(!parsed.result)throw std::runtime_error(std::string(parsed.feedback));
  return *parsed.result;
}
void matrixRows() {
  using Op=fm::MathOperation;
  const auto original=matrix("[2,1|7] [1,-1|-1]");
  expect(!fm::checkMathMove(original,Op::DivideRow1,"0",original.display).result,"zero row division is rejected");
  expect(!fm::checkMathMove(original,Op::DivideRow1,"2","[1,0|2] [0,1|3]").result,"a matching final solution cannot pass an unrelated row operation");
  expect(!fm::checkMathMove(original,Op::Divide,"2",original.display).result,"equation operations cannot act on a matrix");
  expect(!fm::verifyMathSolution(original,matrix("[1,0|3] [0,1|2]")).size(),"identity shape alone does not establish a correct solution");
  for(const auto bad:{"[1,2|3]","[1,2|3] [4,5|6] [7,8|9]","[1,2,3] [4,5,6]","[x,2|3] [4,5|6]","[1,1/0|3] [4,5|6]","[1000000001,2|3] [4,5|6]"})
    expect(!fm::parseAugmentedMatrix(bad).result,"malformed, variable and unbounded matrix cells are refused");
  for(const auto bad:{"[1,1|2] [2,2|4]","[1,1|2] [2,2|5]","[1,0|2] [0,1|3]"})
    expect(!fm::prepareMathWorking(fm::MathWorkingModel::RowReduction,bad).result,"this first matrix exercise requires an unsolved system with a unique solution");
  expect(fm::prepareMathWorking(fm::MathWorkingModel::RowReduction,"[0,2|6] [1,1|5]").result.has_value(),"a zero first pivot is valid when swapping rows can solve it");
  expect(fm::checkMathMove(original,Op::DivideRow1,"2","[1,.5|3.5] [1,-1|-1]").result.has_value(),"matrix arithmetic uses the shared exact fraction and decimal kernel");

  EquationSorterSession session(loadSorterContent(SORTER_STUDY_FIXTURE));
  const auto type=std::find_if(session.view().study.types.begin(),session.view().study.types.end(),[](const auto& t){return t.subject==SorterSubject::LinearAlgebra;});
  expect(type!=session.view().study.types.end() && type->title=="Row reduction" && type->homes.size()==1,"one real linear algebra question is available under its own chapter");
  action(session,SorterActionKind::OpenSolve,6001);auto& game=*session.activeSolve();
  expect(std::holds_alternative<fm::AugmentedMatrix>(game.question().currentRun().math->nodes[0].equation),"matrix content starts the same owner with typed matrix working");
  const auto choose=[&](Op operation,const char* operand,const char* expected) {
    const auto choices=game.question().mathMoveChoices();
    expect(!choices.empty() && choices.size()<=fm::kMathMoveChoiceCapacity,"row choices stay within the shared palette bound");
    const auto found=std::find_if(choices.begin(),choices.end(),[&](const auto& c){return c.operation==operation && c.operand==operand;});
    expect(found!=choices.end(),"required row move is offered");
    const auto known=matrix(expected);const auto selected=std::find(found->results.begin(),found->results.end(),known.display);
    expect(selected!=found->results.end(),"independently calculated matrix appears as a result tile");
    std::size_t correct=0;for(const auto& result:found->results)if(matrix(result).rows==known.rows)++correct;
    expect(correct==1 && std::set(found->results.begin(),found->results.end()).size()==4,"four distinct matrix tiles have exactly one mathematically correct result");
    const auto old=game.question().currentRun().math->active;
    const auto wrong=found->results[(static_cast<std::size_t>(selected-found->results.begin())+1)%4];
    move(game,operation,operand,wrong,false);
    expect(game.question().currentRun().math->active==old,"wrong matrix tile preserves current working");
    const auto stale=command(game,operation,operand,*selected);
    expect(game.dispatch(stale).accepted && game.question().currentRun().math->events.back().correct,"correct matrix tile advances through the shared dispatcher");
    expect(!game.dispatch(stale).accepted,"a duplicate matrix command cannot advance again");
  };
  choose(Op::SwapRows,"","[1,-1|-1] [2,1|7]");
  choose(Op::AddRow1ToRow2,"-2","[1,-1|-1] [0,3|9]");
  choose(Op::DivideRow2,"3","[1,-1|-1] [0,1|3]");
  choose(Op::AddRow2ToRow1,"1","[1,0|2] [0,1|3]");
  expect(game.view().completed && game.view().verification.find("x = 2, y = 3")!=std::string_view::npos,"row reduction finishes with original-system verification");
  const auto summary=fm::summarizeLayeredQuestionRun(game.question().currentRun());
  expect(summary.incorrectCheckedAttempts==4 && summary.correctedAfterRetry==4 && !summary.assisted,"matrix attempts use the existing evidence and summary");
  for(int i=0;i<120;++i)(void)game.dispatch(GalleryTick{.25F});
  expect(game.view().completed,"completed matrix remains until an explicit action");
  for(int i=0;i<4;++i)expect(game.dispatch(command(game,Op::SwapRows,"","",fm::MathMoveKind::Undo)).accepted,"Undo restores each earlier matrix");
  choose(Op::DivideRow1,"2","[1,1/2|7/2] [1,-1|-1]");
  choose(Op::AddRow1ToRow2,"-1","[1,1/2|7/2] [0,-3/2|-9/2]");
  choose(Op::DivideRow2,"-3/2","[1,1/2|7/2] [0,1|3]");
  choose(Op::AddRow2ToRow1,"-1/2","[1,0|2] [0,1|3]");
  expect(game.view().completed && game.question().currentRun().math->nodes.size()==9 &&
      game.question().currentRun().math->nodes[5].parent==0,"fraction route builds a second branch without losing the first");
  action(session,SorterActionKind::ReturnToSorter);action(session,SorterActionKind::OpenSolve,6001);
  expect(session.activeSolve()==&game && game.view().completed,"return and resume retain the completed matrix");
  const auto old=command(game,Op::SwapRows,"",original.display);
  expect(game.dispatch(ReplayQuestion{}).accepted && !game.dispatch(old).accepted,"matrix replay preserves stale-input protection");
  expect(game.question().archivedRuns().back().math->nodes.size()==9 &&
      std::holds_alternative<fm::AugmentedMatrix>(game.question().currentRun().math->nodes.front().equation),"replay archives matrix history and retains its working type");
  action(session,SorterActionKind::ReturnToSorter);action(session,SorterActionKind::OpenStudy);
  action(session,SorterActionKind::ToggleStudyType,0);
  action(session,SorterActionKind::ToggleStudyType,0,static_cast<unsigned>(type-session.view().study.types.begin()));
  action(session,SorterActionKind::StartStudy);
  expect(session.view().solveCount==1 && session.activeSolve()->question().content().id=="sorter_matrix_rows","matrix-only study selection opens exactly the requested question");
}
void rowReferences() {
  EquationSorterSession session(loadSorterContent(SORTER_STUDY_FIXTURE));action(session,SorterActionKind::OpenSolve,6001);
  const auto& question=session.activeSolve()->question();const auto& run=*question.currentRun().math;
  struct Known {const char* id;std::array<std::array<std::string,3>,2> after;std::array<std::string,3> calculations;};
  const std::array expected{
    Known{"row_swap",{{{"1","2","4"},{"3","5","11"}}},{"3 <-> 1","5 <-> 2","11 <-> 4"}},
    Known{"row_scaling",{{{"1","2","4"},{"3","5","11"}}},{"6 ÷ 2 = 3","10 ÷ 2 = 5","22 ÷ 2 = 11"}},
    Known{"row_addition",{{{"1","2","4"},{"1","1","3"}}},{"3 - 2 × 1 = 1","5 - 2 × 2 = 1","11 - 2 × 4 = 3"}}};
  for(const auto& known:expected) {
    const auto* ref=question.mathReference(known.id);expect(ref!=nullptr,"each linked definition is available from the question owner");
    const auto example=fm::mathReferenceExample(*ref);
    expect(example && example->after==known.after && example->calculations==known.calculations,"reference columns match independently calculated examples, including the right-hand value");
    expect(matrix(ref->example).rows!=std::get<fm::AugmentedMatrix>(run.nodes.front().equation).rows,"reference numbers are separate from the player's problem");
    auto bad=*ref;bad.version=0;expect(!fm::mathReferenceExample(bad),"unversioned definitions cannot enter a run");
    bad=*ref;bad.definition=std::string(401,'a');expect(!fm::mathReferenceExample(bad),"reference text is bounded");
    bad=*ref;bad.id="other";expect(!fm::mathReferenceExample(bad),"a row rule cannot be mislabelled as another concept");
  }
  auto fraction=*question.mathReference("row_scaling");fraction.operation=fm::MathOperation::DivideRow1;fraction.operand="-2";
  fraction.example="[-3,1/2|4] [1,2|5]";
  const auto fractional=fm::mathReferenceExample(fraction);
  expect(fractional && fractional->after[0]==std::array<std::string,3>{"3/2","-1/4","-2"} && fractional->calculations[1]=="1/2 ÷ (-2) = -1/4","alternate target rows and signed fractions use the same exact projection");
  fraction.operand="0";expect(!fm::mathReferenceExample(fraction),"invalid example division is refused");
  fraction.operand="1";expect(!fm::mathReferenceExample(fraction),"unchanged examples are refused");
  for(const auto& choice:question.mathMoveChoices())expect(question.mathReference(choice.conceptId)!=nullptr,"every concrete row move resolves its operation family's concept ID");
  expect(!question.mathReference("unknown") && run.active==0 && run.revision==1 && run.nodes.size()==1 && run.events.empty(),"reference lookup and example projection never submit, reveal or advance player working");
  auto bad=question.content();bad.references.push_back(bad.references.front());
  expect(!fm::validateQuestion(bad,fm::QuestionInteraction::MathMoves).valid(),"direct model construction rejects duplicate references");
  action(session,SorterActionKind::ReturnToSorter);action(session,SorterActionKind::OpenSolve,3001);
  expect(!session.activeSolve()->question().mathReference("row_swap"),"unlinked questions do not expose unrelated definitions");
}
void contentAndBounds() {
  auto content=loadSorterContent(SORTER_STUDY_FIXTURE);
  const auto question=*content.front().solution;
  for(const auto input:{"x=5","x=x","x*x=2","x=1/0","4=2x+2"}) {
    auto bad=question;bad.equation=input;
    expect(!fm::validateQuestion(bad,fm::QuestionInteraction::MathMoves).valid(),"opt-in requires a supported unsolved equation with one solution");
  }
  std::ifstream input(std::filesystem::path(SORTER_STUDY_FIXTURE).parent_path()/"../cards/sorter_linear_bracket.json");
  nlohmann::json doc;input>>doc;
  for(const auto invalid:{nlohmann::json(1),nlohmann::json("other")}) {
    doc["working_model"]=invalid;bool caught=false;
    try {(void)parseQuestionContent(doc.dump(),"bad.json");}
    catch(const QuestionContentError& e) {caught=e.field=="/working_model";}
    expect(caught,"invalid mode reports its exact source field");
  }
  GalleryConfig config;config.stopAfterQuestion=true;config.mathematicalMoves=true;
  GallerySession game(config,{question},{0});
  for(std::size_t i=0;i<fm::kMathEventCapacity;++i)move(game,fm::MathOperation::Expand,"","x=999",false);
  const auto revision=game.question().currentRun().math->revision;
  expect(!game.dispatch(command(game,fm::MathOperation::Expand,"","x=999")).accepted &&
      game.question().currentRun().math->revision==revision,"bounded attempt history refuses further mutation without losing evidence");
  auto other=question;other.equation="2x=4";
  GallerySession many(config,{other},{0});
  for(std::size_t i=1;i<fm::kMathNodeCapacity;++i)
    move(many,fm::MathOperation::Add,"1","2x+"+std::to_string(i)+"="+std::to_string(4+i));
  expect(!many.dispatch(command(many,fm::MathOperation::Add,"1","2x+128=132")).accepted,"working graph has a bounded node count");
  expect(many.dispatch(command(many,fm::MathOperation::Expand,"","",fm::MathMoveKind::Undo)).accepted,"Undo remains available at the node limit");
}
int main() {
  try {exactRules();branchesAndAllExamples();visualChoices();matrixRows();rowReferences();contentAndBounds();std::cout<<"Exact algebra and matrix moves, shared row references, visual choices, alternate routes, branches, immutable evidence, resume, Next and bounds passed\n";}
  catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
