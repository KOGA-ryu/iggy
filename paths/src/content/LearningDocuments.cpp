#include "LearningDocuments.hpp"
#include "QuestionContentIO.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace paths {
namespace {
namespace fm=iggy3d::first_move;
using Json=nlohmann::json;
using Path=std::filesystem::path;
constexpr std::size_t maxFile=128*1024,maxExpanded=2*1024*1024,maxDocuments=128;
enum class Template { Lesson, BookLesson, Choices, Linear, Matrix };
constexpr std::array templates{std::pair{"lesson.v1",Template::Lesson},std::pair{"lesson.v2",Template::BookLesson},std::pair{"choices.v1",Template::Choices},std::pair{"linear.v1",Template::Linear},std::pair{"matrix.v1",Template::Matrix}};
constexpr std::array blockKinds{std::pair{"introduction",BookBlockKind::Introduction},std::pair{"definition",BookBlockKind::Definition},
  std::pair{"proposition",BookBlockKind::Proposition},std::pair{"example",BookBlockKind::Example},std::pair{"figure",BookBlockKind::Figure},
  std::pair{"exercise",BookBlockKind::Exercise},std::pair{"summary",BookBlockKind::Summary}};
constexpr std::array helpKinds{std::pair{"proof",BookHelp::Proof},std::pair{"hint",BookHelp::Hint},std::pair{"answer",BookHelp::Answer},std::pair{"solution",BookHelp::Solution}};
enum class Command { Paths,Subject,Chapter,Lesson,Question,Template,Version,Goal,Given,Domain,Step,Choice,TextChoice,Answer,After,Why,Wrong,Definitions,Teaching,Read,Figure,Parameter,Caption,Practice,Text,End,Operation,Block,Prose,Display,Help,Body,Reference,EndBlock,Hint };
constexpr std::array commands{
  std::pair{"paths",Command::Paths},std::pair{"subject",Command::Subject},std::pair{"chapter",Command::Chapter},
  std::pair{"lesson",Command::Lesson},std::pair{"question",Command::Question},std::pair{"template",Command::Template},
  std::pair{"version",Command::Version},std::pair{"goal",Command::Goal},std::pair{"given",Command::Given},std::pair{"domain",Command::Domain},
  std::pair{"step",Command::Step},std::pair{"choice",Command::Choice},std::pair{"textchoice",Command::TextChoice},
  std::pair{"answer",Command::Answer},std::pair{"after",Command::After},std::pair{"why",Command::Why},std::pair{"wrong",Command::Wrong},
  std::pair{"definitions",Command::Definitions},std::pair{"teaching",Command::Teaching},std::pair{"hint",Command::Hint},std::pair{"read",Command::Read},
  std::pair{"figure",Command::Figure},std::pair{"parameter",Command::Parameter},std::pair{"caption",Command::Caption},
  std::pair{"practice",Command::Practice},std::pair{"text",Command::Text},std::pair{"end",Command::End},std::pair{"operation",Command::Operation},
  std::pair{"block",Command::Block},std::pair{"prose",Command::Prose},std::pair{"display",Command::Display},std::pair{"help",Command::Help},
  std::pair{"body",Command::Body},std::pair{"reference",Command::Reference},std::pair{"endblock",Command::EndBlock}};
void require(bool ok,const std::string& why){if(!ok)throw std::invalid_argument(why);}
struct DocumentError : std::invalid_argument {
  std::string file,field,code;std::size_t line;
  DocumentError(std::string f,std::size_t n,std::string key,std::string c,const std::string& why)
    :std::invalid_argument(f+":"+std::to_string(n)+": "+why),file(std::move(f)),field(std::move(key)),code(std::move(c)),line(n){}
};
std::string digest(std::string_view bytes) {
  unsigned char hash[EVP_MAX_MD_SIZE];unsigned length=0;
  require(EVP_Digest(bytes.data(),bytes.size(),hash,&length,EVP_sha256(),nullptr)==1 && length==32,"Cannot hash content");
  constexpr char hex[]="0123456789abcdef";std::string result;result.reserve(64);
  for(unsigned i=0;i<length;++i){result+=hex[hash[i]>>4];result+=hex[hash[i]&15];}return result;
}
void realPath(const Path& path) {
  Path part;for(const auto& component:std::filesystem::absolute(path)) {
    part/=component;require(!std::filesystem::is_symlink(part),"Symlink paths are not supported: "+part.string());
  }
}
std::string readBytes(const Path& path,std::size_t limit) {
  realPath(path);require(std::filesystem::is_regular_file(path),"Missing regular file: "+path.string());
  const auto size=std::filesystem::file_size(path);require(size<=limit,"File exceeds its byte limit: "+path.string());
  std::ifstream in(path,std::ios::binary);require(bool(in),"Cannot open file: "+path.string());std::string bytes(size,'\0');
  in.read(bytes.data(),static_cast<std::streamsize>(size));
  require(in.gcount()==static_cast<std::streamsize>(size) && in.peek()==std::char_traits<char>::eof() && !in.bad(),"File changed or cannot be read: "+path.string());return bytes;
}
Path relativePath(const std::string& name) {
  const Path path(name);require(!name.empty() && name.size()<=240 && !path.is_absolute() && name.find('\\')==name.npos && name.find('\0')==name.npos,"Invalid inventory path");
  for(const auto& part:path)require(!part.empty() && part!="." && part!="..","Inventory paths cannot contain empty or dot segments");
  require(path.generic_string()==name && name.find("//")==name.npos,"Inventory paths must use relative POSIX spelling");return path;
}
Json location(const std::string& file,std::size_t line){return {{"file",file},{"line",line}};}
bool hasFormat(const Json& value,std::string_view name) {
  return value.at("format")==name && value.at("format_version").is_number_unsigned() && value.at("format_version")==1;
}
Json catalogue(const MathCorpus& corpus,const std::vector<CorpusStarter>& questions) {
  Json result{{"subjects",Json::array()},{"chapters",Json::array()},{"readings",Json::array()},{"questions",Json::array()}};
  for(const auto& s:corpus.subjects)result["subjects"].push_back({{"id",s.id},{"title",s.title}});
  for(const auto& t:corpus.topics)result["chapters"].push_back({{"id",t.id},{"title",t.title},{"parent",corpus.subjects[t.subject].id}});
  for(const auto& e:corpus.entries) {
    Json blocks=Json::array();
    const auto passages=[](const auto& content){Json out=Json::array();for(const auto& p:content)out.push_back({p.kind,p.text,p.number});return out;};
    for(const auto& b:e.lesson) {
      Json help=Json::array(),refs=Json::array();for(const auto& h:b.help)help.push_back(passages(h));for(const auto& r:b.references)refs.push_back({r.label,r.target});
      blocks.push_back({b.id,b.kind,b.number,b.title,passages(b.body),help,refs});
    }
    result["readings"].push_back({{"id",e.id},{"title",e.title},{"parent",corpus.topics[e.topic].id},{"body_sha256",digest(e.lesson.empty()?e.body:blocks.dump())}});
  }
  for(const auto& q:questions)result["questions"].push_back({{"id",q.id},{"stamp_sha256",digest(q.stamp)}});
  for(auto& list:result)std::sort(list.begin(),list.end(),[](const auto& a,const auto& b){return a.at("id")<b.at("id");});return result;
}
Json emptyReport() {
  return {{"format","paths_document_report"},{"format_version",1},{"accepted",false},{"diagnostics",Json::array()},
    {"entry_documents",Json::array()},{"files",Json::array()},{"entities",Json::array()}};
}
DocumentImport failedReport(const std::exception& error,const std::string& code="document.input") {
  DocumentImport result;result.accepted=false;result.message="Documents not loaded: "+std::string(error.what());auto detail=emptyReport();
  Json diagnostic{{"code",code},{"file",""},{"line",0},{"field",""},{"message",error.what()}};
  if(const auto* e=dynamic_cast<const DocumentError*>(&error)){diagnostic["code"]=e->code;diagnostic["file"]=e->file;diagnostic["line"]=e->line;diagnostic["field"]=e->field;}
  detail["diagnostics"].push_back(diagnostic);detail["message"]=result.message;result.reportJson=detail.dump(2)+"\n";return result;
}
std::string trim(std::string_view value) {
  const auto a=value.find_first_not_of(" \t\r\n");if(a==value.npos)return {};
  return std::string(value.substr(a,value.find_last_not_of(" \t\r\n")-a+1));
}
std::pair<std::string,std::string> split(std::string_view value,char delimiter) {
  const auto at=value.find(delimiter);require(at!=value.npos,"Expected two values separated by '"+std::string(1,delimiter)+"'");
  auto a=trim(value.substr(0,at)),b=trim(value.substr(at+1));require(!a.empty() && !b.empty(),"Both values are required");return {a,b};
}
std::uint32_t integer(std::string_view value,bool zero=false) {
  std::uint32_t n=0;const auto r=std::from_chars(value.data(),value.data()+value.size(),n);
  require(r.ec==std::errc{} && r.ptr==value.data()+value.size() && (n || zero),"Expected a bounded whole number");return n;
}
void identity(std::string_view id) {
  require(!id.empty() && id.size()<=64 && std::all_of(id.begin(),id.end(),[](char c){return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_';}),"IDs use 1-64 lowercase letters, digits or underscores");
}
std::string textTex(std::string_view value) {
  std::string result="\\text{";
  for(char c:value) {
    switch(c) {
      case '\\':result+="\\textbackslash{}";break;
      case '^':result+="\\textasciicircum{}";break;
      case '~':result+="\\textasciitilde{}";break;
      case '{':case '}':case '_':case '$':case '%':case '&':case '#':result+='\\';[[fallthrough]];
      default:result+=c;
    }
  }
  return result+'}';
}
struct Line {std::string file,text;std::size_t number;};
struct Expander {
  Path root;std::size_t bytes=0;std::set<Path> stack;
  const std::map<Path,std::string>* snapshot=nullptr;
  std::map<Path,std::string> consumed;
  std::vector<Line> read(const Path& relative) {
    require(!relative.empty() && !relative.is_absolute(),"Include must be relative to its document");
    auto file=root;
    for(const auto& part:relative) {
      require(part!=".." && part!=".","Includes cannot leave their document folder or use dot segments");
      file/=part;
    }
    require(relative.generic_string().size()<=160 && file.extension()==".md","Use a .md document/include path of at most 160 bytes");
    require(stack.size()<8 && stack.insert(file).second,"Include cycle or nesting beyond eight files");
    if(!consumed.contains(relative)) {
      if(snapshot){require(snapshot->contains(relative),"Include is absent from the verified inventory: "+relative.generic_string());consumed.emplace(relative,snapshot->at(relative));}
      else consumed.emplace(relative,readBytes(file,maxFile));
    }
    std::istringstream in(consumed.at(relative));
    std::vector<Line> lines;std::string text;std::size_t n=0;bool fence=false;
    while(std::getline(in,text)) {
      ++n;if(!text.empty() && text.back()=='\r')text.pop_back();bytes+=text.size()+1;
      require(bytes<=maxExpanded,"Expanded document folder exceeds 2 MiB");
      require(text.find('\0')==std::string::npos,"NUL is not allowed in a document");
      const auto stripped=trim(text);
      if(stripped.starts_with("```"))fence=!fence;
      if(!fence && stripped.starts_with("@include ")) {
        try {auto nested=read(relative.parent_path()/trim(std::string_view(stripped).substr(9)));lines.insert(lines.end(),std::make_move_iterator(nested.begin()),std::make_move_iterator(nested.end()));}
        catch(const DocumentError&){throw;}
        catch(const std::exception& e){throw DocumentError(relative.generic_string(),n,"include","document.include",e.what());}
      } else lines.push_back({relative.generic_string(),std::move(text),n});
    }
    require(!in.bad(),"Cannot finish reading "+relative.generic_string());require(!fence,"Unclosed code fence in "+relative.generic_string());
    stack.erase(file);return lines;
  }
};
struct Step {std::uint32_t id=0,answer=0;std::string prompt,after,why,wrong,definitions,teaching;std::optional<std::string> hint;std::vector<std::pair<std::uint32_t,std::string>> choices;std::optional<std::size_t> operation;Line source;std::map<std::string,Line> fields;std::vector<Line> choiceSources;};
struct Block {
  bool question=false;std::string id,title,goal,given,domain,body;std::uint32_t version=0;
  std::optional<Template> format;std::vector<Step> steps;std::optional<CorpusFigure> figure;
  std::vector<std::string> links;Line source;
  std::map<std::string,Line> fields;
  std::vector<BookBlock> lesson;
  bool bookBlock=false;
  std::optional<unsigned> help;
  std::uint8_t helpSeen=0;
};
struct Compiler {
  MathCorpus& corpus;std::vector<CorpusStarter>& questions;DocumentImport& report;
  Json& details;
  std::map<std::string,Line> questionSources;
  std::optional<std::size_t> subject,chapter;std::optional<Block> block;
  std::string* prose=nullptr;
  void globalId(const std::string& id)const {
    identity(id);
    const auto used=[&](const auto& list){return std::any_of(list.begin(),list.end(),[&](const auto& e){return e.id==id;});};
    require(!used(corpus.subjects) && !used(corpus.topics) && !used(corpus.entries),"Repeated catalogue ID: "+id);
  }
  Block& current(){require(block.has_value(),"This command needs an open @lesson or @question");return *block;}
  Step& step(){require(current().question && !block->steps.empty(),"This command needs a @step");return block->steps.back();}
  BookBlock& bookBlock(){require(!current().question && block->format==Template::BookLesson && block->bookBlock,"This command needs an open @block in lesson.v2");return block->lesson.back();}
  void field(std::string& target,const std::string& value) {
    require(target.empty(),"Repeated field");target=value.empty()?"":value+'\n';prose=&target;
  }
  void finish(const Line& line) {
    auto& b=current();require(b.format.has_value(),"Missing @template");
    const auto& topic=corpus.topics[*chapter];const auto& s=corpus.subjects[topic.subject];
    const auto format=std::find_if(templates.begin(),templates.end(),[&](const auto& t){return t.second==*b.format;});
    Json entity{{"kind",b.question?"question":"lesson"},{"id",b.id},{"title",b.title},{"parent",topic.id},{"subject",s.id},
      {"template",format->first},{"source",location(b.source.file,b.source.number)},{"links",b.links}};
    if(!b.question) {
      require(*b.format==Template::Lesson || *b.format==Template::BookLesson,"A lesson uses @template lesson.v1 or lesson.v2");
      require(!b.bookBlock,"Close the textbook block with @endblock");
      if(*b.format==Template::BookLesson) {
        require(!b.lesson.empty() && trim(b.body).empty(),"lesson.v2 needs @block sections, without unscoped reading text");
        std::set<std::string> ids,numbers,equationNumbers;
        for(const auto& section:b.lesson) {
          require(ids.insert(section.id).second,"Repeated textbook block ID: "+section.id);
          require(section.number.empty() || numbers.insert(section.number).second,"Repeated textbook block number: "+section.number);
          for(const auto& p:section.body)b.body+=p.kind==BookPassage::Kind::Prose?p.text+"\n\n":"$$"+p.text+"$$\n\n";
          const auto checkNumbers=[&](const auto& passages){for(const auto& p:passages)require(p.number.empty() || equationNumbers.insert(p.number).second,"Repeated display equation number: "+p.number);};
          checkNumbers(section.body);for(const auto& h:section.help)checkNumbers(h);
        }
        for(const auto& section:b.lesson)for(const auto& ref:section.references)require(ids.contains(ref.target),"Unresolved textbook reference: "+ref.target);
      }
      globalId(b.id);require(!trim(b.body).empty(),"A lesson needs reading text");
      Json entry{{"id",b.id},{"title",b.title},{"kind","lesson"},{"topic",topic.id},{"body",trim(b.body)},
        {"source",b.source.file},{"first_line",b.source.number},{"last_line",line.file==b.source.file?line.number:b.source.number}};
      const Json root{{"schema_version",1U},{"review_status","unreviewed"},{"subjects",Json::array({{{"id",s.id},{"title",s.title}}})},
        {"topics",Json::array({{{"id",topic.id},{"title",topic.title},{"subject",s.id}}})},{"entries",Json::array({entry})}};
      auto parsed=parseMathCorpus(root.dump()).entries.front();parsed.topic=*chapter;parsed.document=true;
      if(b.figure){b.figure->caption=trim(b.figure->caption);require(!b.figure->caption.empty() && b.figure->caption.size()<=1024,"A figure needs @caption of at most 1024 bytes describing its meaning");(void)instantiateDocumentFigure(*b.figure);}
      parsed.figure=std::move(b.figure);parsed.questions=std::move(b.links);parsed.lesson=std::move(b.lesson);corpus.entries.push_back(std::move(parsed));++report.lessons;
      if(const auto& figure=corpus.entries.back().figure) {
        const auto caps=Json::parse(learningDocumentCapabilities());
        for(const auto& spec:caps.at("figures"))if(spec.at("key")==figure->provider) {
          Json contract{{"key",figure->provider},{"level",figure->level},{"parameters",Json::array()}};
          for(const auto& [key,value]:figure->parameters)for(const auto& p:spec.at("parameters"))if(p.at("key")==key)contract["parameters"].push_back(p);
          entity["figure"]=std::move(contract);
        }
      }
    } else {
      require(*b.format!=Template::Lesson && *b.format!=Template::BookLesson,"A question uses choices.v1, linear.v1 or matrix.v1");
      require(std::none_of(questions.begin(),questions.end(),[&](const auto& q){return q.id==b.id;}),"Repeated question ID: "+b.id);
      b.goal=trim(b.goal);b.given=trim(b.given);b.domain=trim(b.domain);
      require(b.version && !b.goal.empty() && !b.given.empty() && !b.domain.empty(),"Question needs @version, @goal, @given and @domain");
      require(!b.steps.empty(),"Question needs at least one @step");const bool linear=*b.format==Template::Linear,matrix=*b.format==Template::Matrix,typed=linear || matrix;
      require(!typed || b.links.empty(),"Four-level templates use step @definitions/@teaching, not @read");
      const auto tex=[&](const std::string& raw,const Line& source,std::string_view field)->std::string {
        try {switch(*b.format) {
          case Template::Linear: {
            const auto parsed=fm::parseLinearEquation(raw);require(parsed.equation.has_value(),"linear.v1 expects plain linear equations: "+raw);return fm::linearEquationTex(*parsed.equation);
          }
          case Template::Matrix: {
            const auto parsed=fm::parseAugmentedMatrix(raw);require(parsed.result.has_value(),"matrix.v1 expects [a, b | c] [d, e | f]: "+raw);return fm::matrixEquationTex(*parsed.result);
          }
          default:return raw;
        }} catch(const std::exception& e){throw DocumentError(source.file,source.number,std::string(field),"document.math",e.what());}
      };
      Json states=Json::array({{{"id",1U},{"display",tex(b.given,b.fields.at("given"),"given")}}}),steps=Json::array(),support=Json::array();
      for(std::size_t i=0;i<b.steps.size();++i) {
        auto& st=b.steps[i];st.after=trim(st.after);require(st.answer && !st.after.empty() && !trim(st.why).empty() && !trim(st.wrong).empty(),"Every @step needs @answer, @after, @why and @wrong");
        Json options=Json::array(),responses=Json::array();std::string prefix;
        require(!matrix || st.operation.has_value(),"Each matrix step needs @operation");
        if(linear){const auto at=st.after.find('=');require(at!=std::string::npos,"Linear @after needs '='");prefix=st.after.substr(0,at+1);}
        for(std::size_t j=0;j<st.choices.size();++j) {
          const auto& [id,value]=st.choices[j];
          auto label=value;
          if(linear){label=tex(prefix+value,st.choiceSources[j],"choice");label=label.substr(label.find('=')+1);responses.push_back(value);}
          if(matrix){label=fm::rowOperationTex(fm::rowOperations[*st.operation].operation,value);require(!label.empty(),"Matrix choices need exact numeric operands");responses.push_back(value);}
          options.push_back({{"id",id},{"label",label}});
        }
        states.push_back({{"id",i+2},{"display",tex(st.after,st.fields.at("after"),"after")}});
        steps.push_back({{"id",st.id},{"layer_name","Step "+std::to_string(i+1)},{"prompt",st.prompt},{"options",options},
          {"accepted_option_ids",Json::array({st.answer})},{"wrong_hint",trim(st.wrong)},{"explanation",trim(st.why)},
          {"semantics",{{"purpose","calculation"},{"completion","any_accepted"},{"before",i+1},{"after",i+2}}}});
        if(st.hint) {
          auto hint=trim(*st.hint);const auto& at=st.fields.at("hint");
          if(hint.empty() || hint.size()>8000)throw DocumentError(at.file,at.number,"hint","document.content","@hint needs 1-8000 bytes of guidance; reserve the reached answer for @after/@teaching");
          steps.back()["hint"]=std::move(hint);
        }
        if(typed) {
          support.push_back({{"equation",st.after},{"response_prefix",prefix},{"responses",responses},{"definitions",trim(st.definitions)},{"teaching",trim(st.teaching)}});
          if(matrix)support.back()["operation"]=fm::rowOperations[*st.operation].key;
        }
        else require(st.definitions.empty() && st.teaching.empty(),"choices.v1 uses @read for shared teaching");
      }
      Json question{{"schema_version",1U},{"id",b.id},{"content_version",b.version},{"equation",tex(b.given,b.fields.at("given"),"given")},
        {"skill",typed?(matrix?"matrix_rows_2x2":"linear_balance_ax_b"):"document_choices"},{"description",b.goal+(typed?"":" "+b.domain)},
        {"working_states",states},{"steps",steps}};
      if(typed)question["support"]={{"family",matrix?"matrix_rows_2x2_v1":"linear_balance_ax_b_v1"},{"equation",b.given},{"domain",b.domain},{"steps",support}};
      const Json row{{"id",b.id},{"title",b.title},{"level","practice"},{"subject",s.id},{"topic",topic.id},{"reading_refs",b.links},{"question",question}};
      std::vector<CorpusStarter> parsed;
      try {parsed=parseCorpusStarters(Json{{"schema_version",1U},{"questions",Json::array({row})}}.dump(),corpus,b.source.file+":"+std::to_string(b.source.number));}
      catch(const QuestionContentError& e) {
        if(!e.validation)throw;
        const auto& v=*e.validation;const Step* st=v.stepIndex && *v.stepIndex<b.steps.size()?&b.steps[*v.stepIndex]:nullptr;
        const std::map<std::string_view,std::string_view> names{{"equation",st?"after":"given"},{"responses","choice"},{"acceptedOptions","answer"},{"options","choice"},{"support","template"}};
        const auto name=names.find(v.field);const std::string key(name==names.end()?v.field:name->second);
        const auto& fields=st?st->fields:b.fields;const auto found=fields.find(key);
        auto source=found!=fields.end()?found->second:st?st->source:b.source;
        if(st && key=="choice" && v.optionIndex && *v.optionIndex<st->choiceSources.size())source=st->choiceSources[*v.optionIndex];
        throw DocumentError(source.file,source.number,key,"document.math",b.id+(st?" step "+std::to_string(st->id):"")+": "+std::string(v.reason()));
      }
      questions.push_back(std::move(parsed.front()));questionSources.emplace(b.id,b.source);++report.questions;
      entity["version"]=b.version;entity["stamp_sha256"]=digest(questions.back().stamp);
    }
    details["entities"].push_back(std::move(entity));
    require(corpus.entries.size()<=4096,"Combined catalogue exceeds 4096 reading entries");block.reset();prose=nullptr;
  }
  void document(const std::vector<Line>& lines) {
    subject.reset();chapter.reset();bool header=false,fence=false;
    std::string activeField;
    for(const auto& line:lines)try {
      activeField="text";
      const auto text=trim(line.text);
      if(text.starts_with("```"))fence=!fence;
      if(fence || text.starts_with("```") || !text.starts_with('@') || text.starts_with("@@")) {
        if(text.empty()){if(prose)*prose+='\n';continue;}
        require(header,"Start each document with @paths 1");require(prose,"Text needs a lesson body or a question text field");
        *prose+=(text.starts_with("@@")?text.substr(1):line.text)+'\n';continue;
      }
      const auto at=text.find_first_of(" \t");const auto key=text.substr(1,at==text.npos?text.size()-1:at-1),value=at==text.npos?std::string{}:trim(std::string_view(text).substr(at+1));
      activeField=key;
      const auto found=std::find_if(commands.begin(),commands.end(),[&](const auto& c){return key==c.first;});
      require(found!=commands.end(),"Unknown command @"+key);const auto cmd=found->second;
      require(header || cmd==Command::Paths,"Start each document with @paths 1");prose=nullptr;
      if(block && block->question) {
        block->fields.try_emplace(key,line);
        if(!block->steps.empty())block->steps.back().fields.try_emplace(key,line);
      }
      require((cmd!=Command::Figure && cmd!=Command::Parameter && cmd!=Command::Caption) || !current().bookBlock,
              "@"+key+" belongs outside @block; lesson figures and their metadata are always public");
      switch(cmd) {
        case Command::Paths:require(!header && !block && value=="1","Expected one @paths 1 header");header=true;break;
        case Command::Subject: {
          require(!block,"Close the block with @end before changing subject");const auto [id,title]=split(value,'|');identity(id);
          auto found=std::find_if(corpus.subjects.begin(),corpus.subjects.end(),[&](const auto& s){return s.id==id;});
          if(found==corpus.subjects.end()){globalId(id);require(corpus.subjects.size()<32 && title.size()<=160,"Subject capacity/title limit");corpus.subjects.push_back({id,title});subject=corpus.subjects.size()-1;}
          else {require(found->title==title,"Subject ID has a different title: "+id);subject=found-corpus.subjects.begin();}
          details["entities"].push_back({{"kind","subject"},{"id",id},{"title",title},{"source",location(line.file,line.number)}});
          chapter.reset();break;
        }
        case Command::Chapter: {
          require(!block && subject.has_value(),"Declare @subject before @chapter");const auto [id,title]=split(value,'|');identity(id);
          auto found=std::find_if(corpus.topics.begin(),corpus.topics.end(),[&](const auto& t){return t.id==id;});
          if(found==corpus.topics.end()){globalId(id);require(corpus.topics.size()<512 && title.size()<=240,"Chapter capacity/title limit");corpus.topics.push_back({id,title,*subject});chapter=corpus.topics.size()-1;}
          else {require(found->title==title && found->subject==*subject,"Chapter ID has a different title or subject: "+id);chapter=found-corpus.topics.begin();}
          details["entities"].push_back({{"kind","chapter"},{"id",id},{"title",title},{"parent",corpus.subjects[*subject].id},{"source",location(line.file,line.number)}});break;
        }
        case Command::Lesson:case Command::Question: {
          require(!block && chapter.has_value(),"Declare a chapter and close the preceding block with @end");const auto [id,title]=split(value,'|');identity(id);
          block.emplace();block->question=cmd==Command::Question;block->id=id;block->title=title;block->source=line;
          if(!block->question)prose=&block->body;break;
        }
        case Command::Template: {
          auto& b=current();require(!b.format,"Repeated @template");
          const auto t=std::find_if(templates.begin(),templates.end(),[&](const auto& t){return value==t.first;});
          require(t!=templates.end(),"Unknown template: "+value);b.format=t->second;if(!b.question && b.format==Template::Lesson)prose=&b.body;break;
        }
        case Command::Version:require(current().question && !block->version,"One @version is required per question");block->version=integer(value);break;
        case Command::Goal:require(current().question,"@goal belongs to a question");field(block->goal,value);break;
        case Command::Given:require(current().question,"@given belongs to a question");field(block->given,value);break;
        case Command::Domain:require(current().question,"@domain belongs to a question");field(block->domain,value);break;
        case Command::Step: {
          require(current().question && block->steps.size()<32,"Questions have at most 32 steps");const auto [id,prompt]=split(value,'|');Step st;st.id=integer(id);st.prompt=prompt;st.source=line;block->steps.push_back(std::move(st));break;
        }
        case Command::Operation: {
          auto& s=step();require(current().format==Template::Matrix && !s.operation,"One @operation per matrix step");
          const auto op=std::find_if(fm::rowOperations.begin(),fm::rowOperations.end(),[&](const auto& op){return op.key==value;});
          require(op!=fm::rowOperations.end() && op->kind!=fm::RowMove::Swap,"matrix.v1 needs a named row-addition or row-division operation");
          s.operation=op-fm::rowOperations.begin();break;
        }
        case Command::Choice:case Command::TextChoice: {
          auto& s=step();require(s.choices.size()<8,"A step has at most eight choices");const auto [id,label]=split(value,'|');
          require(cmd!=Command::TextChoice || current().format==Template::Choices,"@textchoice is for choices.v1");s.choices.emplace_back(integer(id),cmd==Command::TextChoice?textTex(label):label);s.choiceSources.push_back(line);break;
        }
        case Command::Answer:require(!step().answer,"One @answer per step");step().answer=integer(value);break;
        case Command::After:field(step().after,value);break;
        case Command::Why:field(step().why,value);break;
        case Command::Wrong:field(step().wrong,value);break;
        case Command::Definitions:field(step().definitions,value);break;
        case Command::Teaching:field(step().teaching,value);break;
        case Command::Hint: {
          auto& s=step();require((current().format==Template::Linear || current().format==Template::Matrix) && !s.hint,"One @hint per linear.v1 or matrix.v1 step");
          s.hint.emplace();field(*s.hint,value);break;
        }
        case Command::Read:require(current().question,"@read belongs to a question");identity(value);require(block->links.size()<8,"At most eight reading references");block->links.push_back(value);break;
        case Command::Practice:require(!current().question && !block->bookBlock,"@practice belongs outside a textbook block");identity(value);require(block->links.size()<16,"At most sixteen question links");block->links.push_back(value);if(block->format==Template::Lesson)prose=&block->body;break;
        case Command::Text:require(!current().question && current().format==Template::Lesson && value.empty(),"@text resumes a lesson.v1 body and takes no arguments");prose=&block->body;break;
        case Command::Block: {
          auto& b=current();require(!b.question && b.format==Template::BookLesson && !b.bookBlock && b.lesson.size()<64,"lesson.v2 has at most 64 closed @block sections");
          const auto [kind,rest]=split(value,'|');const auto [id,labels]=split(rest,'|');const auto [number,title]=split(labels,'|');identity(id);
          const auto found=std::find_if(blockKinds.begin(),blockKinds.end(),[&](const auto& k){return k.first==kind;});
          require(found!=blockKinds.end() && number.size()<=32 && title.size()<=240,"Unknown block kind or overlong number/title");
          b.lesson.push_back({id,found->second,number=="-"?"":number,title,{}});b.bookBlock=true;b.help.reset();b.helpSeen=0;break;
        }
        case Command::Prose:case Command::Display: {
          auto& b=bookBlock();auto& content=block->help?b.help[*block->help]:b.body;require(content.size()<64,"At most 64 passages per body/disclosure");
          require(cmd!=Command::Display || value.size()<=32,"Display equation numbers are at most 32 bytes; put the equation on the next line");
          content.push_back({cmd==Command::Prose?BookPassage::Kind::Prose:BookPassage::Kind::DisplayMath,"",cmd==Command::Display?value:""});
          field(content.back().text,cmd==Command::Prose?value:"");break;
        }
        case Command::Help: {
          auto& b=bookBlock();const auto found=std::find_if(helpKinds.begin(),helpKinds.end(),[&](const auto& h){return h.first==value;});
          require(found!=helpKinds.end(),"Help is proof, hint, answer or solution");const auto h=static_cast<unsigned>(found->second);
          require(!(block->helpSeen&(1u<<h)),"Repeated disclosure; put its passages under one @help command");block->helpSeen|=1u<<h;block->help=h;break;
        }
        case Command::Body:(void)bookBlock();require(value.empty(),"@body takes no arguments");block->help.reset();break;
        case Command::Reference: {
          auto& b=bookBlock();const auto [target,label]=split(value,'|');identity(target);require(b.references.size()<16 && label.size()<=240,"At most sixteen bounded references per block");
          b.references.push_back({label,target});break;
        }
        case Command::EndBlock: {
          auto& b=bookBlock();require(value.empty() && !b.body.empty(),"@endblock takes no arguments; a block needs a public body");
          const auto check=[](auto& content){for(auto& p:content){p.text=trim(p.text);require(!p.text.empty() && p.text.size()<=8192,"Each passage needs 1-8192 bytes of text");}};
          check(b.body);for(unsigned h=0;h<b.help.size();++h){require(!(block->helpSeen&(1u<<h)) || !b.help[h].empty(),"A declared disclosure needs at least one passage");check(b.help[h]);}block->bookBlock=false;block->help.reset();break;
        }
        case Command::Figure: {
          auto& b=current();require(!b.question && !b.figure,"One figure per lesson");const auto [key,level]=split(value,' ');b.figure=CorpusFigure{key,{},integer(level,true)};prose=&b.body;break;
        }
        case Command::Parameter: {
          auto& b=current();require(b.figure.has_value(),"Declare @figure before its @parameter");const auto [key,raw]=split(value,' ');double n=0;
          const auto r=std::from_chars(raw.data(),raw.data()+raw.size(),n);require(r.ec==std::errc{} && r.ptr==raw.data()+raw.size() && std::isfinite(n),"Expected a finite diagram parameter");
          require(b.figure->parameters.size()<16 && std::none_of(b.figure->parameters.begin(),b.figure->parameters.end(),[&](const auto& p){return p.first==key;}),"Repeated parameter or more than sixteen parameters");
          b.figure->parameters.emplace_back(key,n);prose=&b.body;break;
        }
        case Command::Caption:require(current().figure.has_value(),"@caption needs a figure");field(block->figure->caption,value);break;
        case Command::End:require(value.empty(),"@end takes no arguments");finish(line);break;
      }
    } catch(const DocumentError&){throw;}
      catch(const std::exception& e){throw DocumentError(line.file,line.number,activeField,"document.content",e.what());}
    require(header,"Document needs @paths 1");
    if(block)throw DocumentError(block->source.file,block->source.number,"end","document.content","Missing @end");
  }
};
}
MathObjects instantiateDocumentFigure(const CorpusFigure& f) {
  const auto registry=mathObjectSpecs();const auto spec=std::find_if(registry.begin(),registry.end(),[&](const auto& s){return s.key==f.provider;});
  require(spec!=registry.end(),"Unknown diagram provider: "+f.provider);
  MathObjects model;const auto apply=[&](MathAction a){const auto r=model.dispatch(a);require(r.accepted,std::string(r.reason));};
  apply({MathActionKind::Select,spec->id});apply({MathActionKind::SetLevel,spec->id,MathParameter::X,static_cast<double>(f.level)});
  for(const auto& [key,value]:f.parameters) {
    const auto fields=mathParameterSpecs();const auto parameter=std::find_if(fields.begin(),fields.end(),[&](const auto& p){return p.key==key && p.owner==spec->id;});
    require(parameter!=fields.end(),"Unknown parameter for "+f.provider+": "+key);apply({MathActionKind::SetParameter,spec->id,parameter->id,value});
  }
  return model;
}
namespace {
std::vector<Path> documentFiles(const Path& folder) {
  realPath(folder);require(std::filesystem::is_directory(folder),"Document folder must be a real directory: "+folder.string());
  std::vector<Path> files;std::size_t entries=0;
  for(const auto& entry:std::filesystem::recursive_directory_iterator(folder)) {
    require(++entries<=2048,"Document folder has more than 2048 filesystem entries");
    require(!entry.is_symlink(),"Document folder contains a symlink: "+entry.path().filename().string());
    if(entry.is_regular_file() && entry.path().extension()==".md")files.push_back(entry.path().lexically_relative(folder));
  }
  std::sort(files.begin(),files.end());return files;
}
DocumentImport importSnapshot(const Path& folder,MathCorpus& corpus,std::vector<CorpusStarter>& questions,
                              const std::map<Path,std::string>* snapshot=nullptr,bool exactClosure=true) {
  DocumentImport result;auto detail=emptyReport();
  try {
    Expander reader{folder};reader.snapshot=snapshot;std::vector<Path> files;
    if(snapshot) {
      for(const auto& [path,bytes]:*snapshot)if(path.filename().string().ends_with(".paths.md"))files.push_back(path);
    } else {
      for(const auto& file:documentFiles(folder))if(file.filename().string().ends_with(".paths.md"))files.push_back(file);
    }
    require(files.size()<=maxDocuments,"At most 128 .paths.md documents per folder");std::sort(files.begin(),files.end());
    detail["base_catalogue_sha256"]=digest(catalogue(corpus,questions).dump());
    auto stagedCorpus=corpus;auto stagedQuestions=questions;Compiler compiler{stagedCorpus,stagedQuestions,result,detail};
    for(const auto& file:files) {
      try{compiler.document(reader.read(file));}
      catch(const DocumentError&){throw;}
      catch(const std::exception& e){throw DocumentError(file.generic_string(),1,"","document.input",e.what());}
      detail["entry_documents"].push_back(file.generic_string());++result.files;
    }
    for(const auto& e:stagedCorpus.entries)if(e.document)for(const auto& id:e.questions)
      if(std::count_if(stagedQuestions.begin(),stagedQuestions.end(),[&](const auto& q){return q.id==id;})!=1)
        throw DocumentError(e.source,e.firstLine,"practice","document.reference","Unresolved @practice "+id);
    for(std::size_t i=questions.size();i<stagedQuestions.size();++i)for(const auto& id:stagedQuestions[i].readingRefs) {
      const auto& source=compiler.questionSources.at(stagedQuestions[i].id);
      if(std::count_if(stagedCorpus.entries.begin(),stagedCorpus.entries.end(),[&](const auto& e){return e.id==id;})!=1)
        throw DocumentError(source.file,source.number,"read","document.reference","Unresolved @read "+id);
    }
    (void)CorpusPractice(stagedQuestions);
    for(const auto& [path,bytes]:reader.consumed)detail["files"].push_back({{"path",path.generic_string()},{"bytes",bytes.size()},{"sha256",digest(bytes)}});
    if(snapshot && exactClosure)require(reader.consumed==*snapshot,"Published documents differ from the compiler's include closure");
    detail["catalogue"]=catalogue(stagedCorpus,stagedQuestions);
    result.message="Documents loaded: "+std::to_string(result.files)+" files, "+std::to_string(result.lessons)+" lessons, "+std::to_string(result.questions)+" questions.";
    detail["accepted"]=true;detail["message"]=result.message;result.reportJson=detail.dump(2)+"\n";
    corpus=std::move(stagedCorpus);questions=std::move(stagedQuestions);
  } catch(const std::exception& e){return failedReport(e);}
  return result;
}
}
DocumentImport importLearningDocuments(const Path& folder,MathCorpus& corpus,std::vector<CorpusStarter>& questions) {
  return importSnapshot(folder,corpus,questions);
}
LearningDocumentPreview::LearningDocumentPreview(Path folder,const MathCorpus& corpus,const std::vector<CorpusStarter>& questions)
  :folder_(std::move(folder)),baseline_(corpus),questions_(questions) {}
bool LearningDocumentPreview::poll(MathCorpus& corpus,CorpusPractice& practice,Clock::time_point now) {
  if(now<nextPoll_)return false;
  nextPoll_=now+std::chrono::milliseconds(250);
  bool captured=false;
  try {
    std::map<Path,std::string> bytes;std::size_t total=0;
    for(const auto& file:documentFiles(folder_)) {
      auto text=readBytes(folder_/file,maxFile);total+=text.size();
      require(total<=maxExpanded,"Preview Markdown exceeds 2 MiB");bytes.emplace(file,std::move(text));
    }
    captured=true;
    const bool initial=!observed_ && !revision_;
    if(!observed_ || bytes!=*observed_) {
      observed_=std::move(bytes);handled_=false;
      if(!initial)return false; // Two identical observations allow atomic editor saves to settle.
    }
    if(handled_)return false;
    handled_=true;auto staged=baseline_;auto questions=questions_;
    report_=importSnapshot(folder_,staged,questions,&*observed_,false);
    if(!report_.accepted){report_.message+=" Last valid preview retained.";return false;}
    const auto mapping=[](const auto& before,const auto& after) {
      std::vector<std::optional<std::size_t>> result;
      for(const auto& old:before) {
        const auto found=std::find_if(after.begin(),after.end(),[&](const auto& value){return value.id==old.id;});
        result.push_back(found==after.end()?std::nullopt:std::optional<std::size_t>(found-after.begin()));
      }
      return result;
    };
    DocumentRemap remap{mapping(corpus.subjects,staged.subjects),mapping(corpus.topics,staged.topics),mapping(corpus.entries,staged.entries)};
    practice.replacePreview(std::move(questions));corpus=std::move(staged);remap_=std::move(remap);++revision_;
    report_.message="Live preview #"+std::to_string(revision_)+" · Session only";return true;
  } catch(const std::exception& e) {
    if(!captured){observed_.reset();handled_=false;}
    report_=failedReport(e,"document.preview");report_.message+=" Last valid preview retained.";return false;
  }
}
DocumentImport importLearningStore(const Path& store,MathCorpus& corpus,std::vector<CorpusStarter>& questions) {
  try {
    const auto active=Json::parse(readBytes(store/"active.json",4096));
    require(hasFormat(active,"paths_learning_store"),"Unknown learning store format");
    const auto generation=active.at("generation").get<std::string>();
    require(generation.size()==64 && generation.find_first_not_of("0123456789abcdef")==generation.npos,"Invalid generation identity");
    const auto root=store/"generations"/generation;
    const auto manifestBytes=readBytes(root/"library.json",4*1024*1024);
    require(digest(manifestBytes)==generation,"Published library manifest changed");
    const auto manifest=Json::parse(manifestBytes);
    require(hasFormat(manifest,"paths_learning_library"),"Unknown published library format");
    const auto& inventory=manifest.at("files");require(inventory.is_array() && inventory.size()<=4096,"Invalid library inventory size");
    std::map<Path,std::string> documents;std::set<Path> names;std::size_t total=manifestBytes.size();
    for(const auto& item:inventory) {
      const auto name=item.at("path").get<std::string>();const auto relative=relativePath(name);
      require(name!="library.json" && names.insert(relative).second,"Repeated or reserved inventory path");
      const bool document=name.starts_with("documents/");
      require(document || name.starts_with("packages/"),"Unexpected published file location");
      const auto bytes=readBytes(root/relative,document?maxFile:4*1024*1024);total+=bytes.size();
      require(total<=16*1024*1024 && item.at("bytes").is_number_unsigned() && item.at("bytes").get<std::size_t>()==bytes.size() && item.at("sha256")==digest(bytes),"Published file changed: "+name);
      if(document)documents.emplace(relative.lexically_relative("documents"),bytes);
    }
    std::size_t count=0;
    for(const auto& item:std::filesystem::recursive_directory_iterator(root)) {
      require(++count<=6144 && !item.is_symlink(),"Invalid published filesystem entry");
      if(item.is_directory())continue;
      require(item.is_regular_file(),"Published library contains a special file");
      const auto relative=item.path().lexically_relative(root);
      require(relative=="library.json" || names.contains(relative),"Unlisted published file: "+relative.generic_string());
    }
    auto stagedCorpus=corpus;auto stagedQuestions=questions;
    auto result=importSnapshot(root/"documents",stagedCorpus,stagedQuestions,&documents);
    if(!result.accepted)return result;
    const auto detail=Json::parse(result.reportJson);
    require(detail.at("catalogue").at("questions")==manifest.at("questions"),"Published question stamps no longer match this app; original progress retained");
    corpus=std::move(stagedCorpus);questions=std::move(stagedQuestions);return result;
  } catch(const std::exception& e){return failedReport(e,"document.store");}
}
std::string learningDocumentCapabilities() {
  Json result{{"format_version",1},{"document_format","paths.md v1"},{"templates",Json::array()},{"figures",Json::array()}};
  result["limits"]={{"documents",maxDocuments},{"file_bytes",maxFile},{"expanded_bytes",maxExpanded},{"filesystem_entries",2048},{"include_depth",8},{"subjects",32},{"chapters",512},{"readings",4096},{"questions",1024},{"steps",32},{"choices",8}};
  for(const auto& [key,kind]:templates)result["templates"].push_back(key);
  result["step_hint"]={{"templates",{"linear.v1","matrix.v1"}},{"directive","hint"},{"optional",true},{"bytes",8000}};
  result["book_template"]={{"blocks",64},{"passages_per_body_or_help",64},{"passage_bytes",8192},{"references_per_block",16},
    {"kinds",Json::array()},{"help",Json::array()}};
  for(const auto& [key,kind]:blockKinds)result["book_template"]["kinds"].push_back(key);
  for(const auto& [key,kind]:helpKinds)result["book_template"]["help"].push_back(key);
  result["matrix_template"]={{"rows",2},{"variables",{"x","y"}},{"operations",Json::array()},
    {"written_format","One complete matrix per line: [a, b | c] [d, e | f]"},{"requires_unique_solution",true}};
  for(const auto& op:fm::rowOperations)if(op.kind!=fm::RowMove::Swap)result["matrix_template"]["operations"].push_back(op.key);
  for(const auto& s:mathObjectSpecs()) {
    Json parameters=Json::array();for(const auto& p:mathParameterSpecs())if(p.owner==s.id)parameters.push_back({{"key",p.key},{"minimum",p.minimum},{"maximum",p.maximum},{"step",p.step},{"initial",p.initial},{"minimum_level",p.minimumLevel}});
    result["figures"].push_back({{"key",s.key},{"levels",std::max<std::size_t>(1,mathLessons(s.id).size())},{"parameters",parameters}});
  }
  return result.dump(2)+"\n";
}
}
