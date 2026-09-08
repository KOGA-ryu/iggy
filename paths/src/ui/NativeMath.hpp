#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace paths {
// Own this between ImGui context creation and destruction, on the UI thread.
// Layouts retain glyph identities and transforms; atlas UVs are resolved at draw time.
class NativeMath {
  struct Geometry;
  struct Impl;
public:
  struct Equation {
    float width=0, height=0, baseline=0;
    std::string error;
  private:
    friend class NativeMath;
    std::shared_ptr<Geometry> geometry;
  };
  struct Document {
    struct Part {
      enum class Kind {Text,InlineMath,DisplayMath,Source} kind;
      std::size_t begin,end; // Exact, contiguous byte ranges in source, including delimiters.
      Equation equation;
    };
    struct Placement {
      std::size_t part,begin,end;
      float x,y,width,height,baseline;
    };
    std::string source;
    std::vector<Part> parts;
    std::vector<Placement> placements;
    float width=0,height=0;
    std::size_t equations=0,fallbacks=0;
  };
  explicit NativeMath(const std::filesystem::path& resources);
  ~NativeMath();
  NativeMath(const NativeMath&)=delete;
  NativeMath& operator=(const NativeMath&)=delete;
  // The reference remains valid until this bounded cache evicts the equation.
  const Equation& layout(std::string_view latex,float pixels,bool display=true);
  void draw(const Equation& equation,float x,float y,std::uint32_t colour) const;
  // One visible document is cached by source, wrapping width and current prose font.
  // Formula geometry is shared by value, so equation-cache eviction cannot invalidate it.
  const Document& layoutDocument(std::string_view source,float wrapWidth);
  void draw(const Document& document,float x,float y,std::uint32_t prose,std::uint32_t math,std::uint32_t source) const;
private:
  std::unique_ptr<Impl> impl_;
};
} // namespace paths
