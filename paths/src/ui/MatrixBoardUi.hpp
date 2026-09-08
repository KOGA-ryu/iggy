#pragma once
#include "runtime/matrix_board/MatrixBoard.hpp"
namespace paths {
struct MatrixBoardUiState { int row=1,other=2;double real=1,imaginary=0;int selectedRow=-1,selectedColumn=-1;std::string message;bool hasPending=false;BoardAction pending{BoardActionKind::Reset}; };
void applyMatrixBoardPending(MatrixBoard&,MatrixBoardUiState&);
void drawMatrixBoardGrid(const char*,const BoardMatrix&,const MatrixBoardView&,MatrixBoardUiState&,bool given=false);
void drawMatrixBoardContents(MatrixBoard&,MatrixBoardUiState&,bool lockCard=false);
void drawMatrixBoard(MatrixBoard&,MatrixBoardUiState&,bool& active,const char* returnLabel="Back to 3D objects");
}
