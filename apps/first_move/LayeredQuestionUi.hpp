#pragma once

namespace iggy3d::first_move {

class HuntSession;
class LayeredQuestionSession;
struct FirstMoveUiState;

// Draws Guided Question content inside the First Move root window.
void drawLayeredQuestionUi(FirstMoveUiState& ui,
                           HuntSession& hunt,
                           LayeredQuestionSession& guided,
                           float scale);

}  // namespace iggy3d::first_move
