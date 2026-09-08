#include "content/CorpusPractice.hpp"
#include "content/QuestionContentIO.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace paths {
namespace {
namespace fm=iggy3d::first_move;
using Json=nlohmann::json;
constexpr std::size_t maxBytes=8*1024*1024,maxQuestions=1024,maxCommands=200000;
void require(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
std::string read(const std::filesystem::path& path) {
  std::ifstream file(path,std::ios::binary);require(bool(file),"Cannot read "+path.string());
  std::string text(maxBytes+1,'\0');file.read(text.data(),text.size());
  require(!file.bad(),"Cannot read "+path.string());text.resize(file.gcount());
  require(text.size()<=maxBytes,"Starter file exceeds 8 MiB");return text;
}
std::string folded(std::string_view value) {
  std::string text(value);for(auto& c:text)if(c>='A' && c<='Z')c+='a'-'A';return text;
}
constexpr std::array actions{
  fm::LayeredQuestionCommandKind::OpenQuestion,fm::LayeredQuestionCommandKind::SubmitOption,
  fm::LayeredQuestionCommandKind::Continue,fm::LayeredQuestionCommandKind::RestartQuestion};
fm::LayeredQuestionSession session(const CorpusStarter& q) {
  return fm::LayeredQuestionSession({q.question},fm::QuestionInteraction::ArcadeCollect,0,true);
}
}
std::vector<CorpusStarter> loadCorpusStarters(const std::filesystem::path& path,const MathCorpus& corpus) {
  const auto root=Json::parse(read(path));require(root.at("schema_version")==1,"Unknown starter schema");
  const auto& rows=root.at("questions");require(rows.is_array() && !rows.empty() && rows.size()<=maxQuestions,"Invalid starter count");
  std::set<std::string> ids;std::vector<CorpusStarter> result;
  for(const auto& row:rows) {
    CorpusStarter q;q.id=row.at("id");q.title=row.at("title");q.level=row.at("level");
    require(!q.id.empty() && q.id.size()<=80 && ids.insert(q.id).second,"Duplicate or invalid starter ID");
    require(!q.title.empty() && q.title.size()<=240,"Invalid starter title");
    require(q.level=="subject" || q.level=="chapter" || q.level=="subcategory","Invalid starter level");
    const auto subjectId=row.at("subject").get<std::string>();
    const auto subject=std::find_if(corpus.subjects.begin(),corpus.subjects.end(),[&](const auto& s){return s.id==subjectId;});
    require(subject!=corpus.subjects.end(),"Unknown starter subject");q.subject=subject-corpus.subjects.begin();
    if(row.contains("topic")) {
      const auto topicId=row.at("topic").get<std::string>();
      const auto topic=std::find_if(corpus.topics.begin(),corpus.topics.end(),[&](const auto& t){return t.id==topicId;});
      require(topic!=corpus.topics.end() && topic->subject==q.subject,"Unknown or mismatched starter chapter");q.topic=topic-corpus.topics.begin();
    }
    require(q.topic.has_value()==(q.level!="subject"),"Starter level and chapter disagree");
    q.stamp=row.at("question").dump();q.question=parseQuestionContent(q.stamp,path);
    require(q.question.id==q.id,"Starter and question identities disagree");
    const auto valid=fm::validateQuestion(q.question,fm::QuestionInteraction::ArcadeCollect);
    require(valid.valid(),"Invalid starter question "+q.id);
    result.push_back(std::move(q));
  }
  return result;
}
CorpusPractice::CorpusPractice(std::vector<CorpusStarter> questions):questions_(std::move(questions)),attempts_(questions_.size()) {}
std::vector<std::size_t> CorpusPractice::find(std::optional<std::size_t> subject,std::optional<std::size_t> topic,std::string_view query) const {
  const auto needle=folded(query);std::vector<std::size_t> matches;
  for(std::size_t i=0;i<questions_.size();++i) {
    const auto& q=questions_[i];
    if(subject && q.subject!=*subject)continue;
    if(topic && q.topic!=topic)continue;
    if(!needle.empty() && folded(q.title+" "+q.question.description+" "+q.id).find(needle)==std::string::npos)continue;
    matches.push_back(i);
  }
  return matches;
}
void CorpusPractice::open(std::size_t index) {
  if(index>=questions_.size())return;
  if(!attempts_[index]) {
    attempts_[index]=session(questions_[index]);
    (void)attempts_[index]->dispatch({fm::LayeredQuestionCommandKind::OpenQuestion});
  }
  dirty_=dirty_ || selected_!=index;selected_=index;
}
fm::LayeredQuestionSession* CorpusPractice::active(){return selected_?&*attempts_[*selected_]:nullptr;}
const fm::LayeredQuestionSession* CorpusPractice::attempt(std::size_t index) const {
  return index<attempts_.size() && attempts_[index]?&*attempts_[index]:nullptr;
}
bool CorpusPractice::dispatch(const fm::LayeredQuestionCommand& command) {
  if(!active() || std::find(actions.begin(),actions.end(),command.kind)==actions.end() || command.questionIndex || command.archiveUnfinished)return false;
  const auto result=active()->dispatch(command);dirty_=dirty_ || result.changed;return result.accepted;
}
void CorpusPractice::loadProgress(const std::filesystem::path& path) {
  progressPath_=path;if(path.empty()){message_="Progress is kept for this session.";return;}
  try {
    if(!std::filesystem::exists(path)){message_="Starter progress saves automatically.";return;}
    disk_=read(path);const auto root=Json::parse(*disk_);
    require(root.at("format")=="paths_corpus_starters" && root.at("version")==1,"Unknown starter save format");
    const auto& runs=root.at("runs");require(runs.is_array() && runs.size()<=questions_.size(),"Invalid starter save count");
    auto restored=attempts_;std::set<std::string> ids;std::size_t total=0;
    for(const auto& run:runs) {
      const std::string id=run.at("id");require(ids.insert(id).second,"Repeated saved starter "+id);
      const auto q=std::find_if(questions_.begin(),questions_.end(),[&](const auto& value){return value.id==id;});
      require(q!=questions_.end(),"Saved question missing: "+id);
      require(run.at("question").dump()==q->stamp,"Saved mathematics changed: "+id);
      const auto& commands=run.at("commands");require(commands.is_array(),"Invalid starter commands");total+=commands.size();
      require(total<=maxCommands,"Starter history exceeds limit");auto replay=session(*q);
      for(const auto& c:commands) {
        require(c.is_array() && c.size()==2 && c[0].is_number_unsigned() && c[1].is_number_unsigned(),"Invalid starter command");
        require(c[0].get<std::uint64_t>()<actions.size() && c[1].get<std::uint64_t>()<=UINT32_MAX,"Invalid starter command value");
        fm::LayeredQuestionCommand command{actions[c[0].get<std::size_t>()]};command.option.value=c[1].get<std::uint32_t>();
        require(command.kind==fm::LayeredQuestionCommandKind::SubmitOption || command.option.value==0,"Unexpected starter option");
        require(replay.dispatch(command).accepted,"Saved starter command rejected: "+id);
      }
      require(replay.currentRun().phase!=fm::LayeredQuestionPhase::Grid,"Saved starter was never opened");
      restored[q-questions_.begin()]=std::move(replay);
    }
    std::optional<std::size_t> selected;
    if(!root.at("selected").is_null()) {
      const auto selectedId=root.at("selected").get<std::string>();
      for(std::size_t i=0;i<questions_.size();++i)if(questions_[i].id==selectedId)selected=i;
      require(selected && restored[*selected],"Unknown saved starter selection");
    }
    attempts_=std::move(restored);selected_=selected;dirty_=false;message_="Starter progress restored.";
  } catch(const std::exception& error) {blocked_=true;message_=std::string(error.what())+". Original save retained.";}
}
void CorpusPractice::saveProgress() {
  if(progressPath_.empty() || blocked_ || !dirty_)return;
  try {
    Json root{{"format","paths_corpus_starters"},{"version",1},{"selected",selected_?Json(questions_[*selected_].id):Json(nullptr)},{"runs",Json::array()}};
    std::size_t total=0;
    for(std::size_t i=0;i<attempts_.size();++i)if(attempts_[i]) {
      Json commands=Json::array();
      for(const auto& c:attempts_[i]->journal()) {
        const auto action=std::find(actions.begin(),actions.end(),c.kind);require(action!=actions.end(),"Unsupported starter command");
        commands.push_back({static_cast<std::size_t>(action-actions.begin()),c.option.value});
      }
      total+=commands.size();require(total<=maxCommands,"Starter history exceeds limit");
      root["runs"].push_back({{"id",questions_[i].id},{"question",Json::parse(questions_[i].stamp)},{"commands",std::move(commands)}});
    }
    const auto text=root.dump(2)+"\n";require(text.size()<=maxBytes,"Starter save exceeds 8 MiB");
    require(std::filesystem::exists(progressPath_)?disk_ && read(progressPath_)==disk_:!disk_,"Starter save changed outside this window");
    auto temporary=progressPath_;temporary+=".tmp";
    require(!std::filesystem::exists(temporary) && !std::filesystem::is_symlink(temporary),"Starter temporary save already exists");
    std::ofstream file(temporary,std::ios::binary);file<<text;file.close();require(bool(file),"Cannot write starter save");
    std::filesystem::rename(temporary,progressPath_);disk_=text;dirty_=false;message_="Starter progress saved.";
  } catch(const std::exception& error) {blocked_=true;message_=std::string(error.what())+". Previous save retained.";}
}
}
