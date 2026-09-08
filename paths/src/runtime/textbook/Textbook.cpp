#include "Textbook.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace paths {
namespace {
unsigned boardIndex(unsigned section) {
  const auto card=matrixChapter()[section].card;
  const auto& cards=matrixCards();
  return static_cast<unsigned>(std::find_if(cards.begin(),cards.end(),[&](auto c){return c.id==card;})-cards.begin());
}
}
Textbook::Textbook() {
  for(unsigned i=0;i<boards_.size();++i)
    static_cast<void>(boards_[i].dispatch({BoardActionKind::Select,matrixCards()[i].id}));
}
BoardResult Textbook::dispatch(BookAction action) {
  switch(action.kind) {
    case BookActionKind::OpenSection:
      if(action.section>=matrixChapter().size())return {false,"Unknown section."};
      section_=action.section;page_=BookPage::Section;mode_=BookMode::Reading;break;
    case BookActionKind::Next:
      if(section_+1>=matrixChapter().size())return {false,"This is the last section."};
      ++section_;page_=BookPage::Section;mode_=BookMode::Reading;break;
    case BookActionKind::Previous:
      if(section_==0)return {false,"This is the first section."};
      --section_;page_=BookPage::Section;mode_=BookMode::Reading;break;
    case BookActionKind::Contents:page_=BookPage::Contents;break;
    case BookActionKind::Index:page_=BookPage::Index;break;
    case BookActionKind::Resume:page_=BookPage::Section;mode_=BookMode::Reading;break;
    case BookActionKind::Read:
    case BookActionKind::Exercise:
      if(page_!=BookPage::Section)return {false,"Open a section first."};
      mode_=action.kind==BookActionKind::Read?BookMode::Reading:BookMode::Exercise;break;
    case BookActionKind::RememberScroll:
      if(page_!=BookPage::Section||mode_!=BookMode::Reading||action.section!=section_||!std::isfinite(action.value)||action.value<0||action.value>100000)
        return {false,"Invalid reading position."};
      scrolls_[section_]=action.value;break;
    case BookActionKind::SetTextScale:
      if(!std::isfinite(action.value)||action.value<.9||action.value>1.5)return {false,"Text size must be between 90% and 150%."};
      textScale_=action.value;break;
    default:return {false,"Unknown textbook action."};
  }
  return {true,{}};
}
BookView Textbook::view()const {return {page_,mode_,section_,scrolls_[section_],textScale_};}
unsigned Textbook::exerciseIndex()const{return boardIndex(section_);}
MatrixBoard& Textbook::board(){return boards_.at(boardIndex(section_));}
const MatrixBoard& Textbook::board()const{return boards_.at(boardIndex(section_));}
std::string Textbook::bookmark()const {
  std::ostringstream out;out<<"paths-textbook 1\n"<<matrixChapter()[section_].id<<'\n'<<std::setprecision(17)<<textScale_<<'\n';
  for(unsigned i=0;i<scrolls_.size();++i)out<<matrixChapter()[i].id<<' '<<scrolls_[i]<<'\n';
  return out.str();
}
BoardResult Textbook::restoreBookmark(std::string_view data) {
  if(data.size()>4096)return {false,"Reading bookmark is too large."};
  std::istringstream in{std::string(data)};std::string magic,id;unsigned version=0;double scale=0;
  if(!(in>>magic>>version>>id>>scale)||magic!="paths-textbook"||version!=1||!std::isfinite(scale)||scale<.9||scale>1.5)
    return {false,"Unsupported reading bookmark."};
  const auto& sections=matrixChapter();auto section=std::find_if(sections.begin(),sections.end(),[&](auto s){return id==s.id;});
  if(section==sections.end())return {false,"Unknown bookmarked section."};
  std::array<double,7> scrolls{};std::array<bool,7> seen{};
  for(unsigned i=0;i<scrolls.size();++i) {
    double offset=0;if(!(in>>id>>offset)||!std::isfinite(offset)||offset<0||offset>100000)return {false,"Invalid bookmarked position."};
    auto found=std::find_if(sections.begin(),sections.end(),[&](auto s){return id==s.id;});
    if(found==sections.end())return {false,"Unknown bookmarked position."};
    const auto index=static_cast<unsigned>(found-sections.begin());if(seen[index])return {false,"Duplicate bookmarked position."};
    seen[index]=true;scrolls[index]=offset;
  }
  std::string trailing;if(in>>trailing)return {false,"Unexpected bookmark content."};
  section_=static_cast<unsigned>(section-sections.begin());textScale_=scale;scrolls_=scrolls;
  // Restore reading only: exercise states and results are deliberately untouched.
  page_=BookPage::Contents;mode_=BookMode::Reading;return {true,{}};
}
BoardResult readTextbookBookmark(const std::filesystem::path& path,Textbook& book) {
  std::error_code error;
  if(!std::filesystem::exists(path,error))return error?BoardResult{false,error.message()}:BoardResult{true,{}};
  const auto size=std::filesystem::file_size(path,error);
  if(error||size>4096)return {false,"Could not read the bounded reading bookmark."};
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
