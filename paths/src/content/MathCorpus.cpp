#include "content/MathCorpus.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace paths {
namespace {
constexpr std::size_t maxBytes=2*1024*1024;
std::string folded(std::string_view text) {
  std::string result(text);
  for(char& c:result)if(c>='A' && c<='Z')c+=('a'-'A');
  return result;
}
}
std::vector<std::size_t> MathCorpus::find(std::optional<std::size_t> subject,
    std::optional<std::size_t> topic,std::string_view query) const {
  const auto needle=folded(query);std::vector<std::size_t> result;
  for(std::size_t i=0;i<entries.size();++i) {
    const auto& entry=entries[i];const auto& chapter=topics[entry.topic];
    if(subject && chapter.subject!=*subject)continue;
    if(topic && entry.topic!=*topic)continue;
    if(!needle.empty() && folded(entry.title).find(needle)==std::string::npos)continue;
    result.push_back(i);
  }
  return result;
}
MathCorpus parseMathCorpus(std::string_view text) {
  if(text.size()>maxBytes)throw std::invalid_argument("Corpus exceeds 2 MiB");
  using Json=nlohmann::json;
  const auto doc=Json::parse(text);MathCorpus result;
  const auto fail=[](const std::string& field) {throw std::invalid_argument("Invalid corpus field: "+field);};
  if(!doc.at("schema_version").is_number_unsigned() || doc.at("schema_version")!=1)fail("schema_version");
  if(doc.at("review_status")!="unreviewed")fail("review_status");
  const auto list=[&](const char* name,std::size_t limit)->const Json& {
    const auto& value=doc.at(name);
    if(!value.is_array() || value.empty() || value.size()>limit)fail(name);
    return value;
  };
  const auto string=[&](const Json& object,const char* key,std::size_t limit) {
    const auto& value=object.at(key);if(!value.is_string())fail(key);
    auto s=value.get<std::string>();
    if(s.empty() || s.size()>limit || std::all_of(s.begin(),s.end(),[](char c){return c==' ' || c=='\n';}))fail(key);
    for(unsigned char c:s)if(c<32 && c!='\n' && !(c=='\t' && std::string_view(key)=="body"))fail(key);
    return s;
  };
  std::unordered_set<std::string> ids;
  const auto id=[&](const Json& object) {
    auto s=string(object,"id",64);
    if(!std::all_of(s.begin(),s.end(),[](char c){return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_';}) || !ids.insert(s).second)fail("id");
    return s;
  };
  const auto resolve=[&](const auto& records,const Json& object,const char* key) {
    const auto wanted=string(object,key,64);
    const auto found=std::find_if(records.begin(),records.end(),[&](const auto& record){return record.id==wanted;});
    if(found==records.end())fail(key);
    return static_cast<std::size_t>(found-records.begin());
  };
  for(const auto& subject:list("subjects",32))result.subjects.push_back({id(subject),string(subject,"title",160)});
  for(const auto& topic:list("topics",512))result.topics.push_back({id(topic),string(topic,"title",240),resolve(result.subjects,topic,"subject")});
  for(const auto& entry:list("entries",4096)) {
    CorpusEntry e{id(entry),string(entry,"title",1024),string(entry,"kind",16),string(entry,"body",32768),string(entry,"source",160)};
    if(e.kind!="term" && e.kind!="theorem" && e.kind!="proof" && e.kind!="example")fail("kind");
    const std::filesystem::path source(e.source);
    if(source.is_absolute() || source.extension()!=".md")fail("source");
    for(const auto& part:source)if(part==".." || part==".")fail("source");
    e.topic=resolve(result.topics,entry,"topic");
    for(const auto* key:{"first_line","last_line"})
      if(!entry.at(key).is_number_unsigned() || entry.at(key).get<std::uint64_t>()>1000000)fail(key);
    e.firstLine=entry.at("first_line");e.lastLine=entry.at("last_line");
    if(!e.firstLine || e.lastLine<e.firstLine)fail("source lines");
    if(entry.contains("review")) {
      const auto& r=entry.at("review");CorpusReview review;
      if(string(r,"source_title",1024)!=e.title || string(r,"source_body",32768)!=e.body)fail("review/source changed");
      const auto& version=r.at("content_version");
      if(!version.is_number_unsigned() || version.get<std::uint64_t>()>UINT32_MAX)fail("review/content_version");
      review.term={e.id,e.title,string(r,"meaning",160),string(r,"definition",480),string(r,"example",240),version.get<std::uint32_t>()};
      if(!iggy3d::first_move::validNotationDefinition(review.term))fail("review/definition");
      review.reviewedOn=string(r,"reviewed_on",10);review.conditions=string(r,"conditions",800);review.change=string(r,"change",800);
      if(review.reviewedOn.size()!=10 || review.reviewedOn[4]!='-' || review.reviewedOn[7]!='-')fail("review/reviewed_on");
      const auto& citations=r.at("sources");
      if(!citations.is_array() || citations.empty() || citations.size()>8)fail("review/sources");
      std::unordered_set<std::string> urls;
      for(const auto& source:citations) {
        CorpusCitation citation{string(source,"title",160),string(source,"url",512),string(source,"locator",240)};
        if(!citation.url.starts_with("https://") || citation.url.size()<9 || !urls.insert(citation.url).second)fail("review/source URL");
        review.sources.push_back(std::move(citation));
      }
      e.review=std::move(review);++result.reviewedCount;
    }
    result.entries.push_back(std::move(e));
  }
  for(std::size_t i=0;i<result.entries.size();++i) {
    const auto& entry=doc.at("entries")[i];auto& current=result.entries[i];
    if(!entry.contains("related"))continue;
    const auto& links=entry.at("related");
    if(!current.review || !links.is_array() || links.empty() || links.size()>4)fail("related");
    for(const auto& link:links) {
      const auto target=resolve(result.entries,Json{{"target",link}},"target");
      if(target==i || !result.entries[target].review || std::find(current.related.begin(),current.related.end(),target)!=current.related.end())fail("related/target");
      current.related.push_back(target);
    }
  }
  return result;
}
MathCorpus loadMathCorpus(const std::filesystem::path& path) {
  try {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || file.tellg()<0 || file.tellg()>static_cast<std::streamoff>(maxBytes))throw std::invalid_argument("Missing or oversized corpus");
    std::string text(static_cast<std::size_t>(file.tellg()),'\0');file.seekg(0);
    if(!file.read(text.data(),static_cast<std::streamsize>(text.size())))throw std::invalid_argument("Cannot read corpus");
    return parseMathCorpus(text);
  } catch(const std::exception& error) {throw std::invalid_argument(path.string()+": "+error.what());}
}
} // namespace paths
