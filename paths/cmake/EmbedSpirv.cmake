file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" length)
math(EXPR remainder "${length} % 8")
if(remainder OR length LESS 40)
  message(FATAL_ERROR "Invalid SPIR-V length: ${INPUT}")
endif()
# SPIR-V compiler output is little endian. Emit numeric words, not a path to
# build artifacts, so copying the executable does not lose its scene shaders.
string(REGEX REPLACE
  "([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])"
  "0x\\4\\3\\2\\1U," words "${hex}")
file(WRITE "${OUTPUT}" "#pragma once\n#include <cstdint>\ninline constexpr std::uint32_t ${NAME}[] = {${words}};\n")
