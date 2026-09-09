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
std::string learningDocumentCapabilities();
// Both import validation and the live diagram consumer use the existing model's
// public registry and action checks. No document text is executed.
MathObjects instantiateDocumentFigure(const CorpusFigure&);
}
