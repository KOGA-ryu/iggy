#include "ui/NativeMath.hpp"
#include "ui/MathNotationUi.hpp"
#include "content/MathCorpus.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

using namespace paths;
namespace {
struct NativeMathSample {const char* label;const char* title;const char* latex;};
inline constexpr std::array<NativeMathSample,4> nativeMathSamples{{
  {"Fraction","A fraction",R"(\frac{a+b}{c+d})"},
  {"Root","A nested root",R"(\sqrt{1+\sqrt{1+x^{2}}})"},
  {"Matrix","A matrix with a fractional entry",R"(A=\begin{bmatrix}1&\frac{1}{2}&0\\-2&3&1\\0&-1&4\end{bmatrix})"},
  {"095","095 · Eigenmodes of a lopsided drum",R"(\Delta u=-\lambda^{2}\left(1+\frac{x}{2}\right)u)"}
}};
void expect(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool contains(const NotationBounds& a,const NotationBounds& b) {
  return b.x>=a.x-.5F && b.y>=a.y-.5F && b.x+b.width<=a.x+a.width+.5F && b.y+b.height<=a.y+a.height+.5F;
}
struct Harness {
  bool dynamic=false;
  std::unique_ptr<NativeMath> math;
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  Harness(float width,float height,bool dynamicAtlas=false):dynamic(dynamicAtlas) {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;io.DisplaySize={width,height};io.DeltaTime=1.0F/60;
    if(dynamic)io.BackendFlags|=ImGuiBackendFlags_RendererHasTextures|ImGuiBackendFlags_RendererHasVtxOffset;
    math=std::make_unique<NativeMath>(MATH_RESOURCES);frame(3);
  }
  ~Harness(){math.reset();ImGui::DestroyContext();}
  void begin() {
    // Font rasterization stays in memory. There is no window, device or image output.
    if(!dynamic) {
      unsigned char* pixels;int width,height;
      ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
      ImGui::GetIO().Fonts->SetTexID(static_cast<ImTextureID>(1));
    }
    ImGui::NewFrame();
  }
  void end() {
    ImGui::Render();
    if(dynamic) {
      // Acknowledge the same atlas requests as the native backend, using dummy
      // handles. This tests lazy glyphs and UV rebinding without a Vulkan device.
      for(auto* texture:ImGui::GetPlatformIO().Textures) {
        if(texture->Status==ImTextureStatus_WantCreate || texture->Status==ImTextureStatus_WantUpdates) {
          texture->SetTexID(static_cast<ImTextureID>(texture->UniqueID+1));texture->SetStatus(ImTextureStatus_OK);
        } else if(texture->Status==ImTextureStatus_WantDestroy) {
          texture->SetTexID(ImTextureID_Invalid);texture->SetStatus(ImTextureStatus_Destroyed);
        }
      }
      for(const auto* list:ImGui::GetDrawData()->CmdLists)for(const auto& command:list->CmdBuffer)
        if(command.ElemCount)expect(command.GetTexID()!=ImTextureID_Invalid,"native draw commands resolve an updated atlas texture");
    }
  }
  void frame(int count=1){for(int i=0;i<count;++i){begin();end();}}
};
struct Ink {std::vector<NotationBounds> glyphs;std::vector<ImVec2> rules;};
Ink observe(Harness& h,std::string_view source,float pixels) {
  h.begin();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1400,800});ImGui::Begin("Geometry");
  const auto& equation=h.math->layout(source,pixels);
  if(!equation.error.empty())throw std::runtime_error(std::string(source)+": "+equation.error);
  expect(equation.width>0 && equation.height>0,"equation has positive extents");
  auto* draw=ImGui::GetWindowDrawList();const auto start=draw->VtxBuffer.Size;
  h.math->draw(equation,50,50,IM_COL32(19,181,213,255));
  const auto white=ImGui::GetIO().Fonts->TexUvWhitePixel;Ink ink;
  const NotationBounds bounds{48,48,equation.width+4,equation.height+4,true};
  for(int i=start;i<draw->VtxBuffer.Size;) {
    const auto& v=draw->VtxBuffer[i];
    expect(std::isfinite(v.pos.x) && std::isfinite(v.pos.y),"finite submitted vertices");
    expect(contains(bounds,{v.pos.x,v.pos.y,0,0,true}),"submitted ink fits the declared extent");
    if(v.uv.x==white.x && v.uv.y==white.y){if((v.col>>24)!=0)ink.rules.push_back(v.pos);++i;continue;}
    expect(i+3<draw->VtxBuffer.Size,"complete glyph quad");
    float l=v.pos.x,r=l,t=v.pos.y,b=t;
    for(int j=0;j<4;++j){const auto p=draw->VtxBuffer[i+j].pos;l=std::min(l,p.x);r=std::max(r,p.x);t=std::min(t,p.y);b=std::max(b,p.y);}
    ink.glyphs.push_back({l,t,r-l,b-t,true});i+=4;
  }
  ImGui::End();h.end();return ink;
}
void typography(Harness& h) {
  const auto fraction=observe(h,R"(\frac{1}{2})",32);
  expect(fraction.glyphs.size()==2 && !fraction.rules.empty(),"fraction submits two glyphs and a separate bar");
  auto digits=fraction.glyphs;std::sort(digits.begin(),digits.end(),[](auto a,auto b){return a.y<b.y;});
  expect(digits[0].y+digits[0].height<digits[1].y,"fraction numerator and denominator are separated");
  const auto bar=std::minmax_element(fraction.rules.begin(),fraction.rules.end(),[](auto a,auto b){return a.y<b.y;});
  expect(bar.first->y>digits[0].y+digits[0].height && bar.second->y<digits[1].y,"bar sits between numerator and denominator");
  const auto power=observe(h,"x^{2}",32);
  expect(power.glyphs.size()==2 && power.glyphs[1].height<power.glyphs[0].height && power.glyphs[1].y<power.glyphs[0].y,"superscript is smaller and raised");
  const auto matrix=observe(h,R"(\begin{bmatrix}1&2\\3&4\end{bmatrix})",32);
  // The four short glyphs are the digits; brackets are taller than either row.
  std::vector<NotationBounds> cells;for(const auto& g:matrix.glyphs)if(g.height<30)cells.push_back(g);
  expect(cells.size()==4,"matrix retains four distinct cell glyphs");
  std::sort(cells.begin(),cells.end(),[](auto a,auto b){return a.y<b.y;});
  expect(cells[0].y+cells[0].height<cells[2].y,"matrix rows remain distinct");
  const float topWidth=std::abs(cells[1].x-cells[0].x),bottomWidth=std::abs(cells[3].x-cells[2].x);
  expect(topWidth>15 && std::abs(topWidth-bottomWidth)<3,"matrix columns align across rows");
  for(const auto& sample:nativeMathSamples)for(const float size:{16.0F,24.0F,40.0F}) {
    const auto ink=observe(h,sample.latex,size);expect(!ink.glyphs.empty(),"sample has glyph geometry");
    const auto& first=h.math->layout(sample.latex,size);
    expect(&first==&h.math->layout(sample.latex,size),"unchanged equation reuses cached layout");
  }
  expect(!h.math->layout(R"(\notARealMathCommand)",24).error.empty(),"unsupported commands produce an explicit error");
  expect(!h.math->layout(R"(\frac{1}{\notARealMathCommand})",24).error.empty(),"nested unsupported commands cannot yield a partial equation");
  expect(!h.math->layout(R"(\frac{1}{2)",24).error.empty(),"an unclosed group cannot yield a partial equation");
  expect(!h.math->layout("x",std::numeric_limits<float>::infinity()).error.empty(),"invalid size is rejected");
  expect(!h.math->layout(std::string(4097,'x'),24).error.empty(),"oversized input is rejected");
  expect(h.math->layout("x+1",24).error.empty(),"a failed expression cannot poison the next layout");
}
void documentContract(const NativeMath::Document& document) {
  expect(document.width>=0 && document.height>0 && std::isfinite(document.width) && std::isfinite(document.height),"document has bounded finite extents");
  std::size_t end=0;
  for(std::size_t i=0;i<document.parts.size();++i) {
    const auto& part=document.parts[i];expect(part.begin==end && part.end>part.begin,"source spans are contiguous and nonempty");end=part.end;
    std::string submitted;
    for(const auto& p:document.placements)if(p.part==i)submitted.append(document.source,p.begin,p.end-p.begin);
    auto expected=document.source.substr(part.begin,part.end-part.begin);
    const auto compact=[](std::string text){std::erase_if(text,[](auto c){return c==' ' || c=='\t' || c=='\r' || c=='\n';});return text;};
    expect(compact(submitted)==compact(expected),"every non-whitespace source byte has exactly one presentation");
    if(part.kind==NativeMath::Document::Part::Kind::Source)expect(!part.equation.error.empty(),"raw fallback always explains its reason");
  }
  expect(end==document.source.size(),"source spans cover the exact original bytes");
  for(std::size_t i=0;i<document.placements.size();++i) {
    const auto& p=document.placements[i];
    expect(std::isfinite(p.x) && std::isfinite(p.y) && p.x>=0 && p.y>=0 && p.x+p.width<=document.width+.1F && p.y+p.height<=document.height+.1F,"text and math fit declared document bounds");
    for(std::size_t j=0;j<i;++j) {
      const auto& q=document.placements[j];
      expect(p.x+p.width<=q.x+.1F || q.x+q.width<=p.x+.1F || p.y+p.height<=q.y+.1F || q.y+q.height<=p.y+.1F,"prose and equation boxes never overlap");
    }
  }
}
void documents(Harness& h) {
  h.begin();ImGui::Begin("Document contract");ImGui::PushFont(nullptr,13);
  const std::string mixed=R"(Before \(x^{2}\), after \[\frac{1}{2}\] done $a_i$. $$\sum_{i=1}^{n}i$$)";
  const auto wide=h.math->layoutDocument(mixed,600);documentContract(wide);
  expect(wide.source==mixed && wide.equations==4 && wide.fallbacks==0,"four supported delimiter forms retain the source and typeset");
  const auto& inlinePart=wide.parts[1];
  const auto text=wide.placements[0],equation=wide.placements[1];
  expect(std::abs(text.y+text.baseline-equation.y-equation.baseline)<.1F && equation.x>text.x+text.width,"inline formula shares the prose baseline and its authored space");
  expect(inlinePart.equation.baseline>0 && inlinePart.equation.baseline<=inlinePart.equation.height,"formula publishes a usable baseline");
  const auto narrow=h.math->layoutDocument(mixed,60);documentContract(narrow);
  expect(narrow.height>wide.height,"narrow wrapping adds lines without shrinking mathematics");
  const auto& same=h.math->layoutDocument(mixed,60);expect(&same==&h.math->layoutDocument(mixed,60),"unchanged document reuses its layout");
  const auto escaped=h.math->layoutDocument(R"(Cost \$5, literal \\(x\\), then \(y\).)",280);documentContract(escaped);
  expect(escaped.equations==1 && escaped.fallbacks==1,"escaped delimiters stay literal source and cannot become typeset math");
  const auto malformed=h.math->layoutDocument(R"(Before $x\) middle \(y\) after \[z)",280);documentContract(malformed);
  expect(malformed.equations==1 && malformed.fallbacks==2,"mismatched and unclosed delimiters retain source and recover for later valid math");
  const auto unsupported=h.math->layoutDocument(R"(A \(\frac{1}{\notARealMathCommand}\) B \(x+1\).)",280);documentContract(unsupported);
  expect(unsupported.equations==1 && unsupported.fallbacks==1,"an unsupported formula cannot hide adjacent prose or the next formula");
  const auto comments=h.math->layoutDocument("\\(x% \\) ignored\n+1\\)",280);documentContract(comments);
  expect(comments.equations==1 && comments.fallbacks==0,"TeX comments do not close a formula");
  // Hold a document while its individual expressions leave the sixteen-entry cache.
  for(int i=0;i<24;++i)h.math->layout("x+"+std::to_string(i),16);
  h.math->draw(wide,10,10,IM_COL32_WHITE,IM_COL32(89,217,255,255),IM_COL32(255,199,77,255));
  documentContract(wide);
  ImGui::PopFont();ImGui::End();h.end();
}
void corpusCompatibility(Harness& h) {
  nlohmann::json report{{"entries",h.corpus.entries.size()},{"equations",0},{"entries_without_fallback",0},{"fallback_parts",0},{"fallback_entries",nlohmann::json::array()}};
  float widest=0;std::string widestEntry;
  for(const auto& entry:h.corpus.entries) {
    h.begin();ImGui::SetNextWindowSize({340,460});ImGui::Begin("Corpus compatibility");ImGui::PushFont(nullptr,13);
    const auto& document=h.math->layoutDocument(entry.body,280);documentContract(document);
    expect(document.source==entry.body,"corpus source is retained byte for byte");
    h.math->draw(document,20,20,IM_COL32_WHITE,IM_COL32(89,217,255,255),IM_COL32(255,199,77,255));
    report["equations"]=report["equations"].get<std::size_t>()+document.equations;
    report["fallback_parts"]=report["fallback_parts"].get<std::size_t>()+document.fallbacks;
    if(!document.fallbacks)report["entries_without_fallback"]=report["entries_without_fallback"].get<std::size_t>()+1;
    else {
      nlohmann::json failures=nlohmann::json::array();
      for(const auto& part:document.parts)if(!part.equation.error.empty())
        failures.push_back({{"begin",part.begin},{"end",part.end},{"source",document.source.substr(part.begin,part.end-part.begin)},{"reason",part.equation.error}});
      report["fallback_entries"].push_back({{"id",entry.id},{"title",entry.title},{"parts",failures}});
    }
    if(document.width>widest){widest=document.width;widestEntry=entry.id;}
    ImGui::PopFont();ImGui::End();h.end();
  }
  report["widest"]={{"id",widestEntry},{"pixels",widest}};
  std::cout<<"CORPUS_COMPATIBILITY "<<report.dump()<<'\n';h.frame(3);
}
} // namespace
int main() {
  try {
    // These optional typesetter probes do not exercise the retired Library
    // screen. Textbook navigation is verified separately without any fonts.
    for(bool dynamic:{false,true})for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
      Harness h(size.x,size.y,dynamic);typography(h);documents(h);
      if(!dynamic && size.x==1440)corpusCompatibility(h);
    }
    std::cout<<"Native math geometry, mixed documents and dynamic atlas checks passed\n";
  } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
