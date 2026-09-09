#include "content/LearningDocuments.hpp"
#include "content/QuestionContentIO.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <nlohmann/json.hpp>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace {
using Path=std::filesystem::path;
using Json=nlohmann::json;
void expect(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::string read(const Path& p){std::ifstream in(p);return {std::istreambuf_iterator<char>(in),{}};}
void write(const Path& p,const std::string& s){std::filesystem::create_directories(p.parent_path());std::ofstream out(p);out<<s;}
std::string replace(std::string s,const std::string& from,const std::string& to){const auto at=s.find(from);expect(at!=s.npos,"Mutation target exists: "+from);s.replace(at,from.size(),to);return s;}
struct Fixture {
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  std::vector<CorpusStarter> bank=loadCorpusStarters(STARTER_FIXTURE,corpus);
  DocumentImport load(const Path& p){return importLearningDocuments(p,corpus,bank);}
};
const CorpusEntry& lesson(const MathCorpus& c,const std::string& id){const auto e=std::find_if(c.entries.begin(),c.entries.end(),[&](const auto& e){return e.id==id;});expect(e!=c.entries.end(),"Imported lesson exists: "+id);return *e;}
std::size_t question(const std::vector<CorpusStarter>& q,const std::string& id){const auto at=std::find_if(q.begin(),q.end(),[&](const auto& q){return q.id==id;});expect(at!=q.end(),"Imported question exists: "+id);return at-q.begin();}
fm::LayeredQuestionCommand support(CorpusPractice& p,fm::SupportAction a,std::string text={},unsigned value=0) {
  fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::Support};c.support=p.active()->supportView()->command;c.support.action=a;c.support.text=std::move(text);c.support.value=value;return c;
}
void routes(const Fixture& f) {
  CorpusPractice p(f.bank);
  for(const auto* id:{"document_sine_question","document_membrane_question"}) {
    p.open(question(f.bank,id));
    while(!p.active()->currentRun().completed) {
      auto* s=p.active();const auto& step=s->content().steps[s->currentRun().currentStep];const auto correct=fm::firstAcceptedOption(step);
      const auto before=std::string(s->visibleWorking());
      expect(p.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[(correct+1)%step.options.size()].id)),"Wrong authored choice submitted");
      expect(s->visibleWorking()==before,"Wrong imported response retains canonical working");
      expect(p.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[correct].id)),"Correct authored choice submitted");
      expect(p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Existing prepared owner advances the workflow");
    }
    expect(!p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Imported completion stays until navigation");
  }
  for(unsigned level=0;level<4;++level) {
    CorpusPractice linear(f.bank);linear.open(question(f.bank,"document_balance_question"));
    expect(linear.dispatch(support(linear,fm::SupportAction::SelectLevel,{},level)),"Document supports the same four levels");
    if(level<2)for(std::size_t step=0;step<2;++step) {
      const auto& authored=linear.active()->content().steps[step];const auto correct=fm::firstAcceptedOption(authored);
      if(!level)expect(linear.dispatch(support(linear,fm::SupportAction::Choose,{},authored.options[correct].id.value)),"Learn uses imported symbolic choices");
      else {
        const auto text=step?"10/2":"20";
        expect(linear.dispatch(support(linear,fm::SupportAction::EditDraft,text)),"Practice draft is retained");
        expect(linear.dispatch(support(linear,fm::SupportAction::SubmitBlank,text)),"Practice uses the real exact checker");
      }
    } else {
      const std::string text="x-3/4=17/4\nx=5";
      expect(linear.dispatch(support(linear,fm::SupportAction::EditDraft,text)),"Own working composed");
      expect(linear.dispatch(support(linear,fm::SupportAction::CheckWork,text)),"Equivalent alternate route checked");
    }
    expect(linear.active()->currentRun().completed && !linear.active()->supportView()->verification.empty(),"Each imported linear level reaches verified completion");
  }
}
void livePreview(const Path& folder) {
  Fixture base;auto corpus=base.corpus;CorpusPractice p(base.bank);
  const auto root=folder/"live",entry=root/"matrix.paths.md",fragment=root/"parts/matrix.inc.md";
  const auto original=read(Path(DOCUMENT_FIXTURE)/"matrix.paths.md");write(fragment,original);write(entry,"@include parts/matrix.inc.md\n");
  LearningDocumentPreview preview(root,base.corpus,base.bank);auto now=LearningDocumentPreview::Clock::time_point{};
  const auto tick=[&]{now+=std::chrono::milliseconds(250);return preview.poll(corpus,p,now);};
  const auto reload=[&]{expect(!tick(),"Edits wait for a stable second observation");return tick();};
  expect(tick() && preview.revision()==1,"Initial preview uses the existing compiler");
  p.open(question(p.questions(),"document_matrix_question"));
  expect(p.dispatch(support(p,fm::SupportAction::SelectLevel,{},1)),"Preview can use Practice choices");
  expect(p.dispatch(support(p,fm::SupportAction::Choose,{},11)),"Wrong preview choice is recorded");
  expect(p.dispatch(support(p,fm::SupportAction::Choose,{},12)),"Correct preview choice advances");
  expect(p.dispatch(support(p,fm::SupportAction::EditDraft,"-3")),"Preview retains optional typed draft");
  expect(p.dispatch(support(p,fm::SupportAction::Undo)),"Preview records Undo branch");
  const auto working=p.active()->supportView()->working,draft=p.active()->supportView()->draft;
  const auto journal=p.active()->journal().size();const auto stamp=p.questions()[*p.selected()].stamp;
  expect(!preview.poll(corpus,p,now+std::chrono::milliseconds(10)) && !tick(),"Unchanged snapshots do not compile or reset attempts");
  const auto savedTime=std::filesystem::last_write_time(fragment);
  const auto renamed=replace(original,"Document matrix practice","Document matrix preview!");
  expect(renamed.size()==original.size(),"Same-size edit fixture");write(fragment,renamed);std::filesystem::last_write_time(fragment,savedTime);
  expect(reload(),"Byte changes reload even with unchanged file size and timestamp");
  expect(p.questions()[*p.selected()].title=="Document matrix preview!" && p.questions()[*p.selected()].stamp==stamp,"Title edit changes presentation without changing mathematics");
  expect(p.active()->supportView()->working==working && p.active()->supportView()->draft==draft && p.active()->journal().size()==journal,"Unchanged mathematics retains working, drafts and Undo branches");
  const auto oldEntry=static_cast<std::size_t>(&lesson(corpus,"document_matrix_reading")-corpus.entries.data());
  write(root/"aaa.paths.md","@paths 1\n@subject algebra | Algebra\n@chapter preview_extra | Extra\n@lesson preview_intro | Extra reading\n@template lesson.v1\nBefore the matrix.\n@end\n");
  expect(reload(),"New entry documents are discovered");
  expect(preview.remap().entries[oldEntry] && corpus.entries[*preview.remap().entries[oldEntry]].id=="document_matrix_reading" && *preview.remap().entries[oldEntry]!=oldEntry,"Reading selection can follow stable identity after reordering");
  write(fragment,replace(renamed,"@after [1, 1 | 3] [0, -3 | -6]","@after [1, 1 | 3] [0, -3 | -5]"));
  const auto acceptedRevision=preview.revision();expect(!reload() && !preview.report().accepted,"Invalid edit rejects the entire candidate");
  const auto diagnostic=Json::parse(preview.report().reportJson)["diagnostics"][0];
  expect(diagnostic["file"]=="parts/matrix.inc.md" && diagnostic["field"]=="after" && diagnostic["line"].get<unsigned>()>0,"Live errors preserve original include location");
  expect(preview.revision()==acceptedRevision && p.active()->supportView()->working==working && p.questions()[*p.selected()].stamp==stamp,"Rejected edits leave the active catalogue and attempt untouched");
  expect(!tick(),"Unchanged invalid text is not repeatedly compiled");write(fragment,renamed);expect(reload(),"Repairing an error restores accepted status");
  const auto taught=replace(renamed,"The entire first row stays as it was.","The whole first row remains unchanged.");write(fragment,taught);
  expect(reload() && p.active()->supportView()->draft.empty() && p.active()->supportView()->level==fm::SupportLevel::Practice,"Changed teaching starts a fresh preview at the selected support level");
  expect(p.active()->content().support->steps[0].teaching.find("whole first row")!=std::string::npos,"New teaching reaches the question owner");
  auto changed=replace(renamed,"[2, -1 | 0]","[2, -1 | 1]");
  for(const auto& [from,to]:std::array{std::pair{"[1, 1 | 3]","[1, 1 | 5]"},std::pair{"[0, -3 | -6]","[0, -3 | -9]"},std::pair{"[0, 1 | 2]","[0, 1 | 3]"},std::pair{"[1, 0 | 1]","[1, 0 | 2]"}})
    while(changed.find(from)!=changed.npos)changed=replace(changed,from,to);
  write(fragment,changed);expect(reload() && p.questions()[*p.selected()].stamp!=stamp,"Valid changed mathematics gets fresh working");
  for(std::size_t i=0;i<3;++i) {
    const auto& s=p.active()->content().steps[i];
    expect(p.dispatch(support(p,fm::SupportAction::Choose,{},s.options[fm::firstAcceptedOption(s)].id.value)),"New matrix still solves through the existing choice route");
  }
  const auto completed=p.active()->supportView()->working;
  expect(completed.find('3')!=completed.npos && !p.active()->supportView()->verification.empty(),"Changed problem reaches checked completion");
  write(fragment,renamed);expect(reload() && p.active()->supportView()->working==working && p.active()->supportView()->draft==draft && p.active()->journal().size()==journal,"Reverting source recovers the exact earlier attempt and branch history");
  write(fragment,changed);expect(reload() && p.active()->currentRun().completed && p.active()->supportView()->working==completed,"Revisiting the changed source recovers its separate completed attempt");
  const auto finalQuestion=p.questions()[*p.selected()];
  std::filesystem::rename(entry,root/"renamed.paths.md");expect(reload() && p.active()->currentRun().completed,"Atomic editor renames do not lose question identity");
  std::filesystem::remove(root/"renamed.paths.md");expect(reload() && !p.active(),"Removed selection closes without binding to a different question");
  write(entry,"@include parts/matrix.inc.md\n");expect(reload(),"Restored entry returns to the catalogue");p.open(question(p.questions(),"document_matrix_question"));
  expect(p.active()->currentRun().completed,"Removed and restored question retains its attempt in memory");
  std::filesystem::create_symlink(fragment,root/"bad-link.md");expect(!tick() && !preview.report().accepted,"Symbolic source paths are rejected without replacing content");
  std::filesystem::remove(root/"bad-link.md");expect(reload() && preview.report().accepted,"Repairing a filesystem error recovers even when valid document bytes are unchanged");
  write(root/"notes.md",std::string(128*1024+1,'x'));expect(!tick() && !preview.report().accepted,"Live capture enforces the file byte bound");
  std::filesystem::remove(root/"notes.md");expect(reload(),"Bound failure can be repaired");
  CorpusPractice saved(base.bank);const auto save=folder/"protected-progress.json";saved.loadProgress(save);saved.open(0);saved.saveProgress();const auto savedBytes=read(save);
  auto protectedCorpus=base.corpus;LearningDocumentPreview refused(root,base.corpus,base.bank);
  expect(!refused.poll(protectedCorpus,saved) && !refused.report().accepted && refused.revision()==0,"Watcher cannot attach to personal persistence");saved.saveProgress(true);
  expect(read(save)==savedBytes && protectedCorpus.entries.size()==base.corpus.entries.size(),"Rejected attachment preserves both save bytes and current catalogue");
  CorpusPractice bounded({finalQuestion});bounded.open(0);
  for(unsigned i=0;i<32;++i) {
    auto q=finalQuestion;q.question.description+=" "+std::to_string(i);auto data=Json::parse(q.stamp);data["description"]=q.question.description;q.stamp=data.dump();bounded.replacePreview({q});
  }
  bool rejected=false;
  try{bounded.replacePreview({finalQuestion});}catch(const std::exception&){rejected=true;}
  // Restoring an archived revision frees its slot; another new revision must fail.
  expect(!rejected,"Restoring an older preview is allowed at the history bound");
  auto extra=finalQuestion;extra.question.description+=" Extra revision";
  auto extraJson=Json::parse(extra.stamp);extraJson["description"]=extra.question.description;extra.stamp=extraJson.dump();
  try{bounded.replacePreview({extra});}catch(const std::exception&){rejected=true;}
  expect(rejected && bounded.questions()[0].stamp==finalQuestion.stamp,"History bound rejects before replacing the current preview");
  std::cout<<"LIVE_DOCUMENT_PREVIEW {\"stable_bytes\":true,\"invalid_retained\":true,\"source_locations\":true,\"revision_attempts\":true,\"save_protected\":true,\"windows\":0}\n";
}
Json draftPreview(const Path& source,const Path& folder) {
  const auto root=folder/"draft";std::filesystem::copy(source,root,std::filesystem::copy_options::recursive);
  Fixture base;auto corpus=base.corpus;CorpusPractice p(base.bank);LearningDocumentPreview preview(root,base.corpus,base.bank);
  auto now=LearningDocumentPreview::Clock::time_point{};
  const auto tick=[&]{now+=std::chrono::milliseconds(250);return preview.poll(corpus,p,now);};
  const auto reload=[&]{expect(!tick(),"Draft edit waits for settled bytes");return tick();};
  expect(tick(),"Copied draft compiles through the live watcher");
  std::string id;Path file;const auto report=Json::parse(preview.report().reportJson);
  for(const auto& e:report["entities"])if(e["kind"]=="question") {
    const auto& q=p.questions()[question(p.questions(),e["id"])];
    if(q.question.support && !q.question.steps.front().hint.empty()){id=q.id;file=root/e["source"]["file"].get<std::string>();break;}
  }
  expect(!id.empty(),"Generated draft contains authored support");p.open(question(p.questions(),id));
  const auto q=p.active()->content();const auto original=read(file);
  const auto send=[&](fm::SupportAction action,unsigned value=0){expect(p.dispatch(support(p,action,{},value)),"Draft action reaches canonical session");};
  send(fm::SupportAction::SelectLevel,1);const auto& first=q.steps.front();const auto correct=fm::firstAcceptedOption(first);
  send(fm::SupportAction::Choose,first.options[(correct+1)%first.options.size()].id.value);
  send(fm::SupportAction::Choose,first.options[correct].id.value);send(fm::SupportAction::Undo);
  const auto journal=p.active()->journal().size();const auto working=p.active()->supportView()->working;
  const std::string hint="Draft check: compare both sides before choosing the operation.";
  const auto edited=replace(original,first.hint,hint);write(file,edited);
  expect(reload() && p.active()->supportView()->level==fm::SupportLevel::Practice,"Edited teaching starts a separate preview at the selected level");
  send(fm::SupportAction::ReadHelp,2);expect(p.active()->supportView()->reading==hint,"Saved Markdown reaches the actual Hint projection");
  send(fm::SupportAction::Choose,first.options[correct].id.value);
  const auto changedWorking=p.active()->supportView()->working;const auto changedJournal=p.active()->journal().size();
  write(file,replace(edited,"@template ","@template invalid_"));
  expect(!reload() && !preview.report().accepted,"Malformed draft is rejected");
  const auto diagnostic=Json::parse(preview.report().reportJson)["diagnostics"][0];
  expect(diagnostic["file"]==file.lexically_relative(root).generic_string() && diagnostic["line"].get<unsigned>()>0,"Draft rejection identifies the editable source line");
  expect(p.active()->supportView()->working==changedWorking && p.active()->journal().size()==changedJournal,"Rejected draft retains the usable working and attempt");
  write(file,edited);expect(reload(),"Correcting the draft clears the error");
  write(file,original);expect(reload(),"Restoring source reopens the original preview revision");
  expect(p.active()->journal().size()==journal && p.active()->supportView()->working==working,"Original choices and Undo branch return with their revision");
  for(const auto& step:q.steps)send(fm::SupportAction::Choose,step.options[fm::firstAcceptedOption(step)].id.value);
  expect(p.active()->currentRun().completed && !p.active()->supportView()->verification.empty(),"Restored draft solves to checked completion");
  expect(!p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Draft completion waits for Next");
  return {{"accepted",true},{"question_id",id},{"hint_reloaded",true},{"invalid_retained",true},
          {"revision_restored",true},{"completed",true},{"windows",0}};
}
Json referenceCard(const Path& source,const Path& folder) {
  Fixture f;const auto imported=f.load(source);expect(imported.accepted,imported.message);
  expect(imported.questions==1 && imported.lessons==1,"Reference contains one question and its neutral reading");
  const auto& q=f.bank[question(f.bank,"matrix_reference_fraction_01")];const auto& steps=q.question.steps;const auto& supportSteps=q.question.support->steps;
  expect(q.title=="Fractional solutions · Exercise 1","Reference title is a readable label, separate from stable identity");
  std::vector<std::string> after;for(const auto& s:supportSteps)after.push_back(fm::matrixEquationTex(*fm::parseAugmentedMatrix(s.equation).result));
  unsigned disclosureChecks=0;
  for(unsigned level=0;level<4;++level) {
    CorpusPractice p({q});const auto save=folder/("reference-help-"+std::to_string(level)+".json");p.loadProgress(save);p.open(0);
    const auto send=[&](fm::SupportAction action,unsigned value=0,std::string text={}){expect(p.dispatch(support(p,action,std::move(text),value)),"Reference action reaches its existing owner");};
    send(fm::SupportAction::SelectLevel,level);std::string working;
    for(std::size_t i=0;i<steps.size();++i) {
      const auto before=p.active()->supportView()->working;const auto submissions=p.active()->currentRun().support->submissions.size();
      auto view=*p.active()->supportView();
      expect(view.choices.empty()==(level>=2) && view.reading.empty()==(level!=0),"Support levels retain their response and disclosure contracts");
      if(level==0)expect(view.reading.find(supportSteps[i].definitions)!=view.reading.npos && view.reading.find(supportSteps[i].teaching)!=view.reading.npos,"Learn opens definitions and the current worked explanation together");
      if(level==3)expect(view.prompt.empty(),"Write does not reveal a step cue");
      send(fm::SupportAction::ReadHelp,1);expect(p.active()->supportView()->reading==supportSteps[i].definitions,"Terms contains only the current definitions");
      send(fm::SupportAction::ReadHelp,2);view=*p.active()->supportView();
      expect(!steps[i].hint.empty() && view.reading==steps[i].hint && view.reading!=supportSteps[i].teaching,"Hint uses the authored direction, never the worked paragraph");
      for(const auto& state:after)expect(view.reading.find(state)==view.reading.npos,"Hint does not contain a reached matrix");
      if(i==0) {
        p.saveProgress();CorpusPractice restored({q});restored.loadProgress(save);
        expect(restored.active() && restored.active()->supportView()->reading==steps[0].hint && (restored.active()->currentRun().support->exposure&6)==6,"Hint disclosure and exposure survive save/reopen");p=std::move(restored);
      }
      send(fm::SupportAction::ReadHelp,3);view=*p.active()->supportView();expect(view.reading.find(after[i])!=view.reading.npos,"Next line reveals the current reached matrix");
      for(std::size_t j=i+1;j<after.size();++j)expect(view.reading.find(after[j])==view.reading.npos,"Next line does not reveal later matrices");
      send(fm::SupportAction::ReadHelp,4);view=*p.active()->supportView();for(const auto& state:after)expect(view.reading.find(state)!=view.reading.npos,"Solution reveals the complete reference route");
      expect(view.working==before && p.active()->currentRun().support->submissions.size()==submissions,"Reading help never commits mathematical work");
      send(fm::SupportAction::ReadHelp,0);view=*p.active()->supportView();expect(view.assisted && view.reading.empty()==(level!=0),"Closing help restores the default level view while retaining exposure");
      disclosureChecks+=4;
      if(level<2)send(fm::SupportAction::Choose,steps[i].options[fm::firstAcceptedOption(steps[i])].id.value);
      else {working+=supportSteps[i].equation+"\n";send(fm::SupportAction::EditDraft,0,working);send(fm::SupportAction::CheckWork,0,working);}
    }
    expect(p.active()->currentRun().completed && !p.active()->supportView()->verification.empty(),"Reference completes at every support level");
    expect(!p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Reference completion waits for Next");
  }
  auto legacy=q;legacy.question.steps[0].hint.clear();CorpusPractice withoutHint({legacy});withoutHint.open(0);
  expect(withoutHint.dispatch(support(withoutHint,fm::SupportAction::SelectLevel,{},1)) && withoutHint.dispatch(support(withoutHint,fm::SupportAction::ReadHelp,{},2)),"Older documents without @hint remain playable");
  const auto fallback=withoutHint.active()->supportView()->reading;
  expect(!fallback.empty() && fallback!=supportSteps[0].teaching && fallback.find(after[0])==fallback.npos,"Missing hint uses a general direction instead of revealing worked teaching");
  const auto text=read(source/"reference.paths.md");const auto hintStart=text.find("@hint\n"),hintEnd=text.find("@wrong",hintStart);
  expect(hintStart!=text.npos && hintEnd!=text.npos,"Reference authors a separate hint passage");
  unsigned rejections=0;
  for(bool included:{false,true})for(unsigned defect=0;defect<4;++defect) {
    auto bad=text;
    if(defect<3)bad.replace(hintStart,hintEnd-hintStart,defect==0?"@hint\n\n":defect==1?"@hint "+std::string(8001,'x')+"\n":"@hint First direction.\n@hint Repeated direction.\n");
    else bad.insert(bad.find("@template lesson.v2")+std::string("@template lesson.v2").size(),"\n@hint Wrong scope.");
    const auto root=folder/("bad-reference-"+std::to_string(rejections++));
    if(included){write(root/"parts/card.inc.md",bad);write(root/"reference.paths.md","@include parts/card.inc.md\n");}else write(root/"reference.paths.md",bad);
    Fixture rejected;const auto result=rejected.load(root);expect(!result.accepted,"Bad hint format rejects atomically");
    const auto d=Json::parse(result.reportJson)["diagnostics"][0];expect(d["field"]=="hint" && d["file"]==(included?"parts/card.inc.md":"reference.paths.md") && d["line"].get<unsigned>()>0,"Hint rejection names the original directive, including includes");
  }
  const auto root=folder/"reference-preview";write(root/"reference.paths.md",text);Fixture seed;auto corpus=seed.corpus;CorpusPractice p(seed.bank);LearningDocumentPreview preview(root,seed.corpus,seed.bank);
  auto now=LearningDocumentPreview::Clock::time_point{};expect(preview.poll(corpus,p,now),"Reference opens through live preview");p.open(question(p.questions(),q.id));
  expect(p.dispatch(support(p,fm::SupportAction::ReadHelp,{},2)),"Reference hint opens in live preview");
  auto edited=text;edited.replace(hintStart,hintEnd-hintStart,"@hint Compare the signs before choosing a cancelling multiple.\n");write(root/"reference.paths.md",edited);
  now+=std::chrono::milliseconds(250);expect(!preview.poll(corpus,p,now),"Hint edit waits for stable source bytes");now+=std::chrono::milliseconds(250);
  expect(preview.poll(corpus,p,now) && p.dispatch(support(p,fm::SupportAction::ReadHelp,{},2)) && p.active()->supportView()->reading=="Compare the signs before choosing a cancelling multiple.","Markdown hint edit reaches the live help projection");
  return {{"accepted",true},{"question_id",q.id},{"title",q.title},{"support_levels",4},{"disclosure_checks",disclosureChecks},{"hint_format_rejections",rejections},{"hint_save_replay",true},{"live_hint_edit",true},{"question",Json::parse(q.stamp)},{"windows",0}};
}
void matrixDiagnostics(const Path& folder) {
  const auto original=read(Path(DOCUMENT_FIXTURE)/"matrix.paths.md");
  struct Case {std::string from,to,field,reason;};
  const std::array cases{
    Case{"@after [1, 1 | 3] [0, -3 | -6]","@after [1, 1 | 3] [0, -3 | -5]","after","accepted choice"},
    Case{"@choice 13 | -1","@choice 13 | -4/2","choice","duplicates"},
    Case{"@choice 23 | -3","@choice 23 | 0","choice","nonzero"},
    Case{"@given [1, 1 | 3] [2, -1 | 0]","@given [1, 1 | 3] [2, 2 | 6]","given","unique solution"},
    Case{"@after [1, 1 | 3] [0, -3 | -6]","@after [1, 1 | 3] [0, -3 | bad]","after","matrix.v1 expects"}};
  unsigned checks=0;
  for(bool included:{false,true})for(const auto& c:cases) {
    const auto root=folder/("diagnostic-"+std::to_string(checks++));const auto changed=replace(original,c.from,c.to);
    const auto split=changed.find("@question ");const auto text=included?changed.substr(split):changed;
    const auto name=included?"shared/card.inc.md":"matrix.paths.md";
    if(included)write(root/"matrix.paths.md",changed.substr(0,split)+"@include shared/card.inc.md\n");
    write(root/name,text);Fixture f;const auto size=f.bank.size(),entries=f.corpus.entries.size();const auto result=f.load(root);
    expect(!result.accepted && f.bank.size()==size && f.corpus.entries.size()==entries,"Bad matrix imports remain atomic");
    const auto d=Json::parse(result.reportJson)["diagnostics"][0];const auto pos=text.find(c.to);
    expect(pos!=text.npos && d["file"]==name && d["line"]==std::count(text.begin(),text.begin()+pos,'\n')+1 && d["field"]==c.field,
      "Mathematical rejection names the original field and line, including fragments: "+d.dump());
    expect(d["message"].get<std::string>().find(c.reason)!=std::string::npos,"Matrix rejection explains the actual reason: "+d.dump());
  }
  std::cout<<"MATRIX_DIAGNOSTICS "<<checks<<" direct/include failures located exactly\n";
}
void questionBatch(const Path& root,const Path& folder) {
  Fixture f;const auto first=f.bank.size();const auto loaded=f.load(root);expect(loaded.accepted,loaded.message);
  expect(f.bank.size()>first,"Batch must add questions");Json ids=Json::array();std::size_t routes=0,wrongs=0,disclosures=0;
  for(std::size_t n=first;n<f.bank.size();++n) {
    const auto& q=f.bank[n];expect(q.question.support.has_value(),"Batch uses an existing supported checker");ids.push_back(q.id);
    for(unsigned route=0;route<5;++route) {
      const auto level=route==4?1U:route;CorpusPractice p({q});const auto save=folder/(q.id+"-"+std::to_string(route)+".json");p.loadProgress(save);p.open(0);
      const auto send=[&](fm::SupportAction a,std::string text={},unsigned value=0){expect(p.dispatch(support(p,a,std::move(text),value)),"Batch response reaches its canonical owner");};
      send(fm::SupportAction::SelectLevel,{},level);
      expect(p.active()->supportView()->reading.empty()==(level!=0),"Batch teaching respects the selected support level");
      const auto help=[&](std::size_t i) {
        const auto& step=q.question.steps[i];if(step.hint.empty())return;
        const auto& taught=q.question.support->steps[i];const auto before=p.active()->supportView()->working;
        send(fm::SupportAction::ReadHelp,{},1);expect(p.active()->supportView()->reading==taught.definitions,"Generated Terms stays separate");
        send(fm::SupportAction::ReadHelp,{},2);expect(p.active()->supportView()->reading==step.hint && step.hint!=taught.teaching,"Generated Hint uses its direction");
        send(fm::SupportAction::ReadHelp,{},3);expect(p.active()->supportView()->reading.find(q.question.workingStates[i+1].display)!=std::string::npos,"Generated Next line reaches its authored state");
        send(fm::SupportAction::ReadHelp,{},4);for(std::size_t j=1;j<q.question.workingStates.size();++j)expect(p.active()->supportView()->reading.find(q.question.workingStates[j].display)!=std::string::npos,"Generated Solution reveals the full route");
        send(fm::SupportAction::ReadHelp,{},0);expect(p.active()->supportView()->working==before && p.active()->supportView()->reading.empty()==(level!=0),"Generated help closes without committing work");disclosures+=4;
      };
      if(level<2)for(std::size_t i=0;i<q.question.steps.size();++i) {
        help(i);
        const auto& step=q.question.steps[i];const auto correct=fm::firstAcceptedOption(step);const auto before=p.active()->supportView()->working;
        const auto answer=[&](std::size_t j){if(route==4){const auto value=q.question.support->steps[i].responses[j];send(fm::SupportAction::EditDraft,value);send(fm::SupportAction::SubmitBlank,value);}else send(fm::SupportAction::Choose,{},step.options[j].id.value);};
        for(std::size_t j=0;j<step.options.size();++j)if(j!=correct) {
          answer(j);expect(p.active()->supportView()->status==fm::WrittenCheckStatus::Incorrect && p.active()->supportView()->working==before,"Wrong generated tile retains working");++wrongs;
        }
        answer(correct);
        if(route==1 && i==0) {
          const auto count=p.active()->currentRun().support->nodes.size();send(fm::SupportAction::Undo);answer(correct);
          expect(p.active()->currentRun().support->nodes.size()==count+1,"Batch Undo retains its earlier branch");p.saveProgress();
          CorpusPractice resumed({q});resumed.loadProgress(save);expect(resumed.active() && resumed.active()->supportView()->working==p.active()->supportView()->working,"Generated Practice resumes checked working");p=std::move(resumed);
        }
      } else {
        help(0);
        expect(p.active()->supportView()->choices.empty(),"Written batch levels do not disclose choices");std::string work;
        for(const auto& step:q.question.support->steps)work+=step.equation+"\n";
        send(fm::SupportAction::EditDraft,work);send(fm::SupportAction::CheckWork,work);
      }
      expect(p.active()->currentRun().completed && !p.active()->supportView()->verification.empty(),"Every generated route verifies the original problem");
      expect(!p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Generated completion waits for Next");p.saveProgress();
      CorpusPractice restored({q});restored.loadProgress(save);expect(restored.active() && restored.active()->currentRun().completed,"Generated completed state survives reopen");++routes;
    }
  }
  std::cout<<Json{{"accepted",true},{"question_ids",ids},{"routes",routes},{"wrong_choices",wrongs},{"disclosure_checks",disclosures},{"save_replay",true},{"windows",0}}.dump()<<'\n';
}
void questionBatchUpgrade(const Path& before,const Path& after,const Path& folder) {
  Fixture old;const auto first=old.bank.size();expect(old.load(before).accepted,"Original batch imports");
  expect(old.bank.size()>=first+2,"Upgrade needs two original questions");CorpusPractice p(old.bank);
  const auto save=folder/"upgrade.json";p.loadProgress(save);p.open(first);
  for(const auto& step:old.bank[first].question.steps)expect(p.dispatch(support(p,fm::SupportAction::Choose,{},step.options[fm::firstAcceptedOption(step)].id.value)),"Complete original question");
  p.open(first+1);expect(p.dispatch(support(p,fm::SupportAction::SelectLevel,{},1)),"Original Practice opens");
  const auto& step=old.bank[first+1].question.steps[0];const auto correct=fm::firstAcceptedOption(step);
  expect(p.dispatch(support(p,fm::SupportAction::Choose,{},step.options[(correct+1)%step.options.size()].id.value)),"Original wrong response retained");
  expect(p.dispatch(support(p,fm::SupportAction::Choose,{},step.options[correct].id.value)) && p.dispatch(support(p,fm::SupportAction::Undo)) && p.dispatch(support(p,fm::SupportAction::Choose,{},step.options[correct].id.value)),"Original Undo branch retained");
  expect(p.dispatch(support(p,fm::SupportAction::EditDraft,"unfinished fraction")) && p.dispatch(support(p,fm::SupportAction::ReadHelp,{},1)),"Original draft and help retained");p.saveProgress();
  Fixture next;expect(next.load(after).accepted,"Updated batch imports");
  for(const auto& q:old.bank)expect(next.bank[question(next.bank,q.id)].stamp==q.stamp,"Upgrade preserves every old question stamp");
  CorpusPractice restored(next.bank);restored.loadProgress(save);
  expect(restored.active() && restored.questions()[*restored.selected()].id==old.bank[first+1].id,"Upgrade reopens the original selected question");
  const auto a=p.active()->supportView(),b=restored.active()->supportView();
  expect(a->working==b->working && a->draft==b->draft && a->reading==b->reading && a->level==b->level && p.active()->journal().size()==restored.active()->journal().size() && p.active()->currentRun().support->nodes.size()==restored.active()->currentRun().support->nodes.size(),"Upgrade retains working, draft, help, level and branch journal");
  expect(restored.attempt(question(next.bank,old.bank[first].id))->currentRun().completed,"Completed original still waits for Next");
  std::cout<<Json{{"accepted",true},{"save_replay",true},{"old_questions",old.bank.size()-first},{"new_questions",next.bank.size()-old.bank.size()},{"windows",0}}.dump()<<'\n';
}
void imports(const Path& root) {
  Fixture f;const auto report=f.load(root);expect(report.accepted,report.message);
  const auto details=Json::parse(report.reportJson);
  expect(details["accepted"]==true && details["files"].size()==5 && details["entry_documents"].size()==4 && details["entities"].size()==16,"Compiler reports the exact entry/include closure and declarations");
  const auto& inventory=details["files"];
  const auto equality=std::find_if(inventory.begin(),inventory.end(),[](const auto& row){return row["path"]=="shared/equality.inc.md";});
  expect(equality!=inventory.end() && (*equality)["sha256"]=="4b7e51e8110936eac505b1b2d587845aba0c1f3488f2aa785eaf015dcb9a4fae","C++ inventory hash agrees with the independent published SHA-256 value");
  expect(report.files==4 && report.lessons==4 && report.questions==4,"Every document supplies its lesson and workflow");
  expect(f.corpus.subjects.size()==8 && f.corpus.topics.size()==182 && f.corpus.entries.size()==934,"Documents appear in the existing ToC");
  const auto& algebra=lesson(f.corpus,"document_balance_reading");expect(algebra.body.find("adding it back reverses")!=std::string::npos,"Shared prose include expands into the real reader");
  expect(algebra.source=="algebra.paths.md" && algebra.firstLine>0 && algebra.lastLine>algebra.firstLine,"Reader retains document provenance");
  const auto& physics=lesson(f.corpus,"document_oscillation_reading");expect(physics.figure.has_value(),"A diagram command resolves to a registered model");
  auto figure=instantiateDocumentFigure(*physics.figure);
  expect(figure.snapshot().kind==MathObjectKind::Harmonics && figure.snapshot().partCount>0 && figure.snapshot().level==0,"Document constructs existing live geometry");
  expect(figure.parameter(MathParameter::Amplitude1)==1 && figure.parameter(MathParameter::HarmonicTime)==0,"Authored diagram parameters reach the canonical owner");
  expect(std::sin(figure.parameter(MathParameter::HarmonicTime))*figure.parameter(MathParameter::Amplitude1)==0,"Physics example independently agrees with the configured sine projection");
  const auto& bio=f.bank[question(f.bank,"document_membrane_question")].question;
  expect(bio.steps[0].options[1].label=="\\text{Cell membrane}","Plain text choices are formatted automatically");
  const auto caps=Json::parse(learningDocumentCapabilities());expect(caps["templates"].size()==5 && caps["figures"].size()==mathObjectSpecs().size() && caps["matrix_template"]["operations"].size()==4,"Capability listing comes from the live template and diagram registries");
  routes(f);
}
void persistence(const Path& root,const Path& folder) {
  Fixture f;expect(f.load(root).accepted,"Initial import accepted");CorpusPractice p(f.bank);const auto save=folder/"progress.json";p.loadProgress(save);
  p.open(question(f.bank,"document_balance_question"));
  expect(p.dispatch(support(p,fm::SupportAction::SelectLevel,{},3)),"Choose written level");
  expect(p.dispatch(support(p,fm::SupportAction::EditDraft,"4x=20\nx=")),"Partial draft is entered");p.saveProgress();const auto bytes=read(save);
  std::filesystem::rename(root/"algebra.paths.md",root/"z_algebra.paths.md");
  Fixture renamed;expect(renamed.load(root).accepted,"Renamed source reimports");
  expect(f.bank[question(f.bank,"document_balance_question")].stamp==renamed.bank[question(renamed.bank,"document_balance_question")].stamp,"Document filenames and import order do not change question identity");
  CorpusPractice restored(renamed.bank);restored.loadProgress(save);
  expect(restored.active() && restored.active()->supportView()->draft=="4x=20\nx=" && restored.active()->supportView()->level==fm::SupportLevel::Independent,"Draft and mode restore after document reordering");
  restored.saveProgress();expect(read(save)==bytes,"Unchanged restored progress is not rewritten");
  write(root/"new.paths.md","@paths 1\n@subject physics | Physics\n@chapter document_oscillations | Oscillations from documents\n@lesson document_extra_reading | Another reading\n@template lesson.v1\nA new reading can be added without editing any program code.\n@end\n");
  Fixture added;expect(added.load(root).accepted && added.corpus.entries.size()==935,"New file is discovered without a build or manifest edit");
  CorpusPractice afterAddition(added.bank);afterAddition.loadProgress(save);expect(afterAddition.active()->supportView()->draft=="4x=20\nx=","Added content retains the saved question");
  auto altered=read(root/"z_algebra.paths.md");altered=replace(altered,"@why Adding 3", "@why Reviewed: adding 3");write(root/"z_algebra.paths.md",altered);
  Fixture changed;expect(changed.load(root).accepted,"Valid revised document imports");CorpusPractice incompatible(changed.bank);incompatible.loadProgress(save);
  incompatible.open(0);incompatible.saveProgress();expect(read(save)==bytes && incompatible.message().find("Original save retained")!=std::string::npos,"Frozen support content changes retain the original save");
}
void failures(const Path& root) {
  const auto algebra=read(root/"algebra.paths.md"),biology=read(root/"biology.paths.md");
  const std::vector<std::tuple<std::string,std::string,std::string>> bad{
    {"biology.paths.md","@template choices.v1","@template choices.v9"},
    {"biology.paths.md","@answer 12","@answer 99"},
    {"biology.paths.md","@textchoice 13","@textchoice 12"},
    {"biology.paths.md","@read document_membrane_reading","@read missing_reading"},
    {"biology.paths.md","@practice document_membrane_question","@practice missing_question"},
    {"biology.paths.md","@goal Which","@execute Which"},
    {"physics.paths.md","@figure harmonics 0","@figure missing_provider 0"},
    {"physics.paths.md","@figure harmonics 0","@figure harmonics 999"},
    {"physics.paths.md","@parameter amplitude1 1","@parameter amplitude1 99"},
    {"physics.paths.md","@parameter amplitude1 1","@parameter unknown 1"},
    {"physics.paths.md","@parameter amplitude1 1","@parameter amplitude1 nan"},
    {"physics.paths.md","@parameter amplitude1 1","@parameter amplitude1 1\n@parameter amplitude1 1"},
    {"algebra.paths.md","@after 4x=20","@after 4x=21"},
    {"algebra.paths.md","@include shared/equality.inc.md","@include ../outside.md"},
    {"algebra.paths.md","@include shared/equality.inc.md","@include shared/missing.inc.md"},
    {"algebra.paths.md","@paths 1","@paths 9"},
    {"algebra.paths.md","@domain x is real.","@domain"},
    {"matrix.paths.md","@operation add_row_1_to_2","@operation unknown"},
    {"matrix.paths.md","@operation add_row_1_to_2","@operation swap_rows"},
    {"matrix.paths.md","@operation add_row_1_to_2","@operation add_row_2_to_1"},
    {"matrix.paths.md","@operation add_row_1_to_2",""},
    {"matrix.paths.md","@operation divide_row_2","@operation divide_row_2\n@operation divide_row_2"},
    {"matrix.paths.md","@after [1, 1 | 3] [0, -3 | -6]","@after [1, 1 | 3] [0, -3 | -5]"},
    {"matrix.paths.md","@choice 23 | -3","@choice 23 | 0"},
    {"matrix.paths.md","@choice 11 | 2","@choice 11 | -4/2"},
    {"matrix.paths.md","@given [1, 1 | 3] [2, -1 | 0]","@given [1, 1 | 3] [2, 2 | 6]"},
    {"matrix.paths.md","@template matrix.v1","@template linear.v1"}};
  for(const auto& [file,from,to]:bad) {
    const auto original=read(root/file);
    write(root/file,replace(original,from,to));Fixture f;const auto report=f.load(root);
    expect(!report.accepted && report.message.find(file)!=std::string::npos,"Bad source reports its filename: "+report.message);
    expect(f.corpus.entries.size()==930 && f.corpus.subjects.size()==6 && f.bank.size()==278,"Rejected folder commits no partial ToC or question changes");
    write(root/file,original);
  }
  write(root/"duplicate.paths.md",biology);Fixture duplicate;expect(!duplicate.load(root).accepted,"Duplicate lesson/question identities cannot replace content");std::filesystem::remove(root/"duplicate.paths.md");
  write(root/"shared/a.inc.md","@include b.inc.md\n");write(root/"shared/b.inc.md","@include a.inc.md\n");
  write(root/"algebra.paths.md",replace(algebra,"@include shared/equality.inc.md","@include shared/a.inc.md"));Fixture cycle;expect(!cycle.load(root).accepted,"Include cycles fail boundedly");write(root/"algebra.paths.md",algebra);
  write(root/"large.paths.md",std::string(128*1024+1,'x'));Fixture large;expect(!large.load(root).accepted,"File byte limit is enforced");std::filesystem::remove(root/"large.paths.md");
  std::filesystem::create_symlink(root/"shared/equality.inc.md",root/"linked.inc.md");Fixture symlink;expect(!symlink.load(root).accepted,"Symlink imports are rejected");std::filesystem::remove(root/"linked.inc.md");
  const auto literal=replace(algebra,"The goal is to isolate", "```text\n@include missing.inc.md\n@execute literal data\n```\n\nThe goal is to isolate");
  write(root/"algebra.paths.md",literal);Fixture fenced;expect(fenced.load(root).accepted,"Code fences keep commands literal");
  expect(lesson(fenced.corpus,"document_balance_reading").body.find("@execute literal data")!=std::string::npos,"Literal source is preserved, never executed");write(root/"algebra.paths.md",algebra);
  Fixture missing;expect(!missing.load(root/"not_here").accepted,"A mistyped document folder is reported");
  const auto empty=root/"empty";std::filesystem::create_directory(empty);Fixture noDocs;expect(noDocs.load(empty).accepted && noDocs.bank.size()==278,"An empty folder preserves the bundled app");
}
void matrices(const Path& root,const Path& folder) {
  Fixture f;expect(f.load(root).accepted,"Matrix document imports");
  const auto index=question(f.bank,"document_matrix_question");const auto& base=f.bank[index];
  const auto original=*fm::parseAugmentedMatrix(base.question.support->equation).result;
  expect(fm::checkMatrixResponse(original,original,fm::MathOperation::DivideRow2,"0").status==fm::WrittenCheckStatus::Incorrect,"Zero row divisor is a mathematical error");
  expect(fm::checkMatrixResponse(original,original,fm::MathOperation::AddRow1ToRow2,"1000000000").status==fm::WrittenCheckStatus::Unsupported,"Exact arithmetic overflow is not mislabelled a wrong solution");
  expect(fm::checkMatrixWork(original,"[1/0, 0 | 1] [0, 1 | 2]").status==fm::WrittenCheckStatus::Incorrect,"Division by zero in written entries is rejected");
  expect(fm::checkMatrixWork(original,"[1, 0 | 1] [0, 1 | 2] trailing").status==fm::WrittenCheckStatus::Unsupported,"Trailing unimplemented syntax cannot be ignored");
  expect(fm::checkMatrixWork(original,std::string(33,'\n')).status==fm::WrittenCheckStatus::Unsupported &&
    fm::checkMatrixWork(original,std::string(8193,'x')).status==fm::WrittenCheckStatus::Unsupported,"Written matrix limits are bounded");
  const auto send=[](CorpusPractice& p,fm::SupportAction action,std::string text={},unsigned value=0) {
    expect(p.dispatch(support(p,action,std::move(text),value)),"Supported matrix command accepted");
  };
  const auto submit=[&](CorpusPractice& p,const std::string& text) {
    send(p,fm::SupportAction::EditDraft,text);send(p,fm::SupportAction::CheckWork,text);
  };
  const auto number=[](int numerator) {
    const auto g=std::gcd(numerator,3);return std::to_string(numerator/g)+(3/g==1?"":"/"+std::to_string(3/g));
  };
  std::size_t routes=0,fractional=0;
  for(int a:{-3,-1,1,4})for(int b:{-2,0,2,5}) {
    // Independent family: x=(a+b)/3, y=(2a-b)/3. No runtime solver generates these expectations.
    const std::array states{
      "[1, 1 | "+std::to_string(a)+"] [2, -1 | "+std::to_string(b)+"]",
      "[1, 1 | "+std::to_string(a)+"] [0, -3 | "+std::to_string(b-2*a)+"]",
      "[1, 1 | "+std::to_string(a)+"] [0, 1 | "+number(2*a-b)+"]",
      "[1, 0 | "+number(a+b)+"] [0, 1 | "+number(2*a-b)+"]"};
    auto authored=Json::parse(base.stamp);authored["support"]["equation"]=states[0];
    for(std::size_t i=0;i<states.size();++i) {
      const auto matrix=*fm::parseAugmentedMatrix(states[i]).result;
      authored["working_states"][i]["display"]=fm::matrixEquationTex(matrix);
      if(!i)authored["equation"]=fm::matrixEquationTex(matrix);
      else {
        authored["support"]["steps"][i-1]["equation"]=states[i];
        authored["steps"][i-1]["explanation"]="Apply the same row operation to all three entries.";
        authored["support"]["steps"][i-1]["teaching"]="Use the chosen operation across the complete row, including the constant.";
      }
    }
    auto varied=base;varied.stamp=authored.dump();varied.question=parseQuestionContent(varied.stamp,"matrix-variant-test");
    fractional+=(a+b)%3!=0 || (2*a-b)%3!=0;
    for(unsigned route=0;route<5;++route) {
      const auto level=route==4?1U:route;
      CorpusPractice p({varied});p.open(0);send(p,fm::SupportAction::SelectLevel,{},level);
      expect(p.active()->supportView()->working.empty(),"Matrix working starts blank");
      if(level<2)for(std::size_t step=0;step<3;++step) {
        const auto& q=p.active()->content();const auto correct=fm::firstAcceptedOption(q.steps[step]);
        const auto before=p.active()->supportView()->working;
        if(!level || route==4) {
          expect(p.active()->supportView()->reading.empty()==(level==1) && !p.active()->supportView()->choices.empty(),"Matrix Practice offers symbols with teaching closed; Learn opens teaching");
          send(p,fm::SupportAction::Choose,{},q.steps[step].options[(correct+1)%3].id.value);
          expect(p.active()->supportView()->status==fm::WrittenCheckStatus::Incorrect && p.active()->supportView()->working==before,"Wrong row multiplier preserves working");
          send(p,fm::SupportAction::Choose,{},q.steps[step].options[correct].id.value);
        } else {
          expect(p.active()->supportView()->reading.empty() && !p.active()->supportView()->responseCue.empty(),"Practice presents a symbolic blank without opening teaching");
          const auto text=q.support->steps[step].responses[correct];send(p,fm::SupportAction::EditDraft,text);send(p,fm::SupportAction::SubmitBlank,text);
        }
      } else {
        expect(p.active()->supportView()->reading.empty() && p.active()->supportView()->choices.empty() && p.active()->supportView()->responseCue.empty(),"Written levels do not disclose prepared steps");
        submit(p,states[1]+"\n"+states[2]+"\n"+states[3]);
      }
      expect(p.active()->currentRun().completed && !p.active()->supportView()->verification.empty(),"Every matrix level verifies the original equations");
      if(route==4) {
        const auto& events=p.active()->currentRun().support->submissions;
        expect(events.size()==6 && std::all_of(events.begin(),events.end(),[](const auto& e){return e.action==fm::SupportAction::Choose && e.level==fm::SupportLevel::Practice;}),"All wrong and correct matrix tiles retain Practice choice evidence");
        const auto path=folder/"matrix-choices.json";p.loadProgress(path);p.saveProgress();
        CorpusPractice restored({varied});restored.loadProgress(path);
        expect(restored.active() && restored.active()->currentRun().completed && restored.active()->currentRun().support->submissions.size()==6 &&
          restored.active()->review()->wrongAttempts==3 && restored.active()->supportView()->reading.empty(),"Every matrix Practice route reopens completed with its attempts and no teaching leak");
        std::filesystem::remove(path);
      }
      const auto& final=std::get<fm::AugmentedMatrix>(p.active()->currentRun().support->nodes.back().equation);
      const auto x=final.rows[0][2],y=final.rows[1][2];
      expect(3*x.numerator==(a+b)*x.denominator && 3*y.numerator==(2*a-b)*y.denominator,"Final values independently match exact expected fractions");
      expect(!p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Matrix completion waits for navigation");++routes;
    }
  }
  CorpusPractice p(f.bank);const auto save=folder/"matrix-progress.json";p.loadProgress(save);p.open(index);send(p,fm::SupportAction::SelectLevel,{},3);
  expect(!p.dispatch(fm::LayeredQuestionCommand::submitOption({12})),"Prepared answer route cannot bypass the matrix checker");
  const auto wrongs=[&]{return p.active()->review()->wrongAttempts;};
  for(const auto& text:{"[1, 1 | 3] [2, 2 | 6]","[1, 1 | 3] [0, -3 | -5]"}) {
    submit(p,text);expect(p.active()->supportView()->working.empty() && p.active()->supportView()->status==fm::WrittenCheckStatus::Incorrect,"Changed or nonunique solution set is rejected atomically");
  }
  auto prior=wrongs();submit(p,"R2 = R2 - 2R1");expect(wrongs()==prior && p.active()->supportView()->status==fm::WrittenCheckStatus::Unsupported,"Unimplemented notation is retained, not marked wrong mathematics");
  submit(p,"[1, 1 | 3] [0, -3 | -6]");const auto working=p.active()->supportView()->working;
  submit(p,"[1, 1 | 3] [0, -3 | -6]\n[1, 1 | 3] [0, 1 | 3]\n[1, 0 | 1] [0, 1 | 2]");
  expect(p.active()->supportView()->working==working,"Wrong middle line commits none of a matrix batch");
  submit(p,"[2, -1 | 0] [1, 1 | 3]");send(p,fm::SupportAction::SelectLevel,{},0);
  expect(!p.active()->supportView()->canRespond && p.active()->supportView()->choices.empty(),"Alternate written route never borrows a mismatched guided step");
  send(p,fm::SupportAction::SelectLevel,{},3);send(p,fm::SupportAction::ReadHelp,{},4);
  expect(p.active()->supportView()->reading.find("Reference solution")!=std::string::npos,"Explicit help opens the matrix solution");send(p,fm::SupportAction::ReadHelp,{},0);
  submit(p,"[1, 0 | 2/2] [0, 1 | 4/2]");expect(p.active()->currentRun().completed,"Equivalent fractions complete an alternative route");
  const auto nodes=p.active()->currentRun().support->nodes.size();send(p,fm::SupportAction::Undo);
  expect(!p.active()->currentRun().completed && p.active()->currentRun().support->nodes.size()==nodes,"Undo retains the completed matrix branch");
  send(p,fm::SupportAction::EditDraft,"[1, 0 | 1] [0, 1 | ");p.saveProgress();const auto bytes=read(save);
  std::filesystem::rename(root/"matrix.paths.md",root/"z_matrix.paths.md");Fixture renamed;expect(renamed.load(root).accepted,"Matrix source renames safely");
  CorpusPractice restored(renamed.bank);restored.loadProgress(save);
  expect(restored.active() && restored.active()->supportView()->draft=="[1, 0 | 1] [0, 1 | " && restored.active()->supportView()->assisted && restored.active()->currentRun().support->nodes.size()==nodes,"Matrix drafts, exposure and Undo branches survive save replay and reordering");
  restored.saveProgress();expect(read(save)==bytes,"Reading unchanged matrix progress does not rewrite it");
  const auto file=root/"z_matrix.paths.md";const auto source=read(file);write(file,replace(source,"@why Adding -2","@why Reviewed: adding -2"));
  Fixture changed;expect(changed.load(root).accepted,"Valid matrix revision imports");CorpusPractice incompatible(changed.bank);incompatible.loadProgress(save);incompatible.open(0);incompatible.saveProgress();
  expect(read(save)==bytes && incompatible.message().find("Original save retained")!=std::string::npos,"Changed frozen matrix content protects the original save");write(file,source);
  std::filesystem::rename(file,root/"matrix.paths.md");
  std::cout<<"MATRIX_SUPPORT {\"model_routes\":"<<routes<<",\"parameter_sets\":16,\"fractional_answer_sets\":"<<fractional<<",\"saved_matrix_draft_and_branches\":true,\"captures\":0}\n";
}
void published(const Path& store,const Path& folder) {
  auto corpus=loadMathCorpus(CORPUS_FIXTURE);
  const auto builtins=[&]{
    auto bank=loadCorpusStarters(STARTER_FIXTURE,corpus);
    for(const auto* name:{"matrix_reasoning.json","linear_support.json"}) {
      auto next=loadCorpusStarters(Path(CORPUS_FIXTURE).parent_path()/name,corpus);
      bank.insert(bank.end(),std::make_move_iterator(next.begin()),std::make_move_iterator(next.end()));
    }return bank;
  };
  auto before=builtins();auto initial=corpus;expect(importLearningDocuments(DOCUMENT_FIXTURE,initial,before).accepted,"Current documents import before publication");
  CorpusPractice practice(before);const auto save=folder/"published-progress.json";practice.loadProgress(save);
  practice.open(question(before,"document_matrix_question"));
  const auto send=[&](fm::SupportAction action,std::string text={},unsigned value=0){expect(practice.dispatch(support(practice,action,std::move(text),value)),"Existing published-boundary action accepted");};
  send(fm::SupportAction::SelectLevel,{},3);
  send(fm::SupportAction::EditDraft,"[1, 0 | 1] [0, 1 | 2]");
  send(fm::SupportAction::CheckWork,"[1, 0 | 1] [0, 1 | 2]");send(fm::SupportAction::Undo);
  send(fm::SupportAction::ReadHelp,{},4);send(fm::SupportAction::ReadHelp,{},0);
  send(fm::SupportAction::EditDraft,"[1, 0 | 1] [0, 1 | ");practice.saveProgress();const auto bytes=read(save);
  const auto nodes=practice.active()->currentRun().support->nodes.size();
  auto after=builtins();const auto report=importLearningStore(store,corpus,after);expect(report.accepted,report.message);
  expect(after.size()>before.size(),"Published chapter adds a question to the retained library");
  for(const auto& q:before)expect(after[question(after,q.id)].stamp==q.stamp,"All original frozen question stamps survive publication");
  CorpusPractice restored(after);restored.loadProgress(save);
  expect(restored.active() && restored.active()->supportView()->draft=="[1, 0 | 1] [0, 1 | " && restored.active()->supportView()->assisted && restored.active()->currentRun().support->nodes.size()==nodes,"Published library restores original drafts, help exposure and Undo branches");
  restored.saveProgress();expect(read(save)==bytes,"Publication replay preserves original save bytes");
  restored.open(question(after,"published_matrix_question"));
  expect(restored.dispatch(support(restored,fm::SupportAction::SelectLevel,{},3)),"New exported question uses the existing support owner");
  expect(restored.dispatch(support(restored,fm::SupportAction::EditDraft,"[1, 1 | 5] [0, -3 | -9]\n[1, 1 | 5] [0, 1 | 3]\n[1, 0 | 2] [0, 1 | 3]")),"Published matrix draft retained before checking");
  expect(restored.dispatch(support(restored,fm::SupportAction::CheckWork,"[1, 1 | 5] [0, -3 | -9]\n[1, 1 | 5] [0, 1 | 3]\n[1, 0 | 2] [0, 1 | 3]")),"Published matrix route accepted");
  const auto& final=*restored.active()->currentRun().support;
  const auto& matrix=std::get<fm::AugmentedMatrix>(final.nodes[final.active].equation);
  const auto x=matrix.rows[0][2],y=matrix.rows[1][2];
  expect(restored.active()->currentRun().completed && x.numerator*y.denominator+y.numerator*x.denominator==5*x.denominator*y.denominator
    && 2*x.numerator*y.denominator-y.numerator*x.denominator==x.denominator*y.denominator,"Actual published answer independently satisfies both original equations");
  const auto broken=folder/"broken-store";std::filesystem::copy(store,broken,std::filesystem::copy_options::recursive);
  const auto active=Json::parse(read(broken/"active.json"));const auto file=broken/"generations"/active["generation"].get<std::string>()/"documents/matrix_foundations/chapter.paths.md";
  write(file,read(file)+"\nChanged after publication.\n");
  const auto size=corpus.entries.size(),count=after.size();const auto rejected=importLearningStore(broken,corpus,after);
  expect(!rejected.accepted && corpus.entries.size()==size && after.size()==count && read(save)==bytes,"Changed published bytes commit no catalogue or progress changes");
  std::cout<<"PUBLISHED_DOCUMENTS {\"saved_draft_restored\":true,\"undo_branches_retained\":true,\"original_save_unchanged\":true,\"new_question_solved\":true,\"changed_bytes_rejected\":true,\"captures\":0}\n";
}
void bookDocuments(const Path& folder) {
  const auto root=folder/"book-format";std::filesystem::create_directory(root);
  const std::string text=R"(@paths 1
@subject algebra | Algebra
@chapter imported_book_test | Book test
@lesson imported_book_reading | A structured lesson
@template lesson.v2
@block definition | terms | 1.1 | Equality
@prose Two expressions have the same value.
@display eq1
x=x
@endblock
@block exercise | task | 1.2 | Try it
@prose State the unknown.
@help hint
@prose Look at the letter.
@help answer
@display
x
@help solution
@prose Hidden solution sentinel.
@reference terms | Recall equality
@endblock
@end
)";
  const auto file=root/"book.paths.md";write(file,text);Fixture f;const auto imported=f.load(root);expect(imported.accepted,imported.message);
  auto copied=f.corpus;f.corpus.entries.clear();const auto& e=lesson(copied,"imported_book_reading");
  expect(e.lesson.size()==2 && e.body.find("Hidden solution")==std::string::npos,"Copied dynamic lesson owns its strings and does not flatten solutions into reading references");
  auto view=bookLessonView(e.lesson,{});for(const auto& b:view)for(const auto& h:b.help)expect(!h.open && h.passages.empty(),"Imported help starts redacted");
  std::vector<std::uint8_t> masks{0,2};view=bookLessonView(e.lesson,masks);
  expect(view[1].help[1].open && !view[1].help[1].passages.empty() && view[1].help[2].passages.empty() && view[1].help[3].passages.empty(),"Hint is independent of answer and solution");
  expect(std::string(view[0].title)=="Equality" && view[1].references[0].target=="terms","Dynamic titles and reference targets survive source-corpus destruction");
  const std::string figureText="@figure harmonics 0\n@parameter amplitude1 1\n@caption Public teaching diagram.\n";
  const auto fragment=root/"shared/figure.inc.md";
  unsigned publicFigures=0,rejectedFigures=0;
  for(bool trailing:{false,true})for(bool included:{false,true}) {
    write(fragment,figureText);
    const auto declaration=included?"@include shared/figure.inc.md\n":figureText;
    write(file,trailing?replace(text,"@endblock\n@end\n","@endblock\n"+declaration+"@end\n"):
                        replace(text,"@template lesson.v2\n","@template lesson.v2\n"+declaration));
    Fixture valid;const auto result=valid.load(root);expect(result.accepted,result.message);
    const auto& publicLesson=lesson(valid.corpus,"imported_book_reading");
    expect(publicLesson.figure && publicLesson.figure->caption=="Public teaching diagram.","A figure outside textbook blocks retains its public caption");
    const auto model=instantiateDocumentFigure(*publicLesson.figure);
    expect(model.snapshot().kind==MathObjectKind::Harmonics && model.parameter(MathParameter::Amplitude1)==1,"Public figure parameters still reach their existing model");
    for(const auto& b:bookLessonView(publicLesson.lesson,{}))for(const auto& h:b.help)
      expect(!h.open && h.passages.empty(),"A public figure does not open protected teaching");
    expect(Json::parse(result.reportJson)["catalogue"]["questions"]==Json::parse(imported.reportJson)["catalogue"]["questions"],"Public diagrams leave existing question identities and stamps unchanged");
    ++publicFigures;
  }
  const auto header=text.substr(0,text.find("@block"));
  for(const auto& [command,preamble,directive]:std::vector<std::tuple<std::string,std::string,std::string>>{
    {"figure","",figureText},
    {"parameter","@figure harmonics 0\n@caption Public teaching diagram.\n","@parameter amplitude1 1\n"},
    {"caption","@figure harmonics 0\n","@caption Answer-bearing caption sentinel.\n"}})
    for(const std::string help:{"","proof","hint","answer","solution"})for(bool included:{false,true}) {
      auto prefix=header+preamble+"@block exercise | task | 1 | Solve\n@prose Public problem.\n";
      if(!help.empty())prefix+="@help "+help+"\n@prose Protected teaching.\n";
      write(fragment,directive);
      write(file,prefix+(included?"@include shared/figure.inc.md\n":directive)+"@endblock\n@end\n");
      Fixture bad;const auto topics=bad.corpus.topics.size();const auto failure=bad.load(root);
      expect(!failure.accepted,"A textbook "+(help.empty()?std::string("body"):help)+" cannot expose @"+command+" as public figure metadata");
      const auto diagnostic=Json::parse(failure.reportJson)["diagnostics"][0];
      expect(diagnostic["code"]=="document.content" && diagnostic["field"]==command &&
             diagnostic["file"]==(included?"shared/figure.inc.md":"book.paths.md") &&
             diagnostic["line"]==(included?1:std::count(prefix.begin(),prefix.end(),'\n')+1),"Figure scope errors identify the directive's original file, line and field, including fragments");
      const auto message=diagnostic["message"].get<std::string>();
      expect(message.find("outside @block")!=message.npos && message.find("public")!=message.npos,"Scope errors explain where figure metadata belongs and why it cannot be protected");
      expect(bad.corpus.entries.size()==930 && bad.corpus.topics.size()==topics && bad.bank.size()==278,"Invalid figure scope publishes no lesson, chapter or question");
      ++rejectedFigures;
    }
  std::cout<<"DOCUMENT_DISCLOSURE "<<Json{{"public_figures",publicFigures},{"rejected_scopes",rejectedFigures},{"source_locations",true},{"atomic_import",true},{"captures",0}}.dump()<<'\n';
  for(const auto& [from,to]:std::vector<std::pair<std::string,std::string>>{
    {"@block exercise","@block unknown"},{"| task |","| terms |"},{"@reference terms","@reference absent"},
    {"@help hint","@help clue"},{"@prose Look at the letter.",""},{"@help answer","@help hint"},
    {"@display\nx\n","@display eq1\nx\n"},{"@prose State the unknown.","@prose"},
    {"@template lesson.v2","@template lesson.v1"},{"@prose State the unknown.","Unscoped text"},
    {"@endblock\n@end","@end"},{"@prose State the unknown.","@prose "+std::string(8193,'x')},
    {"@block exercise | task | 1.2","@block exercise | task | 1.1"}}) {
    write(file,replace(text,from,to));Fixture bad;const auto failure=bad.load(root);const auto diagnostic=Json::parse(failure.reportJson)["diagnostics"][0];
    expect(!failure.accepted && diagnostic["file"]=="book.paths.md" && diagnostic["line"].get<unsigned>()>0 && !diagnostic["field"].get<std::string>().empty(),"Invalid textbook block reports file, line and command");
    expect(bad.corpus.entries.size()==930 && bad.bank.size()==278,"Malformed book commits neither a reading nor a question");
  }
  write(file,replace(text,"Hidden solution sentinel.","Revised hidden solution."));Fixture revised;const auto next=revised.load(root);expect(next.accepted,next.message);
  expect(Json::parse(imported.reportJson)["catalogue"]["readings"]!=Json::parse(next.reportJson)["catalogue"]["readings"],"Reading fingerprint covers unopened solutions");
  expect(Json::parse(imported.reportJson)["catalogue"]["questions"]==Json::parse(next.reportJson)["catalogue"]["questions"],"Changing a reading never changes the question bank's mathematical stamps");
}
void sourceLesson(const Path& root,const Path& folder,bool store) {
  Fixture f;
  if(store)for(const auto* name:{"matrix_reasoning.json","linear_support.json"}) {
    auto extra=loadCorpusStarters(Path(CORPUS_FIXTURE).parent_path()/name,f.corpus);f.bank.insert(f.bank.end(),extra.begin(),extra.end());
  }
  const auto report=store?importLearningStore(root,f.corpus,f.bank):f.load(root);expect(report.accepted,report.message);
  const auto& e=lesson(f.corpus,"source_002_reading");expect(e.lesson.size()==9 && e.questions==std::vector<std::string>{"source_002_full_problem","source_002_reduction"},"Actual source lesson has nine textbook blocks and two distinct practice links");
  for(const auto& b:bookLessonView(e.lesson,{}))for(const auto& h:b.help)expect(!h.open && h.passages.empty(),"Source lesson starts with independent disclosures closed");
  expect(e.body.find("f(-1)=\\frac32")==std::string::npos,"Original worked substitutions remain in the optional solution");
  CorpusPractice full(f.bank);full.loadProgress(folder/"full.json");full.open(question(f.bank,"source_002_full_problem"));
  const auto original=Json::parse(read(Path(CORPUS_FIXTURE).parent_path().parent_path()/"cards/source_002_quadratic_three_points.json"));
  expect(full.active()->content().steps.size()==13 && !full.active()->supportView(),"Full source route remains the current thirteen prepared decisions");
  for(unsigned i=0;i<13;++i) {
    const auto& step=full.active()->content().steps[i];const auto correct=fm::firstAcceptedOption(step);
    expect(step.id.value==original["steps"][i]["id"] && step.options[correct].id.value==original["steps"][i]["accepted_option_ids"][0],"Reuse the existing route's stable step and answer IDs");
    const auto working=std::string(full.active()->visibleWorking());
    for(unsigned j=0;j<step.options.size();++j)if(j!=correct) {
      expect(full.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[j].id)),"Prepared distractor submitted");
      expect(full.active()->visibleWorking()==working,"Every wrong source choice retains working");
    }
    expect(full.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[correct].id)) && full.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Original source decision advances the prepared owner");
  }
  expect(full.active()->currentRun().completed && !full.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Finished full problem waits for explicit navigation");
  full.saveProgress();const auto fullSave=read(folder/"full.json");CorpusPractice replay(f.bank);replay.loadProgress(folder/"full.json");
  expect(replay.active()->currentRun().completed,"Completed source question restores");replay.saveProgress();expect(read(folder/"full.json")==fullSave,"Prepared save is unchanged on replay");
  for(unsigned level=0;level<4;++level) {
    CorpusPractice p(f.bank);const auto save=folder/("source-level-"+std::to_string(level)+".json");p.loadProgress(save);p.open(question(f.bank,"source_002_reduction"));
    const auto send=[&](fm::SupportAction action,std::string text={},unsigned value=0){expect(p.dispatch(support(p,action,std::move(text),value)),"Source reduction level "+std::to_string(level)+", action "+std::to_string(static_cast<unsigned>(action))+" accepted");};
    send(fm::SupportAction::SelectLevel,{},level);
    if(level<2)for(unsigned i=0;i<3;++i) {
      const auto& q=p.active()->content();const auto& step=q.steps[i];const auto correct=fm::firstAcceptedOption(step);const auto before=p.active()->supportView()->working;
      if(!level) {
        expect(!p.active()->supportView()->reading.empty(),"Learn opens the authored explanation");
        send(fm::SupportAction::Choose,{},step.options[(correct+1)%3].id.value);
        expect(p.active()->supportView()->working==before && p.active()->supportView()->status==fm::WrittenCheckStatus::Incorrect,"Wrong operation leaves the exact matrix unchanged");
        send(fm::SupportAction::Choose,{},step.options[correct].id.value);
      } else {
        const auto answer=q.support->steps[i].responses[correct];send(fm::SupportAction::EditDraft,answer);send(fm::SupportAction::SubmitBlank,answer);
      }
    } else {
      expect(p.active()->supportView()->reading.empty() && p.active()->supportView()->choices.empty(),"Written levels do not prefill the route");
      send(fm::SupportAction::EditDraft,"[1, 0 | 3/2] [0, 1 | -1/2]");send(fm::SupportAction::CheckWork,"[1, 0 | 3/2] [0, 1 | -1/2]");
      expect(p.active()->supportView()->status==fm::WrittenCheckStatus::Incorrect && p.active()->supportView()->working.empty(),"Wrong fractional result is rejected without changing working");
      const std::string work="[1, -1 | 1] [0, 2 | 1]\n[1, -1 | 1] [0, 1 | 1/2]\n[1, 0 | 3/2] [0, 1 | 1/2]";
      send(fm::SupportAction::EditDraft,work);send(fm::SupportAction::CheckWork,work);
    }
    expect(p.active()->currentRun().completed && !p.active()->supportView()->verification.empty(),"Every source support level completes with verification");
    const auto& state=*p.active()->currentRun().support;const auto& result=std::get<fm::AugmentedMatrix>(state.nodes[state.active].equation);
    const auto a=result.rows[0][2],b=result.rows[1][2];
    expect(2*a.numerator==3*a.denominator && 2*b.numerator==b.denominator,"Exact final coefficients are independently 3/2 and 1/2");
    for(const auto& [x,y]:std::array<std::pair<int,int>,3>{{{-1,1},{0,0},{1,2}}})
      expect(a.numerator*x*x*b.denominator+b.numerator*x*a.denominator==y*a.denominator*b.denominator,"Final coefficients satisfy every original observation, not just the reduced rows");
    expect(!p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Completed reduction waits for navigation");
    const auto nodes=state.nodes.size();send(fm::SupportAction::Undo);expect(!p.active()->currentRun().completed && p.active()->currentRun().support->nodes.size()==nodes,"Undo preserves the completed branch");
    send(fm::SupportAction::SelectLevel,{},3);send(fm::SupportAction::EditDraft,"[1, 0 | 3/2] [0, 1 | ");p.saveProgress();const auto bytes=read(save);
    auto reordered=f.bank;std::reverse(reordered.begin(),reordered.end());CorpusPractice restored(reordered);restored.loadProgress(save);
    expect(restored.active() && restored.active()->supportView()->draft=="[1, 0 | 3/2] [0, 1 | " && restored.active()->currentRun().support->nodes.size()==nodes,"Source draft and Undo branches restore after catalogue reordering");
    restored.saveProgress();expect(read(save)==bytes,"Restored source save bytes remain unchanged");
  }
  std::cout<<"SOURCE_LESSON {\"blocks\":9,\"prepared_steps\":13,\"prepared_wrong_choices\":39,\"support_levels\":4,\"original_points_checked\":3,\"save_replay\":true,\"captures\":0}\n";
}
}
int main(int argc,char** argv) {
  const auto folder=std::filesystem::canonical(std::filesystem::temp_directory_path())/("paths-documents-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    if(argc==3 && std::string_view(argv[1])=="--draft-preview") {
      std::filesystem::create_directories(folder);const auto result=draftPreview(argv[2],folder);std::cout<<result.dump()<<'\n';std::filesystem::remove_all(folder);return 0;
    }
    if(argc==4 && std::string_view(argv[1])=="--question-batch-upgrade") {
      std::filesystem::create_directories(folder);questionBatchUpgrade(argv[2],argv[3],folder);std::filesystem::remove_all(folder);return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--reference-card") {
      std::filesystem::create_directories(folder);const auto result=referenceCard(argv[2],folder);std::cout<<result.dump()<<'\n';std::filesystem::remove_all(folder);return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--question-batch") {
      std::filesystem::create_directories(folder);questionBatch(argv[2],folder);std::filesystem::remove_all(folder);return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--published-store") {
      std::filesystem::create_directories(folder);published(argv[2],folder);std::filesystem::remove_all(folder);return 0;
    }
    if(argc==3 && (std::string_view(argv[1])=="--source-lesson" || std::string_view(argv[1])=="--source-store")) {
      std::filesystem::create_directories(folder);sourceLesson(argv[2],folder,std::string_view(argv[1])=="--source-store");std::filesystem::remove_all(folder);return 0;
    }
    expect(argc==1,"Use --published-store FOLDER or no arguments");
    const auto root=folder/"write";std::filesystem::create_directories(folder);std::filesystem::copy(DOCUMENT_FIXTURE,root,std::filesystem::copy_options::recursive);
    imports(root);failures(root);matrixDiagnostics(folder);matrices(root,folder);persistence(root,folder);bookDocuments(folder);livePreview(folder);
    auto reference=referenceCard(REFERENCE_FIXTURE,folder);reference.erase("question");std::cout<<"REFERENCE_CARD "<<reference.dump()<<'\n';std::filesystem::remove_all(folder);
    std::cout<<"DOCUMENT_IMPORT {\"documents\":4,\"subjects_added\":2,\"lessons\":4,\"questions\":4,\"solving_routes\":6,\"atomic_import\":true,\"drop_file_discovery\":true,\"save_replay\":true,\"native_windows\":0,\"captures\":0}\n";
  } catch(const std::exception& e){std::filesystem::remove_all(folder);std::cerr<<e.what()<<'\n';return 1;}
}
