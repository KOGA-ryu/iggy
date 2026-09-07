#include "content/StudyProgressIO.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <limits>
#include <random>

namespace paths {
namespace {
namespace fm=iggy3d::first_move;
using Json=nlohmann::json;
constexpr std::uintmax_t maxBytes=32*1024*1024;
constexpr std::size_t maxCommands=200000;
constexpr std::array actions{
  std::pair{"open",fm::LayeredQuestionCommandKind::OpenQuestion},
  std::pair{"answer",fm::LayeredQuestionCommandKind::SubmitOption},
  std::pair{"hint",fm::LayeredQuestionCommandKind::RequestHint},
  std::pair{"next_move",fm::LayeredQuestionCommandKind::RevealNextMove},
  std::pair{"do_step",fm::LayeredQuestionCommandKind::ApplyPreparedStep},
  std::pair{"continue",fm::LayeredQuestionCommandKind::Continue},
  std::pair{"restart",fm::LayeredQuestionCommandKind::RestartQuestion},
  std::pair{"move",fm::LayeredQuestionCommandKind::MathematicalMove}
};
constexpr std::array<std::string_view,6> scalarOperations{"expand","simplify","add","subtract","multiply","divide"};
void require(bool valid,const char* reason) {if(!valid)throw std::runtime_error(reason);}
std::uint64_t number(const Json& j,std::uint64_t maximum=std::numeric_limits<std::uint32_t>::max()) {
  require(j.is_number_unsigned() && j.get<std::uint64_t>()<=maximum,"invalid saved number");
  return j.get<std::uint64_t>();
}
std::string string(const Json& j,std::size_t limit=160) {
  require(j.is_string() && j.get_ref<const std::string&>().size()<=limit,"invalid saved text");
  return j.get<std::string>();
}
const Json& array(const Json& j,std::size_t limit=sorterEquationCount) {
  require(j.is_array() && j.size()<=limit,"saved list exceeds its limit");return j;
}
std::string_view operationKey(fm::MathOperation op) {
  const auto index=static_cast<std::size_t>(op);
  if(index<scalarOperations.size())return scalarOperations[index];
  for(const auto& row:fm::rowOperations)if(row.operation==op)return row.key;
  throw std::runtime_error("unknown saved mathematical operation");
}
fm::MathOperation operation(const Json& j) {
  const auto key=string(j,32);
  for(std::size_t i=0;i<scalarOperations.size();++i)if(scalarOperations[i]==key)return static_cast<fm::MathOperation>(i);
  for(const auto& row:fm::rowOperations)if(row.key==key)return row.operation;
  throw std::runtime_error("unknown saved mathematical operation");
}
Json commandJson(const fm::LayeredQuestionCommand& command) {
  const auto found=std::find_if(actions.begin(),actions.end(),[&](const auto& a){return a.second==command.kind;});
  require(found!=actions.end(),"unsupported practice command");
  Json j{{"action",found->first}};
  switch(command.kind) {
  case fm::LayeredQuestionCommandKind::SubmitOption:j["option"]=command.option.value;break;
  case fm::LayeredQuestionCommandKind::RestartQuestion:
    require(!command.questionIndex,"saved practice cannot switch its question");j["archive_unfinished"]=command.archiveUnfinished;break;
  case fm::LayeredQuestionCommandKind::MathematicalMove: {
    const auto& m=command.math;
    j["kind"]=m.kind==fm::MathMoveKind::Undo?"undo":"submit";j["operation"]=operationKey(m.operation);
    j["operand"]=m.operand;j["entry"]=m.entry;j["run"]=m.runNumber;j["revision"]=m.revision;break;
  }
  default:break;
  }
  return j;
}
fm::LayeredQuestionCommand readCommand(const Json& j,const SavedStudyQuestion& question) {
  const auto key=string(j.at("action"),32);
  const auto found=std::find_if(actions.begin(),actions.end(),[&](const auto& a){return a.first==key;});
  require(found!=actions.end(),"unknown practice command");
  fm::LayeredQuestionCommand command{found->second};
  switch(command.kind) {
  case fm::LayeredQuestionCommandKind::SubmitOption:command.option.value=number(j.at("option"));break;
  case fm::LayeredQuestionCommandKind::RestartQuestion:
    require(j.at("archive_unfinished").is_boolean(),"invalid saved restart");command.archiveUnfinished=j.at("archive_unfinished").get<bool>();break;
  case fm::LayeredQuestionCommandKind::MathematicalMove: {
    const auto kind=string(j.at("kind"),8);require(kind=="undo" || kind=="submit","unknown saved move kind");
    command.math={kind=="undo"?fm::MathMoveKind::Undo:fm::MathMoveKind::Submit,operation(j.at("operation")),
      string(j.at("operand"),48),string(j.at("entry")),static_cast<std::uint32_t>(number(j.at("run"))),
      number(j.at("revision"),std::numeric_limits<std::uint64_t>::max()),question.questionId,question.contentVersion};break;
  }
  default:break;
  }
  return command;
}
// Pin the teaching material, not its filesystem location. This also catches an
// edited answer key or working chain whose author forgot to change its version.
Json catalogStamp(const EquationSorterSession& session) {
  Json result=Json::array();
  for(const auto& card:session.content())if(card.solution) {
    const auto& q=*card.solution;
    Json item{{"card",card.id},{"id",q.id},{"version",q.version},{"equation",q.equation},
      {"math_moves",q.supportsMathMoves},{"model",q.mathModel},{"steps",Json::array()},{"working",Json::array()}};
    for(const auto& s:q.steps) {
      Json step{{"id",s.id.value},{"prompt",s.prompt},{"accepted",s.acceptedOptions},{"purpose",s.semantics.purpose},
        {"completion",s.semantics.completion},{"before",s.semantics.before.value},{"after",s.semantics.after.value},
        {"hint",s.hint},{"next_move",s.nextMove},{"explanation",s.explanation},{"options",Json::array()}};
      for(const auto& o:s.options)step["options"].push_back({o.id.value,o.label});
      item["steps"].push_back(std::move(step));
    }
    for(const auto& w:q.workingStates)item["working"].push_back({{"id",w.id.value},{"display",w.display},
      {"graph_stage",w.graphStage?Json(*w.graphStage):Json(nullptr)}});
    if(q.lineGraph) {
      const auto& g=*q.lineGraph;
      item["graph"]={g.rise,g.run,g.intercept,g.xMin,g.xMax,g.yMin,g.yMax};
      if(g.second)item["second_line"]={g.second->rise,g.second->run,g.second->intercept};
    }
    result.push_back(std::move(item));
  }
  return result;
}
Json encode(const EquationSorterSession& session) {
  const auto p=session.studyProgress();
  Json j{{"format","paths_practice"},{"version",1},{"catalog",catalogStamp(session)},
    {"mode",p.mode},{"random_count",p.randomCount},{"position",p.position},{"selected",p.selected},{"queue",p.queue},
    {"titles",Json::array()},{"questions",Json::array()}};
  for(const auto& t:p.titles)j["titles"].push_back({{"subject",t.subject},{"chapter",t.chapter},{"type",t.type}});
  for(const auto& q:p.questions) {
    require(q.commands.size()<=maxCommands,"practice history exceeds the save limit");
    Json question{{"card",q.equation},{"id",q.questionId},{"version",q.contentVersion},{"commands",Json::array()}};
    for(const auto& c:q.commands)question["commands"].push_back(commandJson(c));
    j["questions"].push_back(std::move(question));
  }
  return j;
}
StudyProgress decode(const Json& j,const EquationSorterSession& session) {
  require(j.at("format")=="paths_practice" && number(j.at("version"))==1,"unsupported practice save version");
  require(j.at("catalog")==catalogStamp(session),"question content has changed; the saved practice was kept");
  StudyProgress p;
  p.mode=static_cast<StudyMode>(number(j.at("mode"),2));p.randomCount=number(j.at("random_count"),100);p.position=number(j.at("position"),99);
  for(const auto& t:array(j.at("titles")))p.titles.push_back({static_cast<SorterSubject>(number(t.at("subject"),4)),string(t.at("chapter"),80),string(t.at("type"),80)});
  for(const auto& id:array(j.at("selected")))p.selected.push_back(number(id));
  for(const auto& id:array(j.at("queue")))p.queue.push_back(number(id));
  for(const auto& q:array(j.at("questions"))) {
    SavedStudyQuestion question{static_cast<SorterEquationId>(number(q.at("card"))),string(q.at("id")),static_cast<std::uint32_t>(number(q.at("version"))),{}};
    for(const auto& c:array(q.at("commands"),maxCommands))question.commands.push_back(readCommand(c,question));
    p.questions.push_back(std::move(question));
  }
  return p;
}
std::optional<std::string> read(const std::filesystem::path& path) {
  if(!std::filesystem::exists(path))return std::nullopt;
  require(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path)<=maxBytes,"practice file is not readable or is too large");
  std::ifstream input(path,std::ios::binary);require(bool(input),"cannot read practice file");
  std::string text{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
  require(!input.bad() && text.size()<=maxBytes,"cannot finish reading practice file");return text;
}
}
StudyProgressFile::StudyProgressFile(std::filesystem::path path):path_(std::move(path)) {}
void StudyProgressFile::load(EquationSorterSession& session) {
  if(path_.empty())return;
  try {
    disk_=read(path_);
    if(disk_)session.restoreStudyProgress(decode(Json::parse(*disk_),session));
    checkedRevision_=session.progressRevision();
    message_=disk_?(session.view().study.canResume?"Saved practice ready. Resume set.":"Saved selection ready."):"Practice saves automatically.";
  } catch(const std::exception& e) {
    blocked_=failed_=true;message_="Saved practice could not open. Saving paused: "+std::string(e.what());
  }
}
void StudyProgressFile::save(const EquationSorterSession& session) {
  if(path_.empty() || blocked_ || checkedRevision_==session.progressRevision())return;
  checkedRevision_=session.progressRevision();
  std::filesystem::path temporary;
  bool ownsTemporary=false;
  try {
    const auto text=encode(session).dump()+"\n";
    require(text.size()<=maxBytes,"practice history exceeds the save limit");
    if(disk_==text)return;
    require(read(path_)==disk_,"practice file changed in another game; its saved work was kept");
    if(!path_.parent_path().empty())std::filesystem::create_directories(path_.parent_path());
    temporary=path_;temporary+="."+std::to_string(std::random_device{}())+".tmp";
    auto* output=std::fopen(temporary.string().c_str(),"wbx");
    require(output!=nullptr,"cannot create temporary practice file");ownsTemporary=true;
    const bool written=std::fwrite(text.data(),1,text.size(),output)==text.size();
    const bool closed=std::fclose(output)==0;require(written && closed,"cannot write practice file");
    std::filesystem::rename(temporary,path_); // Same directory: replace only after a complete write.
    disk_=text;failed_=false;message_="Saved on this device.";
  } catch(const std::exception& e) {
    if(ownsTemporary) {std::error_code ignored;std::filesystem::remove(temporary,ignored);}
    failed_=true;message_="Practice could not save: "+std::string(e.what());
  }
}
} // namespace paths
