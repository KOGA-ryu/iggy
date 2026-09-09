#include "runtime/textbook/Textbook.hpp"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace paths;
namespace {
unsigned checks=0;
void require(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
void act(Textbook& book,BookAction action){const auto result=book.dispatch(action);if(!result.accepted)throw std::runtime_error(result.reason);}
void same(const BookView& a,const BookView& b){require(a.page==b.page&&a.mode==b.mode&&a.section==b.section&&a.scroll==b.scroll&&a.textScale==b.textScale&&a.anchor==b.anchor&&a.anchorRevision==b.anchorRevision,"Rejected navigation changed reading state");}
void reject(Textbook& book,BookAction action){const auto before=book.view();const auto bookmark=book.bookmark();const auto evidence=book.hasBoard()?book.board().view().steps:book.objectLesson().snapshot(true).revision;require(!book.dispatch(action).accepted,"Invalid action accepted");same(before,book.view());require(book.bookmark()==bookmark&&(book.hasBoard()?book.board().view().steps:book.objectLesson().snapshot(true).revision)==evidence,"Rejected action changed bookmark or exercise");}
std::string disclosures(const Textbook& book){
  std::string result;
  for(const auto& block:book.lessonView()){
    result+=block.id;
    for(const auto& help:block.help){result+=help.open?'1':'0';for(const auto& p:help.passages)result+=p.text;}
  }
  return result;
}
BookBlockView block(const Textbook& book,std::string_view id){
  for(const auto& b:book.lessonView())if(b.id==id)return b;
  throw std::runtime_error("Expected visible lesson block");
}
void rejectedHelp(Textbook& book,BookAction action){
  const auto before=disclosures(book);reject(book,action);
  require(disclosures(book)==before,"Rejected action changed published help");
}
void badBookmark(Textbook& book,const std::string& data){const auto before=book.view();const auto bookmark=book.bookmark();require(!book.restoreBookmark(data).accepted,"Malformed bookmark accepted");same(before,book.view());require(book.bookmark()==bookmark,"Malformed bookmark partially committed");}
std::string bytes(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
struct Temporary {
  std::filesystem::path path=std::filesystem::temp_directory_path()/("paths-textbook-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary(){if(!std::filesystem::create_directory(path))throw std::runtime_error("Cannot create unique test directory");}
  ~Temporary(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(){try{
  Textbook book;const auto& sections=matrixChapter();require(sections.size()>=9&&textbookParts().size()==7,"Book outline size");
  std::set<std::string> ids,terms;std::set<unsigned> cards;
  const std::array<unsigned,8> order{4,4,0,31,18,44,1,59};
  for(unsigned i=0;i<sections.size();++i){const auto& section=sections[i];require(ids.insert(section.id).second,"Duplicate stable section id");if(i<order.size())require(section.card==order[i],"Teaching sequence changed");cards.insert(section.card);
    require(std::string(section.purpose).size()>20&&(!section.lesson.empty()||std::string(section.figurePrompt).size()>20)&&std::string(section.exercisePrompt).size()>20&&std::string(section.reference).size()>20,"Incomplete section");
    for(const auto paragraph:section.explanation)require(std::string(paragraph).size()>40,"Missing explanation");
    for(const auto step:section.exampleSteps)require(std::string(step).size()>30,"Missing worked example step");
    for(const auto& term:section.terms){require(terms.insert(term.name).second,"Duplicate index term");require(std::string(term.definition).size()>20,"Missing definition");}
    act(book,{BookActionKind::OpenSection,i});
    if(section.exercise==BookExerciseKind::Object){
      require(!book.hasBoard()&&section.object&&section.figure.kind==BookFigureKind::Object,"Incomplete object exercise binding");
      require(book.objectLesson().snapshot().feedback==MathFeedback::None&&book.objectLesson().snapshot(true).feedback==MathFeedback::None,"Reading created object feedback");
      act(book,{BookActionKind::RememberScroll,i,123.5*(i+1)});act(book,{BookActionKind::Exercise});
      require(book.objectLesson().snapshot(true).feedback==MathFeedback::None,"Opening object practice checked it");
      act(book,{BookActionKind::Read});continue;
    }
    require(book.board().view().card==section.card,"Wrong exercise binding");require(!book.board().view().working&&!book.board().view().checked,"Reading marked an exercise as worked or checked");
    act(book,{BookActionKind::RememberScroll,i,123.5*(i+1)});act(book,{BookActionKind::Exercise});require(book.board().view().steps==0,"Opening an exercise performed a step");
    if(section.exercise==BookExerciseKind::Systems){
      require(book.systems().dispatch({SystemActionKind::Reveal}).accepted,"Cannot open practice explanation");
      require(book.systems().dispatch({SystemActionKind::RowOperation,0,0,0,{BoardActionKind::Step},true}).accepted,"Cannot reduce practice");
      act(book,{BookActionKind::Read});require(book.board().view().steps==0,"Practice changed exploration");
      act(book,{BookActionKind::Exercise});require(book.board().view().steps==1,"Practice work was lost");
      act(book,{BookActionKind::Read});continue;
    }
    require(book.board().dispatch({BoardActionKind::Step}).accepted,"Exercise cannot step");
    act(book,{BookActionKind::Read});act(book,{BookActionKind::Contents});act(book,{BookActionKind::Index});act(book,{BookActionKind::Resume});
    require(book.view().section==i&&book.view().scroll==123.5*(i+1)&&book.view().mode==BookMode::Reading,"Reading trail lost position");
    require(book.board().view().steps==1,"Reading navigation lost exercise work");
    // The introductory section and RREF section intentionally share card 004.
    if(i==0){act(book,{BookActionKind::OpenSection,1});require(book.board().view().steps==1,"Same card has duplicate state");require(book.board().dispatch({BoardActionKind::Reset}).accepted,"Reset failed");}
  }
  require(cards.size()==7&&terms.size()>=33,"Chapter references/index coverage");
  for(unsigned i=0;i<sections.size();++i){act(book,{BookActionKind::OpenSection,i});if(!book.hasBoard()){require(book.objectLesson().snapshot(true).feedback==MathFeedback::None,"Navigation checked object practice");continue;}require(book.board().view().steps==(sections[i].card==0?0u:1u),"Cross-section exercise work not retained");require(!book.board().view().checked,"Reading created check evidence");}
  reject(book,{BookActionKind::Next});reject(book,{BookActionKind::OpenSection,static_cast<unsigned>(sections.size())});reject(book,{BookActionKind::RememberScroll,0,10});reject(book,{BookActionKind::RememberScroll,6,-1});reject(book,{BookActionKind::RememberScroll,6,std::numeric_limits<double>::infinity()});reject(book,{BookActionKind::SetTextScale,0,2.01});reject(book,{static_cast<BookActionKind>(999)});
  act(book,{BookActionKind::OpenSection,0});reject(book,{BookActionKind::Previous});act(book,{BookActionKind::Contents});reject(book,{BookActionKind::Exercise});reject(book,{BookActionKind::RememberScroll,0,1});
  act(book,{BookActionKind::OpenSection,5});act(book,{BookActionKind::SetTextScale,0,1.3});const auto bookmark=book.bookmark();
  Textbook restored;require(restored.restoreBookmark(bookmark).accepted,"Bookmark did not round trip");require(restored.bookmark()==bookmark&&restored.view().page==BookPage::Contents,"Bookmark changed on restore");act(restored,{BookActionKind::Resume});require(restored.view().section==5&&restored.view().scroll==741,"Continue reading did not restore place");
  for(unsigned i=0;i<sections.size();++i){act(restored,{BookActionKind::OpenSection,i});require(restored.hasBoard()?(restored.board().view().steps==0&&!restored.board().view().checked):(restored.objectLesson().snapshot(true).feedback==MathFeedback::None),"Bookmark restored fabricated exercise results");require(restored.view().scroll==123.5*(i+1),"Section scroll was not saved");}
  const std::string eight=R"(paths-textbook 1
matrix.permutations
1.3
matrix.entries 123.5
matrix.rref 247
matrix.solutions 370.5
matrix.partial-pivoting 494
matrix.lu 617.5
matrix.permutations 741
matrix.bands 864.5
matrix.schur 988
)";
  Textbook eightMigrated;require(eightMigrated.restoreBookmark(eight).accepted,"Eight-section bookmark failed to migrate");
  act(eightMigrated,{BookActionKind::OpenSection,8});require(eightMigrated.view().scroll==0,"New object section inherited an old scroll");
  auto legacy=eight;const auto newLine=legacy.find("matrix.solutions ");const auto newEnd=legacy.find('\n',newLine);legacy.erase(newLine,newEnd-newLine+1);
  Textbook migrated;require(migrated.restoreBookmark(legacy).accepted,"Seven-section bookmark failed to migrate");
  act(migrated,{BookActionKind::Resume});require(std::string_view(matrixChapter()[migrated.view().section].id)=="matrix.permutations"&&migrated.view().scroll==741,"Migration changed stable reading position");
  act(migrated,{BookActionKind::OpenSection,2});require(migrated.view().scroll==0,"New section inherited another section's scroll");
  auto partial=eight;partial.erase(partial.find("matrix.rref 247\n"),16);badBookmark(book,partial);
  badBookmark(book,eight+"matrix.determinant-volume 100\n"+"matrix.determinant-volume 100\n");
  auto impossible=eight;impossible.replace(impossible.find("matrix.permutations"),19,"matrix.determinant-volume");badBookmark(book,impossible);
  // Reading and bookmark restoration must preserve an already checked result.
  require(book.board().dispatch({BoardActionKind::Step}).accepted,"Could not finish pivoted example");
  require(book.board().dispatch({BoardActionKind::Check}).accepted&&book.board().view().passed,"Could not check pivoted example");
  act(book,{BookActionKind::OpenSection,2});act(book,{BookActionKind::OpenSection,5});
  require(book.board().view().passed&&book.board().view().steps==2,"Navigation lost a checked exercise result");
  badBookmark(book,"");badBookmark(book,std::string(4097,'x'));badBookmark(book,bookmark+"unexpected");
  auto corrupt=bookmark;corrupt.replace(corrupt.find("paths-textbook 1"),16,"paths-textbook 9\n");badBookmark(book,corrupt);
  corrupt=bookmark;corrupt.replace(corrupt.find("matrix.permutations"),19,"unknown.section");badBookmark(book,corrupt);
  corrupt=bookmark;corrupt.replace(corrupt.find("123.5"),5,"nan");badBookmark(book,corrupt);
  corrupt=bookmark;const auto offset=corrupt.rfind("matrix.rref");corrupt.replace(offset,11,"matrix.entries");badBookmark(book,corrupt);
  require(book.restoreBookmark(bookmark).accepted,"Valid restore failed after invalid input");act(book,{BookActionKind::Resume});require(book.board().view().steps==2&&book.board().view().passed,"Restoring reading position erased existing board work");
  // Stable references resolve independently of display order and never reveal answers.
  Textbook reading;require(reading.lessonView().empty(),"Contents exposed a lesson");
  act(reading,{BookActionKind::OpenSection,1});
  std::set<std::string> blockIds,numbers;unsigned exercises=0,helpCount=0;
  const auto lesson=matrixChapter()[1].lesson;
  for(const auto& b:lesson){
    require(blockIds.insert(b.id).second,"Duplicate block reference");
    if(!b.number.empty())require(numbers.insert(b.number).second,"Duplicate block number");
    require(!b.body.empty(),"A lesson block has no statement");
    if(b.kind==BookBlockKind::Exercise){
      ++exercises;require(!b.help[1].empty()&&!b.help[2].empty()&&!b.help[3].empty(),"Practice lacks separate hint, answer or solution");
    }
    for(const auto& h:b.help)if(!h.empty())++helpCount;
  }
  require(exercises==3&&helpCount==11,"Expected practice and worked reasoning missing");
  for(const auto& b:lesson)for(const auto& ref:b.references)require(blockIds.contains(ref.target),"Broken internal reference");
  for(const auto& term:matrixChapter()[1].terms)require(blockIds.contains(term.blockId),"Index points outside its definitions");
  for(const auto& b:reading.lessonView())for(const auto& h:b.help)
    require(!h.open&&h.passages.empty(),"Unrequested solution or proof exposed");
  const auto unopened=reading.bookmark();
  const auto hint=BookAction{BookActionKind::ToggleHelp,1,0,"rref.practice.reduce",BookHelp::Hint};
  act(reading,hint);
  auto selected=block(reading,"rref.practice.reduce");
  require(selected.help[1].open&&!selected.help[1].passages.empty(),"Hint did not open");
  require(!selected.help[2].open&&selected.help[2].passages.empty()&&!selected.help[3].open&&selected.help[3].passages.empty(),"Hint disclosed an answer or solution");
  require(!block(reading,"rref.practice.recognize").help[1].open&&!block(reading,"rref.preservation").help[0].open,"Help leaked across blocks");
  require(reading.bookmark()==unopened&&!reading.board().view().working&&!reading.board().view().checked,"Reading help fabricated work or persistence");
  act(reading,{BookActionKind::ToggleHelp,1,0,"rref.practice.reduce",BookHelp::Answer});
  selected=block(reading,"rref.practice.reduce");require(selected.help[1].open&&selected.help[2].open&&!selected.help[3].open,"Answer did not stay separate from solution");
  act(reading,hint);require(!block(reading,"rref.practice.reduce").help[1].open&&block(reading,"rref.practice.reduce").help[2].open,"Closing hint closed the answer");
  rejectedHelp(reading,{BookActionKind::ToggleHelp,1,0,"rref.reduced",BookHelp::Solution});
  rejectedHelp(reading,{BookActionKind::ToggleHelp,1,0,"rref.practice.reduce",BookHelp::Proof});
  rejectedHelp(reading,{BookActionKind::ToggleHelp,1,0,"rref.practice.reduce",static_cast<BookHelp>(255)});
  rejectedHelp(reading,{BookActionKind::ToggleHelp,1,0,"missing.block",BookHelp::Hint});
  rejectedHelp(reading,{BookActionKind::OpenBlock,1,0,"missing.block"});
  rejectedHelp(reading,{BookActionKind::OpenBlock,99,0,"rref.reduced"});
  act(reading,{BookActionKind::Exercise});require(reading.lessonView().empty(),"Exercise mode publishes lesson disclosures");
  rejectedHelp(reading,hint);act(reading,{BookActionKind::Contents});rejectedHelp(reading,hint);
  act(reading,{BookActionKind::OpenSection,2});rejectedHelp(reading,hint);
  act(reading,{BookActionKind::OpenBlock,1,0,"rref.reduced"});
  require(reading.view().anchor=="rref.reduced"&&reading.view().mode==BookMode::Reading,"Reference did not open its definition");
  const auto revision=reading.view().anchorRevision;
  act(reading,{BookActionKind::OpenBlock,1,0,"rref.reduced"});require(reading.view().anchorRevision==revision+1,"Repeated reference cannot be followed again");
  require(block(reading,"rref.practice.reduce").help[2].open&&!block(reading,"rref.example").help[3].open,"Navigation lost help choices or opened unrelated working");
  act(reading,{BookActionKind::OpenSection,1});require(reading.view().anchor.empty(),"Ordinary reading retained a forced reference jump");
  // Even a checked source exercise is unchanged by every supported disclosure.
  while(!reading.board().view().complete)require(reading.board().dispatch({BoardActionKind::Step}).accepted,"Could not reduce the printed matrix");
  require(reading.board().dispatch({BoardActionKind::Check}).accepted&&reading.board().view().passed,"Could not check the printed reduction");
  const auto matrixBefore=reading.board().view();
  for(const auto& b:lesson)for(unsigned h=0;h<b.help.size();++h){
    if(b.help[h].empty())continue;
    const auto wasOpen=block(reading,b.id).help[h].open;
    const BookAction toggle{BookActionKind::ToggleHelp,1,0,b.id,static_cast<BookHelp>(h)};
    act(reading,toggle);auto shown=block(reading,b.id).help[h];
    require(shown.open!=wasOpen&&shown.passages.empty()==wasOpen,"Disclosure does not redact on close");
    act(reading,toggle);
  }
  const auto matrixAfter=reading.board().view();
  require(matrixBefore.current.values==matrixAfter.current.values&&matrixBefore.steps==matrixAfter.steps&&matrixAfter.checked&&matrixAfter.passed&&matrixBefore.residual==matrixAfter.residual,"Disclosure rewrote a checked result");
  act(reading,{BookActionKind::SetTextScale,0,2});
  Textbook fresh;require(fresh.restoreBookmark(reading.bookmark()).accepted&&fresh.view().textScale==2,"200 percent reading size did not round trip");
  act(fresh,{BookActionKind::Resume});
  for(const auto& b:fresh.lessonView())for(const auto& h:b.help)require(!h.open&&h.passages.empty(),"Reading bookmark restored hidden solutions");
  require(!fresh.board().view().working&&!fresh.board().view().checked,"Reading bookmark restored exercise evidence");
  Temporary temp;const auto path=temp.path/"reader"/"position.txt";
  require(readTextbookBookmark(path,restored).accepted,"Missing bookmark should be ordinary startup");
  require(writeTextbookBookmark(path,book).accepted,"Could not write bookmark");require(bytes(path)==book.bookmark(),"Bookmark file bytes changed");
  require(readTextbookBookmark(path,restored).accepted&&restored.bookmark()==book.bookmark(),"File restore failed");
  const auto saved=bytes(path);require(!writeTextbookBookmark(path/"not-a-directory",book).accepted,"Invalid directory accepted");require(bytes(path)==saved,"Failed save damaged previous bookmark");
  {std::ofstream out(path);out<<"bad bookmark";}
  const auto unchanged=restored.bookmark();require(!readTextbookBookmark(path,restored).accepted&&restored.bookmark()==unchanged,"Bad file changed reader");require(bytes(path)=="bad bookmark","Reading rewrote malformed file");
  std::printf("textbook: %u assertions passed; registry-driven sections, six source boards plus independent systems/object practice, index terms, navigation boundaries, read/exercise separation, stable block references, independent redacted help, 200 percent text and bookmark round trips.\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"textbook test: %s\n",e.what());return 1;}}
