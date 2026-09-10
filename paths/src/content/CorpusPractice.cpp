#include "content/CorpusPractice.hpp"
#include "content/QuestionContentIO.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <stdexcept>
#include <sys/file.h>
#include <system_error>
#include <unistd.h>

namespace paths {
namespace {
namespace fm=iggy3d::first_move;
using Json=nlohmann::json;
constexpr std::size_t maxBytes=8*1024*1024,maxQuestions=1024,maxCommands=200000;
void require(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
struct ProgressWriteLock {
  int descriptor;
  explicit ProgressWriteLock(const std::filesystem::path& path) {
    descriptor=::open((path.string()+".lock").c_str(),O_RDWR|O_CREAT|O_CLOEXEC|O_NOFOLLOW,0600);
    if(descriptor<0)throw std::system_error(errno,std::generic_category(),"Cannot open starter save lock");
    if(::flock(descriptor,LOCK_EX|LOCK_NB)!=0) {
      const int error=errno;::close(descriptor);
      throw std::system_error(error,std::generic_category(),"Starter save lock unavailable");
    }
  }
  ~ProgressWriteLock(){::close(descriptor);}
  ProgressWriteLock(const ProgressWriteLock&)=delete;
  ProgressWriteLock& operator=(const ProgressWriteLock&)=delete;
};
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
  fm::LayeredQuestionCommandKind::Continue,fm::LayeredQuestionCommandKind::RestartQuestion,fm::LayeredQuestionCommandKind::Support};
constexpr std::array supportActions{"level","draft","choice","blank","work","help","undo","reference"};
fm::LayeredQuestionSession session(const CorpusStarter& q) {
  return fm::LayeredQuestionSession({q.question},q.question.support?fm::QuestionInteraction::Supported:fm::QuestionInteraction::ArcadeCollect,0,true);
}
}
std::vector<CorpusStarter> loadCorpusStarters(const std::filesystem::path& path,const MathCorpus& corpus) {
  return parseCorpusStarters(read(path),corpus,path);
}
std::vector<CorpusStarter> parseCorpusStarters(std::string_view text,const MathCorpus& corpus,const std::filesystem::path& path) {
  require(text.size()<=maxBytes,"Starter file exceeds 8 MiB");
  const auto root=Json::parse(text);require(root.at("schema_version")==1,"Unknown starter schema");
  const auto& rows=root.at("questions");require(rows.is_array() && !rows.empty() && rows.size()<=maxQuestions,"Invalid starter count");
  std::set<std::string> ids;std::vector<CorpusStarter> result;
  for(const auto& row:rows) {
    CorpusStarter q;q.id=row.at("id");q.title=row.at("title");q.level=row.at("level");
    require(!q.id.empty() && q.id.size()<=80 && ids.insert(q.id).second,"Duplicate or invalid starter ID");
    require(!q.title.empty() && q.title.size()<=240,"Invalid starter title");
    require(q.level=="subject" || q.level=="chapter" || q.level=="subcategory" || q.level=="practice","Invalid question level");
    const auto subjectId=row.at("subject").get<std::string>();
    const auto subject=std::find_if(corpus.subjects.begin(),corpus.subjects.end(),[&](const auto& s){return s.id==subjectId;});
    require(subject!=corpus.subjects.end(),"Unknown starter subject");q.subject=subject-corpus.subjects.begin();
    if(row.contains("topic")) {
      const auto topicId=row.at("topic").get<std::string>();
      const auto topic=std::find_if(corpus.topics.begin(),corpus.topics.end(),[&](const auto& t){return t.id==topicId;});
      require(topic!=corpus.topics.end() && topic->subject==q.subject,"Unknown or mismatched starter chapter");q.topic=topic-corpus.topics.begin();
    }
    require(q.topic.has_value()==(q.level!="subject"),"Starter level and chapter disagree");
    if(row.contains("reading_refs")) {
      const auto& refs=row.at("reading_refs");require(refs.is_array() && refs.size()<=8,"Invalid reading reference count");
      std::set<std::string> unique;
      for(const auto& ref:refs) {
        const auto id=ref.get<std::string>();require(!id.empty() && id.size()<=80 && unique.insert(id).second,"Invalid or repeated reading reference");q.readingRefs.push_back(id);
      }
    }
    q.stamp=row.at("question").dump();q.question=parseQuestionContent(q.stamp,path);
    require(q.question.id==q.id,"Starter and question identities disagree");
    const auto valid=fm::validateQuestion(q.question,fm::QuestionInteraction::ArcadeCollect);
    require(valid.valid(),"Invalid starter question "+q.id);
    result.push_back(std::move(q));
  }
  return result;
}
CorpusPractice::CorpusPractice(std::vector<CorpusStarter> questions):questions_(std::move(questions)),attempts_(questions_.size()) {
  require(questions_.size()<=maxQuestions,"Combined question collection exceeds its limit");
  std::set<std::string> ids;
  for(const auto& q:questions_)require(ids.insert(q.id).second,"Repeated question identity across collections: "+q.id);
}
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
    const auto level=active() && active()->currentRun().support?active()->currentRun().support->level:fm::SupportLevel::Learn;
    attempts_[index]=session(questions_[index]);
    if(auto view=attempts_[index]->supportView()) {
      fm::LayeredQuestionCommand command{fm::LayeredQuestionCommandKind::Support};command.support=view->command;
      command.support.value=static_cast<std::uint32_t>(level);(void)attempts_[index]->dispatch(command);
    }
    (void)attempts_[index]->dispatch({fm::LayeredQuestionCommandKind::OpenQuestion});
  }
  dirty_=dirty_ || selected_!=index;selected_=index;
}
fm::LayeredQuestionSession* CorpusPractice::active(){return selected_?&*attempts_[*selected_]:nullptr;}
const fm::LayeredQuestionSession* CorpusPractice::attempt(std::size_t index) const {
  return index<attempts_.size() && attempts_[index]?&*attempts_[index]:nullptr;
}
bool CorpusPractice::dispatch(const fm::LayeredQuestionCommand& command) {
  if(!active() || std::find(actions.begin(),actions.end(),command.kind)==actions.end() || command.questionIndex)return false;
  if(command.archiveUnfinished && !(active()->currentRun().support && command.kind==fm::LayeredQuestionCommandKind::RestartQuestion))return false;
  const auto result=active()->dispatch(command);dirty_=dirty_ || result.changed;return result.accepted;
}
void CorpusPractice::replacePreview(std::vector<CorpusStarter> questions) {
  require(progressPath_.empty(),"Live preview cannot replace a session with saved progress");
  CorpusPractice next(std::move(questions));next.previewRevisions_=previewRevisions_;
  for(std::size_t i=0;i<attempts_.size();++i)if(attempts_[i])
    next.previewRevisions_.insert_or_assign({questions_[i].id,questions_[i].stamp},*attempts_[i]);
  for(std::size_t i=0;i<next.questions_.size();++i) {
    const auto& q=next.questions_[i];const auto found=next.previewRevisions_.find({q.id,q.stamp});
    if(found!=next.previewRevisions_.end()){next.attempts_[i]=std::move(found->second);next.previewRevisions_.erase(found);}
    if(selected_ && q.id==questions_[*selected_].id)next.selected_=i;
  }
  require(next.previewRevisions_.size()<=32,"Preview retains at most 32 older attempted revisions; restart preview to clear its session history");
  bool fresh=false;
  if(next.selected_) {
    const auto selected=*next.selected_;fresh=!next.attempts_[selected];
    const auto level=active() && active()->currentRun().support?active()->currentRun().support->level:fm::SupportLevel::Learn;
    if(fresh)next.selected_.reset();next.open(selected);
    if(fresh)if(const auto view=next.active()->supportView()) {
      fm::LayeredQuestionCommand command{fm::LayeredQuestionCommandKind::Support};command.support=view->command;
      command.support.value=static_cast<std::uint32_t>(level);(void)next.dispatch(command);
    }
  }
  next.message_=fresh?"Question changed: fresh preview. Revert its source to recover earlier working.":"Live preview: working stays in this session; personal saves are untouched.";
  *this=std::move(next);
}
void CorpusPractice::loadProgress(const std::filesystem::path& path) {
  progressPath_=path;if(path.empty()){message_="Progress is kept for this session.";return;}
  try {
    require(!std::filesystem::is_symlink(path),"Starter save is a symbolic link");
    if(!std::filesystem::exists(path)){message_="Starter progress saves automatically.";return;}
    disk_=read(path);const auto root=Json::parse(*disk_);
    require(root.at("format")=="paths_corpus_starters" && (root.at("version")==1 || root.at("version")==2),"Unknown starter save format");
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
        fm::LayeredQuestionCommand command;
        if(c.is_object()) {
          require(root.at("version")==2 && c.size()==7 && c.at("kind")=="support" && c.at("schema")==1 && q->question.support.has_value(),"Invalid support command");
          const auto key=c.at("action").get<std::string>();const auto action=std::find(supportActions.begin(),supportActions.end(),key);
          require(action!=supportActions.end(),"Unknown saved support action");
          for(const auto* field:{"value","run","revision"})require(c.at(field).is_number_unsigned(),"Invalid saved support integer");
          require(c.at("value").get<std::uint64_t>()<=UINT32_MAX && c.at("run").get<std::uint64_t>()<=UINT32_MAX,"Saved support integer overflow");
          command.kind=fm::LayeredQuestionCommandKind::Support;
          command.support={static_cast<fm::SupportAction>(action-supportActions.begin()),c.at("value").get<std::uint32_t>(),c.at("text").get<std::string>(),q->id,q->question.version,c.at("run").get<std::uint32_t>(),c.at("revision").get<std::uint64_t>()};
        } else {
          require(c.is_array() && (c.size()==2 || (root.at("version")==2 && c.size()==3)) && c[0].is_number_unsigned() && c[1].is_number_unsigned(),"Invalid starter command");
          require(c[0].get<std::uint64_t>()<4 && c[1].get<std::uint64_t>()<=UINT32_MAX,"Invalid starter command value");
          command.kind=actions[c[0].get<std::size_t>()];command.option.value=c[1].get<std::uint32_t>();
          require(command.kind==fm::LayeredQuestionCommandKind::SubmitOption || command.option.value==0,"Unexpected starter option");
          if(c.size()==3) {
            require(command.kind==fm::LayeredQuestionCommandKind::RestartQuestion && q->question.support && c[2].is_boolean(),"Invalid saved restart");
            command.archiveUnfinished=c[2].get<bool>();
          }
        }
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
  } catch(const std::exception& error) {loadBlocked_=true;message_=std::string(error.what())+". Original save retained.";}
}
void CorpusPractice::saveProgress(bool closing) {
  if(progressPath_.empty() || loadBlocked_ || !dirty_ || (!closing && std::chrono::steady_clock::now()<retryAfter_))return;
  std::filesystem::path temporary;bool ownsTemporary=false;
  try {
    Json root{{"format","paths_corpus_starters"},{"version",2},{"selected",selected_?Json(questions_[*selected_].id):Json(nullptr)},{"runs",Json::array()}};
    std::size_t total=0;
    for(std::size_t i=0;i<attempts_.size();++i)if(attempts_[i]) {
      Json commands=Json::array();
      for(const auto& c:attempts_[i]->journal()) {
        const auto action=std::find(actions.begin(),actions.end(),c.kind);require(action!=actions.end(),"Unsupported starter command");
        if(c.kind==fm::LayeredQuestionCommandKind::Support) {
          const auto& s=c.support;
          commands.push_back({{"kind","support"},{"action",supportActions[static_cast<std::size_t>(s.action)]},{"value",s.value},{"text",s.text},{"run",s.runNumber},{"revision",s.revision},{"schema",1}});
        } else {
          Json entry={static_cast<std::size_t>(action-actions.begin()),c.option.value};if(c.archiveUnfinished)entry.push_back(true);commands.push_back(std::move(entry));
        }
      }
      total+=commands.size();require(total<=maxCommands,"Starter history exceeds limit");
      root["runs"].push_back({{"id",questions_[i].id},{"question",Json::parse(questions_[i].stamp)},{"commands",std::move(commands)}});
    }
    auto text=root.dump(2)+"\n";require(text.size()<=maxBytes,"Starter save exceeds 8 MiB");
    const ProgressWriteLock lock(progressPath_);
    const auto unchanged=[&] {
      require(!std::filesystem::is_symlink(progressPath_),"Starter save became a symbolic link");
      require(std::filesystem::exists(progressPath_)?disk_ && read(progressPath_)==disk_:!disk_,"Starter save changed outside this window");
    };
    unchanged();temporary=progressPath_;temporary+="."+std::to_string(std::random_device{}())+".tmp";
    auto* file=std::fopen(temporary.string().c_str(),"wbx");
    if(!file)throw std::system_error(errno,std::generic_category(),"Cannot create starter temporary save");
    ownsTemporary=true;
    const bool written=std::fwrite(text.data(),1,text.size(),file)==text.size();const int closed=std::fclose(file);
    require(written && closed==0,"Cannot finish writing starter save");unchanged();
    std::filesystem::rename(temporary,progressPath_);ownsTemporary=false;
    disk_=std::move(text);dirty_=false;retryAfter_={};message_="Starter progress saved.";
  } catch(const std::exception& error) {
    if(ownsTemporary){std::error_code ignored;std::filesystem::remove(temporary,ignored);}
    retryAfter_=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    message_=std::string(error.what())+". Previous save retained. Saving will retry automatically.";
  }
}
}
