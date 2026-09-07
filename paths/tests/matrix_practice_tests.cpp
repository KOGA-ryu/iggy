#include "content/EquationSorterContentIO.hpp"
#include "content/QuestionContentIO.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <stdexcept>
#include <nlohmann/json.hpp>

using namespace paths;
namespace fm=iggy3d::first_move;
using Json=nlohmann::json;
#ifndef MATRIX_PRACTICE_ROOT
#define MATRIX_PRACTICE_ROOT "."
#endif
void require(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
Json read(const std::filesystem::path& path) {std::ifstream file(path);require(bool(file),"cannot read "+path.string());return Json::parse(file);}
std::size_t longestMatrix=0,longestRow=0;
unsigned longestMatrixId=0;
std::string longestMatrixText;
void measure(std::string_view text,unsigned id) {
  if(text.size()>longestMatrix){longestMatrix=text.size();longestMatrixId=id;longestMatrixText=text;}
  const auto newline=text.find('\n');
  longestRow=std::max(longestRow,newline==std::string_view::npos?text.size():std::max(newline,text.size()-newline-1));
}

// Test-only rational arithmetic independently computes all six result cells.
// It does not call the runtime transformation or use generated answer options.
struct Rational {
  std::int64_t n=0,d=1;
  Rational(std::int64_t numerator=0,std::int64_t denominator=1):n(numerator),d(denominator) {
    require(d!=0,"test rational denominator is nonzero");
    if(d<0){n=-n;d=-d;}const auto divisor=std::gcd(n,d);n/=divisor;d/=divisor;
  }
  explicit Rational(const std::string& text) {
    const auto slash=text.find('/');*this=Rational(std::stoll(text.substr(0,slash)),slash==std::string::npos?1:std::stoll(text.substr(slash+1)));
  }
  bool operator==(const Rational&)const=default;
};
Rational operator+(Rational a,Rational b){return {a.n*b.d+b.n*a.d,a.d*b.d};}
Rational operator-(Rational a,Rational b){return {a.n*b.d-b.n*a.d,a.d*b.d};}
Rational operator*(Rational a,Rational b){return {a.n*b.n,a.d*b.d};}
Rational operator/(Rational a,Rational b){return {a.n*b.d,a.d*b.n};}
using Matrix=std::array<std::array<Rational,3>,2>;
Matrix decoded(std::string_view text) {
  const auto parsed=fm::parseAugmentedMatrix(text);require(parsed.result.has_value(),"runtime parses displayed matrix");
  Matrix result;
  for(std::size_t row=0;row<2;++row)for(std::size_t col=0;col<3;++col)result[row][col]={parsed.result->rows[row][col].numerator,parsed.result->rows[row][col].denominator};
  return result;
}
Matrix original(const Json& recipe) {
  Matrix matrix;
  for(std::size_t row=0;row<2;++row)for(std::size_t col=0;col<3;++col)matrix[row][col]=Rational(recipe.at("rows").at(row).at(col).get<std::string>());
  return matrix;
}
fm::MathOperation operation(const std::string& key) {
  const auto found=std::find_if(fm::rowOperations.begin(),fm::rowOperations.end(),[&](const auto& row){return row.key==key;});
  require(found!=fm::rowOperations.end(),"recipe names an existing semantic row operation");return found->operation;
}
Matrix transformed(Matrix before,fm::MathOperation op,const std::string& operand) {
  using Op=fm::MathOperation;
  if(op==Op::SwapRows){std::swap(before[0],before[1]);return before;}
  const Rational factor(operand);
  if(op==Op::DivideRow1 || op==Op::DivideRow2) {
    for(auto& cell:before[op==Op::DivideRow1?0:1])cell=cell/factor;
  } else {
    require(op==Op::AddRow1ToRow2 || op==Op::AddRow2ToRow1,"only row moves enter the independent checker");
    const auto target=op==Op::AddRow1ToRow2?1:0,source=1-target;
    for(std::size_t col=0;col<3;++col)before[target][col]=before[target][col]+factor*before[source][col];
  }
  return before;
}
MathematicalMove command(const GallerySession& game,fm::MathOperation op,std::string operand,std::string entry,
                        fm::MathMoveKind kind=fm::MathMoveKind::Submit) {
  const auto& run=game.question().currentRun();
  return {game.view().challenge,{kind,op,std::move(operand),std::move(entry),run.runNumber,run.math->revision,run.questionId,run.contentVersion}};
}
void action(EquationSorterSession& session,SorterActionKind kind,std::uint32_t id=0) {
  const auto result=session.dispatch({kind,SorterBucket::A,id,session.view().revision});require(result.accepted,std::string(result.reason));
}
void playRoute(GallerySession& game,const Json& recipe,std::size_t routeIndex,const Json& card) {
  Matrix working=original(recipe);
  const auto& moves=recipe.at("routes").at(routeIndex).at("moves");
  std::size_t step=0;
  for(const auto& move:moves) {
    const auto op=operation(move.at("operation"));const auto operand=move.at("operand").get<std::string>();
    const auto expected=transformed(working,op,operand);
    require(!game.view().completed,"route never continues after completion");
    require(decoded(game.view().working)==working,"game working matches independent route cells");
    const auto choices=game.question().mathMoveChoices();
    measure(game.view().working,recipe.at("sorter_id"));
    require(!choices.empty() && choices.size()<=fm::kMathMoveChoiceCapacity,"live palette is bounded and nonempty");
    for(const auto& choice:choices) {
      require(game.question().mathReference(choice.conceptId)!=nullptr,"every operation resolves the shared row reference");
      require(std::set(choice.results.begin(),choice.results.end()).size()==4,"every operation has four distinct result tiles");
      const auto correct=transformed(working,choice.operation,choice.operand);std::size_t count=0;
      for(const auto& tile:choice.results){measure(tile,recipe.at("sorter_id"));if(decoded(tile)==correct)++count;}
      require(count==1,"every offered move has exactly one independently correct tile");
    }
    const auto choice=std::find_if(choices.begin(),choices.end(),[&](const auto& candidate){return candidate.operation==op && candidate.operand==operand;});
    require(choice!=choices.end(),"authored route move is offered: "+move.dump());
    const auto tile=std::find_if(choice->results.begin(),choice->results.end(),[&](const auto& candidate){return decoded(candidate)==expected;});
    require(tile!=choice->results.end(),"independently calculated result appears in the live tiles");
    if(routeIndex==0) {
      const auto& authored=card.at("steps").at(step);
      require(authored.at("prompt")==choice->label && authored.at("next_move")==choice->label,"authored symbol matches the offered operation");
      require(decoded(card.at("working_states").at(step).at("display").get<std::string>())==working,"authored before state is independent working");
      require(decoded(card.at("working_states").at(step+1).at("display").get<std::string>())==expected,"authored after state is independently calculated");
      std::vector<unsigned> accepted;
      for(const auto& option:authored.at("options"))if(decoded(option.at("label").get<std::string>())==expected)accepted.push_back(option.at("id"));
      require(Json(accepted)==authored.at("accepted_option_ids"),"authored accepted IDs match independent mathematics");
    }
    const auto wrong=std::find_if(choice->results.begin(),choice->results.end(),[&](const auto& candidate){return decoded(candidate)!=expected;});
    const auto active=game.question().currentRun().math->active;
    require(game.dispatch(command(game,op,operand,*wrong)).accepted,"wrong tile is recorded through the canonical owner");
    require(!game.question().currentRun().math->events.back().correct && game.question().currentRun().math->active==active,"wrong tile retains working");
    const auto retry=game.question().mathMoveChoices();
    require(retry.size()==choices.size(),"wrong tile retains operation choices");
    for(std::size_t index=0;index<retry.size();++index)require(retry[index].results==choices[index].results,"wrong tile retains result order");
    const auto submission=command(game,op,operand,*tile);
    require(game.dispatch(submission).accepted && game.question().currentRun().math->events.back().correct,"correct live result advances through GallerySession");
    require(!game.dispatch(submission).accepted,"duplicate result is stale");
    working=expected;++step;
  }
  require(game.view().completed && game.question().mathMoveChoices().empty(),"complete live route reaches the question owner's finish state");
  const Rational x(recipe.at("answer").at(0).get<std::string>()),y(recipe.at("answer").at(1).get<std::string>());
  require(working==Matrix{{{{1,0,x}},{{0,1,y}}}},"final identity contains the declared exact answers");
  for(const auto& row:original(recipe))require(row[0]*x+row[1]*y==row[2],"final x and y satisfy both original equations independently");
  require(game.view().verification.find("Checked.")!=std::string_view::npos,"runtime independently verifies completion in the original system");
}
void exercise(EquationSorterSession& session,const Json& recipe,const std::filesystem::path& root) {
  const auto id=recipe.at("sorter_id").get<unsigned>();const auto identity="sorter_matrix_practice_"+std::to_string(id);
  const auto card=read(root/"content/cards"/(identity+".json"));
  action(session,SorterActionKind::OpenSolve,id);auto& game=*session.activeSolve();
  require(game.question().content().id==identity && game.question().content().version==1,"question identity and initial content version are stable");
  require(game.question().content().mathModel==fm::MathWorkingModel::RowReduction,"new questions use the existing row-reduction owner");
  const auto rows=original(recipe);
  require(rows[0][0]*rows[1][1]-rows[0][1]*rows[1][0]!=Rational{},"original determinant guarantees one solution");
  playRoute(game,recipe,0,card);
  const auto firstCount=recipe.at("routes").at(0).at("moves").size();
  const auto firstNodes=game.question().currentRun().math->nodes.size();
  for(int tick=0;tick<120;++tick)(void)game.dispatch(GalleryTick{.25F});
  require(game.view().completed,"finished working stays until an explicit action");
  for(std::size_t index=0;index<firstCount;++index)require(game.dispatch(command(game,fm::MathOperation::SwapRows,"","",fm::MathMoveKind::Undo)).accepted,"Undo returns to the original matrix");
  require(game.question().currentRun().math->nodes.size()==firstNodes,"Undo preserves all earlier checked nodes");
  playRoute(game,recipe,1,card);
  const auto secondCount=recipe.at("routes").at(1).at("moves").size();
  const auto& run=game.question().currentRun();
  require(run.math->nodes.size()==1+firstCount+secondCount && run.math->nodes[firstNodes].parent==0,"alternate route makes a retained branch from the original");
  const auto summary=fm::summarizeLayeredQuestionRun(run);
  require(summary.incorrectCheckedAttempts==firstCount+secondCount && !summary.assisted,"both routes preserve wrong-choice evidence without assistance");
  action(session,SorterActionKind::ReturnToSorter);action(session,SorterActionKind::OpenSolve,id);
  require(session.activeSolve()==&game && game.view().completed,"return and resume retain this problem and completed branches");
  require(game.dispatch(ReplayQuestion{}).accepted,"existing replay route starts another attempt");
  require(game.question().archivedRuns().back().math->nodes.size()==1+firstCount+secondCount,"replay archives both solving routes intact");
  action(session,SorterActionKind::ReturnToSorter);
  std::cout<<id<<": both offered routes, all prepared states, independent original substitution, retry and branches passed\n";
}
int main(int argc,char** argv) {
  try {
    const std::filesystem::path root=MATRIX_PRACTICE_ROOT;
    const auto document=read(root/"content/authoring/matrix_practice_recipes.json");
    const bool first=argc==2 && std::string_view(argv[1])=="--first-example";
    const auto content=loadSorterContent(root/"content/sorter/matrix_practice_v1.json");
    require(content.size()==100,"standalone practice catalogue has exactly 100 records");
    EquationSorterSession session(content);
    if(first)exercise(session,document.at("recipes").at(0),root);
    else {
      require(document.at("recipes").size()==12,"chapter contains all twelve requested problems");
      for(const auto& recipe:document.at("recipes"))exercise(session,recipe,root);
    }
    std::cout<<(first?"First complete example":"Twelve-problem chapter")<<" passed through the existing GallerySession and LayeredQuestionSession owners\n";
    std::cout<<"Longest live matrix/result: "<<longestMatrix<<" ASCII characters; longest row: "<<longestRow
             <<"; card "<<longestMatrixId<<"\n"<<longestMatrixText<<'\n';
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
