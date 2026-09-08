# MicroTeX in Paths

Source: https://github.com/NanoMichael/MicroTeX
Revision: `0e3707f6dafebb121d98b53c64364d16fefe481d`.

Paths vendors the core C++ headers/sources and complete resource directory.
Upstream GUI backends, demos, tests and build logic are omitted. The local
CMake file builds the core with pinned TinyXML2 10.0.0. `NativeMath.cpp`
implements the font, text-layout and drawing interfaces using the existing
ImGui atlas and draw list. No Qt, GTK, browser or system TeX installation is used.

`src/latex.cpp` has two local lifetime corrections: free the `asprintf` buffer
with `free`, and null the two deleted context pointers during release.
Every original selected-file hash, both archive hashes and dependency revisions
are in `docs/MIGRATION_SEED_MANIFEST.json`; its local patch record identifies the
changed file. All other upstream files and font bytes are unchanged.

The core license is `LICENSE`. Resource-specific licenses remain in
`res/fonts/licences/`, `res/greek/LICENSE`, and `res/cyrillic/LICENSE`.
Consult the actual bundled resource license files for their terms.
