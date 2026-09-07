#include "content/EquationSorterContentIO.hpp"
#include "ui/EquationSorterUi.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <imgui.h>
#include <imgui_internal.h>

using namespace paths;
void expect(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Harness {
  EquationSorterSession session;
  EquationSorterUiState ui;
  Harness(float width = 1440, float height = 900, const std::filesystem::path& content = SORTER_FIXTURE, bool mathematicalMoves = true)
      : session([&] {
          auto result=loadSorterContent(content);
          // Explicit prepared fixtures retain the authored-question UI gate.
          if(!mathematicalMoves)for(auto& equation:result)if(equation.solution) {
            auto prepared=std::make_shared<iggy3d::first_move::LayeredQuestionContent>(*equation.solution);
            prepared->supportsMathMoves=false;equation.solution=std::move(prepared);
          }
          return result;
        }()) {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {width, height};
    io.DeltaTime = 1.0F / 60;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    unsigned char* pixels; int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    io.Fonts->SetTexID(static_cast<ImTextureID>(1));
    frame(3);
  }
  ~Harness() { ImGui::DestroyContext(); }
  void frame(int count = 1) {
    for (int i = 0; i < count; ++i) {
      ImGui::NewFrame();
      beginEquationSorterFrame(ui, session);
      drawEquationSorter(ui, session.view(), session.content(), session.activeSolve());
      ImGui::Render();
    }
  }
  void click(const SorterCardBounds& bounds) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
    frame();
    io.AddMouseButtonEvent(0, true); frame();
    io.AddMouseButtonEvent(0, false); frame(2);
  }
  void press(ImGuiKey key) {
    ImGui::GetIO().AddKeyEvent(key, true); frame(2);
    ImGui::GetIO().AddKeyEvent(key, false); frame(2);
  }
  void focusButton(const char* label) {
    const auto id = ImGui::FindWindowByName("Equation sorter##sorter")->GetID(label);
    for (int i = 0; GImGui->NavId != id && i < 110; ++i) press(ImGuiKey_Tab);
    if (GImGui->NavId != id)
      std::cerr << "unreachable=" << label << " target=" << id << " nav=" << GImGui->NavId
                << " window=" << (GImGui->NavWindow ? GImGui->NavWindow->Name : "none")
                << " inventory=" << session.view().inventory << " countA=" << session.view().counts[1]
                << " pendingEmpty=" << session.view().pendingEmpty << " size=" << ImGui::GetIO().DisplaySize.x << '\n';
    expect(GImGui->NavId == id, "inventory control reachable with Tab");
  }
  void action(SorterActionKind kind, SorterEquationId id = 0, SorterBucket bucket = SorterBucket::A) {
    expect(session.dispatch({kind, bucket, id, session.view().revision}).accepted, "setup action accepted");
    frame(2);
  }
};
bool sameRect(const SorterCardBounds& a, const SorterCardBounds& b) {
  return std::abs(a.x-b.x) < .1F && std::abs(a.y-b.y) < .1F &&
         std::abs(a.width-b.width) < .1F && std::abs(a.height-b.height) < .1F;
}
bool contains(const SorterCardBounds& outer,const SorterCardBounds& inner) {
  return inner.x>=outer.x && inner.y>=outer.y && inner.x+inner.width<=outer.x+outer.width+.1F &&
      inner.y+inner.height<=outer.y+outer.height+.1F;
}
auto workspaceBounds(const EquationSorterUiState& ui) {
  return std::array{ui.solveBoard,ui.solveWorking,ui.solveStage,ui.solveSupport};
}
void fixedWorkspace(const Harness& h,const std::array<SorterCardBounds,4>& original) {
  const auto current=workspaceBounds(h.ui);
  for(std::size_t i=0;i<original.size();++i)expect(sameRect(original[i],current[i]),
      "problem, working, 3D activity and support retain their rectangles across steps and help");
  const auto r=h.ui.shootViewport;
  expect(sameRect(h.ui.solveStage,{0,r.x,r.y,r.width,r.height,true}),"the scene occupies the same activity area in every phase");
}
void pointer() {
  Harness h;
  const auto first = h.ui.cards[0], second = h.ui.cards[1];
  h.click(first); h.click(first);
  expect(h.session.view().counts[0] == 100 && h.session.view().inspected == first.equation, "mouse cannot assign without bucket");
  h.click(h.ui.toolbar[0]);
  expect(!h.session.view().inspected && h.session.view().activeBucket == SorterBucket::A, "actual bucket click clears old inspection");
  h.click(first);
  expect(h.session.view().inspected == first.equation && h.session.view().undoDepth == 0, "one physical click inspects once");
  h.click(second);
  expect(h.session.view().inspected == second.equation && h.session.view().undoDepth == 0, "different pointer card only inspects");
  h.click(second);
  expect(h.session.view().counts[1] == 1 && h.session.view().undoDepth == 1, "second mouse click commits once");
  for (int i = 0; i < 5; ++i) h.click(second);
  expect(h.session.view().counts[1] == 1 && sameRect(second, h.ui.cards[1]) && sameRect(first, h.ui.cards[0]), "vacated grid rectangle cannot assign neighbor");
  h.click(h.ui.toolbar[0]);
  expect(h.session.view().inventory && h.ui.cardCount == 1, "actual active bucket click opens inventory");
  const auto inventory = h.ui.cards[0];
  h.click(inventory); h.click(inventory);
  expect(h.session.view().counts[0] == 100 && sameRect(inventory, h.ui.cards[0]), "inventory return retains actual rectangle");
  for (int i = 0; i < 4; ++i) h.click(inventory);
  expect(h.session.view().counts[0] == 100, "rapid inventory hole clicks do not move cards");
  h.click(h.ui.toolbar[sorterUndoControl]);
  expect(h.session.view().counts[1] == 1 && sameRect(inventory, h.ui.cards[0]), "actual Undo button restores the same slot");
}
void keyboardAndFocus() {
  for (const auto key : {ImGuiKey_Enter, ImGuiKey_Space}) {
    Harness h;
    h.click(h.ui.toolbar[0]);
    h.click(h.ui.cards[0]);
    h.press(ImGuiKey_Escape);
    expect(!h.session.view().inspected && h.session.view().counts[0] == 100, "Escape clears without assignment");
    ImGui::GetIO().AddKeyEvent(key, true); h.frame(90);
    if (h.session.view().inspected != h.ui.cards[0].equation || h.session.view().counts[0] != 100)
      std::cerr << "key=" << key << " inspected=" << h.session.view().inspected.value_or(0)
                << " unsorted=" << h.session.view().counts[0] << " nav=" << GImGui->NavId << '\n';
    expect(h.session.view().inspected == h.ui.cards[0].equation && h.session.view().counts[0] == 100, "held activation inspects without repeat commit");
    ImGui::GetIO().AddKeyEvent(key, false); h.frame(2);
    ImGui::GetIO().AddKeyEvent(key, true); h.frame(90);
    ImGui::GetIO().AddKeyEvent(key, false); h.frame(2);
    expect(h.session.view().counts[1] == 1 && !h.session.view().inventory,
        "fresh second keyboard activation commits without held-key activation of recovered focus");
    const auto owners = h.session.view().owners;
    const auto focus = GImGui->NavId;
    h.press(ImGuiKey_Tab);
    expect(GImGui->NavId != focus, "Tab leaves a card that became a placeholder");
    h.press(ImGuiKey_RightArrow);
    expect(h.session.view().owners == owners && !h.session.view().inspected, "Tab and arrow focus never activate");
    expect(GImGui->NavId != 0 && GImGui->NavCursorVisible, "keyboard focus is visible");
    h.click(h.ui.toolbar[sorterUndoControl]);
    expect(h.session.view().counts[0] == 100 && h.session.view().undoDepth == 0, "Undo becomes disabled after recovery");
    const auto recoveredFocus = GImGui->NavId;
    h.press(ImGuiKey_Tab);
    expect(GImGui->NavId != recoveredFocus && h.session.view().counts[0] == 100,
        "Tab continues after Undo becomes disabled without activating anything");
  }
  Harness h;
  h.click(h.ui.toolbar[0]);
  h.click(h.ui.cards[0]);
  ImGui::GetIO().AddMouseButtonEvent(0, true); h.frame();
  ImGui::GetIO().AddMouseButtonEvent(0, false); h.frame();
  expect(h.ui.pending.has_value(), "actual release queues the second activation");
  ImGui::GetIO().AddFocusEvent(false); h.frame(3);
  expect(!h.ui.pending && !h.session.view().inspected && h.session.view().counts[0] == 100, "focus loss discards queued commit");
  ImGui::GetIO().AddFocusEvent(true); h.frame(3);
  h.click(h.ui.cards[0]);
  expect(h.session.view().counts[0] == 100 && h.session.view().inspected, "refocus requires inspection again");
}
void layoutAndScroll() {
  for (const auto size : {ImVec2{1440,900}, ImVec2{800,600}, ImVec2{360,640}, ImVec2{360,480}}) {
    Harness h(size.x, size.y);
    expect(h.ui.cardCount == 100, "all home slots remain in layout");
    expect(h.ui.cards[0].y + h.ui.cards[0].height < size.y - 18, "minimum viewport displays a complete card");
    for (const auto& c : h.ui.cards) expect(c.x >= 0 && c.x+c.width <= size.x && c.width >= 200, "readable cards within viewport width");
    if (size.x == 360) expect(h.ui.cards[0].x == h.ui.cards[1].x && h.ui.cards[1].y > h.ui.cards[0].y, "narrow view is one column");
    h.click(h.ui.toolbar[0]);
    const float gridTop = h.ui.cards[0].y;
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(h.ui.cards[0].x + 10, h.ui.cards[0].y + 10); h.frame();
    io.AddMouseWheelEvent(0, -4); h.frame(4);
    expect(h.ui.scrollY > 0, "real wheel input scrolls equation area");
    const auto scroll = h.ui.scrollY;
    std::size_t slot = 0;
    while (slot < 100 && (h.ui.cards[slot].y + h.ui.cards[slot].height / 2 < gridTop + 4 ||
                           h.ui.cards[slot].y + h.ui.cards[slot].height / 2 >= size.y - 20)) ++slot;
    expect(slot < 100, "a scrolled card center is visible for pointer input");
    const auto target = h.ui.cards[slot];
    h.click(target); h.click(target);
    expect(h.session.view().counts[1] == 1, "scrolled card commits through actual pointer");
    expect(sameRect(target, h.ui.cards[slot]) && std::abs(h.ui.scrollY-scroll) < .1F, "assignment preserves scroll and rectangles");
    h.click(h.ui.toolbar[sorterUndoControl]);
    expect(sameRect(target, h.ui.cards[slot]) && std::abs(h.ui.scrollY-scroll) < .1F, "Undo preserves scroll and rectangles");
  }
}
void inventoryNavigationAndEmpty() {
  for (const auto size : {ImVec2{1440,900}, ImVec2{800,600}, ImVec2{360,480}}) {
    Harness h(size.x, size.y);
    h.action(SorterActionKind::SelectBucket);
    for (std::size_t i = 0; i < 60; ++i) {
      const auto id = h.session.content()[i].id;
      h.action(SorterActionKind::ActivateEquation, id);
      h.action(SorterActionKind::ActivateEquation, id);
    }
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(h.ui.cards[0].x + 10, h.ui.cards[0].y + 10); h.frame();
    io.AddMouseWheelEvent(0, -8); h.frame(4);
    const auto gridScroll = h.ui.scrollY;
    const auto gridFirst = h.ui.cards[0];
    expect(gridScroll > 0, "grid scrolled before inventory visit");
    h.click(h.ui.toolbar[0]);
    expect(h.session.view().inventory && h.ui.cardCount == 60, "open populated inventory through pointer");
    h.click(h.ui.cards[0]);
    h.focusButton("Back to grid"); h.press(ImGuiKey_Enter);
    expect(!h.session.view().inventory && std::abs(h.ui.scrollY-gridScroll) < .1F && sameRect(gridFirst, h.ui.cards[0]),
        "Back to grid preserves its scroll and home rectangles");
    h.click(h.ui.toolbar[0]);
    h.focusButton("Empty bucket");
    const auto before = h.session.view();
    const auto first = h.ui.cards[0];
    const auto scroll = h.ui.scrollY;
    h.press(ImGuiKey_Enter);
    expect(h.session.view().pendingEmpty && h.session.view().owners == before.owners && sameRect(first, h.ui.cards[0]) &&
        std::abs(h.ui.scrollY-scroll) < .1F, "empty prompt preserves membership, rectangles and scroll");
    h.focusButton("Cancel"); h.press(ImGuiKey_Enter);
    expect(!h.session.view().pendingEmpty && h.session.view().owners == before.owners && h.session.view().undoDepth == before.undoDepth,
        "actual Cancel preserves membership and history");
    h.focusButton("Empty bucket"); h.press(ImGuiKey_Enter);
    h.press(ImGuiKey_Escape);
    expect(!h.session.view().pendingEmpty && h.session.view().owners == before.owners, "Escape cancels the actual empty prompt");
    h.focusButton("Empty bucket"); h.press(ImGuiKey_Enter);
    h.focusButton("Confirm");
    const auto confirmFirst = h.ui.cards[0];
    const auto confirmScroll = h.ui.scrollY;
    h.press(ImGuiKey_Enter);
    expect(!h.session.view().pendingEmpty && h.session.view().counts[0] == 100 &&
        h.session.view().undoDepth == before.undoDepth + 1 && h.session.view().inventorySlots == before.inventorySlots &&
        sameRect(confirmFirst, h.ui.cards[0]) && std::abs(h.ui.scrollY-confirmScroll) < .1F,
        "actual Confirm empties once without shifting frozen inventory slots");
    h.click(h.ui.toolbar[sorterUndoControl]);
    expect(h.session.view().owners == before.owners && h.session.view().undoDepth == before.undoDepth &&
        sameRect(confirmFirst, h.ui.cards[0]) && std::abs(h.ui.scrollY-confirmScroll) < .1F,
        "actual Undo restores the entire bucket without shifting inventory");
  }
}
void mixedNotationAndRecovery() {
  for (const auto size : {ImVec2{1440,900}, ImVec2{800,600}, ImVec2{360,640}, ImVec2{360,480}}) {
    Harness h(size.x, size.y, SORTER_MIXED_FIXTURE);
    expect(h.ui.cardCount == 100, "mixed pack renders all original slots");
    ImGui::NewFrame();
    beginEquationSorterFrame(h.ui, h.session);
    drawEquationSorter(h.ui, h.session.view(), h.session.content(), h.session.activeSolve());
    ImGui::PushFont(nullptr, 19);
    for (const auto& e : h.session.content()) {
      const auto& bounds = h.ui.cards[e.homeIndex];
      const auto text = ImGui::CalcTextSize(e.text.c_str(), nullptr, false, bounds.width - 24);
      expect(text.x <= bounds.width - 24 + .1F && text.y + 32 <= bounds.height + .1F,
          "every mixed expression fits its production card with readable padding");
    }
    ImGui::PopFont();
    ImGui::Render();
    h.click(h.ui.toolbar[0]);
    for (const auto& e : h.session.content()) if (e.id >= 2000) {
      h.action(SorterActionKind::ActivateEquation, e.id);
      h.action(SorterActionKind::ActivateEquation, e.id);
    }
    h.click(h.ui.toolbar[0]);
    expect(h.session.view().inventory && h.ui.cardCount == 20, "new subject inventory opens through the actual bucket control");
    const auto first = h.ui.cards[0];
    h.click(first); h.click(first);
    if (h.session.view().counts[1] != 19 || !sameRect(first, h.ui.cards[0]))
      std::cerr << "mixed width=" << size.x << " height=" << size.y << " count=" << h.session.view().counts[1]
                << " inspected=" << h.session.view().inspected.value_or(0) << " rect=" << first.x << ',' << first.y << ',' << first.width << ',' << first.height
                << " after=" << h.ui.cards[0].x << ',' << h.ui.cards[0].y << ',' << h.ui.cards[0].width << ',' << h.ui.cards[0].height << '\n';
    expect(h.session.view().counts[1] == 19 && sameRect(first, h.ui.cards[0]), "mixed card pointer return leaves the same rectangle");
    h.click(h.ui.toolbar[sorterUndoControl]);
    expect(h.session.view().counts[1] == 20 && sameRect(first, h.ui.cards[0]), "mixed card pointer Undo restores its reserved rectangle");
  }
}
void autoSortAndHintControls() {
  for (const auto size : {ImVec2{1440,900}, ImVec2{800,600}, ImVec2{360,640}, ImVec2{360,480}}) {
    Harness h(size.x, size.y, SORTER_MIXED_FIXTURE);
    const auto initial = h.session.view();
    const auto first = h.ui.cards[0];
    for (const auto& control : h.ui.toolbar)
      expect(control.x >= 0 && control.x + control.width <= size.x && control.y + control.height < first.y,
          "all bucket, Undo, Auto sort and Hint controls fit above the grid");
    h.click(h.ui.toolbar[sorterHintControl]);
    expect(h.session.view().hintVisible && h.session.view().owners == initial.owners && !h.session.view().inspected,
        "actual Hint opens without changing or inspecting cards");
    expect(h.ui.hintClose.available && h.ui.hintClose.y + h.ui.hintClose.height <= size.y,
        "hint close remains accessible even with scrollable help on a narrow screen");
    h.press(ImGuiKey_Escape);
    expect(!h.session.view().hintVisible && sameRect(first, h.ui.cards[0]), "Escape closes hint without shifting the grid");
    h.click(h.ui.cards[0]);
    h.click(h.ui.toolbar[sorterHintControl]);
    expect(h.session.view().hint.find("x + 1 = -11") != std::string::npos && h.session.view().hint.find("Algebra") != std::string::npos,
        "inspected algebra card shows its subject and prepared hint");
    h.click(h.ui.hintClose);
    expect(!h.session.view().hintVisible && h.session.view().inspected == 1001, "Close hint preserves card inspection");
    h.click(h.ui.toolbar[sorterAutoSortControl]);
    expect(h.session.view().counts == std::array<std::size_t,7>{0,80,5,5,5,5,0} && h.session.view().undoDepth == 1 &&
        !h.session.view().inspected && sameRect(first, h.ui.cards[0]), "actual Auto sort groups all subjects once and preserves grid positions");
    h.click(h.ui.toolbar[sorterAutoSortControl]);
    expect(h.session.view().undoDepth == 1, "disabled Auto sort cannot create another transaction");
    h.click(h.ui.toolbar[sorterHintControl]);
    expect(h.session.view().hint.find("All grouped") != std::string::npos, "completed sorting still has a next-step hint");
    h.press(ImGuiKey_Escape);
    h.click(h.ui.toolbar[4]);
    expect(h.session.view().inventory && h.session.view().activeBucket == SorterBucket::E && h.ui.cardCount == 5,
        "actual E button opens the completed discrete group in one click");
    h.focusButton("Empty bucket"); h.press(ImGuiKey_Enter);
    h.click(h.ui.toolbar[sorterHintControl]);
    expect(h.session.view().hintVisible && h.session.view().hint.find("Confirm") != std::string::npos,
        "pending Empty has its own next-step explanation");
    h.press(ImGuiKey_Escape);
    expect(!h.session.view().hintVisible && h.session.view().pendingEmpty, "first Escape closes help and preserves the pending action");
    h.press(ImGuiKey_Escape);
    h.click(h.ui.toolbar[sorterUndoControl]);
    expect(h.session.view().owners == initial.owners && !h.session.view().undoDepth && h.session.view().inventory,
        "one actual Undo restores the entire automatic move from another view");
    expect(h.session.view().nextStep.find("Empty group") != std::string::npos, "empty inventory explains how to continue");
  }
  Harness h;
  h.focusButton("Auto sort");
  ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, true); h.frame(90);
  ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, false); h.frame(2);
  expect(h.session.view().counts[1] == 100 && h.session.view().undoDepth == 1 && !h.session.view().inventory,
      "held Enter auto-sorts the default algebra pack once without activating recovered focus");
  h.click(h.ui.toolbar[sorterHintControl]);
  ImGui::GetIO().DisplaySize = {360,480}; h.frame(3);
  expect(h.session.view().hintVisible && h.ui.hintClose.x >= 0 && h.ui.hintClose.x + h.ui.hintClose.width <= 360 &&
      h.ui.hintClose.y + h.ui.hintClose.height <= 480, "an open hint stays accessible after live window resizing");
  h.click(h.ui.hintClose);
  expect(!h.session.view().hintVisible, "resized hint closes through the real control");
}
void solveControlsAndTargets() {
  for(const auto size:{ImVec2{1440,860},ImVec2{1440,900},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_MIXED_FIXTURE,false);
    h.click(h.ui.toolbar[sorterAutoSortControl]);
    const auto groups=h.session.view();
    expect(h.ui.solveButton.available && h.ui.solveButton.y+h.ui.solveButton.height<h.ui.cards[0].y,
        "solver entry fits its reserved strip above the cards");
    h.click(h.ui.solveButton);
    auto* game=h.session.activeSolve();
    expect(game && game->view().step==1,"real Solve button opens the linked equation");
    const auto workspace=workspaceBounds(h.ui);
    for(std::size_t i=0;i<4;++i) {
      const auto b=h.ui.solveControls[i];
      expect(b.available && b.x>=0 && b.x+b.width<=size.x &&
          b.y>=0 && b.y+b.height<=h.ui.solveBoard.y,
          "the controls remain visible above the problem instead of separating it from the choices");
    }
    const auto clickOperation=[&](std::uint32_t id) {
      std::size_t index=0;
      while(index<h.ui.solveOptionCount && h.ui.solveOptionIds[index].value!=id)++index;
      expect(index<h.ui.solveOptionCount,"requested operation is rendered");
      auto& io=ImGui::GetIO();
      const auto area=h.ui.solveStage;
      io.AddMousePosEvent(area.x+area.width*.5F,area.y+area.height*.5F);h.frame();
      for(int scroll=0;scroll<30;++scroll) {
        const auto b=h.ui.solveOptions[index];
        if(contains(area,b)) {h.click(b);return;}
        io.AddMouseWheelEvent(0,b.y<area.y?1:-1);h.frame(3);
      }
      throw std::runtime_error("operation cannot be reached by scrolling");
    };
    clickOperation(108);
    expect(game->view().step==1 && game->view().wrongHits==1 && game->view().feedback==GalleryFeedback::Incorrect,
        "actual wrong operation remains playable");
    const auto options=h.ui.solveOptions;
    h.click(h.ui.solveControls[1]);fixedWorkspace(h,workspace);
    expect(!game->view().hint.empty() && h.ui.solveHelp.y>=h.ui.solveSupport.y && h.ui.solveHelp.y+20<size.y,
        "requesting Hint brings it into the fixed support area");
    for(std::size_t i=0;i<h.ui.solveOptionCount;++i)expect(sameRect(options[i],h.ui.solveOptions[i]),
        "opening help cannot move any operation choice");
    h.click(h.ui.solveControls[2]);
    expect(!game->view().nextMove.empty() && game->view().step==1,"Show next move reveals without submitting");
    fixedWorkspace(h,workspace);
    clickOperation(101);
    expect(game->view().step==2 && h.ui.shootAvailable && game->view().working=="x + 1 = 22 / (-2)",
        "operation button immediately opens the actual moving-target calculation");
    fixedWorkspace(h,workspace);
    const auto previousFocus=GImGui->NavId;
    h.press(ImGuiKey_Tab);
    expect(GImGui->NavId!=previousFocus,"Tab remains usable after the operation buttons disappear");
    const auto clickTarget=[&](std::uint32_t id) {
      const auto v=game->view();
      const auto row=std::find_if(v.answers.begin(),v.answers.begin()+v.choiceCount,[&](const auto& a){return a.binding.option.value==id;});
      expect(row!=v.answers.begin()+v.choiceCount,"calculation answer has a colour binding");
      const auto bodies=game->scene().objects();
      const auto body=std::find_if(bodies.begin(),bodies.end(),[&](const auto& b){return b.id==row->binding.object;});
      expect(body!=bodies.end(),"calculation answer has a sphere");
      const auto p=game->scene().project(body->position);const auto viewport=h.ui.shootViewport;
      const auto left=game->scene().project(body->position-iggy3d::Vec3{body->size.x*.5F,0,0});
      const auto right=game->scene().project(body->position+iggy3d::Vec3{body->size.x*.5F,0,0});
      if((right.x-left.x)*viewport.width<24 || left.x<=0 || right.x>=1 || p.y<=0 || p.y>=1)
        std::cerr<<"target size="<<size.x<<'x'<<size.y<<" viewport="<<viewport.width<<'x'<<viewport.height
                 <<" diameter="<<(right.x-left.x)*viewport.width<<" left="<<left.x<<" right="<<right.x<<" y="<<p.y<<'\n';
      expect((right.x-left.x)*viewport.width>=24 && left.x>0 && right.x<1 && p.y>0 && p.y<1,
          "prepared arithmetic targets remain visible and at least 24 pixels wide");
      for(std::size_t i=0;i<v.choiceCount;++i)expect(h.ui.solveAnswers[i].available && contains(h.ui.solveStage,h.ui.solveAnswers[i]),
          "each arithmetic answer label is inside the shared 3D activity area");
      h.click({0,viewport.x+p.x*viewport.width,viewport.y+p.y*viewport.height,0,0,true});
    };
    clickTarget(108);
    expect(game->view().step==2 && game->view().wrongHits==2 && h.ui.shootAvailable,
        "actual wrong sphere click neither advances nor blocks the question");
    clickTarget(101);
    expect(game->view().step==3 && game->view().working=="x + 1 = -11","actual correct sphere immediately opens the next operation");
    expect(game->question().currentRun().steps[2].attempts.empty(),
        "releasing a correct shot cannot select an operation that appeared under the pointer");
    fixedWorkspace(h,workspace);
    h.click(h.ui.solveControls[3]);
    if(game->view().step!=4 || !game->question().currentRun().steps[2].answerShown || !game->question().currentRun().steps[2].attempts.empty())
      std::cerr<<"assistance size="<<size.x<<'x'<<size.y<<" step="<<game->view().step
               <<" shown="<<game->question().currentRun().steps[2].answerShown<<" attempts="<<game->question().currentRun().steps[2].attempts.size()
               <<" button="<<h.ui.solveControls[3].x<<','<<h.ui.solveControls[3].y<<','<<h.ui.solveControls[3].width<<','<<h.ui.solveControls[3].height
               <<" hovered="<<(GImGui->HoveredWindow?GImGui->HoveredWindow->Name:"none")<<'\n';
    expect(game->view().step==4 && game->question().currentRun().steps[2].answerShown &&
        game->question().currentRun().steps[2].attempts.empty(),"Do this step advances as assistance");
    ImGui::GetIO().AddFocusEvent(false);h.frame(2);
    expect(game->view().paused && !h.ui.shootAvailable,"losing focus pauses the live gallery");
    ImGui::GetIO().AddFocusEvent(true);h.frame(2);
    h.click(h.ui.solveControls[1]);
    expect(!game->view().paused,"Resume restores the paused solving session");
    h.click(h.ui.solveControls[0]);
    expect(!h.session.activeSolve() && h.session.view().owners==groups.owners && h.session.view().undoDepth==groups.undoDepth,
        "Back to groups preserves the sorter layout and history");
    h.click(h.ui.solveButton);
    expect(h.session.activeSolve()==game && game->view().step==4,"Resume equation returns to the same decision");
    h.click(h.ui.solveControls[3]);
    expect(game->view().completed && h.ui.solveControls[4].available,"final assisted step offers explicit replay");
    fixedWorkspace(h,workspace);
    expect(h.ui.solveVerification.available && h.ui.solveVerification.y>=h.ui.solveSupport.y &&
        h.ui.solveVerification.y<size.y-20 && !game->view().verification.empty(),
        "the substitution check opens in the fixed support area on completion");
    if(h.ui.solveVerification.y+h.ui.solveVerification.height>size.y) {
      auto& io=ImGui::GetIO();io.AddMousePosEvent(h.ui.solveSupport.x+30,size.y-20);h.frame();
      for(int scroll=0;scroll<12 && h.ui.solveVerification.y+h.ui.solveVerification.height>size.y-8;++scroll) {
        io.AddMouseWheelEvent(0,-1);h.frame(3);
      }
      expect(h.ui.solveVerification.y+h.ui.solveVerification.height<=size.y-8,
          "the entire check is reachable by scrolling support without moving the problem or activity");
      fixedWorkspace(h,workspace);
    }
    h.click(h.ui.solveControls[4]);
    expect(game->view().step==1 && !game->view().completed && game->question().currentRun().runNumber==2,
        "real Play again button restarts the equation");
    h.press(ImGuiKey_Escape);
    expect(!h.session.activeSolve() && h.session.view().owners==groups.owners,"Escape returns safely to the same groups");
  }
}
void switchPreparedCards() {
  for(const auto size:{ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_BRACKET_FIXTURE,false);
    GallerySession* first=nullptr;
    for(const auto id:{3004U,3002U,3005U,3004U}) {
      const auto slot=std::find_if(h.ui.cards.begin(),h.ui.cards.begin()+h.ui.cardCount,
          [&](const auto& b){return b.equation==id;})-h.ui.cards.begin();
      expect(slot<static_cast<std::ptrdiff_t>(h.ui.cardCount),"prepared card has a stable visible-grid slot");
      auto& io=ImGui::GetIO();
      io.AddMousePosEvent(size.x/2,size.y-20);h.frame();
      for(int scroll=0;scroll<50;++scroll) {
        const auto b=h.ui.cards[slot];
        const float gridTop=h.ui.cards[0].y+h.ui.scrollY;
        if(b.y>=gridTop && b.y+b.height<size.y-10)break;
        io.AddMouseWheelEvent(0,b.y<gridTop?.15F:-.15F);h.frame(3);
      }
      const auto b=h.ui.cards[slot];
      if(b.y<h.ui.cards[0].y+h.ui.scrollY || b.y+b.height>=size.y)
        std::cerr<<"scroll size="<<size.x<<'x'<<size.y<<" requested="<<id<<" y="<<b.y<<" height="<<b.height
                 <<" grid_top="<<h.ui.cards[0].y+h.ui.scrollY<<" scroll="<<h.ui.scrollY<<'\n';
      expect(b.y>=h.ui.cards[0].y+h.ui.scrollY && b.y+b.height<size.y,"prepared card can be reached by real scrolling");
      h.click(b);h.click(h.ui.solveButton);
      auto* game=h.session.activeSolve();
      if(!game || game->view().equation!=h.session.content()[slot].text)
        std::cerr<<"size="<<size.x<<'x'<<size.y<<" requested="<<id<<" inspected="<<h.session.view().inspected.value_or(0)
                 <<" rectangle="<<b.y<<" scroll="<<h.ui.scrollY<<" active="<<(game?game->view().equation:"none")<<'\n';
      expect(game && game->view().equation==h.session.content()[slot].text,"real inspection and Solve open the chosen card");
      if(id==3004U && first) {
        expect(game==first && game->view().step==2 && game->question().currentRun().steps[0].answerShown,
            "real Resume returns to the earlier fractional question with its help evidence");
      } else {
        if(id==3004U)first=game;
        h.click(h.ui.solveControls[3]);
        expect(game->view().step==2 && h.ui.shootAvailable,"each prepared operation opens its arithmetic immediately");
      }
      h.click(h.ui.solveControls[0]);
      expect(!h.session.activeSolve() && game->view().paused,"real Back pauses the selected question");
    }
    // A queued command belongs to the old UI context even when two cards have
    // the same local challenge/frame numbers. Opening another card consumes it.
    h.ui.pending=GalleryCommand{GalleryHelp{first->view().challenge,GalleryHelpKind::DoStep}};
    h.ui.pending=SorterAction{SorterActionKind::OpenSolve,SorterBucket::A,3002,h.session.view().revision};
    h.ui.shootAvailable=true;h.ui.shootChallenge=first->view().challenge;
    h.frame();
    expect(h.session.activeSolve()->view().step==2 && !h.ui.pending,
        "queued help from another card cannot advance the newly opened question");
  }
}
void responsiveWorkspace() {
  Harness h(1440,860,SORTER_BRACKET_FIXTURE,false);
  h.click(h.ui.solveButton);
  auto* game=h.session.activeSolve();
  const auto challenge=game->view().challenge;
  const auto desktop=workspaceBounds(h.ui);
  const auto desktopChoice=h.ui.solveOptions[0];
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480},ImVec2{1920,1080},ImVec2{1440,860}}) {
    ImGui::GetIO().DisplaySize=size;h.frame(3);
    const SorterCardBounds window{0,0,0,size.x,size.y,true};
    for(const auto& panel:workspaceBounds(h.ui))expect(contains(window,panel),"resized solving panels remain inside the actual window");
    for(std::size_t i=0;i<h.ui.solveOptionCount;++i)
      expect(contains(h.ui.solveStage,h.ui.solveOptions[i]),"all four resized operation buttons remain visible without scrolling");
    const float contentBottom=h.ui.solveOptions[3].y+h.ui.solveOptions[3].height;
    if(size.x>=1400) {
      expect(contentBottom>size.y*.8F && h.ui.solveStage.height>size.y*.35F,
          "large windows use their lower area for actual answer controls instead of leaving a fixed-height strip at the top");
      expect(h.ui.solveBoard.height>=100 && h.ui.solveOptions[0].height>=150,
          "the problem and choices grow together on a desktop-sized window");
    }
    if(size.x==800)expect(h.ui.solveBoard.height<desktop[0].height && h.ui.solveOptions[0].height<desktopChoice.height,
        "shrinking the window reduces the content size as well as its surrounding panels");
    expect(game==h.session.activeSolve() && game->view().challenge==challenge && game->view().step==1 &&
        game->question().currentRun().steps[0].attempts.empty(),"resizing preserves the current question and does not submit an answer");
    fixedWorkspace(h,workspaceBounds(h.ui));
    std::cout<<"Workspace "<<size.x<<'x'<<size.y<<": problem y="<<h.ui.solveBoard.y
        <<" height="<<h.ui.solveBoard.height<<", activity height="<<h.ui.solveStage.height
        <<", last choice bottom="<<contentBottom<<'\n';
  }
  fixedWorkspace(h,desktop);
}
void continuousWorkspace() {
  for(const auto size:{ImVec2{1440,900},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_BRACKET_FIXTURE,false);
    h.click(h.ui.solveButton);
    auto* font=ImGui::GetIO().Fonts->Fonts[0];
    bool symbolsAvailable=true;
    for(const ImWchar glyph:{0x00F7,0x00D7,0x002D,0x002B})if(!font->IsGlyphInFont(glyph)) {
      std::cerr<<"missing symbol U+"<<std::hex<<glyph<<std::dec<<'\n';symbolsAvailable=false;
    }
    expect(symbolsAvailable,"the actual default font contains every displayed operation symbol");
    const auto original=workspaceBounds(h.ui);
    const auto camera=h.session.activeSolve()->scene().camera();
    for(std::size_t question=0;question<6;++question) {
      auto* game=h.session.activeSolve();
      expect(game && h.session.view().solveNumber==question+1 && !game->view().completed,
          "Next opens the next question inside the solving workspace");
      const auto equation=std::string(game->view().equation);
      expect(h.ui.solveOptions[0].y-(h.ui.solveWorking.y+h.ui.solveWorking.height)<=h.ui.solveWorking.height*1.2F,
          "operation choices stay close to the working as the whole workspace grows");
      expect(std::abs((h.ui.solveOptions[0].x+h.ui.solveOptions[1].x+h.ui.solveOptions[1].width)*.5F-
          (h.ui.solveBoard.x+h.ui.solveBoard.width*.5F))<1,"the compact choices are centred below the problem board");
      h.click(h.ui.solveControls[1]);
      while(!game->view().completed) {
        h.click(h.ui.solveControls[3]);fixedWorkspace(h,original);
        expect(game->view().equation==equation,"accepted steps keep the original problem unchanged");
        const auto current=game->scene().camera();
        expect(current.anchorPositionMeters.x==camera.anchorPositionMeters.x &&
            current.anchorPositionMeters.y==camera.anchorPositionMeters.y &&
            current.anchorPositionMeters.z==camera.anchorPositionMeters.z &&
            current.yawDegrees==camera.yawDegrees && current.pitchDegrees==camera.pitchDegrees,
            "operation, arithmetic, completion and Next preserve the camera framing");
      }
      h.frame(120);fixedWorkspace(h,original);
      expect(h.session.activeSolve()==game && game->view().completed && game->view().equation==equation,
          "the completed problem stays visible while the user waits");
      expect(h.ui.solveControls[4].available && h.ui.solveControls[5].available==(question<5),
          "Replay stays available and Next is disabled at the end of prepared content");
      expect(contains(h.ui.solveStage,h.ui.solveControls[4]) && contains(h.ui.solveStage,h.ui.solveControls[5]),
          "completion controls remain inside the same activity area");
      if(question==0) {
        const auto nextFocused=[] {return GImGui->NavWindow && GImGui->NavId==GImGui->NavWindow->GetID("Next problem");};
        for(int tabs=0;!nextFocused() && tabs<80;++tabs)h.press(ImGuiKey_Tab);
        expect(nextFocused(),"Next is reachable through keyboard navigation");
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter,true);h.frame(90);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter,false);h.frame(2);
        expect(h.session.view().solving && h.session.view().solveNumber==2 && h.session.activeSolve()->view().step==1,
            "held Enter on Next advances once and cannot activate a replacement control");
      } else if(question==1) {
        h.ui.pending=GalleryCommand{GalleryHelp{{1},GalleryHelpKind::DoStep}};
        h.ui.pending=SorterAction{SorterActionKind::NextSolve,SorterBucket::A,0,h.session.view().revision};
        h.frame(2);
        expect(h.session.activeSolve()->view().step==1 && !h.ui.pending,
            "Next consumes old queued help even when its local challenge matches the next question");
      } else h.click(h.ui.solveControls[5]);
      fixedWorkspace(h,original);
      if(question<5)expect(game->view().paused && game->view().completed,"Next preserves the previous completed owner");
      else expect(h.session.activeSolve()==game && game->view().completed,"clicking disabled Next cannot wrap the set");
    }
    h.click(h.ui.solveControls[0]);
    expect(h.session.view().solveCandidate==3005,"Back offers Resume for the last question reached with Next");
    h.click(h.ui.solveButton);fixedWorkspace(h,original);
    expect(h.session.activeSolve()->view().completed,"Resume preserves the finished working and check");
  }
}
void studySelectionControls() {
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_BRACKET_FIXTURE,false);
    const auto control=[&](StudyControl which)->const SorterCardBounds& {return h.ui.studyControls[static_cast<std::size_t>(which)];};
    h.click(h.ui.studyEntry);
    expect(h.session.view().studying && h.session.view().study.selectedCount==6,"Contents opens the authored chapter with six prepared questions");
    const SorterCardBounds window{0,0,0,size.x,size.y,true};
    for(const auto which:{StudyControl::All,StudyControl::Random,StudyControl::Specific,StudyControl::Groups,StudyControl::Resume,StudyControl::Start})
      expect(contains(window,control(which)) && control(which).height<=30,"compact selection controls stay within the window, including its footer");
    h.click(h.ui.studySubjects[0]);
    expect(!h.session.view().study.selectedCount && !control(StudyControl::Start).available,"clearing a subject disables Start");
    h.click(control(StudyControl::Start));expect(!h.session.activeSolve(),"disabled Start cannot open a question");
    h.click(h.ui.studyChapterChecks[0]);expect(h.session.view().study.selectedCount==6,"chapter checkbox selects its problem types");
    h.click(h.ui.studyTypes[0]);expect(!h.session.view().study.selectedCount,"type checkbox can remove its questions");
    h.click(h.ui.studyTypes[0]);
    h.click(control(StudyControl::Random));expect(h.session.view().study.selectedCount==3,"Random makes a three-question preview");
    auto& io=ImGui::GetIO();
    const auto countBox=control(StudyControl::Count);
    h.click({0,countBox.x+12,countBox.y+countBox.height*.5F,0,0,true});
    const auto shortcut=io.ConfigMacOSXBehaviors?ImGuiMod_Super:ImGuiMod_Ctrl;
    io.AddKeyEvent(shortcut,true);h.press(ImGuiKey_A);io.AddKeyEvent(shortcut,false);
    io.AddInputCharactersUTF8("2");h.frame(3);
    expect(h.session.view().study.randomCount==2 && h.session.view().study.selectedCount==2,"typing a random count changes the visible selection");
    h.click(control(StudyControl::Shuffle));h.frame(5);
    const auto random=h.session.view().study.selected;h.frame(60);
    expect(h.session.view().study.selected==random,"a random preview remains fixed while waiting");
    h.click(control(StudyControl::All));
    const auto clickQuestion=[&](std::size_t home) {
      const auto panel=h.ui.studyProblemPanel;
      io.AddMousePosEvent(panel.x+panel.width*.7F,panel.y+panel.height*.5F);h.frame();
      for(int scroll=0;scroll<30;++scroll) {
        const auto rect=h.ui.studyQuestions[home];
        if(contains(panel,rect)) {h.click(rect);return;}
        io.AddMouseWheelEvent(0,rect.y<panel.y?1:-1);h.frame(3);
      }
      throw std::runtime_error("study question is unreachable by scrolling");
    };
    for(const auto home:{1U,3U,5U})clickQuestion(home);
    expect(h.session.view().study.mode==StudyMode::Specific && h.session.view().study.selectedCount==3,
        "individual question checkboxes build a specific set even in a scrolled narrow panel");
    h.click(control(StudyControl::Start));
    expect(h.session.view().studyRun && h.session.view().solveCount==3,"Start opens the selected set through the solving workspace");
    const auto workspace=workspaceBounds(h.ui);auto* first=h.session.activeSolve();
    h.click(h.ui.solveControls[3]);h.click(h.ui.solveControls[0]);
    expect(h.session.view().studying && first->view().paused && control(StudyControl::Resume).available,
        "Back to contents pauses the question and offers Resume set");
    h.click(control(StudyControl::All));h.click(control(StudyControl::Resume));
    expect(h.session.activeSolve()==first && first->view().step==2 && h.session.view().solveCount==3,
        "Resume restores the same step and original set after changing the draft");
    for(std::size_t i=0;i<3;++i) {
      auto* game=h.session.activeSolve();
      expect(game->view().equation==h.session.content()[i*2].text && h.session.view().solveNumber==i+1,
          "actual Next follows only the selected questions");
      while(!game->view().completed) {h.click(h.ui.solveControls[3]);fixedWorkspace(h,workspace);}
      h.frame(20);
      expect(h.ui.solveControls[5].available==(i<2),"the last selected question disables Next");
      h.click(h.ui.solveControls[5]);
    }
    h.press(ImGuiKey_Escape);expect(h.session.view().studying,"Escape from a study question returns to contents");
    const auto startFocused=[] {return GImGui->NavWindow && GImGui->NavId==GImGui->NavWindow->GetID("Start set");};
    for(int tabs=0;!startFocused() && tabs<100;++tabs)h.press(ImGuiKey_Tab);
    expect(startFocused(),"Start set is reachable by keyboard across the selection panels");
    io.AddKeyEvent(ImGuiKey_Enter,true);h.frame(90);io.AddKeyEvent(ImGuiKey_Enter,false);h.frame(2);
    expect(h.session.view().solveCount==6 && h.session.activeSolve()->view().step==1 &&
        h.session.activeSolve()->question().currentRun().runNumber==2,"held Enter starts one fresh set without selecting an answer");
    h.press(ImGuiKey_Escape);h.click(control(StudyControl::Groups));
    expect(!h.session.view().studying && h.ui.cardCount==100,"Groups returns to the existing sorter");
  }
}
void coordinateBoardControls() {
  namespace fm=iggy3d::first_move;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);auto& io=ImGui::GetIO();
    h.action(SorterActionKind::OpenStudy);
    h.click(h.ui.studyChapterChecks[0]);h.click(h.ui.studyChapters[1]);h.click(h.ui.studyChapterChecks[1]);
    expect(h.session.view().study.selectedCount==4,"actual chapter controls select the four straight-line questions");
    h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Start)]);
    const auto workspace=workspaceBounds(h.ui);const auto board=h.ui.graphBoard;
    expect(h.session.activeSolve()->view().equation=="y = 2x + 1" && h.ui.graphStage==fm::GraphStage::Grid,
        "Start opens the first graph on an empty coordinate board");
    const auto fits=[&] {
      expect(h.ui.graphBoard.height>=140 && h.ui.solveBoard.height>=48,"compact graph headers retain a usable board even at 360 by 480");
      expect(contains(h.ui.solveStage,h.ui.graphBoard) && contains(h.ui.graphBoard,h.ui.graphPlot),"graph and plot fit the fixed activity area");
      expect(contains(h.ui.solveStage,h.ui.graphSlider) && contains(h.ui.solveStage,h.ui.graphReplay),"probe and motion controls stay within activity bounds");
      const auto& g=*h.session.activeSolve()->question().content().lineGraph;
      expect(std::abs(h.ui.graphPlot.width/(g.xMax-g.xMin)-h.ui.graphPlot.height/(g.yMax-g.yMin))<.001F,"axes use equal-sized units after fitting");
      for(std::size_t i=0;i<h.ui.solveOptionCount;++i)expect(contains(h.ui.solveStage,h.ui.solveOptions[i]),"all numerical choices fit beneath the graph");
      const auto* data=ImGui::GetDrawData();
      expect(data && data->TotalVtxCount>0,"native UI produces draw geometry without a capture");
      for(int list=0;list<data->CmdListsCount;++list)for(const auto& vertex:data->CmdLists[list]->VtxBuffer)
        expect(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y),"all rendered geometry is finite");
    };
    fits();
    const auto answer=[&](std::uint32_t id) {
      for(std::size_t i=0;i<h.ui.solveOptionCount;++i)if(h.ui.solveOptionIds[i].value==id) {h.click(h.ui.solveOptions[i]);return;}
      throw std::runtime_error("graph choice missing from presented buttons");
    };
    answer(108);
    expect(h.ui.graphStage==fm::GraphStage::Grid && h.session.activeSolve()->view().wrongHits==1,"wrong pointer answer leaves the grid blank");
    h.click(h.ui.solveControls[1]);fits();
    expect(h.ui.graphStage==fm::GraphStage::Grid && !h.ui.graphSlider.available,"hint cannot reveal the graph or unlock its probe");
    for(unsigned stage=1;stage<=4;++stage) {
      answer(101);fixedWorkspace(h,workspace);fits();
      expect(static_cast<unsigned>(h.ui.graphStage)==stage && sameRect(board,h.ui.graphBoard) && !h.ui.shootAvailable,
          "real graph choices reveal geometry in one stationary board with no shooting route");
      expect(h.ui.graphMotion<1,"the accepted step starts a short drawing animation");
      h.frame(30);expect(h.ui.graphMotion==1,"animation settles without delaying the next step");
      const auto hits=h.session.activeSolve()->view().correctHits;
      h.click(h.ui.graphReplay);
      expect(h.ui.graphMotion<1 && h.session.activeSolve()->view().correctHits==hits,"Replay repeats only the drawing, preserving answers");
      h.frame(30);
    }
    const auto completed=h.session.activeSolve()->view();
    expect(h.ui.graphSlider.available && h.ui.solveControls[5].available && h.ui.solveCompleteVisible,"completion retains graph and enables probe and explicit Next");
    h.click({0,h.ui.graphSlider.x+h.ui.graphSlider.width*.7F,h.ui.graphSlider.y,2,h.ui.graphSlider.height,true});
    expect(std::abs(h.ui.graphProbeX)>.1F,"actual slider input moves the coordinate probe");
    for(int tabs=0;tabs<80 && (!GImGui->NavWindow || GImGui->NavId!=GImGui->NavWindow->GetID("##graph_x"));++tabs)h.press(ImGuiKey_Tab);
    expect(GImGui->NavWindow && GImGui->NavId==GImGui->NavWindow->GetID("##graph_x"),"probe is reachable by keyboard");
    h.press(ImGuiKey_Space);
    const auto keyBefore=h.ui.graphProbeX;h.press(ImGuiKey_RightArrow);
    expect(h.ui.graphProbeX>keyBefore,"keyboard arrows adjust the same coordinate probe");
    h.press(ImGuiKey_Space);
    const auto before=h.ui.graphProbeX;
    io.AddMousePosEvent(h.ui.graphPlot.x+2,h.ui.graphPlot.y+h.ui.graphPlot.height*.5F);h.frame();
    io.AddMouseButtonEvent(0,true);h.frame(2);
    io.AddMousePosEvent(h.ui.graphPlot.x+h.ui.graphPlot.width-2,h.ui.graphPlot.y+h.ui.graphPlot.height*.5F);h.frame(2);
    io.AddMouseButtonEvent(0,false);h.frame(2);
    expect(h.ui.graphProbeX!=before,"dragging the plot moves the probe through the same projection");
    io.AddFocusEvent(false);h.frame(3);const auto pausedProbe=h.ui.graphProbeX;
    h.click(h.ui.graphBoard);
    expect(h.ui.graphProbeX==pausedProbe && h.session.activeSolve()->view().paused,"focus loss pauses and blocks probe input");
    io.AddFocusEvent(true);h.frame(3);h.click(h.ui.solveControls[1]);
    const auto probe=h.session.activeSolve()->question().coordinateGraph(h.ui.graphProbeX)->probe;
    expect(std::abs(probe.y-(2*probe.x+1))<.0001F,"displayed draggable point stays on y = 2x + 1");
    h.frame(100);
    expect(h.session.activeSolve()->view().correctHits==completed.correctHits && h.session.activeSolve()->view().wrongHits==completed.wrongHits &&
        h.session.view().solveNumber==1 && h.ui.graphStage==fm::GraphStage::Line,"exploration and elapsed time leave completed evidence and Next untouched");
    h.click(h.ui.solveControls[0]);h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Resume)]);
    expect(h.ui.graphStage==fm::GraphStage::Line && h.ui.graphProbeX==probe.x,"contents and Resume preserve the finished graph and probe");
    h.click(h.ui.solveControls[5]);
    expect(h.session.activeSolve()->view().equation=="y = -x + 2" && h.ui.graphStage==fm::GraphStage::Grid && !h.ui.graphSlider.available,
        "explicit Next clears graph reveals for the next selected question");
    for(int i=0;i<4;++i)h.click(h.ui.solveControls[3]);
    h.click(h.ui.solveControls[4]);
    expect(h.ui.graphStage==fm::GraphStage::Grid && h.session.activeSolve()->question().archivedRuns().size()==1,"Play again restarts geometry through the question owner");
    if(size.x==1440) {
      for(const auto resize:{ImVec2{360,480},ImVec2{800,600},ImVec2{1440,860}}) {
        io.DisplaySize=resize;h.frame(3);fits();
        expect(h.session.activeSolve()->view().equation=="y = -x + 2" && h.ui.graphStage==fm::GraphStage::Grid,"live resize retains the current graph question");
      }
    }
  }
}
void simultaneousBoardControls() {
  namespace fm=iggy3d::first_move;
  constexpr std::array stages{fm::GraphStage::Grid,fm::GraphStage::FirstLine,fm::GraphStage::BothLines,fm::GraphStage::Classified,fm::GraphStage::SystemSolution};
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);auto& io=ImGui::GetIO();
    h.action(SorterActionKind::OpenStudy);
    h.click(h.ui.studyChapterChecks[0]);h.click(h.ui.studyChapters[2]);h.click(h.ui.studyChapterChecks[2]);
    expect(h.session.view().study.selectedCount==4,"actual chapter controls select the four simultaneous-equation questions");
    h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Start)]);
    const auto workspace=workspaceBounds(h.ui);const auto board=h.ui.graphBoard;
    const auto fits=[&] {
      expect(contains(h.ui.solveBoard,h.ui.solveEquation),"both original equations fit inside the fixed gold board");
      expect(h.ui.graphBoard.height>=110 && contains(h.ui.solveStage,h.ui.graphBoard),"the two-equation header leaves a usable graph in the small window");
      expect(contains(h.ui.graphBoard,h.ui.graphPlot) && contains(h.ui.solveStage,h.ui.graphSlider),"shared axes and guide control remain inside the activity area");
      for(std::size_t i=0;i<h.ui.solveOptionCount;++i)expect(contains(h.ui.solveStage,h.ui.solveOptions[i]),"fractional pairs and infinity choices fit below the graph");
      const auto& graph=*h.session.activeSolve()->question().content().lineGraph;
      expect(std::abs(h.ui.graphPlot.width/(graph.xMax-graph.xMin)-h.ui.graphPlot.height/(graph.yMax-graph.yMin))<.001F,"both lines share equal axis scale");
      for(const auto* list:ImGui::GetDrawData()->CmdLists)for(const auto& vertex:list->VtxBuffer)
        expect(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y),"system and infinity strokes produce finite vertices without a capture");
    };
    const auto colourVertices=[&](ImU32 colour) {
      std::size_t count=0;
      for(const auto* list:ImGui::GetDrawData()->CmdLists)for(const auto& vertex:list->VtxBuffer)count+=vertex.col==colour;
      return count;
    };
    const auto answer=[&](std::uint32_t id) {
      for(std::size_t i=0;i<h.ui.solveOptionCount;++i)if(h.ui.solveOptionIds[i].value==id) {h.click(h.ui.solveOptions[i]);return;}
      throw std::runtime_error("system answer button missing");
    };
    for(std::size_t question=0;question<4;++question) {
      auto* game=h.session.activeSolve();const auto original=std::string(game->view().equation);
      expect(game->question().content().id==std::array{"sorter_system_integer","sorter_system_fraction","sorter_system_parallel","sorter_system_coincident"}[question],"Next follows the selected systems in order");
      for(std::size_t step=0;step<4;++step) {
        fits();fixedWorkspace(h,workspace);
        expect(sameRect(board,h.ui.graphBoard) && h.ui.graphStage==stages[step],"the same board remains through every system stage and question");
        expect(h.ui.graphSlider.available==(step>=2) && !h.ui.shootAvailable,"shared guide unlocks after both lines with no sphere input");
        if(!step)expect(!colourVertices(IM_COL32(62,199,211,255)) && !colourVertices(IM_COL32(186,142,255,255)),"the initial graph reveals neither line");
        if(step==1)expect(colourVertices(IM_COL32(62,199,211,255)) && !colourVertices(IM_COL32(186,142,255,255)),"only the first accepted line is drawn");
        if(step==2) {
          expect(h.ui.solveInfinityVisible && colourVertices(IM_COL32(186,142,255,255)),"both lines and the native infinity symbol are drawn at solution count");
          h.click({0,h.ui.graphSlider.x+h.ui.graphSlider.width*.65F,h.ui.graphSlider.y,2,h.ui.graphSlider.height,true});
          const auto graph=*game->question().coordinateGraph(h.ui.graphProbeX);
          expect(graph.probe.x==graph.second->probe.x && game->view().correctHits==2 && !graph.intersection,"moving the shared guide reveals values without submitting or marking the solution");
          h.click(h.ui.solveControls[0]);h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Resume)]);
          expect(h.session.activeSolve()==game && h.ui.graphProbeX==graph.probe.x && h.ui.graphStage==stages[step],"contents and Resume preserve an unfinished system and its guide");
        }
        answer(108);expect(h.ui.graphStage==stages[step],"wrong system clicks retain the current drawing");
        h.click(h.ui.solveControls[1]);expect(h.ui.graphStage==stages[step],"hints do not advance graph reveals");
        answer(101);expect(h.ui.graphStage==stages[step+1] && game->view().equation==original,"correct clicks advance once with both equations fixed");
        h.frame(30);const auto hits=game->view().correctHits;
        h.click(h.ui.graphReplay);expect(h.ui.graphMotion<1 && game->view().correctHits==hits,"replaying system motion preserves attempts");
        h.frame(30);
      }
      fits();expect(game->view().completed && h.ui.solveControls[5].available==(question<3),"last selected system disables Next and retains its result");
      const auto originalX=h.ui.graphProbeX;
      io.AddMousePosEvent(h.ui.graphPlot.x+1,h.ui.graphPlot.y+h.ui.graphPlot.height*.5F);h.frame();
      io.AddMouseButtonEvent(0,true);h.frame(2);
      io.AddMousePosEvent(h.ui.graphPlot.x+h.ui.graphPlot.width-1,h.ui.graphPlot.y+h.ui.graphPlot.height*.5F);h.frame(2);
      io.AddMouseButtonEvent(0,false);h.frame(2);
      expect(h.ui.graphProbeX!=originalX,"dragging moves the vertical guide after system completion");
      const auto projected=*game->question().coordinateGraph(h.ui.graphProbeX);
      expect(projected.probe.x==projected.second->probe.x,"dragged readouts retain exactly the same x");
      if(question==3)expect(projected.probe.y==projected.second->probe.y,"coincident lines retain equal readouts while both colours remain visible");
      h.frame(80);expect(h.session.activeSolve()==game && game->view().correctHits==4 && game->view().wrongHits==4,"exploration and elapsed time cannot change attempts or auto-advance");
      if(question<3)h.click(h.ui.solveControls[5]);
    }
    if(size.x==1440) {
      for(const auto resize:{ImVec2{360,480},ImVec2{800,600},ImVec2{1440,860}}) {
        io.DisplaySize=resize;h.frame(3);fits();expect(h.ui.graphStage==fm::GraphStage::SystemSolution,"live resize retains the system result");
      }
    }
    h.click(h.ui.solveControls[4]);
    expect(h.ui.graphStage==fm::GraphStage::Grid && !h.ui.graphSlider.available && h.session.activeSolve()->question().archivedRuns().size()==1,"Play again resets a completed system through the existing attempt owner");
  }
}
void linkedValueControls() {
  namespace fm=iggy3d::first_move;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);auto& io=ImGui::GetIO();
    for(const auto id:{4001U,4002U,4003U,4004U,5001U,5002U,5003U,5004U}) {
      h.action(SorterActionKind::OpenSolve,id);auto& game=*h.session.activeSolve();
      const auto workspace=workspaceBounds(h.ui);const auto original=std::string(game.view().equation);
      const auto fits=[&] {
        expect(contains(h.ui.graphBoard,h.ui.graphTable) && contains(h.ui.graphTable,h.ui.graphCurrentRow),"sample table and highlighted row fit inside the board");
        expect(h.ui.graphPlot.x+h.ui.graphPlot.width<h.ui.graphTable.x,"the table cannot cover the plotted graph");
        for(const auto& row:h.ui.graphSampleRows)expect(contains(h.ui.graphTable,row),"every sample button fits in the value table");
        const auto graph=*game.question().coordinateGraph(h.ui.graphProbeX);
        expect(h.ui.graphDisplayedValues.has_value()==graph.probeAvailable,"unreached numerical values are absent from the drawing");
        for(const auto& readout:h.ui.graphReadouts)if(readout.available) {
          expect(contains(h.ui.graphBoard,readout) && readout.y+readout.height<=h.ui.graphTable.y,"substitutions fit above both graph and table");
        }
        if(graph.probeAvailable) {
          const auto row=*h.ui.graphDisplayedValues;
          expect(row.x==graph.probe.x && row.y==graph.probe.y && (!row.secondY || *row.secondY==graph.second->probe.y),
              "drawn current values agree with the same-frame graph projection");
        }
        expect(game.view().equation==original,"exploration retains the fixed original equation");fixedWorkspace(h,workspace);
      };
      fits();h.click(h.ui.graphSampleRows[2]);
      expect(!h.ui.graphDisplayedValues && h.ui.graphProbeX==0,"unrevealed table rows cannot leak values or move the guide");
      while(!game.question().coordinateGraph()->probeAvailable) {
        const auto choice=std::find_if(h.ui.solveOptionIds.begin(),h.ui.solveOptionIds.begin()+h.ui.solveOptionCount,[](auto value){return value.value==101;});
        expect(choice!=h.ui.solveOptionIds.begin()+h.ui.solveOptionCount,"a prepared accepted graph choice is available");
        h.click(h.ui.solveOptions[choice-h.ui.solveOptionIds.begin()]);fits();
      }
      const auto before=game.view();const auto samples=*game.question().coordinateGraph()->valueTable;
      for(std::size_t i=0;i<samples.size();++i) {
        h.click(h.ui.graphSampleRows[i]);fits();
        expect(h.ui.graphProbeX==samples[i].x && h.ui.graphDisplayedValues->y==samples[i].y,"sample buttons set the exact shared x and listed y");
      }
      for(int tabs=0;tabs<80 && (!GImGui->NavWindow || GImGui->NavId!=GImGui->NavWindow->GetID("##graph_sample_0"));++tabs)h.press(ImGuiKey_Tab);
      expect(GImGui->NavWindow && GImGui->NavId==GImGui->NavWindow->GetID("##graph_sample_0"),"sample rows are keyboard reachable");
      h.press(ImGuiKey_Enter);expect(h.ui.graphProbeX==samples[0].x,"Enter selects the focused sample without submitting an answer");
      io.AddMousePosEvent(h.ui.graphSlider.x+h.ui.graphSlider.width*.61F,h.ui.graphSlider.y+8);h.frame();
      io.AddMouseButtonEvent(0,true);h.frame();fits();
      expect(h.ui.graphDisplayedValues->x==h.ui.graphProbeX && h.ui.graphProbeX!=samples[0].x,"slider input updates the numerical row and graph in the first input frame");
      io.AddMouseButtonEvent(0,false);h.frame(2);
      io.AddMousePosEvent(h.ui.graphPlot.x+h.ui.graphPlot.width*.4F,h.ui.graphPlot.y+h.ui.graphPlot.height*.5F);h.frame();
      io.AddMouseButtonEvent(0,true);h.frame();fits();io.AddMouseButtonEvent(0,false);h.frame(2);
      const auto x=h.ui.graphProbeX;h.click(h.ui.graphReplay);fits();expect(h.ui.graphProbeX==x,"Replay leaves the selected input unchanged");
      expect(game.view().challenge==before.challenge && game.view().correctHits==before.correctHits && game.view().wrongHits==before.wrongHits,
          "all linked value inputs preserve the current answer and attempt totals");
      h.click(h.ui.solveControls[0]);h.action(SorterActionKind::OpenSolve,id);fits();expect(h.ui.graphProbeX==x,"return and resume retain the linked input");
      if(id==5002) {
        io.AddFocusEvent(false);h.frame(3);h.click(h.ui.graphSampleRows[0]);fits();
        expect(game.view().paused && h.ui.graphProbeX==x,"focus loss blocks sample-row input");
        io.AddFocusEvent(true);h.frame(3);h.click(h.ui.solveControls[1]);
      }
      while(!game.view().completed)h.click(h.ui.solveControls[3]);
      h.click(h.ui.solveControls[4]);fits();
      expect(!h.ui.graphDisplayedValues && h.ui.graphProbeX==0,"fresh attempts clear numerical reveals and reset x");
      h.click(h.ui.solveControls[0]);
    }
  }
}
void mathematicalMoveControls() {
  namespace fm=iggy3d::first_move;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);auto& io=ImGui::GetIO();
    h.action(SorterActionKind::OpenStudy);
    h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Start)]);
    auto* game=h.session.activeSolve();expect(game && game->question().currentRun().math,"actual Start opens direct moves in the default pack");
    const auto initial=workspaceBounds(h.ui);
    const SorterCardBounds window{0,0,0,size.x,size.y,true};
    const auto stable=[&] {
      const auto current=workspaceBounds(h.ui);
      for(std::size_t i=0;i<current.size();++i)expect(sameRect(current[i],initial[i]),"mathematical steps keep the problem, controls and working area fixed");
      expect(contains(window,h.ui.solveEquation),"original problem fits the window");
      for(const auto& bounds:h.ui.mathOperations)if(bounds.available)expect(contains(h.ui.solveStage,bounds),"symbol moves fit the fixed activity");
      for(const auto& bounds:h.ui.mathResults)if(bounds.available)expect(contains(h.ui.solveStage,bounds),"result tiles fit the fixed activity");
      expect(!io.WantTextInput,"visual solving never requests the text keyboard");
      expect(h.ui.solveSupport.y+h.ui.solveSupport.height>=size.y-1,"blueprint uses the lower window");
    };
    const auto selectMove=[&](fm::MathOperation operation,const char* operand) {
      const auto found=std::find_if(h.ui.mathChoices.begin(),h.ui.mathChoices.end(),[&](const auto& c){return c.operation==operation && c.operand==operand;});
      expect(found!=h.ui.mathChoices.end(),"concrete move is offered without typing an operand");
      const auto index=static_cast<std::size_t>(found-h.ui.mathChoices.begin());
      h.click(h.ui.mathOperations[index]);
      expect(h.ui.mathSelectedMove==index,"symbol button selects the intended move");
      stable();
    };
    const auto resultIndex=[&](const char* result) {
      const auto parsed=fm::parseLinearEquation(result);expect(parsed.equation.has_value(),"expected result is valid");
      const auto& options=h.ui.mathChoices[*h.ui.mathSelectedMove].results;
      const auto found=std::find(options.begin(),options.end(),parsed.equation->display);
      expect(found!=options.end(),"expected mathematical result is available as a tile");
      return static_cast<std::size_t>(found-options.begin());
    };
    const auto submit=[&](fm::MathOperation operation,const char* operand,const char* result) {
      selectMove(operation,operand);h.click(h.ui.mathResults[resultIndex(result)]);stable();
    };
    expect(!h.ui.shootAvailable && h.ui.solveOptionCount==0 && !h.ui.mathUndo.available,"new workspace starts with symbol controls and no text fields");
    selectMove(fm::MathOperation::Divide,"-2");
    const auto retryChoices=h.ui.mathChoices[*h.ui.mathSelectedMove].results;
    for(const auto resized:{ImVec2{1440,860},ImVec2{360,480},ImVec2{800,600}}) {
      io.DisplaySize=resized;h.frame(4);
      const SorterCardBounds resizedWindow{0,0,0,resized.x,resized.y,true};
      for(const auto& bounds:h.ui.mathResults)expect(bounds.available && contains(resizedWindow,bounds),"all four active result tiles survive live resize");
      expect(h.ui.mathChoices[*h.ui.mathSelectedMove].results==retryChoices && game->question().currentRun().math->events.empty(),"resize retains the selected move and never submits an answer");
    }
    io.DisplaySize=size;h.frame(4);stable();
    h.click(h.ui.mathResults[resultIndex("x+1=22")]);
    expect(h.ui.mathChoices[*h.ui.mathSelectedMove].results==retryChoices,"retry keeps the result tiles in the same positions");
    expect(game->view().wrongHits==1 && game->question().currentRun().math->active==0,"clicking a one-sided division error leaves the equation in place");
    submit(fm::MathOperation::Divide,"-2","x+1=-11");
    expect(game->question().currentRun().math->nodes.size()==2 && game->view().correctHits==1,"clicking the correct result accepts exact division");
    const auto count=game->question().currentRun().math->events.size();
    io.AddKeyEvent(ImGuiKey_Enter,true);h.frame(90);io.AddKeyEvent(ImGuiKey_Enter,false);h.frame(2);
    expect(game->question().currentRun().math->events.size()==count,"held Enter never repeats a mathematical move");
    h.click(h.ui.solveControls[0]);expect(game->view().paused && h.session.view().studying,"Contents pauses and preserves checked working");
    h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Resume)]);
    expect(h.session.activeSolve()==game && game->question().currentRun().math->active==1,"Resume restores the same mathematical owner");
    stable();
    submit(fm::MathOperation::Subtract,"1","x=-12");
    expect(game->view().completed && h.ui.solveControls[5].available && h.ui.solveVerification.available,"completion reveals its check and explicit Next");
    h.frame(90);expect(h.session.activeSolve()==game && game->view().completed,"finished blueprint remains visible");
    h.click(h.ui.mathNodes[1]);
    expect(h.ui.mathInspected==1 && game->view().completed && h.ui.mathInspection.available,"earlier step expands without altering completion");
    h.click(h.ui.mathUndo);h.click(h.ui.mathUndo);stable();
    expect(!game->view().completed && game->question().currentRun().math->nodes.size()==3 && !h.ui.solveControls[5].available,"Undo retains the old route and closes Next");
    submit(fm::MathOperation::Expand,"","-2x-2=22");
    submit(fm::MathOperation::Add,"2","-2x=24");
    submit(fm::MathOperation::Divide,"-2","x=-12");
    expect(game->view().completed && game->question().currentRun().math->nodes.size()==6 && game->view().wrongHits==1,"second route is fully playable and keeps the original mistake");
    h.click(h.ui.solveControls[5]);
    expect(h.session.activeSolve()!=game && h.session.activeSolve()->view().equation=="3(x + 2) = 21","Next changes only on the explicit click");
    game=h.session.activeSolve();
    submit(fm::MathOperation::Expand,"","3x+6=21");
    const auto revision=game->question().currentRun().math->revision;
    io.AddFocusEvent(false);h.frame(3);
    expect(game->view().paused && std::none_of(h.ui.mathResults.begin(),h.ui.mathResults.end(),[](const auto& b){return b.available;}),"focus loss pauses and disables visual answers");
    io.AddFocusEvent(true);h.frame(3);h.click(h.ui.solveControls[1]);
    expect(!game->view().paused && game->question().currentRun().math->revision==revision,"Resume after focus loss preserves the active step");
    // Navigate to the actual result tile by its presented rectangle.
    selectMove(fm::MathOperation::Subtract,"6");
    const auto target=h.ui.mathResults[resultIndex("3x=15")];
    const auto focused=[&] {
      if(!GImGui->NavWindow || !GImGui->NavId)return false;
      const auto r=ImGui::WindowRectRelToAbs(GImGui->NavWindow,GImGui->NavWindow->NavRectRel[GImGui->NavLayer]);
      return std::abs(r.Min.x-target.x)<1 && std::abs(r.Min.y-target.y)<1;
    };
    for(int i=0;!focused() && i<80;++i)h.press(ImGuiKey_Tab);
    expect(focused(),"result tiles are reachable by Tab");h.press(ImGuiKey_Enter);
    expect(game->view().working=="3x = 15","keyboard activation checks the same visual result");
    submit(fm::MathOperation::Divide,"3","x=5");
    const auto previous=game->question().currentRun().math->events.size();
    h.click(h.ui.solveControls[4]);
    expect(!game->view().completed && game->question().currentRun().math->events.empty() &&
        game->question().archivedRuns().back().math->events.size()==previous,"Play again archives the checked route and resets move selection");
    stable();
    selectMove(fm::MathOperation::Expand,"");
    const auto check=h.ui.mathResults[resultIndex("3x+6=21")];
    io.AddMousePosEvent(check.x+check.width*.5F,check.y+check.height*.5F);h.frame();
    io.AddMouseButtonEvent(0,true);h.frame();io.AddMouseButtonEvent(0,false);h.frame();
    expect(h.ui.pending && std::holds_alternative<GalleryCommand>(*h.ui.pending),"result tile release queues the mathematical action");
    io.AddFocusEvent(false);h.frame(3);
    expect(!h.ui.pending && game->question().currentRun().math->events.empty(),"focus loss discards the queued mathematical action");
    io.AddFocusEvent(true);h.frame(3);h.click(h.ui.solveControls[1]);
    struct Remaining {std::uint32_t id;const char *factor,*divided,*amount,*answer;fm::MathOperation operation;};
    for(const auto& e:std::array{
        Remaining{3002,"4","x-3=2","3","x=5",fm::MathOperation::Add},
        Remaining{3003,"5","x+3=3","3","x=0",fm::MathOperation::Subtract},
        Remaining{3004,"4","x+1=5/2","1","x=3/2",fm::MathOperation::Subtract},
        Remaining{3005,"-6","x+2=-3/2","2","x=-7/2",fm::MathOperation::Subtract}}) {
      h.action(SorterActionKind::ReturnToSorter);h.action(SorterActionKind::OpenSolve,e.id);game=h.session.activeSolve();
      submit(fm::MathOperation::Divide,e.factor,e.divided);
      submit(e.operation,e.amount,e.answer);
      expect(game->view().completed && contains(window,h.ui.solveVerification),"signed, zero and fraction tiles finish with a contained verification");
    }
    const auto retained=game->question().currentRun().math->events.size();
    const auto working=std::string(game->view().working);
    for(const auto resized:{ImVec2{1024,768},ImVec2{360,480},ImVec2{800,600}}) {
      io.DisplaySize=resized;h.frame(4);
      const SorterCardBounds resizedWindow{0,0,0,resized.x,resized.y,true};
      expect(contains(resizedWindow,h.ui.solveStage) && contains(resizedWindow,h.ui.solveEquation) &&
          contains(resizedWindow,h.ui.solveVerification),"live resize keeps the mathematical workspace inside the window");
      expect(game->view().working==working && game->question().currentRun().math->events.size()==retained,"live resize preserves working and attempt evidence");
    }
    io.DisplaySize=size;h.frame(4);stable();
  }
}
void matrixMoveControls() {
  namespace fm=iggy3d::first_move;using Op=fm::MathOperation;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);auto& io=ImGui::GetIO();h.action(SorterActionKind::OpenStudy);
    const auto revealContents=[&](const auto& rectangle) {
      ImGuiWindow* panel=nullptr;
      for(auto* window:GImGui->Windows)if(std::string_view(window->Name).find("/Study contents_")!=std::string_view::npos)panel=window;
      expect(panel!=nullptr,"actual contents panel exists");
      for(int i=0;i<80;++i) {
        const auto b=rectangle();
        if(b.y>=panel->InnerRect.Min.y+2 && b.y+b.height<=panel->InnerRect.Max.y-2)return;
        io.AddMousePosEvent(panel->InnerRect.Min.x+25,panel->InnerRect.Min.y+20);h.frame();
        io.AddMouseWheelEvent(0,b.y<panel->InnerRect.Min.y?.25F:-.25F);h.frame(3);
      }
      expect(false,"linear algebra controls can be reached by scrolling the contents");
    };
    h.click(h.ui.studyTypes[0]);
    const auto subject=static_cast<std::size_t>(SorterSubject::LinearAlgebra);
    revealContents([&]{return h.ui.studySubjects[subject];});h.click(h.ui.studySubjects[subject]);
    expect(h.session.view().study.selectedCount==13,"real linear algebra checkbox includes the example and twelve new problems");
    const auto& types=h.session.view().study.types;
    const auto practice=std::find_if(types.begin(),types.end(),[](const auto& t){return t.chapter=="Matrix practice";});
    expect(practice!=types.end(),"new practice chapter is available through the existing contents controls");
    revealContents([&]{return h.ui.studyChapterChecks[practice->chapterId];});h.click(h.ui.studyChapterChecks[practice->chapterId]);
    expect(h.session.view().study.selectedCount==1,"deselecting the new chapter preserves the accepted single-example route");
    const auto found=std::find_if(types.begin(),types.end(),[](const auto& t){return t.subject==SorterSubject::LinearAlgebra;});
    const auto chapter=found->chapterId;
    revealContents([&]{return h.ui.studyChapters[chapter];});h.click(h.ui.studyChapters[chapter]);
    expect(h.ui.studyChapter==chapter && h.ui.studyTypes[chapter].available,"row reduction title opens through the real chapter control");
    h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Start)]);
    auto* game=h.session.activeSolve();expect(game && game->question().content().id=="sorter_matrix_rows","Start opens the chosen matrix in the shared workspace");
    const auto original=workspaceBounds(h.ui);
    const auto stable=[&] {
      const auto panels=workspaceBounds(h.ui);const SorterCardBounds window{0,0,0,size.x,size.y,true};
      for(std::size_t i=0;i<panels.size();++i)expect(sameRect(panels[i],original[i]) && contains(window,panels[i]),"matrix problem, working, tiles and history stay fixed and inside the window");
      expect(contains(h.ui.solveBoard,h.ui.solveEquation),"both original matrix rows fit the gold problem board");
      for(const auto& b:h.ui.mathOperations)if(b.available)expect(contains(h.ui.solveStage,b),"row-operation symbols fit the activity");
      for(const auto& b:h.ui.mathResults)if(b.available)expect(contains(h.ui.solveStage,b) && b.height>=40,"two-line matrix result tiles fit the activity");
      expect(!io.WantTextInput,"matrix solving does not request typing");
    };
    const auto select=[&](Op op,const char* operand) {
      const auto c=std::find_if(h.ui.mathChoices.begin(),h.ui.mathChoices.end(),[&](const auto& m){return m.operation==op && m.operand==operand;});
      expect(c!=h.ui.mathChoices.end(),"row move is offered as a symbol button");
      h.click(h.ui.mathOperations[static_cast<std::size_t>(c-h.ui.mathChoices.begin())]);stable();
    };
    const auto index=[&](const char* text) {
      const auto matrix=fm::parseAugmentedMatrix(text);expect(matrix.result.has_value(),"expected matrix parses");
      const auto& choices=h.ui.mathChoices[*h.ui.mathSelectedMove].results;
      const auto c=std::find(choices.begin(),choices.end(),matrix.result->display);
      expect(c!=choices.end(),"known matrix is available as a result tile");
      return static_cast<std::size_t>(c-choices.begin());
    };
    const auto submit=[&](Op op,const char* operand,const char* text) {
      select(op,operand);h.click(h.ui.mathResults[index(text)]);stable();
    };
    select(Op::SwapRows,"");
    const auto right=index("[1,-1|-1] [2,1|7]");
    h.click(h.ui.mathResults[(right+1)%4]);stable();
    expect(game->view().wrongHits==1 && game->question().currentRun().math->active==0,"wrong matrix click leaves original working active");
    h.click(h.ui.mathResults[right]);stable();
    submit(Op::AddRow1ToRow2,"-2","[1,-1|-1] [0,3|9]");
    h.click(h.ui.solveControls[0]);h.click(h.ui.studyControls[static_cast<std::size_t>(StudyControl::Resume)]);
    expect(h.session.activeSolve()==game && game->question().currentRun().math->active==2,"matrix resume restores the same working");
    submit(Op::DivideRow2,"3","[1,-1|-1] [0,1|3]");
    select(Op::AddRow2ToRow1,"1");
    const auto target=h.ui.mathResults[index("[1,0|2] [0,1|3]")];
    const auto focused=[&] {
      if(!GImGui->NavWindow || !GImGui->NavId)return false;
      const auto r=ImGui::WindowRectRelToAbs(GImGui->NavWindow,GImGui->NavWindow->NavRectRel[GImGui->NavLayer]);
      return std::abs(r.Min.x-target.x)<1 && std::abs(r.Min.y-target.y)<1;
    };
    for(int i=0;!focused() && i<80;++i)h.press(ImGuiKey_Tab);
    expect(focused(),"matrix result tiles are reachable by keyboard");
    io.AddKeyEvent(ImGuiKey_Enter,true);h.frame(90);io.AddKeyEvent(ImGuiKey_Enter,false);h.frame(2);stable();
    expect(game->view().completed && contains(h.ui.solveStage,h.ui.solveVerification) && !h.ui.solveControls[5].available,"checked matrix solution stays visible and a one-question set cannot advance elsewhere");
    expect(h.session.activeSolve()==game && game->question().currentRun().math->events.size()==5,"one held activation submits once and keeps the completed matrix open");
    for(int i=0;i<4;++i)h.click(h.ui.mathUndo);
    expect(!game->view().completed && game->question().currentRun().math->nodes.size()==5,"matrix Undo retains the first branch");
    select(Op::DivideRow1,"2");
    const auto retained=h.ui.mathChoices[*h.ui.mathSelectedMove].results;
    for(const auto resize:{ImVec2{360,480},ImVec2{800,600},ImVec2{1440,860}}) {
      io.DisplaySize=resize;h.frame(4);const SorterCardBounds window{0,0,0,resize.x,resize.y,true};
      for(const auto& b:h.ui.mathResults)expect(b.available && contains(window,b),"all matrix alternatives remain usable through live resizing");
      expect(h.ui.mathChoices[*h.ui.mathSelectedMove].results==retained,"resize retains the selected row move and choices");
    }
    io.DisplaySize=size;h.frame(4);stable();
    io.AddFocusEvent(false);h.frame(3);expect(game->view().paused,"focus loss pauses row operations");
    io.AddFocusEvent(true);h.frame(3);h.click(h.ui.solveControls[1]);
    submit(Op::DivideRow1,"2","[1,1/2|7/2] [1,-1|-1]");
    submit(Op::AddRow1ToRow2,"-1","[1,1/2|7/2] [0,-3/2|-9/2]");
    submit(Op::DivideRow2,"-3/2","[1,1/2|7/2] [0,1|3]");
    submit(Op::AddRow2ToRow1,"-1/2","[1,0|2] [0,1|3]");
    expect(game->view().completed && game->question().currentRun().math->nodes.size()==9,"fraction route and first route share the same completed blueprint");
    h.click(h.ui.solveControls[4]);
    expect(!game->view().completed && game->question().archivedRuns().back().math->nodes.size()==9,"Play again archives both matrix routes");
  }
}

void matrixChapterFractionControls() {
  namespace fm=iggy3d::first_move;using Op=fm::MathOperation;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);h.action(SorterActionKind::OpenSolve,6112);
    const auto original=workspaceBounds(h.ui);const SorterCardBounds window{0,0,0,size.x,size.y,true};
    auto* game=h.session.activeSolve();
    for(const auto& [operation,operand,expected]:std::array{
        std::tuple{Op::DivideRow1,"3/2","[1,-2/3|8/9] [2,3|1/3]"},
        std::tuple{Op::AddRow1ToRow2,"-2","[1,-2/3|8/9] [0,13/3|-13/9]"},
        std::tuple{Op::DivideRow2,"13/3","[1,-2/3|8/9] [0,1|-1/3]"},
        std::tuple{Op::AddRow2ToRow1,"2/3","[1,0|2/3] [0,1|-1/3]"}}) {
      const auto choice=std::find_if(h.ui.mathChoices.begin(),h.ui.mathChoices.end(),[&](const auto& c) {
        return c.operation==operation && c.operand==operand;
      });
      expect(choice!=h.ui.mathChoices.end(),"new fractional route offers its authored operation");
      const auto index=static_cast<std::size_t>(choice-h.ui.mathChoices.begin());h.click(h.ui.mathOperations[index]);
      expect(h.ui.mathSelectedMove==index,"new fractional operation opens through the actual button");
      const auto choices=h.ui.mathChoices[index].results;
      for(std::size_t i=0;i<choices.size();++i) {
        const auto& box=h.ui.mathResults[i];
        const auto text=ImGui::GetFont()->CalcTextSizeA(15,1e6F,0,choices[i].c_str());
        expect(box.available && contains(window,box) && contains(h.ui.solveStage,box) &&
            text.x+16<=box.width && text.y+6<=box.height,
            "fractional result labels fit their actual two-row buttons with padding");
      }
      const auto matrix=fm::parseAugmentedMatrix(expected);
      expect(matrix.result.has_value(),"independent fractional working parses");
      const auto result=std::find(choices.begin(),choices.end(),matrix.result->display);
      expect(result!=choices.end(),"new fractional result is selectable");
      h.click(h.ui.mathResults[static_cast<std::size_t>(result-choices.begin())]);
      expect(game->view().working==matrix.result->display,"actual result click advances to the expected exact fractional working");
      const auto panels=workspaceBounds(h.ui);
      for(std::size_t i=0;i<panels.size();++i)expect(sameRect(panels[i],original[i]) && contains(window,panels[i]),
          "new fraction problems retain the accepted fixed workspace at every tested size");
    }
    expect(game->view().completed && !game->view().verification.empty(),"new fractional final answer remains checked through the existing UI");
    h.frame(90);expect(game->view().completed && game->question().content().id=="sorter_matrix_practice_6112",
        "finished new problem stays until explicit Next");
  }
}
void matrixReferenceControls() {
  namespace fm=iggy3d::first_move;using Op=fm::MathOperation;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);auto& io=ImGui::GetIO();h.action(SorterActionKind::OpenSolve,6001);
    auto* game=h.session.activeSolve();const auto initial=workspaceBounds(h.ui);
    const auto evidence=[&] {
      const auto& run=*game->question().currentRun().math;
      return std::tuple{run.active,run.revision,run.nodes.size(),run.events.size(),std::string(game->view().working),game->view().correctHits,game->view().wrongHits,game->view().completed};
    };
    const auto untouched=evidence();
    const auto select=[&](Op op,const char* operand) {
      const auto found=std::find_if(h.ui.mathChoices.begin(),h.ui.mathChoices.end(),[&](const auto& c){return c.operation==op && c.operand==operand;});
      expect(found!=h.ui.mathChoices.end(),"reference setup move is offered");h.click(h.ui.mathOperations[static_cast<std::size_t>(found-h.ui.mathChoices.begin())]);
    };
    const auto fit=[&] {
      const SorterCardBounds window{0,0,0,io.DisplaySize.x,io.DisplaySize.y,true};
      expect(contains(window,h.ui.mathReferencePanel) && contains(h.ui.mathReferencePanel,h.ui.mathReferenceBody),"reference uses the existing support space with a contained scrollable body");
      for(const auto& control:h.ui.mathReferenceControls)expect(contains(h.ui.mathReferencePanel,control),"reference controls stay visible even in a short window");
      if(io.DisplaySize.x>=900 && io.DisplaySize.y>=800) {
        for(const auto& matrix:h.ui.mathReferenceMatrices)expect(contains(h.ui.mathReferenceBody,matrix),"both example matrices fit the wide reference body without clipping");
        expect(contains(h.ui.mathReferenceBody,h.ui.mathReferenceCalculation),"the current column calculation fits beside the example matrices");
      }
      for(const auto& result:h.ui.mathResults)if(result.available)expect(contains(h.ui.solveStage,result),"reference does not cover or displace answer choices");
      expect(!io.WantTextInput,"reference examples use buttons without typing");
    };
    select(Op::SwapRows,"");const auto selected=h.ui.mathSelectedMove;const auto tiles=h.ui.mathResults;
    const auto choices=h.ui.mathChoices[*selected].results;
    expect(h.ui.mathReferenceButton.available,"selected matrix move has a reference button");h.click(h.ui.mathReferenceButton);fit();
    expect(h.ui.mathReferenceId=="row_swap" && h.ui.mathReferenceStep==0,"question-mark opens the linked rule at the example's beginning");
    for(std::size_t step=1;step<=3;++step) {h.click(h.ui.mathReferenceControls[2]);fit();expect(h.ui.mathReferenceStep==step,"each next click reveals one example column");}
    expect(!h.ui.mathReferenceControls[2].available && h.ui.mathReferenceControls[1].available,"example ends after the right-hand column");
    h.click(h.ui.mathReferenceControls[1]);expect(h.ui.mathReferenceStep==2,"example can be stepped backward");
    expect(evidence()==untouched && h.ui.mathChoices[*selected].results==choices,"reference browsing preserves all mathematical evidence and result order");
    for(std::size_t i=0;i<tiles.size();++i)expect(sameRect(tiles[i],h.ui.mathResults[i]),"opening and stepping a reference retain exact result rectangles");
    const auto current=workspaceBounds(h.ui);for(std::size_t i=0;i<current.size();++i)expect(sameRect(current[i],initial[i]),"reference retains fixed problem, working, activity and history footprints");
    h.press(ImGuiKey_Escape);expect(h.ui.mathReferenceId.empty() && h.session.activeSolve()==game,"Escape closes only the reference");
    expect(h.ui.mathSelectedMove==selected && evidence()==untouched,"closing restores the selected move without an attempt");
    h.click(h.ui.mathReferenceButton);const auto target=h.ui.mathReferenceControls[2];
    const auto focused=[&] {
      if(!GImGui->NavWindow || !GImGui->NavId)return false;
      const auto rect=ImGui::WindowRectRelToAbs(GImGui->NavWindow,GImGui->NavWindow->NavRectRel[GImGui->NavLayer]);
      return std::abs(rect.Min.x-target.x)<1 && std::abs(rect.Min.y-target.y)<1;
    };
    for(int i=0;!focused() && i<100;++i)h.press(ImGuiKey_Tab);
    expect(focused(),"reference example controls are reachable by keyboard");
    io.AddKeyEvent(ImGuiKey_Enter,true);h.frame(90);io.AddKeyEvent(ImGuiKey_Enter,false);h.frame(2);
    expect(h.ui.mathReferenceStep==1 && evidence()==untouched,"held Enter advances the example once without submitting a result");
    h.press(ImGuiKey_Space);expect(h.ui.mathReferenceStep==2,"Space also advances the focused example");
    select(Op::DivideRow1,"2");fit();expect(h.ui.mathReferenceId=="row_scaling" && h.ui.mathReferenceStep==0,"changing operation family updates an open reference");
    h.click(h.ui.mathReferenceControls[2]);select(Op::AddRow1ToRow2,"-1/2");fit();
    expect(h.ui.mathReferenceId=="row_addition" && h.ui.mathReferenceStep==0,"row-addition variants share a separate reusable example");
    h.click(h.ui.mathReferenceControls[2]);
    for(const auto resized:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
      io.DisplaySize=resized;h.frame(4);fit();expect(h.ui.mathReferenceStep==1 && evidence()==untouched,"live resize preserves the example and actual problem");
    }
    io.DisplaySize=size;h.frame(4);fit();
    // The content body scrolls independently; its controls stay pinned above it.
    const auto controls=h.ui.mathReferenceControls;
    io.AddMousePosEvent(h.ui.mathReferenceBody.x+20,h.ui.mathReferenceBody.y+8);h.frame();io.AddMouseWheelEvent(0,-4);h.frame(4);fit();
    for(std::size_t i=0;i<controls.size();++i)expect(sameRect(controls[i],h.ui.mathReferenceControls[i]),"scrolling an explanation leaves its controls in place");
    io.AddFocusEvent(false);h.frame(3);const auto step=h.ui.mathReferenceStep;h.click(h.ui.mathReferenceControls[2]);
    expect(h.ui.mathReferenceStep==step && evidence()==untouched,"focus loss blocks reference controls and preserves player working");
    io.AddFocusEvent(true);h.frame(3);h.click(h.ui.solveControls[1]);
    h.click(h.ui.mathReferenceControls[0]);expect(h.ui.mathReferenceId.empty(),"Close returns to working inspection");
    select(Op::SwapRows,"");h.click(h.ui.mathReferenceButton);
    const auto submit=[&](Op op,const char* operand,const char* result) {
      select(op,operand);const auto expected=fm::parseAugmentedMatrix(result);const auto& options=h.ui.mathChoices[*h.ui.mathSelectedMove].results;
      const auto found=std::find(options.begin(),options.end(),expected.result->display);expect(found!=options.end(),"known actual result remains available beside the reference");
      h.click(h.ui.mathResults[static_cast<std::size_t>(found-options.begin())]);fit();
    };
    submit(Op::SwapRows,"","[1,-1|-1] [2,1|7]");submit(Op::AddRow1ToRow2,"-2","[1,-1|-1] [0,3|9]");
    submit(Op::DivideRow2,"3","[1,-1|-1] [0,1|3]");submit(Op::AddRow2ToRow1,"1","[1,0|2] [0,1|3]");
    expect(game->view().completed && game->question().currentRun().math->events.size()==4 && !h.ui.mathReferenceId.empty(),"four real answers complete the problem while reference activity never becomes an answer");
    h.frame(90);expect(game->view().completed && h.session.activeSolve()==game,"reference completion never advances the problem automatically");
    h.click(h.ui.mathUndo);expect(game->question().currentRun().math->nodes.size()==5 && !game->view().completed,"Undo keeps the completed branch while the reference stays open");
    h.click(h.ui.solveControls[0]);h.action(SorterActionKind::OpenSolve,6001);
    expect(!h.ui.mathReferenceId.empty() && game->question().currentRun().math->active==3,"return and resume preserve current working and its reference");
    h.click(h.ui.solveControls[0]);h.action(SorterActionKind::OpenSolve,3001);
    expect(h.ui.mathReferenceId.empty() && !h.ui.mathReferencePanel.available,"another question clears the previous reference");
    h.click(h.ui.solveControls[0]);h.action(SorterActionKind::OpenSolve,6001);select(Op::AddRow2ToRow1,"1");h.click(h.ui.mathReferenceButton);
    expect(game->dispatch(ReplayQuestion{true}).accepted,"explicit fresh attempt archives unfinished working");h.frame(3);
    expect(h.ui.mathReferenceId.empty() && game->question().currentRun().math->active==0 && !game->question().archivedRuns().empty(),"fresh runs reset reference presentation while retaining archived evidence");
  }
}

void restoredPracticeControls() {
  namespace fm=iggy3d::first_move;
  for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
    Harness h(size.x,size.y,SORTER_STUDY_FIXTURE);
    h.action(SorterActionKind::OpenStudy);h.action(SorterActionKind::StartStudy);
    auto* game=h.session.activeSolve();const auto& run=game->question().currentRun();
    expect(game->dispatch(MathematicalMove{game->view().challenge,{fm::MathMoveKind::Submit,fm::MathOperation::Divide,"-2","x+1=-11",
        run.runNumber,run.math->revision,run.questionId,run.contentVersion}}).accepted,"fixture records real mathematical progress");
    const std::string working(game->view().working);
    const auto saved=h.session.studyProgress();h.session.restoreStudyProgress(saved);h.ui={};
    h.ui.progressMessage="Saved practice ready. Resume set.";h.frame(3);
    expect(contains({0,0,0,size.x,52},h.ui.progressStatus),"save status fits the compact heading at all supported sizes");
    const auto resume=static_cast<std::size_t>(StudyControl::Resume);
    expect(h.ui.studyControls[resume].available,"restored set exposes the real Resume control");
    h.click(h.ui.studyControls[resume]);game=h.session.activeSolve();
    expect(game && game->view().working==working && game->question().currentRun().math->nodes.size()==2,
        "clicking Resume restores working without adding an attempt");
    const auto choice=std::find_if(h.ui.mathChoices.begin(),h.ui.mathChoices.end(),[](const auto& c){return c.operation==fm::MathOperation::Subtract && c.operand=="1";});
    expect(choice!=h.ui.mathChoices.end(),"restored symbolic move is available");
    const auto index=static_cast<std::size_t>(choice-h.ui.mathChoices.begin());
    const auto expected=fm::parseLinearEquation("x=-12").equation->display;
    const auto tile=std::find(choice->results.begin(),choice->results.end(),expected);
    expect(tile!=choice->results.end(),"restored result tile is available");const auto result=static_cast<std::size_t>(tile-choice->results.begin());
    h.click(h.ui.mathOperations[index]);h.click(h.ui.mathResults[result]);
    expect(game->view().completed,"restored problem finishes through the existing visual controls");
    h.click(h.ui.solveControls[0]);h.click(h.ui.studyControls[resume]);
    expect(h.session.activeSolve()->view().completed && h.session.view().solveNumber==1,"Resume keeps a finished problem until Next");
    h.click(h.ui.solveControls[0]);h.ui.progressFailed=true;h.ui.progressMessage="Practice could not save: test failure";h.frame(3);
    expect(contains({0,0,0,size.x,52},h.ui.progressStatus) && h.ui.studyControls[resume].available,"save errors remain readable and do not disable Resume");
  }
}

int main() {
  try { pointer(); keyboardAndFocus(); layoutAndScroll(); inventoryNavigationAndEmpty(); mixedNotationAndRecovery(); autoSortAndHintControls(); solveControlsAndTargets(); switchPreparedCards(); responsiveWorkspace(); continuousWorkspace(); studySelectionControls(); coordinateBoardControls(); simultaneousBoardControls(); linkedValueControls(); mathematicalMoveControls(); matrixMoveControls(); matrixChapterFractionControls(); matrixReferenceControls(); restoredPracticeControls(); std::cout << "Actual visual moves, saved practice Resume, shared references, example navigation, both solution routes, Undo, inspection, keyboard, focus, graphs, fixed workspace and explicit Next passed\n"; }
  catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
