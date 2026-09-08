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
void same(const BookView& a,const BookView& b){require(a.page==b.page&&a.mode==b.mode&&a.section==b.section&&a.scroll==b.scroll&&a.textScale==b.textScale,"Rejected navigation changed reading state");}
void reject(Textbook& book,BookAction action){const auto before=book.view();const auto bookmark=book.bookmark();const auto steps=book.board().view().steps;require(!book.dispatch(action).accepted,"Invalid action accepted");same(before,book.view());require(book.bookmark()==bookmark&&book.board().view().steps==steps,"Rejected action changed bookmark or exercise");}
void badBookmark(Textbook& book,const std::string& data){const auto before=book.view();const auto bookmark=book.bookmark();require(!book.restoreBookmark(data).accepted,"Malformed bookmark accepted");same(before,book.view());require(book.bookmark()==bookmark,"Malformed bookmark partially committed");}
std::string bytes(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
struct Temporary {
  std::filesystem::path path=std::filesystem::temp_directory_path()/("paths-textbook-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary(){if(!std::filesystem::create_directory(path))throw std::runtime_error("Cannot create unique test directory");}
  ~Temporary(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(){try{
  Textbook book;const auto& sections=matrixChapter();require(sections.size()==7&&textbookParts().size()==7,"Book outline size");
  std::set<std::string> ids,terms;std::set<unsigned> cards;
  const std::array<unsigned,7> order{4,4,31,18,44,1,59};
  for(unsigned i=0;i<sections.size();++i){const auto& section=sections[i];require(ids.insert(section.id).second,"Duplicate stable section id");require(section.card==order[i],"Teaching sequence changed");cards.insert(section.card);
    require(std::string(section.purpose).size()>20&&std::string(section.figurePrompt).size()>20&&std::string(section.exercisePrompt).size()>20&&std::string(section.reference).size()>20,"Incomplete section");
    for(const auto paragraph:section.explanation)require(std::string(paragraph).size()>40,"Missing explanation");
    for(const auto step:section.exampleSteps)require(std::string(step).size()>30,"Missing worked example step");
    for(const auto& term:section.terms){require(terms.insert(term.name).second,"Duplicate index term");require(std::string(term.definition).size()>20,"Missing definition");}
    act(book,{BookActionKind::OpenSection,i});require(book.board().view().card==section.card,"Wrong exercise binding");require(!book.board().view().working&&!book.board().view().checked,"Reading marked an exercise as worked or checked");
    act(book,{BookActionKind::RememberScroll,i,123.5*(i+1)});act(book,{BookActionKind::Exercise});require(book.board().view().steps==0,"Opening an exercise performed a step");
    require(book.board().dispatch({BoardActionKind::Step}).accepted,"Exercise cannot step");
    act(book,{BookActionKind::Read});act(book,{BookActionKind::Contents});act(book,{BookActionKind::Index});act(book,{BookActionKind::Resume});
    require(book.view().section==i&&book.view().scroll==123.5*(i+1)&&book.view().mode==BookMode::Reading,"Reading trail lost position");
    require(book.board().view().steps==1,"Reading navigation lost exercise work");
    // The introductory section and RREF section intentionally share card 004.
    if(i==0){act(book,{BookActionKind::OpenSection,1});require(book.board().view().steps==1,"Same card has duplicate state");require(book.board().dispatch({BoardActionKind::Reset}).accepted,"Reset failed");}
  }
  require(cards.size()==6&&terms.size()==21,"Chapter references/index coverage");
  for(unsigned i=0;i<sections.size();++i){act(book,{BookActionKind::OpenSection,i});require(book.board().view().steps==1,"Cross-section exercise work not retained");require(!book.board().view().checked,"Reading created check evidence");}
  reject(book,{BookActionKind::Next});reject(book,{BookActionKind::OpenSection,7});reject(book,{BookActionKind::RememberScroll,0,10});reject(book,{BookActionKind::RememberScroll,6,-1});reject(book,{BookActionKind::RememberScroll,6,std::numeric_limits<double>::infinity()});reject(book,{BookActionKind::SetTextScale,0,2});reject(book,{static_cast<BookActionKind>(999)});
  act(book,{BookActionKind::OpenSection,0});reject(book,{BookActionKind::Previous});act(book,{BookActionKind::Contents});reject(book,{BookActionKind::Exercise});reject(book,{BookActionKind::RememberScroll,0,1});
  act(book,{BookActionKind::OpenSection,4});act(book,{BookActionKind::SetTextScale,0,1.3});const auto bookmark=book.bookmark();
  Textbook restored;require(restored.restoreBookmark(bookmark).accepted,"Bookmark did not round trip");require(restored.bookmark()==bookmark&&restored.view().page==BookPage::Contents,"Bookmark changed on restore");act(restored,{BookActionKind::Resume});require(restored.view().section==4&&restored.view().scroll==617.5,"Continue reading did not restore place");
  for(unsigned i=0;i<sections.size();++i){act(restored,{BookActionKind::OpenSection,i});require(restored.board().view().steps==0&&!restored.board().view().checked,"Bookmark restored fabricated exercise results");require(restored.view().scroll==123.5*(i+1),"Section scroll was not saved");}
  // Reading and bookmark restoration must preserve an already checked result.
  require(book.board().dispatch({BoardActionKind::Step}).accepted,"Could not finish pivoted example");
  require(book.board().dispatch({BoardActionKind::Check}).accepted&&book.board().view().passed,"Could not check pivoted example");
  act(book,{BookActionKind::OpenSection,2});act(book,{BookActionKind::OpenSection,4});
  require(book.board().view().passed&&book.board().view().steps==2,"Navigation lost a checked exercise result");
  badBookmark(book,"");badBookmark(book,std::string(4097,'x'));badBookmark(book,bookmark+"unexpected");
  auto corrupt=bookmark;corrupt.replace(corrupt.find("paths-textbook 1"),16,"paths-textbook 9\n");badBookmark(book,corrupt);
  corrupt=bookmark;corrupt.replace(corrupt.find("matrix.permutations"),19,"unknown.section");badBookmark(book,corrupt);
  corrupt=bookmark;corrupt.replace(corrupt.find("123.5"),5,"nan");badBookmark(book,corrupt);
  corrupt=bookmark;const auto offset=corrupt.rfind("matrix.rref");corrupt.replace(offset,11,"matrix.entries");badBookmark(book,corrupt);
  require(book.restoreBookmark(bookmark).accepted,"Valid restore failed after invalid input");act(book,{BookActionKind::Resume});require(book.board().view().steps==2&&book.board().view().passed,"Restoring reading position erased existing board work");
  Temporary temp;const auto path=temp.path/"reader"/"position.txt";
  require(readTextbookBookmark(path,restored).accepted,"Missing bookmark should be ordinary startup");
  require(writeTextbookBookmark(path,book).accepted,"Could not write bookmark");require(bytes(path)==book.bookmark(),"Bookmark file bytes changed");
  require(readTextbookBookmark(path,restored).accepted&&restored.bookmark()==book.bookmark(),"File restore failed");
  const auto saved=bytes(path);require(!writeTextbookBookmark(path/"not-a-directory",book).accepted,"Invalid directory accepted");require(bytes(path)==saved,"Failed save damaged previous bookmark");
  {std::ofstream out(path);out<<"bad bookmark";}
  const auto unchanged=restored.bookmark();require(!readTextbookBookmark(path,restored).accepted&&restored.bookmark()==unchanged,"Bad file changed reader");require(bytes(path)=="bad bookmark","Reading rewrote malformed file");
  std::printf("textbook: %u assertions passed; seven sections, six retained boards, 21 index terms, navigation boundaries, read/exercise separation and bookmark round trips.\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"textbook test: %s\n",e.what());return 1;}}
