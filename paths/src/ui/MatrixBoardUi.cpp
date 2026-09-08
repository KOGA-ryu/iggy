#include "MatrixBoardUi.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdio>

namespace paths {
void drawMatrixBoardGrid(const char* name,const BoardMatrix& a,const MatrixBoardView& v,MatrixBoardUiState& ui,bool given){
  if(!a.rows){ImGui::TextUnformatted("Unknown: use the Working tab for each chosen pivot. Factors appear when the trace completes.");return;}
  ImGui::Text("%s / %u x %u",name,a.rows,a.cols);
  if(ImGui::BeginTable(name,static_cast<int>(a.cols+1),ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollX|ImGuiTableFlags_ScrollY|ImGuiTableFlags_SizingFixedFit,{0,std::max(120.f,ImGui::GetContentRegionAvail().y-35)})){
    ImGui::TableSetupColumn("row",ImGuiTableColumnFlags_WidthFixed,45);for(unsigned c=0;c<a.cols;++c){char label[20];std::snprintf(label,sizeof(label),"%u",c+1);ImGui::TableSetupColumn(label,ImGuiTableColumnFlags_WidthFixed,115);}
    ImGui::TableSetupScrollFreeze(1,1);ImGui::TableHeadersRow();ImGuiListClipper clip;clip.Begin(static_cast<int>(a.rows));
    while(clip.Step()){for(int i=clip.DisplayStart;i<clip.DisplayEnd;++i){ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::Text("%d",i+1);for(unsigned j=0;j<a.cols;++j){ImGui::TableNextColumn();const auto z=a.at(static_cast<unsigned>(i),j);
      if(given&&v.card==1&&v.structuralZero[static_cast<unsigned>(i)*a.cols+j])ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,IM_COL32(35,42,50,255));
      if(a.rows==v.given.rows&&(v.card==18||v.card==59)&&static_cast<unsigned>(i)<v.partition&&j<v.partition)ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,IM_COL32(35,80,92,255));
      char text[80];if(z.imag()==0)std::snprintf(text,sizeof(text),"%.5g",z.real());else std::snprintf(text,sizeof(text),"%.4g%+.4gi",z.real(),z.imag());
      ImGui::PushID(i*static_cast<int>(a.cols)+static_cast<int>(j));if(ImGui::Selectable(text,ui.selectedRow==i&&ui.selectedColumn==static_cast<int>(j))){ui.selectedRow=i;ui.selectedColumn=static_cast<int>(j);}ImGui::PopID();
      if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::Text("(%d, %u) = %.12g %+.12gi",i+1,j+1,z.real(),z.imag());ImGui::TextUnformatted(given?"Supplied in this instance":"Produced by your chosen operations");if(given&&v.card==1&&v.structuralZero[static_cast<unsigned>(i)*a.cols+j])ImGui::TextUnformatted("Structural zero: prescribed by |i-j| > p");else if(std::abs(z)<1e-12)ImGui::TextUnformatted("Magnitude < 1e-12; numeric smallness is not a structural claim.");ImGui::EndTooltip();}
    }}}
    ImGui::EndTable();
  }
}
void applyMatrixBoardPending(MatrixBoard& board,MatrixBoardUiState& ui){
  if(ui.hasPending){const auto result=board.dispatch(ui.pending);ui.message=result.accepted?"":result.reason;ui.hasPending=false;}
}
void drawMatrixBoardContents(MatrixBoard& board,MatrixBoardUiState& ui,bool lockCard){
  applyMatrixBoardPending(board,ui);
  const auto v=board.view();const auto send=[&](BoardAction a){if(!ui.hasPending){ui.pending=a;ui.hasPending=true;}};
  const auto& cards=matrixCards();auto spec=std::find_if(cards.begin(),cards.end(),[&](auto c){return c.id==v.card;});
  if(!lockCard){
  ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##card",spec->title)){for(auto c:cards)if(ImGui::Selectable(c.title,c.id==v.card)){send({BoardActionKind::Select,c.id});ui.selectedRow=ui.selectedColumn=-1;}ImGui::EndCombo();}
  }
  spec=std::find_if(cards.begin(),cards.end(),[&](auto c){return c.id==v.card;});ImGui::TextWrapped("%s",spec->task);ImGui::TextDisabled("%s",v.working?"WORKING / derived values follow your operations":"SETUP / given values only");
  if(ImGui::BeginTable("workspace",2,ImGuiTableFlags_Resizable)){
    ImGui::TableSetupColumn("Board",ImGuiTableColumnFlags_WidthStretch,3);ImGui::TableSetupColumn("Controls",ImGuiTableColumnFlags_WidthStretch,1);ImGui::TableNextColumn();
    ImGui::BeginChild("matrices",{0,0});
    if(ImGui::BeginTabBar("matrix views")){
      if(ImGui::BeginTabItem("Given A")){drawMatrixBoardGrid("A",v.given,v,ui,true);ImGui::EndTabItem();}
      if(v.rhs.rows&&ImGui::BeginTabItem("Given b")){drawMatrixBoardGrid("b",v.rhs,v,ui,true);ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Working")){drawMatrixBoardGrid("Working matrix",v.current,v,ui);ImGui::EndTabItem();}
      if(v.rhs.rows&&ImGui::BeginTabItem("Working rhs")){drawMatrixBoardGrid("Working rhs",v.currentRhs,v,ui);ImGui::EndTabItem();}
      if(v.card==1||v.card==18||v.card==44){if(ImGui::BeginTabItem("L")){drawMatrixBoardGrid("L",v.lower,v,ui);ImGui::EndTabItem();}if(ImGui::BeginTabItem("U")){drawMatrixBoardGrid("U",v.upper,v,ui);ImGui::EndTabItem();}}
      if(v.card==44&&ImGui::BeginTabItem("P")){drawMatrixBoardGrid("P (PA=LU)",v.permutation,v,ui);ImGui::EndTabItem();}
      if(v.card==59&&ImGui::BeginTabItem("Schur block")){drawMatrixBoardGrid("Trailing block S",v.schurBlock,v,ui);ImGui::EndTabItem();}
      ImGui::EndTabBar();
    }ImGui::EndChild();ImGui::TableNextColumn();ImGui::BeginChild("controls",{0,0});
    ImGui::TextWrapped("%s",spec->provenance);ImGui::Spacing();
    if(spec->cases>1){int part=static_cast<int>(v.example);ImGui::SetNextItemWidth(-1);if(ImGui::SliderInt("Example / part",&part,0,static_cast<int>(spec->cases-1)))send({BoardActionKind::Select,v.card,static_cast<unsigned>(part)});ImGui::Text("Part %c (index %u)",static_cast<char>('a'+v.example),v.example);}
    if(v.card!=4&&v.card!=44){int n=static_cast<int>(v.size),band=static_cast<int>(v.bandwidth),cut=static_cast<int>(v.partition);ImGui::TextUnformatted("Changing a parameter resets this example.");
      if(ImGui::SliderInt("Size",&n,2,MatrixBoard::maxSize))send({BoardActionKind::Configure,static_cast<unsigned>(n),std::min(v.bandwidth,static_cast<unsigned>(n-1)),std::min(v.partition,static_cast<unsigned>(n-1))});
      if(v.card==1&&ImGui::SliderInt("Half-bandwidth",&band,0,n-1))send({BoardActionKind::Configure,v.size,static_cast<unsigned>(band),v.partition});
      if((v.card==18||v.card==59)&&ImGui::SliderInt("Leading block / cut",&cut,1,v.card==18?n:n-1))send({BoardActionKind::Configure,v.size,v.bandwidth,static_cast<unsigned>(cut)});
    }
    ImGui::Separator();ImGui::Text("Operations: %u / %u",v.steps,MatrixBoard::maxHistory);
    ImGui::BeginDisabled(v.complete||v.blocked);if(ImGui::Button("Perform next pivot",{-1,32}))send({BoardActionKind::Step});ImGui::EndDisabled();
    ImGui::BeginDisabled(!v.steps);if(ImGui::Button("Undo"))send({BoardActionKind::Undo});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Reset"))send({BoardActionKind::Reset});
    if(v.card==4||v.card==31){ImGui::Separator();ImGui::TextUnformatted("Manual row operation (1-based rows)");ImGui::InputInt("Target row",&ui.row);ImGui::InputInt("Other row",&ui.other);ImGui::InputDouble("Real multiplier",&ui.real);ImGui::InputDouble("Imaginary multiplier",&ui.imaginary);
      const bool valid=ui.row>=1&&ui.other>=1&&ui.row<=static_cast<int>(v.given.rows)&&ui.other<=static_cast<int>(v.given.rows);ImGui::BeginDisabled(!valid);
      const auto r=static_cast<unsigned>(std::max(1,ui.row)-1),s=static_cast<unsigned>(std::max(1,ui.other)-1);const MatrixScalar z{ui.real,ui.imaginary};
      if(ImGui::Button("Swap rows"))send({BoardActionKind::SwapRows,r,s});if(ImGui::Button("Scale target"))send({BoardActionKind::ScaleRow,r,0,0,z});if(ImGui::Button("Target += multiplier * other"))send({BoardActionKind::AddRow,r,s,0,z});ImGui::EndDisabled();
    }
    ImGui::Separator();ImGui::TextWrapped("%s",v.status.c_str());ImGui::BeginDisabled(!v.working);if(ImGui::Button("Check current work",{-1,32}))send({BoardActionKind::Check});ImGui::EndDisabled();
    if(v.checked){ImGui::TextUnformatted(v.passed?"Numeric check passed":"Check incomplete or failed");ImGui::Text("Residual %.5g / tolerance %.1g",v.residual,v.tolerance);ImGui::Text("Kind: %s",v.checkKind.c_str());ImGui::TextWrapped("%s",v.evidence.c_str());if(v.hasLeadingDeterminant)ImGui::Text("det A[1:%u,1:%u] = %.8g %+.8gi",v.partition,v.partition,v.leadingDeterminant.real(),v.leadingDeterminant.imag());}
    if(!ui.message.empty())ImGui::TextWrapped("%s",ui.message.c_str());
    ImGui::Separator();ImGui::TextWrapped("Scroll to inspect every cell; select a cell or hover for full precision. Blue cells mark the chosen leading block. Dark cells in Given A mark prescribed band zeros. No source-page attempt or progress is written.");
    ImGui::EndChild();ImGui::EndTable();
  }
}
void drawMatrixBoard(MatrixBoard& board,MatrixBoardUiState& ui,bool& active,const char* returnLabel){
  const auto size=ImGui::GetIO().DisplaySize;ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(size);
  ImGui::Begin("Matrix workspace",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
  ImGui::TextUnformatted("PATHS / MATRIX BOARD");ImGui::SameLine();if(ImGui::Button(returnLabel))active=false;
  drawMatrixBoardContents(board,ui,false);ImGui::End();
}
}
