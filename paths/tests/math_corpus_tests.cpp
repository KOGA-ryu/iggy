#include "content/MathCorpus.hpp"
#include "content/EquationSorterContentIO.hpp"
#include "runtime/first_move/LinearEquation.hpp"
#include <fstream>
#include <algorithm>
#include <array>
#include <tuple>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <nlohmann/json.hpp>

void expect(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
int main() {
  try {
    using namespace paths;
    const auto corpus=loadMathCorpus(CORPUS_FIXTURE);
    expect(corpus.subjects.size()==6 && corpus.entries.size()==930,"entire pinned corpus loads");
    std::unordered_set<std::string> ids;std::size_t terms=0;
    for(const auto& e:corpus.entries) {expect(ids.insert(e.id).second,"unique stable occurrence IDs");terms+=e.kind=="term";}
    expect(terms==802,"all terms survive the import");
    expect(corpus.reviewedCount==4,"only the four reviewed teaching adaptations are marked");
    const auto practice=loadSorterContent(SORTER_STUDY_FIXTURE);std::size_t linked=0;
    for(const auto& card:practice)if(card.solution && card.solution->mathModel==iggy3d::first_move::MathWorkingModel::RowReduction) {
      ++linked;expect(card.solution->notation.size()==8,"matrix practice has four base and four reviewed lessons");
      for(const auto& e:corpus.entries)if(e.review) {
        bool found=false;
        for(const auto& lesson:card.solution->notation)for(const auto& token:lesson.tokens)if(token.definition.id==e.id) {
          found=true;expect(token.definition.definition==e.review->term.definition && token.definition.meaning==e.review->term.meaning &&
              token.definition.example==e.review->term.example && token.definition.version==e.review->term.version,"Library and live practice share the same teaching note");
        }
        expect(found,"each matrix question links the reviewed corpus definitions");
      }
    }
    expect(linked==13,"all thirteen matrix questions receive the four reviewed explanations");
    namespace fm=iggy3d::first_move;
    auto matrix=*fm::parseAugmentedMatrix("[1,2|4] [3,5|11]").result;
    for(const auto& [operation,operand,after]:std::array{
        std::tuple{fm::MathOperation::AddRow1ToRow2,"-3","[1,2|4] [0,-1|-1]"},
        std::tuple{fm::MathOperation::DivideRow2,"-1","[1,2|4] [0,1|1]"},
        std::tuple{fm::MathOperation::AddRow2ToRow1,"-2","[1,0|2] [0,1|1]"}}) {
      const auto checked=fm::checkMathMove(matrix,operation,operand,after);
      expect(checked.result.has_value(),"reviewed example agrees with the canonical game kernel");matrix=*checked.result;
    }
    expect(!fm::checkMathMove(matrix,fm::MathOperation::DivideRow2,"0",matrix.display).result,"zero scaling divisor is rejected by the same kernel");
    expect(corpus.find(5,std::nullopt,"").size()==126,"probability survives the incomplete README index");
    const auto qr=corpus.find(3,std::nullopt,"QR DECOMPOSITION");
    expect(qr.size()==4,"duplicate titles preserved and search ignores ASCII case");
    expect(corpus.find(3,corpus.entries[qr[0]].topic,"QR decomposition").size()==1,"topic filter identifies separate source occurrence");
    expect(corpus.find(0,std::nullopt,"QR decomposition").empty(),"subject filter is authoritative");
    expect(corpus.find(std::nullopt,std::nullopt,"no such mathematics 1234").empty(),"empty results supported");
    expect(corpus.find(9000,std::nullopt,"").empty(),"unknown filter safely produces no results");
    for(const auto& e:corpus.entries)expect(e.firstLine>0 && e.lastLine>=e.firstLine && !e.body.empty(),"source notes and line ranges retained");
    std::ifstream file(CORPUS_FIXTURE);nlohmann::json source;file>>source;
    const auto rejects=[&](auto edit) {
      auto copy=source;edit(copy);bool refused=false;
      try{(void)parseMathCorpus(copy.dump());}catch(const std::exception&){refused=true;}
      expect(refused,"malformed corpus must be rejected before publishing UI data");
    };
    rejects([](auto& j){j["entries"][1]["id"]=j["entries"][0]["id"];});
    rejects([](auto& j){j["entries"][0]["topic"]="unknown";});
    rejects([](auto& j){j["topics"][0]["subject"]="unknown";});
    rejects([](auto& j){j["entries"][0]["kind"]="question";});
    rejects([](auto& j){j["entries"][0]["source"]="../secrets.md";});
    rejects([](auto& j){j["entries"][0]["source"]="/tmp/entry.md";});
    rejects([](auto& j){j["entries"][0]["first_line"]=-1;});
    rejects([](auto& j){j["entries"][0]["last_line"]=0;});
    rejects([](auto& j){j["entries"][0]["body"]=std::string("bad\0text",8);});
    rejects([](auto& j){j["entries"][0]["body"]=std::string(32769,'x');});
    rejects([](auto& j){j["subjects"]=nlohmann::json::array();});
    rejects([](auto& j){j["schema_version"]=2;});
    rejects([](auto& j){j["schema_version"]=1.0;});
    rejects([](auto& j){j["review_status"]="verified";});
    const auto reviewed=corpus.find(3,std::nullopt,"Gaussian Elimination").front();
    const auto rank=corpus.find(3,std::nullopt,"Rank").front();
    const auto nullity=corpus.find(3,std::nullopt,"Nullity").front();
    expect(corpus.entries[nullity].related.size()==2 && corpus.entries[nullity].related.front()==rank,
        "Nullity follows the explicit Rank occurrence, not a title match");
    std::size_t links=0;
    for(const auto& entry:corpus.entries)links+=entry.related.size();
    expect(links==7,"seven authored links connect the four reviewed notes");
    auto reordered=source;std::reverse(reordered["entries"].begin(),reordered["entries"].end());
    const auto shuffled=parseMathCorpus(reordered.dump());
    const auto shuffledNullity=std::find_if(shuffled.entries.begin(),shuffled.entries.end(),[](const auto& e){return e.id=="corpus_00514";});
    expect(shuffled.entries[shuffledNullity->related.front()].id=="corpus_00513","reordering source records preserves linked identities");
    auto legacy=source;for(auto& entry:legacy["entries"])entry.erase("related");
    const auto old=parseMathCorpus(legacy.dump());
    expect(old.reviewedCount==4 && old.entries[nullity].related.empty(),"older catalogues without links still load");
    rejects([&](auto& j){j["entries"][reviewed]["related"]=nlohmann::json::array();});
    rejects([&](auto& j){j["entries"][reviewed]["related"]="corpus_00512";});
    rejects([&](auto& j){j["entries"][reviewed]["related"]={nullptr};});
    rejects([&](auto& j){j["entries"][reviewed]["related"]={"missing"};});
    rejects([&](auto& j){j["entries"][reviewed]["related"]={"corpus_00511"};});
    rejects([&](auto& j){j["entries"][reviewed]["related"]={"corpus_00864"};});
    rejects([&](auto& j){j["entries"][reviewed]["related"]={"corpus_00512","corpus_00512"};});
    rejects([&](auto& j){j["entries"][reviewed]["related"]=nlohmann::json::array({"a","b","c","d","e"});});
    rejects([](auto& j){j["entries"][0]["related"]={"corpus_00511"};});
    rejects([&](auto& j){j["entries"][reviewed]["body"]="Edited source";});
    rejects([&](auto& j){j["entries"][reviewed]["title"]="Edited title";});
    rejects([&](auto& j){j["entries"][reviewed]["review"]["content_version"]=0;});
    rejects([&](auto& j){j["entries"][reviewed]["review"]["sources"]=nlohmann::json::array();});
    rejects([&](auto& j){j["entries"][reviewed]["review"]["sources"][0]["url"]="file:///tmp/source";});
    rejects([](auto& j){while(j["entries"].size()<4097)j["entries"].push_back(j["entries"][0]);});
    bool rejected=false;try{(void)parseMathCorpus(std::string(2*1024*1024+1,' '));}catch(const std::exception&){rejected=true;}
    expect(rejected,"oversized input refused");
    std::cout<<"Corpus source coverage, repeated titles, filtering, provenance and bounded loader passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
