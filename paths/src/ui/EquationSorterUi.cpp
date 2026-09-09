#include "ui/EquationSorterUi.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <imgui.h>
#include <imgui_internal.h>

namespace paths {
namespace {
struct SolveLayout { SceneViewport problem, working, controls, prompt, stage, support; float scale=1; };
SolveLayout solveLayout(ImVec2 size,bool graph=false,bool system=false) {
  const bool narrow=size.x<700;
  const float side=narrow?0:std::floor(std::clamp(size.x*.34F,280.0F,400.0F));
  const float mainWidth=size.x-side, controlsHeight=narrow?68.0F:38.0F;
  const float remaining=std::max(1.0F,size.y-controlsHeight);
  const float below=narrow?std::floor(graph?std::min(64.0F,remaining*.25F):std::min(std::max(88.0F,remaining*.25F),remaining*.35F)):0;
  const float mainHeight=remaining-below;
  const float scale=graph?1.0F:std::max(1.0F,std::min(mainWidth/600.0F,mainHeight/400.0F));
  const float height=std::floor(narrow || graph?mainHeight:std::min(mainHeight,400*scale));
  const float top=controlsHeight+std::floor((mainHeight-height)*.5F);
  const float problemHeight=system?74:graph?48:std::floor(64*scale), workingHeight=graph?30:std::floor(52*scale), promptHeight=graph?32:std::floor(48*scale);
  SolveLayout r{{0,top,mainWidth,problemHeight},{0,top+problemHeight,mainWidth,workingHeight},
      {0,0,size.x,controlsHeight},{0,top+problemHeight+workingHeight,mainWidth,promptHeight},{},{},scale};
  const float activityTop=r.prompt.y+r.prompt.height;
  r.stage={0,activityTop,mainWidth,std::max(1.0F,top+height-activityTop)};
  r.support=narrow?SceneViewport{0,top+height,size.x,below}:SceneViewport{mainWidth,top,side,height};
  return r;
}
SorterCardBounds beginSolvePanel(const char* name,SceneViewport rect,ImGuiWindowFlags flags) {
  ImGui::SetCursorScreenPos({rect.x,rect.y});
  ImGui::BeginChild(name,{rect.width,rect.height},ImGuiChildFlags_NavFlattened|ImGuiChildFlags_AlwaysUseWindowPadding,flags);
  const auto position=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
  return {0,position.x,position.y,size.x,size.y,true};
}
float fittedFont(std::string_view text,float largest) {
  const auto available=ImGui::GetContentRegionAvail();
  for(float size=largest;size>14;--size) {
    ImGui::PushFont(nullptr,size);
    const bool fits=ImGui::CalcTextSize(text.data(),text.data()+text.size(),false,available.x).y<=available.y;
    ImGui::PopFont();
    if(fits)return size;
  }
  return 14;
}
void centerLine(std::string_view text) {
  const float width=ImGui::CalcTextSize(text.data(),text.data()+text.size()).x;
  if(width<=ImGui::GetContentRegionAvail().x)ImGui::SetCursorPosX((ImGui::GetWindowWidth()-width)*.5F);
}
SorterCardBounds itemBounds(bool available=true) {
  const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  return {0,a.x,a.y,b.x-a.x,b.y-a.y,available};
}
void recoverFocus(ImGuiWindow* window,ImGuiID fallback,const ImRect& rectangle) {
  auto& nav=*ImGui::GetCurrentContext();
  if(!ImGui::GetIO().AppFocusLost && nav.NavWindow && nav.NavId &&
      (!nav.NavIdIsAlive || (nav.NavIdItemFlags & ImGuiItemFlags_Disabled))) {
    ImGui::SetNavWindow(window);
    ImGui::SetNavID(fallback,ImGuiNavLayer_Main,window->NavRootFocusScopeId,
                    ImGui::WindowRectAbsToRel(window,rectangle));
  }
}
constexpr std::array emphasisColours{ImVec4{1,.78F,.3F,1},ImVec4{.4F,.9F,.8F,1}};
void drawWorking(const GallerySessionView& view) {
  if(view.workingHighlights.empty() || view.working.find('\n')!=std::string_view::npos ||
      ImGui::CalcTextSize(view.working.data()).x>ImGui::GetContentRegionAvail().x) {
    ImGui::TextWrapped("%s",view.working.data());return;
  }
  ImGui::BeginGroup();
  bool first=true;
  const auto text=[&](std::size_t begin,std::size_t end,ImVec4 colour) {
    if(begin==end)return;
    if(!first)ImGui::SameLine(0,0);
    ImGui::PushStyleColor(ImGuiCol_Text,colour);
    ImGui::TextUnformatted(view.working.data()+begin,view.working.data()+end);
    ImGui::PopStyleColor();first=false;
  };
  std::size_t end=0,index=0;
  for(const auto& span:view.workingHighlights) {
    text(end,span.offset,ImGui::GetStyleColorVec4(ImGuiCol_Text));
    end=static_cast<std::size_t>(span.offset)+span.length;
    text(span.offset,end,emphasisColours[index++%emphasisColours.size()]);
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",span.label.c_str());
  }
  text(end,view.working.size(),ImGui::GetStyleColorVec4(ImGuiCol_Text));
  ImGui::EndGroup();
}
void drawCoordinateGraph(EquationSorterUiState& ui,const GallerySession& game) {
  namespace fm=iggy3d::first_move;
  const auto& run=game.question().currentRun();const auto& io=ImGui::GetIO();
  auto graph=*game.question().coordinateGraph(ui.graphProbeX);
  const bool system=graph.second.has_value(),paused=game.scene().paused();
  if(ui.graphQuestion!=run.questionId || ui.graphRun!=run.runNumber) {
    ui.graphQuestion=run.questionId;ui.graphRun=run.runNumber;ui.graphProbeX=0;
    ui.graphStage=graph.stage;ui.graphMotion=0;
    graph=*game.question().coordinateGraph(0);
  }
  if(ui.graphStage!=graph.stage) {ui.graphStage=graph.stage;ui.graphMotion=0;}
  ui.graphProbeX=graph.probe.x;
  if(!paused && !io.AppFocusLost)ui.graphMotion=std::min(1.0F,ui.graphMotion+io.DeltaTime/.28F);
  const float progress=ui.graphMotion*ui.graphMotion*(3-2*ui.graphMotion);
  const auto available=ImGui::GetContentRegionAvail();
  const auto position=ImGui::GetCursorScreenPos();
  ui.graphBoard={0,position.x,position.y,available.x,std::max(24.0F,available.y-78),true};
  const auto board=ui.graphBoard;const auto& g=graph.axes;
  const bool compact=board.height<200;
  const float textSize=compact?11:13, rowHeight=compact?18:22, headerHeight=textSize+3;
  const float readoutHeight=(system?2:1)*(textSize+3), tableWidth=std::clamp(board.width*.28F,132.0F,180.0F);
  const SorterCardBounds canvas{0,board.x,board.y+readoutHeight,board.width-tableWidth-8,board.height-readoutHeight,true};
  ui.graphTable={0,board.x+board.width-tableWidth,canvas.y,tableWidth,headerHeight+4*rowHeight,true};
  const auto table=ui.graphTable;
  auto* draw=ImGui::GetWindowDrawList();
  constexpr auto teal=IM_COL32(62,199,211,255),violet=IM_COL32(186,142,255,255),mint=IM_COL32(102,230,186,255);
  constexpr auto ink=IM_COL32(215,223,230,255),amber=IM_COL32(255,196,75,255),muted=IM_COL32(135,151,163,255);
  draw->AddRectFilled({board.x,board.y},{board.x+board.width,board.y+board.height},IM_COL32(12,20,28,255),5);
  draw->AddRect({board.x,board.y},{board.x+board.width,board.y+board.height},IM_COL32(44,83,93,255),5);
  ImGui::BeginDisabled(paused);
  ImGui::SetCursorScreenPos({canvas.x,canvas.y});
  ImGui::InvisibleButton("##coordinate_board",{canvas.width,canvas.height});
  const float margin=std::min(24.0F,canvas.height*.12F);
  const float unit=std::min((canvas.width-2*margin)/(g.xMax-g.xMin),(canvas.height-2*margin)/(g.yMax-g.yMin));
  const float width=unit*(g.xMax-g.xMin),height=unit*(g.yMax-g.yMin);
  const ImVec2 top{canvas.x+(canvas.width-width)*.5F,canvas.y+(canvas.height-height)*.5F};
  ui.graphPlot={0,top.x,top.y,width,height,true};
  const auto at=[&](fm::GraphPoint point) {return ImVec2{top.x+(point.x-g.xMin)*unit,top.y+(g.yMax-point.y)*unit};};
  if(graph.probeAvailable && ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    ui.graphProbeX=g.xMin+(io.MousePos.x-top.x)/unit;
  }
  // All three inputs change the same requested x before any linked values are drawn.
  ImGui::BeginDisabled(!graph.valueTable);
  for(std::size_t i=0;i<ui.graphSampleRows.size();++i) {
    ImGui::SetCursorScreenPos({table.x,table.y+headerHeight+i*rowHeight});
    if(ImGui::Button(std::array{"##graph_sample_0","##graph_sample_1","##graph_sample_2"}[i],{table.width,rowHeight-1}) && graph.valueTable)
      ui.graphProbeX=(*graph.valueTable)[i].x;
    ui.graphSampleRows[i]=itemBounds(graph.valueTable && !paused && !io.AppFocusLost && !ui.solvePointerHeld);
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Move the x guide to this row. Values are rounded to two decimal places.");
  }
  ImGui::EndDisabled();
  ImGui::SetCursorScreenPos({board.x,board.y+board.height+4});
  ImGui::BeginDisabled(graph.stage==fm::GraphStage::Grid);
  if(ImGui::Button("Replay",{68,24}))ui.graphMotion=0;
  ui.graphReplay=itemBounds(graph.stage!=fm::GraphStage::Grid && !paused);ImGui::EndDisabled();
  if(ImGui::IsItemHovered())ImGui::SetTooltip("Replay the latest graph movement");
  ImGui::SameLine(0,8);ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  ImGui::BeginDisabled(!graph.probeAvailable);
  ImGui::SliderFloat("##graph_x",&ui.graphProbeX,graph.probeMin,graph.probeMax,"x = %.2f",ImGuiSliderFlags_AlwaysClamp);
  ui.graphSlider=itemBounds(graph.probeAvailable && !paused);ImGui::EndDisabled();
  ImGui::EndDisabled();
  graph=*game.question().coordinateGraph(ui.graphProbeX);ui.graphProbeX=graph.probe.x;

  const auto label=[&](ImVec2 pos,const char* text,ImU32 colour=ink) {
    const float size=board.height<160?11:14;
    const auto extent=ImGui::GetFont()->CalcTextSizeA(size,1e6F,0,text);
    pos.x=std::clamp(pos.x,canvas.x+3,std::max(canvas.x+3,canvas.x+canvas.width-extent.x-3));
    pos.y=std::clamp(pos.y,canvas.y+2,std::max(canvas.y+2,canvas.y+canvas.height-extent.y-2));
    draw->AddRectFilled({pos.x-1,pos.y-1},{pos.x+extent.x+1,pos.y+extent.y+1},IM_COL32(12,20,28,235),2);
    draw->AddText(ImGui::GetFont(),size,pos,colour,text);
  };
  const auto origin=at({0,0});char text[64];
  const int stride=unit<18?2:1;
  for(int x=g.xMin;x<=g.xMax;++x) {
    const auto a=at({static_cast<float>(x),static_cast<float>(g.yMin)}),b=at({static_cast<float>(x),static_cast<float>(g.yMax)});
    draw->AddLine(a,b,x?IM_COL32(39,52,63,255):ink,x?1:1.5F);
    if(x && x%stride==0) {std::snprintf(text,sizeof(text),"%d",x);label({a.x+2,origin.y+3},text,IM_COL32(135,151,163,255));}
  }
  for(int y=g.yMin;y<=g.yMax;++y) {
    const auto a=at({static_cast<float>(g.xMin),static_cast<float>(y)}),b=at({static_cast<float>(g.xMax),static_cast<float>(y)});
    draw->AddLine(a,b,y?IM_COL32(39,52,63,255):ink,y?1:1.5F);
    if(y && y%stride==0) {std::snprintf(text,sizeof(text),"%d",y);label({origin.x+3,a.y-14},text,IM_COL32(135,151,163,255));}
  }
  label({origin.x+3,origin.y+3},"0");label({top.x+width+8,origin.y-16},"x");label({origin.x+5,top.y-19},"y");
  const auto start=at(graph.intercept),corner=at(graph.corner),tip=at(graph.tip);
  const auto stroke=[&](ImVec2 a,ImVec2 b,float amount,ImU32 colour,float thickness=3) {
    draw->AddLine(a,{a.x+(b.x-a.x)*amount,a.y+(b.y-a.y)*amount},colour,thickness);
  };
  if(!system && graph.stage>=fm::GraphStage::Rise && g.rise)
    draw->AddTriangleFilled(start,corner,tip,IM_COL32(62,199,211,24));
  if(!system && graph.stage>=fm::GraphStage::Run) {
    stroke(start,corner,graph.stage==fm::GraphStage::Run?progress:1,teal);
    std::snprintf(text,sizeof(text),"+%d",g.run);label({(start.x+corner.x)*.5F-5,start.y+6},text,teal);
  }
  if(!system && graph.stage>=fm::GraphStage::Rise) {
    stroke(corner,tip,graph.stage==fm::GraphStage::Rise?progress:1,teal);
    draw->AddCircleFilled(tip,graph.stage==fm::GraphStage::Rise?2+2*progress:4,mint);
    std::snprintf(text,sizeof(text),"%+d",g.rise);label({corner.x+7,(corner.y+tip.y)*.5F-16},text,teal);
  }
  if(graph.stage==fm::GraphStage::Line) {
    stroke(start,at(graph.lineStart),progress,mint);stroke(start,at(graph.lineEnd),progress,mint);
    const auto p=at(graph.probe);
    for(int dash=0;dash<10;dash+=2) {
      const float a=dash/10.0F,b=(dash+1)/10.0F;
      draw->AddLine({p.x,origin.y+(p.y-origin.y)*a},{p.x,origin.y+(p.y-origin.y)*b},IM_COL32(62,199,211,130));
      draw->AddLine({origin.x+(p.x-origin.x)*a,p.y},{origin.x+(p.x-origin.x)*b,p.y},IM_COL32(62,199,211,130));
    }
    draw->AddCircleFilled(p,6,teal);draw->AddCircle(p,9,ink);
  }
  if(!system && graph.stage>=fm::GraphStage::Intercept) {
    draw->AddCircleFilled(start,graph.stage==fm::GraphStage::Intercept?5*std::max(.2F,progress):5,mint);
    if(graph.stage!=fm::GraphStage::Line) {
      std::snprintf(text,sizeof(text),"(0, %d)",g.intercept);label({start.x+9,start.y-24},text,mint);
    }
  }
  if(system && graph.stage>=fm::GraphStage::FirstLine) {
    stroke(at(graph.lineStart),at(graph.lineEnd),graph.stage==fm::GraphStage::FirstLine?progress:1,teal);
    if(graph.stage>=fm::GraphStage::BothLines) {
      const auto a=at(graph.second->start),b=at(graph.second->end);
      const float amount=graph.stage==fm::GraphStage::BothLines?progress:1;
      // A dashed second line leaves both colours visible when the lines coincide.
      for(int dash=0;dash<40;dash+=2) {
        const float from=dash/40.0F,to=std::min((dash+1)/40.0F,amount);
        if(from>=amount)break;
        draw->AddLine({a.x+(b.x-a.x)*from,a.y+(b.y-a.y)*from},{a.x+(b.x-a.x)*to,a.y+(b.y-a.y)*to},violet,3);
      }
    }
    if(graph.probeAvailable) {
      const auto first=at(graph.probe),second=at(graph.second->probe);
      draw->AddLine({first.x,top.y},{first.x,top.y+height},IM_COL32(219,227,233,140),1.5F);
      draw->AddCircleFilled(first,5,teal);draw->AddCircle(second,8,violet,0,2);
      if(graph.stage>=fm::GraphStage::Classified) {
        draw->AddCircle(first,7+3*progress,ink,0,1.5F);
        draw->AddCircle(second,7+3*progress,ink,0,1.5F);
      }
    }
    if(graph.intersection) {
      const auto p=at(*graph.intersection);
      draw->AddCircle(p,7+3*progress,mint,0,3);draw->AddCircleFilled(p,4,mint);
      std::snprintf(text,sizeof(text),"(%.2f, %.2f)",graph.intersection->x,graph.intersection->y);label({p.x+12,p.y+10},text,mint);
    }
    if(graph.stage==fm::GraphStage::SystemSolution && graph.relation==fm::GraphRelation::Coincident)
      stroke(at(graph.lineStart),at(graph.lineEnd),progress,IM_COL32(102,230,186,95),7);
  }
  const auto number=[](float value) {
    char text[24];std::snprintf(text,sizeof(text),"%.2f",std::abs(value)<.005F?0.0F:value);return std::string(text);
  };
  const std::size_t columns=system?3:2;const float cellWidth=table.width/columns;
  const auto cell=[&](std::size_t column,float y,const char* text,ImU32 colour) {
    const auto extent=ImGui::GetFont()->CalcTextSizeA(textSize,1e6F,0,text);
    draw->AddText(ImGui::GetFont(),textSize,{table.x+(column+.5F)*cellWidth-extent.x*.5F,y},colour,text);
  };
  cell(0,table.y,"x",graph.probeAvailable?amber:muted);
  cell(1,table.y,system?"y1":"y",graph.probeAvailable?teal:muted);
  if(system)cell(2,table.y,"y2",graph.probeAvailable?violet:muted);
  ui.graphCurrentRow={0,table.x,table.y+headerHeight+3*rowHeight,table.width,rowHeight,graph.probeAvailable};
  if(graph.valueTable) {
    const fm::GraphValueRow current{graph.probe.x,graph.probe.y,graph.second?std::optional{graph.second->probe.y}:std::nullopt};
    ui.graphDisplayedValues=current;
    draw->AddRectFilled({ui.graphCurrentRow.x,ui.graphCurrentRow.y},
        {ui.graphCurrentRow.x+table.width,ui.graphCurrentRow.y+rowHeight},IM_COL32(255,196,75,38),3);
    draw->AddRect({ui.graphCurrentRow.x,ui.graphCurrentRow.y},
        {ui.graphCurrentRow.x+table.width,ui.graphCurrentRow.y+rowHeight},amber,3);
    for(std::size_t i=0;i<4;++i) {
      const auto& row=i==3?current:(*graph.valueTable)[i];const float y=table.y+headerHeight+i*rowHeight+(rowHeight-textSize)*.5F;
      cell(0,y,number(row.x).c_str(),i==3?amber:ink);cell(1,y,number(row.y).c_str(),teal);
      if(row.secondY)cell(2,y,number(*row.secondY).c_str(),violet);
    }
    for(std::size_t i=0;i<(system?2U:1U);++i) {
      const auto line=i?*g.second:fm::GraphLine{g.rise,g.run,g.intercept};
      char prefix[48],suffix[64];std::snprintf(prefix,sizeof(prefix),"%s ~ (%d/%d)(",system?i?"y2":"y1":"y",line.rise,line.run);
      std::snprintf(suffix,sizeof(suffix),") + (%d) ~ %s",line.intercept,number(i?*current.secondY:current.y).c_str());
      const float startX=board.x+4,y=board.y+i*(textSize+3);float x=startX;
      const auto part=[&](const char* text,ImU32 colour) {
        draw->AddText(ImGui::GetFont(),textSize,{x,y},colour,text);
        x+=ImGui::GetFont()->CalcTextSizeA(textSize,1e6F,0,text).x;
      };
      part(prefix,i?violet:teal);part(number(current.x).c_str(),amber);part(suffix,i?violet:teal);
      ui.graphReadouts[i]={0,startX,y,x-startX,textSize,true};
    }
  }
}

void drawMathReference(EquationSorterUiState& ui,const iggy3d::first_move::MathReference& reference,bool blocked,bool paused) {
  const auto& example=*ui.mathExample;
  const ImVec4 violet{.76F,.65F,1,1},cyan{.3F,.85F,.95F,1},green{.4F,.9F,.65F,1};
  ImGui::BeginDisabled(blocked);
  constexpr std::array labels{"Close","<",">"};
  auto* window=ImGui::GetCurrentWindow();ImGuiID closeId=0;ImRect closeRect;
  for(std::size_t i=0;i<labels.size();++i) {
    if(i)ImGui::SameLine();
    const bool available=i==0 || (!paused && (i==1?ui.mathReferenceStep>0:ui.mathReferenceStep<3));
    ImGui::BeginDisabled(!available);
    if(ImGui::Button(labels[i],{i==0?46.0F:26.0F,24})) {
      switch(i) {
        case 0:ui.mathReferenceId.clear();break;
        case 1:--ui.mathReferenceStep;ui.mathReferenceFollow=true;break;
        case 2:++ui.mathReferenceStep;ui.mathReferenceFollow=true;break;
      }
    }
    ui.mathReferenceControls[i]=itemBounds(!blocked && available);
    if(!i) {closeId=ImGui::GetItemID();closeRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};}
    ImGui::EndDisabled();
  }
  ImGui::SameLine();ImGui::TextColored(violet,"REFERENCE  %zu/3",ui.mathReferenceStep);
  ImGui::EndDisabled();
  if(ui.mathReferenceFollow && !ui.mathReferenceStep)ImGui::SetNextWindowScroll({0,0});
  ImGui::BeginChild("Reference text",{0,0},ImGuiChildFlags_NavFlattened,ImGuiWindowFlags_NoSavedSettings);
  const auto at=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();ui.mathReferenceBody={0,at.x,at.y,size.x,size.y,true};
  ImGui::TextColored(violet,"%s",reference.title.c_str());
  ImGui::TextWrapped("%s",reference.definition.c_str());
  ImGui::TextWrapped("%s",reference.rule.c_str());
  ImGui::Separator();ImGui::TextDisabled("SEPARATE EXAMPLE");
  ImGui::TextColored(cyan,"%s",example.operation.c_str());
  const auto matrix=[&](bool result) {
    ImGui::BeginGroup();ImGui::TextDisabled("%s",result?"Example working":"Before");
    if(ImGui::BeginTable(result?"Reference result":"Reference source",4,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInnerV)) {
      for(std::size_t row=0;row<2;++row) {
        ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::TextDisabled("R%zu",row+1);
        for(std::size_t col=0;col<3;++col) {
          ImGui::TableNextColumn();
          const bool changed=result && col<ui.mathReferenceStep,focus=ui.mathReferenceStep && col==ui.mathReferenceStep-1;
          const auto colour=result && ui.mathReferenceStep==3?green:focus?cyan:changed?green:ImVec4{.72F,.77F,.82F,1};
          ImGui::TextColored(colour,"%s",(changed?example.after:example.before)[row][col].c_str());
        }
      }
      ImGui::EndTable();
    }
    ImGui::EndGroup();ui.mathReferenceMatrices[result?1:0]=itemBounds();
  };
  matrix(false);
  ImGui::BeginGroup();
  if(ui.mathReferenceStep) {
    constexpr std::array columns{"x column","y column","Right-hand column"};
    ImGui::TextDisabled("%s",columns[ui.mathReferenceStep-1]);
    ImGui::PushStyleColor(ImGuiCol_Text,cyan);ImGui::TextWrapped("%s",example.calculations[ui.mathReferenceStep-1].c_str());ImGui::PopStyleColor();
  } else ImGui::TextDisabled("Use > to follow each column.");
  ImGui::EndGroup();ui.mathReferenceCalculation=itemBounds();
  if(ui.mathReferenceFollow && ui.mathReferenceStep)ImGui::SetScrollHereY(.4F);
  matrix(true);
  ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  ImGui::TextWrapped("%s",ui.mathReferenceStep==3?"Example complete. Return to your question.":"Partial example; finish all three columns.");ImGui::PopStyleColor();
  ui.mathReferenceFollow=false;ImGui::EndChild();
  recoverFocus(window,closeId,closeRect);
}

void drawMathSolving(EquationSorterUiState& ui,const SorterView& sorter,const GallerySession& game) {
  namespace fm=iggy3d::first_move;
  const auto v=game.view();const auto& evidence=game.question().currentRun();const auto& math=*evidence.math;
  const auto& active=math.nodes[math.active];const auto& io=ImGui::GetIO();
  const bool narrow=io.DisplaySize.x<700, split=io.DisplaySize.x>=900;
  const bool matrix=std::holds_alternative<fm::AugmentedMatrix>(active.equation);
  const float header=narrow?66:38, boardHeight=matrix?76:56, workingHeight=50, inputHeight=matrix?226:210;
  const float resultHeight=matrix?42:34, nodeHeight=matrix?50:30;
  const float historyTop=header+boardHeight+workingHeight+inputHeight;
  const auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
  const bool blocked=io.AppFocusLost || ui.solvePointerHeld;
  const bool full=math.nodes.size()>=fm::kMathNodeCapacity || math.events.size()>=fm::kMathEventCapacity;
  const bool canUndo=math.active && !v.paused && math.events.size()<fm::kMathEventCapacity;
  const auto queue=[&](GalleryCommand command) {if(!blocked && !ui.pending)ui.pending=std::move(command);};
  const auto move=[&](fm::MathMoveKind kind,const fm::MathMoveChoice* choice=nullptr,std::string entry={}) {
    queue(MathematicalMove{v.challenge,{kind,choice?choice->operation:fm::MathOperation::Expand,
        choice?choice->operand:std::string{},std::move(entry),evidence.runNumber,math.revision,evidence.questionId,evidence.contentVersion}});
  };
  if(ui.mathQuestion!=evidence.questionId || ui.mathRun!=evidence.runNumber) {
    ui.mathQuestion=evidence.questionId;ui.mathRun=evidence.runNumber;ui.mathRevision=0;
    ui.mathChoices.clear();ui.mathSelectedMove.reset();ui.mathInspected.reset();
    ui.mathReferenceId.clear();ui.mathExample.reset();ui.mathReferenceStep=0;
    ImGui::ClearActiveID();
  }
  if(ui.mathRevision!=math.revision) {
    if(!math.events.empty() && (math.events.back().correct || math.events.back().kind==fm::MathMoveKind::Undo)) {
      ui.mathSelectedMove.reset();ui.mathInspected.reset();
      ImGui::ClearActiveID();
    }
    ui.mathChoices=game.question().mathMoveChoices();
    ui.mathRevision=math.revision;ui.mathFollow=true;
  }
  ui.shootAvailable=false;ui.solveOptionCount=0;ui.solveAnswers={};ui.solveControls={};ui.mathNodes={};
  ui.mathOperations={};ui.mathResults={};
  ui.mathReferenceButton={};ui.mathReferencePanel={};ui.mathReferenceBody={};ui.mathReferenceCalculation={};
  ui.mathReferenceControls={};ui.mathReferenceMatrices={};
  ui.solveHelp={};ui.solveVerification={};ui.mathInspection={};ui.graphBoard={};ui.graphPlot={};
  ui.graphTable={};ui.graphDisplayedValues.reset();ui.graphSlider={};ui.graphReplay={};
  ui.graphSampleRows={};ui.graphCurrentRow={};ui.graphReadouts={};
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
  ImGui::Begin("Solving workspace",nullptr,flags|ImGuiWindowFlags_NoScrollWithMouse);
  ImGui::PopStyleVar();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12,6});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6,4});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,4);
  ImGui::PushStyleColor(ImGuiCol_ChildBg,{.045F,.065F,.085F,1});
  ImGui::PushFont(nullptr,15);ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat,false);
  beginSolvePanel("Math header",{0,0,io.DisplaySize.x,header},flags);
  auto* headerWindow=ImGui::GetCurrentWindow();ImGuiID backId=0;ImRect backRect;
  ImGui::BeginDisabled(blocked);
  const float width=std::min(150.0F,(ImGui::GetContentRegionAvail().x-6*(narrow?2:4))/(narrow?3:5));
  if(ui.progressFailed)ImGui::PushStyleColor(ImGuiCol_Text,{1,.68F,.27F,1});
  if(ImGui::Button(sorter.studyRun?"Contents":"Groups",{width,24}))
    ui.pending=SorterAction{sorter.studyRun?SorterActionKind::ReturnToStudy:SorterActionKind::ReturnToSorter,SorterBucket::A,0,sorter.revision};
  ui.solveControls[0]=itemBounds(!blocked);backId=ImGui::GetItemID();backRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
  if(ui.progressFailed) {ImGui::PopStyleColor();if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",ui.progressMessage.c_str());}
  ImGui::SameLine();if(ImGui::Button(v.paused?"Resume":"Pause",{width,24}))queue(GalleryPause{!v.paused});
  ui.solveControls[1]=itemBounds(!blocked);ImGui::SameLine();ImGui::BeginDisabled(!canUndo);
  if(ImGui::Button("Undo move",{width,24}))move(fm::MathMoveKind::Undo);
  ui.mathUndo=itemBounds(canUndo && !blocked);ui.solveControls[2]=ui.mathUndo;ImGui::EndDisabled();
  if(!narrow)ImGui::SameLine();
  ImGui::BeginDisabled(!v.completed || v.paused);
  if(ImGui::Button("Play again",{width,24}))queue(ReplayQuestion{});
  ui.solveControls[4]=itemBounds(v.completed && !v.paused && !blocked);ImGui::EndDisabled();ImGui::SameLine();
  ImGui::BeginDisabled(!sorter.nextSolve || v.paused);
  if(ImGui::Button("Next problem",{width,24}) && !ui.pending)
    ui.pending=SorterAction{SorterActionKind::NextSolve,SorterBucket::A,0,sorter.revision};
  ui.solveControls[5]=itemBounds(sorter.nextSolve.has_value() && !v.paused && !blocked);ImGui::EndDisabled();
  ImGui::EndDisabled();ImGui::EndChild();

  ImGui::PushStyleColor(ImGuiCol_ChildBg,{.15F,.105F,.035F,1});
  ui.solveBoard=beginSolvePanel("Problem board",{0,header,io.DisplaySize.x,boardHeight},flags);
  ImGui::TextColored({1,.77F,.29F,1},"PROBLEM %zu / %zu",sorter.solveNumber,sorter.solveCount);
  if(matrix) {ImGui::SameLine();ImGui::TextDisabled("x, y | b");}
  drawMathNotationToggle(ui.notation,!game.question().content().notation.empty(),blocked);
  if(ui.notation.open)ui.mathReferenceId.clear();
  const std::string_view original=matrix?math.nodes.front().working.display:v.equation;
  ImGui::PushFont(nullptr,matrix?18:22);centerLine(original);ImGui::TextUnformatted(original.data(),original.data()+original.size());
  ui.solveEquation=itemBounds();ImGui::PopFont();ImGui::EndChild();ImGui::PopStyleColor();
  ui.solveWorking=beginSolvePanel("Current working",{0,header+boardHeight,io.DisplaySize.x,workingHeight},flags);
  ImGui::PushStyleColor(ImGuiCol_Text,{.3F,.85F,.95F,1});
  ImGui::PushFont(nullptr,fittedFont(v.working,matrix?18:20));centerLine(v.working);ImGui::TextWrapped("%s",v.working.data());ImGui::PopFont();
  ImGui::PopStyleColor();ImGui::EndChild();

  ui.solveStage=beginSolvePanel("Math moves",{0,header+boardHeight+workingHeight,io.DisplaySize.x,inputHeight},flags & ~ImGuiWindowFlags_NoScrollbar);
  ImGui::BeginDisabled(blocked || v.paused || v.completed || full);
  // Present the question owner's choices verbatim; the UI never derives maths.
  const auto openReference=[&](std::string_view conceptId) {
    if(const auto* ref=game.question().mathReference(conceptId)) {
      ui.mathExample=fm::mathReferenceExample(*ref);ui.mathReferenceId=ref->id;
      ui.mathReferenceStep=0;ui.mathReferenceFollow=true;
    } else ui.mathReferenceId.clear();
  };
  ImGui::PushID(static_cast<int>(active.working.id.value));
  const float moveWidth=std::min(110.0F,(ImGui::GetContentRegionAvail().x-12)/3);
  const float moveLeft=(io.DisplaySize.x-(moveWidth*3+12))*.5F;
  for(std::size_t i=0;i<ui.mathChoices.size();++i) {
    const auto& choice=ui.mathChoices[i];
    ImGui::SetCursorPos({moveLeft+(i%3)*(moveWidth+6),6+(i/3)*30.0F});
    ImGui::PushStyleColor(ImGuiCol_Button,ui.mathSelectedMove==i?ImVec4{.08F,.39F,.46F,1}:ImVec4{.09F,.14F,.18F,1});
    if(ImGui::Button(choice.label.c_str(),{moveWidth,26})) {
      ui.mathSelectedMove=i;
      if(!ui.mathReferenceId.empty() && ui.mathReferenceId!=choice.conceptId)openReference(choice.conceptId);
    }
    ui.mathOperations[i]=itemBounds(!blocked && !v.paused && !v.completed && !full);ImGui::PopStyleColor();
  }
  ImGui::SetCursorPos({12,68});
  const char* prompt=ui.mathSelectedMove?(matrix?"Choose the resulting matrix.":"Choose the resulting equation."):"Choose a move above.";
  const auto* selected=ui.mathSelectedMove && *ui.mathSelectedMove<ui.mathChoices.size()?&ui.mathChoices[*ui.mathSelectedMove]:nullptr;
  if(!v.completed && selected && game.question().mathReference(selected->conceptId)) {
    const float lineWidth=ImGui::CalcTextSize(selected->label.c_str()).x+32;
    ImGui::SetCursorPos({(io.DisplaySize.x-lineWidth)*.5F,64});
    ImGui::AlignTextToFramePadding();ImGui::TextDisabled("%s",selected->label.c_str());ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,{.30F,.20F,.46F,1});
    if(ImGui::Button("?",{26,22})) {
      ui.notation.open=false;
      if(ui.mathReferenceId==selected->conceptId)ui.mathReferenceId.clear();else openReference(selected->conceptId);
    }
    ui.mathReferenceButton=itemBounds(!blocked && !v.paused && !full);ImGui::PopStyleColor();
  } else if(!v.completed) {centerLine(prompt);ImGui::TextDisabled("%s",prompt);}
  if(ui.mathSelectedMove && *ui.mathSelectedMove<ui.mathChoices.size()) {
    const auto& choice=ui.mathChoices[*ui.mathSelectedMove];
    ImGui::PushID(static_cast<int>(*ui.mathSelectedMove));
    const float resultWidth=std::min(265.0F,(ImGui::GetContentRegionAvail().x-6)/2);
    const float left=(io.DisplaySize.x-(resultWidth*2+6))*.5F;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1);
    ImGui::PushStyleColor(ImGuiCol_Border,{.25F,.62F,.64F,1});
    ImGui::PushStyleColor(ImGuiCol_Button,{.09F,.16F,.19F,1});
    for(std::size_t i=0;i<choice.results.size();++i) {
      ImGui::SetCursorPos({left+(i%2)*(resultWidth+6),90+(i/2)*(resultHeight+4)});
      if(ImGui::Button(choice.results[i].c_str(),{resultWidth,resultHeight}))move(fm::MathMoveKind::Submit,&choice,choice.results[i]);
      ui.mathResults[i]=itemBounds(!blocked && !v.paused && !v.completed && !full);
    }
    ImGui::PopStyleColor(2);ImGui::PopStyleVar();ImGui::PopID();
  }
  ImGui::PopID();ImGui::SetCursorPos({12,102+resultHeight*2});
  ImGui::EndDisabled();
  if(v.completed) {
    ImGui::PushStyleColor(ImGuiCol_Text,{.4F,.9F,.65F,1});ImGui::TextWrapped("%s",active.verification.c_str());
    ImGui::PopStyleColor();ui.solveVerification=itemBounds();
  } else if(full) {
    ImGui::TextWrapped("This attempt has reached its working limit. Your history is retained; start a fresh set from Contents.");
  } else if(!math.events.empty()) {
    const auto& event=math.events.back();
    ImGui::PushStyleColor(ImGuiCol_Text,event.correct || event.kind==fm::MathMoveKind::Undo?ImVec4{.4F,.9F,.65F,1}:ImVec4{1,.55F,.45F,1});
    ImGui::TextWrapped("%s",event.feedback.c_str());ImGui::PopStyleColor();
  } else ImGui::TextDisabled("%s",matrix?"R1, R2: rows. Columns: x, y | b.":"a(b+c): expand.  + - × ÷: both sides.");
  ImGui::EndChild();

  const float historyWidth=split?std::floor(io.DisplaySize.x*.64F):io.DisplaySize.x;
  const auto* reference=game.question().mathReference(ui.mathReferenceId);
  if(!split && (reference || ui.notation.open))ImGui::SetNextWindowScroll({0,0});
  ui.solveSupport=beginSolvePanel("Solution blueprint",{0,historyTop,historyWidth,std::max(50.0F,io.DisplaySize.y-historyTop)},
      !split && (reference || ui.notation.open)?flags|ImGuiWindowFlags_NoScrollWithMouse:flags & ~ImGuiWindowFlags_NoScrollbar);
  if(!split && (reference || ui.notation.open)) {
    if(ui.notation.open)drawMathNotation(ui.notation,game.question().content().notation,blocked,v.paused);
    else {ui.mathReferencePanel=ui.solveSupport;drawMathReference(ui,*reference,blocked,v.paused);}
    ImGui::EndChild();
  } else {
  ImGui::TextDisabled("WORKING    %zu checked moves    %zu attempts",math.nodes.size()-1,math.events.size());
  std::array<bool,fm::kMathNodeCapacity> branch{};
  for(std::size_t i=math.active;;i=math.nodes[i].parent) {branch[i]=true;if(!i)break;}
  auto* draw=ImGui::GetWindowDrawList();
  const auto detail=[&](std::size_t index) {
    const auto& node=math.nodes[index];
    ImGui::TextWrapped("%s",node.explanation.c_str());
    if(index) {
      ImGui::TextWrapped("%s",math.nodes[node.parent].working.display.c_str());
      ImGui::TextColored({.3F,.85F,.95F,1},"%s",node.operation.c_str());
      ImGui::TextWrapped("%s",node.working.display.c_str());
    }
    if(!branch[index])ImGui::TextDisabled("Earlier branch - retained after Undo");
    if(!node.verification.empty())ImGui::TextWrapped("%s",node.verification.c_str());
    if(ImGui::TreeNode("Attempts from this step")) {
      for(const auto& event:math.events)if(event.from==index) {
        ImGui::TextWrapped("%s: %s",event.kind==fm::MathMoveKind::Undo?"Undo":event.correct?"Checked":"Retry",event.entry.c_str());
        ImGui::TextWrapped("%s",event.feedback.c_str());
      }
      ImGui::TreePop();
    }
  };
  ImGui::BeginDisabled(blocked);
  for(std::size_t i=0;i<math.nodes.size();++i) {
    const auto& node=math.nodes[i];ImGui::PushID(static_cast<int>(i));
    const auto at=ImGui::GetCursorScreenPos();
    const ImVec4 colour=!i?ImVec4{1,.77F,.29F,1}:i==math.active && !v.completed?ImVec4{.3F,.85F,.95F,1}:
        branch[i]?ImVec4{.4F,.9F,.65F,1}:ImVec4{.55F,.63F,.7F,1};
    const auto& parent=ui.mathNodes[node.parent];
    if(i && ImGui::IsRectVisible({at.x+4,parent.y+parent.height*.5F-2},{at.x+22,at.y+17})) {
      draw->AddLine({at.x+6,parent.y+parent.height*.5F},{at.x+6,at.y+15},ImGui::ColorConvertFloat4ToU32(colour));
      draw->AddLine({at.x+6,at.y+15},{at.x+20,at.y+15},ImGui::ColorConvertFloat4ToU32(colour));
    }
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+22);
    const auto prefix=std::to_string(i)+"   ";std::string label=prefix+node.working.display;
    if(matrix)label.insert(label.find('\n')+1,prefix.size(),' ');
    ImGui::PushStyleColor(ImGuiCol_Text,colour);
    if(ImGui::Selectable(label.c_str(),ui.mathInspected.value_or(math.active)==i,0,{0,nodeHeight}))ui.mathInspected=i;
    ui.mathNodes[i]=itemBounds(!blocked && ImGui::IsItemVisible());ImGui::PopStyleColor();
    if(i && ImGui::IsRectVisible({at.x+6,at.y+9},{at.x+19,at.y+20})) {
      const ImVec2 tick{at.x+12,at.y+15};
      draw->AddLine({tick.x-4,tick.y},{tick.x-1,tick.y+3},ImGui::ColorConvertFloat4ToU32(colour),1.5F);
      draw->AddLine({tick.x-1,tick.y+3},{tick.x+5,tick.y-4},ImGui::ColorConvertFloat4ToU32(colour),1.5F);
    }
    if(i==math.active && ui.mathFollow)ImGui::SetScrollHereY(.8F);
    if(!split && ui.mathInspected==i) {ImGui::BeginGroup();detail(i);ImGui::EndGroup();ui.mathInspection=itemBounds();}
    ImGui::PopID();
  }
  ui.mathFollow=false;ImGui::EndDisabled();ImGui::EndChild();
  if(split) {
    if(reference || ui.notation.open)ImGui::SetNextWindowScroll({0,0});
    ui.mathInspection=beginSolvePanel("Working inspection",{historyWidth,historyTop,io.DisplaySize.x-historyWidth,std::max(50.0F,io.DisplaySize.y-historyTop)},
        reference || ui.notation.open?flags|ImGuiWindowFlags_NoScrollWithMouse:flags & ~ImGuiWindowFlags_NoScrollbar);
    if(ui.notation.open)drawMathNotation(ui.notation,game.question().content().notation,blocked,v.paused);
    else if(reference) {ui.mathReferencePanel=ui.mathInspection;drawMathReference(ui,*reference,blocked,v.paused);}
    else {
      const auto inspected=ui.mathInspected.value_or(math.active);
      ImGui::TextDisabled("STEP %zu",inspected);ImGui::BeginDisabled(blocked);detail(inspected);ImGui::EndDisabled();
    }
    ImGui::EndChild();
  }
  }
  recoverFocus(headerWindow,backId,backRect);
  ui.solveDisplayedChallenge=v.challenge;ui.solveCompleteVisible=v.completed;
  if(ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !blocked && !io.WantTextInput) {
    if(ui.notation.open)ui.notation.open=false;
    else if(reference)ui.mathReferenceId.clear();
    else ui.pending=SorterAction{sorter.studyRun?SorterActionKind::ReturnToStudy:SorterActionKind::ReturnToSorter,SorterBucket::A,0,sorter.revision};
  }
  ImGui::PopItemFlag();ImGui::PopFont();ImGui::PopStyleColor();ImGui::PopStyleVar(3);ImGui::End();
}
void drawSolving(EquationSorterUiState& ui,const SorterView& sorter,const GallerySession& game) {
  if(game.question().currentRun().math) {drawMathSolving(ui,sorter,game);return;}
  ui.mathOperations={};ui.mathResults={};ui.mathUndo={};ui.mathNodes={};ui.mathInspection={};
  const auto v=game.view();
  const auto& io=ImGui::GetIO();
  const bool graph=game.question().content().lineGraph.has_value();
  const bool system=graph && game.question().content().lineGraph->second.has_value();
  const auto layout=solveLayout(io.DisplaySize,graph,system);
  const float scale=layout.scale;
  const bool changed=ui.solveDisplayedChallenge!=v.challenge || ui.solveCompleteVisible!=v.completed;
  const bool helpChanged=ui.solveHintVisible!=!v.hint.empty() || ui.solveNextVisible!=!v.nextMove.empty();
  if(helpChanged && (!v.hint.empty() || !v.nextMove.empty()))ui.notation.open=false;
  const auto queue=[&](GalleryCommand command) { if(!ui.pending && !io.AppFocusLost)ui.pending=std::move(command); };
  const auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::Begin("Solving workspace",nullptr,flags|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollWithMouse);
  ImGui::PopStyleVar();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12,6});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{8,4});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,5);
  ImGui::PushStyleColor(ImGuiCol_ChildBg,{.055F,.065F,.085F,1});
  ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat,false);
  ui.solveOptionCount=0;ui.solveControls[4]={};ui.solveControls[5]={};
  ui.solveHelp={};ui.solveVerification={};ui.solveAnswers={};
  ui.solveInfinityVisible=false;
  ui.graphBoard={};ui.graphPlot={};ui.graphSlider={};ui.graphReplay={};
  ui.graphTable={};ui.graphCurrentRow={};ui.graphSampleRows={};ui.graphReadouts={};ui.graphDisplayedValues.reset();

  ImGui::PushStyleColor(ImGuiCol_ChildBg,{.16F,.115F,.045F,1});
  ui.solveBoard=beginSolvePanel("Problem board",layout.problem,flags);
  const auto board=ui.solveBoard;
  ImGui::GetWindowDrawList()->AddRect({board.x+1,board.y+1},{board.x+board.width-1,board.y+board.height-1},
      IM_COL32(255,196,75,255),0.0F,2.0F);
  ImGui::PushFont(nullptr,12*scale);
  if(graph)ImGui::TextColored({1,.77F,.29F,1},"PROBLEM %zu / %zu    STEP %zu / %zu",sorter.solveNumber,sorter.solveCount,v.step,v.stepCount);
  else ImGui::TextColored({1,.77F,.29F,1},"ORIGINAL PROBLEM   %zu / %zu",sorter.solveNumber,sorter.solveCount);
  ImGui::PopFont();
  drawMathNotationToggle(ui.notation,!game.question().content().notation.empty(),io.AppFocusLost || ui.solvePointerHeld);
  ImGui::PushFont(nullptr,fittedFont(v.equation,26*scale));centerLine(v.equation);
  ImGui::TextWrapped("%s",v.equation.data());ui.solveEquation=itemBounds();ImGui::PopFont();
  ImGui::EndChild();ImGui::PopStyleColor();

  ui.solveWorking=beginSolvePanel("Current working",layout.working,flags);
  if(!graph) {ImGui::PushFont(nullptr,12*scale);ImGui::TextColored({.4F,.9F,.8F,1},"CURRENT WORKING");ImGui::PopFont();}
  ImGui::PushFont(nullptr,fittedFont(v.working,22*scale));centerLine(v.working);
  drawWorking(v);ImGui::PopFont();ImGui::EndChild();

  beginSolvePanel("Solve header",layout.controls,flags);
  auto* headerWindow=ImGui::GetCurrentWindow();
  ImGuiID backFocus=0;ImRect backRectangle;
  ImGui::BeginDisabled(io.AppFocusLost || ui.solvePointerHeld);
  ImGui::PushFont(nullptr,15);
  const int columns=io.DisplaySize.x<700?2:4;
  const float width=(ImGui::GetContentRegionAvail().x-8*(columns-1))/columns;
  const std::array labels{sorter.studyRun?"Back to contents":"Back to groups",v.paused?"Resume":"Hint","Show next move","Do this step"};
  const std::array enabled{true,v.paused || (v.canHint && !v.transitioning),
      v.canReveal && !v.paused && !v.transitioning,v.canReveal && !v.paused && !v.transitioning};
  for(std::size_t i=0;i<labels.size();++i) {
    if(i%columns)ImGui::SameLine(0,8);
    ImGui::BeginDisabled(!enabled[i]);
    if(i==0 && ui.progressFailed)ImGui::PushStyleColor(ImGuiCol_Text,{1,.68F,.27F,1});
    if(ImGui::Button(labels[i],{width,26})) {
      if(i==0)ui.pending=SorterAction{sorter.studyRun?SorterActionKind::ReturnToStudy:SorterActionKind::ReturnToSorter,SorterBucket::A,0,sorter.revision};
      else if(i==1 && v.paused)queue(GalleryPause{false});
      else queue(GalleryHelp{v.challenge,static_cast<GalleryHelpKind>(i-1)});
    }
    ImGui::EndDisabled();ui.solveControls[i]=itemBounds(enabled[i]);
    if(i==0 && ui.progressFailed) {ImGui::PopStyleColor();if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",ui.progressMessage.c_str());}
    if(i==0) {backFocus=ImGui::GetItemID();backRectangle={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};}
  }
  ImGui::PopFont();ImGui::EndDisabled();ImGui::EndChild();

  beginSolvePanel("Solve prompt",layout.prompt,flags);
  ImGui::PushFont(nullptr,12*scale);
  if(!graph) {
    if(v.completed)ImGui::TextUnformatted(sorter.nextSolve?"PROBLEM SOLVED":sorter.studyRun?"SET COMPLETE":"LAST PREPARED PROBLEM SOLVED");
    else ImGui::Text("STEP %zu / %zu",v.step,v.stepCount);
  }
  if(!graph && (v.feedback==GalleryFeedback::Incorrect || v.feedback==GalleryFeedback::Miss)) {
    ImGui::SameLine(layout.prompt.width-70*scale);
    ImGui::TextColored({1,.5F,.45F,1},"%s",v.feedback==GalleryFeedback::Incorrect?"Wrong":"Miss");
  }
  ImGui::PopFont();ImGui::PushFont(nullptr,16*scale);
  const std::string_view prompt=v.completed?(system?"Compare both lines with the x guide.":graph?"Drag a point or use x below.":"Review the result for as long as you like."):v.prompt;
  centerLine(prompt);
  ImGui::PushStyleColor(ImGuiCol_Text,graph && v.feedback==GalleryFeedback::Incorrect?ImVec4{1,.5F,.45F,1}:ImGui::GetStyleColorVec4(ImGuiCol_Text));
  ImGui::TextWrapped("%s",prompt.data());ImGui::PopStyleColor();
  ImGui::PopFont();ImGui::EndChild();

  const bool calculation=!v.completed && v.purpose==iggy3d::first_move::StepPurpose::Calculation;
  const auto viewport=game.scene().frame().viewport;
  if(changed)ImGui::SetNextWindowScroll({-1,0});
  ui.solveStage=beginSolvePanel("Solve activity",layout.stage,(flags & ~ImGuiWindowFlags_NoScrollbar)|(graph?0:ImGuiWindowFlags_NoBackground));
  ImGui::BeginDisabled(io.AppFocusLost || ui.solvePointerHeld);
  ImGui::PushFont(nullptr,16*scale);
  if(graph)drawCoordinateGraph(ui,game);
  if(v.completed) {
    if(!graph) {
      ImGui::SetCursorPosY(8*scale);
      ImGui::TextWrapped("%s",sorter.nextSolve?"Choose Next when you are ready.":sorter.studyRun?"You have finished your selected questions.":"You have reached the last prepared question.");
    }
    const float buttonWidth=(ImGui::GetContentRegionAvail().x-8)*.5F;
    if(ImGui::Button("Play again",{buttonWidth,graph?32:48*scale}))queue(ReplayQuestion{});
    ui.solveControls[4]=itemBounds();
    ImGui::SameLine(0,8);ImGui::BeginDisabled(!sorter.nextSolve);
    if(ImGui::Button("Next problem",{buttonWidth,graph?32:48*scale}) && !ui.pending)
      ui.pending=SorterAction{SorterActionKind::NextSolve,SorterBucket::A,0,sorter.revision};
    ImGui::EndDisabled();ui.solveControls[5]=itemBounds(sorter.nextSolve.has_value());
  } else if(calculation) {
    const auto colours=galleryDisplayColours();
    for(std::size_t i=0;i<v.choiceCount;++i) {
      const auto& row=v.answers[i];
      const auto bodies=game.scene().objects();
      const auto body=std::find_if(bodies.begin(),bodies.end(),[&](const auto& b){return b.id==row.binding.object;});
      if(body==bodies.end())continue;
      const auto p=game.scene().project(body->position);
      const auto bottom=game.scene().project(body->position-iggy3d::Vec3{0,body->size.y*.5F,0});
      const auto& colour=colours[row.binding.token.value];
      const std::string label=std::string(colour.marker)+"  "+std::string(row.text);
      const auto textSize=ImGui::CalcTextSize(label.c_str());
      const ImVec2 at{std::clamp(viewport.x+p.x*viewport.width-textSize.x*.5F,viewport.x+4,
                                std::max(viewport.x+4,viewport.x+viewport.width-textSize.x-4)),
                      std::clamp(viewport.y+bottom.y*viewport.height+5,viewport.y+4,
                                std::max(viewport.y+4,viewport.y+viewport.height-textSize.y-4))};
      auto* draw=ImGui::GetWindowDrawList();
      draw->AddRectFilled({at.x-3,at.y-2},{at.x+textSize.x+3,at.y+textSize.y+2},IM_COL32(9,14,21,235),3);
      draw->AddText(at,ImGui::ColorConvertFloat4ToU32({colour.rgb.x,colour.rgb.y,colour.rgb.z,1}),label.c_str());
      ui.solveAnswers[i]={0,at.x,at.y,textSize.x,textSize.y,true};
    }
  } else {
    ImGui::PushFont(nullptr,system?15:graph?17:28*scale);
    const std::size_t columns=graph?v.choiceCount:2;
    const float gap=8*scale, rows=std::ceil(static_cast<float>(v.choiceCount)/columns);
    const float gridWidth=std::min(graph?layout.stage.width:560*scale,ImGui::GetContentRegionAvail().x),cellWidth=(gridWidth-gap*(columns-1))/columns;
    float cellHeight=graph?32:std::max(48*scale,(layout.stage.height-gap*(rows+1))/rows);
    for(std::size_t i=0;i<v.choiceCount;++i)
      cellHeight=std::max(cellHeight,ImGui::CalcTextSize(v.answers[i].text.data(),nullptr,false,cellWidth-20).y+12);
    const float left=(layout.stage.width-gridWidth)*.5F,top=graph?ImGui::GetCursorPosY():gap;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1);
    ImGui::PushStyleColor(ImGuiCol_Border,{.25F,.62F,.64F,1});
    ImGui::PushStyleColor(ImGuiCol_Button,{.09F,.16F,.19F,.97F});
    for(std::size_t i=0;i<v.choiceCount;++i) {
      const auto& row=v.answers[i];
      ImGui::PushID(static_cast<int>(row.binding.option.value));
      ImGui::SetCursorPos({left+(i%columns)*(cellWidth+gap),top+(i/columns)*(cellHeight+gap)});
      ImGui::BeginDisabled(v.paused || v.transitioning);
      const auto start=ImGui::GetCursorScreenPos();
      if(ImGui::Button("##operation",{cellWidth,cellHeight}))queue(ChooseAnswer{v.challenge,row.binding.option});
      ui.solveOptions[i]=itemBounds(!v.paused && !v.transitioning);
      ui.solveOptionIds[i]=row.binding.option;++ui.solveOptionCount;
      const auto textSize=ImGui::CalcTextSize(row.text.data(),nullptr,false,cellWidth-20);
      if(row.text=="∞") {
        // Native strokes keep this mathematical symbol independent of font coverage.
        const ImVec2 center{start.x+cellWidth*.5F,start.y+cellHeight*.5F};auto* draw=ImGui::GetWindowDrawList();
        for(float side:{-1.0F,1.0F})draw->AddBezierCubic(center,{center.x+20*side,center.y-20},
            {center.x+20*side,center.y+20},center,ImGui::GetColorU32(ImGuiCol_Text),2);
        ui.solveInfinityVisible=true;
      } else ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
            {start.x+(cellWidth-textSize.x)*.5F,start.y+(cellHeight-textSize.y)*.5F},
            ImGui::GetColorU32(ImGuiCol_Text),row.text.data(),nullptr,cellWidth-20);
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::PopStyleColor(2);ImGui::PopStyleVar();ImGui::PopFont();
  }
  ImGui::PopFont();ImGui::EndDisabled();ImGui::EndChild();

  if(changed || helpChanged || ui.notation.open)ImGui::SetNextWindowScroll({-1,0});
  ui.solveSupport=beginSolvePanel("Solve support",layout.support,flags & ~ImGuiWindowFlags_NoScrollbar);
  ImGui::BeginDisabled(io.AppFocusLost || ui.solvePointerHeld);
  ImGui::PushFont(nullptr,15*std::min(scale,1.3F));
  if(ui.notation.open)drawMathNotation(ui.notation,game.question().content().notation,io.AppFocusLost || ui.solvePointerHeld,v.paused);
  else {
  if(v.completed) {
    ImGui::BeginGroup();ImGui::SeparatorText("Check the result");
    if(system)ImGui::PushStyleColor(ImGuiCol_Text,{.4F,.9F,.73F,1});
    ImGui::TextWrapped("%s",v.verification.data());if(system)ImGui::PopStyleColor();
    ImGui::EndGroup();ui.solveVerification=itemBounds();
  } else if(!v.hint.empty() || !v.nextMove.empty()) {
    ImGui::BeginGroup();
    if(!v.hint.empty()) {ImGui::SeparatorText("Hint");ImGui::TextWrapped("%s",v.hint.data());}
    if(!v.nextMove.empty()) {ImGui::SeparatorText("Next move");ImGui::TextWrapped("%s",v.nextMove.data());}
    ImGui::EndGroup();ui.solveHelp=itemBounds();
  } else {
    ImGui::TextWrapped("%s",graph?"Choose a value below the graph. Each accepted step adds to this board.":calculation?"Shoot the sphere with the matching answer.":"Choose an operation in the activity area.");
    ImGui::TextDisabled("Hints and next moves appear here.");
  }
  if(ImGui::CollapsingHeader("Working so far",io.DisplaySize.x>=700?ImGuiTreeNodeFlags_DefaultOpen:0)) {
    const auto review=game.question().review(0);
    if(review)for(const auto& step:review->steps)ImGui::TextWrapped("%s",step.working.data());
    if(v.completed)ImGui::TextWrapped("%s",v.working.data());
  }
  for(std::size_t i=0;i<v.workingHighlights.size();++i) {
    ImGui::PushStyleColor(ImGuiCol_Text,emphasisColours[i%emphasisColours.size()]);
    ImGui::TextWrapped("%s",v.workingHighlights[i].label.c_str());ImGui::PopStyleColor();
  }
  if(v.step==1)ImGui::TextWrapped("%s",game.question().content().description.c_str());
  }
  ImGui::PopFont();ImGui::EndDisabled();ImGui::EndChild();
  recoverFocus(headerWindow,backFocus,backRectangle);
  ui.solveDisplayedChallenge=v.challenge;ui.solveHintVisible=!v.hint.empty();
  ui.solveNextVisible=!v.nextMove.empty();ui.solveCompleteVisible=v.completed;
  ui.shootAvailable=calculation && v.ready && !v.paused && !v.transitioning;
  ui.shootViewport=viewport;ui.shootFrame=game.scene().frame().id;ui.shootChallenge=v.challenge;
  if(ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !io.AppFocusLost) {
    if(ui.notation.open)ui.notation.open=false;
    else ui.pending=SorterAction{sorter.studyRun?SorterActionKind::ReturnToStudy:SorterActionKind::ReturnToSorter,SorterBucket::A,0,sorter.revision};
  }
  ImGui::PopItemFlag();ImGui::PopStyleColor();ImGui::PopStyleVar(3);
  ImGui::End();
}
void drawStudySelection(EquationSorterUiState& ui,const SorterView& view,std::span<const SorterEquation> content) {
  using Progress=iggy3d::first_move::QuestionProgress;
  const auto& io=ImGui::GetIO();const auto& study=view.study;
  const bool narrow=io.DisplaySize.x<700;
  const float footer=narrow?132.0F:102.0F, top=52, body=io.DisplaySize.y-top-footer;
  const float side=narrow?io.DisplaySize.x:std::floor(std::clamp(io.DisplaySize.x*.28F,220.0F,310.0F));
  const float tocHeight=narrow?std::min(140.0F,body*.42F):body;
  const SceneViewport toc{0,top,side,tocHeight};
  const SceneViewport questions=narrow?SceneViewport{0,top+tocHeight,side,body-tocHeight}:
      SceneViewport{side,top,io.DisplaySize.x-side,body};
  const auto queue=[&](SorterActionKind kind,std::uint32_t value=0,SorterEquationId id=0) {
    if(!ui.pending && !io.AppFocusLost)ui.pending=SorterAction{kind,SorterBucket::A,id,view.revision,value};
  };
  const auto bounds=[&](StudyControl control,bool enabled=true) {ui.studyControls[static_cast<std::size_t>(control)]=itemBounds(enabled);};
  ui.shootAvailable=false;ui.solveButton={};ui.cardCount=0;
  ui.studySubjects={};ui.studyChapters={};ui.studyChapterChecks={};ui.studyTypes={};ui.studyQuestions={};ui.studyControls={};
  ui.studyProgressMarks={};ui.studyChapterCounts={};
  if(!study.types.empty() && (!ui.studyChapter || std::none_of(study.types.begin(),study.types.end(),[&](const auto& t){return t.chapterId==*ui.studyChapter;})))
    ui.studyChapter=study.types[0].chapterId;
  const auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
  ImGui::PushStyleColor(ImGuiCol_WindowBg,{.055F,.065F,.085F,1});
  ImGui::Begin("Study selection",nullptr,flags);ImGui::PopStyleVar();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12,8});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{6,5});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{8,4});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,4);
  ImGui::PushStyleColor(ImGuiCol_ChildBg,{.055F,.065F,.085F,1});
  ImGui::PushStyleColor(ImGuiCol_CheckMark,{.4F,.9F,.8F,1});
  ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat,false);ImGui::BeginDisabled(io.AppFocusLost || ui.solvePointerHeld);
  ImGui::PushFont(nullptr,15);
  beginSolvePanel("Study heading",{0,0,io.DisplaySize.x,top},flags);
  ImGui::PushFont(nullptr,20);ImGui::TextColored({1,.78F,.3F,1},"Table of contents");ImGui::PopFont();
  ui.libraryEntry={};
  ui.motionEntry={};
  if(ui.motion.lesson) {
    const auto cursor=ImGui::GetCursorPos();ImGui::SameLine(ImGui::GetWindowWidth()-(ui.corpus?158:82));
    ImGui::PushStyleColor(ImGuiCol_Button,{.10F,.34F,.40F,1});
    if(ImGui::Button("Motion",{70,22}))ui.motion.open=true;
    ui.motionEntry=itemBounds(!io.AppFocusLost && !ui.solvePointerHeld);
    ImGui::PopStyleColor();ImGui::SetCursorPos(cursor);
  }
  if(ui.corpus) {
    const auto cursor=ImGui::GetCursorPos();ImGui::SameLine(ImGui::GetWindowWidth()-82);
    ImGui::PushStyleColor(ImGuiCol_Button,{.30F,.20F,.46F,1});
    if(ImGui::Button("Library",{70,22}))ui.library.open=true;
    ui.libraryEntry=itemBounds(!io.AppFocusLost && !ui.solvePointerHeld);
    ImGui::PopStyleColor();ImGui::SetCursorPos(cursor);
  }
  if(ui.progressMessage.empty())ImGui::TextDisabled("Choose titles, then problems.");
  else {
    ImGui::TextColored(ui.progressFailed?ImVec4{1,.68F,.27F,1}:ImVec4{.4F,.9F,.65F,1},"%s",
        ui.progressFailed?"Saving paused. Hover for details.":ui.progressMessage.c_str());
    ui.progressStatus=itemBounds(false);
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("%s",ui.progressMessage.c_str());
  }
  ImGui::EndChild();

  beginSolvePanel("Study contents",toc,flags & ~ImGuiWindowFlags_NoScrollbar);
  for(std::size_t subject=0;subject<sorterSubjects.size();++subject) {
    ImGui::PushID(static_cast<int>(subject));
    std::size_t included=0,total=0;
    for(std::size_t i=0;i<study.types.size();++i)if(static_cast<unsigned>(study.types[i].subject)==subject) {
      ++total;if(study.includedTypes[i])++included;
    }
    bool checked=total && included==total;
    ImGui::BeginDisabled(!total);ImGui::PushItemFlag(ImGuiItemFlags_MixedValue,included && included<total);
    if(ImGui::Checkbox("##subject",&checked))queue(SorterActionKind::ToggleStudySubject,subject);
    ui.studySubjects[subject]=itemBounds(total!=0);ImGui::PopItemFlag();ImGui::SameLine();
    const std::string label=std::to_string(subject+1)+". "+std::string(sorterSubjects[subject].name);
    const bool expanded=ImGui::TreeNodeEx(label.c_str(),ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_SpanAvailWidth);
    if(expanded) {
      for(std::size_t i=0;i<study.types.size();++i) {
        const auto& chapter=study.types[i];
        if(static_cast<unsigned>(chapter.subject)!=subject || chapter.chapterId!=i)continue;
        ImGui::PushID(static_cast<int>(i));included=0;total=0;
        for(std::size_t j=0;j<study.types.size();++j)if(study.types[j].chapterId==i) {++total;if(study.includedTypes[j])++included;}
        checked=included==total;ImGui::PushItemFlag(ImGuiItemFlags_MixedValue,included && included<total);
        if(ImGui::Checkbox("##chapter",&checked))queue(SorterActionKind::ToggleStudyChapter,i);
        ui.studyChapterChecks[i]=itemBounds();ImGui::PopItemFlag();ImGui::SameLine();
        const auto count=study.chapters[i];
        const std::string tally=std::to_string(count.completed)+"/"+std::to_string(count.total);
        const float titleWidth=std::max(40.0F,ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(tally.c_str()).x-8);
        const auto titleSize=ImGui::CalcTextSize(chapter.chapter.c_str(),nullptr,false,titleWidth);
        const float height=std::max(26.0F,titleSize.y+4);
        if(ImGui::Selectable("##chapter_title",ui.studyChapter==i,0,{titleWidth,height}))ui.studyChapter=i;
        ui.studyChapters[i]=itemBounds();const auto row=ui.studyChapters[i];
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
            {row.x,row.y+(height-titleSize.y)*.5F},ImGui::GetColorU32(ImGuiCol_Text),chapter.chapter.c_str(),nullptr,titleWidth);
        ImGui::SameLine(0,8);ImGui::SetCursorPosY(ImGui::GetCursorPosY()+(height-ImGui::GetTextLineHeight())*.5F);
        ImGui::TextColored(count.completed?ImVec4{.4F,.9F,.65F,1}:ImVec4{.55F,.63F,.7F,1},"%s",tally.c_str());
        ui.studyChapterCounts[i]=itemBounds();
        if(ImGui::IsItemHovered())ImGui::SetTooltip("Completed / total questions in this chapter (current attempts).");
        ImGui::PopID();
      }
      ImGui::TreePop();
    }
    ImGui::EndDisabled();ImGui::PopID();
  }
  ImGui::EndChild();

  ui.studyProblemPanel=beginSolvePanel("Study problems",questions,flags & ~ImGuiWindowFlags_NoScrollbar);
  if(study.types.empty())ImGui::TextWrapped("This pack has no prepared chapter selection. Open Groups to browse its cards.");
  for(std::size_t i=0;i<study.types.size();++i)if(study.types[i].chapterId==ui.studyChapter) {
    const auto& type=study.types[i];
    if(type.chapterId==i) {ImGui::PushFont(nullptr,17);ImGui::TextUnformatted(type.chapter.c_str());ImGui::PopFont();}
    ImGui::PushID(static_cast<int>(i));bool included=study.includedTypes[i];
    if(ImGui::Checkbox(type.title.c_str(),&included))queue(SorterActionKind::ToggleStudyType,i);
    ui.studyTypes[i]=itemBounds();ImGui::TextDisabled("%s",type.form.c_str());ImGui::PopID();
  }
  ImGui::SeparatorText("Problems from selected titles");
  if(!study.availableCount)ImGui::TextWrapped("Choose a title from the table of contents.");
  for(const auto& e:content)if(study.available[e.homeIndex]) {
    ImGui::PushID(static_cast<int>(e.id));bool selected=study.selected[e.homeIndex];
    const std::string label=std::to_string(e.homeIndex+1)+".  "+e.text;
    const float markX=ImGui::GetCursorScreenPos().x;
    ImGui::Dummy({16,1});ImGui::SameLine(0,6);
    if(ImGui::Checkbox(label.c_str(),&selected))queue(SorterActionKind::ToggleStudyQuestion,0,e.id);
    ui.studyQuestions[e.homeIndex]=itemBounds();ui.studyQuestions[e.homeIndex].equation=e.id;
    const auto row=ui.studyQuestions[e.homeIndex];const ImVec2 centre{markX+8,row.y+row.height*.5F};
    ui.studyProgressMarks[e.homeIndex]={e.id,markX,centre.y-8,16,16,ImGui::IsItemVisible()};
    auto* draw=ImGui::GetWindowDrawList();
    switch(study.progress[e.homeIndex]) {
      case Progress::NotStarted:draw->AddCircle(centre,4,IM_COL32(140,161,179,255),0,1.5F);break;
      case Progress::InProgress:draw->AddCircleFilled(centre,4,IM_COL32(77,217,242,255));break;
      case Progress::Completed:
        draw->AddLine({centre.x-5,centre.y},{centre.x-1,centre.y+4},IM_COL32(102,230,166,255),2);
        draw->AddLine({centre.x-1,centre.y+4},{centre.x+6,centre.y-4},IM_COL32(102,230,166,255),2);break;
    }
    constexpr std::array descriptions{"Not started","In progress","Completed (current attempt)"};
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",descriptions[static_cast<std::size_t>(study.progress[e.homeIndex])]);
    ImGui::PopID();
  }
  ImGui::EndChild();

  beginSolvePanel("Study actions",{0,io.DisplaySize.y-footer,io.DisplaySize.x,footer},flags);
  const std::array labels{"All","Random","Pick"};
  for(std::size_t i=0;i<labels.size();++i) {
    if(i)ImGui::SameLine();
    if(ImGui::RadioButton(labels[i],study.mode==static_cast<StudyMode>(i)))queue(SorterActionKind::SetStudyMode,i);
    bounds(static_cast<StudyControl>(i));
  }
  if(!narrow)ImGui::SameLine();
  ImGui::BeginDisabled(study.mode!=StudyMode::Random);ImGui::SetNextItemWidth(112);
  int count=static_cast<int>(study.randomCount);
  if(ImGui::InputInt("##random_count",&count,1,0))queue(SorterActionKind::SetStudyCount,std::clamp(count,1,100));
  bounds(StudyControl::Count,study.mode==StudyMode::Random);ImGui::SameLine();
  if(ImGui::Button("Shuffle",{78,26}))queue(SorterActionKind::ShuffleStudy);
  bounds(StudyControl::Shuffle,study.mode==StudyMode::Random);ImGui::EndDisabled();
  ImGui::Text("%zu / %zu selected",study.selectedCount,study.availableCount);
  const float buttonWidth=std::min(160.0F,(ImGui::GetContentRegionAvail().x-16)/3);
  if(ImGui::Button("Groups",{buttonWidth,30}))queue(SorterActionKind::CloseStudy);
  bounds(StudyControl::Groups);auto* focusWindow=ImGui::GetCurrentWindow();
  const auto focusId=ImGui::GetItemID();const ImRect focusRect{ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
  ImGui::SameLine();ImGui::BeginDisabled(!study.canResume);
  ImGui::PushStyleColor(ImGuiCol_Button,{.08F,.28F,.40F,1});
  if(ImGui::Button("Resume set",{buttonWidth,30}))queue(SorterActionKind::ResumeStudy);
  bounds(StudyControl::Resume,study.canResume);ImGui::PopStyleColor();ImGui::EndDisabled();ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button,{.10F,.30F,.29F,1});ImGui::BeginDisabled(!study.selectedCount);
  if(ImGui::Button("Start set",{buttonWidth,30}))queue(SorterActionKind::StartStudy);
  bounds(StudyControl::Start,study.selectedCount!=0);ImGui::EndDisabled();ImGui::PopStyleColor();
  ImGui::EndChild();recoverFocus(focusWindow,focusId,focusRect);
  ImGui::PopFont();ImGui::EndDisabled();ImGui::PopItemFlag();ImGui::PopStyleColor(3);ImGui::PopStyleVar(4);ImGui::End();
}
}
void beginEquationSorterFrame(EquationSorterUiState& ui, EquationSorterSession& session, float seconds) {
  if(!ImGui::IsMouseDown(ImGuiMouseButton_Left))ui.solvePointerHeld=false;
  if(ui.motion.open && ui.motion.lesson) {
    ui.pending.reset();ui.shootAvailable=false;
    auto& lesson=*ui.motion.lesson;const auto& io=ImGui::GetIO();
    if(io.AppFocusLost && lesson.playing())(void)lesson.dispatch({MotionActionKind::Pause});
    if(!io.AppFocusLost)(void)lesson.dispatch({MotionActionKind::Tick,0,seconds<0?std::clamp(static_cast<double>(io.DeltaTime),0.0,.25):std::clamp(static_cast<double>(seconds),0.0,.25)});
    return;
  }
  if (ImGui::GetIO().AppFocusLost) {
    ui.pending.reset();
    if(auto* game=session.activeSolve())(void)game->dispatch(GalleryPause{true});
    else (void)session.dispatch({SorterActionKind::ClearInspection});
  } else if (ui.pending) {
    if(const auto* action=std::get_if<SorterAction>(&*ui.pending)) {
      (void)session.dispatch(*action);
      ui.shootAvailable=false;
      ui.solvePointerHeld=ImGui::IsMouseDown(ImGuiMouseButton_Left);
      ui.solveDisplayedChallenge={};
    } else if(auto* game=session.activeSolve())(void)game->dispatch(std::get<GalleryCommand>(*ui.pending));
    ui.pending.reset();
  }
  if(auto* game=session.activeSolve()) {
    const auto& io=ImGui::GetIO();const auto r=ui.shootViewport;
    if(!io.AppFocusLost && ui.shootAvailable && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        io.MousePos.x>=r.x && io.MousePos.x<r.x+r.width && io.MousePos.y>=r.y && io.MousePos.y<r.y+r.height) {
      (void)game->dispatch(Shoot{ui.shootFrame,ui.shootChallenge,(io.MousePos.x-r.x)/r.width,(io.MousePos.y-r.y)/r.height});
      // A shot is judged on press. Its release must not activate an operation
      // that appears beneath the pointer after the immediate step change.
      ui.solvePointerHeld=true;
    }
    const auto& graph=game->question().content().lineGraph;
    (void)game->dispatch(GalleryViewport{solveLayout(io.DisplaySize,graph.has_value(),graph && graph->second.has_value()).stage});
    (void)game->dispatch(GalleryTick{seconds<0?std::clamp(io.DeltaTime,0.0F,.25F):seconds});
    (void)game->publishFrame();
  }
}
void drawEquationSorter(EquationSorterUiState& ui, const SorterView& view,
                        std::span<const SorterEquation> content, const GallerySession* game) {
  auto& io = ImGui::GetIO();
  ui.motion.presented=false;
  ui.library.document.presented=false;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigNavCursorVisibleAlways = true;
  if(view.solving && game) {
    const auto& run=game->question().currentRun();syncMathNotation(ui.notation,run.questionId,run.runNumber);
    drawSolving(ui,view,*game);return;
  }
  if(view.studying) {
    if(ui.motion.open && ui.motion.lesson) {
      ui.shootAvailable=false;ui.solveButton={};ui.cardCount=0;
      drawMotionLesson(ui.motion);return;
    }
    if(ui.library.open && ui.corpus) {
      ui.shootAvailable=false;ui.solveButton={};ui.cardCount=0;
      drawMathCorpus(ui.library,*ui.corpus,ui.solvePointerHeld);return;
    }
    drawStudySelection(ui,view,content);return;
  }
  ui.shootAvailable=false;ui.solveButton={};
  const auto queue = [&](SorterActionKind kind, SorterBucket bucket = SorterBucket::A,
                         SorterEquationId equation = 0) {
    if (!ui.pending && !io.AppFocusLost) ui.pending = SorterAction{kind, bucket, equation, view.revision};
  };
  ImGui::SetNextWindowPos({0, 0});
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20, 18});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.055F, 0.065F, 0.085F, 1});
  ImGui::PushStyleColor(ImGuiCol_PopupBg, {0.055F, 0.065F, 0.085F, 1});
  ImGui::PushStyleColor(ImGuiCol_Text, {0.91F, 0.91F, 0.87F, 1});
  ImGui::PushStyleColor(ImGuiCol_Button, {0.12F, 0.15F, 0.19F, 1});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.19F, 0.24F, 0.29F, 1});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.23F, 0.30F, 0.34F, 1});
  ImGui::Begin("Equation sorter##sorter", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
  ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, false);
  ImGui::BeginDisabled(io.AppFocusLost);
  ImGui::PushFont(nullptr, 25);
  ImGui::TextUnformatted("Equation sorter");
  ImGui::PopFont();
  ImGui::SameLine();ImGui::PushFont(nullptr,15);
  if(ImGui::Button("Contents",{96,26}))queue(SorterActionKind::OpenStudy);
  ui.studyEntry=itemBounds();ImGui::PopFont();
  ImGui::TextUnformatted("Subject suggestions. Your groups.");
  ImGui::Spacing();
  const float available = ImGui::GetContentRegionAvail().x;
  const int toolbarColumns = std::clamp(static_cast<int>((available + 8) / 126), 3, 6);
  const float toolbarWidth = (available - 8 * (toolbarColumns - 1)) / toolbarColumns;
  ImGuiID fallbackFocus = 0;
  ImRect fallbackRect;
  for (std::size_t index = 0; index < sorterBucketCount; ++index) {
    if (index % toolbarColumns) ImGui::SameLine(0, 8);
    ImGui::PushID(static_cast<int>(index));
    const auto bucket = static_cast<SorterBucket>(index);
    const bool active = view.activeBucket == bucket;
    const std::string label = std::string(sorterBucketName(bucket)) + "  (" +
        std::to_string(view.counts[index + 1]) + ")\n" +
        std::string(index < sorterSubjects.size() ? sorterSubjects[index].caption : " ") + "###bucket";
    ImGui::PushStyleColor(ImGuiCol_Border, active ? ImVec4{0.45F, 0.78F, 0.73F, 1} : ImVec4{0.25F, 0.3F, 0.35F, 1});
    if (ImGui::Button(label.c_str(), {toolbarWidth, 50})) queue(SorterActionKind::SelectBucket, bucket);
    if (active || (!view.activeBucket && index == 0)) {
      fallbackFocus = ImGui::GetItemID();
      fallbackRect = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
    }
    ImGui::PopStyleColor();
    const auto start = ImGui::GetItemRectMin();
    ui.toolbar[index] = {0, start.x, start.y, toolbarWidth, 50, true};
    ImGui::PopID();
  }
  const std::array controls{
      std::pair{"Undo", SorterActionKind::Undo}, std::pair{"Auto sort", SorterActionKind::AutoSort},
      std::pair{"Hint / Next", SorterActionKind::ShowHint}};
  const std::array enabled{view.undoDepth != 0, view.autoSortCount != 0, true};
  for (std::size_t i = 0; i < controls.size(); ++i) {
    if (i) ImGui::SameLine(0, 8);
    ImGui::BeginDisabled(!enabled[i]);
    if (ImGui::Button(controls[i].first, {(available - 16) / 3, 32})) queue(controls[i].second);
    ImGui::EndDisabled();
    const auto start = ImGui::GetItemRectMin(), end = ImGui::GetItemRectMax();
    ui.toolbar[sorterUndoControl + i] = {0, start.x, start.y, end.x-start.x, end.y-start.y, enabled[i]};
  }
  ImGui::Spacing();
  ImGui::Text("Unsorted: %zu / 100%s", view.counts[0], view.counts[0] ? "" : "    All 100 assigned");
  if (view.inventory) {
    const auto bucket = *view.activeBucket;
    ImGui::Text("%s inventory  (%zu)", sorterBucketName(bucket).data(), view.counts[static_cast<int>(bucket) + 1]);
    if (ImGui::Button("Back to grid", {125, 32})) queue(SorterActionKind::BackToGrid);
    ImGui::SameLine();
    ImGui::BeginDisabled(view.counts[static_cast<int>(bucket) + 1] == 0);
    if (ImGui::Button("Empty bucket", {125, 32})) queue(SorterActionKind::RequestEmpty);
    ImGui::EndDisabled();
  } else {
    ImGui::TextUnformatted(view.activeBucket ? "Click once to inspect, again to store." : "Select a bucket. You can inspect cards first.");
    ImGui::Dummy({0, 32});
  }
  // Reserve the complete prompt area so opening/cancelling it never moves cards.
  const float promptTop = ImGui::GetCursorPosY();
  if (view.pendingEmpty) {
    ImGui::Text("Empty %zu equations back to grid?", view.counts[static_cast<int>(*view.activeBucket) + 1]);
    if (ImGui::Button("Confirm", {100, 28})) queue(SorterActionKind::ConfirmEmpty);
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {100, 28})) queue(SorterActionKind::CancelEmpty);
  } else {
    ImGui::TextWrapped("%s", view.nextStep.c_str());
    if(view.solveCandidate) {
      if(ImGui::Button(view.solveLabel.c_str(),{ImGui::GetContentRegionAvail().x,28}))
        queue(SorterActionKind::OpenSolve,SorterBucket::A,*view.solveCandidate);
      ui.solveButton=itemBounds();
    }
  }
  ImGui::SetCursorPosY(promptTop + 64);
  ImGui::Separator();
  const std::string child = view.inventory ? "inventory_" + std::string(sorterBucketName(*view.activeBucket)) : "original_grid";
  ui.cardCount = 0;
  if (ImGui::BeginChild(child.c_str(), {0, 0}, ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
    ui.scrollY = ImGui::GetScrollY();
    const float width = ImGui::GetContentRegionAvail().x;
    const int columns = std::max(1, static_cast<int>((width + 10) / 230));
    const float cardWidth = (width - 10 * (columns - 1)) / columns;
    ImGui::PushFont(nullptr, 19);
    float cardHeight = 96;
    // Measure immutable content, including assigned cards: a move cannot reflow the grid.
    for (const auto& e : content)
      cardHeight = std::max(cardHeight, ImGui::CalcTextSize(e.text.c_str(), nullptr, false, cardWidth - 24).y + 32);
    const auto count = view.inventory ? view.slotCount : content.size();
    for (std::size_t slot = 0; slot < count; ++slot) {
      if (slot % columns) ImGui::SameLine(0, 10);
      const auto* e = &content[slot];
      if (view.inventory) {
        e = &*std::find_if(content.begin(), content.end(), [&](const auto& item) {
          return item.id == view.inventorySlots[slot];
        });
      }
      const auto owner = view.owners[e->homeIndex];
      const bool availableCard = view.inventory ? owner == view.activeBucket : !owner;
      const bool prepared = availableCard && bool(e->solution);
      const bool inspected = view.inspected == e->id;
      const ImVec2 start = ImGui::GetCursorScreenPos();
      const std::string id = std::to_string(e->id);
      ImGui::PushID(id.c_str());
      ImGui::PushStyleColor(ImGuiCol_Border, prepared
          ? (inspected ? ImVec4{1,.88F,.59F,1} : ImVec4{1,.77F,.29F,1})
          : (inspected ? ImVec4{.56F,.88F,.8F,1} : ImVec4{.24F,.29F,.34F,1}));
      ImGui::PushStyleColor(ImGuiCol_Button, prepared
          ? (inspected ? ImVec4{.36F,.25F,.09F,1} : ImVec4{.25F,.17F,.06F,1})
          : (inspected ? ImVec4{.13F,.23F,.22F,1} : ImVec4{.12F,.15F,.19F,1}));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, prepared ? ImVec4{.42F,.30F,.11F,1} : ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, prepared ? ImVec4{.50F,.36F,.13F,1} : ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, prepared ? (inspected ? 3.0F : 2.0F) : ImGui::GetStyle().FrameBorderSize);
      if (availableCard) {
        if (ImGui::Button("##equation", {cardWidth, cardHeight})) queue(SorterActionKind::ActivateEquation, SorterBucket::A, e->id);
      } else {
        ImGui::Dummy({cardWidth, cardHeight});
        ImGui::GetWindowDrawList()->AddRectFilled(start, {start.x + cardWidth, start.y + cardHeight}, IM_COL32(21, 25, 32, 255), 6);
        ImGui::GetWindowDrawList()->AddRect(start, {start.x + cardWidth, start.y + cardHeight}, IM_COL32(47, 54, 64, 255), 6);
      }
      std::string label = availableCard ? e->text : owner ? "In " + std::string(sorterBucketName(*owner)) : "Returned to grid";
      const auto size = ImGui::CalcTextSize(label.c_str(), nullptr, false, cardWidth - 24);
      ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
          {start.x + 12, start.y + (cardHeight - size.y) / 2},
          availableCard ? IM_COL32(235, 235, 222, 255) : IM_COL32(123, 133, 146, 255), label.c_str(), nullptr, cardWidth - 24);
      if(prepared)
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),13,{start.x+12,start.y+8},IM_COL32(255,196,75,255),"Solve");
      ui.cards[ui.cardCount++] = {e->id, start.x, start.y, cardWidth, cardHeight, availableCard};
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(4);
      ImGui::PopID();
    }
    if (!count) ImGui::TextDisabled("This bucket is empty.");
    ImGui::PopFont();
  }
  ImGui::EndChild();
  ui.hintClose = {};
  if (view.hintVisible && !ImGui::IsPopupOpen("Hint / Next step")) ImGui::OpenPopup("Hint / Next step");
  ImGui::SetNextWindowSize({std::min(io.DisplaySize.x - 32, 580.0F), std::min(io.DisplaySize.y - 32, 440.0F)});
  ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, ImGuiCond_Always, {.5F, .5F});
  if (ImGui::BeginPopupModal("Hint / Next step", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
    if (!view.hintVisible) ImGui::CloseCurrentPopup();
    if (ImGui::BeginChild("hint_text", {0, -40})) {
      ImGui::PushFont(nullptr, 17);
      ImGui::TextWrapped("%s", view.hint.c_str());
      ImGui::PopFont();
    }
    ImGui::EndChild();
    if (ImGui::Button("Close hint", {ImGui::GetContentRegionAvail().x, 32})) queue(SorterActionKind::CloseHint);
    const auto start = ImGui::GetItemRectMin(), end = ImGui::GetItemRectMax();
    ui.hintClose = {0, start.x, start.y, end.x-start.x, end.y-start.y, true};
    ImGui::EndPopup();
  }
  // A moved card, closed prompt, or exhausted Undo must not strand Tab on a
  // missing/disabled widget. Recover to the persistent destination without
  // activating it or requesting a scroll to another card.
  if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    recoverFocus(ImGui::GetCurrentWindow(),fallbackFocus,fallbackRect);
  // Route Escape to this standalone sorter before ImGui's child-window navigation.
  if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    queue(view.hintVisible ? SorterActionKind::CloseHint : view.pendingEmpty ? SorterActionKind::CancelEmpty : SorterActionKind::ClearInspection);
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive())
    queue(SorterActionKind::ClearInspection);
  ImGui::EndDisabled();
  ImGui::PopItemFlag();
  ImGui::End();
  ImGui::PopStyleColor(6);
  ImGui::PopStyleVar(3);
}
} // namespace paths
