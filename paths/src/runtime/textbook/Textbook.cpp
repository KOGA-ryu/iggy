#include "Textbook.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <set>
#include <system_error>
#include <stdexcept>

namespace paths {
namespace {
unsigned boardIndex(const BookSection& definition) {
  if(definition.exercise==BookExerciseKind::Systems)return static_cast<unsigned>(matrixCards().size());
  if(definition.exercise!=BookExerciseKind::MatrixBoard)throw std::logic_error("This lesson has no matrix-board binding");
  const auto card=definition.card;
  const auto& cards=matrixCards();
  return static_cast<unsigned>(std::find_if(cards.begin(),cards.end(),[&](auto c){return c.id==card;})-cards.begin());
}
}
Textbook::Textbook(std::span<const BookSection> sections):sections_(sections.empty()?matrixChapter():sections),nativeCatalogue_(sections.empty()),scrolls_(sections_.size()),helpMasks_(sections_.size()),objects_(sections_.size()) {
  for(unsigned i=0;i<boards_.size();++i)
    static_cast<void>(boards_[i].dispatch({BoardActionKind::Select,matrixCards()[i].id}));
  for(unsigned i=0;i<helpMasks_.size();++i){
    const auto& section=sections_[i];helpMasks_[i].resize(section.lesson.size());
    if(section.exercise==BookExerciseKind::Object){
      if(!section.object||section.figure.kind!=BookFigureKind::Object)throw std::logic_error("Object lesson binding is incomplete");
      objects_[i]=std::make_unique<ObjectLesson>(*section.object);
    }
  }
}
void Textbook::retainReading(const Textbook& previous) {
  textScale_=previous.textScale_;
  for(unsigned i=0;i<sections_.size();++i)for(unsigned j=0;j<previous.sections_.size();++j)
    if(std::string_view(sections_[i].id)==previous.sections_[j].id){
      scrolls_[i]=previous.scrolls_[j];
      if(j==previous.section_){section_=i;page_=previous.page_;mode_=*sections_[i].exercisePrompt?previous.mode_:BookMode::Reading;}
      break;
    }
  // New source revisions reopen reading disclosures explicitly.
  anchor_={};
}
BoardResult Textbook::dispatch(BookAction action) {
  switch(action.kind) {
    case BookActionKind::OpenSection:
      if(action.section>=sections_.size())return {false,"Unknown section."};
      section_=action.section;page_=BookPage::Section;mode_=BookMode::Reading;anchor_={};break;
    case BookActionKind::Next:
      if(section_+1>=sections_.size())return {false,"This is the last section."};
      ++section_;page_=BookPage::Section;mode_=BookMode::Reading;anchor_={};break;
    case BookActionKind::Previous:
      if(section_==0)return {false,"This is the first section."};
      --section_;page_=BookPage::Section;mode_=BookMode::Reading;anchor_={};break;
    case BookActionKind::Contents:page_=BookPage::Contents;anchor_={};break;
    case BookActionKind::Index:page_=BookPage::Index;anchor_={};break;
    case BookActionKind::Resume:page_=BookPage::Section;mode_=BookMode::Reading;anchor_={};break;
    case BookActionKind::Read:
    case BookActionKind::Exercise:
      if(page_!=BookPage::Section)return {false,"Open a section first."};
      if(action.kind==BookActionKind::Exercise && !*sections_[section_].exercisePrompt)return {false,"This section has no attached exercise."};
      mode_=action.kind==BookActionKind::Read?BookMode::Reading:BookMode::Exercise;anchor_={};break;
    case BookActionKind::RememberScroll:
      if(page_!=BookPage::Section||mode_!=BookMode::Reading||action.section!=section_||!std::isfinite(action.value)||action.value<0||action.value>100000)
        return {false,"Invalid reading position."};
      scrolls_[section_]=action.value;break;
    case BookActionKind::SetTextScale:
      if(!std::isfinite(action.value)||action.value<.9||action.value>2)return {false,"Text size must be between 90% and 200%."};
      textScale_=action.value;break;
    case BookActionKind::OpenBlock:
    case BookActionKind::ToggleHelp: {
      if(action.section>=sections_.size())return {false,"Unknown section."};
      const auto blocks=sections_[action.section].lesson;
      const auto found=std::find_if(blocks.begin(),blocks.end(),[&](const auto& b){return action.target==b.id;});
      if(found==blocks.end())return {false,"Unknown section reference."};
      if(action.kind==BookActionKind::OpenBlock){
        section_=action.section;page_=BookPage::Section;mode_=BookMode::Reading;
        anchor_=found->id;++anchorRevision_;
      }else {
        const auto help=static_cast<unsigned>(action.help);
        if(page_!=BookPage::Section||mode_!=BookMode::Reading||section_!=action.section)
          return {false,"Open this reading section before revealing its help."};
        if(help>=static_cast<unsigned>(BookHelp::Count)||found->help[help].empty())
          return {false,"This block has no such help."};
        helpMasks_[section_][static_cast<std::size_t>(found-blocks.begin())]^=static_cast<std::uint8_t>(1u<<help);
      }
      break;
    }
    default:return {false,"Unknown textbook action."};
  }
  return {true,{}};
}
BookView Textbook::view()const {return {page_,mode_,section_,scrolls_[section_],textScale_,anchor_,anchorRevision_};}
std::vector<BookBlockView> Textbook::lessonView()const {
  if(page_!=BookPage::Section||mode_!=BookMode::Reading)return {};
  return bookLessonView(sections_[section_].lesson,helpMasks_[section_]);
}
std::vector<BookBlockView> bookLessonView(std::span<const BookBlock> blocks,std::span<const std::uint8_t> helpMasks) {
  std::vector<BookBlockView> result;
  result.reserve(blocks.size());
  for(std::size_t i=0;i<blocks.size();++i){
    const auto& b=blocks[i];BookBlockView v{b.id.c_str(),b.kind,b.number.c_str(),b.title.c_str(),b.body,{},b.references};
    for(unsigned h=0;h<b.help.size();++h){
      const bool open=!b.help[h].empty() && i<helpMasks.size() && (helpMasks[i]&(1u<<h))!=0;
      v.help[h]={!b.help[h].empty(),open,open?std::span<const BookPassage>(b.help[h]):std::span<const BookPassage>{}};
    }
    result.push_back(v);
  }
  return result;
}
unsigned Textbook::exerciseIndex()const{return boardIndex(sections_[section_]);}
MatrixBoard& Textbook::board(){return exerciseKind()==BookExerciseKind::Systems?systems_.board(mode_==BookMode::Exercise):boards_.at(boardIndex(sections_[section_]));}
const MatrixBoard& Textbook::board()const{return exerciseKind()==BookExerciseKind::Systems?systems_.board(mode_==BookMode::Exercise):boards_.at(boardIndex(sections_[section_]));}
ObjectLesson& Textbook::objectLesson(){if(!objects_[section_])throw std::logic_error("This section has no object lesson");return *objects_[section_];}
const ObjectLesson& Textbook::objectLesson()const{if(!objects_[section_])throw std::logic_error("This section has no object lesson");return *objects_[section_];}
std::string Textbook::bookmark()const {
  std::ostringstream out;out<<"paths-textbook "<<(nativeCatalogue_?1:2)<<'\n'<<sections_[section_].id<<'\n'<<std::setprecision(17)<<textScale_<<'\n';
  if(!nativeCatalogue_)out<<sections_.size()<<'\n';
  for(unsigned i=0;i<scrolls_.size();++i)out<<sections_[i].id<<' '<<scrolls_[i]<<'\n';
  return out.str();
}
BoardResult Textbook::restoreBookmark(std::string_view data) {
  if(data.size()>262144)return {false,"Reading bookmark is too large."};
  std::istringstream in{std::string(data)};std::string magic,id;unsigned version=0;double scale=0;
  if(!(in>>magic>>version>>id>>scale)||magic!="paths-textbook"||version!=(nativeCatalogue_?1U:2U)||!std::isfinite(scale)||scale<.9||scale>2)
    return {false,"Unsupported reading bookmark."};
  const auto& sections=sections_;auto section=std::find_if(sections.begin(),sections.end(),[&](auto s){return id==s.id;});
  if(section==sections.end())return {false,"Unknown bookmarked section."};
  std::vector<double> scrolls(sections.size());std::vector<bool> seen(sections.size());
  unsigned generation=0,count=0,expected=0;std::set<std::string> identities;
  if(!nativeCatalogue_ && (!(in>>expected)||!expected||expected>10000))return {false,"Invalid reading bookmark catalogue size."};
  while(in>>id) {
    double offset=0;if(!(in>>offset)||!std::isfinite(offset)||offset<0||offset>100000)return {false,"Invalid bookmarked position."};
    auto found=std::find_if(sections.begin(),sections.end(),[&](auto s){return id==s.id;});
    if(!identities.insert(id).second)return {false,"Duplicate bookmarked position."};
    ++count;
    if(found==sections.end()){if(nativeCatalogue_)return {false,"Unknown bookmarked position."};else continue;}
    const auto index=static_cast<unsigned>(found-sections.begin());if(seen[index])return {false,"Duplicate bookmarked position."};
    seen[index]=true;scrolls[index]=offset;generation=std::max(generation,found->bookmarkGeneration);
  }
  // Accept complete older catalogues by stable IDs, never an arbitrary partial save.
  if(!generation||!seen[static_cast<std::size_t>(section-sections.begin())])return {false,"Missing bookmarked section."};
  if(nativeCatalogue_){for(unsigned i=0;i<seen.size();++i)if(!seen[i]&&sections[i].bookmarkGeneration<=generation)return {false,"Missing bookmarked position."};}
  else if(count!=expected)return {false,"Incomplete reading bookmark catalogue."};
  section_=static_cast<unsigned>(section-sections.begin());textScale_=scale;scrolls_=scrolls;
  // Restore reading only: exercise states and results are deliberately untouched.
  page_=BookPage::Contents;mode_=BookMode::Reading;anchor_={};return {true,{}};
}
BoardResult readTextbookBookmark(const std::filesystem::path& path,Textbook& book) {
  std::error_code error;
  if(!std::filesystem::exists(path,error))return error?BoardResult{false,error.message()}:BoardResult{true,{}};
  const auto size=std::filesystem::file_size(path,error);
  if(error||size>262144)return {false,"Could not read the bounded reading bookmark."};
  std::ifstream in(path,std::ios::binary);std::string data(static_cast<std::size_t>(size),'\0');
  if(!in.read(data.data(),static_cast<std::streamsize>(data.size())))return {false,"Could not read the reading bookmark."};
  return book.restoreBookmark(data);
}
BoardResult writeTextbookBookmark(const std::filesystem::path& path,const Textbook& book) {
  if(path.empty())return {false,"No reading bookmark path."};
  std::error_code error;
  if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path(),error);
  if(error)return {false,error.message()};
  static std::atomic<unsigned long long> sequence{0};
  auto temporary=path;temporary+=".tmp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(sequence++);
  {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);const auto data=book.bookmark();
   if(!out.write(data.data(),static_cast<std::streamsize>(data.size()))||!out.flush()){out.close();std::filesystem::remove(temporary,error);return {false,"Could not save the reading bookmark."};}}
  std::filesystem::rename(temporary,path,error);
  if(error){const auto reason=error.message();std::filesystem::remove(temporary,error);return {false,reason};}
  return {true,{}};
}
} // namespace paths
