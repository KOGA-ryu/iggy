#pragma once
#include "ui/MathObjectLayout.hpp"
namespace paths {
struct MathControlEdit { bool changed=false,reset=false;unsigned component=0;double value=0; };
// Shared native adapter only. Callers retain the mathematical action owner.
MathControlEdit compactMathControl(const MathParameterSpec&,std::string_view label,double value,MathControlRange,bool allowReset=false);
MathControlEdit compactMathTuple(const MathControlRow&,const std::array<double,3>&,const std::array<MathControlRange,3>&);
} // namespace paths
