#include "ui/NativeMath.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>
#include <imgui.h>
#include <core/formula.h>
#include <latex.h>
#include <utils/utf.h>

namespace {
constexpr float atlasEm=48;
using FontMap=std::map<std::string,ImFont*>;
// MicroTeX caches font descriptors process-wide. They hold paths, never ImGui
// pointers, so recreating a UI context cannot retain a dead atlas.
thread_local const FontMap* activeFonts=nullptr;
std::string fontKey(const std::filesystem::path& path) {return path.lexically_normal().generic_string();}
struct MathFont final:tex::Font {
  std::string path;
  float size;
  MathFont(std::string file,float pixels):path(fontKey(file)),size(pixels){}
  float getSize() const override {return size;}
  tex::sptr<tex::Font> deriveFont(int style) const override {
    constexpr std::array names{"r10.ttf","bx10.ttf","i10.ttf","bi10.ttf"};
    return std::make_shared<MathFont>((std::filesystem::path(tex::RES_BASE)/"fonts/latin"/names.at(style)).string(),size);
  }
  bool operator==(const tex::Font& other) const override {
    const auto* f=dynamic_cast<const MathFont*>(&other);return f && path==f->path && size==f->size;
  }
  bool operator!=(const tex::Font& other) const override {return !(*this==other);}
  ImFont* imgui() const {
    if(!activeFonts)throw std::runtime_error("Math font context is unavailable");
    return activeFonts->at(path);
  }
};
const ImFontGlyph& glyph(ImFont* font,wchar_t code) {
  const auto* found=font->GetFontBaked(atlasEm)->FindGlyphNoFallback(static_cast<ImWchar>(code));
  if(!found)throw std::runtime_error("Math font is missing symbol "+std::to_string(static_cast<unsigned>(code)));
  return *found;
}
struct TextLayout final:tex::TextLayout {
  std::wstring text;
  tex::sptr<tex::Font> font;
  TextLayout(std::wstring value,tex::sptr<tex::Font> f):text(std::move(value)),font(std::move(f)){}
  void getBounds(tex::Rect& bounds) override {
    const auto& f=static_cast<const MathFont&>(*font);auto* native=f.imgui();
    const float scale=f.size/atlasEm,ascent=native->GetFontBaked(atlasEm)->Ascent;
    float advance=0,left=0,top=0,right=0,bottom=0;
    for(const auto c:text) {
      const auto& g=glyph(native,c);left=std::min(left,advance+g.X0);right=std::max(right,advance+g.X1);
      top=std::min(top,g.Y0-ascent);bottom=std::max(bottom,g.Y1-ascent);advance+=g.AdvanceX;
    }
    bounds={left*scale,top*scale,(std::max(right,advance)-left)*scale,(bottom-top)*scale};
  }
  void draw(tex::Graphics2D& g,float x,float y) override {g.setFont(font.get());g.drawText(text,x,y);}
};
// ImGui's STB loader scales to ascender-minus-descender; TeX metrics use em units.
// Read that ratio from the bundled font instead of stretching tall delimiters.
float emScale(const std::filesystem::path& path) {
  std::ifstream file(path,std::ios::binary);std::vector<unsigned char> b{std::istreambuf_iterator<char>(file),{}};
  const auto u16=[&](std::size_t p){return (unsigned(b.at(p))<<8)|b.at(p+1);};
  const auto u32=[&](std::size_t p){return (u16(p)<<16)|u16(p+2);};
  std::size_t head=0,hhea=0;
  for(unsigned i=0;i<u16(4);++i) {
    const auto p=12+16*i;
    const std::string tag{b.begin()+p,b.begin()+p+4};
    if(tag=="head")head=u32(p+8);else if(tag=="hhea")hhea=u32(p+8);
  }
  if(!head || !hhea || !u16(head+18))throw std::runtime_error("Invalid bundled math font: "+path.string());
  return float(static_cast<std::int16_t>(u16(hhea+4))-static_cast<std::int16_t>(u16(hhea+6)))/u16(head+18);
}
struct Engine {
  std::string resources;
  explicit Engine(std::string root):resources(std::move(root)){tex::LaTeX::init(resources);}
  ~Engine(){tex::LaTeX::release();}
};
struct Transform {
  float a=1,b=0,c=0,d=1,x=0,y=0;
  ImVec2 point(float px,float py) const {return {a*px+c*py+x,b*px+d*py+y};}
  void translate(float dx,float dy){const auto p=point(dx,dy);x=p.x;y=p.y;}
  void scale(float sx,float sy){a*=sx;b*=sx;c*=sy;d*=sy;}
  void rotate(float angle){const auto old=*this;const float cs=std::cos(angle),sn=std::sin(angle);
    a=old.a*cs+old.c*sn;b=old.b*cs+old.d*sn;c=-old.a*sn+old.c*cs;d=-old.b*sn+old.d*cs;}
};
struct Command {
  enum class Kind {Glyph,Line,Fill} kind;
  std::array<ImVec2,4> points{};
  ImFont* font=nullptr;
  wchar_t code=0;
  float thickness=1;
  ImU32 colour=IM_COL32_WHITE;
};
struct Graphics final:tex::Graphics2D {
  std::vector<Command> commands;
  Transform transform;
  tex::Stroke stroke;
  const tex::Font* font=nullptr;
  tex::color colour=tex::white;
  void setColor(tex::color c) override {colour=c;}
  tex::color getColor() const override {return colour;}
  ImU32 tint() const {return IM_COL32(tex::color_r(colour),tex::color_g(colour),tex::color_b(colour),tex::color_a(colour));}
  void setStroke(const tex::Stroke& s) override {stroke=s;}
  const tex::Stroke& getStroke() const override {return stroke;}
  void setStrokeWidth(float width) override {stroke.lineWidth=width;}
  const tex::Font* getFont() const override {return font;}
  void setFont(const tex::Font* f) override {font=f;}
  void translate(float x,float y) override {transform.translate(x,y);}
  void scale(float x,float y) override {transform.scale(x,y);}
  void rotate(float angle) override {transform.rotate(angle);}
  void rotate(float angle,float x,float y) override {translate(x,y);rotate(angle);translate(-x,-y);}
  void reset() override {transform={};}
  float sx() const override {return std::hypot(transform.a,transform.b);}
  float sy() const override {return std::hypot(transform.c,transform.d);}
  void drawChar(wchar_t code,float x,float y) override {
    const auto& f=static_cast<const MathFont&>(*font);auto* native=f.imgui();
    const auto& g=glyph(native,code);if(!g.Visible)return;
    const float factor=f.size/atlasEm,ascent=native->GetFontBaked(atlasEm)->Ascent;
    const float l=x+g.X0*factor,r=x+g.X1*factor,t=y+(g.Y0-ascent)*factor,b=y+(g.Y1-ascent)*factor;
    Command c{Command::Kind::Glyph};c.points={transform.point(l,t),transform.point(r,t),transform.point(r,b),transform.point(l,b)};
    c.font=native;c.code=code;c.colour=tint();commands.push_back(c);
  }
  void drawText(const std::wstring& text,float x,float y) override {
    const auto& f=static_cast<const MathFont&>(*font);
    for(const auto code:text){drawChar(code,x,y);x+=glyph(f.imgui(),code).AdvanceX*f.size/atlasEm;}
  }
  void drawLine(float x1,float y1,float x2,float y2) override {
    Command c{Command::Kind::Line};c.points[0]=transform.point(x1,y1);c.points[1]=transform.point(x2,y2);
    c.thickness=std::max(.6F,stroke.lineWidth*sy());c.colour=tint();commands.push_back(c);
  }
  void fillRect(float x,float y,float w,float h) override {
    Command c{Command::Kind::Fill};c.points={transform.point(x,y),transform.point(x+w,y),transform.point(x+w,y+h),transform.point(x,y+h)};
    c.colour=tint();commands.push_back(c);
  }
  void drawRect(float x,float y,float w,float h) override {drawLine(x,y,x+w,y);drawLine(x+w,y,x+w,y+h);drawLine(x+w,y+h,x,y+h);drawLine(x,y+h,x,y);}
  void drawRoundRect(float,float,float,float,float,float) override {throw std::runtime_error("Rounded equation frames are not supported yet");}
  void fillRoundRect(float,float,float,float,float,float) override {throw std::runtime_error("Rounded equation frames are not supported yet");}
};
ImU32 multiplyColour(ImU32 a,ImU32 b) {
  ImU32 c=0;for(unsigned shift=0;shift<32;shift+=8)c|=(((a>>shift)&255)*((b>>shift)&255)/255)<<shift;return c;
}
struct Delimiter {std::size_t begin,end;std::string_view token;};
std::optional<Delimiter> nextDelimiter(std::string_view source,std::size_t begin,bool inMath=false) {
  for(auto i=begin;i<source.size();++i) {
    if(inMath && source[i]=='%'){while(i<source.size() && source[i]!='\n')++i;continue;}
    if(source[i]=='$') {
      const auto end=i+1+(i+1<source.size() && source[i+1]=='$');return Delimiter{i,end,source.substr(i,end-i)};
    }
    if(source[i]!='\\' || i+1==source.size())continue;
    if(!inMath && source[i+1]=='\\' && i+2<source.size() && std::string_view("()[]").find(source[i+2])!=std::string_view::npos)
      return Delimiter{i,i+3,source.substr(i,3)};
    if(std::string_view("()[]").find(source[i+1])!=std::string_view::npos)return Delimiter{i,i+2,source.substr(i,2)};
    ++i; // Escaped dollars/backslashes are literal; they cannot open or close math.
  }
  return std::nullopt;
}
} // namespace

namespace tex {
Font* Font::create(const std::string& file,float size){return new MathFont(file,size);}
sptr<Font> Font::_create(const std::string&,int style,float size){return MathFont((std::filesystem::path(RES_BASE)/"fonts/latin/r10.ttf").string(),size).deriveFont(style);}
sptr<TextLayout> TextLayout::create(const std::wstring& text,const sptr<Font>& font){return std::make_shared<::TextLayout>(text,font);}
} // namespace tex

namespace paths {
struct NativeMath::Geometry {std::vector<Command> commands;};
struct NativeMath::Impl {
  ImGuiContext* context=ImGui::GetCurrentContext();
  FontMap fonts;
  struct Cached {std::string source;float pixels;bool display;Equation equation;};
  std::list<Cached> cache;
  std::optional<Document> document;
  ImFont* proseFont=nullptr;
  float prosePixels=0,wrapWidth=0;
};
NativeMath::NativeMath(const std::filesystem::path& resources):impl_(std::make_unique<Impl>()) {
  if(!impl_->context)throw std::runtime_error("Native math needs an ImGui context");
  const auto root=std::filesystem::canonical(resources);
  if(!std::filesystem::is_regular_file(root/".clatexmath-res_root"))throw std::runtime_error("Missing bundled math resources");
  static Engine engine(root.string());
  if(engine.resources!=root.string())throw std::runtime_error("Native math resource root changed during this process");
  auto* atlas=ImGui::GetIO().Fonts;if(atlas->Fonts.empty())atlas->AddFontDefault();
  std::vector<std::filesystem::path> files;
  for(const auto& file:std::filesystem::recursive_directory_iterator(root/"fonts"))if(file.path().extension()==".ttf")files.push_back(file.path());
  std::sort(files.begin(),files.end());
  static const ImWchar ranges[]{1,255,0};
  for(const auto& file:files) {
    ImFontConfig config;config.ExtraSizeScale=emScale(file);config.OversampleH=2;config.OversampleV=2;
    auto* font=atlas->AddFontFromFileTTF(file.string().c_str(),atlasEm,&config,ranges);
    if(!font)throw std::runtime_error("Unable to load math font: "+file.string());
    impl_->fonts.emplace(fontKey(file),font);
  }
}
NativeMath::~NativeMath()=default;
const NativeMath::Equation& NativeMath::layout(std::string_view latex,float pixels,bool display) {
  if(ImGui::GetCurrentContext()!=impl_->context)throw std::runtime_error("Native math used with the wrong UI context");
  for(auto i=impl_->cache.begin();i!=impl_->cache.end();++i)if(i->source==latex && i->pixels==pixels && i->display==display) {
    impl_->cache.splice(impl_->cache.begin(),impl_->cache,i);return impl_->cache.front().equation;
  }
  Impl::Cached cached{std::string(latex),pixels,display,{}};auto& result=cached.equation;
  struct FontScope {const FontMap* previous=activeFonts;~FontScope(){activeFonts=previous;}} scope;
  activeFonts=&impl_->fonts;
  try {
    if(latex.empty() || latex.size()>4096 || !std::isfinite(pixels) || pixels<8 || pixels>96)throw std::runtime_error("Equation or size is outside the supported range");
    // Even strict MicroTeX accepts some unfinished groups. Reject them before
    // layout; escaped literal braces and TeX comments do not open groups.
    int depth=0;
    for(std::size_t i=0;i<latex.size();++i) {
      if(latex[i]=='\\'){++i;continue;}
      if(latex[i]=='%'){while(i<latex.size() && latex[i]!='\n')++i;continue;}
      if(latex[i]=='{' && ++depth>64)throw std::runtime_error("Equation nesting is too deep");
      if(latex[i]=='}' && --depth<0)throw std::runtime_error("Equation has unbalanced braces");
    }
    if(depth)throw std::runtime_error("Equation has unbalanced braces");
    tex::Formula formula;
    tex::TeXParser parser(false,tex::utf82wide(std::string(latex)),&formula);
    parser.parse();
    tex::TeXRenderBuilder builder;
    std::unique_ptr<tex::TeXRender> render(builder.setStyle(display?tex::TexStyle::display:tex::TexStyle::text).setTextSize(pixels).setForeground(tex::white).build(formula));
    Graphics graphics;render->draw(graphics,0,0);
    float left=0,top=0,right=static_cast<float>(render->getWidth()),bottom=static_cast<float>(render->getHeight());
    for(const auto& command:graphics.commands) {
      const auto count=command.kind==Command::Kind::Line?2:4;
      const float margin=command.kind==Command::Kind::Line?command.thickness*.5F:0;
      for(int i=0;i<count;++i){const auto p=command.points[i];
        if(!std::isfinite(p.x) || !std::isfinite(p.y))throw std::runtime_error("Equation produced an invalid layout");
        left=std::min(left,p.x-margin);top=std::min(top,p.y-margin);right=std::max(right,p.x+margin);bottom=std::max(bottom,p.y+margin);}
    }
    if(right-left>32768 || bottom-top>32768)throw std::runtime_error("Equation is too large to display");
    for(auto& command:graphics.commands)for(auto& p:command.points){p.x+=2-left;p.y+=2-top;}
    result.width=std::ceil(right-left)+4;result.height=std::ceil(bottom-top)+4;
    const float baseline=render->getBaseline()*render->getHeight()+2-top;
    result.baseline=std::isfinite(baseline)?std::clamp(baseline,0.0F,result.height):result.height*.5F;
    result.geometry=std::make_shared<Geometry>(Geometry{std::move(graphics.commands)});
  } catch(const std::exception& error){result.error=error.what();}
  if(impl_->cache.size()==16)impl_->cache.pop_back();
  impl_->cache.push_front(std::move(cached));return impl_->cache.front().equation;
}
void NativeMath::draw(const Equation& equation,float x,float y,std::uint32_t colour) const {
  if(!equation.geometry)return;
  auto* draw=ImGui::GetWindowDrawList();
  for(const auto& command:equation.geometry->commands) {
    auto points=command.points;for(auto& p:points){p.x+=x;p.y+=y;}
    const auto tint=multiplyColour(command.colour,colour);
    switch(command.kind) {
      case Command::Kind::Glyph: {
        const auto& g=glyph(command.font,command.code);
        draw->AddImageQuad(command.font->OwnerAtlas->TexRef,points[0],points[1],points[2],points[3],{g.U0,g.V0},{g.U1,g.V0},{g.U1,g.V1},{g.U0,g.V1},tint);break;
      }
      case Command::Kind::Line:draw->AddLine(points[0],points[1],tint,command.thickness);break;
      case Command::Kind::Fill:draw->AddConvexPolyFilled(points.data(),4,tint);break;
    }
  }
}
const NativeMath::Document& NativeMath::layoutDocument(std::string_view source,float wrapWidth) {
  if(ImGui::GetCurrentContext()!=impl_->context)throw std::runtime_error("Native math used with the wrong UI context");
  auto* font=ImGui::GetFont();const float pixels=ImGui::GetFontSize();
  wrapWidth=std::max(1.0F,wrapWidth);
  if(impl_->document && impl_->document->source==source && impl_->wrapWidth==wrapWidth && impl_->proseFont==font && impl_->prosePixels==pixels)return *impl_->document;
  Document result;result.source=source;
  using Kind=Document::Part::Kind;
  const auto part=[&](Kind kind,std::size_t begin,std::size_t end,std::string error={}) {
    result.parts.push_back({kind,begin,end,{}});result.parts.back().equation.error=std::move(error);
  };
  std::size_t cursor=0;
  while(const auto open=nextDelimiter(source,cursor)) {
    if(open->begin>cursor)part(Kind::Text,cursor,open->begin);
    const auto closer=[](std::string_view token){return token.back()==')' || token.back()==']';};
    const bool literal=open->token.starts_with(R"(\\)");
    if(closer(open->token)) {
      part(Kind::Source,open->begin,open->end,"Closing math delimiter has no opening delimiter");cursor=open->end;continue;
    }
    const auto close=nextDelimiter(source,open->end,!literal);
    auto expected=std::string(open->token);
    if(expected.back()=='(')expected.back()=')';else if(expected.back()=='[')expected.back()=']';
    if(!close || close->token!=expected) {
      // Recover at a mismatched closer or before a fresh opener. Do not repair
      // the notation or absorb a later, independently delimited equation.
      const auto end=!close?source.size():(closer(close->token)?close->end:close->begin);
      part(Kind::Source,open->begin,end,"Math delimiters do not match");cursor=end;continue;
    }
    if(literal) {
      part(Kind::Source,open->begin,close->end,"Escaped math delimiters are displayed literally");cursor=close->end;continue;
    }
    const bool display=open->token==R"(\[)" || open->token=="$$";
    part(display?Kind::DisplayMath:Kind::InlineMath,open->begin,close->end);
    auto& formula=result.parts.back();formula.equation=layout(source.substr(open->end,close->begin-open->end),display?18.0F:16.0F,display);
    if(!formula.equation.error.empty())formula.kind=Kind::Source;
    cursor=close->end;
  }
  if(cursor<source.size())part(Kind::Text,cursor,source.size());
  const float ascent=font->GetFontBaked(pixels)->Ascent;
  const float space=font->CalcTextSizeA(pixels,1e9F,0," ").x;
  float x=0,y=0,above=ascent,below=pixels-ascent;
  std::size_t line=0;bool pendingSpace=false;
  const auto newline=[&] {
    if(line==result.placements.size()){pendingSpace=false;return;}
    for(auto i=line;i<result.placements.size();++i)result.placements[i].y=y+above-result.placements[i].baseline;
    y+=above+below+5;x=0;above=ascent;below=pixels-ascent;line=result.placements.size();pendingSpace=false;
  };
  const auto place=[&](std::size_t index,std::size_t begin,std::size_t end,float width,float height,float baseline) {
    if(x>0 && x+(pendingSpace?space:0)+width>wrapWidth)newline();
    if(x>0 && pendingSpace)x+=space;
    result.placements.push_back({index,begin,end,x,0,width,height,baseline});
    above=std::max(above,baseline);below=std::max(below,height-baseline);
    x+=width;result.width=std::max(result.width,x);pendingSpace=false;
  };
  for(std::size_t index=0;index<result.parts.size();++index) {
    const auto& p=result.parts[index];
    if(p.kind==Kind::InlineMath || p.kind==Kind::DisplayMath) {
      const bool display=p.kind==Kind::DisplayMath;
      if(display){newline();y+=3;}
      place(index,p.begin,p.end,p.equation.width,p.equation.height,p.equation.baseline);++result.equations;
      if(display){newline();y+=3;}
    } else {
      if(p.kind==Kind::Source)++result.fallbacks;
      for(auto i=p.begin;i<p.end;) {
        if(std::string_view(" \t\r\n").find(source[i])!=std::string_view::npos) {
          int breaks=0;
          do {breaks+=source[i]=='\n';++i;}while(i<p.end && std::string_view(" \t\r\n").find(source[i])!=std::string_view::npos);
          pendingSpace=true;
          if(breaks>1){newline();if(y>0)y+=6;}
          continue;
        }
        const auto begin=i;
        while(i<p.end && std::string_view(" \t\r\n").find(source[i])==std::string_view::npos)++i;
        const float width=font->CalcTextSizeA(pixels,1e9F,0,source.data()+begin,source.data()+i).x;
        place(index,begin,i,width,pixels,ascent);
      }
    }
  }
  newline();result.height=std::max(pixels,y-5);
  impl_->document=std::move(result);impl_->proseFont=font;impl_->prosePixels=pixels;impl_->wrapWidth=wrapWidth;
  return *impl_->document;
}
void NativeMath::draw(const Document& document,float x,float y,std::uint32_t prose,std::uint32_t math,std::uint32_t source) const {
  auto* draw=ImGui::GetWindowDrawList();
  for(const auto& p:document.placements) {
    const auto& part=document.parts[p.part];
    if(part.kind==Document::Part::Kind::InlineMath || part.kind==Document::Part::Kind::DisplayMath)
      this->draw(part.equation,x+p.x,y+p.y,math);
    else {
      draw->AddText(impl_->proseFont,impl_->prosePixels,{x+p.x,y+p.y},part.kind==Document::Part::Kind::Source?source:prose,document.source.data()+p.begin,document.source.data()+p.end);
      if(!part.equation.error.empty() && ImGui::IsMouseHoveringRect({x+p.x,y+p.y},{x+p.x+p.width,y+p.y+p.height}))
        ImGui::SetTooltip("Source shown: %s",part.equation.error.c_str());
    }
  }
}
} // namespace paths
