#include "LessonSpread.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
LessonSpread planLessonSpread(const LessonSpreadRequest& r){
  if(!std::isfinite(r.width)||!std::isfinite(r.height)||r.width<320||r.height<320||
     !std::isfinite(r.readingFraction)||r.readingFraction<.3f||r.readingFraction>.7f||
     static_cast<unsigned>(r.presentation)>static_cast<unsigned>(LessonPresentation::Figure))
    throw std::invalid_argument("Invalid textbook window or presentation.");
  constexpr float top=146,bottom=60,gap=8;
  LessonSpread out;out.header={0,0,r.width,top-gap};
  // A contents rail must not consume the small-window reading/figure workspace.
  out.showContents=r.contents&&(!r.hasFigure||r.width>=1200)&&r.width>=740;
  const float sidebar=out.showContents?std::clamp(r.width*.18f,205.f,250.f):0;
  out.contents={0,top,sidebar,r.height-top};
  const SceneViewport body{sidebar+gap,top,r.width-sidebar-2*gap,r.height-top-bottom-gap};
  out.footer={body.x,r.height-bottom,body.width,bottom};
  out.compact=body.width<900;
  out.split=r.hasFigure&&r.presentation==LessonPresentation::Together&&!out.compact;
  out.showFigure=r.hasFigure&&(out.split||r.presentation==LessonPresentation::Figure);
  out.showReading=!out.showFigure||out.split;
  if(out.split){
    const float left=std::clamp((body.width-gap)*r.readingFraction,400.f,body.width-gap-440.f);
    out.reading={body.x,body.y,left,body.height};
    out.divider={body.x+left,body.y,gap,body.height};
    out.figure={body.x+left+gap,body.y,body.width-left-gap,body.height};
  }else if(out.showFigure)out.figure=body;
  else out.reading=body;
  return out;
}
LessonFigureRegions planLessonFigureRegions(SceneViewport p){
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.width)||!std::isfinite(p.height)||p.x<0||p.y<0||p.width<100||p.height<100)
    throw std::invalid_argument("Invalid lesson figure panel.");
  const float title=std::clamp(p.height*.14f,60.f,90.f);
  if(p.width>=950){
    const float controls=std::clamp(p.width*.28f,300.f,390.f);
    return {{p.x,p.y,p.width,title},{p.x,p.y+title+4,p.width-controls-8,p.height-title-4},{p.x+p.width-controls,p.y+title+4,controls,p.height-title-4}};
  }
  const float controls=std::clamp(p.height*.34f,80.f,255.f);
  const float view=std::max(1.f,p.height-title-controls-8);
  return {{p.x,p.y,p.width,title},{p.x,p.y+title+4,p.width,view},{p.x,p.y+title+view+8,p.width,p.height-title-view-8}};
}
} // namespace paths
