#pragma once
#include "content/CorpusPractice.hpp"
#include "runtime/math_objects/MathObjects.hpp"

namespace paths {
struct DocumentImport {
  bool accepted=true;
  std::size_t files=0,lessons=0,questions=0;
  std::string message;
  std::string reportJson;
};
// Read a bounded folder of .paths.md files, validate the entire import, then
// append atomically to the existing catalogue. No source files or saves are written.
DocumentImport importLearningDocuments(const std::filesystem::path&,MathCorpus&,std::vector<CorpusStarter>&);
// Verify one immutable published generation and import its captured bytes through
// the same compiler. Failure leaves the catalogue untouched.
DocumentImport importLearningStore(const std::filesystem::path&,MathCorpus&,std::vector<CorpusStarter>&);
struct DocumentRemap {
  std::vector<std::optional<std::size_t>> subjects,topics,entries;
};
// Read-only authoring watcher. Capture stable bytes, compile through the same
// importer, then replace an in-memory preview atomically. No native/UI dependency.
class LearningDocumentPreview {
public:
  using Clock=std::chrono::steady_clock;
  LearningDocumentPreview(std::filesystem::path,const MathCorpus&,const std::vector<CorpusStarter>&);
  bool poll(MathCorpus&,CorpusPractice&,Clock::time_point now=Clock::now());
  const DocumentImport& report() const {return report_;}
  const DocumentRemap& remap() const {return remap_;}
  std::uint64_t revision() const {return revision_;}
private:
  std::filesystem::path folder_;
  MathCorpus baseline_;
  std::vector<CorpusStarter> questions_;
  std::optional<std::map<std::filesystem::path,std::string>> observed_;
  bool handled_=false;
  Clock::time_point nextPoll_{};
  DocumentImport report_;
  DocumentRemap remap_;
  std::uint64_t revision_=0;
};
std::string learningDocumentCapabilities();
// Both import validation and the live diagram consumer use the existing model's
// public registry and action checks. No document text is executed.
MathObjects instantiateDocumentFigure(const CorpusFigure&);
}
